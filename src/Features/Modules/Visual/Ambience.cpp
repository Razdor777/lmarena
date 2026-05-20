//
// Ambience Module Implementation
//

#include "Ambience.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <cmath>

void Ambience::onEnable() {
    // Initialize current values from settings
    memcpy(mCurrentSkyColor, mSkyColor.mValue, sizeof(float) * 4);
    memcpy(mCurrentFogColor, mFogColor.mValue, sizeof(float) * 4);
    memcpy(mCurrentCloudColor, mCloudColor.mValue, sizeof(float) * 4);
    memcpy(mCurrentSunColor, mSunColor.mValue, sizeof(float) * 4);
    memcpy(mCurrentMoonColor, mMoonColor.mValue, sizeof(float) * 4);

    // Apply preset if selected
    if (mPreset.mValue != 0) {
        applyPreset(static_cast<AmbiencePreset>(mPreset.mValue));
    }
}

void Ambience::onDisable() {
    // Values will be reset by hooks when module is disabled
}

void Ambience::onRenderEvent(RenderEvent& event) {
    if (!mEnabled) return;

    float deltaTime = ImGui::GetIO().DeltaTime;

    // Check if preset changed
    static int lastPreset = -1;
    if (lastPreset != mPreset.mValue) {
        lastPreset = mPreset.mValue;
        if (mPreset.mValue != 0) {
            applyPreset(static_cast<AmbiencePreset>(mPreset.mValue));
        }
    }

    updateCurrentValues(deltaTime);
}

void Ambience::updateCurrentValues(float deltaTime) {
    float lerpSpeed = 5.0f * deltaTime;

    // Get target colors
    float* targetSky = mSkyColor.mValue;
    float* targetFog = mFogColor.mValue;
    float* targetCloud = mCloudColor.mValue;
    float* targetSun = mSunColor.mValue;
    float* targetMoon = mMoonColor.mValue;

    // Lerp sky color
    for (int i = 0; i < 4; i++) {
        mCurrentSkyColor[i] = std::lerp(mCurrentSkyColor[i], targetSky[i], lerpSpeed);
        mCurrentFogColor[i] = std::lerp(mCurrentFogColor[i], targetFog[i], lerpSpeed);
        mCurrentCloudColor[i] = std::lerp(mCurrentCloudColor[i], targetCloud[i], lerpSpeed);
        mCurrentSunColor[i] = std::lerp(mCurrentSunColor[i], targetSun[i], lerpSpeed);
        mCurrentMoonColor[i] = std::lerp(mCurrentMoonColor[i], targetMoon[i], lerpSpeed);
    }

    // Sky animation
    if (mAnimateSky.mValue) {
        static float time = 0.0f;
        time += deltaTime * mSkyAnimSpeed.mValue;

        float pulse = sinf(time) * 0.1f;
        mCurrentSkyColor[0] = std::clamp(targetSky[0] + pulse, 0.0f, 1.0f);
        mCurrentSkyColor[1] = std::clamp(targetSky[1] + sinf(time * 1.3f) * 0.1f, 0.0f, 1.0f);
        mCurrentSkyColor[2] = std::clamp(targetSky[2] + sinf(time * 0.7f) * 0.1f, 0.0f, 1.0f);
    }

    // Sun animation
    if (mAnimateSun.mValue) {
        static float sunTime = 0.0f;
        sunTime += deltaTime * 2.0f;

        float pulse = 0.9f + sinf(sunTime) * 0.1f;
        mCurrentSunColor[0] = std::clamp(targetSun[0] * pulse, 0.0f, 1.0f);
        mCurrentSunColor[1] = std::clamp(targetSun[1] * pulse, 0.0f, 1.0f);
        mCurrentSunColor[2] = std::clamp(targetSun[2] * pulse, 0.0f, 1.0f);
    }
}

void Ambience::applyPreset(AmbiencePreset preset) {
    switch (preset) {
        case AmbiencePreset::Default:
            mSkyColor.setColor(0.5f, 0.7f, 1.0f, 1.0f);
            mFogColor.setColor(0.6f, 0.8f, 1.0f, 1.0f);
            mCloudColor.setColor(1.0f, 1.0f, 1.0f, 0.8f);
            mSunColor.setColor(1.0f, 0.95f, 0.8f, 1.0f);
            mMoonColor.setColor(0.8f, 0.85f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            mFogStart.setValue(30.0f);
            mFogEnd.setValue(128.0f);
            mFogDensity.setValue(0.3f);
            mCloudDistance.setValue(128.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::BloodMoon:
            mSkyColor.setColor(0.2f, 0.0f, 0.0f, 1.0f);
            mFogColor.setColor(0.3f, 0.0f, 0.0f, 1.0f);
            mCloudColor.setColor(0.5f, 0.0f, 0.0f, 0.9f);
            mSunColor.setColor(1.0f, 0.0f, 0.0f, 1.0f);
            mMoonColor.setColor(1.0f, 0.0f, 0.0f, 1.0f);
            mEndSkyColor.setColor(0.1f, 0.0f, 0.0f, 1.0f);
            mFogStart.setValue(10.0f);
            mFogEnd.setValue(50.0f);
            mFogDensity.setValue(0.7f);
            mCloudDistance.setValue(80.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(true);
            break;

        case AmbiencePreset::Vaporwave:
            mSkyColor.setColor(1.0f, 0.4f, 0.8f, 1.0f);
            mFogColor.setColor(0.8f, 0.2f, 0.6f, 1.0f);
            mCloudColor.setColor(1.0f, 0.6f, 0.9f, 0.7f);
            mSunColor.setColor(1.0f, 0.5f, 0.8f, 1.0f);
            mMoonColor.setColor(0.5f, 0.8f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.3f, 0.0f, 0.3f, 1.0f);
            mFogStart.setValue(20.0f);
            mFogEnd.setValue(80.0f);
            mFogDensity.setValue(0.5f);
            mCloudDistance.setValue(100.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.5f);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Noir:
            mSkyColor.setColor(0.3f, 0.3f, 0.3f, 1.0f);
            mFogColor.setColor(0.1f, 0.1f, 0.1f, 1.0f);
            mCloudColor.setColor(0.5f, 0.5f, 0.5f, 0.6f);
            mSunColor.setColor(1.0f, 1.0f, 1.0f, 1.0f);
            mMoonColor.setColor(0.9f, 0.9f, 0.9f, 1.0f);
            mEndSkyColor.setColor(0.05f, 0.05f, 0.05f, 1.0f);
            mFogStart.setValue(5.0f);
            mFogEnd.setValue(40.0f);
            mFogDensity.setValue(0.8f);
            mCloudDistance.setValue(60.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Alien:
            mSkyColor.setColor(0.0f, 0.8f, 0.3f, 1.0f);
            mFogColor.setColor(0.0f, 0.5f, 0.2f, 1.0f);
            mCloudColor.setColor(0.3f, 1.0f, 0.5f, 0.5f);
            mSunColor.setColor(0.0f, 1.0f, 0.5f, 1.0f);
            mMoonColor.setColor(0.5f, 0.0f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.0f, 0.2f, 0.1f, 1.0f);
            mFogStart.setValue(15.0f);
            mFogEnd.setValue(70.0f);
            mFogDensity.setValue(0.6f);
            mCloudDistance.setValue(90.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.3f);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::SilentHill:
            mSkyColor.setColor(0.7f, 0.7f, 0.7f, 1.0f);
            mFogColor.setColor(0.8f, 0.8f, 0.8f, 1.0f);
            mCloudColor.setColor(0.6f, 0.6f, 0.6f, 0.9f);
            mSunColor.setColor(0.9f, 0.9f, 0.9f, 0.5f);
            mMoonColor.setColor(0.7f, 0.7f, 0.7f, 0.5f);
            mEndSkyColor.setColor(0.3f, 0.3f, 0.3f, 1.0f);
            mFogStart.setValue(2.0f);
            mFogEnd.setValue(20.0f);
            mFogDensity.setValue(1.0f);
            mCloudDistance.setValue(30.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Underwater:
            mSkyColor.setColor(0.0f, 0.2f, 0.4f, 1.0f);
            mFogColor.setColor(0.0f, 0.3f, 0.5f, 1.0f);
            mCloudColor.setColor(0.2f, 0.5f, 0.7f, 0.4f);
            mSunColor.setColor(0.3f, 0.6f, 0.8f, 0.7f);
            mMoonColor.setColor(0.2f, 0.4f, 0.6f, 0.7f);
            mEndSkyColor.setColor(0.0f, 0.1f, 0.2f, 1.0f);
            mFogStart.setValue(1.0f);
            mFogEnd.setValue(15.0f);
            mFogDensity.setValue(0.7f);
            mCloudDistance.setValue(50.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(true);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.2f);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Abyss:
            mSkyColor.setColor(0.0f, 0.0f, 0.02f, 1.0f);
            mFogColor.setColor(0.0f, 0.0f, 0.05f, 1.0f);
            mCloudColor.setColor(0.05f, 0.05f, 0.1f, 0.3f);
            mSunColor.setColor(0.1f, 0.1f, 0.2f, 0.5f);
            mMoonColor.setColor(0.2f, 0.2f, 0.4f, 0.8f);
            mEndSkyColor.setColor(0.0f, 0.0f, 0.01f, 1.0f);
            mFogStart.setValue(5.0f);
            mFogEnd.setValue(30.0f);
            mFogDensity.setValue(0.9f);
            mCloudDistance.setValue(40.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Sunset:
            mSkyColor.setColor(1.0f, 0.4f, 0.2f, 1.0f);
            mFogColor.setColor(0.9f, 0.5f, 0.3f, 1.0f);
            mCloudColor.setColor(1.0f, 0.6f, 0.3f, 0.8f);
            mSunColor.setColor(1.0f, 0.6f, 0.2f, 1.0f);
            mMoonColor.setColor(0.9f, 0.7f, 0.5f, 1.0f);
            mEndSkyColor.setColor(0.2f, 0.05f, 0.0f, 1.0f);
            mFogStart.setValue(20.0f);
            mFogEnd.setValue(100.0f);
            mFogDensity.setValue(0.4f);
            mCloudDistance.setValue(120.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.3f);
            mAnimateSun.setValue(true);
            break;

        case AmbiencePreset::Aurora:
            mSkyColor.setColor(0.0f, 0.1f, 0.2f, 1.0f);
            mFogColor.setColor(0.0f, 0.15f, 0.25f, 1.0f);
            mCloudColor.setColor(0.1f, 0.4f, 0.3f, 0.5f);
            mSunColor.setColor(0.3f, 1.0f, 0.5f, 1.0f);
            mMoonColor.setColor(0.4f, 0.8f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.0f, 0.05f, 0.1f, 1.0f);
            mFogStart.setValue(30.0f);
            mFogEnd.setValue(150.0f);
            mFogDensity.setValue(0.2f);
            mCloudDistance.setValue(200.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.8f);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Neon:
            mSkyColor.setColor(0.05f, 0.0f, 0.15f, 1.0f);
            mFogColor.setColor(0.1f, 0.0f, 0.2f, 1.0f);
            mCloudColor.setColor(0.8f, 0.0f, 1.0f, 0.4f);
            mSunColor.setColor(0.0f, 1.0f, 1.0f, 1.0f);
            mMoonColor.setColor(1.0f, 0.0f, 0.8f, 1.0f);
            mEndSkyColor.setColor(0.02f, 0.0f, 0.05f, 1.0f);
            mFogStart.setValue(10.0f);
            mFogEnd.setValue(60.0f);
            mFogDensity.setValue(0.5f);
            mCloudDistance.setValue(80.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(1.0f);
            mAnimateSun.setValue(true);
            break;

        case AmbiencePreset::Sakura:
            mSkyColor.setColor(1.0f, 0.8f, 0.85f, 1.0f);
            mFogColor.setColor(1.0f, 0.85f, 0.9f, 1.0f);
            mCloudColor.setColor(1.0f, 0.9f, 0.95f, 0.7f);
            mSunColor.setColor(1.0f, 0.85f, 0.9f, 1.0f);
            mMoonColor.setColor(0.95f, 0.8f, 0.9f, 1.0f);
            mEndSkyColor.setColor(0.3f, 0.15f, 0.2f, 1.0f);
            mFogStart.setValue(40.0f);
            mFogEnd.setValue(180.0f);
            mFogDensity.setValue(0.2f);
            mCloudDistance.setValue(150.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Ember:
            mSkyColor.setColor(0.15f, 0.05f, 0.0f, 1.0f);
            mFogColor.setColor(0.2f, 0.08f, 0.0f, 1.0f);
            mCloudColor.setColor(0.6f, 0.2f, 0.0f, 0.7f);
            mSunColor.setColor(1.0f, 0.4f, 0.0f, 1.0f);
            mMoonColor.setColor(0.8f, 0.3f, 0.0f, 1.0f);
            mEndSkyColor.setColor(0.08f, 0.02f, 0.0f, 1.0f);
            mFogStart.setValue(8.0f);
            mFogEnd.setValue(45.0f);
            mFogDensity.setValue(0.6f);
            mCloudDistance.setValue(70.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.4f);
            mAnimateSun.setValue(true);
            break;

        case AmbiencePreset::Arctic:
            mSkyColor.setColor(0.7f, 0.85f, 1.0f, 1.0f);
            mFogColor.setColor(0.8f, 0.9f, 1.0f, 1.0f);
            mCloudColor.setColor(0.95f, 0.97f, 1.0f, 0.9f);
            mSunColor.setColor(0.9f, 0.95f, 1.0f, 1.0f);
            mMoonColor.setColor(0.8f, 0.9f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.2f, 0.25f, 0.3f, 1.0f);
            mFogStart.setValue(15.0f);
            mFogEnd.setValue(60.0f);
            mFogDensity.setValue(0.5f);
            mCloudDistance.setValue(90.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(false);
            mAnimateSun.setValue(false);
            break;

        case AmbiencePreset::Twilight:
            mSkyColor.setColor(0.15f, 0.1f, 0.35f, 1.0f);
            mFogColor.setColor(0.2f, 0.12f, 0.4f, 1.0f);
            mCloudColor.setColor(0.3f, 0.2f, 0.5f, 0.6f);
            mSunColor.setColor(1.0f, 0.6f, 0.3f, 1.0f);
            mMoonColor.setColor(0.6f, 0.5f, 1.0f, 1.0f);
            mEndSkyColor.setColor(0.05f, 0.03f, 0.1f, 1.0f);
            mFogStart.setValue(25.0f);
            mFogEnd.setValue(90.0f);
            mFogDensity.setValue(0.4f);
            mCloudDistance.setValue(100.0f);
            mDisableFog.setValue(false);
            mDisableClouds.setValue(false);
            mAnimateSky.setValue(true);
            mSkyAnimSpeed.setValue(0.5f);
            mAnimateSun.setValue(false);
            break;

        default:
            break;
    }
}