// HitColor - Custom hurt color via real game hook
// Применяется ТОЛЬКО когда игрок получает урон (alpha > 0 в хуке)

#include "HitColor.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>

void HitColor::onEnable()
{
    gFeatureManager->mDispatcher->listen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onDisable()
{
    gFeatureManager->mDispatcher->deafen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onHurtColorEvent(HurtColorEvent& event)
{
    if (!event.mColor) return;

    // Применяем ТОЛЬКО когда есть активный hurt overlay (alpha > 0)
    // Если alpha == 0 — игрок не получал урон, не трогаем
    if (event.mColor[3] <= 0.01f) return;

    if (mRainbowHurt.mValue) {
        float time = static_cast<float>(ImGui::GetTime());
        float hue = fmodf(time * 0.6f, 1.0f);
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
        event.setColor(r, g, b, event.mColor[3]); // сохраняем оригинальную alpha игры
    } else {
        ImColor c = mHurtColorValue.getAsImColor();
        // Если альфа в настройке > 0 — используем её, иначе оригинальную
        float alpha = c.Value.w > 0.01f ? c.Value.w : event.mColor[3];
        event.setColor(c.Value.x, c.Value.y, c.Value.z, alpha);
    }
}
