#include "HitColor.hpp"
#include <Features/FeatureManager.hpp>

void HitColor::onEnable() {
    gFeatureManager->mDispatcher->listen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onDisable() {
    gFeatureManager->mDispatcher->deafen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);
}

void HitColor::onHurtColorEvent(HurtColorEvent& event) {
    if (!event.mColor) return;

    float gameAlpha = event.mColor[3];
    bool isHurt = gameAlpha > 0.01f;

    // При уроне
    if (isHurt && mHurtEnabled.mValue) {
        if (mRainbowHurt.mValue) {
            float time = static_cast<float>(ImGui::GetTime());
            float hue = fmodf(time * 0.6f, 1.0f);
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
            event.setColor(r, g, b, gameAlpha);
        } else {
            ImColor c = mHurtColor.getAsImColor();
            float alpha = c.Value.w > 0.01f ? c.Value.w : gameAlpha;
            event.setColor(c.Value.x, c.Value.y, c.Value.z, alpha);
        }
        return;
    }

    // Без урона
    if (mAlwaysEnabled.mValue) {
        float alpha = mAlwaysAlpha.mValue;

        if (mRainbowAlways.mValue) {
            float time = static_cast<float>(ImGui::GetTime());
            float hue = fmodf(time * 0.6f, 1.0f);
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
            event.setColor(r, g, b, alpha);
        } else {
            ImColor c = mAlwaysColor.getAsImColor();
            event.setColor(c.Value.x, c.Value.y, c.Value.z, alpha);
        }
    }
}