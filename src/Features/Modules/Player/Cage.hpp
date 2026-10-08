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
    BoolSetting              mPlaceCeiling  = BoolSetting("Ceiling", "Close the cage above the target (ring + centre)", true);
    BoolSetting              mPlaceFloor    = BoolSetting("Floor", "Close the cage below the target", true);
    BoolSetting              mBreakUnder    = BoolSetting("Break Under", "Break the block under the target before flooring", true);
    NumberSetting            mDelay         = NumberSetting("Delay", "Ms between placements", 65.f, 0.f, 500.f, 5.f);
    NumberSetting            mBlocksPerTick = NumberSetting("Blocks/Tick", "Max blocks per tick", 1.f, 1.f, 20.f, 1.f);
    NumberSetting            mStepDist      = NumberSetting("Step Dist", "TP step size", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting              mSwing         = BoolSetting("Swing", "Arm swing animation", true);
    BoolSetting              mShowESP       = BoolSetting("Show ESP", "Highlight cage blocks", true);

    Cage() : ModuleBase("Cage", "Traps target in a block cage (infinite range)", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mPriority, &mTargetMode, &mIgnoreFriends,
            &mPlaceCeiling, &mPlaceFloor, &mBreakUnder,
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

    // Куда встать, что кликнуть и куда смотреть, чтобы сервер принял блок.
    // Считается под конкретную клетку: от позиции фейкового игрока луч
    // должен попадать ровно в ту грань, которую мы отправляем в пакете.
    struct PlacePlan {
        bool       valid    = false;
        bool       airPlace = false;
        int        face     = 1;
        glm::ivec3 clicked  = {};   // блок, по которому кликаем
        glm::vec3  stand    = {};   // позиция глаз фейкового игрока
        glm::vec2  rots     = {};   // {pitch, yaw} в MC-градусах
    };

    std::vector<glm::ivec3> mPlaceQueue;
    std::vector<glm::ivec3> mCagePositions;
    glm::vec3 mRots = {};

    Actor* findTarget();
    void   rebuildQueue();
    PlacePlan buildPlacePlan(glm::ivec3 cell);
    bool   placeAt(glm::ivec3 cell, Actor* player, int slot);
    bool   placeBlockAt(glm::ivec3 blockPos, Actor* player);
    void   placeWebAt(glm::ivec3 blockPos, Actor* player);
    void   breakBlockAt(glm::ivec3 blockPos, Actor* player);
    bool   isAirAt(glm::ivec3 pos);
    int    findBlockSlot();
    int    findWebSlot();

    std::shared_ptr<class MovePlayerPacket> makeTPPacket(glm::vec3 pos, const glm::vec2& rots);
    void tpBetween(glm::vec3 from, glm::vec3 to, const glm::vec2& rots);

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
};