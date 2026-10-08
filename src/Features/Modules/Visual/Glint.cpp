//
// Created by vastrakai on 10/19/2024.
//

#include "Glint.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Features/Events/RenderItemInHandDescriptionEvent.hpp>
#include <Hook/Hooks/RenderHooks/RenderItemInHandHook.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

namespace
{
    constexpr float kTwoPi = 6.28318530718f;
}

void Glint::onEnable()
{
    mClock    = 0.f;
    mLastTime = 0.f;

    gFeatureManager->mDispatcher->listen<RenderItemInHandDescriptionEvent, &Glint::onRenderItemInHandDescriptionEvent>(this);
}

void Glint::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderItemInHandDescriptionEvent, &Glint::onRenderItemInHandDescriptionEvent>(this);
}

ImColor Glint::baseColor()
{
    switch (mMode.as<ColorMode>())
    {
    case ColorMode::Theme:
        return ColorUtils::getThemedColor(0);

    case ColorMode::Rainbow:
        // ColorUtils::Rainbow takes the period in seconds, so our "cycles per
        // second" slider is simply its reciprocal.
        return ColorUtils::Rainbow(1.f / (std::max)(mCycle.mValue, 0.05f), 1.f, 1.f, 0);

    case ColorMode::Gradient:
    {
        const float t = 0.5f - 0.5f * std::cos(mClock * kTwoPi * mCycle.mValue);
        const ImColor a = mColor.getAsImColor();
        const ImColor b = mColor2.getAsImColor();
        return ImColor(
            a.Value.x + (b.Value.x - a.Value.x) * t,
            a.Value.y + (b.Value.y - a.Value.y) * t,
            a.Value.z + (b.Value.z - a.Value.z) * t,
            1.f);
    }

    case ColorMode::Custom:
    default:
        return mColor.getAsImColor();
    }
}

float Glint::pulseFactor()
{
    if (!mPulse.mValue) return 1.f;

    // Starts at full strength, dips by Pulse Depth, comes back.
    const float wave = 0.5f - 0.5f * std::cos(mClock * kTwoPi * mPulseSpeed.mValue);
    return std::clamp(1.f - mPulseDepth.mValue * wave, 0.f, 1.f);
}

void Glint::onRenderItemInHandDescriptionEvent(RenderItemInHandDescriptionEvent& event)
{
    if (!event.mThis) return;

    // "Hand Only" skips the UI pass, so inventory / hotbar icons keep the
    // vanilla glint.
    if (mHandOnly.mValue && event.mIsDrawingUI) return;

    // Own clock. The event only fires while the game draws an item, which is
    // exactly when the animation needs to move, and a hitch is clamped so it
    // never jumps. ImGui's clock is not used here on purpose: this hook can run
    // outside an ImGui frame.
    const float now = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (mLastTime <= 0.f) mLastTime = now;

    const float dt = now - mLastTime;
    if (dt > 0.f)
    {
        mClock += std::clamp(dt, 0.f, 0.1f);
        mLastTime = now;
    }

    // Real HSL desaturation, so Saturation 0 gives a grey glint instead of black.
    const ImColor color = ColorUtils::saturate(baseColor(), mSaturation.mValue);

    const float brightness = mBrightness.mValue;
    const glm::vec3 rgb(
        std::clamp(color.Value.x * brightness, 0.f, 1.f),
        std::clamp(color.Value.y * brightness, 0.f, 1.f),
        std::clamp(color.Value.z * brightness, 0.f, 1.f));

    const float alpha = std::clamp(mAlpha.mValue, 0.f, 1.f) * pulseFactor();

    // Written only after the original constructor has run, so the game cannot
    // overwrite our values afterwards.
    event.mThis->mGlintColor = rgb;
    event.mThis->mGlintAlpha = alpha;

    // Optional extra layers. These offsets come from the same header as the glint
    // ones, but unlike them they were never verified against the game — which is
    // why this stays Off by default.
    const auto layer = mExtra.as<ExtraLayer>();
    if (layer == ExtraLayer::Overlay || layer == ExtraLayer::All)
    {
        event.mThis->mOverlayColor = rgb;
        event.mThis->mOverlayAlpha = alpha;
    }
    if (layer == ExtraLayer::Change || layer == ExtraLayer::All)
    {
        event.mThis->mChangeColor = rgb;
        event.mThis->mChangeAlpha = alpha;
    }
    if (layer == ExtraLayer::Tint || layer == ExtraLayer::All)
    {
        event.mThis->mMultiplicativeTintColor = rgb;
        event.mThis->mMultiplicativeTintColorAlpha = alpha;
    }
}
