#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>

class Block;

class OreMiner : public ModuleBase<OreMiner>
{
public:
    NumberSetting mDestroySpeed = NumberSetting("Destroy Speed", "Break threshold", 1.f, 0.01f, 1.f, 0.01f);
    NumberSetting mStepDistance = NumberSetting("Step Distance", "TP step", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting mVeinMiner = BoolSetting("Vein Miner", "Break all connected blocks of same type", false);
    NumberSetting mMineDelay = NumberSetting("Mine Delay", "Delay between blocks (ms)", 50.f, 0.f, 500.f, 10.f);
    NumberSetting mBlocksPerTick = NumberSetting("Blocks/Tick", "Max blocks per tick", 1.f, 1.f, 5.f, 1.f);
    NumberSetting mServerTimeout = NumberSetting("Server Timeout", "How long to wait for server confirmation (ms)", 1500.f, 500.f, 5000.f, 100.f);
    BoolSetting mSwing = BoolSetting("Swing", "Swing animation", false);
    BoolSetting mHotbarOnly = BoolSetting("Hotbar Only", "Hotbar tools only", false);
    BoolSetting mRenderBlock = BoolSetting("Render Block", "Highlight mining block", true);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw TP path", true);
    BoolSetting mRenderTargets = BoolSetting("Render Targets", "Highlight found blocks", false);
    BoolSetting mShowBlockList = BoolSetting("Show Block List", "Show target block names", true);
    NumberSetting mFOV = NumberSetting("FOV", "Mining field of view", 360.f, 30.f, 360.f, 10.f);
    BoolSetting mToolSaver = BoolSetting("Tool Saver", "Switch to another tool when durability is low, or pause mining when none is left", true);
    NumberSetting mMinToolDurability = NumberSetting("Min Tool Durability %", "Stop using a tool below this durability%", 8.f, 1.f, 50.f, 1.f);

    BoolSetting mCoal = BoolSetting("Coal", "Coal ore", false);
    BoolSetting mIron = BoolSetting("Iron", "Iron ore", false);
    BoolSetting mGold = BoolSetting("Gold", "Gold ore", false);
    BoolSetting mDiamond = BoolSetting("Diamond", "Diamond ore", false);
    BoolSetting mEmerald = BoolSetting("Emerald", "Emerald ore", false);
    BoolSetting mLapis = BoolSetting("Lapis", "Lapis ore", false);
    BoolSetting mRedstone = BoolSetting("Redstone", "Redstone ore", false);
    BoolSetting mCopper = BoolSetting("Copper", "Copper ore", false);
    BoolSetting mAncientDebris = BoolSetting("Ancient Debris", "Netherite", false);
    BoolSetting mQuartz = BoolSetting("Quartz", "Nether quartz", false);
    BoolSetting mLeaves = BoolSetting("Leaves", "All leaf types", false);
    BoolSetting mWood = BoolSetting("Wood", "All log types", false);
    BoolSetting mSandstone = BoolSetting("Sandstone", "Sandstone", false);
    BoolSetting mSnow = BoolSetting("Snow", "Snow", false);
    BoolSetting mSpawner = BoolSetting("Spawner", "Mob spawner", false);

    OreMiner() : ModuleBase("OreMiner", "Mine any block at any distance", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mDestroySpeed, &mStepDistance, &mServerTimeout, &mSwing, &mHotbarOnly,
            &mVeinMiner, &mMineDelay, &mBlocksPerTick,
            &mRenderBlock, &mDrawPath, &mRenderTargets, &mShowBlockList, &mFOV,
            &mToolSaver, &mMinToolDurability,
            &mCoal, &mIron, &mGold, &mDiamond, &mEmerald, &mLapis,
            &mRedstone, &mCopper, &mAncientDebris, &mQuartz,
            &mLeaves, &mWood, &mSandstone, &mSnow, &mSpawner
        );

        VISIBILITY_CONDITION(mMinToolDurability, mToolSaver.mValue);

        mNames = {{Lowercase,"oreminer"},{LowercaseSpaced,"ore miner"},{Normal,"OreMiner"},{NormalSpaced,"Ore Miner"}};
    }

    static inline const std::vector<std::string> sCoalN = {"coal_ore"};
    static inline const std::vector<std::string> sIronN = {"iron_ore"};
    static inline const std::vector<std::string> sGoldN = {"gold_ore","nether_gold_ore"};
    static inline const std::vector<std::string> sDiamondN = {"diamond_ore"};
    static inline const std::vector<std::string> sEmeraldN = {"emerald_ore"};
    static inline const std::vector<std::string> sLapisN = {"lapis_ore"};
    static inline const std::vector<std::string> sRedstoneN = {"redstone_ore","lit_redstone_ore"};
    static inline const std::vector<std::string> sCopperN = {"copper_ore"};
    static inline const std::vector<std::string> sDebrisN = {"ancient_debris"};
    static inline const std::vector<std::string> sQuartzN = {"quartz_ore"};
    static inline const std::vector<std::string> sLeafN = {"leaves","azalea_leaves"};
    static inline const std::vector<std::string> sWoodN = {"log","wood","stem","hyphae"};
    static inline const std::vector<std::string> sSandstoneN = {"sandstone"};
    static inline const std::vector<std::string> sSnowN = {"snow","snow_layer"};
    static inline const std::vector<std::string> sSpawnerN = {"mob_spawner","spawner"};

    // Protected blocks (private claims etc)
    std::unordered_set<BlockPos> mProtectedPositions;

    // Custom blocks
    std::vector<std::string> mCustomBlockNames;

    static constexpr float SCAN_RADIUS = 28.f;
    // Spiral only covers chunks actually usable by findBestTarget
    // (28 blocks ≈ 2 chunks — scanning 7 chunks out just wasted minutes)
    static constexpr float CHUNK_RADIUS = 4.f;
    static constexpr float UPDATE_FREQ = 1.8f;
    static constexpr int CHUNKS_PER_TICK = 3;
    // Subchunks of the player's OWN chunk scanned per batch (priority),
    // so ores right next to you are picked up within ~1 second
    static constexpr int OWN_SUBS_PER_TICK = 2;

    struct ScanState {
        ChunkPos center;
        ChunkPos current;
        int subChunkIdx = 0;
        int dirIdx = 0;
        int steps = 1;
        int stepCount = 0;
    } mScan;
    int mOwnSubIdx = 0; // round-robin subchunk index for the player's own chunk

    struct FoundBlock { glm::ivec3 position; std::string name; };
    std::unordered_map<BlockPos, FoundBlock> mFoundBlocks;
    bool mKeyWasDown = false;

    static inline glm::ivec3 mCurrentBlockPos = {INT_MAX,INT_MAX,INT_MAX};
    int mCurrentBlockFace = -1;
    float mBreakingProgress = 0.f;
    float mCurrentDestroySpeed = 1.f;
    static inline bool mIsMiningBlock = false;
    bool mShouldSpoofSlot = true;
    int mPreviousSlot = -1;
    int mToolSlot = -1;

    // Waiting state for far block server confirmation
    bool mWaitingForBreak = false;
    uint64_t mWaitStartTime = 0;
    int mWaitRetries = 0;
    std::string mPendingVeinBlockName; // saved before break for VeinMiner BFS

    glm::vec3 mRots = {0,0,0};
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;

    void onEnable() override;
    void onDisable() override;

    bool isTargetBlock(const std::string& name);
    bool matchNames(const std::string& name, const std::vector<std::string>& list);
    void toggleCustomBlock(const std::string& name);
    bool isCustomBlock(const std::string& name);
    bool hasAnyTarget();

    void resetScanner();
    void moveToNextChunk();
    bool scanSubChunk(ChunkPos chunk, int subIdx);

    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool save);
    // Returns false when no suitable tool exists (Tool Saver) — no packets sent
    bool mineBlockAtPos(const glm::ivec3& pos, Actor* player);
    // Best tool slot with enough durability, or -1 → mining must pause
    int getMiningToolSlot(Block* block);
    void notifyToolStop();
    glm::ivec3 findBestTarget(Actor* player);
    bool isInPlayerFOV(Actor* player, const glm::vec3& blockCenter);

    // VeinMiner: BFS to find all connected blocks of same type
    std::vector<glm::ivec3> getConnectedVein(const glm::ivec3& start, int maxBlocks = 64);
    std::deque<glm::ivec3> mVeinQueue;
    uint64_t mLastMineTime = 0;
    uint64_t mLastToolWarnTime = 0;

    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        if (mIsMiningBlock) return "Mining";
        if (!mProtectedPositions.empty())
            return std::to_string(mFoundBlocks.size()) + " found (" + std::to_string(mProtectedPositions.size()) + " skip)";
        return std::to_string(mFoundBlocks.size()) + " found";
    }
};