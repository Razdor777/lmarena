//
// Created by vastrakai on 6/29/2024.
//

#include "ClickGui.hpp"

#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/KeyEvent.hpp>
#include <Features/GUI/ModernDropdown.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

static ModernGui modernGui = ModernGui();

void ClickGui::onEnable()
{
    mWasGrabbed = !ClientInstance::get()->getMouseGrabbed();
    mGuiOpen = true;
    ClientInstance::get()->releaseMouse();

    gFeatureManager->mDispatcher->listen<MouseEvent, &ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<KeyEvent, &ClickGui::onKeyEvent, nes::event_priority::FIRST>(this);

    modernGui.onOpen();
}

void ClickGui::onDisable()
{
    gFeatureManager->mDispatcher->deafen<MouseEvent, &ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<KeyEvent, &ClickGui::onKeyEvent>(this);

    mGuiOpen = false;
    modernGui.onClose();

    if (mWasGrabbed)
        ClientInstance::get()->grabMouse();
}

void ClickGui::onWindowResizeEvent(WindowResizeEvent& event)
{
    modernGui.onWindowResizeEvent(event);
}

void ClickGui::onMouseEvent(MouseEvent& event)
{
    event.mCancelled = true;
}

void ClickGui::onKeyEvent(KeyEvent& event)
{
    if (modernGui.onKey(event.mKey, event.mPressed)) {
        event.mCancelled = true;
        return;
    }

    if (event.mKey == VK_ESCAPE && event.mPressed) {
        this->toggle();
        event.mCancelled = true;
        return;
    }

    if (event.mKey == VK_SHIFT)
        mIsPressingShift = event.mPressed;
}

float ClickGui::getEaseAnim(EasingUtil ease, int mode)
{
    switch (mode) {
    case 0: return ease.easeOutExpo();
    case 1: return mEnabled ? ease.easeOutElastic() : ease.easeOutBack();
    case 2: return 0.90f + 0.10f * ease.easeOutCubic();
    case 3: return mEnabled ? ease.easeOutElastic() : ease.easeInBack();
    default: return ease.easeOutExpo();
    }
}

void ClickGui::onRenderEvent(RenderEvent& event)
{
    if (mGuiOpen)
        ClientInstance::get()->releaseMouse();

    static float animation = 0;
    static EasingUtil inEase = EasingUtil();

    float delta = ImGui::GetIO().DeltaTime;

    mEnabled
        ? inEase.incrementPercentage(delta * mEaseSpeed.mValue / 10)
        : inEase.decrementPercentage(delta * 2 * mEaseSpeed.mValue / 10);

    float inScale = getEaseAnim(inEase, mAnimation.as<int>());
    if (inEase.isPercentageMax()) inScale = 1.f;
    if (mAnimation.mValue == ClickGuiAnimation::Zoom)
        inScale = MathUtils::clamp(inScale, 0.0f, 1.f);

    animation = MathUtils::lerp(0, 1, inEase.easeOutExpo());

    if (animation < 0.0001f) return;

    if (mStyle.mValue == ClickGuiStyle::Modern)
    {
        modernGui.render(animation, inScale,
            mBlurStrength.mValue, mMidclickRounding.mValue,
            mUiScale.mValue, mAmbientOrbs.mValue, mShowHints.mValue,
            mEffectIntensity.mValue, mPanelGlow.mValue,
            mCursorGlow.mValue, mParallax.mValue, mBackdropGrid.mValue);
    }
}
