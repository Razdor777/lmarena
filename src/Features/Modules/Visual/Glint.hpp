#pragma once
//
// Created by vastrakai on 10/19/2024.
//
// Glint — restyle the enchantment glint on items.
//
// There is no signature and no offset scan in here, on purpose. The module rides
// on RenderItemInHandHook, which detours
// mce::framebuilder::RenderItemInHandDescription::RenderItemInHandDescription
// (see SDK/SigManager.hpp — that is the only signature involved). The game hands
// us the description object already constructed, and the glint fields sit at
// fixed offsets inside it (RenderItemInHandHook.hpp), so all we do is overwrite
// them once per draw, after the original constructor has run.
//
#include <Features/Modules/Module.hpp>

class Glint : public ModuleBase<Glint>
{
public:
    enum class ColorMode
    {
        Theme,      // follows the client theme, wave and shimmer included
        Custom,     // one fixed colour
        Rainbow,    // hue cycles
        Gradient,   // cross-fade between Color and Color 2
    };

    enum class ExtraLayer
    {
        Off,
        Overlay,
        Change,
        Tint,
        All,
    };

    // ─────────────────────────── colour ───────────────────────────
    EnumSettingT<ColorMode> mMode = EnumSettingT<ColorMode>(
        "Mode", "Where the glint colour comes from", ColorMode::Theme,
        "Theme", "Custom", "Rainbow", "Gradient");

    ColorSetting mColor  = ColorSetting("Color",   "Custom colour / first Gradient stop",  0.55f, 0.40f, 1.00f, 1.f);
    ColorSetting mColor2 = ColorSetting("Color 2", "Second Gradient stop",                1.00f, 0.35f, 0.60f, 1.f);
    NumberSetting mCycle = NumberSetting("Cycle Speed", "Full colour cycles per second", 0.30f, 0.05f, 3.f, 0.05f);

    // ────────────────────────── intensity ─────────────────────────
    NumberSetting mSaturation = NumberSetting("Saturation", "How colourful the glint is (0 = grey)", 1.f, 0.f, 1.f, 0.01f);
    NumberSetting mBrightness = NumberSetting("Brightness", "Multiplies the glint colour", 1.f, 0.f, 3.f, 0.05f);
    NumberSetting mAlpha      = NumberSetting("Alpha",      "Glint opacity", 1.f, 0.f, 1.f, 0.05f);

    // ──────────────────────────── pulse ───────────────────────────
    BoolSetting   mPulse      = BoolSetting("Pulse", "Breathe the glint in and out", false);
    NumberSetting mPulseSpeed = NumberSetting("Pulse Speed", "Pulses per second", 0.80f, 0.05f, 5.f, 0.05f);
    NumberSetting mPulseDepth = NumberSetting("Pulse Depth", "How far the pulse dips", 0.60f, 0.f, 1.f, 0.05f);

    // ──────────────────────────── scope ───────────────────────────
    BoolSetting mHandOnly = BoolSetting("Hand Only",
        "Leave glint drawn inside the UI (inventory / hotbar icons) untouched", false);

    EnumSettingT<ExtraLayer> mExtra = EnumSettingT<ExtraLayer>("Extra Layers",
        "[offset untested] also recolour the overlay / change / multiplicative layers",
        ExtraLayer::Off, "Off", "Overlay", "Change", "Tint", "All");

    Glint() : ModuleBase("Glint",
        "Restyled enchantment glint: colour, brightness, pulse", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mMode, &mColor, &mColor2, &mCycle,
            &mSaturation, &mBrightness, &mAlpha,
            &mPulse, &mPulseSpeed, &mPulseDepth,
            &mHandOnly, &mExtra);

        // NOTE: the conditions must not capture a constructor-local by reference —
        // mIsVisible outlives the constructor. Reading the members straight through
        // the implicitly captured `this` is what HitColor does too.
        VISIBILITY_CONDITION(mColor,  mMode.as<ColorMode>() == ColorMode::Custom   ||
                                      mMode.as<ColorMode>() == ColorMode::Gradient);
        VISIBILITY_CONDITION(mColor2, mMode.as<ColorMode>() == ColorMode::Gradient);
        VISIBILITY_CONDITION(mCycle,  mMode.as<ColorMode>() == ColorMode::Rainbow  ||
                                      mMode.as<ColorMode>() == ColorMode::Gradient);
        VISIBILITY_CONDITION(mPulseSpeed, mPulse.mValue);
        VISIBILITY_CONDITION(mPulseDepth, mPulse.mValue);

        mNames = {
            {Lowercase, "glint"},
            {LowercaseSpaced, "glint"},
            {Normal, "Glint"},
            {NormalSpaced, "Glint"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderItemInHandDescriptionEvent(class RenderItemInHandDescriptionEvent& event);

private:
    // Colour before Saturation / Brightness / Pulse are applied.
    ImColor baseColor();

    // Alpha multiplier from the Pulse settings (1 when Pulse is off).
    float pulseFactor();

    // Own clock: the event only fires while the game draws an item, and that is
    // exactly when the animation needs to advance. ImGui's time is not used —
    // this hook can run outside an ImGui frame.
    float mClock    = 0.f;
    float mLastTime = 0.f;
};
