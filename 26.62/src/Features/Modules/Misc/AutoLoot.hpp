#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/MouseEvent.hpp>

class AutoLoot : public ModuleBase<AutoLoot>
{
public:
    enum class LootState {
        WaitingFirstTP,
        AtItem,
        Returned
    };

    NumberSetting mRange = NumberSetting("Range", "Max distance to loot items", 50.f, 5.f, 200.f, 1.f);
    BoolSetting mInfiniteRange = BoolSetting("Infinite Range", "Loot at any distance", false);
    NumberSetting mStepDistance = NumberSetting("Step Distance", "Blocks per TP step", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting mTPBack = BoolSetting("TP Back", "LMB=return+disable, RMB=next item", true);
    BoolSetting mOnlyUseful = BoolSetting("Only Useful", "Only pick up useful items", false);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw path to items", true);

    AutoLoot() : ModuleBase("AutoLoot", "Teleport to dropped items and pick them up",
        ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mRange, &mInfiniteRange, &mStepDistance,
            &mTPBack, &mOnlyUseful, &mDrawPath
        );

        VISIBILITY_CONDITION(mRange, !mInfiniteRange.mValue);

        mNames = {
            {Lowercase, "autoloot"},
            {LowercaseSpaced, "auto loot"},
            {Normal, "AutoLoot"},
            {NormalSpaced, "Auto Loot"}
        };
    }

    // State
    LootState mState = LootState::Returned;
    glm::vec3 mSavedPosition = glm::vec3(0.f);
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;
    bool mPendingDisable = false;

    void onEnable() override;
    void onDisable() override;

    bool findAndTeleportToNearest(Actor* player, glm::vec3 fromPos);
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(Actor* player, glm::vec3 pos);
    void straightLineTP(Actor* player, glm::vec3 from, glm::vec3 to, bool saveForRender);
    void teleportBack(Actor* player);

    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onMouseEvent(class MouseEvent& event);

    std::string getSettingDisplay() override {
        switch (mState) {
            case LootState::AtItem: return "At Item";
            case LootState::WaitingFirstTP: return "Scanning";
            case LootState::Returned: return "Done";
        }
        return "";
    }
};