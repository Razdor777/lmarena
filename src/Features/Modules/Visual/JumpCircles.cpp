//
// Created by alteik on 26/10/2024.
// Rewritten: Premium glow rings with fade + ripple + shockwave
//

#include "JumpCircles.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <cmath>

float JumpCircles::toRadians(float deg)
{
    return deg * (PI / 180.0f);
}

void JumpCircles::addCircle(const glm::vec3& pos) {
    uint64_t currentTime = NOW;
    if (currentTime - lastAddTime < 100) return;

    ImVec4 color = ColorUtils::getThemedColor(0);
    color.w = mOpacity.mValue;
    
    Circle newCircle = { pos, 0.1f, mMaxRadius.mValue, color, mGlowAmount.mValue, mOpacity.mValue, currentTime };
    circles.push_back(newCircle);
    lastAddTime = currentTime;
}

void JumpCircles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &JumpCircles::onRenderEvent>(this);
}

void JumpCircles::onDisable()
{
    circles.clear();
    gFeatureManager->mDispatcher->deafen<RenderEvent, &JumpCircles::onRenderEvent>(this);
}

void JumpCircles::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->getLevel()) return;

    uint64_t currentTime = NOW;

    if (!player->wasOnGround() && player->isOnGround()) {
        glm::vec3 pos = RenderUtils::transform.mPlayerPos;
        pos.y -= 2;
        addCircle(pos);
    }

    auto drawList = ImGui::GetBackgroundDrawList();
    auto corrected = RenderUtils::transform.mMatrix;
    auto resolution = ClientInstance::get()->getGuiData()->mResolution;
    int ringCount = (int)mRings.mValue;
    float thickness = mThickness.mValue;

    for (auto it = circles.begin(); it != circles.end(); ) {
        uint64_t elapsed = currentTime - it->startTime;
        float lifeTime = mLifeTime.mValue;
        
        if (elapsed > (uint64_t)lifeTime) {
            it = circles.erase(it);
            continue;
        }
        
        float lifeProgress = (float)elapsed / lifeTime; // 0→1
        
        // Expand radius
        float targetRadius = mMaxRadius.mValue * lifeProgress;
        it->radius += (targetRadius - it->radius) * 0.1f; // Smooth approach
        it->radius = std::min(it->radius, mMaxRadius.mValue);
        
        // Fade out opacity — smooth ease-out
        float fadeAlpha = 1.0f;
        if (lifeProgress > 0.4f) {
            fadeAlpha = 1.0f - ((lifeProgress - 0.4f) / 0.6f);
        }
        fadeAlpha = std::clamp(fadeAlpha, 0.0f, 1.0f);
        float baseAlpha = mOpacity.mValue * fadeAlpha;
        
        // Theme color (shifts over time for rainbow effect)
        ImVec4 baseColor = ColorUtils::getThemedColor(elapsed * 0.1f);
        
        constexpr int pointCount = 80;
        float ringSpacing = it->radius / (float)(ringCount + 1);

        switch (mStyle.mValue)
        {
        case Style::Ripple:
        {
            for (int ring = 0; ring < ringCount; ring++) {
                float ringRadius = it->radius - ring * ringSpacing;
                if (ringRadius <= 0.01f) continue;
                
                // Each ring fades independently
                float ringAlpha = baseAlpha * (1.0f - (float)ring / (float)ringCount) * 0.8f;
                float ringThickness = thickness * (1.0f - (float)ring / (float)ringCount * 0.5f);
                
                ImVec4 ringColor = baseColor;
                ringColor.w = ringAlpha;
                ImU32 col = IM_COL32((int)(ringColor.x * 255), (int)(ringColor.y * 255), 
                                     (int)(ringColor.z * 255), (int)(ringColor.w * 255));
                
                glm::vec2 prevScreen;
                bool hasPrev = false;
                
                for (int j = 0; j <= pointCount; j++) {
                    float angle = toRadians((float)j / pointCount * 360.0f);
                    glm::vec3 worldPos = {
                        it->position.x + ringRadius * cosf(angle),
                        it->position.y,
                        it->position.z + ringRadius * sinf(angle)
                    };
                    
                    glm::vec2 screenPos;
                    if (corrected.OWorldToScreen(RenderUtils::transform.mOrigin, worldPos, screenPos, MathUtils::fov, resolution)) {
                        if (hasPrev) {
                            drawList->AddLine(
                                ImVec2(prevScreen.x, prevScreen.y),
                                ImVec2(screenPos.x, screenPos.y),
                                col, ringThickness);
                        }
                        prevScreen = screenPos;
                        hasPrev = true;
                    } else {
                        hasPrev = false;
                    }
                }
            }
            break;
        }
        
        case Style::Pulse:
        {
            // Pulsing scale
            float pulse = 1.0f + sinf(lifeProgress * 12.0f) * 0.1f * (1.0f - lifeProgress);
            float pulseRadius = it->radius * pulse;
            
            for (int ring = 0; ring < std::min(ringCount, 3); ring++) {
                float ringRadius = pulseRadius - ring * ringSpacing * 1.5f;
                if (ringRadius <= 0.01f) continue;
                
                float ringAlpha = baseAlpha * (1.0f - (float)ring / 3.0f);
                float ringThickness = thickness * (3.0f - ring) * 0.5f;
                
                ImVec4 ringColor = baseColor;
                ringColor.w = ringAlpha;
                ImU32 col = IM_COL32((int)(ringColor.x * 255), (int)(ringColor.y * 255),
                                     (int)(ringColor.z * 255), (int)(ringColor.w * 255));
                
                glm::vec2 prevScreen;
                bool hasPrev = false;
                
                for (int j = 0; j <= pointCount; j++) {
                    float angle = toRadians((float)j / pointCount * 360.0f);
                    glm::vec3 worldPos = {
                        it->position.x + ringRadius * cosf(angle),
                        it->position.y,
                        it->position.z + ringRadius * sinf(angle)
                    };
                    
                    glm::vec2 screenPos;
                    if (corrected.OWorldToScreen(RenderUtils::transform.mOrigin, worldPos, screenPos, MathUtils::fov, resolution)) {
                        if (hasPrev) {
                            drawList->AddLine(
                                ImVec2(prevScreen.x, prevScreen.y),
                                ImVec2(screenPos.x, screenPos.y),
                                col, ringThickness);
                        }
                        prevScreen = screenPos;
                        hasPrev = true;
                    } else {
                        hasPrev = false;
                    }
                }
            }
            break;
        }
        
        case Style::Shockwave:
        {
            // Single fast-expanding thin ring
            float ringRadius = it->radius;
            float ringAlpha = baseAlpha;
            
            // Glow layers
            for (int glow = 2; glow >= 0; glow--) {
                float glowAlpha = ringAlpha * (0.15f * (float)(glow + 1));
                float glowThick = thickness + (float)(3 - glow) * 3.0f;
                
                ImVec4 glowColor = baseColor;
                glowColor.w = glowAlpha;
                ImU32 col = IM_COL32((int)(glowColor.x * 255), (int)(glowColor.y * 255),
                                     (int)(glowColor.z * 255), (int)(glowColor.w * 255));
                
                glm::vec2 prevScreen;
                bool hasPrev = false;
                
                for (int j = 0; j <= pointCount; j++) {
                    float angle = toRadians((float)j / pointCount * 360.0f);
                    glm::vec3 worldPos = {
                        it->position.x + ringRadius * cosf(angle),
                        it->position.y,
                        it->position.z + ringRadius * sinf(angle)
                    };
                    
                    glm::vec2 screenPos;
                    if (corrected.OWorldToScreen(RenderUtils::transform.mOrigin, worldPos, screenPos, MathUtils::fov, resolution)) {
                        if (hasPrev) {
                            drawList->AddLine(
                                ImVec2(prevScreen.x, prevScreen.y),
                                ImVec2(screenPos.x, screenPos.y),
                                col, glowThick);
                        }
                        prevScreen = screenPos;
                        hasPrev = true;
                    } else {
                        hasPrev = false;
                    }
                }
            }
            break;
        }
        }
        
        ++it;
    }
}