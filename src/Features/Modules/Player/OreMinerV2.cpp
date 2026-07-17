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

#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

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
  mReturnPos = {0.f, 0.f, 0.f};
  mVerifyTicks = 0;
  mTickDelay = 0;
  mBlocksMined = 0;
  mFakeClusters = 0;
  mMinedPositions.clear();
  mPreSnapshot.clear();
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
  mCurrentCluster.clear();
  mMinedPositions.clear();
  mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
  mPacketPositions.clear();
  mFoundBlocks.clear();
  mFilteredBlocks.clear();
  mBlacklistedPositions.clear();
  mIsBindingKey = false;
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

// =============================================================
// FAKE DROP CHECK — garbage increase means fake (anti-xray ore)
// =============================================================

bool OreMinerV2::checkForFakeDrop(
    const std::unordered_map<std::string, int> &before,
    const std::unordered_map<std::string, int> &after,
    int expectedOreId)
{
    // Прирост мусора (булыжник, камень, земля, гравий, etc.)
    static const std::vector<std::string> fakes = {
        "cobblestone", "cobbled_deepslate", "stone", "dirt", "grass",
        "gravel", "andesite", "granite", "diorite", "tuff",
        "netherrack", "end_stone", "deepslate"
    };

    for (auto &fake : fakes) {
        for (auto &[name, cnt] : after) {
            std::string ln = name;
            std::transform(ln.begin(), ln.end(), ln.begin(), ::tolower);
            if (ln.find(fake) == std::string::npos) continue;

            int prev = 0;
            auto it = before.find(name);
            if (it != before.end()) prev = it->second;

            if (cnt > prev) {
                spdlog::warn("[OreMinerV2] FAKE! Got garbage '{}' ({} -> {})", name, prev, cnt);
                return true;
            }
        }
    }

    return false;
}

// =============================================================
// EXPECTED DROP CHECK — true once the mined ore reached inventory
// =============================================================

bool OreMinerV2::hasExpectedDrop(
    const std::unordered_map<std::string, int> &before,
    const std::unordered_map<std::string, int> &after,
    int expectedOreId)
{
    std::string expectedDrop;
    OreType type = getOreType(expectedOreId);
    if (type == OreType::Diamond) expectedDrop = "diamond";
    else if (type == OreType::Coal) expectedDrop = "coal";
    else if (type == OreType::Emerald) expectedDrop = "emerald";

    if (expectedDrop.empty())
        return true; // unknown ore — считаем подтверждённым

    for (auto &[name, cnt] : after) {
        std::string ln = name;
        std::transform(ln.begin(), ln.end(), ln.begin(), ::tolower);
        if (ln.find(expectedDrop) == std::string::npos) continue;

        int prev = 0;
        auto it = before.find(name);
        if (it != before.end()) prev = it->second;

        if (cnt > prev)
            return true;
    }

    return false;
}

// =============================================================
// MINE BLOCK — chain TP → destroy → chain TP back (optional)
// =============================================================

void OreMinerV2::mineBlockAtPos(const BlockPos &pos, Actor *player,
                                bool tpBack) {
  auto sender = ClientInstance::get()->getPacketSender();
  if (!sender) return;
  auto source = ClientInstance::get()->getBlockSource();
  if (!source) return;

  Block *block = source->getBlock(pos);
  if (!block || block->mLegacy->isAir()) return;

  int face = BlockUtils::getExposedFace(pos);
  if (face == -1) face = 1;

  auto supplies = player->getSupplies();
  if (!supplies) return;
  auto container = supplies->getContainer();
  if (!container) return;

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

  // 8) TP back — только если Follow ВЫКЛЮЧЕН и tpBack разрешён.
  //    Если tpBack == false — остаёмся на блоке (нужно для подтверждения дропа,
  //    чтобы сервер успел засчитать подбор предмета).
  if (!tpBack) {
    player->setPosition(mp);
    auto sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
  } else if (!mFollow.mValue) {
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
}

// =============================================================
// RETURN FROM VERIFY — TP обратно после ожидания дропа
// =============================================================

void OreMinerV2::returnFromVerify(Actor *player) {
  if (!player)
    return;
  if (mFollow.mValue)
    return; // Follow: остаёмся на месте кластера

  glm::vec3 cur = *player->getPos();
  if (glm::distance(cur, mReturnPos) < 0.5f)
    return;

  straightLineTP(cur, mReturnPos, false);
  player->setPosition(mReturnPos);
  auto sv = player->getStateVectorComponent();
  if (sv)
    sv->mVelocity = glm::vec3(0.f);
}

// =============================================================
// MAIN TICK — scan chunks + mining state machine
// =============================================================

void OreMinerV2::onBaseTickEvent(BaseTickEvent &event) {
  std::lock_guard<std::recursive_mutex> lk(mMutex);

  auto player = event.mActor;
  if (!player) return;

  if (!ClientInstance::get()->getLevelRenderer()) {
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

    const BlockPos &fp = mCurrentCluster[0];
    Block *fb = source->getBlock(fp);
    if (!fb || fb->mLegacy->isAir()) {
      mMinedPositions.insert(fp);
      mCurrentCluster.clear();
      return;
    }

    mReturnPos = playerPos;
    mPreSnapshot = getInventorySnapshot();
    // Копаем первый блок и ОСТАЁМСЯ на нём (без TP назад), чтобы сервер
    // успел засчитать подбор дропа — иначе реальный кластер палится как фейк.
    mineBlockAtPos(fp, player, false);
    mMinedPositions.insert(fp);
    mCurrentClusterIdx = 1;
    mVerifyTicks = 0;

    mState = MineState::Verifying;
    break;
  }

  // ----- VERIFYING (ждём дроп с первого блока, стоя на руде) -----
  case MineState::Verifying: {
    auto post = getInventorySnapshot();

    // Мусор вместо руды = фейк (анти-xray), блеклистим сразу
    if (checkForFakeDrop(mPreSnapshot, post, mCurrentClusterOreId)) {
      mFakeClusters++;
      spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks)! Blacklisting.",
                   mCurrentCluster.size());

      blacklistPositions(mCurrentCluster);
      for (auto &p : mCurrentCluster) mMinedPositions.insert(p);

      returnFromVerify(player);
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      mState = MineState::Idle;
      mTickDelay = 2;
      return;
    }

    // Дроп пришёл = кластер настоящий — продолжаем копать
    if (hasExpectedDrop(mPreSnapshot, post, mCurrentClusterOreId)) {
      returnFromVerify(player);
      mState = MineState::Mining;
      break;
    }

    // Таймаут — дроп так и не пришёл = ghost-block / фейк
    mVerifyTicks++;
    if (mVerifyTicks > static_cast<int>(mVerifyTimeout.mValue)) {
      mFakeClusters++;
      spdlog::warn("[OreMinerV2] FAKE cluster ({} blocks, no drop in {}t)! Blacklisting.",
                   mCurrentCluster.size(), mVerifyTicks);

      blacklistPositions(mCurrentCluster);
      for (auto &p : mCurrentCluster) mMinedPositions.insert(p);

      returnFromVerify(player);
      mCurrentCluster.clear();
      mCurrentClusterIdx = 0;
      mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
      mState = MineState::Idle;
      mTickDelay = 2;
      return;
    }
    break;
  }

  // ----- MINING -----
  case MineState::Mining: {
    // Mine next block in cluster (strict order, no skipping)
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

    const BlockPos &p = mCurrentCluster[mCurrentClusterIdx];
    if (!mMinedPositions.contains(p)) {
      mineBlockAtPos(p, player);
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

  // ---------- BLOCK ESP ----------
  if (mDrawBlocks.mValue) {
    glm::ivec3 pp = *player->getPos();
    float radSq = mRadius.mValue * mRadius.mValue;
    auto enVec = getEnabledBlockIds();
    std::unordered_set<int> enSet(enVec.begin(), enVec.end());

    int rendered = 0;
    for (auto &[pos, fb] : mFilteredBlocks) {
      if (rendered >= 500)
        break;
      glm::vec3 d = glm::vec3(pos) - glm::vec3(pp);
      if (d.x * d.x + d.y * d.y + d.z * d.z > radSq)
        continue;
      if (!enSet.contains(fb.block->mLegacy->getBlockId()))
        continue;

      ImVec2 cs;
      if (!RenderUtils::worldToScreen(glm::vec3(pos) + glm::vec3(0.5f), cs))
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
    ImColor cc = mState == MineState::Verifying
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
    case MineState::Verifying:
      status = "Verifying drop... " + std::to_string(mVerifyTicks) + "/" +
               std::to_string(static_cast<int>(mVerifyTimeout.mValue)) +
               " | Fakes: " + std::to_string(mFakeClusters);
      break;
    case MineState::Mining:
      status = "Mining " + std::to_string(mCurrentClusterIdx) + "/" +
               std::to_string(mCurrentCluster.size()) +
               " | Fakes: " + std::to_string(mFakeClusters);
      break;
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