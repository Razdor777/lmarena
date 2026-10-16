#include "OreMinerV2.hpp"

#include <Features/FeatureManager.hpp>

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
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
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

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
// KEY HELPERS
// =============================================================

const char *OreMinerV2::getKeyName(int vk) {
  static char buf[32];
  switch (vk) {
  case 0:
    return "None";
  case VK_LBUTTON:
    return "LMB";
  case VK_RBUTTON:
    return "RMB";
  case VK_MBUTTON:
    return "MMB";
  case VK_BACK:
    return "Back";
  case VK_TAB:
    return "Tab";
  case VK_RETURN:
    return "Enter";
  case VK_SHIFT:
    return "Shift";
  case VK_CONTROL:
    return "Ctrl";
  case VK_MENU:
    return "Alt";
  case VK_ESCAPE:
    return "Esc";
  case VK_SPACE:
    return "Space";
  case VK_DELETE:
    return "Del";
  case VK_INSERT:
    return "Ins";
  case VK_HOME:
    return "Home";
  case VK_END:
    return "End";
  case VK_CAPITAL:
    return "CapsLock";
  default:
    break;
  }
  if (vk >= VK_F1 && vk <= VK_F12) {
    snprintf(buf, 32, "F%d", vk - VK_F1 + 1);
    return buf;
  }
  if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
    snprintf(buf, 32, "Num%d", vk - VK_NUMPAD0);
    return buf;
  }
  if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
    snprintf(buf, 32, "%c", (char)vk);
    return buf;
  }
  snprintf(buf, 32, "0x%02X", vk);
  return buf;
}

bool OreMinerV2::sIsAnyKeyHeld() {
  for (int vk = 8; vk <= 254; vk++) {
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU)
      continue;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT)
      continue;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL)
      continue;
    if (vk == VK_LMENU || vk == VK_RMENU)
      continue;
    if (vk == VK_LWIN || vk == VK_RWIN)
      continue;
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
      continue;
    if (vk == VK_ESCAPE)
      continue;
    if (GetAsyncKeyState(vk) & 0x8000)
      return true;
  }
  return false;
}

int OreMinerV2::sFindHeldKey() {
  for (int vk = 8; vk <= 254; vk++) {
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU)
      continue;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT)
      continue;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL)
      continue;
    if (vk == VK_LMENU || vk == VK_RMENU)
      continue;
    if (vk == VK_LWIN || vk == VK_RWIN)
      continue;
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
      continue;
    if (vk == VK_ESCAPE)
      continue;
    if (GetAsyncKeyState(vk) & 0x8000)
      return vk;
  }
  return -1;
}

// =============================================================
// SCAN — reset
// =============================================================

void OreMinerV2::resetScan() {
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mScanComplete = false;
  mScanDirty = false;
  mLastClusterUpdate = 0;
  mStepsCount = 0;
  mSteps = 1;
  mDirectionIndex = 0;
  mSubChunkIndex = 0;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (player) {
    mSearchCenter = ChunkPos(*player->getPos());
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

        if (mBlacklistedPositions.contains(pos))
          continue;
        if (std::ranges::find(enabled, found->mLegacy->getBlockId()) ==
            enabled.end())
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

    for (auto &cp : cluster) {
      auto fit = mFoundBlocks.find(cp);
      if (fit != mFoundBlocks.end())
        mFilteredBlocks[cp] = fit->second;
    }
  }
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

  for (auto &[pos, fb] : mFilteredBlocks) {
    if (exclude.contains(pos))
      continue;
    if (mBlacklistedPositions.contains(pos))
      continue;
    float d = glm::distance(playerPos, glm::vec3(pos) + glm::vec3(0.5f));
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

// Новая сессия. Префикс содержит дату и время, поэтому старые дампы никогда
// не затираются, а каждый запуск Debug — отдельный чистый набор файлов.
void OreMinerV2::debugStart() {
  debugCloseFiles();

  std::string dir = FileUtils::getJuzdexDir() + "Dumps\\";
  try {
    FileUtils::createDirectory(dir);
  } catch (const std::exception &e) {
    spdlog::error("[OreMinerV2][Debug] cannot create {}: {}", dir, e.what());
  }

  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char stamp[32] = {};
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &tmBuf);

  mDebugStamp = stamp;
  mDebugBase = dir + "OreMinerV2_" + mDebugStamp;
  mDbgVerifiedFailed = false;
  mDbgSeenFailed = false;
  mDebugWritten.clear();
  mDebugRows = 0;
  mDbgReal = mDbgFake = mDbgNoDrop = mDbgUnknown = mDbgSkip = 0;
  mDebugStartedAt = NOW;

  spdlog::info("[OreMinerV2][Debug] session started -> {}_*", mDebugBase);
  ChatUtils::displayClientMessage(
      "§7[OreMinerV2] §aDebug ON§7: каждый сломанный блок проверяется по "
      "дропу -> §fDumps\\OreMinerV2_" +
      mDebugStamp + "_verified.csv");
}

void OreMinerV2::debugCloseFiles() {
  for (std::ofstream *f :
       {&mDbgVerified, &mDbgRealTxt, &mDbgFakeTxt, &mDbgSeenCsv, &mDbgSeenTxt}) {
    if (f->is_open()) {
      f->flush();
      f->close();
    }
  }
}

void OreMinerV2::debugStop() {
  const bool wroteVerified = mDbgVerified.is_open();
  const bool wroteSeen = mDbgSeenCsv.is_open();
  debugCloseFiles();

  if (wroteVerified) {
    spdlog::info("[OreMinerV2][Debug] verified: {} real, {} fake, {} nodrop, "
                 "{} unknown, {} skipped -> {}_verified.csv",
                 mDbgReal, mDbgFake, mDbgNoDrop, mDbgUnknown, mDbgSkip,
                 mDebugBase);
    const std::string unknown =
        mDbgUnknown > 0 ? fmt::format(", {} unknown", mDbgUnknown) : "";
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §fDebug сохранён: §a{} real§7, §c{} fake§7, §e{} "
        "nodrop§7{} -> §fDumps\\OreMinerV2_{}_real.txt",
        mDbgReal, mDbgFake, mDbgNoDrop, unknown, mDebugStamp));
  } else {
    ChatUtils::displayClientMessage(
        "§7[OreMinerV2] §fDebug OFF: ни один блок не проверен, файлы не "
        "созданы");
  }
  if (wroteSeen)
    spdlog::info("[OreMinerV2][Debug] {} seen blocks dumped to {}_seen.csv",
                 mDebugRows, mDebugBase);

  mDebugWritten.clear();
  mDebugRows = 0;
  mDebugStartedAt = 0;
  mDebugBase.clear();
  mDebugStamp.clear();
}

bool OreMinerV2::debugOpenVerified() {
  if (mDbgVerified.is_open())
    return true;
  if (mDebugBase.empty() || mDbgVerifiedFailed)
    return false;

  const std::string csv = mDebugBase + "_verified.csv";
  mDbgVerified.open(csv, std::ios::out | std::ios::trunc);
  if (!mDbgVerified.is_open()) {
    mDbgVerifiedFailed = true;
    spdlog::error("[OreMinerV2][Debug] cannot open {}", csv);
    return false;
  }

  // verdict: real / fake / nodrop / unknown / skip
  // got:     что прибавилось в инвентаре, "имя:штук|имя:штук"
  // wait_ms: от удара до прихода дропа (для nodrop — сколько прождали)
  // cluster_index: 1 — первый блок кластера, именно он решает, копать ли
  //                остальные; skip — блок не копали, потому что первый
  //                блок его кластера оказался фейком
  mDbgVerified << "world_x,world_y,world_z,ore,block_id,deepslate,verdict,got,"
                  "wait_ms,cluster,cluster_size,cluster_index,inv_full,time,"
                  "ms\n";
  mDbgVerified.flush();

  mDbgRealTxt.open(mDebugBase + "_real.txt", std::ios::out | std::ios::trunc);
  mDbgFakeTxt.open(mDebugBase + "_fake.txt", std::ios::out | std::ios::trunc);
  return true;
}

bool OreMinerV2::debugOpenSeen() {
  if (mDbgSeenCsv.is_open())
    return true;
  if (mDebugBase.empty() || mDbgSeenFailed)
    return false;

  const std::string csv = mDebugBase + "_seen.csv";
  mDbgSeenCsv.open(csv, std::ios::out | std::ios::trunc);
  if (!mDbgSeenCsv.is_open()) {
    mDbgSeenFailed = true;
    spdlog::error("[OreMinerV2][Debug] cannot open {}", csv);
    return false;
  }

  mDbgSeenCsv << "world_x,world_y,world_z,ore,block_id,deepslate,distance,"
                 "on_screen,screen_x,screen_y,ms\n";
  mDbgSeenCsv.flush();
  mDbgSeenTxt.open(mDebugBase + "_seen.txt", std::ios::out | std::ios::trunc);
  return true;
}

// Dump Seen: всё, что модуль увидел (без проверки, вместе с фейками).
void OreMinerV2::debugFlush(const std::vector<DebugRow> &rows) {
  if (rows.empty())
    return;
  if (!debugOpenSeen())
    return;

  const uint64_t now = NOW;

  for (const auto &r : rows) {
    mDbgSeenCsv << r.pos.x << ',' << r.pos.y << ',' << r.pos.z << ','
                << oreTypeName(r.blockId) << ',' << r.blockId << ','
                << (r.deepslate ? 1 : 0) << ',' << std::fixed
                << std::setprecision(2) << r.distance << ','
                << (r.onScreen ? 1 : 0) << ',' << std::setprecision(1)
                << r.screenX << ',' << r.screenY << ',' << now << '\n';

    if (mDbgSeenTxt.is_open())
      mDbgSeenTxt << r.pos.x << ' ' << r.pos.y << ' ' << r.pos.z << '\n';
  }

  mDbgSeenCsv.flush();
  if (mDbgSeenTxt.is_open())
    mDbgSeenTxt.flush();

  mDebugRows += static_cast<int>(rows.size());
}

// Проверенный блок: строка в _verified.csv + координата в _real/_fake.txt.
void OreMinerV2::debugWriteVerdict(const PendingCheck &pc, DropVerdict v,
                                   uint64_t now) {
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

  if (!debugOpenVerified())
    return;

  const uint64_t gotAt = pc.firstGainAt ? pc.firstGainAt : now;
  const uint64_t waitMs = gotAt > pc.minedAt ? gotAt - pc.minedAt : 0;

  mDbgVerified << pc.pos.x << ',' << pc.pos.y << ',' << pc.pos.z << ','
               << oreTypeName(pc.oreId) << ',' << pc.seenId << ','
               << (isDeepslateOre(pc.seenId) ? 1 : 0) << ',' << verdictName(v)
               << ',' << formatGains(pc.seen) << ',' << waitMs << ','
               << pc.clusterNo << ',' << pc.clusterSize << ',' << pc.clusterIdx
               << ',' << (pc.invFull ? 1 : 0) << ',' << clockString() << ','
               << now << '\n';
  mDbgVerified.flush();

  std::ofstream *plain = v == DropVerdict::Real   ? &mDbgRealTxt
                         : v == DropVerdict::Fake ? &mDbgFakeTxt
                                                  : nullptr;
  if (plain && plain->is_open()) {
    *plain << pc.pos.x << ' ' << pc.pos.y << ' ' << pc.pos.z << '\n';
    plain->flush();
  }
}

// Блок фейкового кластера, который так и не копали: первый блок кластера не
// дал нужного дропа, и весь кластер ушёл в чёрный список. Пишем, чтобы было
// видно, что модуль его видел и почему пропустил.
void OreMinerV2::debugWriteSkip(const BlockPos &pos, int blockId,
                                const PendingCheck &first, int clusterIdx) {
  mDbgSkip++;
  if (!debugOpenVerified())
    return;

  mDbgVerified << pos.x << ',' << pos.y << ',' << pos.z << ','
               << oreTypeName(first.oreId) << ',' << blockId << ','
               << (isDeepslateOre(blockId) ? 1 : 0) << ",skip,,,"
               << first.clusterNo << ',' << first.clusterSize << ','
               << clusterIdx << ",0," << clockString() << ',' << NOW << '\n';
  mDbgVerified.flush();
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
  mQuickTPKeyWasDown = false;
  mIsBindingKey = false;
  mBindWaitRelease = false;

  {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    mPacketPositions.clear();
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
  mIsBindingKey = false;

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
  if (dist > mRadius.mValue)
    return;

  if (mBlacklistedPositions.contains(event.mBlockPos))
    return;

  int newId = event.mNewBlock->mLegacy->getBlockId();
  auto enabled = getEnabledBlockIds();

  if (std::ranges::find(enabled, newId) != enabled.end()) {
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
                            int clusterIdx, InvSnapshot before) {
  mPending = PendingCheck{};
  mPending.active = true;
  mPending.decides = decides;
  mPending.pos = pos;
  mPending.seenId = seenId;
  // Что должно выпасть — по типу кластера; если его почему-то нет, то по
  // блоку, который стоял в мире в момент удара.
  mPending.oreId = getOreType(mCurrentClusterOreId) != OreType::None
                       ? mCurrentClusterOreId
                       : seenId;
  mPending.clusterNo = mClusterNo;
  mPending.clusterSize = static_cast<int>(mCurrentCluster.size());
  mPending.clusterIdx = clusterIdx;
  mPending.minedAt = NOW;
  mPending.before = std::move(before);
}

bool OreMinerV2::pollCheck() {
  if (!mPending.active)
    return true;

  int used = 0;
  const InvSnapshot after = getInventorySnapshot(&used);
  mergeGains(mPending.seen, inventoryGains(mPending.before, after));
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

  finishCheck(judgeDrop(mPending.seen, mPending.oreId), now);
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
    spdlog::info("[OreMinerV2] real {} ({}, {}, {}) got '{}' in {} ms",
                 oreTypeName(pc.oreId), pc.pos.x, pc.pos.y, pc.pos.z,
                 formatGains(pc.seen), waitMs);
  else
    spdlog::warn("[OreMinerV2] {} {} ({}, {}, {}) got '{}' in {} ms{}",
                 verdictName(v), oreTypeName(pc.oreId), pc.pos.x, pc.pos.y,
                 pc.pos.z, formatGains(pc.seen), waitMs,
                 pc.invFull ? " [inventory full]" : "");

  if (mDebug.mValue)
    debugWriteVerdict(pc, v, now);
}

void OreMinerV2::finishCheck(DropVerdict v, uint64_t now) {
  const PendingCheck pc = std::move(mPending);
  mPending = PendingCheck{};
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

  if (!pc.decides || v == DropVerdict::Real)
    return;

  // Первый блок кластера не дал нужного дропа — кластер фейковый: в чёрный
  // список целиком (как и раньше), остальные блоки не трогаем.
  mFakeClusters++;
  spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks)! Blacklisting.",
               mCurrentCluster.size());

  if (mDebug.mValue) {
    for (size_t i = 1; i < mCurrentCluster.size(); i++) {
      const BlockPos &cp = mCurrentCluster[i];
      auto it = mFoundBlocks.find(cp);
      const int id = (it != mFoundBlocks.end() && it->second.block &&
                      it->second.block->mLegacy)
                         ? it->second.block->mLegacy->getBlockId()
                         : pc.oreId;
      debugWriteSkip(cp, id, pc, static_cast<int>(i) + 1);
    }

    const std::string got = formatGains(pc.seen);
    ChatUtils::displayClientMessage(fmt::format(
        "§7[OreMinerV2] §fКластер §e#{}§f {} ({} бл.): §c{}§7 ({}) — пропущен",
        pc.clusterNo, oreTypeName(pc.oreId), pc.clusterSize,
        v == DropVerdict::Fake ? "FAKE" : "NO DROP",
        got.empty() ? std::string("ничего") : got));
  }

  blacklistPositions(mCurrentCluster);
  for (auto &p : mCurrentCluster)
    mMinedPositions.insert(p);

  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mState = MineState::Idle;
  mTickDelay = 2;
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
  mCurrentTarget = pos;
  return true;
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
    resetScan();
    mState = MineState::Idle;
    mCurrentCluster.clear();
    return;
  }

  auto source = ClientInstance::get()->getBlockSource();
  if (!source) return;

  // =================== KEYBIND CAPTURE ===================
  if (mIsBindingKey) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
      mIsBindingKey = false;
      mBindWaitRelease = false;
      return;
    }
    if (mBindWaitRelease) {
      if (!sIsAnyKeyHeld()) mBindWaitRelease = false;
      return;
    }
    int pressed = sFindHeldKey();
    if (pressed > 0) {
      mQuickTPKey.mValue = (float)pressed;
      mIsBindingKey = false;
      mBindWaitRelease = false;
      ChatUtils::displayClientMessage(
          std::string("§a[QuickTP] Key set to: §f") + getKeyName(pressed));
    }
    return;
  }

  // =================== QUICK TP KEYBIND ===================
  {
    int vk = static_cast<int>(mQuickTPKey.mValue);
    if (vk > 0) {
      bool down = GetAsyncKeyState(vk) & 0x8000;
      if (down && !mQuickTPKeyWasDown &&
          !ClientInstance::get()->getMouseGrabbed()) {
        auto rot = player->getActorRotationComponent();
        if (rot) {
          float yaw = rot->mYaw * (3.14159265f / 180.f);
          glm::vec3 from = *player->getPos();
          glm::vec3 dir = glm::vec3(-sinf(yaw), 0.f, cosf(yaw));
          glm::vec3 to = from + dir * mQuickTPDist.mValue;

          straightLineTP(from, to, true);
          player->setPosition(to);
          auto sv = player->getStateVectorComponent();
          if (sv) sv->mVelocity = glm::vec3(0.f);

          ChatUtils::displayClientMessage(
              "§a[QuickTP] Teleported " +
              std::to_string((int)mQuickTPDist.mValue) + " blocks forward!");
        }
      }
      mQuickTPKeyWasDown = down;
    }
  }

  // =================== SCANNING ===================
  {
    bool spiralDone =
        glm::distance(glm::vec2(mCurrentChunkPos), glm::vec2(mSearchCenter)) >
        mChunkRadius.mValue;

    if (spiralDone) {
      if (!mScanComplete) {
        mScanComplete = true;
        if (mScanDirty) {
          updateClusters();
          mLastClusterUpdate = NOW;
          mScanDirty = false;
        }
      }
      mSearchCenter = ChunkPos(*player->getPos());
      mCurrentChunkPos = mSearchCenter;
      mStepsCount = 0;
      mSteps = 1;
      mDirectionIndex = 0;
      mSubChunkIndex = 0;
    }

    for (int i = 0; i < static_cast<int>(mChunkUpdatesPerTick.mValue); i++) {
      bool ok = false;
      tryProcessSub(ok, mCurrentChunkPos, mSubChunkIndex);
      moveToNext();
    }

    BlockPos bp = *player->getPos();
    int sc = (bp.y - source->getBuildDepth()) >> 4;
    bool r = false;
    tryProcessSub(r, ChunkPos(bp), sc);

    if (mScanDirty) {
      uint64_t now = NOW;
      if (now - mLastClusterUpdate > 2000) {
        updateClusters();
        mLastClusterUpdate = now;
        mScanDirty = false;
      }
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

  if (mTickDelay > 0) {
    mTickDelay--;
    return;
  }

  glm::vec3 playerPos = *player->getPos();

  switch (mState) {
  // ----- IDLE -----
  case MineState::Idle: {
    auto cluster = findNearestCluster(playerPos, mMinedPositions);
    if (cluster.empty()) return;

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
    startCheck(fp, seenId, true, 1, std::move(before));

    mState = MineState::Mining;
    mTickDelay = 1;
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
        startCheck(p, seenId, false, mCurrentClusterIdx + 1,
                   std::move(before));
    }
    mMinedPositions.insert(p); // помечаем в любом случае, чтобы не застрять
    mCurrentClusterIdx++;
    mTickDelay = static_cast<int>(mSpeed.mValue);

    // VeinMiner: after mining, BFS from this pos to find more same-type ore
    if (mVeinMiner.mValue) {
      auto cluster = getCluster(p, mFoundBlocks);
      for (auto &cp : cluster) {
        if (mMinedPositions.contains(cp)) continue;
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
}

// =============================================================
// PACKET IN — silent accept + dimension/respawn reset
// =============================================================

void OreMinerV2::onPacketInEvent(PacketInEvent &event) {
  if (mSilentAccept.mValue && event.mPacket->getId() == PacketID::MovePlayer) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player)
      return;
    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt->mPlayerID != player->getRuntimeID())
      return;
    event.cancel();
    ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
  }

  if (event.mPacket->getId() == PacketID::ChangeDimension) {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    abortCheck();
    mState = MineState::Idle;
    mCurrentCluster.clear();
    mCurrentClusterIdx = 0;
    mMinedPositions.clear();
    mPacketPositions.clear();
    mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
    resetScan();
  }

  if (event.mPacket->getId() == PacketID::PlayerAction) {
    auto pkt = event.getPacket<PlayerActionPacket>();
    if (pkt->mAction == PlayerActionType::Respawn) {
      std::lock_guard<std::recursive_mutex> lk(mMutex);
      abortCheck();
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
    float radSq = mRadius.mValue * mRadius.mValue;
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
        row.deepslate = isDeepslateOre(blockId);
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

    switch (mState) {
    case MineState::Idle:
      if (mFilteredBlocks.empty())
        return;
      status =
          "Scanning... [" + std::to_string(mFilteredBlocks.size()) + " ores]";
      break;
    case MineState::Mining:
      if (mPending.active && mPending.decides)
        status = "Checking...";
      else
        status = "Mining " + std::to_string(mCurrentClusterIdx) + "/" +
                 std::to_string(mCurrentCluster.size()) +
                 " | Fakes: " + std::to_string(mFakeClusters);
      break;
    }

    if (mDebug.mValue) {
      status += " | Real: " + std::to_string(mDbgReal) +
                " Fake: " + std::to_string(mDbgFake);
      if (mDbgNoDrop > 0)
        status += " NoDrop: " + std::to_string(mDbgNoDrop);
      if (mDumpSeen.mValue)
        status += " | Seen: " + std::to_string(mDebugRows);
    }

    ImVec2 ts = ImGui::CalcTextSize(status.c_str());
    drawList->AddRectFilled(ImVec2(cx - ts.x * 0.5f - 5.f, topY - 2.f),
                            ImVec2(cx + ts.x * 0.5f + 5.f, topY + ts.y + 2.f),
                            ImColor(0.f, 0.f, 0.f, 0.5f), 4.f);
    drawList->AddText(ImVec2(cx - ts.x * 0.5f, topY),
                      ImColor(1.f, 1.f, 1.f, alpha), status.c_str());
  }

    // ---------- BIND PANEL (слева снизу) ----------
    {
        ImVec2 ss   = ImGui::GetIO().DisplaySize;
        float  panX = 20.f;
        float  panY = ss.y - 80.f;

        std::string btnLabel;
        if (mIsBindingKey)
            btnLabel = mBindWaitRelease ? "Release keys..." : "Press key... (ESC = cancel)";
        else {
            int vk = (int)mQuickTPKey.mValue;
            btnLabel = std::string("QuickTP: [") + getKeyName(vk) + "]";
        }

        ImVec2 labelSize = ImGui::CalcTextSize(btnLabel.c_str());
        float  btnW      = labelSize.x + 24.f;
        float  btnH      = labelSize.y + 10.f;

        ImVec2 btnMin{ panX,        panY };
        ImVec2 btnMax{ panX + btnW, panY + btnH };

        ImVec2 mouse   = ImGui::GetIO().MousePos;
        bool   hovered = (mouse.x >= btnMin.x && mouse.x <= btnMax.x &&
                          mouse.y >= btnMin.y && mouse.y <= btnMax.y);
        bool   clicked = hovered && ImGui::GetIO().MouseClicked[0];

        ImU32 bgColor = mIsBindingKey
            ? IM_COL32(200, 100, 0, 220)
            : (hovered ? IM_COL32(70, 70, 110, 220) : IM_COL32(30, 30, 50, 180));

        drawList->AddRectFilled(btnMin, btnMax, bgColor, 6.f);
        drawList->AddRect(btnMin, btnMax,
            mIsBindingKey ? IM_COL32(255, 150, 50, 255) : IM_COL32(100, 100, 160, 200),
            6.f, 0, 1.5f);
        drawList->AddText(
            { panX + (btnW - labelSize.x) * 0.5f, panY + (btnH - labelSize.y) * 0.5f },
            IM_COL32(255, 255, 255, 255),
            btnLabel.c_str()
        );

        if (clicked && !mIsBindingKey) {
            mIsBindingKey    = true;
            mBindWaitRelease = true;
        }
    }
}