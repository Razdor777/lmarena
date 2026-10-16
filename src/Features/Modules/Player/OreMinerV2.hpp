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
  // Анти-xray рисует фейковые руды по шаблону: ОДНА И ТА ЖЕ фигура на одном и
  // том же месте внутри чанка (x%16, y, z%16) повторяется в соседних чанках.
  // Настоящие жилы случайные и так не повторяются. Включено — такие блоки не
  // копаются вообще (даже первый, ради проверки) и не рисуются.
  BoolSetting mPatternFilter = BoolSetting(
      "Pattern Filter",
      "Skip fakes that repeat at the same spot in neighbour chunks "
      "(anti-xray pattern)",
      true);

  // === Ore toggles ===
  BoolSetting mCoal = BoolSetting("Coal", "Mine coal ore", true);
  BoolSetting mDiamond = BoolSetting("Diamond", "Mine diamond ore", true);
  BoolSetting mEmerald = BoolSetting("Emerald", "Mine emerald ore", true);

  // === Debug ===
  // Включил -> КАЖДЫЙ сломанный блок проверяется по тому, что реально упало в
  // инвентарь (на сервере автоподбор): ждали алмаз и пришёл алмаз — "real",
  // пришло что-то другое (булыжник, глубинный сланец, ...) — "fake", не пришло
  // ничего — "nodrop". Всё пишется в ОДИН файл Juzdex\Dumps\OreMinerV2.txt,
  // строка на блок: "x y z руда вердикт что_упало время". Файл общий для всех
  // сессий: каждая новая дописывается в конец под своим заголовком с датой,
  // в конце сессии — строка с итогом.
  BoolSetting mDebug = BoolSetting(
      "Debug",
      "Check every mined ore by its drop, save real/fake coords to Dumps",
      false);
  // Старый «сырой» дамп: всё, что модуль увидел (вместе с фейками), без
  // проверки. По умолчанию выключен — Debug теперь пишет отфильтрованное.
  // Если включить — отдельный файл Juzdex\Dumps\OreMinerV2_seen.txt.
  BoolSetting mDumpSeen = BoolSetting(
      "Dump Seen", "Also dump every seen ore, unverified (old Debug)", false);
  // Сколько ждать, пока дроп долетит до инвентаря. Пришёл раньше — вердикт
  // выносится сразу, ждать весь таймаут не нужно. Работает и без Debug:
  // по нему же проверяется первый блок каждого кластера.
  NumberSetting mVerifyTimeout = NumberSetting(
      "Verify Timeout", "Max ms to wait for the drop to reach inventory",
      500.f, 100.f, 3000.f, 50.f);

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
                &mChunkRadius, &mChunkUpdatesPerTick, &mOnlyExposedOres,
                &mPatternFilter, &mCoal,
                &mDiamond, &mEmerald, &mDebug, &mDumpSeen, &mVerifyTimeout,
                &mDrawBlocks, &mDrawPath, &mDrawTarget, &mDrawCluster);

    VISIBILITY_CONDITION(mDumpSeen, mDebug.mValue);

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

  // Что должно упасть с настоящей руды. Ищется подстрокой в имени предмета,
  // поэтому шёлковое касание ("diamond_ore", "deepslate_coal_ore") тоже
  // засчитывается как настоящая руда.
  static const char *expectedDropName(int id) {
    switch (getOreType(id)) {
    case OreType::Coal:
      return "coal";
    case OreType::Diamond:
      return "diamond";
    case OreType::Emerald:
      return "emerald";
    default:
      return nullptr;
    }
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

  int mCurrentClusterOreId = 0;

  std::vector<BlockPos> mCurrentCluster;
  int mCurrentClusterIdx = 0;
  BlockPos mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};

  int mTickDelay = 0;
  int mBlocksMined = 0;
  int mFakeClusters = 0;
  int mClusterNo = 0; // сквозной номер кластера (для дампа)

  // =====================================================
  // Drop verification — что реально упало в инвентарь после удара
  // =====================================================
  // Снимок инвентаря: имя предмета -> сколько всего штук.
  using InvSnapshot = std::unordered_map<std::string, int>;
  // Прирост между двумя снимками: (имя предмета, +штук), по имени.
  using DropList = std::vector<std::pair<std::string, int>>;

  enum class DropVerdict {
    Real,   // упало то, что должно падать с этой руды
    Fake,   // упало что-то другое (булыжник, сланец, ...)
    NoDrop, // за Verify Timeout в инвентарь не пришло ничего
    Unknown // проверку прервали (выключение модуля, смена мира)
  };

  // Блок, который уже сломан и ждёт, пока его дроп долетит до инвентаря.
  // Пока проверка не закончена, следующий блок не ломается — иначе дроп
  // нельзя честно привязать к координате.
  struct PendingCheck {
    bool active = false;
    bool decides = false; // первый блок кластера: от него зависит, копать ли
                          // остальные (так же, как раньше)
    BlockPos pos{};
    int oreId = 0;  // какой рудой числился блок -> что должно выпасть
    int clusterNo = 0;
    int clusterSize = 0;
    uint64_t minedAt = 0;
    uint64_t firstGainAt = 0; // когда в инвентаре впервые что-то прибавилось
    bool invFull = false;     // все 36 слотов заняты — дропу некуда лечь
    InvSnapshot before;       // инвентарь прямо перед ударом
    DropList seen;            // максимум прироста за всё ожидание
  };

  PendingCheck mPending;
  int mClusterReal = 0, mClusterFake = 0, mClusterNoDrop = 0;
  uint64_t mLastInvFullWarn = 0;

  std::unordered_set<BlockPos> mMinedPositions;

  // =====================================================
  // Pattern filter — фейки анти-xray повторяются от чанка к чанку
  // =====================================================
  static uint64_t chunkKeyOf(int cx, int cz) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
           static_cast<uint32_t>(cz);
  }
  static uint64_t chunkKeyOf(const BlockPos &p) {
    return chunkKeyOf(p.x >> 4, p.z >> 4);
  }
  // Место внутри чанка: x&15, z&15 и y (со сдвигом, чтобы y<0 тоже влез).
  static uint32_t spotOf(const BlockPos &p) {
    return (static_cast<uint32_t>(p.x & 15) << 16) |
           (static_cast<uint32_t>(p.z & 15) << 12) |
           (static_cast<uint32_t>(p.y + 2048) & 0xFFFu);
  }
  static constexpr int kPatternReach = 8;     // чанков в каждую сторону
  static constexpr int kPatternMinSpots = 2;  // общих мест с чанком-соседом
  static constexpr int kPatternMinChunks = 2; // ...и таких соседей на линии

  // Все руды, которые модуль когда-либо видел, по чанкам. НЕ чистится, когда
  // блок сломан или ушёл в чёрный список: повтор фигуры остаётся уликой.
  // Есть ключ — чанк уже сканировался (даже если руд в нём нет). Живёт, пока
  // тот же мир (переживает выключение модуля), сбрасывается при смене мира.
  std::unordered_map<uint64_t, std::unordered_set<uint32_t>> mChunkSpots;
  const void *mSpotsWorld = nullptr;
  std::unordered_set<BlockPos> mPatternBlocks;  // сейчас считаются шаблоном
  std::unordered_set<BlockPos> mPatternSkipped; // за сессию модуля (счётчик)
  int mDbgPattern = 0;                          // за сессию Debug

  void syncSpotsWorld();
  void markChunkScanned(const ChunkPos &chunk);
  void rememberOre(const BlockPos &pos, bool onlyScannedChunk);
  bool patternReady(const std::vector<BlockPos> &cluster) const;
  void findPatternBlocks(const std::vector<BlockPos> &cluster,
                         std::unordered_set<BlockPos> &out) const;
  void prunePatternMemory();

  // =====================================================
  // NoDrop streak — сервер перестал ломать блоки
  // =====================================================
  static constexpr int kNoDropStreak = 3; // столько «без дропа» подряд
  int mNoDropStreak = 0;     // nodrop подряд (real/fake обнуляют)
  int mStreakLevel = 0;      // сработало подряд -> пауза растёт
  uint64_t mPauseUntil = 0;  // до этого момента ничего не копаем
  std::vector<BlockPos> mSinceLastDrop;      // сломано/в ЧС с последнего дропа
  std::unordered_set<BlockPos> mRetriedOnce; // уже возвращали на перепроверку
  void trackNoDropStreak(DropVerdict v, bool invFull, uint64_t now);

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
  // Debug dump
  // =====================================================
  // Строка «сырого» дампа (Dump Seen): блок, который модуль увидел.
  struct DebugRow {
    BlockPos pos{};
    int      blockId  = 0;
    float    distance = 0.f;
    bool     onScreen = false;
    float    screenX  = 0.f;
    float    screenY  = 0.f;
  };

  // Один общий файл на всё: сессия Debug = заголовок с датой + строки + итог.
  // Файл открывается на дозапись лениво, при первой строке: включил и
  // выключил, ничего не сломав, — в файл ничего не добавится.
  std::string mDebugDir;                // ...\Juzdex\Dumps\ (папка)
  std::string mDebugSession;            // "2026-10-16 14:03:22" — для заголовка
  std::ofstream mDbgFile;               // OreMinerV2.txt      — проверенные блоки
  std::ofstream mDbgSeenFile;           // OreMinerV2_seen.txt — Dump Seen
  bool mDbgFileFailed = false;          // не смогли открыть — не долбим диск
  bool mDbgSeenFailed = false;
  std::unordered_set<BlockPos> mDebugWritten; // Dump Seen: координата — один раз
  int  mDebugRows    = 0;                     // Dump Seen: сколько строк
  int  mDbgReal = 0, mDbgFake = 0, mDbgNoDrop = 0, mDbgUnknown = 0,
       mDbgSkip = 0;
  bool mDebugWasOn   = false;

  void syncDebugSession();  // Debug включили/выключили -> старт/стоп сессии
  void debugStart();        // новая сессия
  void debugStop();         // итог в файл и в чат, закрыть файлы
  void debugCloseFiles();
  bool debugOpenFile();
  bool debugOpenSeen();
  static bool openAppendLog(std::ofstream &f, const std::string &path,
                            const char *legend, const std::string &session);
  void debugFlush(const std::vector<DebugRow> &rows); // Dump Seen: дописать
  void debugWriteRow(const BlockPos &pos, int oreId, const char *verdict,
                     const std::string &got, const char *note);
  void debugWriteVerdict(const PendingCheck &pc, DropVerdict v);
  void debugWriteSkip(const BlockPos &pos, const PendingCheck &first);
  static const char *oreTypeName(int blockId);
  static const char *verdictName(DropVerdict v);
  static std::string clockString();

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
  // false — блок уже воздух / нечем ломать: пакеты не отправлены.
  bool mineBlockAtPos(const BlockPos &pos, Actor *player);

  // usedSlots (необязательно) — сколько из 36 слотов занято.
  InvSnapshot getInventorySnapshot(int *usedSlots = nullptr);

  // Drop verification
  static std::string toLowerAscii(std::string s);
  static DropList inventoryGains(const InvSnapshot &before,
                                 const InvSnapshot &after);
  static void mergeGains(DropList &acc, const DropList &now);
  static bool hasGainOf(const DropList &gains, const char *what);
  static DropVerdict judgeDrop(const DropList &gains, int oreId);
  static std::string formatGains(const DropList &gains);

  void startCheck(const BlockPos &pos, int seenId, bool decides,
                  InvSnapshot before);
  bool pollCheck();  // true — вердикт вынесен (или проверять было нечего)
  void finishCheck(DropVerdict v, uint64_t now);
  void rejectCluster(const PendingCheck &pc, DropVerdict v); // кластер — фейк
  void abortCheck(); // прервать ожидание (выключение, смена мира)
  void recordCheck(const PendingCheck &pc, DropVerdict v, uint64_t now);

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