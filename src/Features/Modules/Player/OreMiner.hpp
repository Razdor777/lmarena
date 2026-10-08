#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>

class Block;
class BlockLegacy;
class BlockSource;

class OreMiner : public ModuleBase<OreMiner>
{
public:
    NumberSetting mRange        = NumberSetting("Range", "Max distance to mine blocks", 12.f, 2.f, 32.f, 1.f);
    NumberSetting mDestroySpeed = NumberSetting("Destroy Speed", "Break threshold", 1.f, 0.01f, 1.f, 0.01f);
    NumberSetting mStepDistance = NumberSetting("Step Distance", "TP step", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting   mVeinMiner    = BoolSetting("Vein Miner", "Break all connected blocks of same type", false);
    NumberSetting mMineDelay    = NumberSetting("Mine Delay", "Delay between blocks (ms)", 50.f, 0.f, 500.f, 10.f);
    NumberSetting mBlocksPerTick= NumberSetting("Blocks/Tick", "Max blocks per tick", 1.f, 1.f, 5.f, 1.f);
    NumberSetting mServerTimeout= NumberSetting("Server Timeout", "How long to wait for server confirmation (ms)", 1500.f, 500.f, 5000.f, 100.f);
    BoolSetting   mSwing        = BoolSetting("Swing", "Swing animation", false);
    BoolSetting   mHotbarOnly   = BoolSetting("Hotbar Only", "Hotbar tools only", false);
    BoolSetting   mRenderBlock  = BoolSetting("Render Block", "Highlight mining block", true);
    BoolSetting   mDrawPath     = BoolSetting("Draw Path", "Draw TP path", true);
    BoolSetting   mRenderTargets= BoolSetting("Render Targets", "Highlight found blocks", false);
    BoolSetting   mShowBlockList= BoolSetting("Show Block List", "Show target block names", true);
    NumberSetting mFOV          = NumberSetting("FOV", "Mining field of view", 360.f, 30.f, 360.f, 10.f);
    BoolSetting   mToolSaver    = BoolSetting("Tool Saver", "Switch to another tool when durability is low, or pause mining when none is left", true);
    NumberSetting mMinToolDurability = NumberSetting("Min Tool Durability %", "Stop using a tool below this durability%", 8.f, 1.f, 50.f, 1.f);
    BoolSetting   mDebug        = BoolSetting("Debug", "Print scanner stats to chat", false);

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
    BoolSetting mWood = BoolSetting("Wood", "Logs", false);
    EnumSetting mWoodType = EnumSetting("Wood Type", "Which logs to mine", 0,
        "All", "Oak", "Birch", "Spruce", "Jungle", "Acacia", "Dark Oak",
        "Mangrove", "Cherry", "Crimson", "Warped", "Pale Oak");
    BoolSetting mSandstone = BoolSetting("Sandstone", "Sandstone", false);
    BoolSetting mSnow = BoolSetting("Snow", "Snow", false);
    BoolSetting mSpawner = BoolSetting("Spawner", "Mob spawner", false);

    OreMiner() : ModuleBase("OreMiner", "Mine any block at any distance", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mRange, &mDestroySpeed, &mStepDistance, &mServerTimeout, &mSwing, &mHotbarOnly,
            &mVeinMiner, &mMineDelay, &mBlocksPerTick,
            &mRenderBlock, &mDrawPath, &mRenderTargets, &mShowBlockList, &mFOV,
            &mToolSaver, &mMinToolDurability, &mDebug,
            &mCoal, &mIron, &mGold, &mDiamond, &mEmerald, &mLapis,
            &mRedstone, &mCopper, &mAncientDebris, &mQuartz,
            &mLeaves, &mWood, &mWoodType, &mSandstone, &mSnow, &mSpawner
        );

        VISIBILITY_CONDITION(mMinToolDurability, mToolSaver.mValue);
        VISIBILITY_CONDITION(mWoodType, mWood.mValue);

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

    std::unordered_set<BlockPos> mProtectedPositions;
    std::vector<std::string> mCustomBlockNames;

    // ================= Scanner =================
    static constexpr uint64_t SCAN_INTERVAL_MS   = 90;   // как часто крутить сканер
    static constexpr int      NEAR_RADIUS        = 6;    // прямой куб вокруг игрока (всегда, гарантированно)
    static constexpr int      SUBCHUNKS_PER_SCAN = 6;    // сабчанков дальнего скана за один проход
    static constexpr int      CLEANUP_PER_TICK   = 200;  // записей mFoundBlocks проверяем за тик

    ChunkPos                 mScanCenter;
    std::vector<glm::ivec2>  mChunkOrder;      // смещения чанков, отсортированные по дистанции
    size_t                   mChunkCursor = 0;
    int                      mSubCursor   = -1;
    size_t                   mCleanCursor = 0;
    uint64_t                 mLastScanTime = 0;
    uint64_t                 mLastDebugTime = 0;

    std::unordered_map<BlockLegacy*, bool> mTargetCache;   // BlockLegacy* -> является ли целью
    uint64_t mTargetSig = 0;                                // сигнатура настроек целей

    struct FoundBlock { glm::ivec3 position; BlockLegacy* legacy; };
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

    bool mWaitingForBreak = false;
    uint64_t mWaitStartTime = 0;
    int mWaitRetries = 0;
    std::string mPendingVeinBlockName;

    glm::vec3 mRots = {0,0,0};
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;

    void onEnable() override;
    void onDisable() override;

    bool isTargetBlock(const std::string& name);
    bool isTargetLegacy(BlockLegacy* legacy);
    bool matchNames(const std::string& name, const std::vector<std::string>& list);
    void toggleCustomBlock(const std::string& name);
    bool isCustomBlock(const std::string& name);
    bool hasAnyTarget();
    uint64_t computeTargetSig();

    void resetScanner();
    int  chunkRadius() const;
    void rebuildChunkOrder(const ChunkPos& center);
    void scanNear(BlockSource* source, Actor* player);
    void scanFarStep(BlockSource* source, Actor* player);
    bool scanSubChunk(const ChunkPos& chunk, int subIdx, int& outFound);
    void cleanupFound(BlockSource* source, Actor* player);
    void addFound(const glm::ivec3& pos, BlockLegacy* legacy);

    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool save);
    bool mineBlockAtPos(const glm::ivec3& pos, Actor* player);
    int  getMiningToolSlot(Block* block);
    void notifyToolStop();
    glm::ivec3 findBestTarget(Actor* player);
    bool isInPlayerFOV(Actor* player, const glm::vec3& blockCenter);

    std::vector<glm::ivec3> getConnectedVein(const glm::ivec3& start, int maxBlocks = 64);
    std::deque<glm::ivec3> mVeinQueue;
    uint64_t mLastMineTime = 0;
    uint64_t mLastToolWarnTime = 0;

    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onBlockChangedEvent(class BlockChangedEvent& event);

    std::string getSettingDisplay() override {
        if (mIsMiningBlock) return "Mining";
        if (!mProtectedPositions.empty())
            return std::to_string(mFoundBlocks.size()) + " found (" + std::to_string(mProtectedPositions.size()) + " skip)";
        return std::to_string(mFoundBlocks.size()) + " found";
    }
};
