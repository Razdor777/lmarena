#pragma once
//
// Created by vastrakai on 7/7/2024.
//
// Ported from Combatant BlockESP (visuals). Scanning uses the Juzdex subchunk
// spiral (equivalent of Combatant's WorldSectionBlockScanner). Visibility ("LOS")
// is approximated with exposed faces, because Juzdex has no render-section data.
//

#include <Features/Modules/Module.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

class BlockESP : public ModuleBase<BlockESP>
{
public:
    enum class BlockRenderMode {
        Filled,
        Outline,
        Both
    };

    enum class BlockScanMode {
        LOS,
        Unrestricted
    };

    EnumSettingT<BlockRenderMode> mRenderMode = EnumSettingT(
        "Render Mode", "The mode to render block", BlockRenderMode::Outline, "Filled", "Outline", "Both");
    EnumSettingT<BlockScanMode> mScanMode = EnumSettingT(
        "Mode", "LOS: only blocks with an exposed face (approximate visibility). Unrestricted: every found block",
        BlockScanMode::LOS, "LOS", "Unrestricted");
    NumberSetting mLineWidth = NumberSetting("Line Width", "Outline thickness", 1.5f, 0.5f, 4.0f, 0.1f);
    ColorSetting mVisibleColor = ColorSetting("Visible Color", "Color of visible (exposed) blocks", 1.f, 0.f, 0.f, 1.f);
    ColorSetting mChunkColor = ColorSetting("Hidden Color", "Color of blocks with no exposed face", 1.f, 1.f, 0.f, 1.f);

    BoolSetting mLimitDistance = BoolSetting("Limit Distance", "Only search and render blocks within Max Distance", true);
    NumberSetting mMaxDistance = NumberSetting("Max Distance", "Maximum distance to blocks (when Limit Distance is on)", 64.f, 1.f, 256.f, 1.f);
    NumberSetting mMaxTargets = NumberSetting("Max Targets", "Maximum number of tracked blocks", 128.f, 1.f, 4096.f, 1.f);
    NumberSetting mUpdateFrequency = NumberSetting("Update Frequency", "The frequency of the block update (in ticks)", 1.f, 1.f, 40.f, 0.01f);
    NumberSetting mChunkUpdatesPerTick = NumberSetting("Chunk Updates Per Tick", "The number of subchunks to update per tick", 5.f, 1.f, 24.f, 1.f);
    BoolSetting mRenderCurrentChunk = BoolSetting("Render Current Chunk", "Renders the current chunk", false);

    BoolSetting mEmerald = BoolSetting("Emerald", "Draws around emerald ore", true);
    BoolSetting mDiamond = BoolSetting("Diamond", "Draws around diamond ore", true);
    BoolSetting mGold = BoolSetting("Gold", "Draws around gold ore", true);
    BoolSetting mIron = BoolSetting("Iron", "Draws around iron ore", true);
    BoolSetting mCoal = BoolSetting("Coal", "Draws around coal ore", true);
    BoolSetting mRedstone = BoolSetting("Redstone", "Draws around redstone ore", true);
    BoolSetting mLapis = BoolSetting("Lapis", "Draws around lapis ore", true);
    BoolSetting mPortal = BoolSetting("Portal", "Draws around portal blocks", true);
    BoolSetting mChests = BoolSetting("Chests", "Draws around chests", false);

    BoolSetting mDeobfuscation = BoolSetting("Deobfuscation", "Briefly mines nearby blocks so obfuscated ores are revealed (sends block-dig packets)", false);
    NumberSetting mDeobfuscationRadius = NumberSetting("Deobfuscation Radius", "Horizontal radius for deobfuscation", 5.f, 1.f, 64.f, 1.f);
    NumberSetting mDeobfuscationUp = NumberSetting("Deobfuscation Up", "Vertical range above the player", 5.f, 1.f, 32.f, 1.f);
    NumberSetting mDeobfuscationDown = NumberSetting("Deobfuscation Down", "Vertical range below the player", 5.f, 1.f, 32.f, 1.f);
    BoolSetting mDeobfuscationFast = BoolSetting("Deobfuscation Fast", "Skip every other block while deobfuscating", false);

    BoolSetting mUseTracers = BoolSetting("Use Tracers", "Draw a line from the bottom of the screen to each block", false);
    BoolSetting mClusterTracers = BoolSetting("Cluster Tracers", "One tracer per connected group of blocks", false);

    BlockESP() : ModuleBase("BlockESP", "Draws a box around selected blocks", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mRenderMode,
            &mScanMode,
            &mLineWidth,
            &mVisibleColor,
            &mChunkColor,
            &mLimitDistance,
            &mMaxDistance,
            &mMaxTargets,
            &mUpdateFrequency,
            &mChunkUpdatesPerTick,
            &mRenderCurrentChunk,
            &mDiamond,
            &mEmerald,
            &mGold,
            &mIron,
            &mCoal,
            &mRedstone,
            &mLapis,
            &mPortal,
            &mChests,
            &mDeobfuscation,
            &mDeobfuscationRadius,
            &mDeobfuscationUp,
            &mDeobfuscationDown,
            &mDeobfuscationFast,
            &mUseTracers,
            &mClusterTracers
        );

        VISIBILITY_CONDITION(mMaxDistance, mLimitDistance.mValue);
        VISIBILITY_CONDITION(mDeobfuscationRadius, mDeobfuscation.mValue);
        VISIBILITY_CONDITION(mDeobfuscationUp, mDeobfuscation.mValue);
        VISIBILITY_CONDITION(mDeobfuscationDown, mDeobfuscation.mValue);
        VISIBILITY_CONDITION(mDeobfuscationFast, mDeobfuscation.mValue);
        VISIBILITY_CONDITION(mClusterTracers, mUseTracers.mValue);

        mNames = {
            {Lowercase, "blockesp"},
            {LowercaseSpaced, "block esp"},
            {Normal, "BlockESP"},
            {NormalSpaced, "Block ESP"}
        };
    }

    // === Scan state ===
    ChunkPos mSearchCenter;
    ChunkPos mCurrentChunkPos;
    int mSubChunkIndex = 0;
    int mDirectionIndex = 0;
    int mSteps = 1;
    int mStepsCount = 0;
    int64_t mSearchStart = 0;

    // === Block data ===
    struct FoundBlock
    {
        const Block* block;
        int blockId;
        AABB aabb;
        bool visible;
    };

    std::unordered_map<BlockPos, FoundBlock> mFoundBlocks = {};

    // Reconcile: re-checks tracked blocks a few at a time (visibility, distance, still a target)
    std::vector<BlockPos> mReconcileKeys = {};
    size_t mReconcileIndex = 0;

    // Deobfuscation state
    std::vector<BlockPos> mDeobQueue = {};
    glm::ivec3 mDeobCenter = {0, 0, 0};
    int mDeobDelayTicks = 0;
    bool mDeobBuilt = false;

    // === Scan methods ===
    void moveToNext();
    void tryProcessSub(bool& processed, ChunkPos currentChunkPos, int subChunkIndex);
    bool processSub(ChunkPos processChunk, int subChunk);
    void reset();

    // === Helpers ===
    int getScanRadius();
    bool withinDistance(const glm::vec3& pos);
    bool acceptsMode(bool visible);
    void reconcileTargets(int budget);
    void tickDeobfuscation();
    void rebuildDeobQueue();
    void renderTracers();
    std::vector<int> getEnabledBlocks();
    std::vector<std::vector<BlockPos>> buildClusters(const std::vector<BlockPos>& positions);

    // === Events ===
    void onEnable() override;
    void onDisable() override;
    void onBlockChangedEvent(class BlockChangedEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);
};
