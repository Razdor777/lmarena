#include "ReachCounter.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

// ============================================================
// Spoof: generate realistic-looking reach value
// Uses normal distribution — most hits appear around 2.7-2.85
// ============================================================
float ReachCounter::generateLegitReach()
{
    std::normal_distribution<float> dist(2.75f, 0.12f);
    float value = dist(mRng);
    return std::clamp(value, 2.45f, 2.98f);
}

void ReachCounter::onEnable() {
    mReach = 0.f;
    mDisplayReach = 0.f;
    mHitCount = 0;
    mLastHit = std::chrono::high_resolution_clock::now();
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &ReachCounter::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &ReachCounter::onRenderEvent>(this);
    // HudEditor only drags visible elements
    if (mElement) mElement->mVisible = true;
}

void ReachCounter::onDisable() {
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &ReachCounter::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ReachCounter::onRenderEvent>(this);
    if (mElement) mElement->mVisible = false;
}

void ReachCounter::onPacketOutEvent(PacketOutEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (event.mPacket->getId() == PacketID::InventoryTransaction) {
        auto pkt = event.getPacket<InventoryTransactionPacket>();
        auto cit = pkt->mTransaction.get();

        if (cit->type == ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) {
            auto iut = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(cit);

            if (iut->mActionType == ItemUseOnActorInventoryTransaction::ActionType::Attack) {
                auto level = player->getLevel();
                if (level) {
                    HitResult* hitResult = level->getHitResult();
                    if (hitResult && hitResult->mType == HitType::ENTITY) {
                        mReach = glm::distance(hitResult->mStartPos, hitResult->mPos);
                        mLastHit = std::chrono::high_resolution_clock::now();
                        mHitCount++;

                        if (mSpoof.mValue && mReach > mSpoofLimit.mValue) {
                            mDisplayReach = generateLegitReach();
                        } else {
                            mDisplayReach = mReach;
                        }
                    }
                }
            }
        }
    }
}

void ReachCounter::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - mLastHit;
    bool expired = duration.count() >= mResetTime.mValue;

    // Don't render if nothing to show (unless in HudEditor sample mode)
    bool sampleMode = mElement && mElement->mSampleMode;
    if (expired && !sampleMode && !mDebug.mValue) {
        mReach = 0.f;
        mDisplayReach = 0.f;
        return;
    }

    if (expired) {
        mReach = 0.f;
        mDisplayReach = 0.f;
    }

    auto drawList = ImGui::GetBackgroundDrawList();

    // Position from HudElement
    ImVec2 hudPos = mElement ? mElement->getPos() : ImVec2(ImGui::GetIO().DisplaySize.x / 2.f, 80.f);

    // ================= GLASS CARD =================
    float cardW = 180.f;
    float cardH = mDebug.mValue ? 75.f : 44.f;
    ImVec2 cardTL = { hudPos.x - cardW / 2, hudPos.y };
    ImVec2 cardBR = { hudPos.x + cardW / 2, hudPos.y + cardH };

    // Background glass
    drawList->AddRectFilled(cardTL, cardBR, IM_COL32(8, 12, 22, 180), 8.f);
    // Border glow
    drawList->AddRect(cardTL, cardBR, IM_COL32(40, 100, 180, 80), 8.f, 0, 1.f);
    // Top accent line
    drawList->AddLine(
        { cardTL.x + 8, cardTL.y },
        { cardBR.x - 8, cardTL.y },
        ColorUtils::getThemedColor(0), 2.f);

    float displayVal = sampleMode ? 2.87f : mDisplayReach;
    float opacity = sampleMode ? 0.5f : 1.f;

    // ================= REACH VALUE =================
    FontHelper::pushPrefFont(true, true); // large bold
    {
        std::string valText = fmt::format("{:.2f}", displayVal);
        float fontSize = 22.f;
        ImVec2 valSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, -1.f, valText.c_str());

        // Value on left side of card
        float valX = cardTL.x + 12;
        float valY = cardTL.y + 6;

        ImColor valColor = ImColor(255, 255, 255, (int)(255 * opacity));
        // Color code: green if low, orange if medium, red if high
        if (displayVal > 0.f) {
            if (displayVal >= 3.5f) valColor = ImColor(255, 80, 80, (int)(255 * opacity));
            else if (displayVal >= 3.0f) valColor = ImColor(255, 180, 60, (int)(255 * opacity));
            else valColor = ImColor(80, 255, 120, (int)(255 * opacity));
        }

        // Shadow
        drawList->AddText(ImGui::GetFont(), fontSize,
            { valX + 1, valY + 1 }, IM_COL32(0, 0, 0, (int)(150 * opacity)), valText.c_str());
        drawList->AddText(ImGui::GetFont(), fontSize,
            { valX, valY }, valColor, valText.c_str());

        // Label "REACH" on right
        std::string label = "REACH";
        float labelFS = 11.f;
        ImVec2 labelSize = ImGui::GetFont()->CalcTextSizeA(labelFS, FLT_MAX, -1.f, label.c_str());
        float labelX = cardBR.x - labelSize.x - 12;
        float labelY = cardTL.y + 8;
        drawList->AddText(ImGui::GetFont(), labelFS,
            { labelX, labelY }, IM_COL32(120, 160, 220, (int)(200 * opacity)), label.c_str());

        // Hit count badge
        if (mHitCount > 0 || sampleMode) {
            std::string hits = "x" + std::to_string(sampleMode ? 5 : mHitCount);
            float hitsFS = 11.f;
            ImVec2 hitsSize = ImGui::GetFont()->CalcTextSizeA(hitsFS, FLT_MAX, -1.f, hits.c_str());
            float hitsX = cardBR.x - hitsSize.x - 12;
            float hitsY = labelY + labelSize.y + 2;
            drawList->AddText(ImGui::GetFont(), hitsFS,
                { hitsX, hitsY }, IM_COL32(180, 180, 200, (int)(140 * opacity)), hits.c_str());
        }
    }

    // ================= REACH BAR =================
    {
        float barX = cardTL.x + 12;
        float barY = cardTL.y + 32;
        float barW = cardW - 24;
        float barH = 4.f;
        float maxReach = 3.5f;
        float fill = std::clamp(displayVal / maxReach, 0.f, 1.f);

        // Bar background
        drawList->AddRectFilled(
            { barX, barY }, { barX + barW, barY + barH },
            IM_COL32(30, 30, 50, (int)(160 * opacity)), 2.f);

        // Filled portion with gradient
        if (fill > 0.01f) {
            ImColor barLeft = ImColor(40, 200, 100, (int)(220 * opacity));
            ImColor barRight;
            if (displayVal >= 3.0f) barRight = ImColor(255, 80, 80, (int)(220 * opacity));
            else barRight = ImColor(60, 180, 255, (int)(220 * opacity));

            drawList->AddRectFilledMultiColor(
                { barX, barY }, { barX + barW * fill, barY + barH },
                barLeft, barRight, barRight, barLeft);
        }

        // Glow at fill edge
        if (fill > 0.02f) {
            float edgeX = barX + barW * fill;
            drawList->AddCircleFilled({ edgeX, barY + barH / 2 }, 4.f,
                IM_COL32(255, 255, 255, (int)(40 * opacity)), 8);
        }
    }

    // ================= DEBUG INFO =================
    if (mDebug.mValue) {
        float debugFS = 10.f;
        float debugY = cardTL.y + 42;
        std::string dbg1 = fmt::format("Real: {:.2f} | Show: {:.2f}", mReach, mDisplayReach);
        std::string dbg2 = fmt::format("Hits: {} | Timer: {:.1f}s", mHitCount, duration.count());

        drawList->AddText(ImGui::GetFont(), debugFS,
            { cardTL.x + 12, debugY }, IM_COL32(100, 130, 180, 180), dbg1.c_str());
        drawList->AddText(ImGui::GetFont(), debugFS,
            { cardTL.x + 12, debugY + 14 }, IM_COL32(100, 130, 180, 180), dbg2.c_str());
    }

    ImGui::PopFont();

    // Update element size for HudEditor
    if (mElement) mElement->mSize = { cardW, cardH };
}