#pragma once

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <array>
#include <atomic>
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

  // === Auto Relocate ===
  // То, что раньше делалось руками: всё вокруг выкопано -> 2-3 раза
  // ".tp 10000 ~ 10000" с паузой ~0.6 с -> копать дальше. Теперь само:
  //   - копать нечего (всё, что ближе 60 блоков, выкопано или дальше
  //     60 — не достать) Idle Time секунд с последнего блока/дропа;
  //   - или в инвентарь перестало что-либо падать (3 проверки подряд без
  //     дропа) — сразу, без ожидания.
  // Переезд: Hops прыжков по 300 блоков на +X +Z с паузой Hop Delay
  // (упёрлись в стену — следующий переезд повернёт на 90°), той же цепочкой
  // телепорт-пакетов, что и .tp; потом ждём чанки и копаем на новом месте.
  // Сторож координат (ниже) главнее: сервер кинул игрока — переезд
  // отменяется, сначала пауза и /rtp, и только потом снова переезд.
  BoolSetting mAutoRelocate = BoolSetting(
      "Auto Relocate",
      "Nothing left to mine (or nothing drops) -> hop to a new spot and keep "
      "mining",
      false);
  NumberSetting mIdleTime = NumberSetting(
      "Idle Time",
      "Seconds since the last mined block before hopping (0 = right away)",
      2.f, 0.f, 10.f, 0.5f);
  // «КД» переезда: сколько раз телепортнуться и через сколько ждать между
  // прыжками (сервер за это время догружает чанки).
  NumberSetting mHopCount = NumberSetting(
      "Hops", "Teleports per relocation (like .tp 10000 ~ 10000 N times)",
      3.f, 1.f, 10.f, 1.f);
  NumberSetting mHopDelay = NumberSetting(
      "Hop Delay", "Ms between teleports (chunks load meanwhile)", 600.f,
      100.f, 3000.f, 50.f);
  // Всё, что принесла копка, кроме алмазов (и угля/изумрудов, если их
  // копаем), выкидывается. Выкидывается в момент, когда сервер видит нас у
  // руды (до телепорта обратно), — вещи остаются там, автоподбор не
  // подбирает их снова.
  BoolSetting mAutoDrop = BoolSetting(
      "Auto Drop",
      "Throw out everything mining brings except diamonds (andesite, "
      "cobblestone, ...)",
      false);

  // === Scan settings ===
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
  // Кластеры: руда ищется жилами, и жила неправдоподобного размера (анти-xray
  // фейк) отбрасывается. Выключено — копается каждая найденная руда, без
  // фильтра размера (BFS остаётся: по нему выбирается, что копать рядом).
  BoolSetting mClusters = BoolSetting(
      "Clusters",
      "Only mine ore clusters of a plausible size (off = every found ore)",
      true);

  // === Ore toggles ===
  // Железо и золото — только обычная руда (iron_ore / gold_ore), глубинные
  // варианты не копаются.
  BoolSetting mCoal = BoolSetting("Coal", "Mine coal ore", true);
  BoolSetting mIron = BoolSetting("Iron", "Mine iron ore (normal only)", false);
  BoolSetting mGold = BoolSetting("Gold", "Mine gold ore (normal only)", false);
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
            "Mine coal / iron / gold / diamond / emerald ores, auto-skip fakes",
            ModuleCategory::Player, 0, false) {
    addSettings(&mSpeed, &mStepDistance, &mSilentAccept, &mSwing,
                &mVeinMiner, &mFollow, &mAutoRelocate, &mIdleTime, &mHopCount,
                &mHopDelay, &mAutoDrop, &mOnlyExposedOres, &mPatternFilter,
                &mClusters, &mCoal, &mIron, &mGold, &mDiamond, &mEmerald,
                &mDebug, &mDumpSeen, &mVerifyTimeout, &mDrawBlocks, &mDrawPath,
                &mDrawTarget, &mDrawCluster);

    VISIBILITY_CONDITION(mDumpSeen, mDebug.mValue);
    VISIBILITY_CONDITION(mIdleTime, mAutoRelocate.mValue);
    VISIBILITY_CONDITION(mHopCount, mAutoRelocate.mValue);
    VISIBILITY_CONDITION(mHopDelay, mAutoRelocate.mValue);

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
  static constexpr int IRON_ORE_ID = 15;
  static constexpr int GOLD_ORE_ID = 14;
  static constexpr int DIAMOND_ORE_ID = 56;
  static constexpr int DEEPSLATE_DIAMOND_ORE_ID = 660;
  static constexpr int EMERALD_ORE_ID = 129;
  static constexpr int DEEPSLATE_EMERALD_ORE_ID = 662;

  enum class OreType { None, Coal, Iron, Gold, Diamond, Emerald };

  static OreType getOreType(int id) {
    if (id == COAL_ORE_ID || id == DEEPSLATE_COAL_ORE_ID)
      return OreType::Coal;
    if (id == IRON_ORE_ID)
      return OreType::Iron;
    if (id == GOLD_ORE_ID)
      return OreType::Gold;
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
    case OreType::Iron:
      return "iron";
    case OreType::Gold:
      return "gold";
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
    case OreType::Iron:
      lo = 1;
      hi = 16;
      break;
    case OreType::Gold:
      lo = 1;
      hi = 9;
      break;
    case OreType::Diamond:
    case OreType::Emerald:
      lo = 2;
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
    case OreType::Iron:
      return ImColor(0.85f, 0.72f, 0.62f, 1.f);
    case OreType::Gold:
      return ImColor(1.f, 0.84f, 0.2f, 1.f);
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

  // Руды, найденные сканером в каждом подчанке при последнем чтении: при
  // следующем чтении того же подчанка пропавшие из него удаляются из
  // mFoundBlocks. Раньше для этого на КАЖДЫЙ блок воздуха шёл поиск в
  // хэш-таблице (тысячи на подчанк) — это и ело FPS, особенно с углём.
  std::unordered_map<uint64_t, std::vector<BlockPos>> mSubOres;
  static uint64_t subKeyOf(int cx, int cz, int subY) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(cx) & 0xFFFFFFu) << 40) |
           (static_cast<uint64_t>(static_cast<uint32_t>(cz) & 0xFFFFFFu) << 16) |
           static_cast<uint64_t>(static_cast<uint16_t>(subY));
  }
  bool isEnabledOre(int blockId) const;
  void noteSubOre(const BlockPos &pos); // руда пришла событием, а не сканером
  void invalidateNearest() { mNearestValid = false; }
  // Ближайший кластер ищем не каждый тик: с углём в списке бывают тысячи руд,
  // и полный обход на каждом кадре — это лишние просадки FPS.
  std::vector<BlockPos> mNearestCache;
  bool mNearestValid = false;
  uint64_t mNearestAt = 0;
  static constexpr uint64_t kNearestCacheMs = 250;

  // Скрыто: копаем руду не дальше kMineReach блоков (от глаз до центра
  // блока) — дальше цепочка телепортов чаще не доходит, вместо дропа nodrop.
  // Сканер читает чуть шире (чанки, чья ближняя точка ближе kScanReach по
  // горизонтали), чтобы у Pattern Filter были соседние чанки, и только
  // подчанки в пределах y ± kMineReach — остальные копать всё равно нельзя.
  static constexpr float kMineReach = 60.f;
  static constexpr float kScanReach = kMineReach + 16.f;
  static constexpr int kScanRings = 5;    // ceil(76 / 16): дальше колец нет
  static constexpr int kScanPerTick = 16; // прочитанных подчанков за тик

  ChunkPos mSearchCenter;
  ChunkPos mCurrentChunkPos;
  int mSubChunkIndex = 0;
  int mDirectionIndex = 0;
  int mSteps = 1;
  int mStepsCount = 0;
  // Первый полный проход после включения/переезда закончен — только после
  // него Auto Relocate может решить, что копать нечего. Проход считается,
  // если все чанки в пределах kMineReach были у клиента (иначе руда могла
  // быть в непришедшем чанке) — или если скан на этом месте идёт уже
  // kScanGiveUpMs (какие-то чанки так и не приходят).
  static constexpr uint64_t kScanGiveUpMs = 5000;
  bool mScanComplete = false;
  int mPassMissing = 0;        // чанков в пределах досягаемости не было
  uint64_t mScanStartedAt = 0; // resetScan: скан на этом месте начат
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
  float mLastMineDist = -1.f; // дистанция до блока в последнем mineBlockAtPos

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
    float dist = -1.f;        // от игрока до блока в момент удара (-1 — нет)
    uint64_t firstGainAt = 0; // когда в инвентаре впервые что-то прибавилось
    bool invFull = false;     // все 36 слотов заняты — дропу некуда лечь
    InvSnapshot before;       // инвентарь прямо перед ударом
    InvSnapshot dropped;      // Auto Drop выкинул в этом же ударе (имя -> шт.)
    bool dropApplied = false; // ...и сервер это уже применил
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

  // =====================================================
  // Auto Relocate — вокруг всё выкопано, прыгаем на новое место
  // =====================================================
  enum class HopPhase {
    None,    // обычная работа
    Hopping, // прыгаем: Hops раз по kHopDistance с паузой Hop Delay
    Settling // прыгнули — ждём, пока клиент получит чанки вокруг
  };
  static constexpr int kMaxEmptyRelocations = 10; // подряд без дропа -> стоп
  static constexpr uint64_t kSettleMaxMs = 5000;  // дольше чанки не ждём
  // Скрыто: как руками ".tp 10000 ~ 10000" три раза с паузой ~0.6 с.
  static constexpr float kHopDistance = 300.f;
  int hopCount() const;
  uint64_t hopDelayMs() const;

  HopPhase mHopPhase = HopPhase::None;
  // Пока переезжаем, Silent Accept не глотает откаты сервера: он пускает
  // только до края прогруженных чанков и возвращает туда — клиент должен
  // встать туда же, иначе позиции клиента и сервера разъедутся. Читается из
  // onPacketInEvent, поэтому atomic.
  std::atomic<bool> mRelocating{false};
  std::atomic<int> mHopCorrections{0}; // сколько раз сервер поправил позицию
  std::string mRelocateNow; // не пусто — переехать в этом тике (в инвентарь
                            // ничего не падает), строка — причина
  std::string mHopReason;   // почему переезжаем (чат/дамп)
  int mHopsDone = 0;
  int mHopTurn = 0;            // поворотов на 90° (прыжки упёрлись в стену)
  int mEmptyRelocations = 0;   // переездов подряд, после которых не было
                               // ни одного дропа (real/fake)
  int mRelocations = 0;        // за включение модуля
  int mDropsSinceHop = 0;      // дропов (real/fake) с прошлого переезда
  uint64_t mHopNextAt = 0;     // когда можно следующий прыжок / замер
  uint64_t mHopStartedAt = 0;  // начало переезда
  uint64_t mSettleSince = 0;   // начало ожидания чанков
  uint64_t mLastBusyAt = 0;    // когда модулю последний раз было что делать
  glm::vec3 mHopOrigin{};      // где были до переезда
  glm::vec3 mHopFrom{};        // откуда последний прыжок
  glm::vec3 mHopDir{};         // направление (одно на весь переезд)
  std::vector<int> mHopMoves;  // реальный сдвиг каждого прыжка, блоков
  int mOutOfReach = 0;         // руд дальше kMineReach (для статуса)

  glm::vec3 hopDirection() const;
  void startRelocation(Actor *player, std::string why, uint64_t now);
  void tickRelocation(Actor *player, uint64_t now);
  void finishRelocation(Actor *player, bool chunksReady, uint64_t now);
  void cancelRelocation();
  bool chunksReadyAround(const glm::vec3 &pos, int radius);
  void maybeAutoRelocate(Actor *player, uint64_t now);

  // =====================================================
  // Auto Drop — мусор после копки на пол (у руды, а не под ноги)
  // =====================================================
  static constexpr int kDropStacksPerBlock = 4; // за один сломанный блок
  static constexpr int kDropSettleTicks = 6;    // после выброса: ответ сервера
                                                // успевает прийти до снимка
                                                // «до» следующего удара
  std::unordered_set<std::string> mJunkNames; // что копка приносила, кроме
                                              // нужного (имена предметов)
  InvSnapshot mInvPrev;                       // инвентарь прошлого тика
  bool mInvPrevValid = false;
  std::array<uint64_t, 36> mDropAskedAt{};    // слот уже выкидывали (ждём
                                              // ответ сервера)
  InvSnapshot mJustDropped; // выкинуто в последнем mineBlockAtPos: уходит в
                            // проверку дропа (PendingCheck::dropped)
  uint64_t mLastMinedAt = 0;
  bool mDropSettle = false; // в последнем ударе что-то выкинули
  int mJunkDropped = 0;     // стаков выкинуто за включение (для статуса)
  bool isKeptItem(const std::string &lowerName) const;
  void learnJunk(uint64_t now);            // раз в тик: что прибавилось
  void dropJunkHere(class Inventory *container); // из mineBlockAtPos, у руды

  // Render (protected by mMutex)
  std::vector<glm::vec3> mPacketPositions;
  uint64_t mLastPathTime = 0;
  std::recursive_mutex mMutex;

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

  // Дистанция -> вердикт по каждому проверенному блоку сессии Debug: из них в
  // итоге таблица real/fake/nodrop по дистанции и порог, дальше которого
  // nodrop заметно чаще (чтобы видеть, хорош ли скрытый лимит 60 блоков).
  // nodrop при полном инвентаре сюда не идёт — дистанция ни при чём.
  struct DistSample {
    float d = 0.f;
    DropVerdict v = DropVerdict::Unknown;
  };
  struct DistAdvice {
    bool ok = false; // nodrop заметно чаще дальше какой-то дистанции
    int limit = 0;   // сама эта дистанция
    int nearNo = 0, nearAll = 0, farNo = 0, farAll = 0;
    int noDrops = 0;           // nodrop всего (без полного инвентаря)
    float farthestReal = -1.f; // самый дальний real
  };
  std::vector<DistSample> mDbgDist;
  static DistAdvice adviseMaxDistance(std::vector<DistSample> samples);
  void debugWriteDistanceSummary();
  void debugNoteSettings(); // скрытые лимиты и т.п. — строкой-комментарием

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
                     float dist, const std::string &got, const char *note);
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

  // =====================================================
  // Events
  // =====================================================
  void onBaseTickEvent(class BaseTickEvent &event);
  void onPacketOutEvent(class PacketOutEvent &event);
  void onPacketInEvent(class PacketInEvent &event);
  void onRenderEvent(class RenderEvent &event);
  void onBlockChangedEvent(class BlockChangedEvent &event);

  std::string getSettingDisplay() override;
};