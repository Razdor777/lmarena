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
#include <filesystem>
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
// BLOCK HELPERS
// =============================================================

bool OreMinerV2::isAirOrLiquid(const Block *block) {
  if (!block || !block->mLegacy)
    return true;
  const int id = block->mLegacy->getBlockId();
  if (id >= 8 && id <= 11) // flowing_water / water / flowing_lava / lava
    return true;
  return block->mLegacy->isAir();
}

std::string OreMinerV2::blockNameOf(const Block *block) {
  if (!block || !block->mLegacy)
    return "unknown";
  std::string name = block->mLegacy->mName;
  if (name.rfind("minecraft:", 0) == 0)
    name.erase(0, 10);
  if (name.empty())
    name = "id_" + std::to_string(block->mLegacy->getBlockId());
  std::replace(name.begin(), name.end(), ',', ' '); // чтобы не ломать CSV
  return name;
}

// =============================================================
// DEBUG DUMP — сохраняем только то, что проверено по дропу
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

const char *OreMinerV2::verdictName(DropVerdict verdict) {
  switch (verdict) {
  case DropVerdict::Real:
    return "REAL";
  case DropVerdict::Fake:
    return "FAKE";
  case DropVerdict::NoDrop:
    return "NODROP";
  case DropVerdict::Skipped:
    return "SKIPPED";
  }
  return "UNKNOWN";
}

std::string OreMinerV2::localTimeString(const char *fmt) {
  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char buf[64] = {};
  std::strftime(buf, sizeof(buf), fmt, &tmBuf);
  return std::string(buf);
}

// Галочку Debug можно щёлкать на ходу: включил — открылись новые файлы,
// выключил — файлы закрылись и в чат упал итог.
void OreMinerV2::syncDebugState() {
  const bool want = mDebug.mValue;
  if (want && !mDebugWasOn)
    debugStart();
  else if (!want && mDebugWasOn)
    debugStop();
  mDebugWasOn = want;
}

// Открывает новый комплект файлов. В имени дата и время, поэтому старые дампы
// никогда не затираются: каждый запуск Debug — отдельные чистые файлы.
void OreMinerV2::debugStart() {
  debugStop(); // на всякий случай закрыть хвосты прошлой сессии

  const std::string dir = FileUtils::getJuzdexDir() + "Dumps\\";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);

  std::string base =
      dir + "OreMinerV2_" + localTimeString("%Y-%m-%d_%H-%M-%S");
  // Debug щёлкнули дважды за одну секунду — не перезаписываем прошлый дамп
  if (std::filesystem::exists(base + "_verified.csv", ec)) {
    int n = 2;
    while (std::filesystem::exists(
        base + "_" + std::to_string(n) + "_verified.csv", ec))
      n++;
    base += "_" + std::to_string(n);
  }

  mDebugCsvPath = base + "_verified.csv";
  mDebugRealPath = base + "_real.txt";
  mDebugFakePath = base + "_fake.txt";

  mDebugCsv.open(mDebugCsvPath, std::ios::out | std::ios::trunc);
  mDebugReal.open(mDebugRealPath, std::ios::out | std::ios::trunc);
  mDebugFake.open(mDebugFakePath, std::ios::out | std::ios::trunc);

  mDebugWritten.clear();
  mDebugRealCount = 0;
  mDebugFakeCount = 0;
  mDebugNoDropCount = 0;
  mDebugSkippedCount = 0;

  if (!mDebugCsv.is_open()) {
    spdlog::error("[OreMinerV2][Debug] cannot open {}", mDebugCsvPath);
    ChatUtils::displayClientMessage(
        std::string("§c[OreMinerV2] Debug: cannot create dump file"));
    return;
  }

  // time      — когда проверили (локальное время)
  // verdict   — REAL / FAKE / NODROP / SKIPPED
  // reason    — drop | revealed | rejected | no_drop | inventory_full | fake_cluster
  // details   — что реально выпало ("diamond x2", "cobblestone x1")
  //             или каким блоком сервер заменил руду (для revealed)
  // wait_ms   — сколько ждали дроп после ломания
  mDebugCsv << "time,x,y,z,ore,block_id,deepslate,verdict,reason,details,"
               "wait_ms,cluster,cluster_size,cluster_index\n";
  mDebugCsv.flush();

  const std::string fileName =
      mDebugCsvPath.substr(mDebugCsvPath.find_last_of("\\/") + 1);
  spdlog::info("[OreMinerV2][Debug] dump started -> {}", mDebugCsvPath);
  ChatUtils::displayClientMessage(
      "§b[OreMinerV2] §fDebug ON, checked ores -> §7Dumps\\" + fileName);
}

void OreMinerV2::debugStop() {
  const bool wasOpen = mDebugCsv.is_open();

  if (mDebugCsv.is_open()) {
    mDebugCsv.flush();
    mDebugCsv.close();
  }
  if (mDebugReal.is_open()) {
    mDebugReal.flush();
    mDebugReal.close();
  }
  if (mDebugFake.is_open()) {
    mDebugFake.flush();
    mDebugFake.close();
  }

  if (wasOpen) {
    spdlog::info("[OreMinerV2][Debug] saved: real {}, fake {}, nodrop {}, "
                 "skipped {} -> {}",
                 mDebugRealCount, mDebugFakeCount, mDebugNoDropCount,
                 mDebugSkippedCount, mDebugCsvPath);
    if (ClientInstance::get()->getLocalPlayer())
      ChatUtils::displayClientMessage(fmt::format(
          "§b[OreMinerV2] §fDebug saved: §aREAL {} §7| §cFAKE {} §7| "
          "§eNODROP {} §7| SKIPPED {}",
          mDebugRealCount, mDebugFakeCount, mDebugNoDropCount,
          mDebugSkippedCount));
  }

  mDebugWritten.clear();
}

// Одна проверенная координата -> строка в CSV + (если REAL/FAKE) строка в
// соответствующий txt. Пишем и сразу сбрасываем на диск, чтобы ничего не
// потерять, даже если игра вылетит.
void OreMinerV2::debugRecord(const BlockPos &pos, int oreId,
                             DropVerdict verdict, const char *reason,
                             const std::string &details, uint64_t waitMs,
                             int clusterIndex) {
  if (!mDebugCsv.is_open())
    return;
  if (!mDebugWritten.insert(pos).second)
    return; // эта координата уже записана

  // clusterIndex == 0 -> блок не из текущего кластера (сервер раскрыл фейк сам)
  const int clusterId = clusterIndex > 0 ? mClusterSerial : 0;
  const int clusterSize =
      clusterIndex > 0 ? static_cast<int>(mCurrentCluster.size()) : 0;

  mDebugCsv << localTimeString("%H:%M:%S") << ',' << pos.x << ',' << pos.y
            << ',' << pos.z << ',' << oreTypeName(oreId) << ',' << oreId
            << ',' << (isDeepslateOre(oreId) ? 1 : 0) << ','
            << verdictName(verdict) << ',' << reason << ',' << details << ','
            << waitMs << ',' << clusterId << ',' << clusterSize << ','
            << clusterIndex << '\n';
  mDebugCsv.flush();

  switch (verdict) {
  case DropVerdict::Real:
    mDebugRealCount++;
    if (mDebugReal.is_open()) {
      mDebugReal << pos.x << ' ' << pos.y << ' ' << pos.z << ' '
                 << oreTypeName(oreId) << '\n';
      mDebugReal.flush();
    }
    break;
  case DropVerdict::Fake:
    mDebugFakeCount++;
    if (mDebugFake.is_open()) {
      mDebugFake << pos.x << ' ' << pos.y << ' ' << pos.z << ' '
                 << oreTypeName(oreId) << '\n';
      mDebugFake.flush();
    }
    break;
  case DropVerdict::NoDrop:
    mDebugNoDropCount++;
    break;
  case DropVerdict::Skipped:
    mDebugSkippedCount++;
    break;
  }
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
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mNeedFirstVerify = false;
  mHasPending = false;
  mPending = PendingCheck{};
  mTickDelay = 0;
  mBlocksMined = 0;
  mFakeClusters = 0;
  mClusterSerial = 0;
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
  resetMining();
  mMinedPositions.clear();
  mPacketPositions.clear();
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mIsBindingKey = false;

  // Файлы дампа закрываем, как только модуль выключили: всё, что успело
  // накопиться, уже сброшено на диск построчно в debugRecord().
  if (mDebugWasOn)
    debugStop();
  mDebugWasOn = false;
}

// =============================================================
// BLOCK CHANGED — incremental update + детект фейков сервером
// =============================================================

void OreMinerV2::onBlockChangedEvent(BlockChangedEvent &event) {
  if (!ClientInstance::get()->getLevelRenderer())
    return;
  if (!event.mNewBlock || !event.mNewBlock->mLegacy)
    return;

  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  const BlockPos pos = event.mBlockPos;
  const int newId = event.mNewBlock->mLegacy->getBlockId();
  const int oldId = (event.mOldBlock && event.mOldBlock->mLegacy)
                        ? event.mOldBlock->mLegacy->getBlockId()
                        : 0;

  const bool isPendingPos = mHasPending && pos == mPending.pos;
  const bool wasOre = getOreType(oldId) != OreType::None;

  // Событие летит на КАЖДОЕ изменение блока в мире, поэтому имена/isAir
  // считаем только когда это нам реально нужно.
  if (isPendingPos || wasOre) {
    const bool newIsEmpty = isAirOrLiquid(event.mNewBlock);

    // 1) Блок, дроп которого мы сейчас ждём: запоминаем, что сервер поставил
    //    на его место. Вернул руду -> ломание не прошло; вернул камень и т.п.
    //    -> там изначально была не руда (фейк). Воздух -> блок сломан, ок.
    if (isPendingPos) {
      if (newIsEmpty) {
        mPending.restoredId = -1;
        mPending.restoredName.clear();
      } else {
        mPending.restoredId = newId;
        mPending.restoredName = blockNameOf(event.mNewBlock);
      }
    }

    // 2) Анти-xray сам раскрыл фейк: сканер видел тут руду, а сервер прислал
    //    камень/глубинный сланец/... (так бывает, когда ломаешь соседний
    //    блок). Это 100% фейк, причём без лишнего ломания.
    if (wasOre && !newIsEmpty && !isSameOreType(oldId, newId) &&
        mFoundBlocks.contains(pos)) {
      const std::string nowName = blockNameOf(event.mNewBlock);
      spdlog::info("[OreMinerV2] FAKE (revealed) {} at ({}, {}, {}) -> {}",
                   oreTypeName(oldId), pos.x, pos.y, pos.z, nowName);
      if (mDebug.mValue) {
        int clusterIndex = 0; // 0 = не из текущего кластера
        for (size_t i = 0; i < mCurrentCluster.size(); i++)
          if (mCurrentCluster[i] == pos) {
            clusterIndex = static_cast<int>(i) + 1;
            break;
          }
        debugRecord(pos, oldId, DropVerdict::Fake, "revealed", nowName, 0,
                    clusterIndex);
      }
    }
  }

  // 3) Обычное инкрементальное обновление найденных руд
  float dist = glm::distance(glm::vec3(pos) + glm::vec3(0.5f),
                             *player->getPos());
  if (dist > mRadius.mValue)
    return;

  if (mBlacklistedPositions.contains(pos))
    return;

  auto enabled = getEnabledBlockIds();

  if (std::ranges::find(enabled, newId) != enabled.end()) {
    mFoundBlocks[pos] = {event.mNewBlock,
                         AABB(pos, glm::vec3(1.f, 1.f, 1.f)),
                         getOreColor(newId)};
    mScanDirty = true;
  } else {
    removeBlockIncremental(pos);
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

std::unordered_map<std::string, int> OreMinerV2::getInventorySnapshot() {
  std::unordered_map<std::string, int> snap;
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
  }
  return snap;
}

int OreMinerV2::countFreeSlots() {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return 0;
  auto supplies = player->getSupplies();
  if (!supplies)
    return 0;
  auto container = supplies->getContainer();
  if (!container)
    return 0;

  int free = 0;
  for (int i = 0; i < 36; i++) {
    auto item = container->getItem(i);
    if (!item || !item->mItem || !item->getItem() || item->mCount <= 0)
      free++;
  }
  return free;
}

// =============================================================
// DROP CHECK — что реально прилетело в инвентарь после ломания
// =============================================================

// Только ПРИБАВКА предметов: инвентарь после минус инвентарь до.
// Износ кирки, съеденная еда и т.п. (убыль) сюда не попадают.
std::map<std::string, int>
OreMinerV2::diffInventory(const std::unordered_map<std::string, int> &before,
                          const std::unordered_map<std::string, int> &after) {
  std::map<std::string, int> gained;
  for (const auto &[name, cnt] : after) {
    auto it = before.find(name);
    const int prev = it == before.end() ? 0 : it->second;
    if (cnt > prev)
      gained[name] = cnt - prev;
  }
  return gained;
}

const char *OreMinerV2::expectedDropName(int oreId) {
  switch (getOreType(oreId)) {
  case OreType::Diamond:
    return "diamond";
  case OreType::Coal:
    return "coal";
  case OreType::Emerald:
    return "emerald";
  default:
    return nullptr;
  }
}

// Ищем ожидаемый предмет по подстроке без учёта регистра: "diamond",
// "minecraft:diamond", "diamond_ore" (шёлковое касание) — всё подходит.
bool OreMinerV2::hasExpectedDrop(const std::map<std::string, int> &gained,
                                 int oreId) {
  const char *want = expectedDropName(oreId);
  if (!want)
    return false;

  for (const auto &[name, cnt] : gained) {
    if (cnt <= 0)
      continue;
    std::string ln = name;
    std::transform(ln.begin(), ln.end(), ln.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (ln.find(want) != std::string::npos)
      return true;
  }
  return false;
}

// {"cobblestone": 1, "flint": 1} -> "cobblestone x1; flint x1"
std::string OreMinerV2::formatItems(const std::map<std::string, int> &items) {
  std::string out;
  for (const auto &[name, cnt] : items) {
    std::string n = name;
    if (n.rfind("minecraft:", 0) == 0)
      n.erase(0, 10);
    std::replace(n.begin(), n.end(), ',', ' '); // чтобы не ломать CSV
    if (!out.empty())
      out += "; ";
    out += n + " x" + std::to_string(cnt);
  }
  return out;
}

// Вызывается СРАЗУ после mineBlockAtPos(). `before` — снимок инвентаря,
// сделанный ДО отправки пакетов ломания.
void OreMinerV2::beginVerify(const BlockPos &pos, int oreId, int clusterIndex,
                             bool firstOfCluster,
                             std::unordered_map<std::string, int> before) {
  mPending = PendingCheck{};
  mPending.pos = pos;
  mPending.oreId = oreId;
  mPending.clusterIndex = clusterIndex;
  mPending.firstOfCluster = firstOfCluster;
  mPending.sentAt = NOW;
  mPending.before = std::move(before);
  mHasPending = true;

  mNeedFirstVerify = firstOfCluster;
  mState = MineState::Verifying;
}

// Каждый тик в состоянии Verifying. Ждём, пока плагин авто-подбора положит
// дроп в инвентарь (это минимум один пинг), а не фиксированные 1-2 тика,
// поэтому на высоком пинге настоящие кластеры больше не улетают в фейки.
bool OreMinerV2::tickVerify() {
  if (!mHasPending) {
    mNeedFirstVerify = false;
    mState = MineState::Mining;
    return true;
  }

  const uint64_t now = NOW;
  const uint64_t waited = now > mPending.sentAt ? now - mPending.sentAt : 0;
  auto gained = diffInventory(mPending.before, getInventorySnapshot());

  if (!gained.empty()) {
    // Дроп пришёл. Даём ещё один тик: если сервер разложил его по слотам
    // разными пакетами, хвост тоже попадёт в ЭТУ проверку, а не в проверку
    // следующего блока.
    if (!mPending.dropSeen) {
      mPending.dropSeen = true;
      mPending.gained = gained;
      return false;
    }
  } else if (mPending.dropSeen) {
    gained = mPending.gained; // дроп успели куда-то деть — берём то, что видели
  }

  if (!gained.empty()) {
    // Пришёл ожидаемый предмет -> руда настоящая (даже если вместе с ним
    // случайно подобралось что-то ещё). Иначе это фейк.
    const bool real = hasExpectedDrop(gained, mPending.oreId);
    return finishVerify(real ? DropVerdict::Real : DropVerdict::Fake, "drop",
                        formatItems(gained), waited);
  }

  if (waited < static_cast<uint64_t>(mDropTimeout.mValue))
    return false; // ещё ждём дроп

  // Дропа нет. Смотрим, что сервер сделал с блоком.
  if (mPending.restoredId >= 0) {
    if (!isSameOreType(mPending.restoredId, mPending.oreId))
      // Сервер поставил на место камень и т.п. — руды там и не было.
      return finishVerify(DropVerdict::Fake, "revealed", mPending.restoredName,
                          waited);
    // Сервер вернул руду обратно — ломание не прошло (анти-чит / лаг).
    return finishVerify(DropVerdict::NoDrop, "rejected", mPending.restoredName,
                        waited);
  }

  const bool full = countFreeSlots() == 0;
  if (full && now - mLastInvFullWarn > 10000) {
    mLastInvFullWarn = now;
    ChatUtils::displayClientMessage(
        std::string("§c[OreMinerV2] Inventory is full, drops can't be checked!"));
  }
  return finishVerify(DropVerdict::NoDrop, full ? "inventory_full" : "no_drop",
                      "", waited);
}

bool OreMinerV2::finishVerify(DropVerdict verdict, const char *reason,
                              std::string details, uint64_t waitedMs) {
  const PendingCheck done = std::move(mPending);
  mPending = PendingCheck{};
  mHasPending = false;
  mNeedFirstVerify = false;

  if (verdict == DropVerdict::Real)
    spdlog::info("[OreMinerV2] REAL {} at ({}, {}, {}) got [{}] in {} ms",
                 oreTypeName(done.oreId), done.pos.x, done.pos.y, done.pos.z,
                 details, waitedMs);
  else
    spdlog::warn("[OreMinerV2] {} {} at ({}, {}, {}) reason={} got [{}] in {} ms",
                 verdictName(verdict), oreTypeName(done.oreId), done.pos.x,
                 done.pos.y, done.pos.z, reason, details, waitedMs);

  if (mDebug.mValue)
    debugRecord(done.pos, done.oreId, verdict, reason, details, waitedMs,
                done.clusterIndex);

  // Дропа не было — а вдруг он просто опаздывает (пинг больше Drop Timeout)?
  // Тогда следующий блок ломаем только через ещё один Drop Timeout: опоздавший
  // дроп успеет упасть ДО снимка инвентаря следующего блока и не будет
  // засчитан ему (иначе фейк мог бы получить чужой алмаз и стать REAL).
  // Если сервер вернул блок на место (rejected) — дропа точно не будет.
  const int graceTicks =
      (verdict == DropVerdict::NoDrop && done.restoredId < 0)
          ? static_cast<int>(mDropTimeout.mValue / 50.f)
          : 0;

  // Первый блок кластера не настоящий -> весь кластер в чёрный список
  // (как и раньше). Остальные блоки не ломаем, в дамп они идут как SKIPPED.
  if (done.firstOfCluster && verdict != DropVerdict::Real) {
    mFakeClusters++;
    spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks)! Blacklisting.",
                 mCurrentCluster.size());

    if (mDebug.mValue) {
      for (size_t i = 0; i < mCurrentCluster.size(); i++) {
        const BlockPos &cp = mCurrentCluster[i];
        if (cp == done.pos)
          continue;
        auto fit = mFoundBlocks.find(cp);
        const int id = (fit != mFoundBlocks.end() && fit->second.block &&
                        fit->second.block->mLegacy)
                           ? fit->second.block->mLegacy->getBlockId()
                           : mCurrentClusterOreId;
        debugRecord(cp, id, DropVerdict::Skipped, "fake_cluster", "", 0,
                    static_cast<int>(i) + 1);
      }
    }

    blacklistPositions(mCurrentCluster);
    for (auto &p : mCurrentCluster)
      mMinedPositions.insert(p);
    finishCluster();
    mTickDelay = std::max(mTickDelay, graceTicks);
    return false;
  }

  mState = MineState::Mining;
  if (graceTicks > 0) {
    mTickDelay = std::max(mTickDelay, graceTicks);
    return false; // следующий блок — после паузы
  }
  return true;
}

// =============================================================
// MINE BLOCK — chain TP → destroy → chain TP back (optional)
// =============================================================

// Возвращает true, если пакеты ломания реально ушли на сервер.
bool OreMinerV2::mineBlockAtPos(const BlockPos &pos, Actor *player) {
  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return false;
  auto source = ClientInstance::get()->getBlockSource();
  if (!source) return false;

  Block *block = source->getBlock(pos);
  if (!block || !block->mLegacy || block->mLegacy->isAir()) return false;

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
// MINING STATE MACHINE — helpers
// =============================================================

void OreMinerV2::finishCluster() {
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mNeedFirstVerify = false;
  mState = MineState::Idle;
  mTickDelay = 2;
}

void OreMinerV2::resetMining() {
  mState = MineState::Idle;
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mNeedFirstVerify = false;
  mHasPending = false;
  mPending = PendingCheck{};
}

// ----- IDLE: берём ближайший кластер и ломаем его первый блок -----
void OreMinerV2::startNextCluster(Actor *player, BlockSource *source,
                                  glm::vec3 playerPos) {
  auto cluster = findNearestCluster(playerPos, mMinedPositions);
  if (cluster.empty())
    return;

  mCurrentCluster = cluster;
  mCurrentClusterIdx = 0;
  mNeedFirstVerify = false;

  // Запоминаем тип руды кластера по первому блоку
  auto it = mFilteredBlocks.find(mCurrentCluster[0]);
  mCurrentClusterOreId =
      (it != mFilteredBlocks.end() && it->second.block &&
       it->second.block->mLegacy)
          ? it->second.block->mLegacy->getBlockId()
          : 0;

  const BlockPos fp = mCurrentCluster[0];
  Block *fb = source->getBlock(fp);
  if (!fb || !fb->mLegacy || fb->mLegacy->isAir()) {
    mMinedPositions.insert(fp);
    mCurrentCluster.clear();
    return;
  }

  const int curId = fb->mLegacy->getBlockId();
  if (mCurrentClusterOreId == 0)
    mCurrentClusterOreId = curId;

  // Сервер уже показал, что тут не руда (камень и т.п.) — фейк, ломать
  // незачем. Выкидываем только эту позицию, остальной кластер пересоберётся.
  if (!isSameOreType(curId, mCurrentClusterOreId)) {
    spdlog::info("[OreMinerV2] FAKE (revealed) at ({}, {}, {}) -> {}", fp.x,
                 fp.y, fp.z, blockNameOf(fb));
    if (mDebug.mValue)
      debugRecord(fp, mCurrentClusterOreId, DropVerdict::Fake, "revealed",
                  blockNameOf(fb), 0, 0);
    mMinedPositions.insert(fp);
    blacklistPositions({fp});
    mCurrentCluster.clear();
    return;
  }

  mClusterSerial++;

  auto before = getInventorySnapshot(); // снимок ДО ломания
  const bool sent = mineBlockAtPos(fp, player);
  mMinedPositions.insert(fp);
  mCurrentClusterIdx = 1;

  if (!sent) {
    // Пакеты не ушли — проверить первый блок нечем, а ломать кластер без
    // проверки нельзя. Бросаем его, в следующий тик возьмём другой.
    finishCluster();
    return;
  }

  // Первый блок решает судьбу кластера: ждём его дроп (Verifying)
  beginVerify(fp, curId, 1, true, std::move(before));
  mTickDelay = 1; // дроп в тот же тик всё равно не придёт
}

// ----- MINING: ломаем следующий блок кластера (строго по порядку) -----
void OreMinerV2::mineNextInCluster(Actor *player, BlockSource *source) {
  while (mCurrentClusterIdx < static_cast<int>(mCurrentCluster.size())) {
    const BlockPos p = mCurrentCluster[mCurrentClusterIdx]; // копия: VeinMiner
                                                            // может дописать
                                                            // вектор
    const int clusterIndex = ++mCurrentClusterIdx; // номер блока (с 1)

    if (mMinedPositions.contains(p))
      continue;

    Block *b = source->getBlock(p);
    if (!b || !b->mLegacy || b->mLegacy->isAir()) {
      mMinedPositions.insert(p); // уже пусто — пропускаем
      continue;
    }

    const int curId = b->mLegacy->getBlockId();
    if (!isSameOreType(curId, mCurrentClusterOreId)) {
      // Анти-xray уже заменил «руду» на настоящий блок — это был фейк.
      // Не тратим на него ломание (иначе просто получим булыжник).
      spdlog::info("[OreMinerV2] FAKE (revealed) at ({}, {}, {}) -> {}", p.x,
                   p.y, p.z, blockNameOf(b));
      if (mDebug.mValue)
        debugRecord(p, mCurrentClusterOreId, DropVerdict::Fake, "revealed",
                    blockNameOf(b), 0, clusterIndex);
      mMinedPositions.insert(p);
      continue;
    }

    // Debug -> проверяем по дропу КАЖДЫЙ блок, а не только первый
    const bool verifyThis = mDebug.mValue;
    std::unordered_map<std::string, int> before;
    if (verifyThis)
      before = getInventorySnapshot();

    const bool sent = mineBlockAtPos(p, player);
    mMinedPositions.insert(p); // помечаем в любом случае, чтобы не застрять
    mTickDelay = static_cast<int>(mSpeed.mValue);

    if (sent && verifyThis)
      beginVerify(p, curId, clusterIndex, false, std::move(before));

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
    return; // один блок за раз
  }

  spdlog::info("[OreMinerV2] Cluster done! Mined: {}, Fakes: {}", mBlocksMined,
               mFakeClusters);
  finishCluster();
}

// =============================================================
// MAIN TICK — scan chunks + mining state machine
// =============================================================

void OreMinerV2::onBaseTickEvent(BaseTickEvent &event) {
  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto player = event.mActor;
  if (!player) return;

  // Галочка Debug -> открыть / закрыть файлы дампа
  syncDebugState();

  if (!ClientInstance::get()->getLevelRenderer()) {
    resetScan();
    resetMining();
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
  if (mTickDelay > 0) {
    mTickDelay--;
    return;
  }

  glm::vec3 playerPos = *player->getPos();

  switch (mState) {
  // ----- IDLE -----
  case MineState::Idle:
    startNextCluster(player, source, playerPos);
    break;

  // ----- VERIFYING: ждём дроп сломанного блока -----
  case MineState::Verifying:
    if (!tickVerify())
      break; // дроп ещё не пришёл, либо кластер оказался фейком
    // Проверка пройдена — следующий блок ломаем в этот же тик
    [[fallthrough]];

  // ----- MINING -----
  case MineState::Mining:
    mineNextInCluster(player, source);
    break;
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
    resetMining();
    mMinedPositions.clear();
    mPacketPositions.clear();
    resetScan();
  }

  if (event.mPacket->getId() == PacketID::PlayerAction) {
    auto pkt = event.getPacket<PlayerActionPacket>();
    if (pkt->mAction == PlayerActionType::Respawn) {
      std::lock_guard<std::recursive_mutex> lk(mMutex);
      resetMining();
      mMinedPositions.clear();
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

  // ---------- BLOCK ESP ----------
  // (Debug-дамп отсюда убран: теперь в файл пишутся только блоки, проверенные
  //  по дропу — это делается в тике, см. finishVerify / debugRecord.)
  if (mDrawBlocks.mValue) {
    glm::ivec3 pp = *player->getPos();
    float radSq = mRadius.mValue * mRadius.mValue;
    auto enVec = getEnabledBlockIds();
    std::unordered_set<int> enSet(enVec.begin(), enVec.end());

    int rendered = 0;
    for (auto &[pos, fb] : mFilteredBlocks) {
      if (rendered >= 500)
        break;
      if (!fb.block || !fb.block->mLegacy)
        continue;
      if (!enSet.contains(fb.block->mLegacy->getBlockId()))
        continue;

      glm::vec3 d = glm::vec3(pos) - glm::vec3(pp);
      if (d.x * d.x + d.y * d.y + d.z * d.z > radSq)
        continue;

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
      if (mFilteredBlocks.empty())
        return;
      status =
          "Scanning... [" + std::to_string(mFilteredBlocks.size()) + " ores]";
      break;
    case MineState::Mining:
    case MineState::Verifying:
      if (mNeedFirstVerify)
        status = "Checking...";
      else
        status = "Mining " + std::to_string(mCurrentClusterIdx) + "/" +
                 std::to_string(mCurrentCluster.size()) +
                 " | Fakes: " + std::to_string(mFakeClusters);
      break;
    }

    if (mDebug.mValue) {
      status += " | Real: " + std::to_string(mDebugRealCount) +
                "  Fake: " + std::to_string(mDebugFakeCount);
      if (mDebugNoDropCount > 0)
        status += "  NoDrop: " + std::to_string(mDebugNoDropCount);
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
