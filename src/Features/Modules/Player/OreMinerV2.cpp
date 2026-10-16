#include "OreMinerV2.hpp"

#include <Features/Command/Commands/TeleportCommand.hpp>
#include <Features/FeatureManager.hpp>

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/CommandRequestPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>

#include <Utils/FileUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/CommandUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <map>
#include <sstream>

// =============================================================
// СКРЫТОЕ — без настроек, всегда включено (в .hpp ничего нет)
// =============================================================
//
//  1. "/heal" раз в 5 минут, пока модуль включён (первый — через 5 минут).
//  2. Сторож координат. Копали на 8931 13 9313 — и вдруг стоим на 383 67 312
//     (сервер кинул на спавн, смерть, смена мира...). Тогда:
//       - пауза: ничего не копаем, проверку текущего блока бросаем;
//       - через 1.5 с "/rtp" и ждём, пока он перекинет (до 20 с, 3 попытки);
//       - ".tp ~ 10 ~": вниз на y=10, x и z те же — так же, как .tp (цепочка
//         телепорт-пакетов + клиент туда же);
//       - скан заново — и копаем дальше.
//     Скачок — сдвиг больше 100 блоков за тик (столько не пройти и не
//     пролететь). Свои прыжки (переезд, спуск) и ручной .tp — не скачок. Если
//     /rtp только что набрали руками, сразу спуск: второй /rtp не шлём.
namespace {
constexpr uint64_t kHealEveryMs = 5ull * 60ull * 1000ull;
constexpr float kJumpDist = 100.f;      // за один тик столько не пройти
constexpr uint64_t kJumpPauseMs = 1500; // скачок -> пауза -> /rtp
constexpr uint64_t kRtpWaitMs = 20000;  // столько ждём телепорт от /rtp
constexpr int kRtpTries = 3;
constexpr uint64_t kRtpRecentMs = 60000; // /rtp ушёл недавно -> скачок от него
constexpr float kDownY = 10.f;          // ".tp ~ 10 ~"
constexpr uint64_t kDownMinMs = 800;    // после rtp: дать прийти чанкам...
constexpr uint64_t kDownMaxMs = 6000;   // ...но не дольше
constexpr uint64_t kResumeMs = 500;     // после спуска: откат успевает прийти
constexpr uint64_t kQuietMs = 2000;     // после своих прыжков скачки не ловим
constexpr uint64_t kManualTpMs = 3000;  // ручной .tp — не скачок

enum class Recover { None, Pause, WaitRtp, Down, Resume };

struct HiddenState {
  uint64_t nextHealAt = 0;
  bool havePos = false;
  glm::vec3 lastPos{};       // позиция в конце прошлого тика
  uint64_t quietUntil = 0;   // свои телепорты: сдвиг скачком не считается
  std::atomic<uint64_t> rtpSentAt{0}; // когда ушёл "/rtp" (наш или руками)
  Recover phase = Recover::None;
  uint64_t phaseAt = 0;      // когда следующий шаг (или конец ожидания)
  uint64_t phaseStart = 0;
  glm::vec3 jumpFrom{}, jumpTo{};
  glm::vec3 rtpFrom{};       // где стояли, когда отправили /rtp
  int rtpTries = 0;
};
HiddenState sHidden;

uint64_t since(uint64_t now, uint64_t t) { return now > t ? now - t : 0; }

std::string fmtPos(const glm::vec3 &p) {
  return fmt::format("{} {} {}", static_cast<int>(std::floor(p.x)),
                     static_cast<int>(std::floor(p.y)),
                     static_cast<int>(std::floor(p.z)));
}

// Чанк, в котором стоит точка (floor: ChunkPos(vec3) режет к нулю и на
// минусовых координатах ошибается на чанк).
ChunkPos chunkOf(const glm::vec3 &p) {
  return ChunkPos(static_cast<int>(std::floor(p.x)) >> 4,
                  static_cast<int>(std::floor(p.z)) >> 4);
}

void hiddenReset(uint64_t now) {
  auto &h = sHidden;
  h.nextHealAt = now + kHealEveryMs;
  h.havePos = false;
  h.quietUntil = 0;
  h.phase = Recover::None;
  h.rtpTries = 0;
}

// Свой телепорт (переезд закончен и т.п.): точка отсчёта — новая позиция,
// поздний откат сервера скачком не считается.
void hiddenQuiet(uint64_t until) {
  sHidden.quietUntil = (std::max)(sHidden.quietUntil, until);
  sHidden.havePos = false;
}

void hiddenRtpSent(uint64_t now) { sHidden.rtpSentAt.store(now); }

bool hiddenRecovering() { return sHidden.phase != Recover::None; }

std::string hiddenStatus(uint64_t now) {
  const auto &h = sHidden;
  switch (h.phase) {
  case Recover::Pause:
    return fmt::format("Position jumped! /rtp in {:.1f}s",
                       static_cast<double>(since(h.phaseAt, now)) / 1000.0);
  case Recover::WaitRtp:
    return fmt::format("Waiting for /rtp (try {}/{})", h.rtpTries, kRtpTries);
  case Recover::Down:
    return "RTP done, going down to y=10";
  case Recover::Resume:
    return "At y=10, resuming";
  default:
    return {};
  }
}

void hiddenLog(OreMinerV2 &m, const std::string &line) {
  spdlog::info("[OreMinerV2] {}", line);
  if (m.mDebug.mValue && m.debugOpenFile()) {
    m.mDbgFile << "# " << OreMinerV2::clockString() << ' ' << line << '\n';
    m.mDbgFile.flush();
  }
}

void sendRtp(OreMinerV2 &m, const glm::vec3 &pos, uint64_t now) {
  auto &h = sHidden;
  h.rtpTries++;
  h.rtpFrom = pos;
  h.phase = Recover::WaitRtp;
  h.phaseStart = now;
  h.phaseAt = now + kRtpWaitMs;
  CommandUtils::executeCommand("/rtp");
  hiddenLog(m, fmt::format("/rtp (попытка {}/{}) с {}", h.rtpTries, kRtpTries,
                           fmtPos(pos)));
}

void startRecovery(OreMinerV2 &m, const glm::vec3 &from, const glm::vec3 &to,
                   uint64_t now) {
  auto &h = sHidden;
  m.abortCheck();
  m.cancelRelocation();
  m.mRelocateNow.clear();
  m.mState = OreMinerV2::MineState::Idle;
  m.mCurrentCluster.clear();
  m.mCurrentClusterIdx = 0;
  m.mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  m.mMinedPositions.clear(); // то место далеко, его блоки больше не нужны
  m.mPauseUntil = 0;
  m.mNoDropStreak = m.mStreakLevel = 0;
  m.mSinceLastDrop.clear();
  m.resetScan();
  m.mRelocating.store(true); // откаты сервера не глотаем, пока чиним

  h.jumpFrom = from;
  h.jumpTo = to;
  h.rtpTries = 0;
  h.phaseStart = now;
  // /rtp только что набрали (руками или мы) — это его телепорт: сразу вниз.
  const bool byRtp = since(now, h.rtpSentAt.load()) < kRtpRecentMs;
  h.rtpSentAt.store(0);
  const int dist = static_cast<int>(glm::distance(from, to));
  if (byRtp) {
    h.phase = Recover::Down;
    h.phaseAt = now + kDownMinMs;
    hiddenLog(m, fmt::format("rtp: {} -> {} ({} бл.), вниз на y=10",
                             fmtPos(from), fmtPos(to), dist));
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §bRTP§7: {} -> {}. Спускаюсь на y=10 и копаю дальше",
        fmtPos(from), fmtPos(to)));
  } else {
    h.phase = Recover::Pause;
    h.phaseAt = now + kJumpPauseMs;
    hiddenLog(m, fmt::format("скачок позиции: {} -> {} ({} бл.), пауза, /rtp",
                             fmtPos(from), fmtPos(to), dist));
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §eПозицию резко сменило§7: {} -> {}. Пауза, потом "
        "§f/rtp§7 и вниз на y=10",
        fmtPos(from), fmtPos(to)));
  }
}

void finishRecovery(OreMinerV2 &m, const glm::vec3 &pos, uint64_t now,
                    bool ok) {
  auto &h = sHidden;
  h.phase = Recover::None;
  m.mRelocating.store(false);
  m.resetScan();       // центр скана — новое место
  m.mLastBusyAt = now; // Idle Time — заново
  hiddenQuiet(now + kQuietMs);
  if (ok) {
    hiddenLog(m, fmt::format("на месте: {}, копаю", fmtPos(pos)));
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §aНа месте§7 ({}) — копаю дальше", fmtPos(pos)));
  } else {
    hiddenLog(m, fmt::format("/rtp не перекинул ({} попытки), копаю на {}",
                             h.rtpTries, fmtPos(pos)));
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §c/rtp не перекинул§7 ({} попытки) — копаю здесь",
        h.rtpTries));
  }
}

// Раз в тик, до всего остального. true — в этом тике модуль не сканирует и
// не копает (идёт восстановление после скачка).
bool hiddenTick(OreMinerV2 &m, Actor *player, uint64_t now) {
  auto &h = sHidden;
  if (now >= h.nextHealAt) {
    h.nextHealAt = now + kHealEveryMs;
    CommandUtils::executeCommand("/heal");
  }

  const glm::vec3 pos = *player->getPos();
  if (h.phase == Recover::None) {
    const bool ownMove =
        m.mRelocating.load() || now < h.quietUntil ||
        since(now, TeleportCommand::sLastUse.load()) < kManualTpMs;
    if (h.havePos && !ownMove && glm::distance(pos, h.lastPos) > kJumpDist)
      startRecovery(m, h.lastPos, pos, now);
    h.lastPos = pos;
    h.havePos = true;
    return h.phase != Recover::None;
  }

  h.lastPos = pos;
  h.havePos = true;
  m.mRelocating.store(true); // смена измерения и т.п. могли сбросить
  switch (h.phase) {
  case Recover::Pause:
    if (now >= h.phaseAt)
      sendRtp(m, pos, now);
    return true;

  case Recover::WaitRtp:
    if (glm::distance(pos, h.rtpFrom) > kJumpDist) {
      h.rtpSentAt.store(0);
      h.phase = Recover::Down;
      h.phaseStart = now;
      h.phaseAt = now + kDownMinMs;
      hiddenLog(m, fmt::format("rtp -> {}, вниз на y=10", fmtPos(pos)));
      return true;
    }
    if (now < h.phaseAt)
      return true;
    if (h.rtpTries < kRtpTries) {
      sendRtp(m, pos, now);
      return true;
    }
    finishRecovery(m, pos, now, false);
    return false;

  case Recover::Down: {
    if (now < h.phaseAt)
      return true;
    if (!m.chunksReadyAround(pos, 1) && since(now, h.phaseStart) < kDownMaxMs)
      return true;
    if (pos.y > kDownY + 3.f) {
      // ".tp ~ 10 ~": как TeleportCommand — телепорт-пакеты по Step
      // Distance до точки, потом сам клиент туда же.
      const glm::vec3 to(pos.x, kDownY, pos.z);
      m.straightLineTP(pos, to, true);
      player->setPosition(to);
      if (auto sv = player->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);
      h.lastPos = to;
      hiddenLog(m, fmt::format("вниз: {} -> {}", fmtPos(pos), fmtPos(to)));
    }
    h.phase = Recover::Resume;
    h.phaseAt = now + kResumeMs;
    return true;
  }

  case Recover::Resume:
    if (now < h.phaseAt)
      return true;
    finishRecovery(m, pos, now, true);
    return false;

  default:
    return false;
  }
}
} // namespace

// =============================================================
// ENABLED BLOCK IDS
// =============================================================

std::vector<int> OreMinerV2::getEnabledBlockIds() {
  std::vector<int> ids;
  if (mCoal.mValue) {
    ids.push_back(COAL_ORE_ID);
    ids.push_back(DEEPSLATE_COAL_ORE_ID);
  }
  if (mDiamond.mValue) {
    ids.push_back(DIAMOND_ORE_ID);
    ids.push_back(DEEPSLATE_DIAMOND_ORE_ID);
  }
  if (mEmerald.mValue) {
    ids.push_back(EMERALD_ORE_ID);
    ids.push_back(DEEPSLATE_EMERALD_ORE_ID);
  }
  return ids;
}

// =============================================================
// SCAN — reset
// =============================================================

void OreMinerV2::resetScan() {
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mPatternBlocks.clear();
  mScanComplete = false;
  mPassMissing = 0;
  mScanStartedAt = static_cast<uint64_t>(NOW);
  mScanDirty = false;
  mLastClusterUpdate = 0;
  mStepsCount = 0;
  mSteps = 1;
  mDirectionIndex = 0;
  mSubChunkIndex = 0;
  mOutOfReach = 0;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (player) {
    mSearchCenter = chunkOf(*player->getPos());
    mCurrentChunkPos = mSearchCenter;
  }
}

// =============================================================
// SCAN — spiral advance
// =============================================================

void OreMinerV2::moveToNext() {
  if (!ClientInstance::get()->getLevelRenderer()) {
    resetScan();
    return;
  }

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;
  auto blockSource = ClientInstance::get()->getBlockSource();

  static const std::vector<std::pair<int, int>> dirs = {
      {1, 0}, {0, 1}, {-1, 0}, {0, -1}};

  size_t numSub =
      (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / 16;
  if (numSub - 1 > static_cast<size_t>(mSubChunkIndex)) {
    mSubChunkIndex++;
    return;
  }

  mCurrentChunkPos.x += dirs[mDirectionIndex].first;
  mCurrentChunkPos.y += dirs[mDirectionIndex].second;

  mStepsCount++;
  if (mStepsCount >= mSteps) {
    mStepsCount = 0;
    mDirectionIndex = (mDirectionIndex + 1) % static_cast<int>(dirs.size());
    if (mDirectionIndex % 2 == 0)
      mSteps++;
  }
  mSubChunkIndex = 0;
}

// =============================================================
// SCAN — process one sub-chunk
// =============================================================

bool OreMinerV2::processSub(ChunkPos processChunk, int index) {
  if (!ClientInstance::get()->getLevelRenderer())
    return false;

  auto ci = ClientInstance::get();
  auto player = ci->getLocalPlayer();
  if (!player)
    return false;
  auto blockSource = ci->getBlockSource();

  size_t numSub =
      (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / 16;
  if (index < 0 || static_cast<size_t>(index) >= numSub)
    return false;

  LevelChunk *chunk = blockSource->getChunk(processChunk);
  if (!chunk)
    return false;

  auto subChunk = (*chunk->getSubChunks())[index];
  SubChunkBlockStorage *reader = subChunk.blockReadPtr;
  if (!reader)
    return false;

  markChunkScanned(processChunk);
  std::vector<int> enabled = getEnabledBlockIds();
  bool changed = false;

  for (uint16_t x = 0; x < 16; x++) {
    for (uint16_t z = 0; z < 16; z++) {
      uint16_t yMax = static_cast<uint16_t>(
          (blockSource->getBuildHeight() - blockSource->getBuildDepth()) /
          chunk->getSubChunks()->size());
      for (uint16_t y = 0; y < yMax; y++) {

        uint16_t eid = (x * 0x10 + z) * 0x10 + (y & 0xf);
        const Block *found = reader->getElement(eid);

        BlockPos pos;
        pos.x = processChunk.x * 16 + x;
        pos.z = processChunk.y * 16 + z;
        pos.y = y + subChunk.subchunkIndex * 16;

        if (found->mLegacy->getBlockId() == 0) {
          if (mFoundBlocks.erase(pos)) {
            mFilteredBlocks.erase(pos);
            changed = true;
          }
          continue;
        }

        const bool wanted =
            std::ranges::find(enabled, found->mLegacy->getBlockId()) !=
            enabled.end();
        // В память шаблонов — до чёрного списка и Only Exposed: повтор фигуры
        // по чанкам улика независимо от того, копаем мы этот блок или нет.
        if (wanted)
          rememberOre(pos, false);

        if (mBlacklistedPositions.contains(pos))
          continue;
        if (!wanted)
          continue;
        if (mOnlyExposedOres.mValue && BlockUtils::getExposedFace(pos) == -1)
          continue;

        if (!mFoundBlocks.contains(pos)) {
          int bid = found->mLegacy->getBlockId();
          mFoundBlocks[pos] = {found, AABB(pos, glm::vec3(1.f, 1.f, 1.f)),
                               getOreColor(bid)};
          changed = true;
        }
      }
    }
  }

  if (changed)
    mScanDirty = true;
  return true;
}

void OreMinerV2::tryProcessSub(bool &ok, ChunkPos chunk, int index) {
  TRY_CALL([&]() {
    if (processSub(chunk, index))
      ok = true;
  });
}

// =============================================================
// CLUSTER — BFS (same ore type)
// =============================================================

std::vector<BlockPos>
OreMinerV2::getCluster(const BlockPos &startPos,
                       const std::unordered_map<BlockPos, FoundBlock> &blocks) {
  std::vector<BlockPos> cluster;
  std::unordered_set<BlockPos> visited;
  std::queue<BlockPos> queue;

  auto it = blocks.find(startPos);
  if (it == blocks.end())
    return cluster;

  int startId = it->second.block->mLegacy->getBlockId();

  queue.push(startPos);
  visited.insert(startPos);

  static const BlockPos nbr[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                 {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};

  while (!queue.empty()) {
    BlockPos cur = queue.front();
    queue.pop();
    cluster.push_back(cur);

    for (auto &d : nbr) {
      BlockPos n = {cur.x + d.x, cur.y + d.y, cur.z + d.z};
      if (visited.contains(n))
        continue;
      auto nit = blocks.find(n);
      if (nit == blocks.end())
        continue;
      if (!isSameOreType(startId, nit->second.block->mLegacy->getBlockId()))
        continue;
      visited.insert(n);
      queue.push(n);
    }
  }
  return cluster;
}

// =============================================================
// CLUSTER — rebuild filtered map (ore-specific size ranges)
// =============================================================

void OreMinerV2::updateClusters() {
  mFilteredBlocks.clear();
  mPatternBlocks.clear();

  const bool usePattern = mPatternFilter.mValue;
  if (usePattern)
    prunePatternMemory();

  std::unordered_set<BlockPos> processed;

  for (auto &[pos, fb] : mFoundBlocks) {
    if (processed.contains(pos))
      continue;
    if (mBlacklistedPositions.contains(pos))
      continue;

    int bid = fb.block->mLegacy->getBlockId();
    OreType type = getOreType(bid);
    if (type == OreType::None)
      continue;

    auto cluster = getCluster(pos, mFoundBlocks);
    for (auto &cp : cluster)
      processed.insert(cp);

    int lo, hi;
    getClusterRange(type, lo, hi);
    int sz = static_cast<int>(cluster.size());

    if (sz < lo || sz > hi)
      continue; // wrong size → skip

    bool bl = false;
    for (auto &cp : cluster)
      if (mBlacklistedPositions.contains(cp)) {
        bl = true;
        break;
      }
    if (bl)
      continue;

    // Шаблон анти-xray: блоки, чья фигура повторяется в соседних чанках, не
    // копаем совсем. Остаток кластера (настоящая жила, к которой прилип
    // кусок шаблона) копается как обычно — размер кластера считали выше
    // целиком, как и раньше.
    std::unordered_set<BlockPos> pattern;
    if (usePattern) {
      if (!patternReady(cluster)) {
        // Соседние чанки ещё не прочитаны — сравнить не с чем. Вернёмся к
        // кластеру при следующем пересчёте (не позже конца прохода сканера),
        // даже если новых руд больше не найдётся: раньше такой кластер так и
        // оставался непрочитанным, если он был последним найденным.
        mScanDirty = true;
        continue;
      }
      findPatternBlocks(cluster, pattern);
      for (auto &cp : pattern) {
        mPatternBlocks.insert(cp);
        if (mPatternSkipped.insert(cp).second && mDebug.mValue)
          mDbgPattern++;
      }
    }

    for (auto &cp : cluster) {
      if (pattern.contains(cp))
        continue;
      auto fit = mFoundBlocks.find(cp);
      if (fit != mFoundBlocks.end())
        mFilteredBlocks[cp] = fit->second;
    }
  }
}

// =============================================================
// PATTERN FILTER — фейки анти-xray повторяются от чанка к чанку
// =============================================================
//
// По логу Debug с сервера (4 сессии, 446 проверенных блоков): фейковые алмазы —
// это одна и та же фигура на одном и том же месте внутри чанка (x&15, y, z&15),
// повторённая в соседних чанках (там — вдоль X, у каждой «строки» чанков своя).
// 286 из 301 фейкового кластера нашлись точно такими же ещё в каком-нибудь
// чанке; из 121 настоящего блока на таком месте стоял один. Настоящие жилы
// случайные и так не повторяются — значит, повтор = фейк, и копать его, даже
// ради проверки, незачем.

void OreMinerV2::syncSpotsWorld() {
  const void *world = ClientInstance::get()->getBlockSource();
  if (world != mSpotsWorld) {
    mChunkSpots.clear();
    mSpotsWorld = world;
  }
}

void OreMinerV2::markChunkScanned(const ChunkPos &chunk) {
  mChunkSpots.try_emplace(chunkKeyOf(chunk.x, chunk.y));
}

// onlyScannedChunk: блок пришёл событием (сервер что-то обновил), а не со
// сканера — пишем, только если чанк уже сканировали, иначе «пустой» ключ
// выглядел бы как прочитанный чанк.
void OreMinerV2::rememberOre(const BlockPos &pos, bool onlyScannedChunk) {
  if (onlyScannedChunk) {
    auto it = mChunkSpots.find(chunkKeyOf(pos));
    if (it != mChunkSpots.end())
      it->second.insert(spotOf(pos));
    return;
  }
  mChunkSpots[chunkKeyOf(pos)].insert(spotOf(pos));
}

// Пока первый проход сканирования не закончен, соседние чанки могут быть ещё
// не прочитаны: фейк в них не с чем сравнить, и модуль копнул бы его вслепую.
// Поэтому до конца первого прохода кластер берём, только когда и по строке, и
// по столбцу (±2 чанка) прочитано хотя бы по 2 соседа.
bool OreMinerV2::patternReady(const std::vector<BlockPos> &cluster) const {
  if (mScanComplete)
    return true;
  std::unordered_set<uint64_t> checked;
  for (auto &p : cluster) {
    const int cx = p.x >> 4, cz = p.z >> 4;
    if (!checked.insert(chunkKeyOf(cx, cz)).second)
      continue;
    if (!mChunkSpots.contains(chunkKeyOf(cx, cz)))
      continue; // сам чанк сканер не читал — сравнивать всё равно не с чем
    int row = 0, col = 0;
    for (int d = -2; d <= 2; d++) {
      if (d == 0)
        continue;
      row += mChunkSpots.contains(chunkKeyOf(cx + d, cz)) ? 1 : 0;
      col += mChunkSpots.contains(chunkKeyOf(cx, cz + d)) ? 1 : 0;
    }
    if (row < 2 || col < 2)
      return false;
  }
  return true;
}

// Часть кластера в чанке C — шаблон, если на одной линии с C (та же строка
// или тот же столбец, до kPatternReach чанков) есть хотя бы kPatternMinChunks
// чанков, у каждого из которых руда стоит минимум на kPatternMinSpots тех же
// местах. Шаблонным считается блок, чьё место повторяется минимум в
// kPatternMinChunks из этих чанков. Требование «два соседа на ОДНОЙ линии»
// отсекает случайные совпадения с фигурой из другой строки чанков.
void OreMinerV2::findPatternBlocks(const std::vector<BlockPos> &cluster,
                                   std::unordered_set<BlockPos> &out) const {
  std::unordered_map<uint64_t, std::vector<BlockPos>> parts;
  for (auto &p : cluster)
    parts[chunkKeyOf(p)].push_back(p);

  for (auto &[key, ps] : parts) {
    if (static_cast<int>(ps.size()) < kPatternMinSpots)
      continue;
    const int cx = ps[0].x >> 4, cz = ps[0].z >> 4;
    for (int axis = 0; axis < 2; axis++) {
      const std::unordered_set<uint32_t> *hits[2 * kPatternReach] = {};
      int nh = 0;
      for (int d = -kPatternReach; d <= kPatternReach; d++) {
        if (d == 0)
          continue;
        auto it = mChunkSpots.find(axis == 0 ? chunkKeyOf(cx + d, cz)
                                             : chunkKeyOf(cx, cz + d));
        if (it == mChunkSpots.end() || it->second.empty())
          continue;
        int common = 0;
        for (auto &p : ps)
          common += it->second.contains(spotOf(p)) ? 1 : 0;
        if (common >= kPatternMinSpots)
          hits[nh++] = &it->second;
      }
      if (nh < kPatternMinChunks)
        continue;
      for (auto &p : ps) {
        const uint32_t s = spotOf(p);
        int n = 0;
        for (int i = 0; i < nh; i++)
          n += hits[i]->contains(s) ? 1 : 0;
        if (n >= kPatternMinChunks)
          out.insert(p);
      }
    }
  }
}

// Память о чанках растёт, пока летаешь; далёкие чанки для сравнения не нужны.
void OreMinerV2::prunePatternMemory() {
  if (mChunkSpots.size() <= 8192)
    return;
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;
  const BlockPos pp = *player->getPos();
  const int pcx = pp.x >> 4, pcz = pp.z >> 4;
  std::erase_if(mChunkSpots, [&](const auto &kv) {
    const int cx = static_cast<int32_t>(static_cast<uint32_t>(kv.first >> 32));
    const int cz = static_cast<int32_t>(static_cast<uint32_t>(kv.first));
    return std::abs(cx - pcx) > 64 || std::abs(cz - pcz) > 64;
  });
}

// =============================================================
// CLUSTER — helpers
// =============================================================

void OreMinerV2::removeBlockIncremental(const BlockPos &pos) {
  mFoundBlocks.erase(pos);
  mFilteredBlocks.erase(pos);
}

void OreMinerV2::blacklistPositions(const std::vector<BlockPos> &positions) {
  for (auto &p : positions) {
    mBlacklistedPositions.insert(p);
    mFoundBlocks.erase(p);
    mFilteredBlocks.erase(p);
  }
}

std::vector<BlockPos>
OreMinerV2::findNearestCluster(glm::vec3 playerPos,
                               const std::unordered_set<BlockPos> &exclude) {
  BlockPos nearestPos;
  float nearestDist = FLT_MAX;
  bool found = false;
  // Кластер берём, только если его ближний блок не дальше kMineReach (60).
  // Остальные руды не трогаем — они ждут, пока игрок окажется ближе (Follow),
  // в статусе видно, сколько их «не достать». Ближе нечего — Auto Relocate.
  mOutOfReach = 0;

  for (auto &[pos, fb] : mFilteredBlocks) {
    if (exclude.contains(pos))
      continue;
    if (mBlacklistedPositions.contains(pos))
      continue;
    float d = glm::distance(playerPos, glm::vec3(pos) + glm::vec3(0.5f));
    if (d > kMineReach) {
      mOutOfReach++;
      continue;
    }
    if (d < nearestDist) {
      nearestDist = d;
      nearestPos = pos;
      found = true;
    }
  }
  if (!found)
    return {};

  // build BFS source without excluded
  std::unordered_map<BlockPos, FoundBlock> valid;
  for (auto &[pos, fb] : mFilteredBlocks)
    if (!exclude.contains(pos) && !mBlacklistedPositions.contains(pos))
      valid[pos] = fb;

  auto cluster = getCluster(nearestPos, valid);

  std::sort(cluster.begin(), cluster.end(),
            [&](const BlockPos &a, const BlockPos &b) {
              return glm::distance(playerPos, glm::vec3(a) + glm::vec3(0.5f)) <
                     glm::distance(playerPos, glm::vec3(b) + glm::vec3(0.5f));
            });

  return cluster;
}

// =============================================================
// ENABLE / DISABLE
// =============================================================

// =============================================================
// DEBUG DUMP
// =============================================================
const char *OreMinerV2::oreTypeName(int blockId) {
  switch (getOreType(blockId)) {
  case OreType::Coal:
    return "coal";
  case OreType::Diamond:
    return "diamond";
  case OreType::Emerald:
    return "emerald";
  default:
    return "other";
  }
}

const char *OreMinerV2::verdictName(DropVerdict v) {
  switch (v) {
  case DropVerdict::Real:
    return "real";
  case DropVerdict::Fake:
    return "fake";
  case DropVerdict::NoDrop:
    return "nodrop";
  default:
    return "unknown";
  }
}

// Локальное время "ЧЧ:ММ:СС" — для глаз; колонка ms — для скриптов.
std::string OreMinerV2::clockString() {
  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char buf[16] = {};
  std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmBuf);
  return buf;
}

// Сессию открывает/закрывает и тик, и рендер: кто первым увидел, что Debug
// переключили, тот и делает. Оба вызывают это под mMutex.
void OreMinerV2::syncDebugSession() {
  const bool want = mDebug.mValue;
  if (want && !mDebugWasOn)
    debugStart();
  else if (!want && mDebugWasOn)
    debugStop();
  mDebugWasOn = want;
}

// Новая сессия. Файл один и тот же (Dumps\OreMinerV2.txt), сессия — это
// заголовок с датой внутри него; сам заголовок пишется при первой строке.
void OreMinerV2::debugStart() {
  debugCloseFiles();

  mDebugDir = FileUtils::getJuzdexDir() + "Dumps\\";
  try {
    FileUtils::createDirectory(mDebugDir);
  } catch (const std::exception &e) {
    spdlog::error("[OreMinerV2][Debug] cannot create {}: {}", mDebugDir,
                  e.what());
  }

  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char stamp[32] = {};
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmBuf);

  mDebugSession = stamp;
  mDbgFileFailed = false;
  mDbgSeenFailed = false;
  mDebugWritten.clear();
  mDebugRows = 0;
  mDbgReal = mDbgFake = mDbgNoDrop = mDbgUnknown = mDbgSkip = 0;
  mDbgPattern = 0;
  mDbgDist.clear();

  spdlog::info("[OreMinerV2][Debug] session {} -> {}OreMinerV2.txt",
               mDebugSession, mDebugDir);
  ChatUtils::displayClientMessage(
      "§7[OreMinerV2] §aDebug ON§7: каждый сломанный блок проверяется по "
      "дропу -> §fDumps\\OreMinerV2.txt");
}

void OreMinerV2::debugCloseFiles() {
  for (std::ofstream *f : {&mDbgFile, &mDbgSeenFile}) {
    if (f->is_open()) {
      f->flush();
      f->close();
    }
  }
}

void OreMinerV2::debugStop() {
  const bool wrote = mDbgFile.is_open();
  const bool wroteSeen = mDbgSeenFile.is_open();
  const DistAdvice advice = adviseMaxDistance(mDbgDist);
  if (wrote) {
    mDbgFile << "# итог: " << mDbgReal << " real, " << mDbgFake << " fake, "
             << mDbgNoDrop << " nodrop";
    if (mDbgUnknown > 0)
      mDbgFile << ", " << mDbgUnknown << " unknown";
    if (mDbgSkip > 0)
      mDbgFile << ", " << mDbgSkip << " skip";
    if (mDbgPattern > 0)
      mDbgFile << ", " << mDbgPattern << " pattern";
    mDbgFile << '\n';
    debugWriteDistanceSummary();
  }
  if (wroteSeen)
    spdlog::info("[OreMinerV2][Debug] {} seen blocks -> {}OreMinerV2_seen.txt",
                 mDebugRows, mDebugDir);
  debugCloseFiles();

  if (wrote) {
    spdlog::info("[OreMinerV2][Debug] {} real, {} fake, {} nodrop, {} unknown, "
                 "{} skipped -> {}OreMinerV2.txt",
                 mDbgReal, mDbgFake, mDbgNoDrop, mDbgUnknown, mDbgSkip,
                 mDebugDir);
    std::string extra =
        mDbgUnknown > 0 ? fmt::format(", {} unknown", mDbgUnknown) : "";
    if (mDbgPattern > 0)
      extra += fmt::format(", §b{} по шаблону§7", mDbgPattern);
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §fDebug сохранён: §a{} real§7, §c{} fake§7, §e{} "
        "nodrop§7{} -> §fDumps\\OreMinerV2.txt",
        mDbgReal, mDbgFake, mDbgNoDrop, extra));
    if (advice.ok)
      ChatUtils::displayClientMessage(fmt::format(
          "§7[OreMinerV2] §fДистанция: дальше §e{}§f бл. nodrop §e{} из {}§f, "
          "ближе — {} из {} (копаю до {:.0f})",
          advice.limit, advice.farNo, advice.farAll, advice.nearNo,
          advice.nearAll, kMineReach));
  } else if (wroteSeen) {
    ChatUtils::displayClientMessage(
        "§7[OreMinerV2] §fDebug OFF: ни один блок не проверен (Dump Seen -> "
        "§fDumps\\OreMinerV2_seen.txt§f)");
  } else if (mDbgPattern > 0) {
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §fDebug OFF: ни один блок не проверен (§b{} пропущено "
        "по шаблону§f), в файл ничего не записано",
        mDbgPattern));
  } else {
    ChatUtils::displayClientMessage(
        "§7[OreMinerV2] §fDebug OFF: ни один блок не проверен, в файл ничего "
        "не записано");
  }

  mDebugWritten.clear();
  mDebugRows = 0;
  mDebugDir.clear();
  mDebugSession.clear();
  mDbgDist.clear();
}

// Открыть общий файл на дозапись. Файла нет (или он пустой) — сначала
// легенда, что значат колонки; дальше каждый раз заголовок новой сессии.
bool OreMinerV2::openAppendLog(std::ofstream &f, const std::string &path,
                               const char *legend,
                               const std::string &session) {
  std::error_code ec;
  const auto size = std::filesystem::file_size(path, ec);
  const bool fresh = ec || size == 0;

  f.open(path, std::ios::out | std::ios::app);
  if (!f.is_open())
    return false;

  if (fresh)
    f << legend;
  else
    f << '\n'; // пустая строка между сессиями
  f << "# ===== " << session << " =====\n";
  f.flush();
  return true;
}

bool OreMinerV2::debugOpenFile() {
  if (mDbgFile.is_open())
    return true;
  if (mDebugDir.empty() || mDbgFileFailed)
    return false;

  const std::string path = mDebugDir + "OreMinerV2.txt";
  if (!openAppendLog(
          mDbgFile, path,
          "# OreMinerV2 Debug: x y z | руда | вердикт | d=дистанция | что "
          "упало | время\n"
          "#   real    - упала руда, блок настоящий\n"
          "#   fake    - упало другое (булыжник, сланец...), фейк анти-xray\n"
          "#   nodrop  - не упало ничего (далеко, призрак или полный "
          "инвентарь)\n"
          "#   skip    - не копали: первый блок этого кластера оказался "
          "фейком\n"
          "#   unknown - проверку прервали (выключил модуль, сменил мир)\n"
          "#   pattern - (только в итоге) не копали вовсе: та же фигура на том "
          "же месте в соседних чанках = фейк\n"
          "#   d=      - сколько блоков было от тебя до руды в момент удара; "
          "в итоге сессии - таблица по дистанции\n",
          mDebugSession)) {
    mDbgFileFailed = true;
    spdlog::error("[OreMinerV2][Debug] cannot open {}", path);
    return false;
  }
  debugNoteSettings();
  return true;
}

// Строка-комментарий с тем, от чего зависят вердикты: по дампу видно, при
// каких условиях копали. Пишется при открытии файла.
void OreMinerV2::debugNoteSettings() {
  if (!mDbgFile.is_open())
    return;
  mDbgFile << "# " << clockString() << " копаю до "
           << static_cast<int>(kMineReach) << " бл., Step Distance: "
           << std::fixed << std::setprecision(1) << mStepDistance.mValue
           << ", Follow: " << (mFollow.mValue ? "on" : "off")
           << ", Auto Relocate: ";
  if (mAutoRelocate.mValue)
    mDbgFile << "on (Idle Time " << mIdleTime.mValue << " с, "
             << kHopCount << " x " << static_cast<int>(kHopDistance) << ")";
  else
    mDbgFile << "off";
  mDbgFile << ", Auto Drop: " << (mAutoDrop.mValue ? "on" : "off") << '\n';
  mDbgFile.flush();
}

bool OreMinerV2::debugOpenSeen() {
  if (mDbgSeenFile.is_open())
    return true;
  if (mDebugDir.empty() || mDbgSeenFailed)
    return false;

  const std::string path = mDebugDir + "OreMinerV2_seen.txt";
  if (!openAppendLog(
          mDbgSeenFile, path,
          "# OreMinerV2 Dump Seen: всё, что модуль увидел, БЕЗ проверки "
          "(фейки тоже)\n"
          "# x y z | руда | дистанция | экран x y (\"-\" - был за кадром)\n",
          mDebugSession)) {
    mDbgSeenFailed = true;
    spdlog::error("[OreMinerV2][Debug] cannot open {}", path);
    return false;
  }
  return true;
}

// Dump Seen: всё, что модуль увидел (без проверки, вместе с фейками).
void OreMinerV2::debugFlush(const std::vector<DebugRow> &rows) {
  if (rows.empty())
    return;
  if (!debugOpenSeen())
    return;

  for (const auto &r : rows) {
    mDbgSeenFile << r.pos.x << ' ' << r.pos.y << ' ' << r.pos.z << ' '
                 << oreTypeName(r.blockId) << ' ' << std::fixed
                 << std::setprecision(1) << r.distance;
    if (r.onScreen)
      mDbgSeenFile << ' ' << std::setprecision(0) << r.screenX << ' '
                   << r.screenY;
    else
      mDbgSeenFile << " -";
    mDbgSeenFile << '\n';
  }
  mDbgSeenFile.flush();

  mDebugRows += static_cast<int>(rows.size());
}

// Одна строка в OreMinerV2.txt:
//   "x y z руда вердикт d=дистанция что_упало время [пометка]".
// Первые три числа — координаты, так что файл можно и читать глазами, и
// разбирать скриптом/фильтровать по слову (real/fake). d=- — блок не копали
// (skip), дистанции нет.
void OreMinerV2::debugWriteRow(const BlockPos &pos, int oreId,
                               const char *verdict, float dist,
                               const std::string &got, const char *note) {
  if (!debugOpenFile())
    return;

  mDbgFile << pos.x << ' ' << pos.y << ' ' << pos.z << ' '
           << oreTypeName(oreId) << ' ' << verdict << " d=";
  if (dist >= 0.f)
    mDbgFile << std::fixed << std::setprecision(1) << dist;
  else
    mDbgFile << '-';
  mDbgFile << ' ' << (got.empty() ? std::string("-") : got) << ' '
           << clockString();
  if (note && *note)
    mDbgFile << ' ' << note;
  mDbgFile << '\n';
  mDbgFile.flush();
}

void OreMinerV2::debugWriteVerdict(const PendingCheck &pc, DropVerdict v) {
  switch (v) {
  case DropVerdict::Real:
    mDbgReal++;
    break;
  case DropVerdict::Fake:
    mDbgFake++;
    break;
  case DropVerdict::NoDrop:
    mDbgNoDrop++;
    break;
  default:
    mDbgUnknown++;
    break;
  }

  const char *note = (v == DropVerdict::NoDrop && pc.invFull)
                         ? "(инвентарь полон)"
                         : nullptr;
  // В статистику по дистанции — только честные вердикты. nodrop при полном
  // инвентаре — не из-за дистанции, unknown — проверку прервали.
  if (pc.dist >= 0.f && v != DropVerdict::Unknown &&
      !(v == DropVerdict::NoDrop && pc.invFull))
    mDbgDist.push_back({pc.dist, v});
  debugWriteRow(pc.pos, pc.oreId, verdictName(v), pc.dist,
                formatGains(pc.seen), note);
}

// Блок фейкового кластера, который так и не копали: первый блок кластера не
// дал нужного дропа, и весь кластер ушёл в чёрный список. Пишем, чтобы было
// видно, что модуль его видел и почему пропустил.
void OreMinerV2::debugWriteSkip(const BlockPos &pos,
                                const PendingCheck &first) {
  mDbgSkip++;
  debugWriteRow(pos, first.oreId, "skip", -1.f, "", nullptr);
}

// =============================================================
// DEBUG — дистанция: таблица и порог nodrop
// =============================================================
//
// Совет — простейший порог: перебираем все границы между отсортированными
// дистанциями и берём ту, где меньше всего «ошибок» = nodrop ближе порога +
// нормальных дропов (real/fake) дальше порога. Если ни одна граница не лучше,
// чем «порога нет», — значит nodrop от дистанции не зависит. При равенстве
// берём порог побольше: лишний nodrop дешевле потерянного алмаза.
OreMinerV2::DistAdvice
OreMinerV2::adviseMaxDistance(std::vector<DistSample> s) {
  DistAdvice a;
  std::sort(s.begin(), s.end(),
            [](const DistSample &x, const DistSample &y) { return x.d < y.d; });
  const int n = static_cast<int>(s.size());
  for (const auto &e : s) {
    if (e.v == DropVerdict::NoDrop)
      a.noDrops++;
    if (e.v == DropVerdict::Real)
      a.farthestReal = (std::max)(a.farthestReal, e.d);
  }
  if (n < 10 || a.noDrops < 3)
    return a; // мало данных для вывода

  // k — сколько первых (ближних) сэмплов «до порога». Без порога (k = n)
  // ошибки = все nodrop; порог берём, только если он строго лучше. Смотрим
  // только пороги, у которых с обеих сторон хватает данных (ближе — хотя бы
  // 3 блока, дальше — хотя бы 3 nodrop) и ближе порога nodrop меньше
  // половины: если и вблизи сплошь nodrop, дело не в дистанции.
  int bestK = -1, bestErr = INT_MAX;
  int nearNo = 0;
  for (int k = 0; k < n; k++) {
    if (k >= 3 && s[k - 1].d < s[k].d && a.noDrops - nearNo >= 3 &&
        nearNo * 2 < k) {
      const int farDrops = (n - k) - (a.noDrops - nearNo);
      const int err = nearNo + farDrops;
      if (err <= bestErr) { // <= : при равенстве — порог дальше
        bestErr = err;
        bestK = k;
      }
    }
    if (s[k].v == DropVerdict::NoDrop)
      nearNo++;
  }
  if (bestK < 0 || bestErr >= a.noDrops)
    return a; // порог не помогает (или даже ближние блоки — сплошь nodrop)

  a.nearAll = bestK;
  a.farAll = n - bestK;
  a.nearNo = 0;
  for (int i = 0; i < bestK; i++)
    a.nearNo += s[i].v == DropVerdict::NoDrop ? 1 : 0;
  a.farNo = a.noDrops - a.nearNo;
  // Целый порог между самым дальним «ближним» и самым ближним «дальним»:
  // середина, но так, чтобы ближние блоки под лимит всё-таки попадали.
  const float nearMax = s[bestK - 1].d, farMin = s[bestK].d;
  int limit = static_cast<int>(std::floor((nearMax + farMin) * 0.5f));
  if (static_cast<float>(limit) < nearMax && std::ceil(nearMax) < farMin)
    limit = static_cast<int>(std::ceil(nearMax));
  a.limit = limit;
  a.ok = a.limit > 0;
  return a;
}

// В конец сессии: real/fake/nodrop по корзинам в 10 блоков + совет.
void OreMinerV2::debugWriteDistanceSummary() {
  if (!mDbgFile.is_open() || mDbgDist.empty())
    return;

  constexpr int kBucket = 10;
  std::map<int, std::array<int, 3>> buckets; // начало корзины -> r/f/n
  for (const auto &e : mDbgDist) {
    auto &b = buckets[static_cast<int>(e.d) / kBucket * kBucket];
    b[e.v == DropVerdict::Real ? 0 : e.v == DropVerdict::Fake ? 1 : 2]++;
  }
  mDbgFile << "# дистанция до руды (блоков) -> real / fake / nodrop:\n";
  for (const auto &[from, c] : buckets) {
    char line[96];
    snprintf(line, sizeof(line), "#   %3d-%-3d  %3d / %3d / %3d\n", from,
             from + kBucket, c[0], c[1], c[2]);
    mDbgFile << line;
  }

  const DistAdvice a = adviseMaxDistance(mDbgDist);
  if (a.ok)
    mDbgFile << "# nodrop заметно чаще дальше " << a.limit << " бл. (ближе: "
             << a.nearNo << " nodrop из " << a.nearAll << ", дальше: "
             << a.farNo << " из " << a.farAll << "), копаю до "
             << static_cast<int>(kMineReach) << "\n";
  else if (a.noDrops < 3 || mDbgDist.size() < 10)
    mDbgFile << "# порог nodrop: пока мало данных (nodrop: " << a.noDrops
             << ")\n";
  else
    mDbgFile << "# nodrop от дистанции не зависит\n";
  if (a.farthestReal >= 0.f)
    mDbgFile << "# самый дальний real: " << std::fixed << std::setprecision(1)
             << a.farthestReal << '\n';
}

void OreMinerV2::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &OreMinerV2::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->listen<PacketOutEvent, &OreMinerV2::onPacketOutEvent,
               nes::event_priority::ABSOLUTE_LAST>(this);
  gFeatureManager->mDispatcher
      ->listen<PacketInEvent, &OreMinerV2::onPacketInEvent>(this);
  gFeatureManager->mDispatcher->listen<RenderEvent, &OreMinerV2::onRenderEvent>(
      this);
  gFeatureManager->mDispatcher
      ->listen<BlockChangedEvent, &OreMinerV2::onBlockChangedEvent>(this);

  mState = MineState::Idle;
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mPending = PendingCheck{};
  mTickDelay = 0;
  mBlocksMined = 0;
  mFakeClusters = 0;
  mClusterNo = 0;
  mClusterReal = mClusterFake = mClusterNoDrop = 0;
  mMinedPositions.clear();
  mPatternSkipped.clear();
  mNoDropStreak = mStreakLevel = 0;
  mPauseUntil = 0;
  mSinceLastDrop.clear();
  mRetriedOnce.clear();
  cancelRelocation();
  mHopTurn = 0;
  mEmptyRelocations = 0;
  mRelocations = 0;
  mDropsSinceHop = 0;
  mOutOfReach = 0;
  mLastBusyAt = static_cast<uint64_t>(NOW);
  // Auto Drop: что считать мусором, узнаём заново.
  mJunkNames.clear();
  mInvPrev.clear();
  mInvPrevValid = false;
  mDropAskedAt.fill(0);
  mJustDropped.clear();
  mLastMinedAt = 0;
  mDropSettle = false;
  mJunkDropped = 0;
  hiddenReset(static_cast<uint64_t>(NOW));

  {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    mPacketPositions.clear();
    syncSpotsWorld(); // тот же мир — память шаблонов с прошлого включения цела
    resetScan();
  }

  auto player = ClientInstance::get()->getLocalPlayer();
  if (player) {
    auto rot = player->getActorRotationComponent();
    if (rot)
      mRots = {rot->mPitch, rot->mYaw, rot->mYaw};
  }
}

void OreMinerV2::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &OreMinerV2::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<PacketOutEvent, &OreMinerV2::onPacketOutEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<PacketInEvent, &OreMinerV2::onPacketInEvent>(this);
  gFeatureManager->mDispatcher->deafen<RenderEvent, &OreMinerV2::onRenderEvent>(
      this);
  gFeatureManager->mDispatcher
      ->deafen<BlockChangedEvent, &OreMinerV2::onBlockChangedEvent>(this);

  std::lock_guard<std::recursive_mutex> lk(mMutex);

  // Блок, который сломали прямо перед выключением, ещё ждёт дроп — пишем его
  // с тем, что успело прийти (или unknown), пока файлы открыты.
  abortCheck();

  mState = MineState::Idle;
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mMinedPositions.clear();
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mPacketPositions.clear();
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mPatternBlocks.clear();
  mNoDropStreak = mStreakLevel = 0;
  mPauseUntil = 0;
  mSinceLastDrop.clear();
  mRetriedOnce.clear();
  cancelRelocation();
  hiddenReset(static_cast<uint64_t>(NOW)); // восстановление после скачка — стоп

  // Файлы дампа закрываем, как только модуль выключили: каждая строка уже
  // сброшена на диск в момент записи, здесь только итог в чат.
  if (mDebugWasOn)
    debugStop();
  mDebugWasOn = false;
}

// =============================================================
// BLOCK CHANGED — incremental update
// =============================================================

void OreMinerV2::onBlockChangedEvent(BlockChangedEvent &event) {
  if (!ClientInstance::get()->getLevelRenderer())
    return;

  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  float dist = glm::distance(glm::vec3(event.mBlockPos) + glm::vec3(0.5f),
                             *player->getPos());
  if (dist > kScanReach)
    return;

  if (mBlacklistedPositions.contains(event.mBlockPos))
    return;

  int newId = event.mNewBlock->mLegacy->getBlockId();
  auto enabled = getEnabledBlockIds();

  if (std::ranges::find(enabled, newId) != enabled.end()) {
    rememberOre(event.mBlockPos, true);
    mFoundBlocks[event.mBlockPos] = {
        event.mNewBlock, AABB(event.mBlockPos, glm::vec3(1.f, 1.f, 1.f)),
        getOreColor(newId)};
    mScanDirty = true;
  } else {
    removeBlockIncremental(event.mBlockPos);
  }
}

// =============================================================
// MOVE PACKET HELPER
// =============================================================

std::shared_ptr<MovePlayerPacket>
OreMinerV2::createPacketForPos(glm::vec3 pos) {
  auto player = ClientInstance::get()->getLocalPlayer();
  auto pkt = MinecraftPackets::createPacket<MovePlayerPacket>();
  pkt->mPos = pos;
  pkt->mPlayerID = player->getRuntimeID();
  pkt->mRot = {mRots.x, mRots.y};
  pkt->mYHeadRot = mRots.z;
  pkt->mResetPosition = PositionMode::Teleport;
  pkt->mOnGround = true;
  pkt->mRidingID = -1;
  pkt->mCause = TeleportationCause::Unknown;
  pkt->mSourceEntityType = ActorType::Player;
  pkt->mTick = 0;
  return pkt;
}

// =============================================================
// CHAIN TP
// =============================================================

void OreMinerV2::straightLineTP(glm::vec3 from, glm::vec3 to,
                                bool saveForRender) {
  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender)
    return;

  float step = mStepDistance.mValue;
  glm::vec3 diff = to - from;
  float total = glm::length(diff);

  if (total < 0.01f) {
    sender->sendToServer(createPacketForPos(to).get());
    return;
  }

  glm::vec3 dir = glm::normalize(diff);
  glm::vec3 cur = from;
  std::vector<glm::vec3> positions;

  while (glm::distance(cur, to) > step) {
    cur += dir * step;
    positions.push_back(cur);
    sender->sendToServer(createPacketForPos(cur).get());
  }
  positions.push_back(to);
  sender->sendToServer(createPacketForPos(to).get());

  if (saveForRender) {
    mPacketPositions = positions;
    mLastPathTime = NOW;
  }
}

// =============================================================
// INVENTORY SNAPSHOT
// =============================================================

OreMinerV2::InvSnapshot OreMinerV2::getInventorySnapshot(int *usedSlots) {
  InvSnapshot snap;
  if (usedSlots)
    *usedSlots = 0;
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return snap;
  auto supplies = player->getSupplies();
  if (!supplies)
    return snap;
  auto container = supplies->getContainer();
  if (!container)
    return snap;

  for (int i = 0; i < 36; i++) {
    auto item = container->getItem(i);
    if (!item || !item->mItem || !item->getItem() || item->mCount <= 0)
      continue;
    snap[item->getItem()->mName] += item->mCount;
    if (usedSlots)
      (*usedSlots)++;
  }
  return snap;
}

// =============================================================
// DROP VERIFICATION — по дропу в инвентаре (на сервере автоподбор)
// =============================================================
//
//  1. Прямо перед ударом снимаем инвентарь (startCheck).
//  2. Каждый тик сравниваем текущий инвентарь с этим снимком (pollCheck):
//     прибавилось то, что падает с этой руды            -> real;
//     прибавилось что-то другое (булыжник, сланец, ...)  -> fake — но перед
//       вердиктом ждём ещё тик: вдруг нужный предмет идёт следующим пакетом;
//     за Verify Timeout не прибавилось ничего            -> nodrop.
//     Дроп пришёл раньше таймаута — вердикт сразу, ждать до конца не нужно.
//  3. Пока вердикта нет, следующий блок не ломаем: только так каждый дроп
//     честно привязан к своей координате.
//
// Первый блок кластера проверяется всегда (как и раньше): не real — весь
// кластер в чёрный список. Последний — тоже всегда: пока его дроп не пришёл,
// следующий кластер не начинаем (иначе хвост дропа прошлого кластера попадёт
// в проверку нового). С Debug проверяется каждый блок, и всё пишется в дамп.

std::string OreMinerV2::toLowerAscii(std::string s) {
  for (char &c : s)
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');
  return s;
}

OreMinerV2::DropList OreMinerV2::inventoryGains(const InvSnapshot &before,
                                                const InvSnapshot &after) {
  DropList gains;
  for (const auto &[name, cnt] : after) {
    auto it = before.find(name);
    const int prev = it == before.end() ? 0 : it->second;
    if (cnt > prev)
      gains.emplace_back(name, cnt - prev);
  }
  std::sort(gains.begin(), gains.end());
  return gains;
}

// Копим максимум прироста за всё ожидание: если другой модуль (InvManager и
// т.п.) успел выкинуть булыжник из инвентаря, мы всё равно помним, что он упал.
void OreMinerV2::mergeGains(DropList &acc, const DropList &now) {
  for (const auto &[name, cnt] : now) {
    auto it = std::find_if(acc.begin(), acc.end(),
                           [&](const auto &e) { return e.first == name; });
    if (it == acc.end())
      acc.emplace_back(name, cnt);
    else
      it->second = (std::max)(it->second, cnt);
  }
  std::sort(acc.begin(), acc.end());
}

bool OreMinerV2::hasGainOf(const DropList &gains, const char *what) {
  if (!what || !*what)
    return false;
  for (const auto &[name, cnt] : gains)
    if (cnt > 0 && toLowerAscii(name).find(what) != std::string::npos)
      return true;
  return false;
}

OreMinerV2::DropVerdict OreMinerV2::judgeDrop(const DropList &gains,
                                              int oreId) {
  if (hasGainOf(gains, expectedDropName(oreId)))
    return DropVerdict::Real;
  if (!gains.empty())
    return DropVerdict::Fake;
  return DropVerdict::NoDrop;
}

std::string OreMinerV2::formatGains(const DropList &gains) {
  std::string s;
  for (const auto &[name, cnt] : gains) {
    if (!s.empty())
      s += '|';
    s += name;
    s += ':';
    s += std::to_string(cnt);
  }
  return s;
}

void OreMinerV2::startCheck(const BlockPos &pos, int seenId, bool decides,
                            InvSnapshot before) {
  mPending = PendingCheck{};
  mPending.active = true;
  mPending.decides = decides;
  mPending.pos = pos;
  // Что должно выпасть — по типу кластера; если его почему-то нет, то по
  // блоку, который стоял в мире в момент удара.
  mPending.oreId = getOreType(mCurrentClusterOreId) != OreType::None
                       ? mCurrentClusterOreId
                       : seenId;
  mPending.clusterNo = mClusterNo;
  mPending.clusterSize = static_cast<int>(mCurrentCluster.size());
  mPending.minedAt = NOW;
  mPending.dist = mLastMineDist; // мерили до телепорта (Follow двигает игрока)
  mPending.before = std::move(before);
  // Auto Drop выкинул стаки в этом же ударе — учитывается в pollCheck.
  mPending.dropped = std::move(mJustDropped);
  mJustDropped.clear();
}

bool OreMinerV2::pollCheck() {
  if (!mPending.active)
    return true;

  int used = 0;
  const InvSnapshot after = getInventorySnapshot(&used);
  if (mPending.dropped.empty()) {
    mergeGains(mPending.seen, inventoryGains(mPending.before, after));
  } else {
    // Auto Drop выкинул стаки в этом же ударе. Пока сервер этого не
    // подтвердил, стаки ещё в инвентаре; потом их станет меньше на столько
    // же. Стало меньше, чем было до удара, — выброс применён, считаем от
    // «было минус выкинуто», иначе — от «было». Так дроп с этого блока
    // (булыжник у фейка) не теряется на фоне выкинутого и не появляется
    // ложный прирост, пока сервер ещё не ответил.
    InvSnapshot base = mPending.before;
    for (const auto &[name, cnt] : mPending.dropped) {
      auto a = after.find(name);
      const int have = a == after.end() ? 0 : a->second;
      int &b = base[name];
      if (have < b) {
        b = (std::max)(0, b - cnt);
        mPending.dropApplied = true;
      }
    }
    mergeGains(mPending.seen, inventoryGains(base, after));
  }
  mPending.invFull = used >= 36;

  const uint64_t now = NOW;
  const bool gotExpected =
      hasGainOf(mPending.seen, expectedDropName(mPending.oreId));

  if (!mPending.seen.empty() && mPending.firstGainAt == 0) {
    mPending.firstGainAt = now;
    if (!gotExpected)
      return false; // пришло «не то» — ещё тик, вдруг нужное идёт следом
  }

  const bool timedOut =
      now - mPending.minedAt >= static_cast<uint64_t>(mVerifyTimeout.mValue);
  if (mPending.seen.empty() && !timedOut)
    return false; // дроп ещё в пути

  DropVerdict v = judgeDrop(mPending.seen, mPending.oreId);
  // Сервер ломает и выкидывает по порядку: дроп с блока (булыжник у фейка)
  // мог лечь в тот же стак и уйти вместе с ним. Тогда «ничего не пришло» —
  // не nodrop, а неизвестно (в серию «без дропа» не идёт).
  if (v == DropVerdict::NoDrop && mPending.dropApplied)
    v = DropVerdict::Unknown;
  finishCheck(v, now);
  return true;
}

// Общая часть finishCheck/abortCheck: счётчики кластера, лог, строка в дамп.
void OreMinerV2::recordCheck(const PendingCheck &pc, DropVerdict v,
                             uint64_t now) {
  if (pc.clusterNo == mClusterNo) {
    if (v == DropVerdict::Real)
      mClusterReal++;
    else if (v == DropVerdict::Fake)
      mClusterFake++;
    else if (v == DropVerdict::NoDrop)
      mClusterNoDrop++;
  }

  const uint64_t gotAt = pc.firstGainAt ? pc.firstGainAt : now;
  const uint64_t waitMs = gotAt > pc.minedAt ? gotAt - pc.minedAt : 0;
  if (v == DropVerdict::Real)
    spdlog::info("[OreMinerV2] real {} ({}, {}, {}) at {:.1f} blocks got '{}' "
                 "in {} ms",
                 oreTypeName(pc.oreId), pc.pos.x, pc.pos.y, pc.pos.z, pc.dist,
                 formatGains(pc.seen), waitMs);
  else
    spdlog::warn("[OreMinerV2] {} {} ({}, {}, {}) at {:.1f} blocks got '{}' "
                 "in {} ms{}",
                 verdictName(v), oreTypeName(pc.oreId), pc.pos.x, pc.pos.y,
                 pc.pos.z, pc.dist, formatGains(pc.seen), waitMs,
                 pc.invFull ? " [inventory full]" : "");

  if (mDebug.mValue)
    debugWriteVerdict(pc, v);
}

void OreMinerV2::finishCheck(DropVerdict v, uint64_t now) {
  const PendingCheck pc = std::move(mPending);
  mPending = PendingCheck{};
  mLastBusyAt = now;
  if (v == DropVerdict::Real || v == DropVerdict::Fake)
    mDropsSinceHop++; // что-то упало — переезд был не впустую
  recordCheck(pc, v, now);

  // Полный инвентарь: автоподбору некуда положить дроп, и любой кластер
  // будет выглядеть «пустым». Предупреждаем не чаще раза в 10 секунд.
  if (v == DropVerdict::NoDrop && pc.invFull &&
      now - mLastInvFullWarn > 10000) {
    mLastInvFullWarn = now;
    ChatUtils::displayClientMessage(
        "§7[OreMinerV2] §cИнвентарь полон§7 — дропу некуда упасть, руду "
        "проверить нельзя");
  }

  if (pc.decides && v != DropVerdict::Real)
    rejectCluster(pc, v);
  trackNoDropStreak(v, pc.invFull, now);
}

// Первый блок кластера не дал нужного дропа — кластер фейковый: в чёрный
// список целиком (как и раньше), остальные блоки не трогаем.
void OreMinerV2::rejectCluster(const PendingCheck &pc, DropVerdict v) {
  mFakeClusters++;
  spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks)! Blacklisting.",
               mCurrentCluster.size());

  if (mDebug.mValue) {
    for (size_t i = 1; i < mCurrentCluster.size(); i++)
      debugWriteSkip(mCurrentCluster[i], pc);

    const std::string got = formatGains(pc.seen);
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §fКластер §e#{}§f {} ({} бл., d={:.0f}): §c{}§7 ({}) — "
        "пропущен",
        pc.clusterNo, oreTypeName(pc.oreId), pc.clusterSize, pc.dist,
        v == DropVerdict::Fake ? "FAKE" : "NO DROP",
        got.empty() ? std::string("ничего") : got));
  }

  blacklistPositions(mCurrentCluster);
  for (auto &p : mCurrentCluster)
    mMinedPositions.insert(p);
  // Выкинут за «без дропа»: если окажется, что это сервер перестал ломать
  // блоки, trackNoDropStreak вернёт кластер на перепроверку.
  if (v == DropVerdict::NoDrop)
    mSinceLastDrop.insert(mSinceLastDrop.end(), mCurrentCluster.begin(),
                          mCurrentCluster.end());

  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mState = MineState::Idle;
  mTickDelay = 2;
}

// Столько переездов подряд — и ни одного дропа: дело не в месте (не те руды
// включены, стоишь слишком высоко, регион, где ломать нельзя...). Дальше не
// прыгаем, чтобы не улететь неизвестно куда: Auto Relocate выключается.
// true — выключили, переезжать нельзя.
static bool stopIfEmptyRelocations(OreMinerV2 &m) {
  if (m.mRelocations == 0 || m.mDropsSinceHop > 0 ||
      m.mEmptyRelocations + 1 < OreMinerV2::kMaxEmptyRelocations)
    return false;
  m.mAutoRelocate.mValue = false;
  m.mEmptyRelocations = 0;
  std::string hint = "Проверь, какие руды включены";
  if (auto player = ClientInstance::get()->getLocalPlayer()) {
    const float y = player->getPos()->y;
    if (y > 40.f)
      hint = fmt::format("Ты на y={:.0f}, а алмазы ниже y=16 — спустись "
                         "(.tp ~ 10 ~)",
                         y);
  }
  spdlog::warn("[OreMinerV2] Auto Relocate stopped: {} relocations in a row "
               "without any drop",
               OreMinerV2::kMaxEmptyRelocations);
  ChatUtils::displayClientMessage(fmt::format(
      "§7[OreMinerV2] §cAuto Relocate выключен§7: {} переездов подряд — и ни "
      "одного дропа. {}",
      OreMinerV2::kMaxEmptyRelocations, hint));
  return true;
}

// =============================================================
// AUTO DROP — мусор после копки
// =============================================================
//
// Мусор — всё, что прибавилось в инвентаре, пока модуль копает (ломает блок
// или прошло меньше 2 с с последнего), кроме нужного: алмазов (и угля /
// изумрудов, если их копаем) и на всякий случай инструментов, брони, еды.
// Выкидывается целыми стаками прямо в mineBlockAtPos, между ударом и
// телепортом обратно: сервер в этот момент видит нас у руды, и вещи падают
// там (до 60 блоков от нас), автоподбор не возвращает их обратно.

bool OreMinerV2::isKeptItem(const std::string &n) const {
  if (n.find("diamond") != std::string::npos)
    return true; // алмазы, алмазная руда (шёлк), алмазные инструменты
  if (mCoal.mValue && n.find("coal") != std::string::npos)
    return true;
  if (mEmerald.mValue && n.find("emerald") != std::string::npos)
    return true;
  static const char *const kKeep[] = {
      "pickaxe", "sword",  "_axe",    "shovel", "_hoe",   "helmet",
      "chestplate", "leggings", "boots", "totem", "elytra", "shield",
      "bow",     "apple",  "bread",   "beef",   "porkchop", "steak",
      "carrot",  "potato", "golden",  "pearl",  "shulker", "netherite"};
  for (const char *k : kKeep)
    if (n.find(k) != std::string::npos)
      return true;
  return false;
}

void OreMinerV2::learnJunk(uint64_t now) {
  if (!mAutoDrop.mValue) {
    mJunkNames.clear();
    mInvPrevValid = false;
    return;
  }
  InvSnapshot cur = getInventorySnapshot();
  const bool fromMining = mPending.active || mState == MineState::Mining ||
                          (mLastMinedAt != 0 && since(now, mLastMinedAt) < 2000);
  if (mInvPrevValid && fromMining) {
    for (const auto &[name, cnt] : cur) {
      auto it = mInvPrev.find(name);
      const int was = it == mInvPrev.end() ? 0 : it->second;
      if (cnt > was && !isKeptItem(toLowerAscii(name)) &&
          mJunkNames.insert(name).second)
        spdlog::info("[OreMinerV2] Auto Drop: '{}' -> junk", name);
    }
  }
  mInvPrev = std::move(cur);
  mInvPrevValid = true;
}

void OreMinerV2::dropJunkHere(Inventory *container) {
  mJustDropped.clear();
  if (!mAutoDrop.mValue || mJunkNames.empty() || !container)
    return;
  const uint64_t now = NOW;
  int n = 0;
  for (int i = 0; i < 36 && n < kDropStacksPerBlock; i++) {
    auto item = container->getItem(i);
    if (!item || !item->mItem || !item->getItem() || item->mCount <= 0)
      continue;
    const std::string &name = item->getItem()->mName;
    if (!mJunkNames.contains(name) || isKeptItem(toLowerAscii(name)))
      continue;
    if (mDropAskedAt[i] != 0 && since(now, mDropAskedAt[i]) < 1500)
      continue; // уже выкидывали — ждём, пока сервер ответит
    mJustDropped[name] += item->mCount; // до dropSlot: стак ещё на месте
    container->dropSlot(i);
    mDropAskedAt[i] = now;
    n++;
  }
  mJunkDropped += n;
  if (n > 0)
    mDropSettle = true;
}

// Сервер иногда перестаёт ломать блоки (откат античита, сломалась кирка, лаг):
// тогда «без дропа» идёт с ЛЮБОГО блока — даже с фейка, который обычно даёт
// булыжник. В логе так было дважды (9 и 24 проверки подряд), и два настоящих
// алмаза из такой полосы ушли в чёрный список — в следующей сессии те же
// координаты дали real. Поэтому kNoDropStreak «без дропа» подряд -> пауза
// (2, 4, 8, 16 с), а всё, что сломано/выкинуто с последнего нормального дропа,
// возвращается на перепроверку. Каждый блок — максимум один раз, чтобы честные
// пустышки (фейк поверх воздуха/бедрока) не крутились по кругу.
void OreMinerV2::trackNoDropStreak(DropVerdict v, bool invFull, uint64_t now) {
  if (v == DropVerdict::Real || v == DropVerdict::Fake) {
    // Дроп идёт — сервер блоки ломает; одиночный nodrop до этого был честным.
    mNoDropStreak = 0;
    mStreakLevel = 0;
    mSinceLastDrop.clear();
    return;
  }
  if (v != DropVerdict::NoDrop)
    return;
  if (++mNoDropStreak % kNoDropStreak != 0)
    return;

  // Auto Relocate: в инвентарь ничего не падает — не ждём и не ставим на
  // паузу, а сразу переезжаем (переезд — в этом же тике, см. mRelocateNow).
  // Полный инвентарь переездом не лечится — тогда, как и раньше, пауза.
  if (mAutoRelocate.mValue && !invFull && !stopIfEmptyRelocations(*this)) {
    spdlog::warn("[OreMinerV2] {} checks in a row without any drop -> "
                 "relocating",
                 mNoDropStreak);
    mRelocateNow = fmt::format("{} блока подряд без дропа", mNoDropStreak);
    return;
  }

  auto source = ClientInstance::get()->getBlockSource();
  const std::vector<int> enabled = getEnabledBlockIds();
  int returned = 0;
  for (const BlockPos &p : mSinceLastDrop) {
    if (!mRetriedOnce.insert(p).second)
      continue;
    mBlacklistedPositions.erase(p);
    mMinedPositions.erase(p);
    returned++;
    // Не сломанный сервером блок возвращается клиенту — подхватываем сразу,
    // не дожидаясь, пока сканер снова дойдёт до этого чанка.
    Block *b = source ? source->getBlock(p) : nullptr;
    if (b && b->mLegacy &&
        std::ranges::find(enabled, b->mLegacy->getBlockId()) != enabled.end())
      mFoundBlocks[p] = {b, AABB(p, glm::vec3(1.f, 1.f, 1.f)),
                         getOreColor(b->mLegacy->getBlockId())};
  }
  mSinceLastDrop.clear();
  mScanDirty = true;

  // Недокопанный кластер бросаем: его остаток не помечен как сломанный и
  // вернётся вместе с остальными после паузы.
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mState = MineState::Idle;

  const int pauseMs = 2000 << std::min(mStreakLevel, 3); // 2, 4, 8, 16 с
  mStreakLevel++;
  mPauseUntil = now + static_cast<uint64_t>(pauseMs);

  const char *why = invFull ? "инвентарь полон, освободи место"
                            : "похоже, сервер не ломает блоки (откат "
                              "античита? сломалась кирка? лаг?)";
  spdlog::warn("[OreMinerV2] {} checks in a row without any drop{} -> pause {} "
               "ms, {} blocks back for re-check",
               mNoDropStreak, invFull ? " (inventory full)" : "", pauseMs,
               returned);
  ChatUtils::displayClientMessage(fmt::format(
      "§7[OreMinerV2] §cПодряд без дропа: {}§7 — {}. Пауза {} с, на "
      "перепроверку: {} бл.",
      mNoDropStreak, why, pauseMs / 1000, returned));
  if (mDebug.mValue && mDbgFile.is_open()) {
    mDbgFile << "# " << clockString() << " подряд без дропа: " << mNoDropStreak
             << " - " << (invFull ? "инвентарь полон" : "сервер не ломает блоки")
             << ", пауза " << pauseMs / 1000 << " с, на перепроверку: "
             << returned << " бл.\n";
    mDbgFile.flush();
  }
}

void OreMinerV2::abortCheck() {
  if (!mPending.active)
    return;
  const PendingCheck pc = std::move(mPending);
  mPending = PendingCheck{};
  // Что успело прийти — по тому и судим. Не пришло ничего — честно unknown:
  // таймаут не истёк, утверждать nodrop нельзя.
  const DropVerdict v =
      pc.seen.empty() ? DropVerdict::Unknown : judgeDrop(pc.seen, pc.oreId);
  recordCheck(pc, v, NOW);
}

// =============================================================
// MINE BLOCK — chain TP → destroy → chain TP back (optional)
// =============================================================

bool OreMinerV2::mineBlockAtPos(const BlockPos &pos, Actor *player) {
  mJustDropped.clear();
  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return false;
  auto source = ClientInstance::get()->getBlockSource();
  if (!source) return false;

  Block *block = source->getBlock(pos);
  if (!block || block->mLegacy->isAir()) return false;

  int face = BlockUtils::getExposedFace(pos);
  if (face == -1) face = 1;

  auto supplies = player->getSupplies();
  if (!supplies) return false;
  auto container = supplies->getContainer();
  if (!container) return false;

  int bestTool = ItemUtils::getBestBreakingTool(block, true);
  int oldSlot = supplies->mSelectedSlot;
  glm::vec3 pp = *player->getPos();
  glm::vec3 mp = glm::vec3(pos.x + 0.5f, pos.y + 2.62f, pos.z + 0.5f);
  // Дистанция «от меня до руды» — для дампа (d=) и совета по Max Distance.
  mLastMineDist = glm::distance(pp, glm::vec3(pos) + glm::vec3(0.5f));

  // 1) TP to block
  straightLineTP(pp, mp, true);

  // 2) Equip
  if (bestTool != oldSlot) {
    auto eq = PacketUtils::createMobEquipmentPacket(bestTool);
    sender->sendToServer(eq.get());
  }

  // 3) Swing
  if (mSwing.mValue) player->swing();

  // 4) StartDestroyBlock
  auto sp = MinecraftPackets::createPacket<PlayerActionPacket>();
  sp->mPos = pos;
  sp->mResultPos = pos;
  sp->mFace = face;
  sp->mAction = static_cast<PlayerActionType>(0);
  sp->mRuntimeId = player->getRuntimeID();
  sp->mtIsFromServerPlayerMovementSystem = false;
  sender->sendToServer(sp.get());

  // 5) StopDestroyBlock
  auto st = MinecraftPackets::createPacket<PlayerActionPacket>();
  st->mPos = pos;
  st->mResultPos = pos;
  st->mFace = face;
  st->mAction = PlayerActionType::StopDestroyBlock;
  st->mRuntimeId = player->getRuntimeID();
  st->mtIsFromServerPlayerMovementSystem = false;
  sender->sendToServer(st.get());

  // 6) Destroy transaction
  auto tp = MinecraftPackets::createPacket<InventoryTransactionPacket>();
  auto cit = std::make_unique<ItemUseInventoryTransaction>();
  cit->mActionType = ItemUseInventoryTransaction::ActionType::Destroy;
  cit->mSlot = bestTool;
  cit->mItemInHand = NetworkItemStackDescriptor(*container->getItem(bestTool));
  cit->mBlockPos = pos;
  cit->mFace = face;
  cit->mTargetBlockRuntimeId = 0;
  cit->mPlayerPos = mp;
  cit->mClickPos = glm::vec3(0.5f, 1.0f, 0.5f);
  tp->mTransaction = std::move(cit);
  sender->sendToServer(tp.get());

  // 7) Restore slot
  if (bestTool != oldSlot) {
    auto r = PacketUtils::createMobEquipmentPacket(oldSlot);
    sender->sendToServer(r.get());
  }

  // 7.5) Auto Drop: мусор выкидываем сейчас — сервер видит нас у руды, вещи
  // упадут там, а не под ноги (иначе автоподбор поднял бы их обратно).
  dropJunkHere(container);

  // 8) TP back — только если Follow ВЫКЛЮЧЕН
  if (!mFollow.mValue) {
    straightLineTP(mp, pp, false);
  } else {
    player->setPosition(mp);
    auto sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
  }

  // 9) Clear locally
  TRY_CALL([&]() { BlockUtils::clearBlock(pos); });

  mBlocksMined++;
  mLastBusyAt = NOW;
  mLastMinedAt = mLastBusyAt;
  mCurrentTarget = pos;
  // Сломано, но дроп ещё не подтверждён — если окажется, что сервер в это
  // время блоки не ломал, вернём на перепроверку (trackNoDropStreak).
  if (mSinceLastDrop.size() > 1024)
    mSinceLastDrop.erase(mSinceLastDrop.begin(), mSinceLastDrop.begin() + 512);
  mSinceLastDrop.push_back(pos);
  return true;
}

// =============================================================
// AUTO RELOCATE — всё вокруг выкопано -> прыгаем на новое место
// =============================================================
//
// Руками это было так: модуль всё выкопал -> выключить его -> 2-3 раза
// ".tp 10000 ~ 10000" с паузой ~0.6 с -> включить. Сервер пускает телепорт
// только до края чанков, прогруженных у него вокруг игрока, поэтому первый
// прыжок длинный, а следующие сразу за ним короче — чанки впереди ещё не
// прогрузились. Здесь то же самое, но само. Когда переезжать:
//   A. копать нечего (ближе 60 блоков руды нет — выкопано или фейки, а что
//      дальше, не достать) Idle Time секунд с последнего сломанного блока
//      или дропа. Решение — только после полного прохода сканера по новому
//      месту, но таймер идёт и во время прохода (считается от последней
//      работы, а не от конца скана);
//   B. в инвентарь перестало падать что-либо (kNoDropStreak проверок подряд
//      без дропа) — сразу, без Idle Time.
// Переезд: kHopCount прыжков по kHopDistance на +X +Z (упёрлись — следующий
// переезд повернёт на 90°) с паузой kHopDelayMs, той же цепочкой
// телепорт-пакетов, что и .tp; сколько сервер реально пустил, меряется после
// каждого прыжка; потом ждём чанки вокруг нового места (до 5 с) и сканируем.
// Пока идёт переезд, Silent Accept не глотает откаты сервера, иначе клиент
// останется там, куда сервер его не пустил. Уже сломанное (mMinedPositions) и
// память шаблонов не забываются — не то что при выключении модуля руками.

glm::vec3 OreMinerV2::hopDirection() const {
  glm::vec3 dir(1.f, 0.f, 1.f); // как ".tp 10000 ~ 10000": +X +Z
  // Прыжки упирались в стену (граница мира, сервер не пускает) — каждый раз
  // поворачиваем на 90° по часовой (вид сверху: +X -> +Z -> -X -> -Z).
  for (int i = 0; i < (mHopTurn & 3); i++)
    dir = glm::vec3(-dir.z, 0.f, dir.x);
  return glm::normalize(dir);
}

// Клиент получил чанки вокруг: в каждом из (2r+1)^2 чанков есть данные
// подчанка, в котором стоит игрок (то же, что читает сканер).
bool OreMinerV2::chunksReadyAround(const glm::vec3 &pos, int radius) {
  auto bs = ClientInstance::get()->getBlockSource();
  if (!bs)
    return false;
  // floor, а не ChunkPos(vec3): тот режет к нулю и на минусовых координатах
  // ошибается на чанк.
  const ChunkPos c(static_cast<int>(std::floor(pos.x)) >> 4,
                   static_cast<int>(std::floor(pos.z)) >> 4);
  const int sub =
      (static_cast<int>(std::floor(pos.y)) - bs->getBuildDepth()) >> 4;
  bool ready = true;
  const bool ok = TRY_CALL([&]() {
    for (int dx = -radius; dx <= radius && ready; dx++) {
      for (int dz = -radius; dz <= radius && ready; dz++) {
        LevelChunk *ch = bs->getChunk(ChunkPos(c.x + dx, c.y + dz));
        auto *subs = ch ? ch->getSubChunks() : nullptr;
        if (!subs || sub < 0 || sub >= static_cast<int>(subs->size()) ||
            !(*subs)[sub].blockReadPtr)
          ready = false;
      }
    }
  });
  return ok && ready;
}

// Вызывается из Idle, когда копать нечего (findNearestCluster пуст).
void OreMinerV2::maybeAutoRelocate(Actor *player, uint64_t now) {
  if (!mAutoRelocate.mValue)
    return;
  // Сканер ещё не прошёл всё вокруг — руда, может, просто не найдена пока.
  // Таймер при этом не сбрасываем: Idle Time считается от последней работы.
  if (!mScanComplete)
    return;
  const auto idleMs = static_cast<uint64_t>(
      std::clamp(mIdleTime.mValue, 0.f, 10.f) * 1000.f + 0.5f);
  const uint64_t idle = since(now, mLastBusyAt);
  if (idle < idleMs)
    return;
  if (stopIfEmptyRelocations(*this))
    return;
  std::string why =
      fmt::format("нечего копать {:.1f} с", static_cast<double>(idle) / 1000.0);
  if (mOutOfReach > 0)
    why += fmt::format(", дальше {} бл. ещё {}", static_cast<int>(kMineReach),
                       mOutOfReach);
  startRelocation(player, std::move(why), now);
}

void OreMinerV2::startRelocation(Actor *player, std::string why,
                                 uint64_t now) {
  mRelocateNow.clear();
  abortCheck(); // сюда приходят уже без ожидающей проверки — на всякий случай

  // Переезды подряд, после которых ничего не упало (стоп-кран). Считаются
  // дропы, а не сломанные блоки: блоки «ломаются» и тогда, когда сервер их
  // не ломает. Место, где модуль включили, — не результат переезда.
  if (mRelocations > 0)
    mEmptyRelocations = mDropsSinceHop == 0 ? mEmptyRelocations + 1 : 0;
  mDropsSinceHop = 0;

  mState = MineState::Idle;
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mTickDelay = 0;
  // Серия «без дропа» и пауза относятся к старому месту.
  mPauseUntil = 0;
  mNoDropStreak = mStreakLevel = 0;
  mSinceLastDrop.clear();
  // Найденное вокруг старого места больше не нужно (сломанное помним в
  // mMinedPositions, шаблоны — в mChunkSpots).
  resetScan();

  mHopPhase = HopPhase::Hopping;
  mRelocating.store(true);
  mHopCorrections.store(0);
  mHopReason = std::move(why);
  mHopsDone = 0;
  mHopNextAt = now; // первый прыжок — сразу
  mHopStartedAt = now;
  mSettleSince = 0;
  mHopOrigin = *player->getPos();
  mHopFrom = mHopOrigin;
  mHopDir = hopDirection();
  mHopMoves.clear();
  mRelocations++;

  // Куда летим — по сторонам света (для Look и после поворотов тоже).
  std::string where;
  if (mHopDir.x > 0.38f)
    where = "+X";
  else if (mHopDir.x < -0.38f)
    where = "-X";
  if (mHopDir.z > 0.38f)
    where += where.empty() ? "+Z" : " +Z";
  else if (mHopDir.z < -0.38f)
    where += where.empty() ? "-Z" : " -Z";
  if (mHopTurn % 4 != 0)
    where += " (повернул)";
  spdlog::info("[OreMinerV2] relocate #{} ({}): {} hops x {} blocks, dir {}",
               mRelocations, mHopReason, kHopCount,
               static_cast<int>(kHopDistance), where);
  ChatUtils::displayClientMessage(fmt::format(
      "§7[OreMinerV2] §bПереезд§7 ({}): {} x {} бл. на §f{}", mHopReason,
      kHopCount, static_cast<int>(kHopDistance), where));

  tickRelocation(player, now); // первый прыжок — сразу, в этом же тике
}

void OreMinerV2::tickRelocation(Actor *player, uint64_t now) {
  const glm::vec3 pos = *player->getPos();

  if (mHopPhase == HopPhase::Hopping) {
    if (now < mHopNextAt)
      return; // пауза между прыжками: сервер догружает чанки, откат доходит

    bool stuck = false;
    if (mHopsDone > 0) {
      // Где мы на самом деле — после возможного отката сервера.
      const float moved =
          glm::length(glm::vec2(pos.x - mHopFrom.x, pos.z - mHopFrom.z));
      mHopMoves.push_back(static_cast<int>(std::lround(moved)));
      stuck = moved < 4.f; // не пустил вовсе — дальше биться бесполезно
    }
    if (mHopsDone >= kHopCount || stuck) {
      mHopPhase = HopPhase::Settling;
      mSettleSince = now;
      return;
    }

    // Прыжок — как .tp: цепочка телепорт-пакетов по Step Distance + сам
    // клиент туда же. Не пустит сервер — вернёт откатом (его не глотаем).
    const glm::vec3 to = pos + mHopDir * kHopDistance;
    straightLineTP(pos, to, true);
    player->setPosition(to);
    if (auto sv = player->getStateVectorComponent())
      sv->mVelocity = glm::vec3(0.f);
    mHopFrom = pos;
    mHopsDone++;
    mHopNextAt = now + kHopDelayMs;
    return;
  }

  // Settling. Минимум полсекунды: запоздавший откат сервера должен успеть
  // прийти, пока Silent Accept выключен. Дальше — пока не придут чанки
  // вокруг (радиус 2: столько соседей нужно Pattern Filter), но не дольше
  // kSettleMaxMs.
  if (now - mSettleSince < 500)
    return;
  const bool ready = chunksReadyAround(pos, 2);
  if (!ready && now - mSettleSince < kSettleMaxMs)
    return;
  finishRelocation(player, ready, now);
}

void OreMinerV2::finishRelocation(Actor *player, bool chunksReady,
                                  uint64_t now) {
  const glm::vec3 pos = *player->getPos();
  const int total = static_cast<int>(std::lround(
      glm::length(glm::vec2(pos.x - mHopOrigin.x, pos.z - mHopOrigin.z))));
  std::string parts;
  for (size_t i = 0; i < mHopMoves.size(); i++) {
    if (i)
      parts += '+';
    parts += std::to_string(mHopMoves[i]);
  }
  if (parts.empty())
    parts = "0";
  const int corrections = mHopCorrections.load();
  const double secs = static_cast<double>(now - mHopStartedAt) / 1000.0;

  mHopPhase = HopPhase::None;
  mRelocating.store(false);
  mHopsDone = 0;
  resetScan();       // центр спирали — здесь, mScanComplete = false
  mLastBusyAt = now; // Idle Time — заново, от прибытия
  hiddenQuiet(now + kQuietMs); // поздний откат сервера — не «скачок»

  // Сдвинулись меньше чем на чанк — дальше в эту сторону не пускают.
  const bool blocked = total < 16;
  if (blocked)
    mHopTurn++;

  spdlog::info("[OreMinerV2] relocated {} blocks ({}) in {:.1f} s, {} server "
               "corrections{}{}",
               total, parts, secs, corrections,
               chunksReady ? "" : ", chunks not ready",
               blocked ? ", blocked -> turning 90 deg" : "");
  std::string msg = fmt::format(
      "§7[OreMinerV2] §bПереехал§7 на §f{}§7 бл. ({}) за {:.1f} с", total,
      parts, secs);
  if (corrections > 0)
    msg += fmt::format(", сервер поправил позицию {} р.", corrections);
  if (!chunksReady)
    msg += ", §eчанки ещё грузятся§7";
  if (blocked)
    msg += ". §eДальше не пускает§7 — в следующий раз поверну на 90°";
  ChatUtils::displayClientMessage(msg);

  if (mDebug.mValue && debugOpenFile()) {
    mDbgFile << "# " << clockString() << " переезд (" << mHopReason << "): "
             << static_cast<int>(std::floor(mHopOrigin.x)) << ' '
             << static_cast<int>(std::floor(mHopOrigin.y)) << ' '
             << static_cast<int>(std::floor(mHopOrigin.z)) << " -> "
             << static_cast<int>(std::floor(pos.x)) << ' '
             << static_cast<int>(std::floor(pos.y)) << ' '
             << static_cast<int>(std::floor(pos.z)) << " = " << total
             << " бл. (" << parts << ")";
    if (corrections > 0)
      mDbgFile << ", откатов " << corrections;
    if (!chunksReady)
      mDbgFile << ", чанки не догрузились";
    mDbgFile << '\n';
    mDbgFile.flush();
  }
}

void OreMinerV2::cancelRelocation() {
  mHopPhase = HopPhase::None;
  mRelocating.store(false);
  mRelocateNow.clear();
  mHopsDone = 0;
  mHopMoves.clear();
}

// =============================================================
// MAIN TICK — scan chunks + mining state machine
// =============================================================

void OreMinerV2::onBaseTickEvent(BaseTickEvent &event) {
  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto player = event.mActor;
  if (!player) return;

  syncDebugSession();

  if (!ClientInstance::get()->getLevelRenderer()) {
    abortCheck();
    cancelRelocation();
    resetScan();
    mState = MineState::Idle;
    mCurrentCluster.clear();
    return;
  }

  auto source = ClientInstance::get()->getBlockSource();
  if (!source) return;
  syncSpotsWorld();

  const uint64_t now = static_cast<uint64_t>(NOW);

  // =================== HIDDEN ===================
  // /heal раз в 5 минут и сторож координат (скачок -> /rtp -> вниз на y=10).
  // Пока сторож чинит позицию, ничего не сканируем и не копаем.
  if (hiddenTick(*this, player, now))
    return;

  // Auto Drop: что принесла копка (вещи выкидываются в mineBlockAtPos).
  learnJunk(now);

  // =================== RELOCATION ===================
  // Пока переезжаем, не сканируем и не копаем: чанки вокруг меняются, а всё
  // найденное раньше осталось позади.
  if (mHopPhase != HopPhase::None) {
    tickRelocation(player, now);
    return;
  }

  // =================== SCANNING ===================
  // Спираль по чанкам вокруг игрока. Читаются только подчанки, где может быть
  // руда в пределах досягаемости: чанк ближней точкой не дальше kScanReach
  // по горизонтали и подчанк в пределах y ± kMineReach. Остальное
  // пропускается, не тратя бюджет тика, поэтому полный проход — пара секунд.
  {
    const glm::vec3 pp = *player->getPos();
    const int depth = source->getBuildDepth();
    const int numSub = (source->getBuildHeight() - depth) / 16;
    const int subLo = (std::max)(
        0, static_cast<int>(std::floor((pp.y - kMineReach - depth) / 16.f)));
    const int subHi = (std::min)(
        numSub - 1,
        static_cast<int>(std::floor((pp.y + kMineReach - depth) / 16.f)));
    int budget = kScanPerTick;
    for (int guard = 0; budget > 0 && guard < 4096; guard++) {
      const int ring =
          (std::max)(std::abs(mCurrentChunkPos.x - mSearchCenter.x),
                     std::abs(mCurrentChunkPos.y - mSearchCenter.y));
      if (ring > kScanRings) {
        // Проход закончен. Первым полным считается тот, в котором у клиента
        // были все чанки в пределах досягаемости (или какие-то так и не
        // пришли за kScanGiveUpMs — тогда хватит и того, что есть).
        if (!mScanComplete &&
            (mPassMissing == 0 || since(now, mScanStartedAt) >= kScanGiveUpMs))
          mScanComplete = true;
        mPassMissing = 0;
        if (mScanDirty) {
          mScanDirty = false; // до вызова: updateClusters может вернуть true
          updateClusters();
          mLastClusterUpdate = now;
        }
        mSearchCenter = chunkOf(pp);
        mCurrentChunkPos = mSearchCenter;
        mStepsCount = 0;
        mSteps = 1;
        mDirectionIndex = 0;
        mSubChunkIndex = 0;
        break;
      }
      const float cx0 = mCurrentChunkPos.x * 16.f, cz0 = mCurrentChunkPos.y * 16.f;
      const float hx = pp.x - std::clamp(pp.x, cx0, cx0 + 16.f);
      const float hz = pp.z - std::clamp(pp.z, cz0, cz0 + 16.f);
      const float h2 = hx * hx + hz * hz;
      if (h2 > kScanReach * kScanReach || subLo > subHi ||
          mSubChunkIndex > subHi) {
        mSubChunkIndex = numSub - 1; // весь чанк (или его остаток) — мимо
        moveToNext();
        continue;
      }
      if (mSubChunkIndex <= subLo) {
        // Первый подчанк окна: есть ли у клиента этот чанк вообще.
        mSubChunkIndex = subLo;
        LevelChunk *ch = nullptr;
        TRY_CALL([&]() { ch = source->getChunk(mCurrentChunkPos); });
        if (!ch) {
          if (h2 <= kMineReach * kMineReach)
            mPassMissing++;
          mSubChunkIndex = numSub - 1;
          moveToNext();
          continue;
        }
      }
      bool ok = false;
      tryProcessSub(ok, mCurrentChunkPos, mSubChunkIndex);
      if (ok)
        budget--;
      moveToNext();
    }

    BlockPos bp = *player->getPos();
    int sc = (bp.y - depth) >> 4;
    bool r = false;
    tryProcessSub(r, chunkOf(pp), sc);

    if (mScanDirty && now - mLastClusterUpdate > 2000) {
      mScanDirty = false; // до вызова: updateClusters может вернуть true
      updateClusters();
      mLastClusterUpdate = now;
    }
  }

  // =================== MINING ===================
  // Сломанный блок ждёт свой дроп. Ожидание идёт каждый тик параллельно с
  // задержкой Speed, но следующий блок не ломается, пока нет вердикта, —
  // иначе дроп нельзя привязать к координате.
  if (mPending.active && !pollCheck()) {
    if (mTickDelay > 0)
      mTickDelay--;
    return;
  }

  // В инвентарь ничего не падает (см. trackNoDropStreak) — переезжаем сразу.
  if (!mRelocateNow.empty()) {
    std::string why = std::move(mRelocateNow);
    mRelocateNow.clear();
    startRelocation(player, std::move(why), now);
    return;
  }

  if (mTickDelay > 0) {
    mTickDelay--;
    return;
  }

  // Пауза после серии «без дропа» (см. trackNoDropStreak): сканер работает,
  // проверки дожидаются, но новые блоки не ломаем.
  if (now < mPauseUntil) {
    mLastBusyAt = now; // пауза — не «нечего копать»
    return;
  }

  glm::vec3 playerPos = *player->getPos();

  switch (mState) {
  // ----- IDLE -----
  case MineState::Idle: {
    auto cluster = findNearestCluster(playerPos, mMinedPositions);
    if (cluster.empty()) {
      maybeAutoRelocate(player, now); // всё вокруг выкопано?
      return;
    }

    mCurrentCluster = cluster;
    mCurrentClusterIdx = 0;

    // Запоминаем тип руды кластера по первому блоку
    auto it = mFilteredBlocks.find(mCurrentCluster[0]);
    mCurrentClusterOreId = (it != mFilteredBlocks.end())
        ? it->second.block->mLegacy->getBlockId() : 0;

    const BlockPos fp = mCurrentCluster[0];
    Block *fb = source->getBlock(fp);
    if (!fb || fb->mLegacy->isAir()) {
      mMinedPositions.insert(fp);
      mCurrentCluster.clear();
      return;
    }

    const int seenId = fb->mLegacy->getBlockId();
    InvSnapshot before = getInventorySnapshot();
    if (!mineBlockAtPos(fp, player)) {
      mMinedPositions.insert(fp);
      mCurrentCluster.clear();
      return;
    }
    mMinedPositions.insert(fp);
    mCurrentClusterIdx = 1;

    // Новый кластер: свой номер в дампе и свои счётчики.
    mClusterNo++;
    mClusterReal = mClusterFake = mClusterNoDrop = 0;

    // Первый блок решает судьбу всего кластера — его проверяем всегда.
    startCheck(fp, seenId, true, std::move(before));

    mState = MineState::Mining;
    mTickDelay = 1;
    if (std::exchange(mDropSettle, false))
      mTickDelay = (std::max)(mTickDelay, kDropSettleTicks);
    break;
  }

  // ----- MINING -----
  case MineState::Mining: {
    // Сюда попадаем только с готовым вердиктом: фейковый первый блок уже
    // перевёл модуль в Idle внутри finishCheck().

    // Mine next block in cluster (strict order, no skipping)
    if (mCurrentClusterIdx >= static_cast<int>(mCurrentCluster.size())) {
      spdlog::info("[OreMinerV2] Cluster done! Mined: {}, Fakes: {}",
                   mBlocksMined, mFakeClusters);
      if (mDebug.mValue)
        ChatUtils::displayClientMessage(fmt::format(
            "§7[OreMinerV2] §fКластер §e#{}§f {} ({} бл.): §a{} real§7, §c{} "
            "fake§7, §e{} nodrop",
            mClusterNo, oreTypeName(mCurrentClusterOreId),
            mCurrentCluster.size(), mClusterReal, mClusterFake,
            mClusterNoDrop));
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      mState = MineState::Idle;
      mTickDelay = 2;
      return;
    }

    // Копия, а не ссылка: VeinMiner ниже дописывает в mCurrentCluster.
    const BlockPos p = mCurrentCluster[mCurrentClusterIdx];
    if (!mMinedPositions.contains(p)) {
      // С Debug по дропу проверяется каждый блок. Без Debug — ещё и
      // последний блок кластера: дроп приходит по порядку, так что, дождавшись
      // его, мы знаем, что в пути не осталось ничего от этого кластера, и
      // запоздавший алмаз/булыжник не попадёт в проверку следующего.
      const bool lastInCluster =
          mCurrentClusterIdx + 1 == static_cast<int>(mCurrentCluster.size());
      const bool verify = mDebug.mValue || lastInCluster;
      Block *b = source->getBlock(p);
      const int seenId = (b && b->mLegacy) ? b->mLegacy->getBlockId() : 0;
      InvSnapshot before;
      if (verify)
        before = getInventorySnapshot();
      if (mineBlockAtPos(p, player) && verify)
        startCheck(p, seenId, false, std::move(before));
    }
    mMinedPositions.insert(p); // помечаем в любом случае, чтобы не застрять
    mCurrentClusterIdx++;
    mTickDelay = static_cast<int>(mSpeed.mValue);
    if (std::exchange(mDropSettle, false))
      mTickDelay = (std::max)(mTickDelay, kDropSettleTicks);

    // VeinMiner: after mining, BFS from this pos to find more same-type ore
    if (mVeinMiner.mValue) {
      auto cluster = getCluster(p, mFoundBlocks);
      for (auto &cp : cluster) {
        if (mMinedPositions.contains(cp)) continue;
        if (mPatternBlocks.contains(cp)) continue;
        bool already = false;
        for (auto &ep : mCurrentCluster) {
          if (ep == cp) { already = true; break; }
        }
        if (!already) mCurrentCluster.push_back(cp);
      }
    }
    break;
  }
  }
}

// =============================================================
// PACKET OUT — capture rotations
// =============================================================

void OreMinerV2::onPacketOutEvent(PacketOutEvent &event) {
  if (event.mPacket->getId() == PacketID::MovePlayer) {
    auto pkt = event.getPacket<MovePlayerPacket>();
    mRots = {pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot};
  }
  // Ушёл /rtp (наш или набранный руками): следующий скачок позиции — его
  // телепорт, сторож сразу спустится на y=10, а не пошлёт второй /rtp.
  if (event.mPacket->getId() == PacketID::CommandRequest) {
    auto pkt = event.getPacket<CommandRequestPacket>();
    const std::string c = toLowerAscii(pkt->mCommand);
    for (const char *k : {"/rtp", "/wild", "/randomtp"})
      if (c.rfind(k, 0) == 0)
        hiddenRtpSent(static_cast<uint64_t>(NOW));
  }
}

// =============================================================
// PACKET IN — silent accept + dimension/respawn reset
// =============================================================

void OreMinerV2::onPacketInEvent(PacketInEvent &event) {
  if (event.mPacket->getId() == PacketID::MovePlayer) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player)
      return;
    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt->mPlayerID != player->getRuntimeID())
      return;
    if (mRelocating.load()) {
      // Переезд: сервер пустил только до края прогруженных чанков и вернул
      // туда. Пропускаем как есть — клиент должен встать туда же, где нас
      // видит сервер (Silent Accept тут оставил бы клиента «впереди»).
      // Откатом считаем, только если ставят не туда, где мы уже стоим
      // (некоторые серверы просто подтверждают телепорт).
      if (glm::distance(pkt->mPos, *player->getPos()) > 2.f)
        mHopCorrections.fetch_add(1);
    } else if (mSilentAccept.mValue &&
               glm::distance(pkt->mPos, *player->getPos()) <= kJumpDist) {
      event.cancel();
      ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
    }
    // Дальше 100 блоков — не откат нашей цепочки (та не дальше 60), а сервер
    // сам переставил игрока (спавн, /rtp, смерть): пускаем как есть, клиент
    // должен оказаться там же, а сторож координат разберётся, что делать.
  }

  if (event.mPacket->getId() == PacketID::ChangeDimension) {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    abortCheck();
    cancelRelocation();
    mState = MineState::Idle;
    mCurrentCluster.clear();
    mCurrentClusterIdx = 0;
    mMinedPositions.clear();
    mPacketPositions.clear();
    mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
    mChunkSpots.clear(); // другое измерение — шаблоны оттуда не годятся
    resetScan();
  }

  if (event.mPacket->getId() == PacketID::PlayerAction) {
    auto pkt = event.getPacket<PlayerActionPacket>();
    if (pkt->mAction == PlayerActionType::Respawn) {
      std::lock_guard<std::recursive_mutex> lk(mMutex);
      abortCheck();
      cancelRelocation();
      mState = MineState::Idle;
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mMinedPositions.clear();
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      resetScan();
    }
  }
}

// =============================================================
// RENDER
// =============================================================

void OreMinerV2::onRenderEvent(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto drawList = ImGui::GetBackgroundDrawList();

  float alpha = 1.0f;
  uint64_t fadeTime = 500;
  uint64_t now = NOW;

  if (mLastPathTime + fadeTime < now)
    mPacketPositions.clear();
  else {
    alpha = 1.f - static_cast<float>(now - mLastPathTime) /
                      static_cast<float>(fadeTime);
    alpha = std::clamp(alpha, 0.f, 1.f);
  }

  // ---------- BLOCK ESP + DUMP SEEN ----------
  // Сессию Debug (старт/стоп файлов) ведёт syncDebugSession — её зовёт и тик.
  // Проверенные блоки (real/fake) пишет тик, здесь — только «сырой» дамп
  // Dump Seen: тем же проходом, что и рендер, дампятся ровно те блоки, которые
  // модуль считает найденными (радиус + тип руды), включая те, что сейчас за
  // спиной/за кадром. Каждая координата пишется один раз. Фейки тут тоже
  // есть — это то, что модуль ВИДИТ, а не то, что реально оказалось рудой.
  syncDebugSession();

  const bool wantEsp = mDrawBlocks.mValue;
  const bool wantDump = mDebug.mValue && mDumpSeen.mValue;

  if (wantEsp || wantDump) {
    glm::ivec3 pp = *player->getPos();
    const float radSq = kMineReach * kMineReach; // что модуль будет копать
    auto enVec = getEnabledBlockIds();
    std::unordered_set<int> enSet(enVec.begin(), enVec.end());

    std::vector<DebugRow> dumpRows;
    int rendered = 0;

    for (auto &[pos, fb] : mFilteredBlocks) {
      if (!fb.block || !fb.block->mLegacy)
        continue;

      const int blockId = fb.block->mLegacy->getBlockId();
      if (!enSet.contains(blockId))
        continue;

      glm::vec3 d = glm::vec3(pos) - glm::vec3(pp);
      if (d.x * d.x + d.y * d.y + d.z * d.z > radSq)
        continue;

      ImVec2 cs;
      bool onScreen = false;
      if (wantDump)
        onScreen =
            RenderUtils::worldToScreen(glm::vec3(pos) + glm::vec3(0.5f), cs);

      // Рисование — как и раньше, с прежним лимитом 500 боксов на кадр.
      if (wantEsp && rendered < 500) {
        auto pts = MathUtils::getImBoxPoints(fb.aabb);
        if (!pts.empty()) {
          drawList->AddPolyline(pts.data(), pts.size(), fb.color, 0, 2.0f);
          drawList->AddConvexPolyFilled(
              pts.data(), pts.size(),
              ImColor(fb.color.Value.x, fb.color.Value.y, fb.color.Value.z,
                      0.15f));
          rendered++;
        }
      }

      if (wantDump && mDebugWritten.insert(pos).second) {
        DebugRow row;
        row.pos = pos;
        row.blockId = blockId;
        row.distance = glm::length(d);
        row.onScreen = onScreen;
        row.screenX = onScreen ? cs.x : 0.f;
        row.screenY = onScreen ? cs.y : 0.f;
        dumpRows.push_back(row);
      }
    }

    if (wantDump)
      debugFlush(dumpRows);
  }

  // ---------- TP PATH ----------
  if (mDrawPath.mValue && !mPacketPositions.empty()) {
    std::vector<ImVec2> pts;
    for (auto &p : mPacketPositions) {
      ImVec2 s;
      if (RenderUtils::worldToScreen(p, s))
        pts.push_back(s);
    }
    for (size_t i = 0; i + 1 < pts.size(); i++) {
      ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
      c.Value.w *= alpha;
      drawList->AddLine(pts[i], pts[i + 1], c, 2.0f);
    }
  }

  // ---------- CURRENT TARGET ----------
  if (mDrawTarget.mValue && mCurrentTarget.x != INT_MAX &&
      !mCurrentCluster.empty()) {
    AABB box;
    box.mMin = glm::vec3(mCurrentTarget);
    box.mMax = glm::vec3(mCurrentTarget) + glm::vec3(1.f);
    auto pts = MathUtils::getImBoxPoints(box);
    if (!pts.empty()) {
      drawList->AddConvexPolyFilled(
          pts.data(), pts.size(),
          ImColor(0, 255, 0, static_cast<int>(40 * alpha)));
      drawList->AddPolyline(pts.data(), pts.size(),
                            ImColor(0, 255, 0, static_cast<int>(200 * alpha)),
                            true, 2.f);
    }

    ImVec2 ps, ts;
    glm::vec3 center = glm::vec3(mCurrentTarget) + glm::vec3(0.5f);
    if (RenderUtils::worldToScreen(*player->getPos(), ps) &&
        RenderUtils::worldToScreen(center, ts)) {
      ImColor lc = ColorUtils::getThemedColor(0);
      lc.Value.w = 0.4f * alpha;
      drawList->AddLine(ps, ts, lc, 1.5f);
    }
  }

  // ---------- REMAINING CLUSTER ----------
  if (mDrawCluster.mValue && !mCurrentCluster.empty() &&
      mState != MineState::Idle) {
    // Жёлтый — ждём дроп первого блока (решается, фейк ли кластер).
    const bool checkingCluster = mPending.active && mPending.decides;
    ImColor cc = checkingCluster
                     ? ImColor(255, 255, 0, static_cast<int>(60 * alpha))
                     : ImColor(0, 200, 255, static_cast<int>(60 * alpha));

    for (size_t i = mCurrentClusterIdx; i < mCurrentCluster.size(); i++) {
      AABB box;
      box.mMin = glm::vec3(mCurrentCluster[i]);
      box.mMax = glm::vec3(mCurrentCluster[i]) + glm::vec3(1.f);
      auto pts = MathUtils::getImBoxPoints(box);
      if (!pts.empty())
        drawList->AddConvexPolyFilled(pts.data(), pts.size(), cc);
    }
  }

  // ---------- STATUS TEXT ----------
  {
    ImVec2 ss = ImGui::GetIO().DisplaySize;
    float cx = ss.x * 0.5f;
    float topY = ss.y * 0.05f;
    std::string status;

    const bool paused = now < mPauseUntil;
    const bool recovering = hiddenRecovering();
    // Auto Relocate ждёт Idle Time — показываем обратный отсчёт.
    const bool idleWait = mAutoRelocate.mValue && !recovering &&
                          mHopPhase == HopPhase::None &&
                          mState == MineState::Idle && !paused &&
                          !mPending.active;
    if (recovering) {
      status = hiddenStatus(now);
    } else if (mHopPhase == HopPhase::Hopping) {
      status = "Relocating: hop " + std::to_string(mHopsDone) + "/" +
               std::to_string(kHopCount);
    } else if (mHopPhase == HopPhase::Settling) {
      status = "Relocating: waiting for chunks";
    } else {
      switch (mState) {
      case MineState::Idle: {
        if (mFilteredBlocks.empty() && !paused && !idleWait)
          return;
        const size_t reach =
            mFilteredBlocks.size() > static_cast<size_t>(mOutOfReach)
                ? mFilteredBlocks.size() - static_cast<size_t>(mOutOfReach)
                : 0;
        status = reach == 0 ? std::string("Nothing to mine")
                            : "Scanning... [" + std::to_string(reach) +
                                  " ores]";
        if (mOutOfReach > 0)
          status += " | " + std::to_string(mOutOfReach) + " out of reach (>" +
                    std::to_string(static_cast<int>(kMineReach)) + ")";
        if (idleWait && reach == 0) {
          if (!mScanComplete) {
            status += " | scanning area";
          } else {
            const auto idleMs = static_cast<uint64_t>(
                std::clamp(mIdleTime.mValue, 0.f, 10.f) * 1000.f + 0.5f);
            const uint64_t idle = since(now, mLastBusyAt);
            const uint64_t left = idle < idleMs ? idleMs - idle : 0;
            char buf[48];
            snprintf(buf, sizeof(buf), " | hop in %.1fs",
                     static_cast<double>(left) / 1000.0);
            status += buf;
          }
        }
        break;
      }
      case MineState::Mining:
        if (mPending.active && mPending.decides)
          status = "Checking...";
        else
          status = "Mining " + std::to_string(mCurrentClusterIdx) + "/" +
                   std::to_string(mCurrentCluster.size()) +
                   " | Fakes: " + std::to_string(mFakeClusters);
        break;
      }
    }

    if (mDebug.mValue) {
      status += " | Real: " + std::to_string(mDbgReal) +
                " Fake: " + std::to_string(mDbgFake);
      if (mDbgNoDrop > 0)
        status += " NoDrop: " + std::to_string(mDbgNoDrop);
      if (mDumpSeen.mValue)
        status += " | Seen: " + std::to_string(mDebugRows);
    }
    if (mPatternFilter.mValue && !mPatternSkipped.empty())
      status += " | Pattern: " + std::to_string(mPatternSkipped.size());
    if (mAutoDrop.mValue && mJunkDropped > 0)
      status += " | Dropped: " + std::to_string(mJunkDropped);
    if (paused)
      status = "Paused: no drops (" +
               std::to_string((mPauseUntil - now + 999) / 1000) + "s) | " +
               status;

    ImVec2 ts = ImGui::CalcTextSize(status.c_str());
    drawList->AddRectFilled(ImVec2(cx - ts.x * 0.5f - 5.f, topY - 2.f),
                            ImVec2(cx + ts.x * 0.5f + 5.f, topY + ts.y + 2.f),
                            ImColor(0.f, 0.f, 0.f, 0.5f), 4.f);
    drawList->AddText(ImVec2(cx - ts.x * 0.5f, topY),
                      ImColor(1.f, 1.f, 1.f, alpha), status.c_str());
  }
}

std::string OreMinerV2::getSettingDisplay() {
  if (hiddenRecovering())
    return "RTP";
  if (mHopPhase == HopPhase::Hopping)
    return "Hop " + std::to_string(mHopsDone) + "/" + std::to_string(kHopCount);
  if (mHopPhase == HopPhase::Settling)
    return "Hop: chunks";
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
