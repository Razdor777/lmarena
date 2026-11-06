#pragma once

#include <Features/Modules/Module.hpp>
#include <queue>

class ChainTP : public ModuleBase<ChainTP> {
public:
    NumberSetting mTargetX = NumberSetting("X", "Target X", 0, -30000, 30000, 1);
    NumberSetting mTargetY = NumberSetting("Y", "Target Y", 64, -64, 320, 1);
    NumberSetting mTargetZ = NumberSetting("Z", "Target Z", 0, -30000, 30000, 1);
    NumberSetting mStepSize = NumberSetting("Step Size", "Blocks per tick (max ~8)", 7.5f, 1.0f, 9.0f, 0.5f);
    NumberSetting mStepsPerTick = NumberSetting("Steps/Tick", "How many steps per tick", 1, 1, 5, 1);
    BoolSetting mGrabPos = BoolSetting("Grab Target", "Set target from crosshair block", false);
    BoolSetting mGrabCurrentPos = BoolSetting("Set Current Pos", "Set target to current position", false);
    BoolSetting mAutoDisable = BoolSetting("Auto Disable", "Disable when arrived", true);
    BoolSetting mSpoofAuthInput = BoolSetting("Spoof Auth", "Spoof position in PlayerAuthInput", true);

    ChainTP() : ModuleBase("ChainTP",
        "Teleport anywhere by chaining small movements",
        ModuleCategory::Movement, 0, false)
    {
        addSettings(&mTargetX, &mTargetY, &mTargetZ, &mStepSize,
                    &mStepsPerTick, &mGrabPos, &mGrabCurrentPos,
                    &mAutoDisable, &mSpoofAuthInput);
        mNames = {
            {Lowercase, "chaintp"},
            {LowercaseSpaced, "chain tp"},
            {Normal, "ChainTP"},
            {NormalSpaced, "Chain TP"}
        };
    }

    bool mIsTeleporting = false;
    glm::vec3 mCurrentFakePos = {0, 0, 0};
    glm::vec3 mTargetPos = {0, 0, 0};
    float mTotalDistance = 0;
    float mTraveledDistance = 0;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);

    std::string getSettingDisplay() override {
        if (mIsTeleporting) {
            float remaining = glm::distance(mCurrentFakePos, mTargetPos);
            return std::to_string((int)remaining) + "m left";
        }
        return "";
    }
};