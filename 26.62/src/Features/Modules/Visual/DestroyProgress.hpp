#pragma once
//
// DestroyProgress — the custom animation you see while mining a block.
//
// The old version drew a single shrinking box. This one is a small effect
// stack, all driven by the real break progress:
//
//   * the whole block gets a clean wireframe outline the moment you start mining,
//   * an inner cube shrinks with the progress and drives the colour ramp,
//   * optional glow + breathing pulse while you are mining,
//   * the outline fades out smoothly when you stop instead of snapping away,
//   * a quick expanding "pop" the moment the block actually breaks.
//
// Colours: Progress (red -> amber -> green), Theme (follows the client theme,
// so it inherits its animations) or Custom.
//
#include <Features/Modules/Module.hpp>
#include <vector>
#include <glm/glm.hpp>

class DestroyProgress : public ModuleBase<DestroyProgress>
{
public:
    enum class ColorMode {
        Progress,
        Theme,
        Custom,
    };

    EnumSettingT<ColorMode> mColorMode = EnumSettingT<ColorMode>(
        "Color", "How the box is coloured",
        ColorMode::Progress, "Progress", "Theme", "Custom");

    ColorSetting mCustomColor = ColorSetting(
        "Custom Color", "Used when Color = Custom", 0.35f, 0.85f, 1.0f, 1.0f);

    NumberSetting mOpacity   = NumberSetting("Opacity",    "Overall opacity of the box", 0.65f, 0.05f, 1.f, 0.05f);
    NumberSetting mThickness = NumberSetting("Thickness",  "Outline thickness", 1.6f, 0.5f, 5.f, 0.1f);
    NumberSetting mFadeSpeed = NumberSetting("Fade Speed", "How fast the box fades out when you stop mining", 12.f, 2.f, 40.f, 0.5f);

    BoolSetting mOutline = BoolSetting("Outline", "Outline the whole block", true);
    BoolSetting mFilled  = BoolSetting("Filled",  "Fill the shrinking cube", true);
    BoolSetting mGlow    = BoolSetting("Glow",    "Soft glow around the outline", true);
    BoolSetting mPulse   = BoolSetting("Pulse",   "Breathing pulse while mining", true);
    BoolSetting mPop     = BoolSetting("Pop",     "Expanding flash when the block breaks", true);

    DestroyProgress() : ModuleBase("DestroyProgress",
        "Custom block destroy animation", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mColorMode, &mCustomColor,
            &mOpacity, &mThickness, &mFadeSpeed,
            &mOutline, &mFilled, &mGlow, &mPulse, &mPop
        );

        VISIBILITY_CONDITION(mCustomColor, mColorMode.mValue == ColorMode::Custom);

        mNames = {
            {Lowercase,       "destroyprogress"},
            {LowercaseSpaced, "destroy progress"},
            {Normal,          "DestroyProgress"},
            {NormalSpaced,    "Destroy Progress"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

private:
    // ---- animation state (members, not statics: they reset with the module) ----
    glm::ivec3 mLastPos{0};      // block currently being mined
    float      mAnim       = 0.f;  // smoothed break progress 0..1
    float      mFade       = 0.f;  // 1 while mining -> 0 after we stop
    float      mPopAnim    = 0.f;  // 0..1 expanding pop after a break
    bool       mHasTarget  = false;

    ImColor progressColor(float progress) const;
    ImColor sourceColor(float progress) const;
};
