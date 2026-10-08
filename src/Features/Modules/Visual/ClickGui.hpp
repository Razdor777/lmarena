#pragma once
//
// Created by vastrakai on 6/29/2024.
//

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>

class ClickGui : public ModuleBase<ClickGui>
{
public:
    enum class ClickGuiStyle {
        Modern,
    };
    enum class ClickGuiAnimation {
        Zoom,
        Bounce,
        Float,
        Elastic
    };

    EnumSettingT<ClickGuiStyle> mStyle = EnumSettingT<ClickGuiStyle>(
        "Style", "The style of the ClickGui.", ClickGuiStyle::Modern, "Modern");
    EnumSettingT<ClickGuiAnimation> mAnimation = EnumSettingT<ClickGuiAnimation>(
        "Animation", "The animation of the ClickGui.", ClickGuiAnimation::Bounce,
        "Zoom", "Bounce", "Float", "Elastic");
    NumberSetting mBlurStrength = NumberSetting("Blur Strength", "The strength of the blur.", 7.f, 0.f, 20.f, 0.1f);
    NumberSetting mEaseSpeed = NumberSetting("Ease Speed", "The speed of the easing.", 18.f, 5.f, 20.f, 0.1f);
    NumberSetting mMidclickRounding = NumberSetting("Midclick Rounding",
        "The value to round to when middle-clicking a NumberSetting.", 1.f, 0.01f, 1.f, 0.01f);
    NumberSetting mUiScale = NumberSetting("UI Scale", "Global scale of the dropdown.", 1.f, 0.75f, 1.5f, 0.05f);
    BoolSetting mAmbientOrbs = BoolSetting("Ambient Orbs", "Soft glowing orbs in the background.", true);
    BoolSetting mBackdropGrid = BoolSetting("Motion Grid", "Subtle moving depth grid behind the panels.", true);
    BoolSetting mPanelGlow = BoolSetting("Panel Glow", "Theme-colored focus glow around panels.", true);
    BoolSetting mCursorGlow = BoolSetting("Cursor Trail", "Soft theme trail and halo around the cursor.", true);
    BoolSetting mParallax = BoolSetting("Parallax", "Ambient effects react to cursor movement.", true);
    NumberSetting mEffectIntensity = NumberSetting("Effect Intensity", "Strength of ambient motion and glow.", 1.f, 0.25f, 2.f, 0.05f);
    BoolSetting mShowHints = BoolSetting("Show Hints", "Show control hints for a few seconds after opening.", true);

    bool mIsPressingShift = false;
    bool mWasGrabbed = false;
    bool mGuiOpen = false;

    ClickGui() : ModuleBase("ClickGui", "A customizable GUI for toggling modules.", ModuleCategory::Visual, VK_TAB, false)
    {
        gFeatureManager->mDispatcher->listen<RenderEvent, &ClickGui::onRenderEvent, nes::event_priority::LAST>(this);
        gFeatureManager->mDispatcher->listen<WindowResizeEvent, &ClickGui::onWindowResizeEvent>(this);
        addSetting(&mStyle);
        addSetting(&mAnimation);
        addSetting(&mBlurStrength);
        addSetting(&mEaseSpeed);
        addSetting(&mMidclickRounding);
        addSetting(&mUiScale);
        addSetting(&mAmbientOrbs);
        addSetting(&mBackdropGrid);
        addSetting(&mPanelGlow);
        addSetting(&mCursorGlow);
        addSetting(&mParallax);
        addSetting(&mEffectIntensity);
        addSetting(&mShowHints);

        mNames = {
            {Lowercase, "clickgui"},
            {LowercaseSpaced, "click gui"},
            {Normal, "ClickGui"},
            {NormalSpaced, "Click Gui"}
        };
    }

    void onEnable() override;
    void onDisable() override;

    void onWindowResizeEvent(class WindowResizeEvent& event);
    void onMouseEvent(class MouseEvent& event);
    void onKeyEvent(class KeyEvent& event);
    float getEaseAnim(EasingUtil ease, int mode);
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        return mStyle.mValues[mStyle.as<int>()];
    }
};
