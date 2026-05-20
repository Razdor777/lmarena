#include "FakePlayer.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

void FakePlayer::onEnable() {
    mPosSet = false;
    auto ci = ClientInstance::get();
    auto player = ci ? ci->getLocalPlayer() : nullptr;
    if (player) {
        auto pos = player->getPos();
        if (pos) {
            mClonePos = *pos;
            mPosSet = true;
        }
        auto rot = player->getActorRotationComponent();
        if (rot) mCloneYaw = rot->mYaw;
        mCloneName = player->getNameTag();
    }
    gFeatureManager->mDispatcher->listen<RenderEvent, &FakePlayer::onRenderEvent>(this);
}

void FakePlayer::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &FakePlayer::onRenderEvent>(this);
}

void FakePlayer::onRenderEvent(RenderEvent& event)
{
    if (!mPosSet) return;
    if (!ClientInstance::get()) return;

    auto drawList = ImGui::GetBackgroundDrawList();

    float halfW = 0.3f;
    float height = 1.8f;
    glm::vec3 feetPos = mClonePos;
    feetPos.y -= 1.62f;

    // ================= 3D HITBOX RENDERING =================
    glm::vec3 corners[8] = {
        { feetPos.x - halfW, feetPos.y,          feetPos.z - halfW },
        { feetPos.x + halfW, feetPos.y,          feetPos.z - halfW },
        { feetPos.x + halfW, feetPos.y,          feetPos.z + halfW },
        { feetPos.x - halfW, feetPos.y,          feetPos.z + halfW },
        { feetPos.x - halfW, feetPos.y + height, feetPos.z - halfW },
        { feetPos.x + halfW, feetPos.y + height, feetPos.z - halfW },
        { feetPos.x + halfW, feetPos.y + height, feetPos.z + halfW },
        { feetPos.x - halfW, feetPos.y + height, feetPos.z + halfW },
    };

    ImVec2 sc[8];
    bool allVisible = true;
    for (int c = 0; c < 8; c++) {
        if (!RenderUtils::worldToScreen(corners[c], sc[c])) {
            allVisible = false;
            break;
        }
    }

    if (!allVisible) return;

    ImColor themeColor = ColorUtils::getThemedColor(0);

    // Pulsing glow
    float pulse = sin(NOW / 500.f) * 0.15f + 0.85f;

    // Face fills
    ImColor fillColor = themeColor;
    fillColor.Value.w = 0.08f * pulse;

    struct Face { int a, b, c, d; };
    Face faces[] = {
        {0, 1, 2, 3}, {4, 5, 6, 7},
        {0, 1, 5, 4}, {2, 3, 7, 6},
        {0, 3, 7, 4}, {1, 2, 6, 5},
    };

    for (auto& face : faces) {
        ImVec2 quad[4] = { sc[face.a], sc[face.b], sc[face.c], sc[face.d] };
        drawList->AddConvexPolyFilled(quad, 4, fillColor);
    }

    // Edge rendering — glow + core
    int edges[][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
    };

    ImColor glowEdge = themeColor;
    glowEdge.Value.w = 0.15f * pulse;

    ImColor coreEdge = themeColor;
    coreEdge.Value.w = 0.7f * pulse;

    for (auto& edge : edges) {
        drawList->AddLine(sc[edge[0]], sc[edge[1]], glowEdge, 4.f);
        drawList->AddLine(sc[edge[0]], sc[edge[1]], coreEdge, 1.5f);
    }

    // ================= HEAD =================
    ImVec2 topCenter = {
        (sc[4].x + sc[5].x + sc[6].x + sc[7].x) / 4,
        (sc[4].y + sc[5].y + sc[6].y + sc[7].y) / 4
    };

    float boxHeight = abs(sc[0].y - sc[4].y);
    float headR = std::max(4.f, boxHeight * 0.08f);

    ImColor headGlow = themeColor;
    headGlow.Value.w = 0.2f;
    drawList->AddCircleFilled(topCenter, headR + 4, headGlow, 16);

    ImColor headColor = themeColor;
    headColor.Value.w = 0.8f;
    drawList->AddCircleFilled(topCenter, headR, headColor, 16);
    drawList->AddCircle(topCenter, headR, ImColor(255, 255, 255, 180), 16, 1.f);

    // ================= GROUND CIRCLE =================
    ImVec2 bottomCenter = {
        (sc[0].x + sc[1].x + sc[2].x + sc[3].x) / 4,
        (sc[0].y + sc[1].y + sc[2].y + sc[3].y) / 4
    };

    float groundR = std::max(8.f, boxHeight * 0.15f);
    ImColor groundGlow = themeColor;
    groundGlow.Value.w = 0.1f;
    drawList->AddCircle(bottomCenter, groundR, groundGlow, 24, 2.f);
    drawList->AddCircle(bottomCenter, groundR * 0.6f, groundGlow, 24, 1.f);

    // ================= NAMETAG =================
    if (mShowNametag.mValue && !mCloneName.empty()) {
        std::string label = mCloneName;

        auto ts = ImGui::CalcTextSize(label.c_str());
        float tx = topCenter.x - ts.x / 2;
        float ty = topCenter.y - headR - ts.y - 14;

        // Glass card background
        float pad = 6.f;
        drawList->AddRectFilled(
            { tx - pad, ty - pad / 2 },
            { tx + ts.x + pad, ty + ts.y + pad / 2 },
            IM_COL32(5, 10, 20, 200), 6.f);
        drawList->AddRect(
            { tx - pad, ty - pad / 2 },
            { tx + ts.x + pad, ty + ts.y + pad / 2 },
            IM_COL32(themeColor.Value.x * 255, themeColor.Value.y * 255, themeColor.Value.z * 255, 100),
            6.f, 0, 1.f);

        drawList->AddText({ tx, ty }, IM_COL32(255, 255, 255, 240), label.c_str());

        // [CLONE] badge
        std::string badge = "CLONE";
        auto bs = ImGui::CalcTextSize(badge.c_str());
        float bx = tx + ts.x + pad + 4;
        float by = ty + (ts.y - bs.y) / 2;

        drawList->AddRectFilled(
            { bx - 3, by - 1 },
            { bx + bs.x + 3, by + bs.y + 1 },
            IM_COL32(themeColor.Value.x * 255, themeColor.Value.y * 255, themeColor.Value.z * 255, 60),
            3.f);
        drawList->AddText({ bx, by },
            IM_COL32(themeColor.Value.x * 255, themeColor.Value.y * 255, themeColor.Value.z * 255, 220),
            badge.c_str());
    }

    // ================= HEALTH BAR =================
    if (mShowHealthBar.mValue) {
        float barW = std::clamp(boxHeight * 0.6f, 30.f, 80.f);
        float barH = 4.f;
        float barX = topCenter.x - barW / 2;
        float barY = topCenter.y - headR - 6;
        if (mShowNametag.mValue) barY -= 22;

        // Background
        drawList->AddRectFilled({ barX, barY }, { barX + barW, barY + barH },
            IM_COL32(20, 20, 30, 180), 2.f);

        // Full health gradient
        drawList->AddRectFilledMultiColor(
            { barX, barY }, { barX + barW, barY + barH },
            IM_COL32(40, 200, 80, 220), IM_COL32(80, 255, 120, 220),
            IM_COL32(80, 255, 120, 220), IM_COL32(40, 200, 80, 220));

        // Glow at edge
        drawList->AddCircleFilled({ barX + barW, barY + barH / 2 }, 3.f,
            IM_COL32(100, 255, 100, 50), 8);
    }
}
