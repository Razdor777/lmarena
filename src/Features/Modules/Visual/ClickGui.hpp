#pragma once
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>
#include <Features/GUI/ModernDropdown.hpp>

class ClickGui : public ModuleBase<ClickGui>
{
public:
    enum class ClickGuiStyle {
        Modern,
    };
    enum class ClickGuiAnimation {
        Zoom,
        Bounce
    };
    enum class ClickGuiTheme {
        Dark,
        Light,
        Cyber,
        Nord,
        Crimson,
        Mint
    };

    EnumSettingT<ClickGuiStyle>     mStyle     = EnumSettingT<ClickGuiStyle>("Style", "The style of the ClickGui.", ClickGuiStyle::Modern, "Modern");
    EnumSettingT<ClickGuiAnimation> mAnimation = EnumSettingT<ClickGuiAnimation>("Animation", "The animation of the ClickGui.", ClickGuiAnimation::Bounce, "Zoom", "Bounce");
    EnumSettingT<ClickGuiTheme>     mTheme     = EnumSettingT<ClickGuiTheme>("Background Style", "The background style of the ClickGui.", ClickGuiTheme::Dark, "Dark", "Light");
    NumberSetting mBlurStrength     = NumberSetting("Blur Strength", "The strength of the blur.", 7.f, 0.f, 20.f, 0.1f);
    NumberSetting mEaseSpeed        = NumberSetting("Ease Speed", "The speed of the easing.", 18.f, 5.f, 20.f, 0.1f);
    NumberSetting mMidclickRounding = NumberSetting("Midclick Rounding", "The value to round to when middle-clicking a NumberSetting.", 1.f, 0.01f, 1.f, 0.01f);

    ClickGui() : ModuleBase("ClickGui", "A customizable GUI for toggling modules.", ModuleCategory::Visual, VK_TAB, false) {
        gFeatureManager->mDispatcher->listen<RenderEvent, &ClickGui::onRenderEvent, nes::event_priority::LAST>(this);
        gFeatureManager->mDispatcher->listen<WindowResizeEvent, &ClickGui::onWindowResizeEvent>(this);
        addSetting(&mStyle);
        addSetting(&mAnimation);
        addSetting(&mBlurStrength);
        addSetting(&mEaseSpeed);
        addSetting(&mMidclickRounding);

        mNames = {
            {Lowercase,       "clickgui"},
            {LowercaseSpaced, "click gui"},
            {Normal,          "ClickGui"},
            {NormalSpaced,    "Click Gui"}
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

private:
    bool mLastMouseState=false;
    bool mWasMouseGrabbed=false;
    bool mIsPressingShift=false;
    bool mIsAnimatingClose=false;  // true пока идёт анимация закрытия
    ModernGui mModernGui;
};