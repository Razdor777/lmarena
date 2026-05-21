#pragma once
#include <Features/Modules/Module.hpp>
#include <glm/glm.hpp>
#include <vector>

class Cage : public ModuleBase<Cage>
{
public:
    enum class Priority { Closest, Armor };
    enum class TargetMode { Players, Mobs, All };

    EnumSettingT<Priority>   mPriority      = EnumSettingT<Priority>("Priority", "Target priority", Priority::Closest, "Closest", "Armor");
    EnumSettingT<TargetMode> mTargetMode    = EnumSettingT<TargetMode>("Target", "Who to cage", TargetMode::Players, "Players", "Mobs", "All");
    BoolSetting              mIgnoreFriends = BoolSetting("Ignore Friends", "Don't cage friends", true);
    BoolSetting              mPlaceCeiling  = BoolSetting("Ceiling", "Place blocks above target", true);
    BoolSetting              mPlaceFloor    = BoolSetting("Floor", "Place blocks below target", false);
    NumberSetting            mDelay         = NumberSetting("Delay", "Ms between placements", 65.f, 0.f, 500.f, 5.f);
    NumberSetting            mBlocksPerTick = NumberSetting("Blocks/Tick", "Max blocks per tick", 1.f, 1.f, 20.f, 1.f);
    NumberSetting            mStepDist      = NumberSetting("Step Dist", "TP step size", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting              mSwing         = BoolSetting("Swing", "Arm swing animation", true);
    BoolSetting              mShowESP       = BoolSetting("Show ESP", "Highlight cage blocks", true);

    Cage() : ModuleBase("Cage", "Traps target in a block cage (infinite range)", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mPriority, &mTargetMode, &mIgnoreFriends,
            &mPlaceCeiling, &mPlaceFloor,
            &mDelay, &mBlocksPerTick, &mStepDist, &mSwing, &mShowESP
        );
        mNames = {
            {Lowercase, "cage"}, {LowercaseSpaced, "cage"},
            {Normal, "Cage"}, {NormalSpaced, "Cage"}
        };
    }

    Actor*    mTarget     = nullptr;
    glm::vec3 mTargetPrev = {};
    glm::vec3 mTargetVel  = {};
    uint64_t  mLastPlace  = 0;

    std::vector<glm::ivec3> mPlaceQueue;
    std::vector<glm::ivec3> mCagePositions;
    glm::vec3 mRots = {};

    Actor* findTarget();
    void   rebuildQueue();
    void   placeBlockAt(glm::ivec3 blockPos, Actor* player);
    void   placeWebAt(glm::ivec3 blockPos, Actor* player);
    bool   isAirAt(glm::ivec3 pos);
    int    findBlockSlot();
    int    findWebSlot();

    std::shared_ptr<class MovePlayerPacket> makeTPPacket(glm::vec3 pos);
    void tpBetween(glm::vec3 from, glm::vec3 to);

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
};