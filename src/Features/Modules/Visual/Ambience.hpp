#pragma once
//
// Ambience Module - Custom Sky, Fog, Clouds, Sun/Moon colors
// Created based on reverse engineering of Minecraft Bedrock rendering functions
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>

enum class AmbiencePreset {
    Custom,
    Default,
    BloodMoon,
    Vaporwave,
    Noir,
    Alien,
    SilentHill,
    Underwater,
    Abyss,
    // Extended presets
    Sunset,
    Aurora,
    Neon,
    Sakura,
    Ember,
    Arctic,
    Twilight
};

class Ambience : public ModuleBase<Ambience> {
public:
    // === PRESETS ===
    EnumSetting mPreset = EnumSetting("Preset", "Quick preset selection", 0,
        "Custom", "Default", "Blood Moon", "Vaporwave", "Noir", "Alien", "Silent Hill", "Underwater", "Abyss",
        "Sunset", "Aurora", "Neon", "Sakura", "Ember", "Arctic", "Twilight");

    // === SKY ===
    BoolSetting mModifySky = BoolSetting("Modify Sky", "Enable custom sky color", true);
    ColorSetting mSkyColor = ColorSetting("Sky Color", "Color of the sky", 0.5f, 0.7f, 1.0f, 1.0f);
    BoolSetting mAnimateSky = BoolSetting("Animate Sky", "Animate sky color over time", false);
    NumberSetting mSkyAnimSpeed = NumberSetting("Sky Anim Speed", "Speed of sky animation", 0.5f, 0.1f, 3.0f, 0.1f);

    // === END SKY ===
    BoolSetting mModifyEndSky = BoolSetting("Modify End Sky", "Enable custom End dimension sky", true);
    ColorSetting mEndSkyColor = ColorSetting("End Sky Color", "Color of the End sky", 0.0f, 0.0f, 0.0f, 1.0f);

    // === FOG ===
    BoolSetting mModifyFog = BoolSetting("Modify Fog", "Enable custom fog settings", true);
    BoolSetting mDisableFog = BoolSetting("Disable Fog", "Completely disable fog", false);
    ColorSetting mFogColor = ColorSetting("Fog Color", "Color of the fog", 0.6f, 0.8f, 1.0f, 1.0f);
    NumberSetting mFogStart = NumberSetting("Fog Start", "Distance where fog starts", 30.0f, 0.0f, 500.0f, 1.0f);
    NumberSetting mFogEnd = NumberSetting("Fog End", "Distance where fog is fully opaque", 128.0f, 1.0f, 1000.0f, 1.0f);
    NumberSetting mFogDensity = NumberSetting("Fog Density", "Density of the fog", 0.3f, 0.0f, 1.0f, 0.05f);

    // === CLOUDS ===
    BoolSetting mModifyClouds = BoolSetting("Modify Clouds", "Enable custom cloud settings", true);
    BoolSetting mDisableClouds = BoolSetting("Disable Clouds", "Completely disable clouds", false);
    ColorSetting mCloudColor = ColorSetting("Cloud Color", "Color of the clouds", 1.0f, 1.0f, 1.0f, 0.8f);
    NumberSetting mCloudDistance = NumberSetting("Cloud Distance", "Cloud render distance", 128.0f, 16.0f, 512.0f, 8.0f);

    // === SUN/MOON ===
    BoolSetting mModifySunMoon = BoolSetting("Modify Sun/Moon", "Enable custom sun/moon colors", true);
    ColorSetting mSunColor = ColorSetting("Sun Color", "Color of the sun", 1.0f, 0.95f, 0.8f, 1.0f);
    ColorSetting mMoonColor = ColorSetting("Moon Color", "Color of the moon", 0.8f, 0.85f, 1.0f, 1.0f);
    BoolSetting mAnimateSun = BoolSetting("Animate Sun", "Pulse animation for sun", false);

    Ambience() : ModuleBase("Ambience", "Customize world visuals: sky, fog, clouds, sun/moon",
                            ModuleCategory::Visual, 0, false) {
        mNames = {
            {Lowercase, "ambience"},
            {LowercaseSpaced, "ambience"},
            {Normal, "Ambience"},
            {NormalSpaced, "Ambience"}
        };

        // Add all settings
        addSettings(
            &mPreset,
            // Sky
            &mModifySky,
            &mSkyColor,
            &mAnimateSky,
            &mSkyAnimSpeed,
            // End Sky
            &mModifyEndSky,
            &mEndSkyColor,
            // Fog
            &mModifyFog,
            &mDisableFog,
            &mFogColor,
            &mFogStart,
            &mFogEnd,
            &mFogDensity,
            // Clouds
            &mModifyClouds,
            &mDisableClouds,
            &mCloudColor,
            &mCloudDistance,
            // Sun/Moon
            &mModifySunMoon,
            &mSunColor,
            &mMoonColor,
            &mAnimateSun
        );

        // Set visibility conditions
        VISIBILITY_CONDITION(mSkyColor, mModifySky.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mAnimateSky, mModifySky.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mSkyAnimSpeed, mModifySky.mValue && mAnimateSky.mValue && mPreset.mValue == 0);

        VISIBILITY_CONDITION(mEndSkyColor, mModifyEndSky.mValue && mPreset.mValue == 0);

        VISIBILITY_CONDITION(mDisableFog, mModifyFog.mValue);
        VISIBILITY_CONDITION(mFogColor, mModifyFog.mValue && !mDisableFog.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mFogStart, mModifyFog.mValue && !mDisableFog.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mFogEnd, mModifyFog.mValue && !mDisableFog.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mFogDensity, mModifyFog.mValue && !mDisableFog.mValue && mPreset.mValue == 0);

        VISIBILITY_CONDITION(mDisableClouds, mModifyClouds.mValue);
        VISIBILITY_CONDITION(mCloudColor, mModifyClouds.mValue && !mDisableClouds.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mCloudDistance, mModifyClouds.mValue && !mDisableClouds.mValue && mPreset.mValue == 0);

        VISIBILITY_CONDITION(mSunColor, mModifySunMoon.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mMoonColor, mModifySunMoon.mValue && mPreset.mValue == 0);
        VISIBILITY_CONDITION(mAnimateSun, mModifySunMoon.mValue && mPreset.mValue == 0);

        // Register events
        gFeatureManager->mDispatcher->listen<RenderEvent, &Ambience::onRenderEvent>(this);
    }

    // Current values (for smooth transitions)
    float mCurrentSkyColor[4] = {0.5f, 0.7f, 1.0f, 1.0f};
    float mCurrentFogColor[4] = {0.6f, 0.8f, 1.0f, 1.0f};
    float mCurrentCloudColor[4] = {1.0f, 1.0f, 1.0f, 0.8f};
    float mCurrentSunColor[4] = {1.0f, 0.95f, 0.8f, 1.0f};
    float mCurrentMoonColor[4] = {0.8f, 0.85f, 1.0f, 1.0f};

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

    void applyPreset(AmbiencePreset preset);
    void updateCurrentValues(float deltaTime);

    // Getters for hooks
    bool shouldModifySky() const { return mEnabled && mModifySky.mValue; }
    bool shouldModifyEndSky() const { return mEnabled && mModifyEndSky.mValue; }
    bool shouldModifyFog() const { return mEnabled && mModifyFog.mValue; }
    bool shouldDisableFog() const { return mEnabled && mModifyFog.mValue && mDisableFog.mValue; }
    bool shouldModifyClouds() const { return mEnabled && mModifyClouds.mValue; }
    bool shouldDisableClouds() const { return mEnabled && mModifyClouds.mValue && mDisableClouds.mValue; }
    bool shouldModifySunMoon() const { return mEnabled && mModifySunMoon.mValue; }

    float* getSkyColor() { return mCurrentSkyColor; }
    float* getEndSkyColor() { return mEndSkyColor.mValue; }
    float* getFogColor() { return mCurrentFogColor; }
    float getFogStart() const { return mFogStart.mValue; }
    float getFogEnd() const { return mFogEnd.mValue; }
    float getFogDensity() const { return mFogDensity.mValue; }
    float* getCloudColor() { return mCurrentCloudColor; }
    float getCloudDistance() const { return mCloudDistance.mValue; }
    float* getSunColor() { return mCurrentSunColor; }
    float* getMoonColor() { return mCurrentMoonColor; }

    std::string getSettingDisplay() override {
        if (mPreset.mValue != 0) {
            return mPreset.mValues[mPreset.mValue];
        }
        return "";
    }
};