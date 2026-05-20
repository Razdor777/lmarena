//
// Created by vastrakai on 10/4/2024.
// Rewritten: Premium Tracers with gradient, glow, distance
//

#include "Tracers.hpp"

#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

void Tracers::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Tracers::onRenderEvent>(this);
}

void Tracers::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Tracers::onRenderEvent>(this);
}

// Closest point on polyline to a given point
static ImVec2 getClosestPointOnLine(const std::vector<ImVec2>& points, ImVec2 center) {
    if (points.empty()) return ImVec2(-1, -1);
    float minDist = FLT_MAX;
    ImVec2 closest(0, 0);
    for (size_t i = 0; i < points.size(); i++) {
        ImVec2 p1 = points[i];
        ImVec2 p2 = points[(i + 1) % points.size()];
        ImVec2 v1(p2.x - p1.x, p2.y - p1.y);
        ImVec2 v2(center.x - p1.x, center.y - p1.y);
        float dot = v1.x * v2.x + v1.y * v2.y;
        float lenSq = v1.x * v1.x + v1.y * v1.y;
        float param = (lenSq > 0) ? dot / lenSq : 0;
        param = std::clamp(param, 0.0f, 1.0f);
        ImVec2 pt(p1.x + param * v1.x, p1.y + param * v1.y);
        float dx = pt.x - center.x, dy = pt.y - center.y;
        float dist = dx * dx + dy * dy;
        if (dist < minDist) { minDist = dist; closest = pt; }
    }
    return closest;
}

void Tracers::onRenderEvent(RenderEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) return;

    auto actors = ActorUtils::getActorList(false, true);
    auto localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImVec2 fromPoint;
    switch (mCenterPoint.mValue) {
        case CenterPoint::Top:    fromPoint = ImVec2(displaySize.x * 0.5f, 0); break;
        case CenterPoint::Center: fromPoint = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f); break;
        case CenterPoint::Bottom: fromPoint = ImVec2(displaySize.x * 0.5f, displaySize.y); break;
    }

    auto playerPos = *localPlayer->getPos();
    float thickness = mThickness.mValue;

    for (auto actor : actors)
    {
        if (actor == localPlayer && ClientInstance::get()->getOptions()->mThirdPerson->value == 0 && !localPlayer->getFlag<RenderCameraComponent>()) continue;
        if (actor == localPlayer && !mRenderLocal.mValue) continue;
        auto shapeComp = actor->getAABBShapeComponent();
        if (!shapeComp) continue;

        ImColor themeColor = ColorUtils::getThemedColor(0);
        bool isFriend = false;
        
        if (actor->isPlayer()) {
            if (gFriendManager->isFriend(actor)) {
                if (mShowFriends.mValue) { themeColor = ImColor(0.0f, 1.0f, 0.0f); isFriend = true; }
                else continue;
            }
        }

        auto points = MathUtils::getImBoxPoints(actor->getAABB());
        if (points.empty()) continue;
        ImVec2 toPoint = getClosestPointOnLine(points, fromPoint);
        
        float actorDist = glm::distance(playerPos, *actor->getPos());
        // Fade alpha by distance (far = more transparent)
        float distAlpha = std::clamp(1.0f - (actorDist - 30.0f) / 100.0f, 0.3f, 1.0f);

        switch (mStyle.mValue)
        {
            case Style::Line:
            {
                ImColor lineColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, distAlpha);
                drawList->AddLine(fromPoint, toPoint, lineColor, thickness);
                break;
            }
            case Style::Gradient:
            {
                // Bottom/near = white, top/far = theme color
                ImColor startColor(1.0f, 1.0f, 1.0f, distAlpha * 0.15f);
                ImColor endColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, distAlpha * 0.9f);
                
                // Multi-segment gradient line
                constexpr int segments = 12;
                for (int i = 0; i < segments; i++) {
                    float t0 = (float)i / segments;
                    float t1 = (float)(i + 1) / segments;
                    
                    ImVec2 p0(fromPoint.x + (toPoint.x - fromPoint.x) * t0,
                              fromPoint.y + (toPoint.y - fromPoint.y) * t0);
                    ImVec2 p1(fromPoint.x + (toPoint.x - fromPoint.x) * t1,
                              fromPoint.y + (toPoint.y - fromPoint.y) * t1);
                    
                    float alpha0 = startColor.Value.w + (endColor.Value.w - startColor.Value.w) * t0;
                    float alpha1 = startColor.Value.w + (endColor.Value.w - startColor.Value.w) * t1;
                    float r0 = startColor.Value.x + (endColor.Value.x - startColor.Value.x) * t0;
                    float g0 = startColor.Value.y + (endColor.Value.y - startColor.Value.y) * t0;
                    float b0 = startColor.Value.z + (endColor.Value.z - startColor.Value.z) * t0;
                    
                    drawList->AddLine(p0, p1, ImColor(r0, g0, b0, (alpha0 + alpha1) * 0.5f), thickness);
                }
                break;
            }
            case Style::Glow:
            {
                // Glow layers
                float glowSize = mGlowSize.mValue;
                for (int i = 3; i >= 1; i--) {
                    float alpha = 0.06f * (float)i * distAlpha;
                    float t = thickness + glowSize * 0.3f * (float)(4 - i);
                    drawList->AddLine(fromPoint, toPoint,
                        ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, alpha), t);
                }
                // Core line
                drawList->AddLine(fromPoint, toPoint,
                    ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, distAlpha), thickness);
                break;
            }
            case Style::Arrow:
            {
                // Main line
                ImColor lineColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, distAlpha);
                drawList->AddLine(fromPoint, toPoint, lineColor, thickness);
                
                // Arrowhead at endpoint
                float dx = toPoint.x - fromPoint.x;
                float dy = toPoint.y - fromPoint.y;
                float len = sqrtf(dx * dx + dy * dy);
                if (len > 10.0f) {
                    float nx = dx / len, ny = dy / len;
                    float arrowSize = 8.0f;
                    
                    ImVec2 arrowPts[3] = {
                        toPoint,
                        ImVec2(toPoint.x - arrowSize * nx + arrowSize * 0.5f * ny,
                               toPoint.y - arrowSize * ny - arrowSize * 0.5f * nx),
                        ImVec2(toPoint.x - arrowSize * nx - arrowSize * 0.5f * ny,
                               toPoint.y - arrowSize * ny + arrowSize * 0.5f * nx),
                    };
                    drawList->AddConvexPolyFilled(arrowPts, 3, lineColor);
                }
                break;
            }
        }
        
        // Distance text at endpoint
        if (mShowDistance.mValue) {
            char distText[16];
            snprintf(distText, sizeof(distText), "%.0fm", actorDist);
            constexpr float fontSize = 13.0f;
            auto textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, distText);
            ImVec2 textPos(toPoint.x - textSize.x * 0.5f, toPoint.y + 4.0f);
            
            // Background pill
            drawList->AddRectFilled(
                ImVec2(textPos.x - 3, textPos.y - 1),
                ImVec2(textPos.x + textSize.x + 3, textPos.y + textSize.y + 1),
                ImColor(0.0f, 0.0f, 0.0f, 0.5f * distAlpha), 3.0f);
            
            drawList->AddText(ImGui::GetFont(), fontSize, textPos,
                ImColor(1.0f, 1.0f, 1.0f, 0.9f * distAlpha), distText);
        }
    }
}
