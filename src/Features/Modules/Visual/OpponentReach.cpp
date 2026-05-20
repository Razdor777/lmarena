#include "OpponentReach.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Network/Packets/ActorEventPacket.hpp>
#include <SDK/Minecraft/Network/PacketID.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <tuple>

// ============================================================
// Correct AABB-based reach: eye → closest hitbox point
// ============================================================
float OpponentReach::calculateReachToAABB(Actor* attacker, Actor* target)
{
    if (!attacker || !target) return 0.f;

    auto attackerPos = attacker->getPos();
    if (!attackerPos) return 0.f;

    // Use real target AABB from engine instead of hardcoded offsets.
    const auto aabb = target->getAABB(true);
    const glm::vec3 aabbMin = aabb.mMin;
    const glm::vec3 aabbMax = aabb.mMax;

    auto distFrom = [&](const glm::vec3& from) -> float {
        glm::vec3 closest;
        closest.x = std::clamp(from.x, aabbMin.x, aabbMax.x);
        closest.y = std::clamp(from.y, aabbMin.y, aabbMax.y);
        closest.z = std::clamp(from.z, aabbMin.z, aabbMax.z);
        return glm::distance(from, closest);
    };

    // Depending on mapping/version, getPos() may be feet-ish or eye-ish.
    // Use the smaller physically plausible value to avoid systemic overestimation.
    const glm::vec3 base = *attackerPos;
    const float dBase = distFrom(base);
    const float dEye  = distFrom(base + glm::vec3(0.f, 1.62f, 0.f));
    return std::min(dBase, dEye);
}

float OpponentReach::getLookAngleDeg(Actor* attacker, Actor* target)
{
    if (!attacker || !target) return 180.f;

    auto attackerPos = attacker->getPos();
    auto targetPos = target->getPos();
    if (!attackerPos || !targetPos) return 180.f;

    glm::vec3 delta = *targetPos - *attackerPos;
    const float len = glm::length(delta);
    if (len < 0.0001f) return 0.f;
    glm::vec3 toTarget = delta / len;

    auto rot = attacker->getActorRotationComponent();
    if (!rot) return 0.f;

    float yaw = glm::radians(rot->mYaw);
    float pitch = glm::radians(rot->mPitch);

    glm::vec3 lookDir = glm::normalize(glm::vec3(
        -sin(yaw) * cos(pitch),
        -sin(pitch),
        cos(yaw) * cos(pitch)
    ));

    float dot = glm::dot(lookDir, toTarget);
    return glm::degrees(acos(std::clamp(dot, -1.f, 1.f)));
}

bool OpponentReach::isLookingAt(Actor* attacker, Actor* target, float maxAngleDeg)
{
    return getLookAngleDeg(attacker, target) <= maxAngleDeg;
}

bool OpponentReach::isOnSameTeam(Actor* player, Actor* other)
{
    if (!player || !other) return false;

    std::string playerName = player->getNameTag();
    std::string otherName = other->getNameTag();

    if (playerName.length() >= 4 && otherName.length() >= 4) {
        if (playerName[0] == '\xC2' && otherName[0] == '\xC2') {
            return playerName.substr(0, 4) == otherName.substr(0, 4);
        }
    }
    return false;
}

void OpponentReach::onEnable() {
    mOpponentReach = 0.f;
    mHitCount = 0;
    mAttackerName.clear();
    mLastHit = std::chrono::high_resolution_clock::now();
    mLastAttackSignal = mLastHit;
    mLastAttackerRuntimeId = -1;
    gFeatureManager->mDispatcher->listen<PacketInEvent, &OpponentReach::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &OpponentReach::onRenderEvent>(this);
}

void OpponentReach::onDisable() {
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &OpponentReach::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &OpponentReach::onRenderEvent>(this);
}

void OpponentReach::onPacketInEvent(PacketInEvent& event)
{
    if (!event.mPacket) return;
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;
    auto level = player->getLevel();
    if (!level) return;

    if (event.mPacket->getId() != PacketID::ActorEvent) return;

    auto packet = event.getPacket<ActorEventPacket>();
    if (!packet) return;

    // Cache likely attacker candidates slightly before HURT arrives.
    if (packet->mEvent == ActorEvent::START_ATTACKING && packet->mRuntimeID != player->getRuntimeID()) {
        for (Actor* actor : level->getRuntimeActorList()) {
            if (!actor || !actor->isValid() || !actor->isPlayer()) continue;
            if (actor->getRuntimeID() != packet->mRuntimeID) continue;

            float reach = calculateReachToAABB(actor, player);
            float angle = getLookAngleDeg(actor, player);
            if (reach > 0.0f && reach <= 4.8f && angle <= 45.f) {
                mLastAttackerRuntimeId = actor->getRuntimeID();
                mLastAttackSignal = std::chrono::high_resolution_clock::now();
            }
            break;
        }
        return;
    }

    if (packet->mEvent != ActorEvent::HURT) return;
    if (packet->mRuntimeID != player->getRuntimeID()) return;

    auto actors = level->getRuntimeActorList();

    std::vector<std::tuple<Actor*, float, float>> candidates; // actor, reach, angle
    for (Actor* actor : actors)
    {
        if (!actor || !actor->isValid() || !actor->isPlayer()) continue;
        if (actor == player) continue;
        if (mExcludeTeam.mValue && isOnSameTeam(player, actor)) continue;

        float reach = calculateReachToAABB(actor, player);
        if (reach <= 0.f || reach > 4.2f) continue;
        float angle = getLookAngleDeg(actor, player);
        if (angle > 35.f) continue;

        candidates.push_back({ actor, reach, angle });
    }

    if (candidates.empty()) return;

    std::sort(candidates.begin(), candidates.end(),
        [this](const auto& a, const auto& b) {
            const auto now = std::chrono::high_resolution_clock::now();
            const bool hasFreshAttackSignal =
                mLastAttackerRuntimeId != -1 &&
                std::chrono::duration<double>(now - mLastAttackSignal).count() <= 0.45;

            const bool aMatches = hasFreshAttackSignal &&
                (std::get<0>(a)->getRuntimeID() == mLastAttackerRuntimeId);
            const bool bMatches = hasFreshAttackSignal &&
                (std::get<0>(b)->getRuntimeID() == mLastAttackerRuntimeId);

            const float bonusA = aMatches ? -0.35f : 0.f;
            const float bonusB = bMatches ? -0.35f : 0.f;

            const float scoreA = std::get<1>(a) + std::get<2>(a) * 0.020f + bonusA;
            const float scoreB = std::get<1>(b) + std::get<2>(b) * 0.020f + bonusB;
            return scoreA < scoreB;
        });

    Actor* likelyAttacker = std::get<0>(candidates[0]);
    float reach = std::get<1>(candidates[0]);

    if (reach >= 0.5f && reach <= 6.5f) {
        mOpponentReach = reach;
        mAttackerName = likelyAttacker->getNameTag();
        mLastHit = std::chrono::high_resolution_clock::now();
        mHitCount++;
    }
}

void OpponentReach::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - mLastHit;
    bool expired = duration.count() >= mResetTime.mValue;

    bool sampleMode = mElement && mElement->mSampleMode;
    if (expired && !sampleMode && !mDebug.mValue) {
        mOpponentReach = 0.f;
        return;
    }

    if (expired) mOpponentReach = 0.f;

    auto drawList = ImGui::GetBackgroundDrawList();

    ImVec2 hudPos = mElement ? mElement->getPos() : ImVec2(ImGui::GetIO().DisplaySize.x / 2.f, 130.f);

    // ================= GLASS CARD =================
    float cardW = 200.f;
    float cardH = mDebug.mValue ? 85.f : 52.f;
    ImVec2 cardTL = { hudPos.x - cardW / 2, hudPos.y };
    ImVec2 cardBR = { hudPos.x + cardW / 2, hudPos.y + cardH };

    // Background
    drawList->AddRectFilled(cardTL, cardBR, IM_COL32(8, 12, 22, 180), 8.f);
    drawList->AddRect(cardTL, cardBR, IM_COL32(40, 100, 180, 80), 8.f, 0, 1.f);

    // Top accent — red/orange gradient for "danger" theme
    ImColor accentL = ImColor(200, 60, 60, 200);
    ImColor accentR = ImColor(255, 140, 40, 200);
    drawList->AddRectFilledMultiColor(
        { cardTL.x + 8, cardTL.y },
        { cardBR.x - 8, cardTL.y + 2 },
        accentL, accentR, accentR, accentL);

    float displayVal = sampleMode ? 3.12f : mOpponentReach;
    float opacity = sampleMode ? 0.5f : 1.f;
    std::string displayName = sampleMode ? "Player123" : mAttackerName;

    FontHelper::pushPrefFont(true, true);

    // ================= ATTACKER NAME =================
    {
        std::string nameText = displayName.empty() ? "---" : displayName;
        float nameFS = 11.f;
        ImVec2 nameSize = ImGui::GetFont()->CalcTextSizeA(nameFS, FLT_MAX, -1.f, nameText.c_str());
        float nameX = cardTL.x + 12;
        float nameY = cardTL.y + 5;

        drawList->AddText(ImGui::GetFont(), nameFS,
            { nameX, nameY }, IM_COL32(180, 180, 200, (int)(180 * opacity)), nameText.c_str());

        // Label
        std::string label = "OPPONENT";
        ImVec2 labelSize = ImGui::GetFont()->CalcTextSizeA(nameFS, FLT_MAX, -1.f, label.c_str());
        drawList->AddText(ImGui::GetFont(), nameFS,
            { cardBR.x - labelSize.x - 12, nameY },
            IM_COL32(200, 100, 80, (int)(180 * opacity)), label.c_str());
    }

    // ================= REACH VALUE =================
    {
        std::string valText = fmt::format("{:.2f}", displayVal);
        float fontSize = 20.f;

        ImColor valColor = ImColor(255, 255, 255, (int)(255 * opacity));
        if (displayVal > 0.f) {
            if (displayVal >= 4.0f) valColor = ImColor(255, 50, 50, (int)(255 * opacity));      // Cheater
            else if (displayVal >= 3.5f) valColor = ImColor(255, 100, 40, (int)(255 * opacity));  // Sus
            else if (displayVal >= 3.0f) valColor = ImColor(255, 200, 60, (int)(255 * opacity));  // Borderline
            else valColor = ImColor(80, 255, 120, (int)(255 * opacity));                         // Legit
        }

        float valX = cardTL.x + 12;
        float valY = cardTL.y + 18;

        // Shadow
        drawList->AddText(ImGui::GetFont(), fontSize,
            { valX + 1, valY + 1 }, IM_COL32(0, 0, 0, (int)(150 * opacity)), valText.c_str());
        drawList->AddText(ImGui::GetFont(), fontSize,
            { valX, valY }, valColor, valText.c_str());

        // "blocks" label
        ImVec2 valSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, -1.f, valText.c_str());
        drawList->AddText(ImGui::GetFont(), 10.f,
            { valX + valSize.x + 4, valY + fontSize - 12 },
            IM_COL32(140, 160, 190, (int)(150 * opacity)), "blocks");

        // Hit counter badge on right
        if (mHitCount > 0 || sampleMode) {
            std::string hits = "x" + std::to_string(sampleMode ? 3 : mHitCount);
            ImVec2 hitsSize = ImGui::GetFont()->CalcTextSizeA(11.f, FLT_MAX, -1.f, hits.c_str());
            float hitsX = cardBR.x - hitsSize.x - 12;
            float hitsY = valY + 4;

            // Badge background
            drawList->AddRectFilled(
                { hitsX - 4, hitsY - 1 },
                { hitsX + hitsSize.x + 4, hitsY + hitsSize.y + 1 },
                IM_COL32(255, 60, 60, (int)(40 * opacity)), 4.f);
            drawList->AddText(ImGui::GetFont(), 11.f,
                { hitsX, hitsY }, IM_COL32(255, 150, 150, (int)(200 * opacity)), hits.c_str());
        }
    }

    // ================= REACH BAR =================
    {
        float barX = cardTL.x + 12;
        float barY = cardTL.y + 42;
        float barW = cardW - 24;
        float barH = 4.f;
        float maxReach = 5.0f;
        float fill = std::clamp(displayVal / maxReach, 0.f, 1.f);

        drawList->AddRectFilled(
            { barX, barY }, { barX + barW, barY + barH },
            IM_COL32(30, 30, 50, (int)(160 * opacity)), 2.f);

        if (fill > 0.01f) {
            // Green → Yellow → Red gradient based on reach
            ImColor barLeft = ImColor(40, 200, 100, (int)(220 * opacity));
            ImColor barRight;
            if (displayVal >= 3.5f) barRight = ImColor(255, 50, 50, (int)(220 * opacity));
            else if (displayVal >= 3.0f) barRight = ImColor(255, 180, 40, (int)(220 * opacity));
            else barRight = ImColor(60, 180, 255, (int)(220 * opacity));

            drawList->AddRectFilledMultiColor(
                { barX, barY }, { barX + barW * fill, barY + barH },
                barLeft, barRight, barRight, barLeft);
        }

        // Glow edge
        if (fill > 0.02f) {
            float edgeX = barX + barW * fill;
            drawList->AddCircleFilled({ edgeX, barY + barH / 2 }, 4.f,
                IM_COL32(255, 255, 255, (int)(40 * opacity)), 8);
        }

        // 3.0 limit marker
        {
            float limit = 3.0f / maxReach;
            float markerX = barX + barW * limit;
            drawList->AddLine(
                { markerX, barY - 2 }, { markerX, barY + barH + 2 },
                IM_COL32(255, 255, 255, (int)(60 * opacity)), 1.f);
        }
    }

    // ================= DEBUG =================
    if (mDebug.mValue) {
        float debugFS = 10.f;
        float debugY = cardTL.y + 52;
        drawList->AddText(ImGui::GetFont(), debugFS,
            { cardTL.x + 12, debugY },
            IM_COL32(100, 130, 180, 180),
            fmt::format("Reach: {:.2f} | Hits: {} | Time: {:.1f}s",
                mOpponentReach, mHitCount, duration.count()).c_str());
        drawList->AddText(ImGui::GetFont(), debugFS,
            { cardTL.x + 12, debugY + 14 },
            IM_COL32(100, 130, 180, 180),
            ("Attacker: " + (mAttackerName.empty() ? "none" : mAttackerName)).c_str());
    }

    ImGui::PopFont();

    if (mElement) mElement->mSize = { cardW, cardH };
}