#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/HurtColorEvent.hpp>

class HitColor : public ModuleBase<HitColor> {
public:
    BoolSetting  mAlwaysEnabled  = BoolSetting("AlwaysEnabled", "Включить постоянный цвет", true);
    ColorSetting mAlwaysColor    = ColorSetting("AlwaysColor", "Цвет в обычном состоянии", 0.0f, 1.0f, 0.0f, 0.35f);
    NumberSetting mAlwaysAlpha   = NumberSetting("AlwaysAlpha", "Прозрачность постоянного цвета", 0.35f, 0.05f, 1.0f, 0.05f);
    BoolSetting  mRainbowAlways  = BoolSetting("RainbowAlways", "Радужный цвет в обычном состоянии", false);

    BoolSetting  mHurtEnabled    = BoolSetting("HurtEnabled", "Включить цвет при уроне", true);
    ColorSetting mHurtColor      = ColorSetting("HurtColor", "Цвет при получении урона", 0.0f, 0.3f, 1.0f, 0.7f);
    BoolSetting  mRainbowHurt    = BoolSetting("RainbowHurt", "Радужный цвет при уроне", false);

    HitColor() : ModuleBase("HitColor", "Custom color: always + on hurt", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mAlwaysEnabled, &mAlwaysColor, &mAlwaysAlpha, &mRainbowAlways,
            &mHurtEnabled, &mHurtColor, &mRainbowHurt
        );

        VISIBILITY_CONDITION(mAlwaysColor, !mRainbowAlways.mValue && mAlwaysEnabled.mValue);
        VISIBILITY_CONDITION(mAlwaysAlpha, !mRainbowAlways.mValue && mAlwaysEnabled.mValue);
        VISIBILITY_CONDITION(mHurtColor, !mRainbowHurt.mValue && mHurtEnabled.mValue);

        mNames = {
            {Lowercase,       "hitcolor"},
            {LowercaseSpaced, "hit color"},
            {Normal,          "HitColor"},
            {NormalSpaced,    "Hit Color"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void onHurtColorEvent(HurtColorEvent& event);
};