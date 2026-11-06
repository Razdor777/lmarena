#include "Glint.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Features/Events/SetupShaderParameterEvent.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

namespace
{
    constexpr float kTwoPi = 6.28318530718f;
}

void Glint::onEnable()
{
    mClock    = 0.f;
    mLastTime = 0.f;

    gFeatureManager->mDispatcher->listen<SetupShaderParameterEvent, &Glint::onSetupShaderParameterEvent>(this);
}

void Glint::onDisable()
{
    gFeatureManager->mDispatcher->deafen<SetupShaderParameterEvent, &Glint::onSetupShaderParameterEvent>(this);
}

ImColor Glint::baseColor()
{
    switch (mMode.as<ColorMode>())
    {
    case ColorMode::Theme:
        return ColorUtils::getThemedColor(0);

    case ColorMode::Rainbow:
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

    const float wave = 0.5f - 0.5f * std::cos(mClock * kTwoPi * mPulseSpeed.mValue);
    return std::clamp(1.f - mPulseDepth.mValue * wave, 0.f, 1.f);
}

void Glint::onSetupShaderParameterEvent(SetupShaderParameterEvent& event)
{
    if (!event.mglintColor) return;

    const float now = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (mLastTime <= 0.f) mLastTime = now;

    const float dt = now - mLastTime;
    if (dt > 0.f)
    {
        mClock += std::clamp(dt, 0.f, 0.1f);
        mLastTime = now;
    }

    const ImColor color = ColorUtils::saturate(baseColor(), mSaturation.mValue);

    const float brightness = mBrightness.mValue;
    const float alpha = std::clamp(mAlpha.mValue, 0.f, 1.f) * pulseFactor();
    const glm::vec4 rgba(
        std::clamp(color.Value.x * brightness, 0.f, 1.f),
        std::clamp(color.Value.y * brightness, 0.f, 1.f),
        std::clamp(color.Value.z * brightness, 0.f, 1.f),
        alpha);

    *event.mglintColor = rgba;

    const auto layer = mExtra.as<ExtraLayer>();
    if ((layer == ExtraLayer::Overlay || layer == ExtraLayer::All) && event.moverlay)
    {
        *event.moverlay = rgba;
    }
    if ((layer == ExtraLayer::Change || layer == ExtraLayer::All) && event.mchangeColor)
    {
        *event.mchangeColor = rgba;
    }
    if ((layer == ExtraLayer::Tint || layer == ExtraLayer::All) && event.mchangeColor2)
    {
        *event.mchangeColor2 = rgba;
    }
}
