//
// Created by vastrakai on 7/7/2024.
//

#include "ESP.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

void ESP::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &ESP::onRenderEvent>(this);
}

void ESP::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ESP::onRenderEvent>(this);
}

// ==================== RENDER HELPERS ====================

void ESP::renderGlowBox(ImDrawList* drawList, const std::vector<ImVec2>& points, ImColor color, float glowSize)
{
    if (points.empty()) return;

    for (int i = 3; i >= 1; i--) {
        float alpha = 0.08f * (float)i;
        float thickness = glowSize * 0.3f * (float)(4 - i);
        ImColor glowColor(color.Value.x, color.Value.y, color.Value.z, alpha);
        drawList->AddPolyline(points.data(), (int)points.size(), glowColor, ImDrawFlags_Closed, thickness);
    }

    drawList->AddPolyline(points.data(), (int)points.size(), color, ImDrawFlags_Closed, 2.0f);
}

void ESP::renderCorners(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImColor color, float length, float thickness)
{
    float w = max.x - min.x;
    float h = max.y - min.y;
    float cornerLen = fminf(length, fminf(w, h) * 0.4f);

    // Top-left
    drawList->AddLine(min, ImVec2(min.x + cornerLen, min.y), color, thickness);
    drawList->AddLine(min, ImVec2(min.x, min.y + cornerLen), color, thickness);

    // Top-right
    drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x - cornerLen, min.y), color, thickness);
    drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + cornerLen), color, thickness);

    // Bottom-left
    drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + cornerLen, max.y), color, thickness);
    drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x, max.y - cornerLen), color, thickness);

    // Bottom-right
    drawList->AddLine(max, ImVec2(max.x - cornerLen, max.y), color, thickness);
    drawList->AddLine(max, ImVec2(max.x, max.y - cornerLen), color, thickness);
}

// ==================== ENTITY RENDER ====================

void ESP::renderEntity(ImDrawList* drawList, Actor* actor, Actor* localPlayer, ImColor themeColor)
{
    if (!actor || !drawList) return;
    
    AABB aabb;
    try {
        aabb = actor->getAABB();
    } catch (...) {
        return;
    }
    
    // Проверяем что AABB адекватный
    if (!std::isfinite(aabb.mMin.x) || !std::isfinite(aabb.mMin.y) || !std::isfinite(aabb.mMin.z)) return;
    if (!std::isfinite(aabb.mMax.x) || !std::isfinite(aabb.mMax.y) || !std::isfinite(aabb.mMax.z)) return;
    if (aabb.mMax.x - aabb.mMin.x > 10.f) return;
    if (aabb.mMax.y - aabb.mMin.y > 10.f) return;
    if (aabb.mMax.z - aabb.mMin.z > 10.f) return;
    
    std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(aabb);
    if (imPoints.size() < 3) return;

    ImVec2 bbMin(FLT_MAX, FLT_MAX);
    ImVec2 bbMax(-FLT_MAX, -FLT_MAX);
    bool validPoints = true;
    
    for (auto& p : imPoints) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
            validPoints = false;
            break;
        }
        if (p.x < -5000.f || p.x > 15000.f || p.y < -5000.f || p.y > 15000.f) {
            validPoints = false;
            break;
        }
        bbMin.x = fminf(bbMin.x, p.x);
        bbMin.y = fminf(bbMin.y, p.y);
        bbMax.x = fmaxf(bbMax.x, p.x);
        bbMax.y = fmaxf(bbMax.y, p.y);
    }
    
    if (!validPoints) return;
    if (bbMax.x - bbMin.x < 1.f || bbMax.y - bbMin.y < 1.f) return;

    switch (mStyle.mValue)
    {
        case Style::Glow:
        {
            if (mRenderFilled.mValue) {
                drawList->AddConvexPolyFilled(imPoints.data(), (int)imPoints.size(),
                    ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, 0.12f));
            }

            if (mOuterGlow.mValue)
                renderGlowBox(drawList, imPoints, themeColor, mGlowSize.mValue);
            else
                drawList->AddPolyline(imPoints.data(), (int)imPoints.size(), themeColor, ImDrawFlags_Closed, 2.0f);

            break;
        }
        case Style::Corners:
        {
            renderCorners(drawList, bbMin, bbMax, themeColor, mCornerLength.mValue, 2.5f);

            if (mRenderFilled.mValue) {
                drawList->AddRectFilled(bbMin, bbMax,
                    ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, 0.06f));
            }

            if (mOuterGlow.mValue) {
                drawList->AddShadowRect(bbMin, bbMax,
                    ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, 0.15f),
                    mGlowSize.mValue, ImVec2(), 0, 0.0f);
            }
            break;
        }
        case Style::Style3D:
        default:
        {
            if (mRenderFilled.mValue) {
                drawList->AddConvexPolyFilled(imPoints.data(), (int)imPoints.size(),
                    ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, 0.25f));
            }
            drawList->AddPolyline(imPoints.data(), (int)imPoints.size(), themeColor, ImDrawFlags_Closed, 2.0f);
            break;
        }
    }
}

// ==================== MAIN RENDER ====================

void ESP::onRenderEvent(RenderEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) return;

    auto actors = ActorUtils::getActorList(false, true);
    auto localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();

    for (auto actor : actors)
    {
        if (!actor) continue;
        
        // Проверяем что актор живой
        try {
            if (actor->isDead()) continue;
            if (actor->getHealth() <= 0.f) continue;
        } catch (...) {
            continue;
        }
        
        if (actor == localPlayer && 
            ClientInstance::get()->getOptions()->mThirdPerson->value == 0 && 
            !localPlayer->getFlag<RenderCameraComponent>()) continue;
        if (actor == localPlayer && !mRenderLocal.mValue) continue;
        
        auto shape = actor->getAABBShapeComponent();
        if (!shape) continue;
        
        // Проверяем что размеры адекватные (не мусор из памяти)
        if (shape->mWidth <= 0.f || shape->mWidth > 10.f) continue;
        if (shape->mHeight <= 0.f || shape->mHeight > 10.f) continue;

        // Проверяем позицию актора
        glm::vec3 actorPos;
        try {
            auto* pos = actor->getPos();
            if (!pos) continue;
            actorPos = *pos;
        } catch (...) {
            continue;
        }
        
        // Проверяем что позиция не мусор
        if (!std::isfinite(actorPos.x) || 
            !std::isfinite(actorPos.y) || 
            !std::isfinite(actorPos.z)) continue;
            
        // Ограничиваем дистанцию ESP (призраки обычно далеко или в странных местах)
        try {
            float dist = actor->distanceTo(localPlayer);
            if (dist > 64.f) continue; // дальше 64 блоков не рендерим
            if (dist <= 0.f) continue;
        } catch (...) {
            continue;
        }

        auto themeColor = ColorUtils::getThemedColor(0);

        if (actor->isPlayer())
        {
            if (gFriendManager && gFriendManager->isFriend(actor))
            {
                if (mShowFriends.mValue) themeColor = ImColor(0.0f, 1.0f, 0.0f);
                else continue;
            }
        }

        try {
            renderEntity(drawList, actor, localPlayer, themeColor);
        } catch (...) {}
    }
}