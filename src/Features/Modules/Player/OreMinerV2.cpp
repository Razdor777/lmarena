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
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string_view>

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

// Файлы дампа живут, пока стоит галочка Debug (и модуль включён). Зовётся и
// из тика (там пишутся вердикты), и из рендера (там пишется Dump Seen) —
// оба места держат mMutex.
void OreMinerV2::debugSync() {
  const bool want = mDebug.mValue;
  if (want && !mDebugWasOn)
    debugStart();
  else if (!want && mDebugWasOn)
    debugStop();
  mDebugWasOn = want;
}

// Открывает новый комплект файлов. Имя содержит дату и время, поэтому старые
// дампы никогда не затираются, а каждый запуск Debug — отдельный комплект:
//   OreMinerV2_<stamp>.csv       все проверенные координаты + вердикт
//   OreMinerV2_<stamp>_real.txt  только настоящие руды  ("x y z ore")
//   OreMinerV2_<stamp>_fake.txt  только фейки           ("x y z ore reason")
//   OreMinerV2_<stamp>_seen.csv  (Dump Seen) всё, что модуль видел
void OreMinerV2::debugStart() {
  const std::string dir = FileUtils::getJuzdexDir() + "Dumps\\";
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

  // Два запуска Debug в одну секунду не должны затирать друг друга.
  mDebugName = "OreMinerV2_" + std::string(stamp);
  try {
    for (int n = 2;
         n < 100 && FileUtils::fileExists(dir + mDebugName + ".csv"); n++)
      mDebugName = "OreMinerV2_" + std::string(stamp) + "_" + std::to_string(n);
  } catch (...) {
  }
  mDebugBase = dir + mDebugName;
  mDebugCsvPath = mDebugBase + ".csv";

  mDebugCsv.open(mDebugCsvPath, std::ios::out | std::ios::trunc);
  mDebugRealFile.open(mDebugBase + "_real.txt", std::ios::out | std::ios::trunc);
  mDebugFakeFile.open(mDebugBase + "_fake.txt", std::ios::out | std::ios::trunc);

  if (mDebugCsv.is_open()) {
    mDebugCsv << "time,world_x,world_y,world_z,ore,block_id,deepslate,verdict,"
                 "got,wait_ms,cluster,cluster_size,cluster_idx,head\n";
    mDebugCsv.flush();
  } else {
    spdlog::error("[OreMinerV2][Debug] cannot open {}", mDebugCsvPath);
  }

  mDebugWritten.clear();
  mDebugSeenTried = false;
  mDebugRows = 0;
  mDbgReal = mDbgFake = mDbgSkipped = mDbgUnknown = mDbgSeenRows = 0;
  mDebugStartedAt = NOW;

  spdlog::info("[OreMinerV2][Debug] dump started -> {}", mDebugCsvPath);
  ChatUtils::displayClientMessage(
      "§b[OreMinerV2] §7Debug: checking every ore, saving to §fDumps\\" +
      mDebugName + ".csv");
}

// Одна проверенная координата: строка в .csv и, по вердикту, в _real.txt или
// _fake.txt. Каждая строка сразу сбрасывается на диск — если игра вылетит,
// всё, что уже проверено, останется в файлах.
void OreMinerV2::debugRecord(const BlockPos &pos, int blockId, DropVerdict v,
                             const std::string &got, uint64_t waitMs,
                             int clusterId, int clusterSize, int clusterIdx,
                             bool head) {
  switch (v) {
  case DropVerdict::Real:
    mDbgReal++;
    break;
  case DropVerdict::Fake:
  case DropVerdict::NoDrop:
  case DropVerdict::Revealed:
    mDbgFake++;
    break;
  case DropVerdict::InvFull:
    mDbgUnknown++;
    break;
  case DropVerdict::Skipped:
    mDbgSkipped++;
    break;
  }

  if (!mDebugCsv.is_open())
    return;

  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char hms[16] = {};
  std::strftime(hms, sizeof(hms), "%H:%M:%S", &tmBuf);

  const char *ore = oreTypeName(blockId);

  mDebugCsv << hms << ',' << pos.x << ',' << pos.y << ',' << pos.z << ','
            << ore << ',' << blockId << ',' << (isDeepslateOre(blockId) ? 1 : 0)
            << ',' << verdictName(v) << ',' << got << ',' << waitMs << ','
            << clusterId << ',' << clusterSize << ',' << clusterIdx << ','
            << (head ? 1 : 0) << '\n';
  mDebugCsv.flush();
  mDebugRows++;

  if (v == DropVerdict::Real) {
    if (mDebugRealFile.is_open()) {
      mDebugRealFile << pos.x << ' ' << pos.y << ' ' << pos.z << ' ' << ore
                     << '\n';
      mDebugRealFile.flush();
    }
  } else if (v == DropVerdict::Fake || v == DropVerdict::NoDrop ||
             v == DropVerdict::Revealed) {
    if (mDebugFakeFile.is_open()) {
      std::string reason;
      if (v == DropVerdict::NoDrop)
        reason = "nodrop";
      else if (v == DropVerdict::Revealed)
        reason = "revealed:" + got;
      else
        reason = got; // что выпало вместо руды, напр. "cobblestone x1"
      mDebugFakeFile << pos.x << ' ' << pos.y << ' ' << pos.z << ' ' << ore
                     << ' ' << reason << '\n';
      mDebugFakeFile.flush();
    }
  }
}

// Dump Seen: старый дамп «всё, что модуль нашёл» (без проверки дропом).
void OreMinerV2::debugFlushSeen(const std::vector<SeenRow> &rows) {
  if (rows.empty() || mDebugBase.empty())
    return;

  if (!mDebugSeenFile.is_open()) {
    if (mDebugSeenTried)
      return; // уже пробовали и не смогли — не спамим ошибкой каждый кадр
    mDebugSeenTried = true;

    const std::string path = mDebugBase + "_seen.csv";
    mDebugSeenFile.open(path, std::ios::out | std::ios::trunc);
    if (!mDebugSeenFile.is_open()) {
      spdlog::error("[OreMinerV2][Debug] cannot open {}", path);
      return;
    }
    mDebugSeenFile << "world_x,world_y,world_z,ore,block_id,deepslate,distance,"
                      "on_screen,screen_x,screen_y,ms\n";
  }

  const uint64_t now = NOW;

  for (const auto &r : rows) {
    mDebugSeenFile << r.pos.x << ',' << r.pos.y << ',' << r.pos.z << ','
                   << oreTypeName(r.blockId) << ',' << r.blockId << ','
                   << (r.deepslate ? 1 : 0) << ',' << std::fixed
                   << std::setprecision(2) << r.distance << ','
                   << (r.onScreen ? 1 : 0) << ',' << std::setprecision(1)
                   << r.screenX << ',' << r.screenY << ',' << now << '\n';
  }

  mDebugSeenFile.flush();
  mDbgSeenRows += static_cast<int>(rows.size());
}

void OreMinerV2::debugStop() {
  const bool wasOpen = mDebugCsv.is_open();

  if (wasOpen) {
    mDebugCsv.flush();
    mDebugCsv.close();
  }
  if (mDebugRealFile.is_open())
    mDebugRealFile.close();
  if (mDebugFakeFile.is_open())
    mDebugFakeFile.close();
  if (mDebugSeenFile.is_open())
    mDebugSeenFile.close();

  if (wasOpen) {
    spdlog::info("[OreMinerV2][Debug] saved {} real / {} fake / {} skipped / "
                 "{} unknown ({} rows, {} seen) -> {}",
                 mDbgReal, mDbgFake, mDbgSkipped, mDbgUnknown, mDebugRows,
                 mDbgSeenRows, mDebugCsvPath);
    ChatUtils::displayClientMessage(
        "§b[OreMinerV2] §7Debug saved: §a" + std::to_string(mDbgReal) +
        " real§7, §c" + std::to_string(mDbgFake) + " fake§7 -> §fDumps\\" +
        mDebugName);
  }

  mDebugWritten.clear();
  mDebugSeenTried = false;
  mDebugRows = 0;
  mDbgReal = mDbgFake = mDbgSkipped = mDbgUnknown = mDbgSeenRows = 0;
  mDebugStartedAt = 0;
}

// =============================================================
// ENABLE / DISABLE
// =============================================================

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
  mCurrentClusterId = 0;
  mClusterSeq = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mTickDelay = 0;
  mBlocksMined = 0;
  mFakeClusters = 0;
  mMinedPositions.clear();
  mLastInvFullWarn = 0;
  mQuickTPKeyWasDown = false;
  mIsBindingKey = false;
  mBindWaitRelease = false;

  {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    abortDropCheck();
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
  // Удар, по которому ещё не пришёл дроп, просто забываем: без дропа
  // вердикт вынести не по чему.
  abortDropCheck();
  mState = MineState::Idle;
  mCurrentCluster.clear();
  mMinedPositions.clear();
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mPacketPositions.clear();
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mIsBindingKey = false;

  // Файлы дампа закрываем, как только модуль выключили: каждая строка и так
  // уже сброшена на диск в момент записи.
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
// DROP CHECK — настоящая руда или фейк, по тому, что упало в инвентарь
// =============================================================
//
// Сервер (плагин авто-подбора) кладёт дроп сразу в инвентарь. Поэтому перед
// ударом снимаем инвентарь, а потом каждый тик смотрим, что прибавилось:
//   * прибавился ожидаемый предмет (алмаз / уголь / изумруд, или сама руда
//     при шёлковом касании)                          -> REAL;
//   * прибавилось что-то другое (булыжник, сланец...) -> FAKE;
//   * за Drop Timeout не прибавилось ничего           -> NODROP
//     (или INVFULL, если места под дроп в инвентаре и не было).
// Ждём не фиксированный тик, а реальный приход дропа, поэтому вердикт не
// зависит от пинга.

std::string OreMinerV2::normalizeItemName(const std::string &name) {
  std::string s = name;
  const size_t colon = s.rfind(':');
  if (colon != std::string::npos)
    s.erase(0, colon + 1); // "minecraft:diamond" -> "diamond"
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

const char *OreMinerV2::expectedDropKey(int oreId) {
  switch (getOreType(oreId)) {
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

// "diamond" (обычный дроп, с удачей — несколько штук), "diamond_ore" /
// "deepslate_diamond_ore" (шёлковое касание), "diamond_block" (плагины
// авто-сжатия) — всё это настоящая руда.
bool OreMinerV2::isExpectedDrop(const std::string &itemName, int oreId) {
  const char *key = expectedDropKey(oreId);
  if (!key || itemName.find(key) == std::string::npos)
    return false;
  // Алмазная кирка/броня тоже содержит "diamond", но с руды она не падает.
  // И не стакается: иначе неполный «стак» кирки (1 < 64) выглядел бы как
  // место под алмаз, и забитый инвентарь давал бы NODROP вместо INVFULL.
  static constexpr std::string_view kGear[] = {
      "_pickaxe", "_axe",        "_shovel",   "_hoe",   "_sword",
      "_helmet",  "_chestplate", "_leggings", "_boots", "_horse_armor"};
  for (std::string_view suffix : kGear)
    if (itemName.ends_with(suffix))
      return false;
  return true;
}

bool OreMinerV2::hasRoomForDrop(const InvSnapshot &snap, int oreId) {
  if (!snap.valid || snap.freeSlots > 0)
    return true;
  for (const auto &name : snap.partial)
    if (isExpectedDrop(name, oreId))
      return true;
  return false;
}

const char *OreMinerV2::verdictName(DropVerdict v) {
  switch (v) {
  case DropVerdict::Real:
    return "REAL";
  case DropVerdict::Fake:
    return "FAKE";
  case DropVerdict::NoDrop:
    return "NODROP";
  case DropVerdict::InvFull:
    return "INVFULL";
  case DropVerdict::Revealed:
    return "REVEALED";
  case DropVerdict::Skipped:
    return "SKIPPED";
  }
  return "?";
}

OreMinerV2::InvSnapshot OreMinerV2::takeInventorySnapshot() {
  InvSnapshot snap;
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
    if (!item || !item->mItem || !item->getItem() || item->mCount <= 0) {
      snap.freeSlots++;
      continue;
    }
    const std::string name = normalizeItemName(item->getItem()->mName);
    snap.counts[name] += item->mCount;
    if (item->mCount < 64)
      snap.partial.insert(name);
  }
  snap.valid = true;
  return snap;
}

void OreMinerV2::beginDropCheck(const BlockPos &pos, int blockId, bool head,
                                int clusterIdx, InvSnapshot &&before) {
  mCheck = DropCheck{};
  if (!before.valid)
    return; // инвентарь не прочитался — сравнивать не с чем

  mCheck.active = true;
  mCheck.pos = pos;
  mCheck.blockId = blockId;
  mCheck.head = head;
  mCheck.clusterId = mCurrentClusterId;
  mCheck.clusterSize = static_cast<int>(mCurrentCluster.size());
  mCheck.clusterIdx = clusterIdx;
  mCheck.sentAt = NOW;
  mCheck.before = std::move(before);
  mNeedFirstVerify = head;
}

void OreMinerV2::abortDropCheck() {
  mCheck = DropCheck{};
  mNeedFirstVerify = false;
}

void OreMinerV2::pollDropCheck() {
  if (!mCheck.active)
    return;

  const uint64_t now = NOW;
  const uint64_t waited = now - mCheck.sentAt;
  const uint64_t timeout = static_cast<uint64_t>(mDropTimeout.mValue);

  InvSnapshot cur = takeInventorySnapshot();

  // Что прибавилось с момента удара (убыль — инструмент, еда и т.п. — не важна).
  std::vector<std::pair<std::string, int>> gained;
  bool gotExpected = false;
  if (cur.valid) {
    for (const auto &[name, cnt] : cur.counts) {
      auto it = mCheck.before.counts.find(name);
      const int prev = it == mCheck.before.counts.end() ? 0 : it->second;
      if (cnt <= prev)
        continue;
      gained.emplace_back(name, cnt - prev);
      if (isExpectedDrop(name, mCheck.blockId))
        gotExpected = true;
    }
  }

  if (!gained.empty()) {
    // "cobblestone x1 + dirt x2" — без запятых, это одна ячейка CSV.
    std::sort(gained.begin(), gained.end());
    std::string s;
    for (const auto &[name, n] : gained) {
      if (!s.empty())
        s += " + ";
      s += name + " x" + std::to_string(n);
    }
    std::replace(s.begin(), s.end(), ',', ' ');
    mCheck.got = std::move(s);
  }

  if (gotExpected) {
    finishDropCheck(DropVerdict::Real, mCheck.got, waited);
    return;
  }

  if (!gained.empty() && mCheck.firstGainAt == 0)
    mCheck.firstGainAt = now;

  if (mCheck.firstGainAt != 0) {
    // Пришло что-то, но не руда. Даём ещё FAKE_SETTLE_MS: вдруг руда идёт
    // отдельным пакетом следом.
    if (now - mCheck.firstGainAt >= FAKE_SETTLE_MS || waited >= timeout)
      finishDropCheck(DropVerdict::Fake, mCheck.got,
                      mCheck.firstGainAt - mCheck.sentAt);
    return;
  }

  if (waited >= timeout) {
    const bool room = hasRoomForDrop(mCheck.before, mCheck.blockId);
    finishDropCheck(room ? DropVerdict::NoDrop : DropVerdict::InvFull, "-",
                    waited);
  }
}

void OreMinerV2::finishDropCheck(DropVerdict v, std::string got,
                                 uint64_t waitMs) {
  const DropCheck c = std::move(mCheck);
  mCheck = DropCheck{};
  if (got.empty())
    got = "-";

  const char *ore = oreTypeName(c.blockId);
  if (v == DropVerdict::Real)
    spdlog::info("[OreMinerV2] REAL {} at {} {} {} (got {}, {} ms)", ore,
                 c.pos.x, c.pos.y, c.pos.z, got, waitMs);
  else
    spdlog::warn("[OreMinerV2] {} {} at {} {} {} (got {}, {} ms)",
                 verdictName(v), ore, c.pos.x, c.pos.y, c.pos.z, got, waitMs);

  if (mDebug.mValue)
    debugRecord(c.pos, c.blockId, v, got, waitMs, c.clusterId, c.clusterSize,
                c.clusterIdx, c.head);

  if (v == DropVerdict::InvFull) {
    const uint64_t now = NOW;
    if (now - mLastInvFullWarn > 10000) {
      mLastInvFullWarn = now;
      ChatUtils::displayClientMessage(
          "§b[OreMinerV2] §cInventory is full§7 - drops can't be checked, "
          "fakes are not skipped");
    }
  }

  if (!c.head)
    return;
  mNeedFirstVerify = false;

  // Real — копаем кластер дальше; InvFull — по инвентарю ничего не понять,
  // кластер не трогаем.
  if (v != DropVerdict::Fake && v != DropVerdict::NoDrop)
    return;

  // Первый блок кластера оказался фейком -> весь кластер в чёрный список.
  mFakeClusters++;
  spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks)! Blacklisting.",
               mCurrentCluster.size());

  if (mDebug.mValue) {
    // Остальные блоки кластера не копались — в дамп они идут как SKIPPED,
    // в _real.txt / _fake.txt не попадают (их никто не проверял).
    for (size_t i = 0; i < mCurrentCluster.size(); i++) {
      const BlockPos sp = mCurrentCluster[i];
      if (sp == c.pos || mMinedPositions.contains(sp))
        continue;
      int id = c.blockId;
      auto it = mFoundBlocks.find(sp);
      if (it != mFoundBlocks.end() && it->second.block &&
          it->second.block->mLegacy)
        id = it->second.block->mLegacy->getBlockId();
      debugRecord(sp, id, DropVerdict::Skipped, "head_fake", 0, c.clusterId,
                  c.clusterSize, static_cast<int>(i), false);
    }
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

  debugSync();

  if (!ClientInstance::get()->getLevelRenderer()) {
    resetScan();
    abortDropCheck();
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
  // Проверка дропа с прошлого удара идёт каждый тик, даже пока тикает
  // задержка Speed, — так вердикт выносится сразу, как только дроп пришёл.
  if (mCheck.active)
    pollDropCheck(); // может сам закрыть кластер (первый блок — фейк)

  if (mTickDelay > 0) {
    mTickDelay--;
    return;
  }

  // Пока не ясно, что выпало с прошлого проверяемого блока, следующий не
  // ломаем — иначе дропы перемешаются и не понять, какая координата что дала.
  if (mCheck.active)
    return;

  glm::vec3 playerPos = *player->getPos();

  switch (mState) {
  // ----- IDLE -----
  case MineState::Idle: {
    auto cluster = findNearestCluster(playerPos, mMinedPositions);
    if (cluster.empty()) return;

    mCurrentCluster = cluster;
    mCurrentClusterIdx = 0;
    mCurrentClusterId = ++mClusterSeq;
    mNeedFirstVerify = false;

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

    const int firstId = fb->mLegacy->getBlockId();
    if (mCurrentClusterOreId == 0)
      mCurrentClusterOreId = firstId;
    if (!isSameOreType(firstId, mCurrentClusterOreId)) {
      // Клиент уже видит тут не ту руду (или вообще не руду) — сервер успел
      // прислать настоящий блок. Значит, показанная руда была фейком.
      if (mDebug.mValue) {
        std::string real = normalizeItemName(fb->mLegacy->mName);
        if (real.empty()) real = "id" + std::to_string(firstId);
        debugRecord(fp, mCurrentClusterOreId, DropVerdict::Revealed, real, 0,
                    mCurrentClusterId,
                    static_cast<int>(mCurrentCluster.size()), 0, true);
      }
      removeBlockIncremental(fp);
      mMinedPositions.insert(fp);
      mCurrentCluster.clear();
      return;
    }

    // Инвентарь ДО удара — потом сравним, что прибавилось.
    InvSnapshot before = takeInventorySnapshot();
    const bool sent = mineBlockAtPos(fp, player);
    mMinedPositions.insert(fp);
    if (!sent) {
      mCurrentCluster.clear();
      return;
    }

    // Первый блок решает судьбу кластера: пока по нему нет вердикта, дальше
    // не копаем (см. pollDropCheck / finishDropCheck).
    beginDropCheck(fp, firstId, true, 0, std::move(before));
    mCurrentClusterIdx = 1;
    mState = MineState::Mining;
    mTickDelay = static_cast<int>(mSpeed.mValue);
    break;
  }

  // ----- MINING -----
  case MineState::Mining: {
    // Сюда попадаем, только когда первый блок уже признан настоящим (иначе
    // finishDropCheck увёл кластер в чёрный список и вернул нас в Idle).
    if (mCurrentClusterIdx >= static_cast<int>(mCurrentCluster.size())) {
      spdlog::info("[OreMinerV2] Cluster done! Mined: {}, Fakes: {}",
                   mBlocksMined, mFakeClusters);
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      mState = MineState::Idle;
      mTickDelay = 2;
      return;
    }

    // Копия, а не ссылка: VeinMiner ниже дописывает в mCurrentCluster.
    const int idx = mCurrentClusterIdx++;
    const BlockPos p = mCurrentCluster[idx];
    if (mMinedPositions.contains(p))
      break;
    mMinedPositions.insert(p); // помечаем в любом случае, чтобы не застрять

    Block *b = source->getBlock(p);
    if (!b || b->mLegacy->isAir())
      break; // уже сломан

    const int bid = b->mLegacy->getBlockId();
    if (!isSameOreType(bid, mCurrentClusterOreId)) {
      // Пока ломали соседей, сервер (анти-xray) прислал настоящий блок: тут
      // не руда, а камень / сланец / ... — это был фейк, ломать нечего.
      removeBlockIncremental(p);
      if (mDebug.mValue) {
        std::string real = normalizeItemName(b->mLegacy->mName);
        if (real.empty()) real = "id" + std::to_string(bid);
        debugRecord(p, mCurrentClusterOreId, DropVerdict::Revealed, real, 0,
                    mCurrentClusterId,
                    static_cast<int>(mCurrentCluster.size()), idx, false);
      }
      break;
    }

    // Debug: проверяем по дропу КАЖДЫЙ блок, чтобы у каждой координаты был
    // свой вердикт. Без Debug — как раньше, проверяется только первый.
    const bool verify = mDebug.mValue;
    InvSnapshot before;
    if (verify)
      before = takeInventorySnapshot();
    if (mineBlockAtPos(p, player) && verify)
      beginDropCheck(p, bid, false, idx, std::move(before));
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
    abortDropCheck();
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
      abortDropCheck();
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
  // Проверенные (REAL/FAKE) координаты пишутся из тика, в момент вердикта.
  // Здесь — только необязательный Dump Seen: всё, что модуль считает
  // найденным (радиус + тип руды), без проверки, каждая координата один раз.
  debugSync();
  const bool wantEsp = mDrawBlocks.mValue;
  const bool wantDump = mDebug.mValue && mDumpSeen.mValue;

  if (wantEsp || wantDump) {
    glm::ivec3 pp = *player->getPos();
    float radSq = mRadius.mValue * mRadius.mValue;
    auto enVec = getEnabledBlockIds();
    std::unordered_set<int> enSet(enVec.begin(), enVec.end());

    std::vector<SeenRow> dumpRows;
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
        SeenRow row;
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
      debugFlushSeen(dumpRows);
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
    ImColor cc = mNeedFirstVerify
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
      if (mFilteredBlocks.empty()) {
        if (!mDebug.mValue)
          return;
        status = "Idle"; // с Debug счётчик сохранённого виден всегда
      } else {
        status =
            "Scanning... [" + std::to_string(mFilteredBlocks.size()) + " ores]";
      }
      break;
    case MineState::Mining:
      if (mNeedFirstVerify)
        status = "Checking...";
      else
        status = "Mining " + std::to_string(mCurrentClusterIdx) + "/" +
                 std::to_string(mCurrentCluster.size()) +
                 " | Fakes: " + std::to_string(mFakeClusters);
      break;
    }

    if (mDebug.mValue)
      status += " | Saved: " + std::to_string(mDbgReal) + " real / " +
                std::to_string(mDbgFake) + " fake";

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