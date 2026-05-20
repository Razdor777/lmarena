#pragma once
#include <Features/Modules/Module.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <mutex>

//
// Cage — трапит цель в клетку из блоков
// Умно предсказывает движение цели и перекрывает путь впереди
//

class Cage : public ModuleBase<Cage>
{
public:
    // ── Target ────────────────────────────────────────────────────────────
    NumberSetting mRange       = NumberSetting("Range",        "Max distance to target",           16.f, 1.f, 64.f,  0.5f);

    // ── Cage shape ────────────────────────────────────────────────────────
    NumberSetting mCageRadius  = NumberSetting("Cage Radius",  "Horizontal radius of cage",        1.f,  1.f, 4.f,   1.f);
    NumberSetting mCageHeight  = NumberSetting("Cage Height",  "Height of cage (blocks)",          2.f,  1.f, 5.f,   1.f);
    BoolSetting   mPlaceCeiling= BoolSetting  ("Ceiling",      "Place blocks above target",        true);
    BoolSetting   mPlaceFloor  = BoolSetting  ("Floor",        "Place blocks below target",        false);
    BoolSetting   mHollowWalls = BoolSetting  ("Hollow Walls", "Only outline, not solid walls",    true);

    // ── Prediction & smart logic ──────────────────────────────────────────
    BoolSetting   mPredictMotion = BoolSetting("Predict Motion","Place blocks ahead of movement",  true);
    NumberSetting mPredictTicks  = NumberSetting("Predict Ticks","How many ticks ahead to predict",8.f,  1.f, 20.f,  1.f);
    BoolSetting   mCutoffMode    = BoolSetting ("Cut-off Mode", "Block path ahead, not trail",     true);

    // ── Placement ─────────────────────────────────────────────────────────
    NumberSetting mDelay       = NumberSetting("Delay",        "Ms between block placements",      65.f, 0.f, 500.f, 5.f);
    NumberSetting mBlocksPerTick= NumberSetting("Blocks/Tick", "Max blocks placed per tick",       1.f,  1.f, 20.f,  1.f);
    NumberSetting mStepDist    = NumberSetting("Step Dist",    "TP step size (RegionFill style)",  8.f,  1.f, 12.f,  0.5f);
    BoolSetting   mVisible     = BoolSetting  ("Visible",      "Show placement packets",           true);
    BoolSetting   mSwing       = BoolSetting  ("Swing",        "Animate arm swing on place",       true);
    BoolSetting   mAutoTarget  = BoolSetting  ("Auto Target",  "Automatically pick nearest enemy", true);
    BoolSetting   mShowESP     = BoolSetting  ("Show ESP",     "Highlight cage blocks in world",   true);

    Cage() : ModuleBase("Cage", "Traps the target in a block cage", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mRange,
            &mCageRadius, &mCageHeight, &mPlaceCeiling, &mPlaceFloor, &mHollowWalls,
            &mPredictMotion, &mPredictTicks, &mCutoffMode,
            &mDelay, &mBlocksPerTick, &mStepDist, &mVisible, &mSwing, &mAutoTarget, &mShowESP
        );
        mNames = {
            {Lowercase,       "cage"},
            {LowercaseSpaced, "cage"},
            {Normal,          "Cage"},
            {NormalSpaced,    "Cage"}
        };
    }

    // ── State ─────────────────────────────────────────────────────────────
    Actor*              mTarget      = nullptr;
    glm::vec3           mTargetPrev  = {};   // position last tick (for velocity)
    glm::vec3           mTargetVel   = {};   // estimated velocity
    uint64_t            mLastPlace   = 0;

    // Blocks the cage wants to fill — sorted by priority
    std::vector<glm::ivec3>  mPlaceQueue;

    // For ESP
    std::vector<glm::ivec3>  mCagePositions;

    // Rotation snapshot for TP packets
    glm::vec3 mRots = {};

    // ── Internal helpers ──────────────────────────────────────────────────
    Actor* findTarget();
    void   rebuildQueue();
    void   placeBlock(glm::ivec3 pos, Actor* player);
    bool   isAirAt(glm::ivec3 pos);

    // RegionFill-style TP + block place
    std::shared_ptr<class MovePlayerPacket> makeTPPacket(glm::vec3 pos);
    void   tpBetween(glm::vec3 from, glm::vec3 to);
    int    findBlockSlot();

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent (class RenderEvent&    event);
};
