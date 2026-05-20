#pragma once
//
// HitColor - Custom hurt color via real game hook + glow for players
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/HurtColorEvent.hpp>

class HitColor : public ModuleBase<HitColor>
{
public:
    // === HURT COLOR (real hook) ===
    BoolSetting mHurtColor = BoolSetting("Hurt Color", "Custom color when ANY entity takes damage (real hook)", true);
    ColorSetting mHurtColorValue = ColorSetting("Hurt Color Value", "The color overlay when hurt", 1.0f, 0.0f, 0.0f, 0.5f);
    BoolSetting mRainbowHurt = BoolSetting("Rainbow Hurt", "Cycle through rainbow colors on hurt", false);

    // === GLOW (ОТДЕЛЬНО) ===
    BoolSetting mGlow = BoolSetting("Glow", "Add glow effect around players", false);
    ColorSetting mGlowColor = ColorSetting("Glow Color", "Color of the glow effect", 1.0f, 1.0f, 1.0f, 0.8f);
    NumberSetting mGlowIntensity = NumberSetting("Glow Intensity", "Intensity of the glow", 1.0f, 0.1f, 3.0f, 0.1f);
    NumberSetting mGlowThickness = NumberSetting("Glow Thickness", "Thickness of the glow outline", 2.0f, 1.0f, 10.0f, 0.5f);

    HitColor() : ModuleBase("HitColor", "Custom hurt color (real hook!) and glow for players", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mHurtColor,
            &mHurtColorValue,
            &mRainbowHurt,
            &mGlow,
            &mGlowColor,
            &mGlowIntensity,
            &mGlowThickness
        );

        VISIBILITY_CONDITION(mHurtColorValue, mHurtColor.mValue && !mRainbowHurt.mValue);
        VISIBILITY_CONDITION(mRainbowHurt, mHurtColor.mValue);
        VISIBILITY_CONDITION(mGlowColor, mGlow.mValue);
        VISIBILITY_CONDITION(mGlowIntensity, mGlow.mValue);
        VISIBILITY_CONDITION(mGlowThickness, mGlow.mValue);

        mNames = {
            {Lowercase, "hitcolor"},
            {LowercaseSpaced, "hit color"},
            {Normal, "HitColor"},
            {NormalSpaced, "Hit Color"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onHurtColorEvent(HurtColorEvent& event);
};