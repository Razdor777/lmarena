#pragma once
//
// Ported from Combatant TazikHat (visuals).
// The 3D bowl mesh is drawn as 2D projections (ImGui background draw list).
// See TazikHat.cpp for the list of dropped parts.
//

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

#include <cstdint>
#include <unordered_map>

class TazikHat : public ModuleBase<TazikHat>
{
public:
    enum class RenderTarget {
        Self,
        Players,
        Both
    };

    enum class ColorMode {
        Static,
        Rainbow,
        DoubleColor,
        Theme
    };

    EnumSettingT<RenderTarget> mRenderTarget = EnumSettingT(
        "Render Target", "Who gets a hat", RenderTarget::Both, "Self", "Players", "Both");
    EnumSettingT<ColorMode> mColorMode = EnumSettingT(
        "Color Mode", "How the hat is colored", ColorMode::Static, "Static", "Rainbow", "Double Color", "Theme");
    ColorSetting mPrimaryColor = ColorSetting("Primary Color", "Main color of the hat", 0xCC17C765ULL);
    ColorSetting mSecondaryColor = ColorSetting("Secondary Color", "Second color (Double Color mode)", 0xAA079C46ULL);
    ColorSetting mRimColor = ColorSetting("Rim Color", "Color of the brim", 0xEE52FFA0ULL);
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "Speed of animated colors", 18.f, 2.f, 54.f, 1.f);
    NumberSetting mScale = NumberSetting("Scale", "Size of the hat", 1.15f, 0.45f, 2.0f, 0.01f);
    NumberSetting mHelmetScale = NumberSetting("Helmet Scale", "Extra size when a helmet is worn", 1.16f, 1.0f, 1.45f, 0.01f);
    NumberSetting mHeadTopOffset = NumberSetting("Head Top Offset", "Height above the eyes", 0.255f, 0.05f, 0.45f, 0.005f);
    NumberSetting mHeadForwardOffset = NumberSetting("Head Forward Offset", "Shift forward from the head", 0.f, -0.25f, 0.25f, 0.005f);
    NumberSetting mHeadRightOffset = NumberSetting("Head Right Offset", "Shift right from the head", 0.f, -0.25f, 0.25f, 0.005f);
    NumberSetting mWobbleStrength = NumberSetting("Wobble Strength", "Idle sway amount", 1.f, 0.f, 2.5f, 0.05f);
    BoolSetting mDeathFlight = BoolSetting("Death Flight", "The hat flies off when the player dies", true);
    NumberSetting mDeathFlightPower = NumberSetting("Death Flight Power", "Launch strength of the flying hat", 1.f, 0.25f, 3.f, 0.05f);

    TazikHat() : ModuleBase("TazikHat", "Draws a tazik hat on players' heads", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mRenderTarget,
            &mColorMode,
            &mPrimaryColor,
            &mSecondaryColor,
            &mRimColor,
            &mColorSpeed,
            &mScale,
            &mHelmetScale,
            &mHeadTopOffset,
            &mHeadForwardOffset,
            &mHeadRightOffset,
            &mWobbleStrength,
            &mDeathFlight,
            &mDeathFlightPower
        );

        VISIBILITY_CONDITION(mColorSpeed, mColorMode.mValue == ColorMode::Rainbow || mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mSecondaryColor, mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mDeathFlightPower, mDeathFlight.mValue);

        mNames = {
            {Lowercase, "tazikhat"},
            {LowercaseSpaced, "tazik hat"},
            {Normal, "TazikHat"},
            {NormalSpaced, "Tazik Hat"}
        };
    }

    // A hat that was launched when its player died.
    struct FlyingTazik {
        float scale = 1.f;
        int seed = 0;
        uint64_t createdAtMs = 0;
        glm::vec3 pos = {0.f, 0.f, 0.f};
        glm::vec3 velocity = {0.f, 0.f, 0.f};
        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        float yawVelocity = 0.f;
        float pitchVelocity = 0.f;
        float rollVelocity = 0.f;
    };

    std::unordered_map<int64_t, uint64_t> mHandledDeaths = {};
    std::unordered_map<int64_t, FlyingTazik> mFlying = {};

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);

    void renderAttached(ImDrawList* drawList, class Actor* actor);
    void renderFlying(ImDrawList* drawList);
    void spawnFlying(class Actor* actor);
    bool updateFlying(FlyingTazik& tazik);
};
