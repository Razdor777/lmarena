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
    // HudEditor only renders/drags elements that are visible — without this
    // the radar could never be moved
    if (mElement) mElement->mVisible = true;
}

void EntityRadar::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &EntityRadar::onRenderEvent>(this);
    if (mElement) mElement->mVisible = false;
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

    // Position from HudElement (draggable via HudEditor)
    ImVec2 elemPos = mElement ? mElement->getPos() : ImVec2(ImGui::GetIO().DisplaySize.x - radius * 2 - 20, 20);
    ImVec2 center  = { elemPos.x + radius, elemPos.y + radius };

    if (mElement) mElement->mSize = { mSize.mValue, mSize.mValue };

    // Player yaw
    float yawRad = 0.f;
    if (mRotate.mValue) {
        auto rot = player->getActorRotationComponent();
        if (rot) yawRad = glm::radians(rot->mYaw);
    }
    float cosY = cosf(yawRad);
    float sinY = sinf(yawRad);

    // World delta (dx, dz) -> radar axes:
    //   outX = "right"    (screen +x)
    //   outZ = "forward"  (screen up)
    // MC: forward = (-sinθ, cosθ), right = (-cosθ, -sinθ).
    // The old math mixed the signs, which is why people on your right
    // showed up on the left. This one is verified against cardinal dirs.
    auto project = [&](float dx, float dz, float& outX, float& outZ) {
        if (mRotate.mValue) {
            outX = -(dx * cosY + dz * sinY);      // dot(delta, right)
            outZ = -dx * sinY + dz * cosY;        // dot(delta, forward)
        } else {
            outX = dx;   // east = right
            outZ = -dz;  // north (-Z) = up
        }
    };

    ImColor accent = ColorUtils::getThemedColor(0.f);

    // ================================================================
    // Background — clean dark disc with a soft top light + faint rim
    // ================================================================
    {
        // soft outer aura
        for (int g = 0; g < 4; g++) {
            int a = (int)(10 * opacity * (1.f - g / 4.f));
            drawList->AddCircle(center, radius + 3.f + g * 2.f, IM_COL32(
                (int)(accent.Value.x * 255), (int)(accent.Value.y * 255), (int)(accent.Value.z * 255), a), 64, 1.f);
        }
    }

    drawList->AddCircleFilled(center, radius, IM_COL32(10, 11, 17, (int)(225 * opacity)), 64);
    // subtle top-light crescent
    drawList->AddCircleFilled({ center.x, center.y - radius * 0.35f }, radius * 0.85f,
        IM_COL32(22, 25, 34, (int)(70 * opacity)), 48);

    // hairline border + accent outer ring
    drawList->AddCircle(center, radius, IM_COL32(255, 255, 255, (int)(14 * opacity)), 64, 1.f);
    drawList->AddCircle(center, radius - 1.5f, accent, 64, 1.2f);

    // ================================================================
    // Range rings + crosshair
    // ================================================================
    for (int ring = 1; ring <= 3; ring++) {
        float ringR = radius * ring / 3.f;
        drawList->AddCircle(center, ringR, IM_COL32(255, 255, 255, (int)(9 * opacity)), 56, 0.5f);
    }
    // range label at the outer ring
    {
        std::string lbl = std::to_string((int)range) + "m";
        auto ts = ImGui::CalcTextSize(lbl.c_str());
        drawList->AddText({ center.x + radius * 0.66f - ts.x - 3, center.y - ts.y - 3 },
            IM_COL32(255, 255, 255, (int)(70 * opacity)), lbl.c_str());
    }

    drawList->AddLine({ center.x - radius, center.y }, { center.x + radius, center.y },
        IM_COL32(255, 255, 255, (int)(7 * opacity)), 0.5f);
    drawList->AddLine({ center.x, center.y - radius }, { center.x, center.y + radius },
        IM_COL32(255, 255, 255, (int)(7 * opacity)), 0.5f);

    // ================================================================
    // Sweep — thin rotating fan + crisp leading edge
    // ================================================================
    mSweepAngle += dt * 1.8f;
    if (mSweepAngle > 6.2831853f) mSweepAngle -= 6.2831853f;

    {
        float sweepSpan = 1.1f;
        int segments = 36;
        for (int i = 0; i < segments; i++) {
            float t = (float)i / segments;
            float angle = mSweepAngle - sweepSpan * t;
            float alpha = (1.f - t) * (1.f - t) * 0.16f * opacity;
            ImU32 col = IM_COL32(
                (int)(accent.Value.x * 255), (int)(accent.Value.y * 255), (int)(accent.Value.z * 255),
                (int)(255 * alpha));
            ImVec2 outer = { center.x + cosf(angle) * (radius - 2.f), center.y + sinf(angle) * (radius - 2.f) };
            drawList->AddLine(center, outer, col, 1.1f - t * 0.7f);
        }
        // leading edge
        ImVec2 edge = { center.x + cosf(mSweepAngle) * (radius - 2.f), center.y + sinf(mSweepAngle) * (radius - 2.f) };
        drawList->AddLine(center, edge, IM_COL32(
            (int)(accent.Value.x * 255), (int)(accent.Value.y * 255), (int)(accent.Value.z * 255),
            (int)(140 * opacity)), 1.2f);
    }

    // ================================================================
    // Cardinal directions — computed with the SAME projection as the
    // entities, so they can never disagree (N was on the wrong side!)
    // ================================================================
    struct CardinalDir { const char* label; float dx; float dz; bool isNorth; };
    static const CardinalDir dirs[] = {
        { "N",  0.f, -1.f, true  },
        { "E",  1.f,  0.f, false },
        { "S",  0.f,  1.f, false },
        { "W", -1.f,  0.f, false },
    };

    for (auto& d : dirs) {
        float px, pz;
        project(d.dx, d.dz, px, pz);
        float nx = center.x + px * (radius - 12.f);
        float ny = center.y - pz * (radius - 12.f);
        auto ts = ImGui::CalcTextSize(d.label);
        ImU32 col = d.isNorth
            ? IM_COL32(255, 90, 90, (int)(220 * opacity))
            : IM_COL32(200, 210, 225, (int)(110 * opacity));
        drawList->AddText({ nx - ts.x / 2, ny - ts.y / 2 }, col, d.label);
    }

    // ================================================================
    // Entities — clean dots: soft halo, core, hairline, facing arrow
    // ================================================================
    auto level = player->getLevel();
    if (!level) goto finish;

    {
        auto actors = level->getRuntimeActorList();
        ImVec2 mousePos = ImGui::GetMousePos();
        float dotScale = radius * 0.88f;

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
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist > range) continue;

            float relX, relZ;
            project(dx, dz, relX, relZ);

            float dotX = center.x + (relX / range) * dotScale;
            float dotY = center.y - (relZ / range) * dotScale;

            // Clamp to circle
            float ddx = dotX - center.x, ddy = dotY - center.y;
            float dotDist = sqrtf(ddx * ddx + ddy * ddy);
            if (dotDist > radius - 8.f) {
                float scale = (radius - 8.f) / dotDist;
                dotX = center.x + ddx * scale;
                dotY = center.y + ddy * scale;
            }

            if (isPlayer) {
                // gentle pulse ring
                float pulse = sinf((float)(NOW % 100000) / 350.f + dist) * 0.25f + 0.75f;
                drawList->AddCircle({ dotX, dotY }, 6.5f * pulse,
                    IM_COL32(255, 70, 70, (int)(60 * opacity)), 14, 1.2f);
                // dot
                drawList->AddCircleFilled({ dotX, dotY }, 3.5f, IM_COL32(255, 75, 75, 255), 12);
                drawList->AddCircle({ dotX, dotY }, 3.5f, IM_COL32(15, 15, 20, 160), 12, 1.f);

                // facing arrow — project the actor's look dir the same way as
                // positions, so it's right in BOTH radar modes (rotate/static)
                auto actorRot = actor->getActorRotationComponent();
                if (actorRot) {
                    float actYaw = glm::radians(actorRot->mYaw);
                    float ax, az;
                    project(-sinf(actYaw), cosf(actYaw), ax, az);
                    float arrowLen = 7.f;
                    ImVec2 arrowTip = { dotX + ax * arrowLen, dotY - az * arrowLen };
                    drawList->AddLine({ dotX, dotY }, arrowTip,
                        IM_COL32(255, 120, 120, (int)(160 * opacity)), 1.2f);
                }

                // Name tooltip on hover
                if (mShowNames.mValue) {
                    float hoverDist = sqrtf(
                        (mousePos.x - dotX) * (mousePos.x - dotX) +
                        (mousePos.y - dotY) * (mousePos.y - dotY));
                    if (hoverDist < 14.f) {
                        std::string label = actor->getNameTag() + "  " + std::to_string((int)dist) + "m";
                        auto ts = ImGui::CalcTextSize(label.c_str());
                        float tx = dotX - ts.x / 2;
                        float ty = dotY - ts.y - 8;

                        drawList->AddRectFilled({ tx - 5, ty - 3 }, { tx + ts.x + 5, ty + ts.y + 3 },
                            IM_COL32(8, 9, 14, 230), 4.f);
                        drawList->AddRect({ tx - 5, ty - 3 }, { tx + ts.x + 5, ty + ts.y + 3 },
                            IM_COL32(255, 255, 255, 30), 4.f, 0, 1.f);
                        drawList->AddText({ tx, ty }, IM_COL32(240, 240, 245, 255), label.c_str());
                    }
                }
            }
            else {
                drawList->AddCircleFilled({ dotX, dotY }, 2.2f,
                    IM_COL32(170, 175, 185, (int)(190 * opacity)), 8);
            }
        }
    }

    finish:

    // ================================================================
    // Center — you (small dot + facing triangle)
    // ================================================================
    {
        drawList->AddCircleFilled(center, 3.f, accent, 10);
        drawList->AddCircle(center, 3.f, IM_COL32(15, 15, 20, 160), 10, 1.f);

        // "you" triangle — project your own look dir too: points where you
        // actually face (straight up when the radar rotates with you)
        float fx, fz;
        project(-sinY, cosY, fx, fz);
        float triAngle = atan2f(-fz, fx); // screen space (y down)
        float triR = 7.5f;
        ImVec2 triTip = { center.x + cosf(triAngle) * triR, center.y + sinf(triAngle) * triR };
        ImVec2 triL = { center.x + cosf(triAngle + 2.6f) * (triR * 0.45f), center.y + sinf(triAngle + 2.6f) * (triR * 0.45f) };
        ImVec2 triRr = { center.x + cosf(triAngle - 2.6f) * (triR * 0.45f), center.y + sinf(triAngle - 2.6f) * (triR * 0.45f) };
        drawList->AddTriangleFilled(triTip, triL, triRr, accent);
    }
}
