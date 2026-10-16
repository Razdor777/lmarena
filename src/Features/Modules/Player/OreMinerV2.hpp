#pragma once

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <fstream>


class OreMinerV2 : public ModuleBase<OreMinerV2> {
public:
  // === Mining settings ===
  NumberSetting mSpeed = NumberSetting("Speed", "Ticks between mining blocks",
                                       2.f, 0.f, 20.f, 1.f);
  NumberSetting mStepDistance = NumberSetting(
      "Step Distance", "Blocks per TP step", 8.f, 1.f, 12.f, 0.5f);
  BoolSetting mSilentAccept =
      BoolSetting("Silent Accept", "Prevent rubber banding", true);
  BoolSetting mSwing = BoolSetting("Swing", "Swing animation", true);
  BoolSetting mVeinMiner =
      BoolSetting("Vein Miner", "Mine all connected ores of same type", false);

  // === Follow mode (TP to cluster and stay there) ===
  BoolSetting mFollow = BoolSetting("Follow", "Teleport to clusters and mine continuously without returning", false);

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
  BoolSetting mEmerald = BoolSetting("Emerald", "Mine emerald ore", true);

  // === Fake check / Debug ===
  // Сервер (плагин авто-подбора) кладёт дроп сломанного блока сразу в
  // инвентарь. Поэтому «инвентарь до слома» vs «инвентарь после» однозначно
  // говорит, что на этой координате было НА САМОМ ДЕЛЕ:
  //   пришёл алмаз/уголь/изумруд (или сама руда при шёлке) -> руда настоящая;
  //   пришло что-то другое (булыжник, камень, ...)         -> это был фейк.
  // Verify Timeout — сколько максимум ждать, пока дроп долетит до инвентаря.
  // Вердикт выносится сразу, как только инвентарь изменился; таймаут нужен
  // только на случай, когда не пришло вообще ничего.
  NumberSetting mVerifyTimeout = NumberSetting(
      "Verify Timeout", "Max ms to wait for the drop to land in inventory",
      1000.f, 100.f, 5000.f, 50.f);

  // Включил -> так проверяется КАЖДЫЙ сломанный блок (а не только первый в
  // кластере), и в Juzdex\Dumps\ сохраняется только проверенное:
  //   OreMinerV2_<дата_время>.csv       — все вердикты по координатам
  //   OreMinerV2_<дата_время>_real.txt  — только настоящие руды, "x y z ore"
  //   OreMinerV2_<дата_время>_fake.txt  — только фейки,         "x y z ore"
  BoolSetting mDebug = BoolSetting(
      "Debug", "Check every mined ore by its drop, save real/fake coords to Dumps",
      false);

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
            "Mine coal / diamond / emerald ores with infinite reach, auto-skip fakes",
            ModuleCategory::Player, 0, false) {
    addSettings(&mSpeed, &mStepDistance, &mSilentAccept, &mSwing,
                &mVeinMiner, &mFollow, &mQuickTPDist, &mQuickTPKey, &mRadius,
                &mChunkRadius, &mChunkUpdatesPerTick, &mOnlyExposedOres, &mCoal,
                &mDiamond, &mEmerald, &mVerifyTimeout, &mDebug, &mDrawBlocks,
                &mDrawPath, &mDrawTarget, &mDrawCluster);

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
  static constexpr int EMERALD_ORE_ID = 129;
  static constexpr int DEEPSLATE_EMERALD_ORE_ID = 662;

  enum class OreType { None, Coal, Diamond, Emerald };

  static OreType getOreType(int id) {
    if (id == COAL_ORE_ID || id == DEEPSLATE_COAL_ORE_ID)
      return OreType::Coal;
    if (id == DIAMOND_ORE_ID || id == DEEPSLATE_DIAMOND_ORE_ID)
      return OreType::Diamond;
    if (id == EMERALD_ORE_ID || id == DEEPSLATE_EMERALD_ORE_ID)
      return OreType::Emerald;
    return OreType::None;
  }

  static bool isSameOreType(int a, int b) {
    OreType ta = getOreType(a), tb = getOreType(b);
    return ta != OreType::None && ta == tb;
  }

  static bool isDeepslateOre(int id) {
    return id == DEEPSLATE_COAL_ORE_ID || id == DEEPSLATE_DIAMOND_ORE_ID ||
           id == DEEPSLATE_EMERALD_ORE_ID;
  }

  static void getClusterRange(OreType t, int &lo, int &hi) {
    switch (t) {
    case OreType::Coal:
      lo = 8;
      hi = 24;
      break;
    case OreType::Diamond:
    case OreType::Emerald:
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
    case OreType::Emerald:
      return ImColor(0.1f, 1.f, 0.25f, 1.f);
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
  enum class MineState { Idle, Mining };

  MineState mState = MineState::Idle;
  glm::vec3 mRots = {0, 0, 0};

  int mCurrentClusterOreId = 0; // тип руды кластера (по первому блоку)

  std::vector<BlockPos> mCurrentCluster;
  int mCurrentClusterIdx = 0;
  BlockPos mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};

  bool mNeedFirstVerify = false; // ждём дроп с первого блока кластера

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

  // Key binding state
  bool mIsBindingKey = false;
  bool mBindWaitRelease = false;

  // =====================================================
  // Drop verification (сервер кладёт дроп сразу в инвентарь)
  // =====================================================
  enum class DropVerdict {
    Pending,  // в инвентаре ещё ничего не прибавилось, таймаут не вышел
    Real,     // прибавилась именно эта руда       -> блок был настоящим
    Fake,     // прибавилось что-то другое          -> блок был фейком
    NoDrop,   // за Verify Timeout не пришло ничего (лаг / сервер не дал сломать)
    InvFull,  // ничего не пришло, а свободных слотов нет — проверить нельзя
    Revealed, // не ломали: сервер уже показал, что тут не эта руда
    Skipped,  // не ломали: первый блок кластера оказался фейком
  };

  struct DropCheck {
    DropVerdict verdict = DropVerdict::Pending;
    std::string got; // что прибавилось: "diamond x1", "cobblestone x1 + dirt x1"
  };

  // Блок, который уже сломан и чей дроп мы ждём. Одновременно ждём максимум
  // один блок: следующий ломается только после вердикта по предыдущему, иначе
  // нельзя сказать, чей именно дроп упал в инвентарь.
  struct PendingCheck {
    bool active = false;
    bool probe = false; // первый блок кластера: его вердикт решает весь кластер
    BlockPos pos{};
    int blockId = 0;
    uint64_t sentAt = 0;
    int clusterNo = 0;
    int clusterIdx = 0;
    int clusterSize = 0;
    bool settling = false; // инвентарь уже изменился, ждём тик «досыпания»
    int seenWaitMs = 0;    // через сколько мс после слома он изменился
    std::unordered_map<std::string, int> before; // инвентарь до слома
  };

  enum class PollResult {
    Waiting,       // дроп ещё не пришёл — ждём дальше
    Resolved,      // вердикт вынесен, можно ломать следующий блок
    ClusterDropped // вердикт по первому блоку — кластер брошен
  };

  PendingCheck mPending;
  int mClusterNo = 0;         // сквозной номер кластера (для CSV)
  bool mInvFullPause = false; // нет свободного слота — дроп некуда класть

  // =====================================================
  // Debug dump — только проверенные дропом блоки
  // =====================================================
  struct DebugRow {
    BlockPos pos{};
    int blockId = 0;
    DropVerdict verdict = DropVerdict::Skipped;
    std::string got;
    int waitMs = -1; // -1 = блок не ломали
    int clusterNo = 0;
    int clusterIdx = 0;
    int clusterSize = 0;
    bool listAsFake = false; // попадает ли координата в _fake.txt
  };

  std::string mDebugBasePath; // ...\Dumps\OreMinerV2_<дата_время>
  std::ofstream mDebugCsv;
  std::ofstream mDebugRealTxt;
  std::ofstream mDebugFakeTxt;
  // Одна строка на координату. Исключение: «skipped» (вывод без слома) можно
  // потом уточнить настоящим вердиктом, если блок всё-таки сломали.
  std::unordered_map<BlockPos, DropVerdict> mDebugWritten;
  bool mDebugWasOn = false;
  bool mDebugOpenFailed = false;
  int mDebugReal = 0;
  int mDebugFake = 0;
  int mDebugNoDrop = 0;
  int mDebugRevealed = 0;
  int mDebugSkipped = 0;

  void debugSync();                     // Debug вкл/выкл -> новая сессия / закрыть
  bool debugOpen();                     // файлы создаются на первом вердикте
  void debugWrite(const DebugRow &row); // дописать вердикт во все файлы
  void debugClose();                    // закрыть файлы + итог в чат
  static const char *oreTypeName(int blockId);
  static const char *expectedDropName(int blockId);
  static const char *verdictName(DropVerdict v);
  static std::string cleanName(std::string name); // без "minecraft:", для CSV

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
  bool mineBlockAtPos(const BlockPos &pos, Actor *player);

  std::unordered_map<std::string, int> getInventorySnapshot();
  bool hasFreeInventorySlot();
  void pauseForFullInventory();
  void resumeAfterFullInventory();

  // Сравнить инвентарь до/после слома руды oreBlockId -> вердикт.
  static DropCheck
  classifyDrop(const std::unordered_map<std::string, int> &before,
               const std::unordered_map<std::string, int> &after,
               int oreBlockId);
  void startPendingCheck(const BlockPos &pos, int blockId, bool probe,
                         int clusterIdx,
                         std::unordered_map<std::string, int> before);
  PollResult pollPendingCheck();
  void recordVerdict(const PendingCheck &pc, const DropCheck &chk, int waitMs);
  void recordUnminedRow(const BlockPos &pos, int blockId, DropVerdict verdict,
                        const std::string &got, int clusterIdx,
                        bool listAsFake);
  int knownBlockIdAt(const BlockPos &pos) const;
  void abandonCluster(); // бросить кластер без чёрного списка

  // Key helpers
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