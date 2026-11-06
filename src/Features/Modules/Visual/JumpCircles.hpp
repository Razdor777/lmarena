#pragma once

//
// JumpCircles — перенос из combatant-client (visuals/JumpCircles.java).
// Круг на земле в точке, где игрок оторвался от земли.
//   Default: расширяющийся диск с вращающейся подсветкой.
//   Bubble:  только обводка, без заливки.
//   Pulse:   пружинящий круг с пульсацией яркости.
//
// Круги процедурные (combatant использовал PNG-текстуры, их в Juzdex нет),
// поэтому depth test и часть цветовых режимов combatant не переносились.
//

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

class JumpCircles : public ModuleBase<JumpCircles>
{
public:
    enum class Mode {
        Default,
        Bubble,
        Pulse
    };

    enum class ColorMode {
        Static,
        Rainbow,
        DoubleColor,
        Theme
    };

    EnumSettingT<Mode> mMode = EnumSettingT<Mode>(
        "Mode", "Circle look", Mode::Default, "Default", "Bubble", "Pulse");
    BoolSetting mEaseOut = BoolSetting("Ease Out", "Ease the circle growth", true);
    NumberSetting mRotateSpeed = NumberSetting("Rotate Speed", "How fast the highlight rotates", 2.f, 0.5f, 5.f, 0.1f);
    NumberSetting mCircleScale = NumberSetting("Circle Scale", "Overall circle size", 1.f, 0.5f, 5.f, 0.1f);
    NumberSetting mPulseMaxSize = NumberSetting("Pulse Max Size", "Peak size in Pulse mode", 2.5f, 1.f, 3.f, 0.05f);
    NumberSetting mPulseLifetime = NumberSetting("Pulse Lifetime", "Pulse duration (ms)", 1000, 500, 5000, 50);
    BoolSetting mOnlySelf = BoolSetting("Only Self", "Only draw circles for yourself", false);
    EnumSettingT<ColorMode> mColorMode = EnumSettingT<ColorMode>(
        "Color Mode", "How the color is chosen", ColorMode::Static, "Static", "Rainbow", "DoubleColor", "Theme");
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "Speed of animated colors", 18, 2, 54, 1);
    ColorSetting mColor = ColorSetting("Color", "Primary color", 0xFF55FFFF);
    ColorSetting mColor2 = ColorSetting("Color 2", "Secondary color (DoubleColor)", 0xFFFF5555);

    JumpCircles() : ModuleBase("JumpCircles", "Circles on the ground where you jump from", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mMode,
            &mEaseOut,
            &mRotateSpeed,
            &mCircleScale,
            &mPulseMaxSize,
            &mPulseLifetime,
            &mOnlySelf,
            &mColorMode,
            &mColorSpeed,
            &mColor,
            &mColor2
        );

        VISIBILITY_CONDITION(mEaseOut, mMode.mValue != Mode::Pulse);
        VISIBILITY_CONDITION(mRotateSpeed, mMode.mValue == Mode::Default);
        VISIBILITY_CONDITION(mPulseMaxSize, mMode.mValue == Mode::Pulse);
        VISIBILITY_CONDITION(mPulseLifetime, mMode.mValue == Mode::Pulse);
        VISIBILITY_CONDITION(mColorSpeed, mColorMode.mValue == ColorMode::Rainbow || mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mColor2, mColorMode.mValue == ColorMode::DoubleColor);

        mNames = {
            {Lowercase, "jumpcircles"},
            {LowercaseSpaced, "jump circles"},
            {Normal, "JumpCircles"},
            {NormalSpaced, "Jump Circles"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override
    {
        return mMode.mValues[mMode.as<int>()];
    }

private:
    struct Circle
    {
        glm::vec3 pos;   // точка на земле (под ногами)
        uint64_t start;  // момент отрыва от земли, мс
    };

    std::vector<Circle> mCircles;
    std::unordered_map<int64_t, bool> mGroundState; // runtime id -> был ли на земле

    uint64_t lifetimeMs() const;
    ImVec4 colorFor(float index, uint64_t now) const;
    void drawCircle(ImDrawList* dl, const Circle& c, uint64_t now);
};
