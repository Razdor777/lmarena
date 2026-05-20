#include "EntityRadar.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/FontHelper.hpp>

void EntityRadar::onEnable() {
    mSweepAngle = 0.f;
    gFeatureManager->mDispatcher->listen<RenderEvent, &EntityRadar::onRenderEvent>(this);
}

void EntityRadar::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &EntityRadar::onRenderEvent>(this);
}

void EntityRadar::onRenderEvent(RenderEvent& event)
{
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    auto playerPos = player->getPos();
    if (!playerPos) return;

    float radius    = mSize.mValue / 2.f;
    float range     = mRange.mValue;
    float opacity   = mOpacity.mValue;
    float dt        = ImGui::GetIO().DeltaTime;

    // Position from HudElement
    ImVec2 elemPos = mElement ? mElement->getPos() : ImVec2(ImGui::GetIO().DisplaySize.x - radius * 2 - 20, 20);
    ImVec2 center  = { elemPos.x + radius, elemPos.y + radius };

    if (mElement) mElement->mSize = { mSize.mValue, mSize.mValue };

    // Player yaw
    float yawRad = 0.f;
    if (mRotate.mValue) {
        auto rot = player->getActorRotationComponent();
        if (rot) yawRad = glm::radians(rot->mYaw);
    }

    // ================================================================
    // LAYER 1: Deep background — outer glow ring
    // ================================================================
    for (int g = 0; g < 8; g++) {
        float gr = radius + 2.f + g * 1.5f;
        int alpha = (int)(30.f * opacity * (1.f - g / 8.f));
        drawList->AddCircle(center, gr, IM_COL32(0, 180, 255, alpha), 64, 1.f);
    }

    // ================================================================
    // LAYER 2: Main background — dark gradient circle
    // ================================================================
    // Multi-layer fill for depth illusion
    for (int i = 0; i < 20; i++) {
        float r = radius * (1.f - i / 20.f);
        int baseAlpha = (int)(220 * opacity);
        // Darker towards center
        int rComp = (int)(8 + 6 * (float)i / 20.f);
        int gComp = (int)(12 + 8 * (float)i / 20.f);
        int bComp = (int)(20 + 15 * (float)i / 20.f);
        drawList->AddCircleFilled(center, r, IM_COL32(rComp, gComp, bComp, baseAlpha), 64);
    }

    // ================================================================
    // LAYER 3: Range rings with subtle glow
    // ================================================================
    for (int ring = 1; ring <= 4; ring++) {
        float ringR = radius * ring / 4.f;
        // Glow
        drawList->AddCircle(center, ringR, IM_COL32(40, 100, 160, (int)(30 * opacity)), 64, 2.f);
        // Sharp line
        drawList->AddCircle(center, ringR, IM_COL32(50, 120, 180, (int)(60 * opacity)), 64, 0.5f);

        // Range labels on right side
        if (mShowNames.mValue) {
            int rangeAtRing = (int)(range * ring / 4.f);
            std::string label = std::to_string(rangeAtRing);
            auto ts = ImGui::CalcTextSize(label.c_str());
            drawList->AddText(
                { center.x + ringR - ts.x - 2, center.y - ts.y - 1 },
                IM_COL32(80, 140, 200, (int)(100 * opacity)),
                label.c_str());
        }
    }

    // ================================================================
    // LAYER 4: Cross lines (axes) with gradient
    // ================================================================
    auto drawGradientLine = [&](ImVec2 a, ImVec2 b, ImU32 colorCenter, ImU32 colorEdge) {
        ImVec2 mid = { (a.x + b.x) / 2, (a.y + b.y) / 2 };
        drawList->AddLine(a, mid, colorEdge, 0.7f);
        drawList->AddLine(mid, b, colorEdge, 0.7f);
        // Brighter center section
        ImVec2 q1 = { a.x + (b.x - a.x) * 0.35f, a.y + (b.y - a.y) * 0.35f };
        ImVec2 q3 = { a.x + (b.x - a.x) * 0.65f, a.y + (b.y - a.y) * 0.65f };
        drawList->AddLine(q1, q3, colorCenter, 1.f);
    };

    drawGradientLine(
        { center.x - radius, center.y }, { center.x + radius, center.y },
        IM_COL32(80, 150, 220, (int)(90 * opacity)),
        IM_COL32(40, 80, 120, (int)(40 * opacity)));
    drawGradientLine(
        { center.x, center.y - radius }, { center.x, center.y + radius },
        IM_COL32(80, 150, 220, (int)(90 * opacity)),
        IM_COL32(40, 80, 120, (int)(40 * opacity)));

    // ================================================================
    // LAYER 5: Sweep animation — gorgeous gradient fan
    // ================================================================
    mSweepAngle += dt * 1.8f;
    if (mSweepAngle > 6.2831853f) mSweepAngle -= 6.2831853f;

    float sweepSpan = 1.2f; // radians
    int sweepSegments = 60;
    for (int i = 0; i < sweepSegments; i++) {
        float t = (float)i / sweepSegments;
        float angle = mSweepAngle - yawRad - sweepSpan * t;

        // Alpha: strongest at front, fading to nothing
        float alpha = (1.f - t) * (1.f - t) * 0.35f * opacity;

        // Color shifts from cyan-green to transparent
        int r = (int)(0 + 20 * t);
        int g = (int)(255 - 80 * t);
        int b = (int)(120 + 100 * t);

        float innerR = radius * 0.05f;
        float outerR = radius * (0.95f - t * 0.1f);

        ImVec2 inner = { center.x + cos(angle) * innerR, center.y + sin(angle) * innerR };
        ImVec2 outer = { center.x + cos(angle) * outerR, center.y + sin(angle) * outerR };

        drawList->AddLine(inner, outer, IM_COL32(r, g, b, (int)(255 * alpha)), 1.5f - t * 0.8f);
    }

    // Sweep leading edge — bright line
    {
        float angle = mSweepAngle - yawRad;
        ImVec2 inner = { center.x + cos(angle) * (radius * 0.05f), center.y + sin(angle) * (radius * 0.05f) };
        ImVec2 outer = { center.x + cos(angle) * (radius * 0.92f), center.y + sin(angle) * (radius * 0.92f) };
        // Glow
        drawList->AddLine(inner, outer, IM_COL32(0, 255, 180, (int)(60 * opacity)), 4.f);
        // Sharp
        drawList->AddLine(inner, outer, IM_COL32(0, 255, 160, (int)(200 * opacity)), 1.5f);
    }

    // ================================================================
    // LAYER 6: Cardinal directions (N/S/E/W)
    // ================================================================
    struct CardinalDir { const char* label; float angle; ImU32 color; };
    CardinalDir dirs[] = {
        { "N", -1.5708f, IM_COL32(255, 70, 70, (int)(220 * opacity)) },
        { "E",  0.f,     IM_COL32(180, 200, 220, (int)(120 * opacity)) },
        { "S",  1.5708f, IM_COL32(180, 200, 220, (int)(120 * opacity)) },
        { "W",  3.1416f, IM_COL32(180, 200, 220, (int)(120 * opacity)) },
    };

    for (auto& d : dirs) {
        float angle = d.angle - yawRad;
        float nx = center.x + cos(angle) * (radius - 12);
        float ny = center.y + sin(angle) * (radius - 12);
        auto ts = ImGui::CalcTextSize(d.label);
        drawList->AddText({ nx - ts.x / 2, ny - ts.y / 2 }, d.color, d.label);
    }

    // ================================================================
    // LAYER 7: Entities — dots with glow + trails
    // ================================================================
    auto level = player->getLevel();
    if (!level) goto finish;

    {
        auto actors = level->getRuntimeActorList();
        ImVec2 mousePos = ImGui::GetMousePos();

        for (Actor* actor : actors)
        {
            if (!actor || !actor->isValid()) continue;
            if (actor == player) continue;

            bool isPlayer = actor->isPlayer();
            if (isPlayer && !mShowPlayers.mValue) continue;
            if (!isPlayer && !mShowMobs.mValue) continue;

            auto actorPos = actor->getPos();
            if (!actorPos) continue;

            float dx = actorPos->x - playerPos->x;
            float dz = actorPos->z - playerPos->z;
            float dist = sqrt(dx * dx + dz * dz);
            if (dist > range) continue;

            // Rotate
            float relX = dx, relZ = dz;
            if (mRotate.mValue) {
                // Convert world delta to player-local radar coordinates:
                // +X = right, +Z = forward.
                float cosY = cos(yawRad);
                float sinY = sin(yawRad);
                relX = dx * cosY - dz * sinY;
                relZ = dx * sinY + dz * cosY;
            }

            // Map to radar
            float dotX = center.x + (relX / range) * (radius * 0.88f);
            // On 2D radar: forward should be "up" (smaller Y).
            float dotY = center.y - (relZ / range) * (radius * 0.88f);

            // Clamp to circle
            float ddx = dotX - center.x, ddy = dotY - center.y;
            float dotDist = sqrt(ddx * ddx + ddy * ddy);
            bool clamped = false;
            if (dotDist > radius - 8) {
                float scale = (radius - 8) / dotDist;
                dotX = center.x + ddx * scale;
                dotY = center.y + ddy * scale;
                clamped = true;
            }

            if (isPlayer) {
                // ---- PLAYER DOT — premium multi-layer glow ----
                // Outer pulse glow
                float pulse = sin(NOW / 400.f + dist) * 0.3f + 0.7f;
                float glowR = 12.f * pulse;
                drawList->AddCircleFilled({ dotX, dotY }, glowR, IM_COL32(255, 50, 50, (int)(25 * opacity)), 16);
                drawList->AddCircleFilled({ dotX, dotY }, glowR * 0.7f, IM_COL32(255, 50, 50, (int)(50 * opacity)), 16);
                drawList->AddCircleFilled({ dotX, dotY }, glowR * 0.4f, IM_COL32(255, 80, 80, (int)(100 * opacity)), 16);

                // Core dot
                drawList->AddCircleFilled({ dotX, dotY }, 4.f, IM_COL32(255, 60, 60, 255), 12);
                drawList->AddCircle({ dotX, dotY }, 4.f, IM_COL32(255, 200, 200, 200), 12, 1.f);

                // Arrow indicator (direction they're facing)
                auto actorRot = actor->getActorRotationComponent();
                if (actorRot) {
                    float actYaw = glm::radians(actorRot->mYaw) - yawRad;
                    float arrowLen = 8.f;
                    ImVec2 arrowTip = {
                        dotX + sin(actYaw) * arrowLen,
                        dotY - cos(actYaw) * arrowLen
                    };
                    drawList->AddLine({ dotX, dotY }, arrowTip,
                        IM_COL32(255, 100, 100, (int)(180 * opacity)), 1.5f);
                }

                // Name tooltip on hover
                if (mShowNames.mValue) {
                    float hoverDist = sqrt(
                        (mousePos.x - dotX) * (mousePos.x - dotX) +
                        (mousePos.y - dotY) * (mousePos.y - dotY));
                    if (hoverDist < 18.f) {
                        std::string name = actor->getNameTag();
                        std::string label = name + " §7" + fmt::format("{:.0f}m", dist);

                        auto ts = ImGui::CalcTextSize(label.c_str());
                        float tx = dotX - ts.x / 2;
                        float ty = dotY - ts.y - 10;

                        // Tooltip background
                        float pad = 5.f;
                        drawList->AddRectFilled(
                            { tx - pad, ty - pad / 2 },
                            { tx + ts.x + pad, ty + ts.y + pad / 2 },
                            IM_COL32(5, 10, 20, 220), 4.f);
                        drawList->AddRect(
                            { tx - pad, ty - pad / 2 },
                            { tx + ts.x + pad, ty + ts.y + pad / 2 },
                            IM_COL32(40, 100, 180, 150), 4.f, 0, 1.f);
                        drawList->AddText({ tx, ty }, IM_COL32(255, 255, 255, 255), label.c_str());
                    }
                }
            }
            else {
                // ---- MOB DOT — subtle ----
                float mobGlow = 5.f;
                drawList->AddCircleFilled({ dotX, dotY }, mobGlow, IM_COL32(120, 120, 120, (int)(30 * opacity)), 8);
                drawList->AddCircleFilled({ dotX, dotY }, 2.5f, IM_COL32(160, 160, 170, (int)(180 * opacity)), 8);
            }
        }
    }

    finish:

    // ================================================================
    // LAYER 8: Center indicator — player dot with pulse
    // ================================================================
    {
        float pulse = sin(NOW / 300.f) * 0.3f + 0.7f;
        drawList->AddCircleFilled(center, 6.f * pulse, IM_COL32(0, 180, 255, (int)(40 * opacity)), 16);
        drawList->AddCircleFilled(center, 4.f, IM_COL32(0, 200, 255, (int)(220 * opacity)), 12);
        drawList->AddCircle(center, 4.f, IM_COL32(180, 240, 255, (int)(180 * opacity)), 12, 1.f);

        // Player direction triangle
        float triAngle = -yawRad - 1.5708f;
        float triR = 7.f;
        ImVec2 triTip = { center.x + cos(triAngle) * triR, center.y + sin(triAngle) * triR };
        ImVec2 triL = { center.x + cos(triAngle + 2.5f) * (triR * 0.5f), center.y + sin(triAngle + 2.5f) * (triR * 0.5f) };
        ImVec2 triRr = { center.x + cos(triAngle - 2.5f) * (triR * 0.5f), center.y + sin(triAngle - 2.5f) * (triR * 0.5f) };
        drawList->AddTriangleFilled(triTip, triL, triRr, IM_COL32(0, 200, 255, (int)(150 * opacity)));
    }

    // ================================================================
    // LAYER 9: Border ring — premium double stroke
    // ================================================================
    drawList->AddCircle(center, radius + 1, IM_COL32(15, 25, 40, (int)(255 * opacity)), 64, 2.f);
    drawList->AddCircle(center, radius, IM_COL32(50, 120, 200, (int)(180 * opacity)), 64, 1.5f);
    drawList->AddCircle(center, radius - 1, IM_COL32(30, 70, 120, (int)(80 * opacity)), 64, 0.5f);

    // ================================================================
    // LAYER 10: Info label at bottom
    // ================================================================
    {
        std::string info = fmt::format("RADAR  {}m", (int)range);
        auto ts = ImGui::CalcTextSize(info.c_str());
        float tx = center.x - ts.x / 2;
        float ty = center.y + radius + 6;

        // Background pill
        drawList->AddRectFilled(
            { tx - 8, ty - 1 },
            { tx + ts.x + 8, ty + ts.y + 1 },
            IM_COL32(5, 10, 20, (int)(200 * opacity)), 8.f);
        drawList->AddRect(
            { tx - 8, ty - 1 },
            { tx + ts.x + 8, ty + ts.y + 1 },
            IM_COL32(40, 80, 140, (int)(120 * opacity)), 8.f, 0, 1.f);

        drawList->AddText({ tx, ty }, IM_COL32(100, 180, 240, (int)(200 * opacity)), info.c_str());
    }
}
