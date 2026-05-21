//
// HitColor - Custom hurt color via real game hook + glow for players
//

#include "HitColor.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>

void HitColor::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &HitColor::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &HitColor::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onHurtColorEvent(HurtColorEvent& event)
{
    if (!mHurtColor.mValue) return;
    if (!event.mColor) return;

    // Only apply when actually hurt (alpha > 0 means hurt overlay is active)
    if (event.mColor[3] <= 0.01f) return;

    if (mRainbowHurt.mValue) {
        // Rainbow: cycle through hue based on time
        float time = static_cast<float>(ImGui::GetTime());
        float hue = fmodf(time * 0.5f, 1.0f); // 0.5 = speed

        // HSV to RGB conversion
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);

        event.setColor(r, g, b, 0.6f);
    } else {
        // Custom color from setting
        ImColor c = mHurtColorValue.getAsImColor();
        event.setColor(c.Value.x, c.Value.y, c.Value.z, c.Value.w);
    }
}

void HitColor::onRenderEvent(RenderEvent& event)
{
    if (!mGlow.mValue) return;

    auto ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) return;

    auto localPlayer = ci->getLocalPlayer();
    if (!localPlayer) return;

    auto actors = ActorUtils::getActorList(true, true); // только игроки, без ботов
    auto drawList = ImGui::GetBackgroundDrawList();

    for (auto actor : actors)
    {
        // Только игроки
        if (!actor->isPlayer()) continue;

        // Пропускаем себя
        if (actor == localPlayer) continue;

        auto shape = actor->getAABBShapeComponent();
        if (!shape) continue;

        AABB aabb = actor->getAABB();
        std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(aabb);

        if (imPoints.empty()) continue;

        ImColor glowColor = mGlowColor.getAsImColor();
        float thickness = mGlowThickness.mValue;
        float intensity = mGlowIntensity.mValue;

        // Рисуем несколько слоёв для эффекта свечения
        for (int i = 0; i < 3; i++)
        {
            float layerThickness = thickness + (i * 2.0f * intensity);
            float layerAlpha = glowColor.Value.w * (1.0f - (i * 0.3f)) * intensity;

            if (layerAlpha <= 0.0f) continue;

            ImColor layerColor = ImColor(
                glowColor.Value.x,
                glowColor.Value.y,
                glowColor.Value.z,
                layerAlpha
            );

            drawList->AddPolyline(imPoints.data(), imPoints.size(), layerColor, ImDrawFlags_Closed, layerThickness);
        }

        // Основной контур
        drawList->AddPolyline(imPoints.data(), imPoints.size(), glowColor, ImDrawFlags_Closed, thickness);
    }
}