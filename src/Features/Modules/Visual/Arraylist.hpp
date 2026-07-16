#pragma once
#include <Features/Modules/Module.hpp>
#include <unordered_map>

class Arraylist : public ModuleBase<Arraylist>
{
public:
    enum class BackgroundStyle {
        None,
    };

    enum class Display {
        Outline,
        Bar,
        Split,
        None
    };

    enum class ModuleVisibility {
        All,
        Bound,
    };

    EnumSettingT<BackgroundStyle> mBackground = EnumSettingT("Background", "Background style", BackgroundStyle::None, "None");
    EnumSettingT<Display> mDisplay = EnumSettingT("Display", "Outline style", Display::None, "None");
    EnumSettingT<ModuleVisibility> mVisibility = EnumSettingT("Visibility", "Module visibility", ModuleVisibility::All, "All", "Bound");
    BoolSetting mRenderMode = BoolSetting("Render Mode", "Renders the module mode next to the module name", true);
    BoolSetting mGlow = BoolSetting("Glow", "Enables glow", true);
    NumberSetting mGlowStrength = NumberSetting("Glow Strength", "The strength of the glow", 0.8f, 0.2f, 1.f, 0.05f);
    BoolSetting mBoldText = BoolSetting("Bold Text", "Makes the text bold", false);
    NumberSetting mFontSize = NumberSetting("Font Size", "The size of the font", 23.f, 10.f, 40.f, 0.01f);
    NumberSetting mScale = NumberSetting("Scale", "Overall scale of the arraylist", 1.f, 0.5f, 2.f, 0.01f);
    
    NumberSetting mEnableAnimSpeed = NumberSetting("Enable Speed", "Animation speed on enable", 14.f, 5.f, 40.f, 0.5f);
    NumberSetting mDisableAnimSpeed = NumberSetting("Disable Speed", "Animation speed on disable", 8.f, 1.f, 20.f, 0.5f);
    BoolSetting mElasticAnim = BoolSetting("Elastic Animation", "Elastic pop-in effect on enable", true);
    BoolSetting mStaggerAnim = BoolSetting("Stagger Animation", "Staggered slide-in effect", true);

    Arraylist() : ModuleBase("Arraylist", "Displays a list of active modules", ModuleCategory::Visual, 0, true) {
        addSettings(
            &mBackground,
            &mVisibility,
            &mRenderMode,
            &mGlow,
            &mGlowStrength,
            &mBoldText,
            &mFontSize,
            &mScale,
            &mEnableAnimSpeed,
            &mDisableAnimSpeed,
            &mElasticAnim,
            &mStaggerAnim
        );

        mNames = {
            {Lowercase, "arraylist"},
            {LowercaseSpaced, "array list"},
            {Normal, "Arraylist"},
            {NormalSpaced, "Array List"}
        };
    }
    void onEnable() override;
    void onDisable() override;

    void onRenderEvent(class RenderEvent& event);
    std::string getSettingDisplay() override {
          return "Sleek";
    }
};
