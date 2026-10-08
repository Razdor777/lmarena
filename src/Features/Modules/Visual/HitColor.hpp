#pragma once
//
// HitColor — recolours the overlay the game draws on the block you are hitting
// (the "highlight" you see while aiming/mining) and the flash you get when you
// take damage.
//
// What is new compared to the old version:
//   * Theme mode — the colour comes from the client theme, so it inherits the
//     theme's animations instead of being a fixed rgb value,
//   * a real cross-fade between the idle colour and the damage colour instead
//     of the game's hard on/off switch (the overlay no longer pops),
//   * a short bright pulse on impact,
//   * and a fix for the old alpha feedback loop: the game's own alpha was
//     overwritten by us and then read back on the next call, so every overlay
//     after the first one looked like a damage frame.
//
#include <Features/Modules/Module.hpp>
#include <Features/Events/HurtColorEvent.hpp>

class HitColor : public ModuleBase<HitColor>
{
public:
    enum class ColorMode {
        Custom,
        Theme,
    };

    // ---------------- idle (no damage) ----------------
    BoolSetting mAlwaysEnabled = BoolSetting("AlwaysEnabled", "Use a colour even when you are not hurt", true);

    EnumSettingT<ColorMode> mAlwaysMode = EnumSettingT<ColorMode>(
        "Always Mode", "Where the idle colour comes from", ColorMode::Custom, "Custom", "Theme");

    ColorSetting  mAlwaysColor   = ColorSetting("AlwaysColor", "Colour in the idle state", 0.0f, 1.0f, 0.0f, 0.35f);
    NumberSetting mAlwaysAlpha   = NumberSetting("AlwaysAlpha", "Opacity in the idle state", 0.35f, 0.05f, 1.0f, 0.05f);
    BoolSetting   mRainbowAlways = BoolSetting("RainbowAlways", "Cycle the idle hue", false);

    // ---------------- on damage ----------------
    BoolSetting mHurtEnabled = BoolSetting("HurtEnabled", "Recolour the damage flash", true);

    EnumSettingT<ColorMode> mHurtMode = EnumSettingT<ColorMode>(
        "Hurt Mode", "Where the damage colour comes from", ColorMode::Custom, "Custom", "Theme");

    ColorSetting mHurtColor   = ColorSetting("HurtColor", "Colour while taking damage", 0.0f, 0.3f, 1.0f, 0.7f);
    BoolSetting  mRainbowHurt = BoolSetting("RainbowHurt", "Cycle the damage hue", false);

    // ---------------- shared ----------------
    NumberSetting mRainbowSpeed = NumberSetting("Rainbow Speed", "Hue cycling speed", 0.6f, 0.05f, 4.f, 0.05f);
    NumberSetting mFadeSpeed    = NumberSetting("Fade Speed", "How fast the colours cross-fade", 14.f, 2.f, 40.f, 1.f);
    BoolSetting   mFlash        = BoolSetting("Flash", "Bright pulse the moment you get hit", true);

    HitColor() : ModuleBase("HitColor",
        "Highlight (block overlay) + damage flash colours", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mAlwaysEnabled, &mAlwaysMode, &mAlwaysColor, &mAlwaysAlpha, &mRainbowAlways,
            &mHurtEnabled, &mHurtMode, &mHurtColor, &mRainbowHurt,
            &mRainbowSpeed, &mFadeSpeed, &mFlash
        );

        VISIBILITY_CONDITION(mAlwaysColor, !mRainbowAlways.mValue && mAlwaysEnabled.mValue);
        VISIBILITY_CONDITION(mAlwaysAlpha, !mRainbowAlways.mValue && mAlwaysEnabled.mValue);
        VISIBILITY_CONDITION(mHurtColor,   !mRainbowHurt.mValue && mHurtEnabled.mValue);

        mNames = {
            {Lowercase,       "hitcolor"},
            {LowercaseSpaced, "hit color"},
            {Normal,          "HitColor"},
            {NormalSpaced,    "Hit Color"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onHurtColorEvent(HurtColorEvent& event);

private:
    // Cross-faded colour actually sent to the game.
    ImColor mShown{0.f, 0.f, 0.f, 0.f};
    float   mShownAlpha = 0.f;

    float  mFlashAnim = 0.f;    // 1 -> 0 impact pulse
    float  mClock     = 0.f;    // own clock for the hue cycling
    double mLastStep  = 0.0;    // last frame we advanced the animation
    bool   mFrameHurt = false;  // did anything this frame report damage?
    bool   mPrimed    = false;  // first call: start at the target colour, not at black

    // Not const: ColorSetting::getAsImColor() is non-const.
    ImColor idleColor();
    ImColor damagedColor();
};
