#pragma once

#include <Features/Modules/Player/SchematicBuilder.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>

class RegionFill : public ModuleBase<RegionFill>
{
public:
    enum class State { Idle, Clearing, Filling };

    NumberSetting mDelay          = NumberSetting("Delay",         "ms between actions",   50.f,  0.f, 500.f, 5.f);
    NumberSetting mStepDistance   = NumberSetting("Step Distance", "TP step size",           8.f,  1.f,  12.f, 0.5f);
    BoolSetting   mClearFirst     = BoolSetting  ("Clear First",   "Break ALL blocks in region before filling", true);
    BoolSetting   mSilentAccept   = BoolSetting  ("Silent Accept", "Prevent rubber banding", true);
    BoolSetting   mShowSelection  = BoolSetting  ("Show Selection","Show selection box",     true);
    BoolSetting   mShowProgress   = BoolSetting  ("Show Progress", "Show progress bar",      true);
    BoolSetting   mSwing          = BoolSetting  ("Swing",         "Swing animation",        false);
    // ── Mixed Blocks ──────────────────────────────────────────────────────────
    BoolSetting   mMixedBlocks    = BoolSetting  ("Mixed Blocks",  "Use multiple block types from hotbar for a varied fill", false);
    BoolSetting   mDiverseOnly    = BoolSetting  ("Diverse Only",  "Only cycle DIFFERENT block types (skip duplicates)", false);

    RegionFill() : ModuleBase("RegionFill", "Fill pos1-pos2 cube with hotbar blocks",
        ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mDelay, &mStepDistance,
            &mClearFirst, &mSilentAccept,
            &mShowSelection, &mShowProgress, &mSwing,
            &mMixedBlocks, &mDiverseOnly
        );
        VISIBILITY_CONDITION(mDiverseOnly, mMixedBlocks.mValue == true);
        mNames = {
            {Lowercase,       "regionfill"},
            {LowercaseSpaced, "region fill"},
            {Normal,          "RegionFill"},
            {NormalSpaced,    "Region Fill"}
        };
    }

    // === State ===
    State mState = State::Idle;

    // Clear phase
    std::vector<glm::ivec3> mClearQueue;
    size_t mClearIndex = 0;

    // Fill phase
    std::vector<glm::ivec3> mFillQueue;
    size_t mFillIndex = 0;

    // Timing
    uint64_t mLastActionTime = 0;
    bool     mIsTPing        = false;

    // TP render
    glm::vec3              mRots           = {0, 0, 0};
    std::vector<glm::vec3> mPacketPositions;
    uint64_t               mLastPathTime   = 0;
    std::mutex             mMutex;

    // Stats
    int      mBlocksCleared = 0;
    int      mBlocksPlaced  = 0;
    int      mTotalToClear  = 0;
    int      mTotalToFill   = 0;
    uint64_t mStartTime     = 0;

    // === Methods ===
    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    // TP
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);

    // Block ops
    void breakBlockAtPos(glm::ivec3 pos, Actor* player);
    bool placeAnyBlockAtPos(glm::ivec3 pos, Actor* player);
    int  findAnyPlaceableSlot();         // single-type: always picks first available
    int  findMixedSlot();                // mixed-type: rotates through all block types
    int  mMixedSlotCursor = 0;          // current position in mixed rotation

    // Helpers
    bool        hasValidSelection();
    glm::ivec3  getSelectionMin();
    glm::ivec3  getSelectionMax();
    void        buildQueues();

    // Render
    void renderSelection();
    void renderProgress();

    std::string getSettingDisplay() override {
        switch (mState) {
            case State::Idle:     return "Idle";
            case State::Clearing: return fmt::format("Clear {}/{}", mClearIndex, mTotalToClear);
            case State::Filling:  return fmt::format("Fill {}/{}", mBlocksPlaced, mTotalToFill);
            default:              return "";
        }
    }
};