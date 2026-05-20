#pragma once
#include <Features/Modules/Module.hpp>
#include <unordered_map>

class Arraylist : public ModuleBase<Arraylist>
{
public:
    enum class BackgroundStyle {
        None,       // NO background — text only
        Opacity,    // Semi-transparent solid bg
        Shadow,     // Blur + shadow only
        Both        // Everything combined
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

    EnumSettingT<BackgroundStyle> mBackground = EnumSettingT("Background", "Background style", BackgroundStyle::None, "None", "Opacity", "Shadow", "Both");
    NumberSetting mBackgroundOpacity = NumberSetting("Opacity", "The opacity of the background", 0.55f, 0.0f, 1.f, 0.01f);
    NumberSetting mBackgroundValue = NumberSetting("Background Value", "The value of the background", 0.08f, 0.0f, 1.f, 0.01f);
    NumberSetting mBlurStrength = NumberSetting("Blur Strength", "The strength of the blur.", 0.f, 0.f, 10.f, 0.1f);
    EnumSettingT<Display> mDisplay = EnumSettingT("Display", "Outline style", Display::Split, "Outline", "Bar", "Split", "None");
    EnumSettingT<ModuleVisibility> mVisibility = EnumSettingT("Visibility", "Module visibility", ModuleVisibility::All, "All", "Bound");
    BoolSetting mRenderMode = BoolSetting("Render Mode", "Renders the module mode next to the module name", true);
    BoolSetting mGlow = BoolSetting("Glow", "Enables glow", true);
    NumberSetting mGlowStrength = NumberSetting("Glow Strength", "The strength of the glow", 1.f, 0.5f, 1.f, 0.1f);
    BoolSetting mBoldText = BoolSetting("Bold Text", "Makes the text bold", true);
    NumberSetting mFontSize = NumberSetting("Font Size", "The size of the font", 25.f, 10.f, 40.f, 0.01f);
    BoolSetting mHoverHighlight = BoolSetting("Hover Highlight", "Highlight module on hover", true);
    NumberSetting mHoverAnimSpeed = NumberSetting("Hover Speed", "Hover animation speed", 12.f, 1.f, 30.f, 0.5f);
    NumberSetting mScale = NumberSetting("Scale", "Overall scale of the arraylist", 1.f, 0.5f, 2.f, 0.01f);
    // NEW: Animation controls
    NumberSetting mEnableAnimSpeed = NumberSetting("Enable Speed", "Animation speed on enable", 18.f, 5.f, 40.f, 0.5f);
    NumberSetting mDisableAnimSpeed = NumberSetting("Disable Speed", "Animation speed on disable", 6.f, 1.f, 20.f, 0.5f);
    BoolSetting mElasticAnim = BoolSetting("Elastic Animation", "Elastic pop-in effect on enable", true);
    BoolSetting mStaggerAnim = BoolSetting("Stagger Animation", "Staggered slide-in effect", true);

    Arraylist() : ModuleBase("Arraylist", "Displays a list of modules", ModuleCategory::Visual, 0, true) {
        addSettings(
            &mBackground,
            &mBackgroundOpacity,
            &mBackgroundValue,
            &mBlurStrength,
            &mVisibility,
            &mRenderMode,
            &mGlow,
            &mGlowStrength,
            &mBoldText,
            &mFontSize,
            &mHoverHighlight,
            &mHoverAnimSpeed,
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
          return mDisplay.mValues[mDisplay.as<int>()];
    }
};