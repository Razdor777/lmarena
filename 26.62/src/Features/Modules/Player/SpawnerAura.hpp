#pragma once

#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class SpawnerAura : public ModuleBase<SpawnerAura>
{
public:
    enum class ClickMode {
        RightClick,
        LeftClick,
        Both
    };

    enum class ScanState {
        Idle,
        Scanning,
        Found
    };

    // --- Settings ---
    NumberSetting mRange = NumberSetting("Range", "Search radius for spawner blocks", 100.f, 10.f, 500.f, 10.f);
    NumberSetting mAPS = NumberSetting("APS", "Interactions per second", 10.f, 1.f, 20.f, 0.5f);
    NumberSetting mStepDistance = NumberSetting("Step Distance", "Blocks per TP packet", 8.f, 0.1f, 12.f, 0.01f);
    EnumSettingT<ClickMode> mClickMode = EnumSettingT("Click Mode", "How to click the spawner",
        ClickMode::Both, "Right Click", "Left Click", "Both");
    NumberSetting mBlocksPerTick = NumberSetting("Scan Speed (K)", "Thousands of blocks per tick", 50, 10, 200, 10);
    BoolSetting mSilentAccept = BoolSetting("Silent Accept", "Prevent rubber banding", true);
    BoolSetting mRepeatMode = BoolSetting("Repeat", "Keep hitting same spawner without rescan", true);
    BoolSetting mMultiSpawner = BoolSetting("Multi", "Hit all discovered spawners round-robin", false);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw TP path", true);
    BoolSetting mDrawTarget = BoolSetting("Draw Target", "Highlight spawner block", true);

    SpawnerAura() : ModuleBase("SpawnerAura", "Auto-interact with nearest spawner via chain TP",
        ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mRange, &mAPS, &mStepDistance, &mClickMode,
            &mBlocksPerTick, &mSilentAccept,
            &mRepeatMode, &mMultiSpawner,
            &mDrawPath, &mDrawTarget
        );

        mNames = {
            {Lowercase, "spawneraura"},
            {LowercaseSpaced, "spawner aura"},
            {Normal, "SpawnerAura"},
            {NormalSpaced, "Spawner Aura"}
        };
    }

    // --- State ---
    glm::vec3 mRots = {0, 0, 0};
    uint64_t mLastInteract = 0;

    // Scan state
    ScanState mScanState = ScanState::Idle;
    glm::ivec3 mScanCenter = {0, 0, 0};
    int mScanSlice = 0;        // current X-slice index (center-outward)
    int mScanZ = 0;            // current Z offset within slice
    int mScanY = 0;            // current Y offset within column
    int mTotalSlices = 0;
    uint64_t mLastScanEnd = 0;

    // Target
    glm::ivec3 mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
    bool mHasTarget = false;

    // Render
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;

    // Multi-spawner
    std::vector<glm::ivec3> mAllSpawners;
    int mCurrentSpawnerIdx = 0;

    // --- Methods ---
    void onEnable() override;
    void onDisable() override;
    static int getXOffset(int slice);
    void scanTick(Actor* player);
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    std::string getSettingDisplay() override {
        if (mScanState == ScanState::Scanning) return "Scanning...";
        if (mMultiSpawner.mValue && !mAllSpawners.empty())
            return fmt::format("{}x Active", mAllSpawners.size());
        if (mHasTarget) return "Active";
        return "Idle";
    }
};