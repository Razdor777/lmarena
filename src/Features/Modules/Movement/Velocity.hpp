#pragma once
#include <Features/Modules/Module.hpp>

class Velocity : public ModuleBase<Velocity> {
public:
    enum class Mode {
        Full,
        Percent,
        Redirect,
    };

    enum class Direction {
        Reverse,
        Left,
        Right,
        Custom,
    };

    EnumSettingT<Mode> mMode = EnumSettingT<Mode>("Mode", "The mode of the velocity.",
        Mode::Full, "Full", "Percent", "Redirect");

    NumberSetting mHorizontal = NumberSetting("Horizontal",
        "Horizontal velocity multiplier", 0, -2, 2, 0.01);
    NumberSetting mVertical = NumberSetting("Vertical",
        "Vertical velocity multiplier", 0, -2, 2, 0.01);

    EnumSettingT<Direction> mDirection = EnumSettingT<Direction>("Direction",
        "Knockback redirect direction",
        Direction::Reverse, "Reverse", "Left", "Right", "Custom");
    NumberSetting mCustomAngle = NumberSetting("Custom Angle",
        "Custom rotation angle in degrees", 180.f, 0.f, 360.f, 1.f);
    NumberSetting mRedirectStrength = NumberSetting("Strength",
        "Horizontal knockback multiplier", 1.0f, 0.0f, 3.0f, 0.05f);
    NumberSetting mRedirectVertical = NumberSetting("Vert Mult",
        "Vertical knockback multiplier", 1.0f, 0.0f, 2.0f, 0.05f);

    NumberSetting mChance = NumberSetting("Chance",
        "Probability of KB reduction per hit (%)", 100.f, 0.f, 100.f, 1.f);

    Velocity() : ModuleBase("Velocity",
        "Modify your velocity from attacks, explosions, etc.",
        ModuleCategory::Movement, 0, false)
    {
        addSettings(&mMode, &mHorizontal, &mVertical,
            &mDirection, &mCustomAngle, &mRedirectStrength, &mRedirectVertical,
            &mChance);

        VISIBILITY_CONDITION(mHorizontal, mMode.mValue == Mode::Percent);
        VISIBILITY_CONDITION(mVertical, mMode.mValue == Mode::Percent);
        VISIBILITY_CONDITION(mDirection, mMode.mValue == Mode::Redirect);
        VISIBILITY_CONDITION(mCustomAngle, mMode.mValue == Mode::Redirect && mDirection.mValue == Direction::Custom);
        VISIBILITY_CONDITION(mRedirectStrength, mMode.mValue == Mode::Redirect);
        VISIBILITY_CONDITION(mRedirectVertical, mMode.mValue == Mode::Redirect);

        mNames = {
            {Lowercase, "velocity"},
            {LowercaseSpaced, "velocity"},
            {Normal, "Velocity"},
            {NormalSpaced, "Velocity"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onPacketInEvent(class PacketInEvent& event);  // БЕЗ const!

    std::string getSettingDisplay() override {
        std::string base;
        if (mMode.mValue == Mode::Full) base = "Full";
        else if (mMode.mValue == Mode::Redirect) {
            switch (mDirection.mValue) {
                case Direction::Reverse: base = "Reverse"; break;
                case Direction::Left:    base = "Left"; break;
                case Direction::Right:   base = "Right"; break;
                case Direction::Custom:  base = fmt::format("{}deg", (int)mCustomAngle.mValue); break;
            }
        } else {
            base = fmt::format("{}% {}%",
                (int)(mHorizontal.mValue * 100),
                (int)(mVertical.mValue * 100));
        }
        if ((int)mChance.mValue < 100)
            base += fmt::format(" C{}%", (int)mChance.mValue);
        return base;
    }
};