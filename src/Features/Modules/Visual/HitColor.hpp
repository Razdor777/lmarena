#pragma once
// HitColor - Custom hurt color via real game hook
// Применяется ТОЛЬКО когда игрок получает урон

#include <Features/Modules/Module.hpp>
#include <Features/Events/HurtColorEvent.hpp>

class HitColor : public ModuleBase<HitColor>
{
public:
    // === HURT COLOR ===
    ColorSetting mHurtColorValue = ColorSetting("Color", "Цвет оверлея при получении урона", 1.0f, 0.0f, 0.0f, 0.6f);
    BoolSetting  mRainbowHurt   = BoolSetting("Rainbow", "Радужный цвет при получении урона", false);

    HitColor() : ModuleBase("HitColor", "Custom hurt flash color (only triggers on damage)", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mHurtColorValue,
            &mRainbowHurt
        );

        VISIBILITY_CONDITION(mHurtColorValue, !mRainbowHurt.mValue);

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
