#pragma once

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <fstream>
#include <map>


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

  // === Drop check ===
  // Сколько максимум ждать, пока сервер (плагин авто-подбора) положит дроп
  // сломанного блока в инвентарь. Пришёл ожидаемый предмет (алмаз/уголь/
  // изумруд) -> руда настоящая. Пришло что-то другое (булыжник, камень...)
  // -> фейк. За это время не пришло ничего -> NODROP.
  NumberSetting mDropTimeout = NumberSetting(
      "Drop Timeout", "Max wait (ms) for the drop to land in inventory",
      800.f, 100.f, 3000.f, 50.f);

  // === Debug ===
  // Включил -> модуль проверяет КАЖДЫЙ сломанный блок по тому, что реально
  // упало в инвентарь, и сохраняет результат в Juzdex\Dumps\:
  //   OreMinerV2_<дата>_verified.csv - все проверенные координаты + вердикт,
  //                                    что именно выпало, сколько ждали;
  //   OreMinerV2_<дата>_real.txt     - ТОЛЬКО настоящие руды  "x y z руда";
  //   OreMinerV2_<дата>_fake.txt     - ТОЛЬКО фейки           "x y z руда".
  // Вердикты:
  //   REAL    - выпал алмаз / уголь / изумруд;
  //   FAKE    - выпало другое (булыжник...) или сервер сам показал камень
  //             вместо руды (reason = revealed, такой блок даже не ломаем);
  //   NODROP  - дроп не пришёл: инвентарь полон / сервер отменил ломание /
  //             пинг больше Drop Timeout (в real/fake.txt не попадает);
  //   SKIPPED - не ломали, т.к. первый блок кластера оказался фейком.
  // Каждая координата пишется один раз, файл сбрасывается на диск сразу.
  BoolSetting mDebug = BoolSetting(
      "Debug", "Check every mined ore by its drop, save REAL/FAKE coords to Juzdex\\Dumps",
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
                &mDiamond, &mEmerald, &mDropTimeout, &mDebug, &mDrawBlocks,
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

  // Воздух/вода/лава (с учётом блоков без legacy-ID, см. BlockLegacy::isAir)
  static bool isAirOrLiquid(const Block *block);
  // Короткое имя блока без "minecraft:" (для логов)
  static std::string blockNameOf(const Block *block);

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
  // Idle      - ищем ближайший кластер
  // Mining    - ломаем следующий блок кластера
  // Verifying - блок сломан, ждём дроп в инвентаре (первый блок кластера
  //             всегда; при включённом Debug - каждый блок)
  enum class MineState { Idle, Mining, Verifying };

  MineState mState = MineState::Idle;
  glm::vec3 mRots = {0, 0, 0};

  int mCurrentClusterOreId = 0;

  std::vector<BlockPos> mCurrentCluster;
  int mCurrentClusterIdx = 0;
  BlockPos mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};

  bool mNeedFirstVerify = false; // сейчас проверяется первый блок кластера

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
  // Drop check (real / fake по дропу в инвентаре)
  // =====================================================
  enum class DropVerdict {
    Real,   // выпал ожидаемый предмет (алмаз / уголь / изумруд)
    Fake,   // выпало что-то другое, либо сервер показал вместо руды камень
    NoDrop, // за Drop Timeout в инвентарь ничего не пришло
    Skipped // не ломали: первый блок кластера оказался фейком
  };

  struct PendingCheck {
    BlockPos pos{};
    int oreId = 0;               // какую руду видел сканер на этой позиции
    int clusterIndex = 0;        // номер блока в кластере (с 1)
    bool firstOfCluster = false; // решает судьбу всего кластера
    uint64_t sentAt = 0;         // когда ушли пакеты ломания (ms)
    bool dropSeen = false;       // дроп уже пришёл, ждём ещё тик на хвосты
    std::map<std::string, int> gained; // что прибавилось в инвентаре
    int restoredId = -1;         // что сервер вернул на место блока (-1 - ничего)
    std::string restoredName;
    std::unordered_map<std::string, int> before; // инвентарь ДО ломания
  };

  bool mHasPending = false;
  PendingCheck mPending;
  uint64_t mLastInvFullWarn = 0;

  // =====================================================
  // Debug dump (только проверенные блоки)
  // =====================================================
  std::string mDebugCsvPath;
  std::string mDebugRealPath;
  std::string mDebugFakePath;
  std::ofstream mDebugCsv;
  std::ofstream mDebugReal;
  std::ofstream mDebugFake;
  std::unordered_set<BlockPos> mDebugWritten; // каждая координата - ровно один раз
  int mDebugRealCount = 0;
  int mDebugFakeCount = 0;
  int mDebugNoDropCount = 0;
  int mDebugSkippedCount = 0;
  bool mDebugWasOn = false;
  int mClusterSerial = 0; // номер кластера в текущей сессии (для CSV)

  void syncDebugState(); // открыть/закрыть файлы по галочке Debug
  void debugStart();     // новые файлы под дамп
  void debugStop();      // закрыть файлы + итог в чат
  void debugRecord(const BlockPos &pos, int oreId, DropVerdict verdict,
                   const char *reason, const std::string &details,
                   uint64_t waitMs, int clusterIndex);
  static const char *oreTypeName(int blockId);
  static const char *verdictName(DropVerdict verdict);
  static std::string localTimeString(const char *fmt);

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

  void startNextCluster(Actor *player, class BlockSource *source,
                        glm::vec3 playerPos);
  void mineNextInCluster(Actor *player, class BlockSource *source);
  void finishCluster();
  void resetMining();

  // =====================================================
  // Drop check methods
  // =====================================================
  std::unordered_map<std::string, int> getInventorySnapshot();
  int countFreeSlots();
  static std::map<std::string, int>
  diffInventory(const std::unordered_map<std::string, int> &before,
                const std::unordered_map<std::string, int> &after);
  static const char *expectedDropName(int oreId);
  static bool hasExpectedDrop(const std::map<std::string, int> &gained,
                              int oreId);
  static std::string formatItems(const std::map<std::string, int> &items);

  void beginVerify(const BlockPos &pos, int oreId, int clusterIndex,
                   bool firstOfCluster,
                   std::unordered_map<std::string, int> before);
  bool tickVerify(); // true -> проверка закончена, кластер ломаем дальше
  // details по значению: сюда часто передают поле самого mPending, которое
  // внутри finishVerify обнуляется
  bool finishVerify(DropVerdict verdict, const char *reason,
                    std::string details, uint64_t waitedMs);

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
    case MineState::Verifying:
      s = std::to_string(mCurrentClusterIdx) + "/" +
          std::to_string(mCurrentCluster.size());
      break;
    }
    if (!mFilteredBlocks.empty())
      s += " [" + std::to_string(mFilteredBlocks.size()) + "]";
    return s;
  }
};
