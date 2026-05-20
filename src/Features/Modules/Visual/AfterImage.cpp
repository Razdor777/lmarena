#include "AfterImage.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

void AfterImage::onEnable() {
    mFrames.clear();
    mTickCounter = 0;
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AfterImage::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &AfterImage::onRenderEvent>(this);
}

void AfterImage::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AfterImage::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AfterImage::onRenderEvent>(this);
    mFrames.clear();
}

void AfterImage::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto pos = player->getPos();
    if (!pos) return;

    mTickCounter++;

    if (mOnlyMoving.mValue) {
        float dist = glm::distance(*pos, mLastPos);
        mLastPos = *pos;
        if (dist < 0.05f) return;
    }

    int spacing = std::max(1, static_cast<int>(mSpacing.mValue));
    if (mTickCounter % spacing != 0) return;

    GhostFrame frame;
    frame.position = *pos;
    frame.timestamp = NOW;

    auto rot = player->getActorRotationComponent();
    frame.yaw = rot ? rot->mYaw : 0.f;

    mFrames.push_back(frame);

    int maxFrames = static_cast<int>(mTrailCount.mValue);
    while ((int)mFrames.size() > maxFrames)
        mFrames.pop_front();
}

void AfterImage::onRenderEvent(RenderEvent& event)
{
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;

    if (mFrames.empty()) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    int totalFrames = (int)mFrames.size();

    // ================= CONNECTING TRAIL LINE =================
    // Draw a flowing line between all ghosts (bottom center)
    ImVec2 prevBottomScreen = {};
    bool prevValid = false;

    for (int i = 0; i < totalFrames; i++)
    {
        const GhostFrame& frame = mFrames[i];
        float ageRatio = (float)i / (float)totalFrames;
        float alpha = ageRatio * mFadeSpeed.mValue;
        alpha = std::clamp(alpha, 0.02f, 0.8f);

        glm::vec3 feetPos = frame.position;
        feetPos.y -= 1.62f;

        // Get color
        ImColor baseColor;
        switch (mColorMode.mValue) {
            case 0: baseColor = ColorUtils::getThemedColor(i * 0.15f); break;
            case 1: baseColor = ImColor(220, 220, 255, 255); break;
            case 2: {
                float hue = fmod((float)i / totalFrames + NOW / 3000.f, 1.f);
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.85f, 1.0f, r, g, b);
                baseColor = ImColor(r, g, b, 1.f);
                break;
            }
            case 3: baseColor = ImColor(255, 50, 50, 255); break;
            case 4: baseColor = ImColor(50, 100, 255, 255); break;
            default: baseColor = ImColor(255, 255, 255, 255); break;
        }

        // ================= 3D GHOST HITBOX =================
        float halfW = 0.3f;
        float height = 1.8f;

        // 8 corners of the hitbox
        glm::vec3 corners[8] = {
            { feetPos.x - halfW, feetPos.y,          feetPos.z - halfW }, // 0: bottom-left-back
            { feetPos.x + halfW, feetPos.y,          feetPos.z - halfW }, // 1: bottom-right-back
            { feetPos.x + halfW, feetPos.y,          feetPos.z + halfW }, // 2: bottom-right-front
            { feetPos.x - halfW, feetPos.y,          feetPos.z + halfW }, // 3: bottom-left-front
            { feetPos.x - halfW, feetPos.y + height, feetPos.z - halfW }, // 4: top-left-back
            { feetPos.x + halfW, feetPos.y + height, feetPos.z - halfW }, // 5: top-right-back
            { feetPos.x + halfW, feetPos.y + height, feetPos.z + halfW }, // 6: top-right-front
            { feetPos.x - halfW, feetPos.y + height, feetPos.z + halfW }, // 7: top-left-front
        };

        ImVec2 screenCorners[8];
        bool allVisible = true;
        for (int c = 0; c < 8; c++) {
            if (!RenderUtils::worldToScreen(corners[c], screenCorners[c])) {
                allVisible = false;
                break;
            }
        }

        if (allVisible) {
            ImColor fillColor = baseColor;
            fillColor.Value.w = alpha * 0.15f;

            ImColor lineColor = baseColor;
            lineColor.Value.w = alpha * 0.7f;

            ImColor glowColor = baseColor;
            glowColor.Value.w = alpha * 0.08f;

            // Draw filled faces (front and back quads)
            struct Face { int a, b, c, d; };
            Face faces[] = {
                {0, 1, 2, 3}, // bottom
                {4, 5, 6, 7}, // top
                {0, 1, 5, 4}, // back
                {2, 3, 7, 6}, // front
                {0, 3, 7, 4}, // left
                {1, 2, 6, 5}, // right
            };

            for (auto& face : faces) {
                ImVec2 quad[4] = { screenCorners[face.a], screenCorners[face.b],
                                    screenCorners[face.c], screenCorners[face.d] };
                drawList->AddConvexPolyFilled(quad, 4, fillColor);
            }

            // Draw edges
            int edges[][2] = {
                {0,1},{1,2},{2,3},{3,0}, // bottom
                {4,5},{5,6},{6,7},{7,4}, // top
                {0,4},{1,5},{2,6},{3,7}, // vertical
            };

            for (auto& edge : edges) {
                // Glow pass
                drawList->AddLine(screenCorners[edge[0]], screenCorners[edge[1]],
                    glowColor, 3.f);
                // Sharp pass
                drawList->AddLine(screenCorners[edge[0]], screenCorners[edge[1]],
                    lineColor, 1.2f);
            }

            // Head circle at top center
            ImVec2 topCenter = {
                (screenCorners[4].x + screenCorners[5].x + screenCorners[6].x + screenCorners[7].x) / 4,
                (screenCorners[4].y + screenCorners[5].y + screenCorners[6].y + screenCorners[7].y) / 4
            };

            float boxHeight = abs(screenCorners[0].y - screenCorners[4].y);
            float headR = std::max(3.f, boxHeight * 0.06f);

            ImColor headColor = baseColor;
            headColor.Value.w = alpha * 0.6f;

            ImColor headGlow = baseColor;
            headGlow.Value.w = alpha * 0.15f;

            drawList->AddCircleFilled(topCenter, headR + 3, headGlow, 12);
            drawList->AddCircleFilled(topCenter, headR, headColor, 12);
        }

        // Connecting trail line
        ImVec2 bottomScreen;
        glm::vec3 bottomCenter = { feetPos.x, feetPos.y + 0.9f, feetPos.z };
        if (RenderUtils::worldToScreen(bottomCenter, bottomScreen)) {
            if (prevValid) {
                ImColor trailColor = baseColor;
                trailColor.Value.w = alpha * 0.4f;

                // Glow trail
                drawList->AddLine(prevBottomScreen, bottomScreen, trailColor, 3.f);
                trailColor.Value.w = alpha * 0.8f;
                drawList->AddLine(prevBottomScreen, bottomScreen, trailColor, 1.2f);
            }
            prevBottomScreen = bottomScreen;
            prevValid = true;
        } else {
            prevValid = false;
        }
    }
}
