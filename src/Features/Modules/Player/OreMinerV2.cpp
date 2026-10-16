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
// DEBUG DUMP — только проверенные дропом блоки
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

// Что должно прийти в инвентарь с НАСТОЯЩЕЙ руды. Ищется подстрокой, поэтому
// ловит и саму руду при шёлковом касании ("diamond_ore",
// "deepslate_diamond_ore"), а удача (несколько алмазов) просто даёт xN.
const char *OreMinerV2::expectedDropName(int blockId) {
  switch (getOreType(blockId)) {
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

const char *OreMinerV2::verdictName(DropVerdict v) {
  switch (v) {
  case DropVerdict::Real:
    return "real";
  case DropVerdict::Fake:
    return "fake";
  case DropVerdict::NoDrop:
    return "no_drop";
  case DropVerdict::InvFull:
    return "inv_full";
  case DropVerdict::Revealed:
    return "revealed";
  case DropVerdict::Skipped:
    return "skipped";
  default:
    return "pending";
  }
}

// "minecraft:Cobbled_Deepslate" -> "cobbled_deepslate"; всё, что может
// сломать CSV (запятые, переводы строк, мусор), заменяется на '_'.
std::string OreMinerV2::cleanName(std::string name) {
  if (const auto colon = name.find(':'); colon != std::string::npos)
    name.erase(0, colon + 1);
  for (char &c : name) {
    const auto uc = static_cast<unsigned char>(c);
    if (uc >= 'A' && uc <= 'Z')
      c = static_cast<char>(uc - 'A' + 'a');
    else if (uc < 0x21 || uc > 0x7E || c == ',' || c == '"')
      c = '_';
  }
  return name.empty() ? std::string("?") : name;
}

// Debug включили/выключили (вызывается каждый тик под mMutex).
void OreMinerV2::debugSync() {
  const bool on = mDebug.mValue;
  if (on == mDebugWasOn)
    return;
  mDebugWasOn = on;

  if (!on) {
    debugClose();
    return;
  }

  // Новая сессия дампа. Сами файлы создаются на первом вердикте — чтобы
  // простое щёлканье галочкой не плодило пустые дампы.
  mDebugWritten.clear();
  mDebugOpenFailed = false;
  mDebugReal = mDebugFake = mDebugNoDrop = mDebugRevealed = mDebugSkipped = 0;
  ChatUtils::displayClientMessage(
      "§7[OreMinerV2] §fDebug ON§7: every mined ore is checked by its drop");
}

// Новый набор файлов. Имя содержит дату и время, поэтому старые дампы никогда
// не затираются, а каждая сессия Debug — отдельные чистые файлы.
bool OreMinerV2::debugOpen() {
  if (mDebugCsv.is_open())
    return true;
  if (mDebugOpenFailed)
    return false;

  const std::string dir = FileUtils::getJuzdexDir() + "Dumps\\";
  try {
    FileUtils::createDirectory(dir);
  } catch (...) {
    // не вышло создать папку — open() ниже скажет об этом сам
  }

  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char stamp[32] = {};
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &tmBuf);

  const std::string name = "OreMinerV2_" + std::string(stamp);
  mDebugBasePath = dir + name;

  mDebugCsv.open(mDebugBasePath + ".csv", std::ios::out | std::ios::trunc);
  if (!mDebugCsv.is_open()) {
    mDebugOpenFailed = true;
    spdlog::error("[OreMinerV2][Debug] cannot open {}.csv", mDebugBasePath);
    ChatUtils::displayClientMessage("§c[OreMinerV2] Debug: cannot create " +
                                    mDebugBasePath + ".csv");
    return false;
  }
  mDebugRealTxt.open(mDebugBasePath + "_real.txt",
                     std::ios::out | std::ios::trunc);
  mDebugFakeTxt.open(mDebugBasePath + "_fake.txt",
                     std::ios::out | std::ios::trunc);

  mDebugCsv << "time,verdict,x,y,z,ore,block_id,deepslate,got,wait_ms,"
               "cluster,cluster_idx,cluster_size\n";
  mDebugCsv.flush();

  spdlog::info("[OreMinerV2][Debug] dump started -> {}.csv", mDebugBasePath);
  ChatUtils::displayClientMessage("§7[OreMinerV2] Debug dump -> §fDumps\\" +
                                  name + ".csv");
  return true;
}

void OreMinerV2::debugWrite(const DebugRow &r) {
  // Одна строка на координату; «skipped» можно уточнить реальным вердиктом.
  const auto prev = mDebugWritten.find(r.pos);
  const bool upgrade = prev != mDebugWritten.end() &&
                       prev->second == DropVerdict::Skipped &&
                       r.verdict != DropVerdict::Skipped;
  if (prev != mDebugWritten.end() && !upgrade)
    return;
  if (!debugOpen())
    return;
  if (upgrade)
    mDebugSkipped--;
  mDebugWritten[r.pos] = r.verdict;

  std::time_t t = std::time(nullptr);
  std::tm tmBuf{};
  localtime_s(&tmBuf, &t);
  char timeBuf[16] = {};
  std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);

  const char *ore = oreTypeName(r.blockId);

  mDebugCsv << timeBuf << ',' << verdictName(r.verdict) << ',' << r.pos.x << ','
            << r.pos.y << ',' << r.pos.z << ',' << ore << ',' << r.blockId
            << ',' << (isDeepslateOre(r.blockId) ? 1 : 0) << ',' << r.got
            << ',';
  if (r.waitMs >= 0)
    mDebugCsv << r.waitMs;
  mDebugCsv << ',' << r.clusterNo << ',' << r.clusterIdx << ','
            << r.clusterSize << '\n';
  mDebugCsv.flush();

  switch (r.verdict) {
  case DropVerdict::Real:
    mDebugReal++;
    break;
  case DropVerdict::NoDrop:
  case DropVerdict::InvFull:
    mDebugNoDrop++;
    break;
  case DropVerdict::Revealed:
    mDebugRevealed++;
    break;
  case DropVerdict::Skipped:
    mDebugSkipped++;
    break;
  default:
    break;
  }

  std::ofstream *list = nullptr;
  if (r.verdict == DropVerdict::Real)
    list = &mDebugRealTxt;
  else if (r.listAsFake) {
    list = &mDebugFakeTxt;
    mDebugFake++;
  }
  if (list && list->is_open()) {
    *list << r.pos.x << ' ' << r.pos.y << ' ' << r.pos.z << ' ' << ore << '\n';
    list->flush();
  }
}

void OreMinerV2::debugClose() {
  if (mDebugCsv.is_open()) {
    mDebugCsv.close();
    spdlog::info("[OreMinerV2][Debug] saved {}.csv: {} real, {} fake, {} no "
                 "drop, {} revealed, {} skipped",
                 mDebugBasePath, mDebugReal, mDebugFake, mDebugNoDrop,
                 mDebugRevealed, mDebugSkipped);
    if (ClientInstance::get()->getLocalPlayer())
      ChatUtils::displayClientMessage(
          "§a[OreMinerV2] Debug saved: §f" + std::to_string(mDebugReal) +
          " real§a, §c" + std::to_string(mDebugFake) + " fake§a, §7" +
          std::to_string(mDebugNoDrop) + " no drop");
  }
  if (mDebugRealTxt.is_open())
    mDebugRealTxt.close();
  if (mDebugFakeTxt.is_open())
    mDebugFakeTxt.close();
  mDebugWritten.clear();
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
  mNeedFirstVerify = false;
  mTickDelay = 0;
  mBlocksMined = 0;
  mFakeClusters = 0;
  mMinedPositions.clear();
  mQuickTPKeyWasDown = false;
  mIsBindingKey = false;
  mBindWaitRelease = false;

  {
    std::lock_guard<std::recursive_mutex> lk(mMutex);
    mPending = PendingCheck{};
    mClusterNo = 0;
    mInvFullPause = false;
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
  mCurrentCluster.clear();
  mMinedPositions.clear();
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mPacketPositions.clear();
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mIsBindingKey = false;

  // Дроп последнего сломанного блока может ещё лететь — непроверенное в дамп
  // не пишем. Всё уже проверенное сброшено на диск построчно в debugWrite().
  mPending = PendingCheck{};
  mNeedFirstVerify = false;
  mInvFullPause = false;
  if (mDebugWasOn)
    debugClose();
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
    if (!item || !item->mItem)
      continue;
    snap[item->getItem()->mName] += item->mCount;
  }
  return snap;
}

// Есть ли куда упасть дропу. Без свободного слота плагин авто-подбора не
// положит в инвентарь ни руду, ни булыжник — значит, и фейк не отличить.
bool OreMinerV2::hasFreeInventorySlot() {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return true; // не можем посмотреть — не блокируем
  auto supplies = player->getSupplies();
  if (!supplies)
    return true;
  auto container = supplies->getContainer();
  if (!container)
    return true;

  for (int i = 0; i < 36; i++) {
    auto item = container->getItem(i);
    if (!item || !item->mItem)
      return true;
  }
  return false;
}

void OreMinerV2::pauseForFullInventory() {
  if (mInvFullPause)
    return;
  mInvFullPause = true;
  spdlog::warn("[OreMinerV2] inventory full - mining paused");
  ChatUtils::displayClientMessage(
      "§c[OreMinerV2] Inventory full - mining paused until a slot is free "
      "(drops must land in inventory to tell real ore from fake)");
}

void OreMinerV2::resumeAfterFullInventory() {
  if (!mInvFullPause)
    return;
  mInvFullPause = false;
  spdlog::info("[OreMinerV2] inventory has space - mining resumed");
  ChatUtils::displayClientMessage(
      "§a[OreMinerV2] Inventory has space - mining resumed");
}

// =============================================================
// DROP CHECK — что прибавилось в инвентаре после слома
// =============================================================
//   прибавилась ожидаемая руда           -> Real  (настоящая)
//   прибавилось что-то другое            -> Fake  (фейк: булыжник, камень, ...)
//   не прибавилось ничего (пока)         -> Pending (решает таймаут)
// Если одновременно пришли и руда, и мусор — считаем настоящей: мусор мог
// прилететь случайно, а вот руда из ниоткуда не берётся.

OreMinerV2::DropCheck
OreMinerV2::classifyDrop(const std::unordered_map<std::string, int> &before,
                         const std::unordered_map<std::string, int> &after,
                         int oreBlockId) {
  DropCheck res;

  // Уменьшения (сломался инструмент и т.п.) не интересны — только прирост.
  std::vector<std::pair<std::string, int>> gained;
  for (const auto &[name, cnt] : after) {
    const auto it = before.find(name);
    const int prev = it == before.end() ? 0 : it->second;
    if (cnt > prev)
      gained.emplace_back(cleanName(name), cnt - prev);
  }
  if (gained.empty())
    return res; // дроп ещё не долетел

  std::sort(gained.begin(), gained.end());

  const char *expected = expectedDropName(oreBlockId);
  bool gotExpected = false;
  for (const auto &[name, count] : gained) {
    if (expected && name.find(expected) != std::string::npos)
      gotExpected = true;
    if (!res.got.empty())
      res.got += " + ";
    res.got += name + " x" + std::to_string(count);
  }

  res.verdict = gotExpected ? DropVerdict::Real : DropVerdict::Fake;
  return res;
}

void OreMinerV2::startPendingCheck(const BlockPos &pos, int blockId,
                                   bool probe, int clusterIdx,
                                   std::unordered_map<std::string, int> before) {
  mPending.active = true;
  mPending.probe = probe;
  mPending.pos = pos;
  mPending.blockId = blockId;
  mPending.sentAt = static_cast<uint64_t>(NOW);
  mPending.clusterNo = mClusterNo;
  mPending.clusterIdx = clusterIdx;
  mPending.clusterSize = static_cast<int>(mCurrentCluster.size());
  mPending.before = std::move(before);
  if (probe)
    mNeedFirstVerify = true;
}

// Вызывается каждый тик, пока ждём дроп. Вердикт выносится в тот же тик, когда
// инвентарь изменился, — без фиксированной задержки, поэтому работает и при
// большом пинге. Не пришло ничего за Verify Timeout -> NoDrop / InvFull.
OreMinerV2::PollResult OreMinerV2::pollPendingCheck() {
  if (!mPending.active)
    return PollResult::Resolved;

  DropCheck chk = classifyDrop(mPending.before, getInventorySnapshot(),
                               mPending.blockId);
  const uint64_t now = static_cast<uint64_t>(NOW);
  const uint64_t waited = now > mPending.sentAt ? now - mPending.sentAt : 0;

  if (chk.verdict == DropVerdict::Pending) {
    if (waited < static_cast<uint64_t>(mVerifyTimeout.mValue))
      return PollResult::Waiting;
    chk.verdict =
        hasFreeInventorySlot() ? DropVerdict::NoDrop : DropVerdict::InvFull;
  } else if (mDebug.mValue && !mPending.settling) {
    // Инвентарь только что изменился. В Debug следующий блок ломается сразу
    // после вердикта, поэтому даём ещё один тик, чтобы дроп «досыпался»
    // целиком, — иначе его хвост приписался бы следующей координате.
    mPending.settling = true;
    mPending.seenWaitMs = static_cast<int>(waited);
    return PollResult::Waiting;
  }

  const PendingCheck pc = std::move(mPending);
  mPending = PendingCheck{};
  if (pc.probe)
    mNeedFirstVerify = false;

  // wait_ms — когда дроп реально появился (без тика на «досыпание»)
  recordVerdict(pc, chk, pc.settling ? pc.seenWaitMs : static_cast<int>(waited));

  // Блоки внутри уже подтверждённого кластера на ход добычи не влияют.
  if (!pc.probe || chk.verdict == DropVerdict::Real)
    return PollResult::Resolved;

  if (chk.verdict == DropVerdict::InvFull) {
    // Дроп просто некуда было положить — это не повод хоронить кластер,
    // который может быть настоящим. Вернёмся к нему, когда будет место.
    pauseForFullInventory();
    abandonCluster();
    mTickDelay = 2;
    return PollResult::ClusterDropped;
  }

  // Первый блок — фейк (или вообще без дропа): весь кластер в чёрный список.
  mFakeClusters++;
  spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks, first block {})! "
               "Blacklisting.",
               mCurrentCluster.size(), verdictName(chk.verdict));

  // Остальные блоки кластера не ломали — в дампе они «skipped» (в _fake.txt
  // не попадают: их дроп никто не видел).
  for (size_t i = 1; i < mCurrentCluster.size(); i++) {
    const BlockPos &bp = mCurrentCluster[i];
    if (!mMinedPositions.contains(bp))
      recordUnminedRow(bp, knownBlockIdAt(bp), DropVerdict::Skipped, "",
                       static_cast<int>(i), false);
  }

  blacklistPositions(mCurrentCluster);
  for (auto &bp : mCurrentCluster)
    mMinedPositions.insert(bp);

  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mState = MineState::Idle;
  mTickDelay = 2;
  return PollResult::ClusterDropped;
}

void OreMinerV2::recordVerdict(const PendingCheck &pc, const DropCheck &chk,
                               int waitMs) {
  const std::string got = chk.got.empty() ? std::string("nothing") : chk.got;
  if (chk.verdict == DropVerdict::Real)
    spdlog::info("[OreMinerV2] {} at {} {} {}: real ({}, {} ms)",
                 oreTypeName(pc.blockId), pc.pos.x, pc.pos.y, pc.pos.z, got,
                 waitMs);
  else
    spdlog::warn("[OreMinerV2] {} at {} {} {}: {} ({}, {} ms)",
                 oreTypeName(pc.blockId), pc.pos.x, pc.pos.y, pc.pos.z,
                 verdictName(chk.verdict), got, waitMs);

  if (!mDebug.mValue)
    return;

  DebugRow row;
  row.pos = pc.pos;
  row.blockId = pc.blockId;
  row.verdict = chk.verdict;
  row.got = chk.got;
  row.waitMs = waitMs;
  row.clusterNo = pc.clusterNo;
  row.clusterIdx = pc.clusterIdx;
  row.clusterSize = pc.clusterSize;
  row.listAsFake = chk.verdict == DropVerdict::Fake;
  debugWrite(row);
}

// Строка дампа для блока текущего кластера, который НЕ ломали.
void OreMinerV2::recordUnminedRow(const BlockPos &pos, int blockId,
                                  DropVerdict verdict, const std::string &got,
                                  int clusterIdx, bool listAsFake) {
  if (!mDebug.mValue)
    return;

  DebugRow row;
  row.pos = pos;
  row.blockId = blockId;
  row.verdict = verdict;
  row.got = got;
  row.waitMs = -1;
  row.clusterNo = mClusterNo;
  row.clusterIdx = clusterIdx;
  row.clusterSize = static_cast<int>(mCurrentCluster.size());
  row.listAsFake = listAsFake;
  debugWrite(row);
}

// ID руды по данным скана (если запись уже удалили — тип текущего кластера).
int OreMinerV2::knownBlockIdAt(const BlockPos &pos) const {
  const auto it = mFoundBlocks.find(pos);
  if (it != mFoundBlocks.end() && it->second.block && it->second.block->mLegacy)
    return it->second.block->mLegacy->getBlockId();
  return mCurrentClusterOreId;
}

// Бросить текущий кластер, НЕ помечая остаток ни сломанным, ни фейком:
// модуль сам вернётся к нему, когда снова сможет проверять дроп.
void OreMinerV2::abandonCluster() {
  mCurrentCluster.clear();
  mCurrentClusterIdx = 0;
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mNeedFirstVerify = false;
  mPending = PendingCheck{};
  mState = MineState::Idle;
}

// =============================================================
// MINE BLOCK — chain TP → destroy → chain TP back (optional)
// =============================================================

// Возвращает true, если пакеты слома ушли на сервер (значит, можно ждать дроп).
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

  debugSync(); // Debug включили/выключили -> открыть/закрыть сессию дампа

  if (!ClientInstance::get()->getLevelRenderer()) {
    resetScan();
    mState = MineState::Idle;
    mCurrentCluster.clear();
    mPending = PendingCheck{};
    mNeedFirstVerify = false;
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
  // Задержка Speed тикает как обычно, а дроп проверяется КАЖДЫЙ тик (даже
  // пока задержка не вышла) — вердикт выносится сразу, как только дроп пришёл.
  const bool delayed = mTickDelay > 0;
  if (delayed)
    mTickDelay--;

  if (mPending.active && pollPendingCheck() != PollResult::Resolved)
    return; // дроп ещё летит, либо кластер только что брошен как фейк

  if (delayed)
    return;

  glm::vec3 playerPos = *player->getPos();

  switch (mState) {
  // ----- IDLE -----
  case MineState::Idle: {
    const bool hasSpace = hasFreeInventorySlot();
    if (hasSpace)
      resumeAfterFullInventory();

    auto cluster = findNearestCluster(playerPos, mMinedPositions);
    if (cluster.empty()) return;

    // Без свободного слота дроп не упадёт в инвентарь: ни руду не соберём,
    // ни фейк от настоящей не отличим. Ждём, пока игрок освободит место.
    if (!hasSpace) {
      pauseForFullInventory();
      return;
    }

    // Тип руды кластера — по первому (ближайшему) блоку
    const BlockPos fp = cluster[0];
    auto it = mFilteredBlocks.find(fp);
    const int oreId =
        (it != mFilteredBlocks.end() && it->second.block &&
         it->second.block->mLegacy)
            ? it->second.block->mLegacy->getBlockId()
            : 0;

    // Блок уже не эта руда (сломан / сервер показал настоящий блок) —
    // устаревшая запись скана, пропускаем.
    Block *fb = source->getBlock(fp);
    const int curId = (fb && fb->mLegacy) ? fb->mLegacy->getBlockId() : 0;
    if (!isSameOreType(curId, oreId)) {
      mMinedPositions.insert(fp);
      return;
    }

    mCurrentCluster = std::move(cluster);
    mCurrentClusterIdx = 1;
    mCurrentClusterOreId = oreId;
    mClusterNo++;

    // Первый блок ломаем «на пробу»: его дроп решает судьбу всего кластера.
    auto before = getInventorySnapshot();
    mMinedPositions.insert(fp);
    if (!mineBlockAtPos(fp, player)) {
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      return;
    }
    startPendingCheck(fp, curId, true, 0, std::move(before));

    mState = MineState::Mining;
    mTickDelay = static_cast<int>(mSpeed.mValue);
    break;
  }

  // ----- MINING -----
  // Сюда попадаем только когда первый блок подтверждён настоящим
  // (pollPendingCheck выше), а в Debug — ещё и после вердикта по предыдущему.
  case MineState::Mining: {
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

    // В Debug дроп проверяется у каждого блока, а без свободного слота это
    // невозможно. Кластер (уже подтверждённый) не бросаем — просто стоим и
    // ждём, пока освободится место. Выключил Debug — доломает без проверки.
    if (mDebug.mValue) {
      if (!hasFreeInventorySlot()) {
        pauseForFullInventory();
        return;
      }
      resumeAfterFullInventory();
    }

    // Строго по порядку, без пропусков
    const int idx = mCurrentClusterIdx++;
    const BlockPos p = mCurrentCluster[idx]; // копия: VeinMiner ниже дописывает вектор
    if (mMinedPositions.contains(p)) return;
    mMinedPositions.insert(p); // помечаем в любом случае, чтобы не застрять

    // Сервер уже показал, что тут не эта руда (антиксрей раскрывает соседей
    // после первого слома) — ломать незачем, это фейк и так видно.
    Block *cur = source->getBlock(p);
    const int curId = (cur && cur->mLegacy) ? cur->mLegacy->getBlockId() : 0;
    if (!isSameOreType(curId, mCurrentClusterOreId)) {
      const bool nowAir = curId == 0;
      const std::string now =
          nowAir ? std::string("now air")
                 : "now " + cleanName(cur->mLegacy->mName);
      recordUnminedRow(p, knownBlockIdAt(p), DropVerdict::Revealed, now, idx,
                       !nowAir);
      return;
    }

    // В Debug каждый блок проверяется по дропу (следующий ломаем только
    // после вердикта по этому). Без Debug остаток кластера ломается подряд.
    std::unordered_map<std::string, int> before;
    if (mDebug.mValue)
      before = getInventorySnapshot();
    if (mineBlockAtPos(p, player) && mDebug.mValue)
      startPendingCheck(p, curId, false, idx, std::move(before));
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
    mState = MineState::Idle;
    mCurrentCluster.clear();
    mCurrentClusterIdx = 0;
    mMinedPositions.clear();
    mPacketPositions.clear();
    mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
    mPending = PendingCheck{}; // инвентарь сейчас поменяется не от дропа
    mNeedFirstVerify = false;
    resetScan();
  }

  if (event.mPacket->getId() == PacketID::PlayerAction) {
    auto pkt = event.getPacket<PlayerActionPacket>();
    if (pkt->mAction == PlayerActionType::Respawn) {
      std::lock_guard<std::recursive_mutex> lk(mMutex);
      mState = MineState::Idle;
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mMinedPositions.clear();
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      mPending = PendingCheck{}; // после смерти инвентарь меняется не от дропа
      mNeedFirstVerify = false;
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
  // (Debug-дамп пишется не отсюда, а из тика — по факту дропа с каждого
  // сломанного блока, см. pollPendingCheck / debugWrite.)
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
      if (pts.empty())
        continue;
      drawList->AddPolyline(pts.data(), pts.size(), fb.color, 0, 2.0f);
      drawList->AddConvexPolyFilled(
          pts.data(), pts.size(),
          ImColor(fb.color.Value.x, fb.color.Value.y, fb.color.Value.z, 0.15f));
      rendered++;
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
  // Раньше при пустом скане тут стоял return из всего onRenderEvent — вместе
  // со статусом пропадала и кнопка QuickTP ниже. Теперь просто не рисуем текст.
  {
    ImVec2 ss = ImGui::GetIO().DisplaySize;
    float cx = ss.x * 0.5f;
    float topY = ss.y * 0.05f;
    std::string status;

    if (mInvFullPause) {
      status = "Inventory full - free a slot";
    } else {
      switch (mState) {
      case MineState::Idle:
        if (!mFilteredBlocks.empty())
          status = "Scanning... [" + std::to_string(mFilteredBlocks.size()) +
                   " ores]";
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
    }

    if (mDebug.mValue) {
      if (!status.empty())
        status += " | ";
      status += "Dump: " + std::to_string(mDebugReal) + " real / " +
                std::to_string(mDebugFake) + " fake";
      if (mDebugNoDrop > 0)
        status += " / " + std::to_string(mDebugNoDrop) + " no drop";
    }

    if (!status.empty()) {
      ImVec2 ts = ImGui::CalcTextSize(status.c_str());
      drawList->AddRectFilled(ImVec2(cx - ts.x * 0.5f - 5.f, topY - 2.f),
                              ImVec2(cx + ts.x * 0.5f + 5.f, topY + ts.y + 2.f),
                              ImColor(0.f, 0.f, 0.f, 0.5f), 4.f);
      const ImColor textColor = mInvFullPause
                                    ? ImColor(1.f, 0.4f, 0.4f, 1.f)
                                    : ImColor(1.f, 1.f, 1.f, alpha);
      drawList->AddText(ImVec2(cx - ts.x * 0.5f, topY), textColor,
                        status.c_str());
    }
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