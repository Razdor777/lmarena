#pragma once

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>


class OreMinerV2 : public ModuleBase<OreMinerV2> {
public:
  // === Mining settings ===
  NumberSetting mSpeed = NumberSetting("Speed", "Ticks between mining blocks",
                                       2.f, 0.f, 20.f, 1.f);
  NumberSetting mStepDistance = NumberSetting(
      "Step Distance", "Blocks per TP step", 8.f, 1.f, 12.f, 0.5f);
  NumberSetting mVerifyWait = NumberSetting(
      "Verify Wait", "Ticks to wait for drop check", 5.f, 2.f, 15.f, 1.f);
  BoolSetting mSilentAccept =
      BoolSetting("Silent Accept", "Prevent rubber banding", true);
  BoolSetting mSwing = BoolSetting("Swing", "Swing animation", true);
  BoolSetting mVeinMiner =
      BoolSetting("Vein Miner", "Mine all connected ores of same type", false);

  // === Quick TP (keybind to TP forward) ===
  NumberSetting mQuickTPDist = NumberSetting(
      "Quick TP Dist", "Distance to TP (blocks)", 100.f, 10.f, 500.f, 10.f);
  NumberSetting mQuickTPKey = NumberSetting(
      "Quick TP Key", "Key VK code (0=disabled)", 0.f, 0.f, 255.f, 1.f);

  // === Scan settings ===
  NumberSetting mRadius =
      NumberSetting("Radius", "Render / mine radius", 20.f, 1.f, 100.f, 0.01f);
  NumberSetting mChunkRadius = NumberSetting(
      "Chunk Radius", "Max chunk radius to scan", 4.f, 1.f, 32.f, 1.f);
  NumberSetting mChunkUpdatesPerTick = NumberSetting(
      "Chunks/Tick", "Subchunks to scan per tick", 5.f, 1.f, 24.f, 1.f);
  BoolSetting mOnlyExposedOres =
      BoolSetting("Only Exposed", "Only mine exposed ores", false);

  // === Ore toggles ===
  BoolSetting mCoal = BoolSetting("Coal", "Mine coal ore", true);
  BoolSetting mDiamond = BoolSetting("Diamond", "Mine diamond ore", true);

  // === Render settings ===
  BoolSetting mDrawBlocks =
      BoolSetting("Draw Blocks", "Draw ESP on found ores", true);
  BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw TP path", true);
  BoolSetting mDrawTarget =
      BoolSetting("Draw Target", "Highlight current block", true);
  BoolSetting mDrawCluster =
      BoolSetting("Draw Cluster", "Highlight active cluster", true);

  OreMinerV2()
      : ModuleBase(
            "OreMinerV2",
            "Mine coal / diamond ores with infinite reach, auto-skip fakes",
            ModuleCategory::Player, 0, false) {
    addSettings(&mSpeed, &mStepDistance, &mVerifyWait, &mSilentAccept, &mSwing,
                &mVeinMiner, &mQuickTPDist, &mQuickTPKey, &mRadius,
                &mChunkRadius, &mChunkUpdatesPerTick, &mOnlyExposedOres, &mCoal,
                &mDiamond, &mDrawBlocks, &mDrawPath, &mDrawTarget,
                &mDrawCluster);

    mNames = {{Lowercase, "oreminerv2"},
              {LowercaseSpaced, "ore miner v2"},
              {Normal, "OreMinerV2"},
              {NormalSpaced, "Ore Miner V2"}};
  }

  // =====================================================
  // Ore helpers
  // =====================================================
  static constexpr int COAL_ORE_ID = 16;
  static constexpr int DEEPSLATE_COAL_ORE_ID = 661;
  static constexpr int DIAMOND_ORE_ID = 56;
  static constexpr int DEEPSLATE_DIAMOND_ORE_ID = 660;

  enum class OreType { None, Coal, Diamond };

  static OreType getOreType(int id) {
    if (id == COAL_ORE_ID || id == DEEPSLATE_COAL_ORE_ID)
      return OreType::Coal;
    if (id == DIAMOND_ORE_ID || id == DEEPSLATE_DIAMOND_ORE_ID)
      return OreType::Diamond;
    return OreType::None;
  }

  static bool isSameOreType(int a, int b) {
    OreType ta = getOreType(a), tb = getOreType(b);
    return ta != OreType::None && ta == tb;
  }

  static void getClusterRange(OreType t, int &lo, int &hi) {
    switch (t) {
    case OreType::Coal:
      lo = 8;
      hi = 24;
      break;
    case OreType::Diamond:
      lo = 3;
      hi = 9;
      break;
    default:
      lo = 1;
      hi = 999;
      break;
    }
  }

  static ImColor getOreColor(int id) {
    switch (getOreType(id)) {
    case OreType::Coal:
      return ImColor(0.35f, 0.35f, 0.35f, 1.f);
    case OreType::Diamond:
      return ImColor(0.f, 1.f, 1.f, 1.f);
    default:
      return ImColor(1.f, 1.f, 1.f, 1.f);
    }
  }

  // =====================================================
  // Scan data
  // =====================================================
  struct FoundBlock {
    const Block *block;
    AABB aabb;
    ImColor color;
  };

  std::unordered_map<BlockPos, FoundBlock> mFoundBlocks;
  std::unordered_map<BlockPos, FoundBlock> mFilteredBlocks;
  std::unordered_set<BlockPos> mBlacklistedPositions;

  ChunkPos mSearchCenter;
  ChunkPos mCurrentChunkPos;
  int mSubChunkIndex = 0;
  int mDirectionIndex = 0;
  int mSteps = 1;
  int mStepsCount = 0;
  bool mScanComplete = false;
  bool mScanDirty = false;
  uint64_t mLastClusterUpdate = 0;

  // =====================================================
  // Mining state machine
  // =====================================================
  enum class MineState { Idle, Verifying, Mining };

  MineState mState = MineState::Idle;
  glm::vec3 mRots = {0, 0, 0};

  std::vector<BlockPos> mCurrentCluster;
  int mCurrentClusterIdx = 0;
  BlockPos mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};

  int mVerifyTicksLeft = 0;
  std::unordered_map<std::string, int> mPreSnapshot;

  int mTickDelay = 0;
  int mBlocksMined = 0;
  int mFakeClusters = 0;

  std::unordered_set<BlockPos> mMinedPositions;

  // Render (protected by mMutex)
  std::vector<glm::vec3> mPacketPositions;
  uint64_t mLastPathTime = 0;
  std::recursive_mutex mMutex;

  // Quick TP key state
  bool mQuickTPKeyWasDown = false;

  // ← ДОБАВЛЕНО: Key binding state
  bool mIsBindingKey = false;
  bool mBindWaitRelease = false;

  // =====================================================
  // Scan methods
  // =====================================================
  std::vector<int> getEnabledBlockIds();
  void resetScan();
  void moveToNext();
  bool processSub(ChunkPos chunk, int index);
  void tryProcessSub(bool &ok, ChunkPos chunk, int index);

  // =====================================================
  // Cluster methods
  // =====================================================
  std::vector<BlockPos>
  getCluster(const BlockPos &start,
             const std::unordered_map<BlockPos, FoundBlock> &blocks);
  void updateClusters();
  void removeBlockIncremental(const BlockPos &pos);
  void blacklistPositions(const std::vector<BlockPos> &positions);
  std::vector<BlockPos>
  findNearestCluster(glm::vec3 playerPos,
                     const std::unordered_set<BlockPos> &exclude);

  // =====================================================
  // Mining methods
  // =====================================================
  void onEnable() override;
  void onDisable() override;

  std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
  void straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
  void mineBlockAtPos(const BlockPos &pos, Actor *player);

  std::unordered_map<std::string, int> getInventorySnapshot();
  bool checkForFakeDrop(const std::unordered_map<std::string, int> &before,
                        const std::unordered_map<std::string, int> &after);

  // ← ДОБАВЛЕНО: Key helpers
  static const char *getKeyName(int vk);
  static bool sIsAnyKeyHeld();
  static int sFindHeldKey();

  // =====================================================
  // Events
  // =====================================================
  void onBaseTickEvent(class BaseTickEvent &event);
  void onPacketOutEvent(class PacketOutEvent &event);
  void onPacketInEvent(class PacketInEvent &event);
  void onRenderEvent(class RenderEvent &event);
  void onBlockChangedEvent(class BlockChangedEvent &event);

  std::string getSettingDisplay() override {
    std::string s;
    switch (mState) {
    case MineState::Idle:
      s = "Idle";
      break;
    case MineState::Verifying:
      s = "Verify";
      break;
    case MineState::Mining:
      s = std::to_string(mCurrentClusterIdx) + "/" +
          std::to_string(mCurrentCluster.size());
      break;
    }
    if (!mFilteredBlocks.empty())
      s += " [" + std::to_string(mFilteredBlocks.size()) + "]";
    return s;
  }
};