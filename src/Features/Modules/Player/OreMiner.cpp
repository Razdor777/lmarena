#include "OreMiner.hpp"
#include <queue>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Player/Regen.hpp>
#include <Features/Modules/Player/ChestStealer.hpp>
#include <Features/Modules/Player/Scaffold.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

static std::string stripNS(const std::string& name)
{
    size_t c = name.find(':');
    return (c != std::string::npos) ? name.substr(c + 1) : name;
}

// =========================================================
// Block matching
// =========================================================
bool OreMiner::matchNames(const std::string& name, const std::vector<std::string>& list)
{
    for (const auto& pattern : list)
        if (name == pattern || name.find(pattern) != std::string::npos) return true;
    return false;
}

bool OreMiner::isTargetBlock(const std::string& rawName)
{
    if (rawName.empty()) return false;
    std::string name = stripNS(rawName);

    if (mCoal.mValue && matchNames(name, sCoalN)) return true;
    if (mIron.mValue && matchNames(name, sIronN)) return true;
    if (mGold.mValue && matchNames(name, sGoldN)) return true;
    if (mDiamond.mValue && matchNames(name, sDiamondN)) return true;
    if (mEmerald.mValue && matchNames(name, sEmeraldN)) return true;
    if (mLapis.mValue && matchNames(name, sLapisN)) return true;
    if (mRedstone.mValue && matchNames(name, sRedstoneN)) return true;
    if (mCopper.mValue && matchNames(name, sCopperN)) return true;
    if (mAncientDebris.mValue && matchNames(name, sDebrisN)) return true;
    if (mQuartz.mValue && matchNames(name, sQuartzN)) return true;
    if (mLeaves.mValue && matchNames(name, sLeafN)) return true;
    if (mWood.mValue && matchNames(name, sWoodN)) return true;
    if (mSandstone.mValue && matchNames(name, sSandstoneN)) return true;
    if (mSnow.mValue && matchNames(name, sSnowN)) return true;
    if (mSpawner.mValue && matchNames(name, sSpawnerN)) return true;

    return isCustomBlock(name);
}

bool OreMiner::isCustomBlock(const std::string& name)
{
    for (const auto& n : mCustomBlockNames)
        if (n == name) return true;
    return false;
}

void OreMiner::toggleCustomBlock(const std::string& name)
{
    std::string n = stripNS(name);
    for (auto it = mCustomBlockNames.begin(); it != mCustomBlockNames.end(); ++it)
    {
        if (*it == n) { mCustomBlockNames.erase(it); return; }
    }
    mCustomBlockNames.push_back(n);
}

bool OreMiner::hasAnyTarget()
{
    return mCoal.mValue || mIron.mValue || mGold.mValue || mDiamond.mValue ||
           mEmerald.mValue || mLapis.mValue || mRedstone.mValue || mCopper.mValue ||
           mAncientDebris.mValue || mQuartz.mValue || mLeaves.mValue || mWood.mValue ||
           mSandstone.mValue || mSnow.mValue || mSpawner.mValue || !mCustomBlockNames.empty();
}

// =========================================================
// Enable / Disable
// =========================================================
void OreMiner::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &OreMiner::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &OreMiner::onPacketOutEvent, nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &OreMiner::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &OreMiner::onRenderEvent>(this);

    mCurrentBlockPos = {INT_MAX, INT_MAX, INT_MAX};
    mCurrentBlockFace = -1;
    mIsMiningBlock = false;
    mBreakingProgress = 0;
    mShouldSpoofSlot = true;
    mKeyWasDown = false;
    mWaitingForBreak = false;
    mWaitStartTime = 0;
    mWaitRetries = 0;
    mPendingVeinBlockName.clear();
    // CRITICAL: clear ALL leftover mining state, otherwise after re-enabling
    // the module keeps mining blocks around the OLD spot (e.g. where you
    // disabled it 80 blocks away)
    mVeinQueue.clear();
    mLastMineTime = 0;
    mLastToolWarnTime = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();
    { std::lock_guard<std::mutex> lk(mMutex); mPacketPositions.clear(); }

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player)
    {
        auto rot = player->getActorRotationComponent();
        if (rot) mRots = { rot->mPitch, rot->mYaw, rot->mYaw };
    }

    resetScanner();
}

void OreMiner::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &OreMiner::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &OreMiner::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &OreMiner::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &OreMiner::onRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player && mIsMiningBlock) player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);

    mIsMiningBlock = false;
    mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
    mCurrentBlockFace = -1;
    mBreakingProgress = 0;
    mWaitingForBreak = false;
    mWaitRetries = 0;
    mPendingVeinBlockName.clear();
    // Same stale-state cleanup as onEnable (leftover vein targets!)
    mVeinQueue.clear();
    mLastMineTime = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();
    std::lock_guard<std::mutex> lk(mMutex);
    mPacketPositions.clear();
}

// =========================================================
// Scanner
// =========================================================
void OreMiner::resetScanner()
{
    mScan.subChunkIdx = 0;
    mScan.dirIdx = 0;
    mScan.steps = 1;
    mScan.stepCount = 0;
    mOwnSubIdx = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) { mScan.center = ChunkPos(0, 0); mScan.current = mScan.center; return; }
    mScan.center = ChunkPos(*player->getPos());
    mScan.current = mScan.center;
}

void OreMiner::moveToNextChunk()
{
    static const std::pair<int, int> dirs[] = {{1,0},{0,1},{-1,0},{0,-1}};
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) { resetScanner(); return; }
    size_t numSubs = (source->getBuildHeight() - source->getBuildDepth()) / 16;
    if (numSubs == 0) return;
    if ((size_t)mScan.subChunkIdx < numSubs - 1) { mScan.subChunkIdx++; return; }
    mScan.current.x += dirs[mScan.dirIdx].first;
    mScan.current.y += dirs[mScan.dirIdx].second;
    mScan.stepCount++;
    if (mScan.stepCount >= mScan.steps)
    {
        mScan.stepCount = 0;
        mScan.dirIdx = (mScan.dirIdx + 1) % 4;
        if (mScan.dirIdx % 2 == 0) mScan.steps++;
    }
    mScan.subChunkIdx = 0;
}

bool OreMiner::scanSubChunk(ChunkPos chunk, int subIdx)
{
    auto ci = ClientInstance::get();
    auto source = ci->getBlockSource();
    if (!source) return false;
    LevelChunk* lc = source->getChunk(chunk);
    if (!lc) return false;
    auto subs = lc->getSubChunks();
    if (subIdx < 0 || (size_t)subIdx >= subs->size()) return false;
    auto& sub = (*subs)[subIdx];
    auto* storage = sub.blockReadPtr;
    if (!storage) return false;

    int subH = (source->getBuildHeight() - source->getBuildDepth()) / (int)subs->size();

    for (int x = 0; x < 16; x++)
    for (int z = 0; z < 16; z++)
    for (int y = 0; y < subH; y++)
    {
        uint16_t eid = (x * 16 + z) * 16 + (y & 0xf);
        const Block* block = storage->getElement(eid);
        if (!block || block->mLegacy->getBlockId() == 0) continue;

        std::string name = block->mLegacy->getmName();
        if (!isTargetBlock(name)) continue;

        BlockPos pos;
        pos.x = chunk.x * 16 + x;
        pos.z = chunk.y * 16 + z;
        pos.y = y + (sub.subchunkIndex * 16);

        // Skip protected blocks
        if (mProtectedPositions.count(pos)) continue;

        if (mFoundBlocks.find(pos) == mFoundBlocks.end())
            mFoundBlocks[pos] = { pos, stripNS(name) };
    }
    return true;
}

// =========================================================
// Chain TP and mining
// =========================================================
std::shared_ptr<MovePlayerPacket> OreMiner::createPacketForPos(glm::vec3 pos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto pkt = MinecraftPackets::createPacket<MovePlayerPacket>();
    pkt->mPos = pos;
    pkt->mPlayerID = player->getRuntimeID();
    pkt->mRot = { mRots.x, mRots.y };
    pkt->mYHeadRot = mRots.z;
    pkt->mResetPosition = PositionMode::Teleport;
    pkt->mOnGround = true;
    pkt->mRidingID = -1;
    pkt->mCause = TeleportationCause::Unknown;
    pkt->mSourceEntityType = ActorType::Player;
    pkt->mTick = 0;
    return pkt;
}

void OreMiner::straightLineTP(glm::vec3 from, glm::vec3 to, bool save)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;
    float stepSize = mStepDistance.mValue;
    float dist = glm::length(to - from);
    if (dist < 0.01f) { sender->sendToServer(createPacketForPos(to).get()); return; }

    glm::vec3 dir = glm::normalize(to - from);
    glm::vec3 cur = from;
    std::vector<glm::vec3> positions;

    while (glm::distance(cur, to) > stepSize)
    {
        cur += dir * stepSize;
        positions.push_back(cur);
        sender->sendToServer(createPacketForPos(cur).get());
    }
    positions.push_back(to);
    sender->sendToServer(createPacketForPos(to).get());

    if (save)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mPacketPositions = positions;
        mLastPathTime = NOW;
    }
}

// =========================================================
// Tool selection with durability protection (Tool Saver)
// =========================================================
int OreMiner::getMiningToolSlot(Block* block)
{
    if (!block) return -1;
    if (!mToolSaver.mValue)
        return ItemUtils::getBestBreakingTool(block, mHotbarOnly.mValue);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    int limit = mHotbarOnly.mValue ? 9 : 36;
    int bestSlot = -1;
    float bestSpeed = 0.f;

    for (int i = 0; i < limit; i++)
    {
        auto item = container->getItem(i);
        if (!item || !item->mItem) continue;

        // Only real tools qualify (a bare hand doesn't), and skip tools
        // that are about to break
        if (!item->hasDurability()) continue;
        if (item->getDurabilityPercent() * 100.f < mMinToolDurability.mValue) continue;

        float speed = ItemUtils::getDestroySpeed(i, block);
        if (speed > bestSpeed)
        {
            bestSpeed = speed;
            bestSlot = i;
        }
    }

    return bestSlot; // -1 → no usable tool → mining must pause
}

void OreMiner::notifyToolStop()
{
    if (NOW - mLastToolWarnTime < 4000) return;
    mLastToolWarnTime = NOW;
    NotifyUtils::notify("OreMiner: no tool with enough durability — mining paused!", 3.f, Notification::Type::Warning);
}

bool OreMiner::mineBlockAtPos(const glm::ivec3& pos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    auto source = ClientInstance::get()->getBlockSource();
    if (!sender || !source) return false;

    Block* block = source->getBlock(pos);
    if (!block || block->mLegacy->isAir()) return false;

    int face = BlockUtils::getExposedFace(pos);
    if (face == -1) face = 1;

    auto supplies = player->getSupplies();
    if (!supplies) return false;
    auto container = supplies->getContainer();
    if (!container) return false;

    int bestTool = getMiningToolSlot(block);
    if (bestTool == -1) return false; // Tool Saver: nothing usable — send nothing
    int oldSlot = supplies->mSelectedSlot;
    glm::vec3 playerPos = *player->getPos();
    glm::vec3 minePos = { pos.x + 0.5f, pos.y + 2.62f, pos.z + 0.5f };

    straightLineTP(playerPos, minePos, true);

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(bestTool).get());

    if (mSwing.mValue) player->swing();

    auto startPkt = MinecraftPackets::createPacket<PlayerActionPacket>();
    startPkt->mPos = pos; startPkt->mResultPos = pos; startPkt->mFace = face;
    startPkt->mAction = static_cast<PlayerActionType>(0);
    startPkt->mRuntimeId = player->getRuntimeID();
    startPkt->mtIsFromServerPlayerMovementSystem = false;
    sender->sendToServer(startPkt.get());

    auto stopPkt = MinecraftPackets::createPacket<PlayerActionPacket>();
    stopPkt->mPos = pos; stopPkt->mResultPos = pos; stopPkt->mFace = face;
    stopPkt->mAction = PlayerActionType::StopDestroyBlock;
    stopPkt->mRuntimeId = player->getRuntimeID();
    stopPkt->mtIsFromServerPlayerMovementSystem = false;
    sender->sendToServer(stopPkt.get());

    auto txnPkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ItemUseInventoryTransaction>();
    cit->mActionType = ItemUseInventoryTransaction::ActionType::Destroy;
    cit->mSlot = bestTool;
    cit->mItemInHand = NetworkItemStackDescriptor(*container->getItem(bestTool));
    cit->mBlockPos = pos; cit->mFace = face; cit->mTargetBlockRuntimeId = 0;
    cit->mPlayerPos = minePos; cit->mClickPos = { 0.5f, 1.0f, 0.5f };
    txnPkt->mTransaction = std::move(cit);
    sender->sendToServer(txnPkt.get());

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    straightLineTP(minePos, playerPos, false);

    // NOTE: Do NOT call BlockUtils::clearBlock here!
    // Clearing the block locally before server confirms causes blocks to be
    // "skipped" — the code thinks they're mined but the server never processed it.
    return true;
}

// =========================================================
// VeinMiner: BFS to find connected blocks of same type
// =========================================================
std::vector<glm::ivec3> OreMiner::getConnectedVein(const glm::ivec3& start, int maxBlocks)
{
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return {};

    Block* startBlock = source->getBlock(start);
    if (!startBlock || startBlock->mLegacy->isAir()) return {};
    std::string targetName = startBlock->mLegacy->getmName();

    std::vector<glm::ivec3> result;
    std::unordered_set<BlockPos> visited;
    std::queue<glm::ivec3> queue;

    queue.push(start);
    visited.insert(start);

    static const glm::ivec3 offsets[] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };

    while (!queue.empty() && (int)result.size() < maxBlocks)
    {
        glm::ivec3 cur = queue.front();
        queue.pop();
        result.push_back(cur);

        for (const auto& off : offsets) {
            glm::ivec3 neighbor = cur + off;
            BlockPos npos = neighbor;
            if (visited.count(npos)) continue;
            visited.insert(npos);

            Block* nBlock = source->getBlock(neighbor);
            if (!nBlock || nBlock->mLegacy->isAir()) continue;
            if (nBlock->mLegacy->getmName() != targetName) continue;

            queue.push(neighbor);
        }
    }

    return result;
}

// =========================================================
// FOV check: is the block within the player's horizontal look angle?
// =========================================================
bool OreMiner::isInPlayerFOV(Actor* player, const glm::vec3& blockCenter)
{
    // FOV=360 means no filtering
    if (mFOV.mValue >= 360.f) return true;

    auto rot = player->getActorRotationComponent();
    if (!rot) return true;

    glm::vec3 pp = *player->getPos();
    float yawRad = glm::radians(rot->mYaw);

    // Player's horizontal look direction (Minecraft: yaw 0 = south, 90 = west)
    glm::vec2 lookDir = glm::vec2(-sinf(yawRad), cosf(yawRad));
    glm::vec2 toBlock = glm::vec2(blockCenter.x - pp.x, blockCenter.z - pp.z);

    float lenSq = glm::dot(toBlock, toBlock);
    if (lenSq < 1.0f) return true; // block is right on top of us

    toBlock = glm::normalize(toBlock);
    float dot = glm::dot(lookDir, toBlock);
    dot = glm::clamp(dot, -1.0f, 1.0f);
    float angle = glm::degrees(acosf(dot));

    return angle <= mFOV.mValue * 0.5f;
}

// =========================================================
// Find nearest target (sorted by distance)
// =========================================================
glm::ivec3 OreMiner::findBestTarget(Actor* player)
{
    glm::vec3 pp = *player->getPos();
    glm::ivec3 best = { INT_MAX, INT_MAX, INT_MAX };
    float bestDistSq = FLT_MAX;

    for (auto& pair : mFoundBlocks)
    {
        const BlockPos& pos = pair.first;

        // Skip protected blocks
        if (mProtectedPositions.count(pos)) continue;

        // FOV check: only mine blocks the player is looking towards
        if (!isInPlayerFOV(player, glm::vec3(pos) + 0.5f)) continue;

        float dx = pos.x - pp.x, dy = pos.y - pp.y, dz = pos.z - pp.z;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq > SCAN_RADIUS * SCAN_RADIUS) continue;
        if (BlockUtils::getExposedFace(pos) == -1) continue;
        if (distSq < bestDistSq) { bestDistSq = distSq; best = pos; }
    }
    return best;
}

// =========================================================
// MAIN TICK
// =========================================================
void OreMiner::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;
    auto source = ClientInstance::get()->getBlockSource();
    auto supplies = player->getSupplies();
    if (!source || !supplies) return;

    mPreviousSlot = supplies->getmSelectedSlot();

    // Conflicts
    auto chestStealer = gFeatureManager->mModuleManager->getModule<ChestStealer>();
    auto scaffold = gFeatureManager->mModuleManager->getModule<Scaffold>();
    if (Regen::mIsMiningBlock || Regen::mWasMiningBlock ||
        player->getStatusFlag(ActorFlags::Noai) || player->isDestroying() ||
        (chestStealer && chestStealer->mEnabled && chestStealer->mIsStealing) ||
        (scaffold && scaffold->mEnabled))
    {
        if (mIsMiningBlock)
        {
            player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);
            mIsMiningBlock = false;
            mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
            mBreakingProgress = 0;
        }
        return;
    }

    // Middle-click → add/remove custom block
    {
        bool down = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
        if (down && !mKeyWasDown)
        {
            HitResult* hit = player->getLevel()->getHitResult();
            if (hit && hit->mType == HitType::BLOCK)
            {
                Block* block = source->getBlock(hit->mBlockPos);
                if (block && !block->mLegacy->isAir())
                {
                    toggleCustomBlock(block->mLegacy->getmName());
                    resetScanner();
                }
            }
        }
        mKeyWasDown = down;
    }

    if (!hasAnyTarget()) { mIsMiningBlock = false; return; }

    // === Scanner ===
    {
        static uint64_t lastScan = 0;
        uint64_t now = NOW;
        uint64_t freq = static_cast<uint64_t>(UPDATE_FREQ * 50.f);
        if (now - lastScan >= freq)
        {
            lastScan = now;
            ChunkPos playerChunk(*player->getPos());

            // Player walked away while the spiral was still scanning →
            // restart it around the player right away
            if (std::max(abs(mScan.center.x - playerChunk.x),
                         abs(mScan.center.y - playerChunk.y)) > 1)
            {
                mScan.center = playerChunk;
                mScan.current = mScan.center;
                mScan.stepCount = 0;
                mScan.steps = 1;
                mScan.dirIdx = 0;
                mScan.subChunkIdx = 0;
            }

            // Spiral finished → restart it (keeps outer chunks fresh)
            if (glm::distance(glm::vec2(mScan.current), glm::vec2(mScan.center)) > CHUNK_RADIUS)
            {
                mScan.center = playerChunk;
                mScan.current = mScan.center;
                mScan.stepCount = 0;
                mScan.steps = 1;
                mScan.dirIdx = 0;
                mScan.subChunkIdx = 0;
            }

            // 1) Priority pass: the player's OWN chunk, round-robin through
            // its subchunks. This is what makes ore right next to you show
            // up in ~1s instead of "sometimes it sees it, sometimes not".
            size_t numSubs = (source->getBuildHeight() - source->getBuildDepth()) / 16;
            if (numSubs == 0) numSubs = 1;
            for (int i = 0; i < OWN_SUBS_PER_TICK; i++)
            {
                TRY_CALL([&]() { scanSubChunk(playerChunk, mOwnSubIdx); });
                mOwnSubIdx = (mOwnSubIdx + 1) % (int)numSubs;
            }

            // 2) Spiral pass for the surroundings
            for (int i = 0; i < CHUNKS_PER_TICK - OWN_SUBS_PER_TICK; i++)
            {
                TRY_CALL([&]() { scanSubChunk(mScan.current, mScan.subChunkIdx); });
                moveToNextChunk();
            }
        }
    }

    // Clean up air/invalid/protected
    for (auto it = mFoundBlocks.begin(); it != mFoundBlocks.end();)
    {
        if (mProtectedPositions.count(it->first))
        {
            it = mFoundBlocks.erase(it);
            continue;
        }
        // Skip blocks in chunks that aren't loaded — otherwise valid targets
        // get purged every time a chunk unloads, and the module "forgets"
        // ores it already saw
        if (!source->getChunk(ChunkPos(it->first.x >> 4, it->first.z >> 4)))
        {
            ++it;
            continue;
        }
        Block* block = source->getBlock(it->first);
        if (!block || block->mLegacy->isAir() || !isTargetBlock(block->mLegacy->getmName()))
            it = mFoundBlocks.erase(it);
        else
            ++it;
    }

    // === Mining in progress ===
    if (mIsMiningBlock)
    {
        Block* block = source->getBlock(mCurrentBlockPos);

        // Block gone → successfully mined
        if (!block || block->mLegacy->isAir() || !isTargetBlock(block->mLegacy->getmName()))
        {
            player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);

            // VeinMiner: find connected blocks from NEIGHBORS (not the broken block itself!)
            if (mVeinMiner.mValue && mVeinQueue.empty() && !mPendingVeinBlockName.empty()) {
                static const glm::ivec3 offsets[] = {
                    {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
                };
                for (const auto& off : offsets) {
                    glm::ivec3 neighbor = mCurrentBlockPos + off;
                    Block* nBlock = source->getBlock(neighbor);
                    if (!nBlock || nBlock->mLegacy->isAir()) continue;
                    std::string nName = stripNS(nBlock->mLegacy->getmName());
                    if (nName != stripNS(mPendingVeinBlockName)) continue;
                    auto vein = getConnectedVein(neighbor);
                    for (auto& vp : vein)
                        mVeinQueue.push_back(vp);
                    break; // found a valid neighbor, BFS will handle the rest
                }
            }

            mFoundBlocks.erase(mCurrentBlockPos);
            mIsMiningBlock = false;
            mWaitingForBreak = false;
            mWaitRetries = 0;
            mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
            mBreakingProgress = 0;
            mShouldSpoofSlot = true;
            mLastMineTime = NOW; // enforce delay before next block
            mPendingVeinBlockName.clear();
        }
        else if (mWaitingForBreak)
        {
            // Far block: we sent TP+break packets, waiting for server to confirm
            uint64_t elapsed = NOW - mWaitStartTime;
            uint64_t timeout = static_cast<uint64_t>(mServerTimeout.mValue);

            if (elapsed >= timeout)
            {
                mWaitRetries++;
                if (mWaitRetries >= 3)
                {
                    // Block survived 3 retries → mark as protected and skip
                    mProtectedPositions.insert(mCurrentBlockPos);
                    mFoundBlocks.erase(mCurrentBlockPos);
                    player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);
                    mIsMiningBlock = false;
                    mWaitingForBreak = false;
                    mWaitRetries = 0;
                    mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
                    mBreakingProgress = 0;
                    mShouldSpoofSlot = true;
                    mPendingVeinBlockName.clear();
                }
                else
                {
                    // Retry: send TP+break packets again
                    if (!mineBlockAtPos(mCurrentBlockPos, player))
                    {
                        // No usable tool (Tool Saver) → pause instead of
                        // blacklisting the block for no reason
                        player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);
                        mIsMiningBlock = false;
                        mWaitingForBreak = false;
                        mWaitRetries = 0;
                        mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
                        mBreakingProgress = 0;
                        mShouldSpoofSlot = true;
                        mPendingVeinBlockName.clear();
                        notifyToolStop();
                        return;
                    }
                    mWaitStartTime = NOW;
                }
            }
            // Otherwise just keep waiting — don't flood the server!
            return;
        }
        else
        {
            // Close block: progressive mining
            int bestTool = getMiningToolSlot(block);
            if (bestTool == -1)
            {
                // Tool Saver: tool got too damaged mid-block → pause
                player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);
                mIsMiningBlock = false;
                mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
                mBreakingProgress = 0;
                mShouldSpoofSlot = true;
                mPendingVeinBlockName.clear();
                notifyToolStop();
                return;
            }
            if (mShouldSpoofSlot) { PacketUtils::spoofSlot(bestTool, false); mShouldSpoofSlot = false; return; }
            mToolSlot = bestTool;

            float destroySpeed = ItemUtils::getDestroySpeed(bestTool, block);
            mCurrentDestroySpeed = mDestroySpeed.mValue;
            mBreakingProgress += destroySpeed;

            if (mBreakingProgress >= mCurrentDestroySpeed)
            {
                int face = BlockUtils::getExposedFace(mCurrentBlockPos);
                if (face == -1) face = 0;
                float dist = glm::distance(*player->getPos(), glm::vec3(mCurrentBlockPos) + glm::vec3(0.5f));

                // Save block name for VeinMiner BEFORE breaking
                mPendingVeinBlockName = block->mLegacy->getmName();

                if (dist > 6.0f)
                {
                    // Far block: TP + break, then WAIT for server
                    if (mineBlockAtPos(mCurrentBlockPos, player))
                    {
                        mWaitingForBreak = true;
                        mWaitStartTime = NOW;
                        mWaitRetries = 0;
                    }
                    else
                    {
                        player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);
                        mIsMiningBlock = false;
                        mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
                        mShouldSpoofSlot = true;
                        mPendingVeinBlockName.clear();
                        notifyToolStop();
                    }
                }
                else
                {
                    supplies->mSelectedSlot = bestTool;
                    if (mSwing.mValue) player->swing();
                    BlockUtils::destroyBlock(mCurrentBlockPos, face, false);
                    supplies->mSelectedSlot = mPreviousSlot;
                }

                mBreakingProgress = 0;
                return;
            }
            return;
        }
    }

    // === Process vein queue (VeinMiner) ===
    if (!mIsMiningBlock && mVeinMiner.mValue && !mVeinQueue.empty())
    {
        uint64_t now = NOW;
        uint64_t delayMs = static_cast<uint64_t>(mMineDelay.mValue);
        if (delayMs > 0 && now - mLastMineTime < delayMs) return;

        // Tool Saver: don't even start the vein with a dying tool
        if (Block* frontBlock = source->getBlock(mVeinQueue.front());
            frontBlock && !frontBlock->mLegacy->isAir() && getMiningToolSlot(frontBlock) == -1)
        {
            notifyToolStop();
            return;
        }

        int blocksThisTick = static_cast<int>(mBlocksPerTick.mValue);
        for (int b = 0; b < blocksThisTick && !mVeinQueue.empty(); b++) {
            glm::ivec3 veinPos = mVeinQueue.front();
            mVeinQueue.pop_front();

            Block* vBlock = source->getBlock(veinPos);
            if (vBlock && !vBlock->mLegacy->isAir() && isTargetBlock(vBlock->mLegacy->getmName())) {
                if (!mineBlockAtPos(veinPos, player))
                {
                    notifyToolStop();
                    break;
                }
                mFoundBlocks.erase(veinPos);
            }
        }
        mLastMineTime = now;
        return;
    }

    // === Find next block (nearest first) ===
    if (!mIsMiningBlock)
    {
        // Enforce delay between blocks to avoid flooding the server
        uint64_t delayMs = static_cast<uint64_t>(mMineDelay.mValue);
        if (delayMs > 0 && NOW - mLastMineTime < delayMs) return;

        glm::ivec3 target = findBestTarget(player);
        if (target.x == INT_MAX) return;

        Block* targetBlock = source->getBlock(target);
        if (!targetBlock || targetBlock->mLegacy->isAir()) return;

        // Tool Saver: no usable tool → pause and warn (module stays enabled
        // and resumes as soon as you get a fresh tool)
        int toolSlot = getMiningToolSlot(targetBlock);
        if (toolSlot == -1)
        {
            notifyToolStop();
            return;
        }

        // Save block name for VeinMiner
        mPendingVeinBlockName = targetBlock->mLegacy->getmName();

        float dist = glm::distance(*player->getPos(), glm::vec3(target) + glm::vec3(0.5f));
        if (dist > 6.0f)
        {
            // Far block: TP + break, then wait for server confirmation
            if (!mineBlockAtPos(target, player)) { notifyToolStop(); return; }
            mCurrentBlockPos = target;
            mIsMiningBlock = true;
            mWaitingForBreak = true;
            mWaitStartTime = NOW;
            mWaitRetries = 0;
            mBreakingProgress = 0;
            mShouldSpoofSlot = true;
        }
        else
        {
            // Close block: start progressive mining
            mCurrentBlockPos = target;
            mCurrentBlockFace = BlockUtils::getExposedFace(target);
            if (mCurrentBlockFace == -1) mCurrentBlockFace = 0;
            mIsMiningBlock = true;
            mBreakingProgress = 0;
            mShouldSpoofSlot = true;
            mWaitingForBreak = false;
            mWaitRetries = 0;

            int bestTool = toolSlot;
            PacketUtils::spoofSlot(bestTool, false);
            mShouldSpoofSlot = false;
            mToolSlot = bestTool;

            BlockUtils::startDestroyBlock(target, mCurrentBlockFace);
        }
    }
}

// =========================================================
void OreMiner::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() == PacketID::MovePlayer)
    {
        auto pkt = event.getPacket<MovePlayerPacket>();
        mRots = { pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot };
    }
    else if (event.mPacket->getId() == PacketID::PlayerAuthInput)
    {
        if (mIsMiningBlock && mCurrentBlockPos.x != INT_MAX)
        {
            auto player = ClientInstance::get()->getLocalPlayer();
            if (!player) return;
            auto paip = event.getPacket<PlayerAuthInputPacket>();
            auto blockAABB = AABB(glm::vec3(mCurrentBlockPos), glm::vec3(1, 1, 1));
            glm::vec2 rotations = MathUtils::getRots(*player->getPos(), blockAABB);
            paip->mRot = rotations;
            paip->mYHeadRot = rotations.y;
        }
    }
}

void OreMiner::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() == PacketID::MovePlayer)
    {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return;
        auto pkt = event.getPacket<MovePlayerPacket>();
        if (pkt->mPlayerID != player->getRuntimeID()) return;
        event.cancel();
        ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
    }
    if (event.mPacket->getId() == PacketID::ChangeDimension)
    {
        mIsMiningBlock = false;
        mWaitingForBreak = false;
        mWaitRetries = 0;
        mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
        mBreakingProgress = 0;
        mPendingVeinBlockName.clear();
        mVeinQueue.clear();
        mFoundBlocks.clear();
        mProtectedPositions.clear();
        resetScanner();
        std::lock_guard<std::mutex> lk(mMutex);
        mPacketPositions.clear();
    }
}

// =========================================================
void OreMiner::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto drawList = ImGui::GetBackgroundDrawList();

    // Mining block
    if (mRenderBlock.mValue && mIsMiningBlock && mCurrentBlockPos.x != INT_MAX)
    {
        float progress = std::clamp(mBreakingProgress / mCurrentDestroySpeed, 0.f, 1.f);
        if (progress > 0.01f)
        {
            glm::vec3 blockPos = glm::vec3(mCurrentBlockPos) + glm::vec3(0.5f - progress / 2.f);
            RenderUtils::drawOutlinedAABB(AABB(blockPos, glm::vec3(progress)), true, ImColor(0, 255, 0, 255));
        }
    }

    // TP path
    if (mDrawPath.mValue)
    {
        std::lock_guard<std::mutex> lk(mMutex);
        uint64_t now = NOW;
        float alpha = 1.f;
        if (mLastPathTime + 500 < now) mPacketPositions.clear();
        else alpha = std::clamp(1.f - float(now - mLastPathTime) / 500.f, 0.f, 1.f);

        if (!mPacketPositions.empty())
        {
            std::vector<ImVec2> pts;
            for (auto& pos : mPacketPositions)
            {
                ImVec2 sp;
                if (RenderUtils::worldToScreen(pos, sp)) pts.push_back(sp);
            }
            for (size_t i = 0; i + 1 < pts.size(); i++)
            {
                ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
                c.Value.w *= alpha;
                drawList->AddLine(pts[i], pts[i + 1], c, 2.f);
            }
        }
    }

    // Target blocks
    if (mRenderTargets.mValue)
    {
        glm::vec3 pp = *player->getPos();
        int rendered = 0;
        for (auto& pair : mFoundBlocks)
        {
            if (rendered >= 30) break;
            const BlockPos& pos = pair.first;
            if (mProtectedPositions.count(pos)) continue;
            if (glm::distance(pp, glm::vec3(pos)) > 40.f) continue;
            if (!isInPlayerFOV(player, glm::vec3(pos) + 0.5f)) continue;

            auto pts = MathUtils::getImBoxPoints(AABB(glm::vec3(pos), glm::vec3(1)));
            if (pts.empty()) continue;
            ImColor c = ColorUtils::getThemedColor(0);
            c.Value.w = 0.5f;
            drawList->AddPolyline(pts.data(), pts.size(), c, true, 1.5f);
            rendered++;
        }
    }

    // Custom block list
    if (mShowBlockList.mValue && !mCustomBlockNames.empty())
    {
        float sx = 10;
        float sy = ImGui::GetIO().DisplaySize.y * 0.4f;
        float maxW = 0;
        for (auto& name : mCustomBlockNames)
            maxW = std::max(maxW, ImGui::CalcTextSize(name.c_str()).x);

        float listH = mCustomBlockNames.size() * 16.f + 22.f;
        drawList->AddRectFilled(ImVec2(sx - 4, sy - 4), ImVec2(sx + maxW + 20, sy + listH), IM_COL32(0, 0, 0, 140), 4.f);
        drawList->AddText(ImVec2(sx, sy), IM_COL32(255, 255, 100, 255), "Custom Blocks (MMB):");
        sy += 18;
        for (auto& name : mCustomBlockNames)
        {
            drawList->AddText(ImVec2(sx + 4, sy), IM_COL32(200, 200, 200, 255), name.c_str());
            sy += 16;
        }
    }

    // Protected blocks info
    if (!mProtectedPositions.empty())
    {
        ImVec2 ss = ImGui::GetIO().DisplaySize;
        char buf[64];
        snprintf(buf, sizeof(buf), "Protected (skipped): %d", (int)mProtectedPositions.size());
        ImVec2 ts = ImGui::CalcTextSize(buf);
        float bx = ss.x - ts.x - 15;
        float by = ss.y * 0.4f;
        drawList->AddRectFilled({bx - 4, by - 2}, {bx + ts.x + 4, by + ts.y + 2}, IM_COL32(0, 0, 0, 120), 3.f);
        drawList->AddText({bx, by}, IM_COL32(255, 100, 100, 200), buf);
    }
}