#include "PathTracer.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

void PathTracer::onEnable() {
    mTrails.clear();
    mTickCounter = 0;
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PathTracer::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &PathTracer::onRenderEvent>(this);
}

void PathTracer::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PathTracer::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &PathTracer::onRenderEvent>(this);
    mTrails.clear();
}

ImColor PathTracer::getColorForEntity(uint64_t runtimeId, int index)
{
    float hue = fmod((float)(runtimeId * 137) / 360.f, 1.f);
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(hue, 0.75f, 1.0f, r, g, b);
    return ImColor(r, g, b, 1.0f);
}

void PathTracer::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    mTickCounter++;
    int sampleRate = std::max(1, static_cast<int>(mSampleRate.mValue));
    if (mTickCounter % sampleRate != 0) return;

    auto level = player->getLevel();
    if (!level) return;

    uint64_t now = NOW;
    uint64_t maxAge = static_cast<uint64_t>(mTrailLength.mValue * 1000);

    if (mShowSelf.mValue) {
        auto pos = player->getPos();
        if (pos) {
            uint64_t id = player->getRuntimeID();
            auto& trail = mTrails[id];
            trail.push_back({ *pos, now });
            while (!trail.empty() && (now - trail.front().timestamp > maxAge))
                trail.pop_front();
        }
    }

    if (mShowOthers.mValue) {
        auto actors = level->getRuntimeActorList();
        for (Actor* actor : actors) {
            if (!actor || !actor->isValid() || !actor->isPlayer()) continue;
            if (actor == player) continue;

            auto pos = actor->getPos();
            if (!pos) continue;

            uint64_t id = actor->getRuntimeID();
            auto& trail = mTrails[id];
            trail.push_back({ *pos, now });
            while (!trail.empty() && (now - trail.front().timestamp > maxAge))
                trail.pop_front();
        }
    }

    for (auto it = mTrails.begin(); it != mTrails.end(); ) {
        if (it->second.empty()) it = mTrails.erase(it);
        else ++it;
    }
}

void PathTracer::onRenderEvent(RenderEvent& event)
{
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    uint64_t now = NOW;
    uint64_t maxAge = static_cast<uint64_t>(mTrailLength.mValue * 1000);
    float lineWidth = mLineWidth.mValue;

    for (auto& [runtimeId, trail] : mTrails)
    {
        if (trail.size() < 2) continue;

        bool isSelf = (runtimeId == player->getRuntimeID());
        if (isSelf && !mShowSelf.mValue) continue;
        if (!isSelf && !mShowOthers.mValue) continue;

        ImColor baseColor = isSelf
            ? ColorUtils::getThemedColor(0)
            : getColorForEntity(runtimeId, 0);

        // ================= 3D TRAIL RENDERING =================
        ImVec2 prevScreen = {};
        bool prevValid = false;
        int segIdx = 0;

        for (size_t i = 0; i < trail.size(); i++)
        {
            const auto& point = trail[i];
            ImVec2 screenPos;

            // Feet level for ground path
            glm::vec3 drawPos = point.position;
            drawPos.y -= 1.62f;

            if (!RenderUtils::worldToScreen(drawPos, screenPos)) {
                prevValid = false;
                continue;
            }

            if (prevValid) {
                float age = mFade.mValue
                    ? (float)(now - point.timestamp) / (float)maxAge
                    : 0.f;
                float alpha = std::clamp(1.f - age, 0.05f, 1.f);

                // Glow line (wider, dimmer)
                ImColor glowColor = baseColor;
                glowColor.Value.w = alpha * 0.15f;
                drawList->AddLine(prevScreen, screenPos, glowColor, lineWidth + 4.f);

                // Mid glow
                ImColor midColor = baseColor;
                midColor.Value.w = alpha * 0.4f;
                drawList->AddLine(prevScreen, screenPos, midColor, lineWidth + 1.5f);

                // Core line (sharp)
                ImColor coreColor = baseColor;
                coreColor.Value.w = alpha * 0.9f;
                drawList->AddLine(prevScreen, screenPos, coreColor, lineWidth);

                // Direction dots every N segments
                if (segIdx % 8 == 0 && alpha > 0.3f) {
                    ImColor dotColor = baseColor;
                    dotColor.Value.w = alpha * 0.7f;
                    drawList->AddCircleFilled(screenPos, lineWidth + 1.f, dotColor, 8);
                }

                // Direction arrow every 16 segments
                if (segIdx % 16 == 0 && alpha > 0.4f) {
                    // Calculate arrow direction
                    ImVec2 dir = { screenPos.x - prevScreen.x, screenPos.y - prevScreen.y };
                    float len = sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (len > 5.f) {
                        dir.x /= len;
                        dir.y /= len;

                        float arrowSize = lineWidth * 3;
                        ImVec2 tip = screenPos;
                        ImVec2 left = { tip.x - dir.x * arrowSize + dir.y * arrowSize * 0.5f,
                                        tip.y - dir.y * arrowSize - dir.x * arrowSize * 0.5f };
                        ImVec2 right = { tip.x - dir.x * arrowSize - dir.y * arrowSize * 0.5f,
                                         tip.y - dir.y * arrowSize + dir.x * arrowSize * 0.5f };

                        ImColor arrowColor = baseColor;
                        arrowColor.Value.w = alpha * 0.6f;
                        drawList->AddTriangleFilled(tip, left, right, arrowColor);
                    }
                }

                segIdx++;
            }

            prevScreen = screenPos;
            prevValid = true;
        }

        // ================= TRAIL HEAD (latest position) =================
        if (prevValid && !trail.empty()) {
            const auto& lastPoint = trail.back();
            float headAlpha = mFade.mValue
                ? std::clamp(1.f - (float)(now - lastPoint.timestamp) / (float)maxAge, 0.f, 1.f)
                : 1.f;

            if (headAlpha > 0.2f) {
                // Pulsing head dot
                float pulse = sin(NOW / 400.f) * 0.3f + 0.7f;

                ImColor headGlow = baseColor;
                headGlow.Value.w = headAlpha * 0.2f;
                drawList->AddCircleFilled(prevScreen, 8.f * pulse, headGlow, 12);

                ImColor headColor = baseColor;
                headColor.Value.w = headAlpha * 0.8f;
                drawList->AddCircleFilled(prevScreen, 3.f, headColor, 12);
                drawList->AddCircle(prevScreen, 3.f, ImColor(1.f, 1.f, 1.f, headAlpha * 0.5f), 12, 1.f);
            }
        }
    }
}
