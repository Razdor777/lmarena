#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>

class NoClip : public ModuleBase<NoClip> {
public:
    NumberSetting mSpeed = NumberSetting("Speed", "Flight speed", 5.6f, 0.1f, 20.f, 0.1f);
    BoolSetting mApplyGlideFlags = BoolSetting("Glide Flags",
        "Apply glide flags to bypass some anticheats", true);
    BoolSetting mTimerBoost = BoolSetting("Timer Boost",
        "Boost the game timer", false);
    NumberSetting mTimerBoostValue = NumberSetting("Timer Value",
        "Timer multiplier", 1.f, 0.01f, 60.f, 0.01f);
    BoolSetting mAntiPush = BoolSetting("Anti Push",
        "Prevent being pushed out of blocks on disable", true);

    NoClip() : ModuleBase("NoClip",
        "Fly through blocks freely",
        ModuleCategory::Movement, 0, false)
    {
        addSettings(
            &mSpeed,
            &mApplyGlideFlags,
            &mAntiPush,
            &mTimerBoost,
            &mTimerBoostValue
        );

        VISIBILITY_CONDITION(mTimerBoostValue, mTimerBoost.mValue);

        mNames = {
            {Lowercase, "noclip"},
            {LowercaseSpaced, "no clip"},
            {Normal, "NoClip"},
            {NormalSpaced, "No Clip"}
        };
    }

    // Saved engine flag states
    bool mHadCollision = true;
    bool mHadGravity = true;
    bool mTimerWasSet = false;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);

    std::string getSettingDisplay() override {
        return std::to_string((int)mSpeed.mValue);
    }
};