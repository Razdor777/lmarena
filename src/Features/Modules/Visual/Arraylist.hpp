#pragma once
#include <Features/Modules/Module.hpp>
#include <unordered_map>

//
// Arraylist — redesigned renderer
//
// Every entry is rendered as its own "card": background (dark / gradient / glass / blur),
// an accent element (bar / outline / split / glow) tinted with the client theme and a
// crisp text pass with an optional drop shadow.
//
class Arraylist : public ModuleBase<Arraylist>
{
public:
    // Style of the plate drawn behind an entry
    enum class BackgroundStyle {
        None,
        Dark,
        Gradient,
        Glass,
        Blur,
    };

    // Accent element of an entry
    enum class Display {
        Bar,
        Outline,
        Split,
        Glow,
        None,
    };

    enum class ModuleVisibility {
        All,
        Bound,
    };

    // Where the entry colors come from
    enum class ColorMode {
        Theme,
        Rainbow,
        Category,
        Solid,
    };

    // Screen edge the list is anchored to
    enum class Side {
        Right,
        Left,
    };

    enum class Sorting {
        Width,
        Alphabetical,
        Register,
    };

    EnumSettingT<BackgroundStyle> mBackground = EnumSettingT("Background", "Style of the plate behind every entry", BackgroundStyle::Dark, "None", "Dark", "Gradient", "Glass", "Blur");
    EnumSettingT<Display> mDisplay = EnumSettingT("Display", "Accent element of every entry", Display::Bar, "Bar", "Outline", "Split", "Glow", "None");
    EnumSettingT<ColorMode> mColorMode = EnumSettingT("Color Mode", "Where the entry colors come from", ColorMode::Theme, "Theme", "Rainbow", "Category", "Solid");
    ColorSetting mSolidColor = ColorSetting("Solid Color", "Color used by the 'Solid' color mode", 1.0f, 0.55f, 0.35f, 1.0f);
    EnumSettingT<Side> mSide = EnumSettingT("Side", "Screen edge the list is anchored to", Side::Right, "Right", "Left");
    EnumSettingT<Sorting> mSorting = EnumSettingT("Sorting", "How the entries are ordered", Sorting::Width, "Width", "Alphabetical", "Register");
    EnumSettingT<ModuleVisibility> mVisibility = EnumSettingT("Visibility", "Which modules are shown", ModuleVisibility::All, "All", "Bound");

    BoolSetting mRenderMode = BoolSetting("Render Mode", "Renders the module mode next to the module name", true);
    BoolSetting mAccentSuffix = BoolSetting("Accent Suffix", "Draws the mode in the accent color instead of gray", true);
    BoolSetting mBoldText = BoolSetting("Bold Text", "Makes the text bold", false);
    BoolSetting mTextShadow = BoolSetting("Text Shadow", "Drop shadow behind the text for readability", true);

    NumberSetting mFontSize = NumberSetting("Font Size", "The size of the font", 23.f, 10.f, 40.f, 0.01f);
    NumberSetting mScale = NumberSetting("Scale", "Overall scale of the arraylist", 1.f, 0.5f, 2.f, 0.01f);
    NumberSetting mRounding = NumberSetting("Rounding", "Corner rounding of the plates", 5.f, 0.f, 16.f, 0.1f);
    NumberSetting mPadding = NumberSetting("Padding", "Horizontal text padding inside a plate", 7.f, 0.f, 24.f, 0.1f);
    NumberSetting mSpacing = NumberSetting("Spacing", "Extra gap between entries", 2.f, 0.f, 16.f, 0.1f);
    NumberSetting mOffsetX = NumberSetting("Offset X", "Distance from the screen edge", 10.f, 0.f, 300.f, 0.1f);
    NumberSetting mOffsetY = NumberSetting("Offset Y", "Distance from the top of the screen", 8.f, 0.f, 400.f, 0.1f);

    BoolSetting mGlow = BoolSetting("Glow", "Enables the neon glow around the accent", true);
    NumberSetting mGlowStrength = NumberSetting("Glow Strength", "The strength of the glow", 0.8f, 0.1f, 1.f, 0.05f);
    NumberSetting mBlurStrength = NumberSetting("Blur Strength", "Strength of the 'Blur' background", 6.f, 1.f, 20.f, 0.1f);

    BoolSetting mHoverEffect = BoolSetting("Hover Effect", "Highlights the entry under the cursor", true);
    BoolSetting mClickToggle = BoolSetting("Click To Toggle", "Clicking an entry toggles the module", false);

    NumberSetting mEnableAnimSpeed = NumberSetting("Enable Speed", "Animation speed on enable", 14.f, 5.f, 40.f, 0.5f);
    NumberSetting mDisableAnimSpeed = NumberSetting("Disable Speed", "Animation speed on disable", 8.f, 1.f, 20.f, 0.5f);
    BoolSetting mElasticAnim = BoolSetting("Elastic Animation", "Elastic pop-in effect on enable", true);
    BoolSetting mStaggerAnim = BoolSetting("Stagger Animation", "Staggered slide-in effect", true);

    Arraylist() : ModuleBase("Arraylist", "Displays a list of active modules", ModuleCategory::Visual, 0, true) {
        addSettings(
            &mBackground,
            &mDisplay,
            &mColorMode,
            &mSolidColor,
            &mSide,
            &mSorting,
            &mVisibility,
            &mRenderMode,
            &mAccentSuffix,
            &mBoldText,
            &mTextShadow,
            &mFontSize,
            &mScale,
            &mRounding,
            &mPadding,
            &mSpacing,
            &mOffsetX,
            &mOffsetY,
            &mGlow,
            &mGlowStrength,
            &mBlurStrength,
            &mHoverEffect,
            &mClickToggle,
            &mEnableAnimSpeed,
            &mDisableAnimSpeed,
            &mElasticAnim,
            &mStaggerAnim
        );

        VISIBILITY_CONDITION(mSolidColor, mColorMode.mValue == ColorMode::Solid);
        VISIBILITY_CONDITION(mBlurStrength, mBackground.mValue == BackgroundStyle::Blur);

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
        int index = mBackground.as<int>();
        if (mBackground.mValues.empty()) return "Sleek";
        if (index < 0 || index >= static_cast<int>(mBackground.mValues.size())) index = 0;
        return mBackground.mValues[index];
    }
};
