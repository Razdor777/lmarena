#include "HitColor.hpp"

#include <algorithm>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

namespace
{
    ImColor rainbowColor(float clock, float speed)
    {
        float hue = std::fmod(clock * speed, 1.f);
        if (hue < 0.f) hue += 1.f;
        return ImColor::HSV(hue, 1.f, 1.f);
    }
}

void HitColor::onEnable()
{
    gFeatureManager->mDispatcher->listen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);

    mShown      = ImColor(0.f, 0.f, 0.f, 0.f);
    mShownAlpha = 0.f;
    mFlashAnim  = 0.f;
    mClock      = 0.f;
    mLastStep   = 0.0;
    mFrameHurt  = false;
    mPrimed     = false;
}

void HitColor::onDisable()
{
    gFeatureManager->mDispatcher->deafen<HurtColorEvent, &HitColor::onHurtColorEvent>(this);

    mShown      = ImColor(0.f, 0.f, 0.f, 0.f);
    mShownAlpha = 0.f;
    mFlashAnim  = 0.f;
    mFrameHurt  = false;
    mPrimed     = false;
}

ImColor HitColor::idleColor()
{
    if (mRainbowAlways.mValue) return rainbowColor(mClock, mRainbowSpeed.mValue);
    if (mAlwaysMode.as<ColorMode>() == ColorMode::Theme) return ColorUtils::getThemedColor(0);

    ImColor c = mAlwaysColor.getAsImColor();
    return ImColor(c.Value.x, c.Value.y, c.Value.z, 1.f);
}

ImColor HitColor::damagedColor()
{
    if (mRainbowHurt.mValue) return rainbowColor(mClock, mRainbowSpeed.mValue);
    if (mHurtMode.as<ColorMode>() == ColorMode::Theme) return ColorUtils::getThemedColor(0);

    ImColor c = mHurtColor.getAsImColor();
    return ImColor(c.Value.x, c.Value.y, c.Value.z, 1.f);
}

void HitColor::onHurtColorEvent(HurtColorEvent& event)
{
    if (!event.mColor) return;

    // This hook can fire several times per frame (block overlay, entity flash,
    // ...) and we write into the very same buffer the game gave us. So the
    // game's own alpha is only trusted to *raise* the damage flag inside one
    // frame, and is never used to lower it — otherwise the alpha we wrote on
    // the first call would masquerade as "the game says we are hurt" on the
    // next one, which is what the old version did.
    const bool gameSaysHurt = event.mColor[3] > 0.01f;

    const double now = ImGui::GetTime();
    float dt = 0.f;

    if (mLastStep <= 0.0) mLastStep = now;

    if (now > mLastStep)
    {
        dt = std::clamp(static_cast<float>(now - mLastStep), 0.f, 0.1f);
        mLastStep = now;

        mClock += dt;

        if (mFlashAnim > 0.f)
        {
            mFlashAnim -= dt * 4.f;
            if (mFlashAnim < 0.f) mFlashAnim = 0.f;
        }

        // New frame: the damage flag starts from scratch.
        mFrameHurt = false;
    }

    if (gameSaysHurt) mFrameHurt = true;

    const bool hurt = mFrameHurt;

    // ---- pick the target colour ------------------------------------------
    ImColor target;
    float   targetAlpha;

    if (hurt)
    {
        if (!mHurtEnabled.mValue) return;   // leave the game's flash alone
        target = damagedColor();

        // Keep a little of the game's own flash intensity so the overlay still
        // breathes with the damage instead of being a flat tint.
        const float gameAlpha = std::clamp(event.mColor[3], 0.f, 1.f);
        targetAlpha = std::clamp(mHurtColor.mValue[3] * (0.55f + 0.45f * gameAlpha), 0.f, 1.f);

        if (mFlash.mValue && mFlashAnim <= 0.f) mFlashAnim = 1.f;
    }
    else
    {
        if (!mAlwaysEnabled.mValue) return;
        target = idleColor();
        targetAlpha = std::clamp(mAlwaysAlpha.mValue, 0.f, 1.f);
    }

    // ---- cross-fade towards it -------------------------------------------
    // dt is non-zero on the first call of a frame only; the later calls of the
    // same frame must reuse the colour we already faded to, never snap to the
    // target (that would defeat the fade completely).
    if (!mPrimed)
    {
        mShown      = target;
        mShownAlpha = targetAlpha;
        mPrimed     = true;
    }
    else if (dt > 0.f)
    {
        const float k = std::clamp(dt * mFadeSpeed.mValue, 0.f, 1.f);
        mShown.Value.x += (target.Value.x - mShown.Value.x) * k;
        mShown.Value.y += (target.Value.y - mShown.Value.y) * k;
        mShown.Value.z += (target.Value.z - mShown.Value.z) * k;
        mShownAlpha    += (targetAlpha      - mShownAlpha)    * k;
    }

    // ---- impact flash ----------------------------------------------------
    const float boost = 1.f + mFlashAnim * 0.75f;

    event.setColor(
        std::clamp(mShown.Value.x * boost, 0.f, 1.f),
        std::clamp(mShown.Value.y * boost, 0.f, 1.f),
        std::clamp(mShown.Value.z * boost, 0.f, 1.f),
        std::clamp(mShownAlpha * (1.f + mFlashAnim * 0.5f), 0.f, 1.f));
}
