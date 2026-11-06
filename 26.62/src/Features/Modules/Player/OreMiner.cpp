#include "OreMiner.hpp"
#include <queue>
#include <algorithm>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Player/ChestStealer.hpp>
#include <Features/Modules/Player/Scaffold.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
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
#include <Utils/GameUtils/ChatUtils.hpp>
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

static ChunkPos chunkOf(const glm::vec3& p)
{
    return ChunkPos((int)std::floor(p.x) >> 4, (int)std::floor(p.z) >> 4);
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

    if (mWood.mValue && matchNames(name, sWoodN))
    {
        // "stem" ловит грядки — выкидываем
        bool isCrop = name.find("melon_stem") != std::string::npos ||
                      name.find("pumpkin_stem") != std::string::npos;
        if (!isCrop)
        {
            int wt = mWoodType.mValue;
            if (wt == 0) return true;
            static const char* kinds[] = {
                "", "oak", "birch", "spruce", "jungle", "acacia", "dark_oak",
                "mangrove", "cherry", "crimson", "warped", "pale_oak"
            };
            if (wt > 0 && wt < (int)(sizeof(kinds) / sizeof(kinds[0])))
            {
                std::string k = kinds[wt];
                bool hit = name.find(k) != std::string::npos;
                // "oak" не должен ловить "dark_oak"/"pale_oak"
                if (hit && k == "oak" &&
                    (name.find("dark_oak") != std::string::npos ||
                     name.find("pale_oak") != std::string::npos))
                    hit = false;
                if (hit) return true;
            }
        }
    }

    if (mSandstone.mValue && matchNames(name, sSandstoneN)) return true;
    if (mSnow.mValue && matchNames(name, sSnowN)) return true;
    if (mSpawner.mValue && matchNames(name, sSpawnerN)) return true;

    return isCustomBlock(name);
}

// Кэшированная проверка по BlockLegacy* — без копирования строк на каждый блок
bool OreMiner::isTargetLegacy(BlockLegacy* legacy)
{
    if (!legacy) return false;
    auto it = mTargetCache.find(legacy);
    if (it != mTargetCache.end()) return it->second;

    bool result = false;
    bool air = true;
    TRY_CALL([&]() { air = legacy->isAir(); });
    if (!air)
    {
        std::string name;
        TRY_CALL([&]() { name = legacy->getmName(); });
        result = isTargetBlock(name);
    }
    if (mTargetCache.size() > 8192) mTargetCache.clear();
    mTargetCache[legacy] = result;
    return result;
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

// Сигнатура настроек целей — при изменении сбрасываем кэш и найденное
uint64_t OreMiner::computeTargetSig()
{
    uint64_t s = 0; int bit = 0;
    auto add = [&](bool v) { if (v) s |= (1ull << bit); bit++; };
    add(mCoal.mValue); add(mIron.mValue); add(mGold.mValue); add(mDiamond.mValue);
    add(mEmerald.mValue); add(mLapis.mValue); add(mRedstone.mValue); add(mCopper.mValue);
    add(mAncientDebris.mValue); add(mQuartz.mValue); add(mLeaves.mValue); add(mWood.mValue);
    add(mSandstone.mValue); add(mSnow.mValue); add(mSpawner.mValue);
    s ^= (uint64_t)mWoodType.mValue << 40;
    for (const auto& n : mCustomBlockNames)
        s ^= std::hash<std::string>{}(n) * 0x9E3779B97F4A7C15ull;
    return s;
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
    gFeatureManager->mDispatcher->listen<BlockChangedEvent, &OreMiner::onBlockChangedEvent>(this);

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
    mVeinQueue.clear();
    mLastMineTime = 0;
    mLastToolWarnTime = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();
    mTargetCache.clear();
    mTargetSig = computeTargetSig();
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
    gFeatureManager->mDispatcher->deafen<BlockChangedEvent, &OreMiner::onBlockChangedEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player && mIsMiningBlock) player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);

    mIsMiningBlock = false;
    mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
    mCurrentBlockFace = -1;
    mBreakingProgress = 0;
    mWaitingForBreak = false;
    mWaitRetries = 0;
    mPendingVeinBlockName.clear();
    mVeinQueue.clear();
    mLastMineTime = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();
    mChunkOrder.clear();
    std::lock_guard<std::mutex> lk(mMutex);
    mPacketPositions.clear();
}

// =========================================================
// Scanner
// =========================================================
void OreMiner::resetScanner()
{
    mChunkOrder.clear();
    mChunkCursor = 0;
    mSubCursor = -1;
    mCleanCursor = 0;
    mFoundBlocks.clear();
    mProtectedPositions.clear();
    mScanCenter = ChunkPos(0, 0);
    mLastScanTime = 0;
}

int OreMiner::chunkRadius() const
{
    return (int)std::ceil(mRange.mValue / 16.f) + 1;
}

void OreMiner::rebuildChunkOrder(const ChunkPos& center)
{
    mScanCenter = center;
    mChunkOrder.clear();
    int r = chunkRadius();
    for (int dx = -r; dx <= r; dx++)
        for (int dz = -r; dz <= r; dz++)
            mChunkOrder.emplace_back(dx, dz);
    std::sort(mChunkOrder.begin(), mChunkOrder.end(),
        [](const glm::ivec2& a, const glm::ivec2& b) {
            return a.x * a.x + a.y * a.y < b.x * b.x + b.y * b.y;
        });
    mChunkCursor = 0;
    mSubCursor = -1;
}

void OreMiner::addFound(const glm::ivec3& pos, BlockLegacy* legacy)
{
    BlockPos bp = pos;
    if (mProtectedPositions.count(bp)) return;
    auto it = mFoundBlocks.find(bp);
    if (it == mFoundBlocks.end())
        mFoundBlocks[bp] = { pos, legacy };
    else
        it->second.legacy = legacy;
}

// Ближний скан — прямой куб вокруг игрока. Границы чанков ему безразличны,
// поэтому берёза в одном блоке от тебя видна ВСЕГДА.
void OreMiner::scanNear(BlockSource* source, Actor* player)
{
    glm::ivec3 c = glm::floor(*player->getPos());
    const int r = NEAR_RADIUS;

    for (int dx = -r; dx <= r; dx++)
    for (int dy = -r; dy <= r; dy++)
    for (int dz = -r; dz <= r; dz++)
    {
        glm::ivec3 pos = c + glm::ivec3(dx, dy, dz);
        Block* b = source->getBlock(pos.x, pos.y, pos.z);
        if (!b) continue;
        BlockLegacy* legacy = b->mLegacy;
        if (isTargetLegacy(legacy))
        {
            addFound(pos, legacy);
        }
        else
        {
            // Блок рядом перестал быть целью (сломан/заменён) — убираем сразу
            auto it = mFoundBlocks.find(BlockPos(pos));
            if (it != mFoundBlocks.end()) mFoundBlocks.erase(it);
        }
    }
}

// Дальний скан — чанки по возрастанию дистанции, только сабчанки в окне ±Range по Y.
// Найденное НЕ чистится при перезапуске — за мусор отвечает cleanupFound().
void OreMiner::scanFarStep(BlockSource* source, Actor* player)
{
    glm::vec3 pp = *player->getPos();
    ChunkPos pc = chunkOf(pp);

    if (mChunkOrder.empty() ||
        std::abs(pc.x - mScanCenter.x) > 1 || std::abs(pc.y - mScanCenter.y) > 1)
        rebuildChunkOrder(pc);

    const int depth   = source->getBuildDepth();
    const int height  = source->getBuildHeight();
    const int numSubs = std::max(1, (height - depth) / 16);
    const int range   = (int)mRange.mValue;

    int yMin = std::max((int)std::floor(pp.y) - range, depth);
    int yMax = std::min((int)std::floor(pp.y) + range, height - 1);
    int subMin = std::clamp((yMin - depth) / 16, 0, numSubs - 1);
    int subMax = std::clamp((yMax - depth) / 16, 0, numSubs - 1);

    int budget = SUBCHUNKS_PER_SCAN;
    int guard  = (int)mChunkOrder.size() * 2 + 16; // защита от бесконечного цикла
    while (budget > 0 && guard-- > 0)
    {
        if (mChunkCursor >= mChunkOrder.size())
        {
            // Полный цикл завершён — начинаем заново вокруг игрока
            rebuildChunkOrder(pc);
            if (mChunkOrder.empty()) return;
        }

        ChunkPos cp(mScanCenter.x + mChunkOrder[mChunkCursor].x,
                    mScanCenter.y + mChunkOrder[mChunkCursor].y);

        // Незагруженный чанк — пропускаем целиком, бюджет не тратим
        if (!source->getChunk(cp))
        {
            mChunkCursor++;
            mSubCursor = -1;
            continue;
        }

        if (mSubCursor < subMin) mSubCursor = subMin;

        int found = 0;
        TRY_CALL([&]() { scanSubChunk(cp, mSubCursor, found); });
        budget--;

        mSubCursor++;
        if (mSubCursor > subMax)
        {
            mSubCursor = -1;
            mChunkCursor++;
        }
    }
}

bool OreMiner::scanSubChunk(const ChunkPos& chunk, int subIdx, int& outFound)
{
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return false;
    LevelChunk* lc = source->getChunk(chunk);
    if (!lc) return false;
    auto subs = lc->getSubChunks();
    if (!subs || subIdx < 0 || (size_t)subIdx >= subs->size()) return false;
    auto& sub = (*subs)[subIdx];
    auto* storage = sub.blockReadPtr;
    if (!storage) return false;

    const int baseX = chunk.x * 16;
    const int baseZ = chunk.y * 16;
    const int baseY = source->getBuildDepth() + subIdx * 16;   // основной вариант
    const int altY  = (int)sub.subchunkIndex * 16;             // запасной (если маппинг иной)

    BlockLegacy* lastLegacy = nullptr;
    bool lastResult = false;

    for (int x = 0; x < 16; x++)
    for (int z = 0; z < 16; z++)
    for (int y = 0; y < 16; y++)
    {
        uint16_t eid = (uint16_t)((x << 8) | (z << 4) | y);
        Block* block = storage->getElement(eid);
        if (!block) continue;
        BlockLegacy* legacy = block->mLegacy;
        if (!legacy) continue;

        bool hit;
        if (legacy == lastLegacy) hit = lastResult;
        else { hit = isTargetLegacy(legacy); lastLegacy = legacy; lastResult = hit; }
        if (!hit) continue;

        // Верифицируем позицию через getBlock — если маппинг сабчанков врёт,
        // мусор в список не попадёт
        glm::ivec3 pos(baseX + x, baseY + y, baseZ + z);
        Block* verify = source->getBlock(pos);
        if (!verify || !isTargetLegacy(verify->mLegacy))
        {
            pos.y = altY + y;
            verify = source->getBlock(pos);
            if (!verify || !isTargetLegacy(verify->mLegacy)) continue;
        }

        addFound(pos, verify->mLegacy);
        outFound++;
    }
    return true;
}

// Инкрементальная чистка mFoundBlocks: далёкие, защищённые, сломанные
void OreMiner::cleanupFound(BlockSource* source, Actor* player)
{
    if (mFoundBlocks.empty()) { mCleanCursor = 0; return; }

    glm::vec3 pp = *player->getPos();
    float maxD = mRange.mValue + 24.f;
    float maxDSq = maxD * maxD;

    if (mCleanCursor >= mFoundBlocks.size()) mCleanCursor = 0;
    auto it = mFoundBlocks.begin();
    std::advance(it, mCleanCursor);

    for (int i = 0; i < CLEANUP_PER_TICK && it != mFoundBlocks.end(); i++)
    {
        const BlockPos& p = it->first;
        bool erase = false;

        float dx = p.x + 0.5f - pp.x, dy = p.y + 0.5f - pp.y, dz = p.z + 0.5f - pp.z;
        if (dx * dx + dy * dy + dz * dz > maxDSq) erase = true;
        else if (mProtectedPositions.count(p)) erase = true;
        else if (source->getChunk(ChunkPos(p.x >> 4, p.z >> 4)))
        {
            Block* b = source->getBlock(p);
            if (!b || !isTargetLegacy(b->mLegacy)) erase = true;
        }
        // незагруженный чанк — не трогаем, вернёмся позже

        if (erase) it = mFoundBlocks.erase(it);
        else { ++it; mCleanCursor++; }
    }
    if (it == mFoundBlocks.end()) mCleanCursor = 0;
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
        if (!item->hasDurability()) continue;
        if (item->getDurabilityPercent() * 100.f < mMinToolDurability.mValue) continue;

        float speed = ItemUtils::getDestroySpeed(i, block);
        if (speed > bestSpeed) { bestSpeed = speed; bestSlot = i; }
    }
    return bestSlot;
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
    if (bestTool == -1) return false;
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
    return true;
}

// =========================================================
// VeinMiner
// =========================================================
std::vector<glm::ivec3> OreMiner::getConnectedVein(const glm::ivec3& start, int maxBlocks)
{
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return {};

    Block* startBlock = source->getBlock(start);
    if (!startBlock || startBlock->mLegacy->isAir()) return {};
    BlockLegacy* targetLegacy = startBlock->mLegacy;

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

        for (const auto& off : offsets)
        {
            glm::ivec3 neighbor = cur + off;
            BlockPos npos = neighbor;
            if (visited.count(npos)) continue;
            visited.insert(npos);

            Block* nBlock = source->getBlock(neighbor);
            if (!nBlock || nBlock->mLegacy != targetLegacy) continue;
            queue.push(neighbor);
        }
    }
    return result;
}

// =========================================================
// FOV
// =========================================================
bool OreMiner::isInPlayerFOV(Actor* player, const glm::vec3& blockCenter)
{
    if (mFOV.mValue >= 360.f) return true;
    auto rot = player->getActorRotationComponent();
    if (!rot) return true;

    glm::vec3 pp = *player->getPos();
    float yawRad = glm::radians(rot->mYaw);
    glm::vec2 lookDir = glm::vec2(-sinf(yawRad), cosf(yawRad));
    glm::vec2 toBlock = glm::vec2(blockCenter.x - pp.x, blockCenter.z - pp.z);

    float lenSq = glm::dot(toBlock, toBlock);
    if (lenSq < 1.0f) return true;

    toBlock = glm::normalize(toBlock);
    float dot = glm::clamp(glm::dot(lookDir, toBlock), -1.0f, 1.0f);
    float angle = glm::degrees(acosf(dot));
    return angle <= mFOV.mValue * 0.5f;
}

// =========================================================
// Nearest target — сортируем, exposed-check только у ближайших
// =========================================================
glm::ivec3 OreMiner::findBestTarget(Actor* player)
{
    glm::vec3 pp = *player->getPos();
    float rangeSq = mRange.mValue * mRange.mValue;

    struct Cand { float d; glm::ivec3 p; };
    std::vector<Cand> cands;
    cands.reserve(128);

    for (auto& pair : mFoundBlocks)
    {
        const BlockPos& pos = pair.first;
        if (mProtectedPositions.count(pos)) continue;

        glm::vec3 center = glm::vec3(pos) + 0.5f;
        float dx = center.x - pp.x, dy = center.y - pp.y, dz = center.z - pp.z;
        float dsq = dx * dx + dy * dy + dz * dz;
        if (dsq > rangeSq) continue;
        if (!isInPlayerFOV(player, center)) continue;

        cands.push_back({ dsq, glm::ivec3(pos) });
    }

    if (cands.empty()) return { INT_MAX, INT_MAX, INT_MAX };

    std::sort(cands.begin(), cands.end(),
        [](const Cand& a, const Cand& b) { return a.d < b.d; });

    int checked = 0;
    for (const auto& c : cands)
    {
        if (BlockUtils::getExposedFace(c.p) != -1) return c.p;
        if (++checked >= 64) break;
    }
    return { INT_MAX, INT_MAX, INT_MAX };
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

    // Настройки целей поменялись → сброс кэша и найденного
    {
        uint64_t sig = computeTargetSig();
        if (sig != mTargetSig)
        {
            mTargetSig = sig;
            mTargetCache.clear();
            mFoundBlocks.clear();
            mChunkOrder.clear();
        }
    }

    // Conflicts
    auto chestStealer = gFeatureManager->mModuleManager->getModule<ChestStealer>();
    auto scaffold = gFeatureManager->mModuleManager->getModule<Scaffold>();
    if (player->getStatusFlag(ActorFlags::Noai) || player->isDestroying() ||
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
                    std::string n = block->mLegacy->getmName();
                    toggleCustomBlock(n);
                    ChatUtils::displayClientMessage(
                        isCustomBlock(stripNS(n)) ? "§aOreMiner: added §f{}" : "§cOreMiner: removed §f{}", stripNS(n));
                    // сигнатура сменится → кэш сбросится сам на следующем тике
                }
            }
        }
        mKeyWasDown = down;
    }

    if (!hasAnyTarget()) { mIsMiningBlock = false; return; }

    // === Scanner ===
    {
        uint64_t now = NOW;
        if (now - mLastScanTime >= SCAN_INTERVAL_MS)
        {
            mLastScanTime = now;
            TRY_CALL([&]() { scanNear(source, player); });
            TRY_CALL([&]() { scanFarStep(source, player); });
        }
        cleanupFound(source, player);

        if (mDebug.mValue && now - mLastDebugTime > 2000)
        {
            mLastDebugTime = now;
            ChatUtils::displayClientMessage(
                "§7[OreMiner] found=§f{} §7chunk=§f{}/{} §7sub=§f{} §7cache=§f{} §7range=§f{}",
                mFoundBlocks.size(), mChunkCursor, mChunkOrder.size(), mSubCursor,
                mTargetCache.size(), (int)mRange.mValue);
        }
    }

    // === Mining in progress ===
    if (mIsMiningBlock)
    {
        Block* block = source->getBlock(mCurrentBlockPos);

        if (!block || !isTargetLegacy(block->mLegacy))
        {
            player->getGameMode()->stopDestroyBlock(mCurrentBlockPos);

            if (mVeinMiner.mValue && mVeinQueue.empty() && !mPendingVeinBlockName.empty())
            {
                static const glm::ivec3 offsets[] = {
                    {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
                };
                for (const auto& off : offsets)
                {
                    glm::ivec3 neighbor = mCurrentBlockPos + off;
                    Block* nBlock = source->getBlock(neighbor);
                    if (!nBlock || nBlock->mLegacy->isAir()) continue;
                    std::string nName = stripNS(nBlock->mLegacy->getmName());
                    if (nName != stripNS(mPendingVeinBlockName)) continue;
                    auto vein = getConnectedVein(neighbor);
                    for (auto& vp : vein) mVeinQueue.push_back(vp);
                    break;
                }
            }

            mFoundBlocks.erase(mCurrentBlockPos);
            mIsMiningBlock = false;
            mWaitingForBreak = false;
            mWaitRetries = 0;
            mCurrentBlockPos = { INT_MAX, INT_MAX, INT_MAX };
            mBreakingProgress = 0;
            mShouldSpoofSlot = true;
            mLastMineTime = NOW;
            mPendingVeinBlockName.clear();
        }
        else if (mWaitingForBreak)
        {
            uint64_t elapsed = NOW - mWaitStartTime;
            uint64_t timeout = static_cast<uint64_t>(mServerTimeout.mValue);

            if (elapsed >= timeout)
            {
                mWaitRetries++;
                if (mWaitRetries >= 3)
                {
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
                    if (!mineBlockAtPos(mCurrentBlockPos, player))
                    {
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
            return;
        }
        else
        {
            int bestTool = getMiningToolSlot(block);
            if (bestTool == -1)
            {
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

                mPendingVeinBlockName = block->mLegacy->getmName();

                if (dist > 6.0f)
                {
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

    // === Vein queue ===
    if (!mIsMiningBlock && mVeinMiner.mValue && !mVeinQueue.empty())
    {
        uint64_t now = NOW;
        uint64_t delayMs = static_cast<uint64_t>(mMineDelay.mValue);
        if (delayMs > 0 && now - mLastMineTime < delayMs) return;

        if (Block* frontBlock = source->getBlock(mVeinQueue.front());
            frontBlock && !frontBlock->mLegacy->isAir() && getMiningToolSlot(frontBlock) == -1)
        {
            notifyToolStop();
            return;
        }

        int blocksThisTick = static_cast<int>(mBlocksPerTick.mValue);
        for (int b = 0; b < blocksThisTick && !mVeinQueue.empty(); b++)
        {
            glm::ivec3 veinPos = mVeinQueue.front();
            mVeinQueue.pop_front();

            Block* vBlock = source->getBlock(veinPos);
            if (vBlock && isTargetLegacy(vBlock->mLegacy))
            {
                if (!mineBlockAtPos(veinPos, player)) { notifyToolStop(); break; }
                mFoundBlocks.erase(veinPos);
            }
        }
        mLastMineTime = now;
        return;
    }

    // === Find next block ===
    if (!mIsMiningBlock)
    {
        uint64_t delayMs = static_cast<uint64_t>(mMineDelay.mValue);
        if (delayMs > 0 && NOW - mLastMineTime < delayMs) return;

        glm::ivec3 target = findBestTarget(player);
        if (target.x == INT_MAX) return;

        Block* targetBlock = source->getBlock(target);
        if (!targetBlock || targetBlock->mLegacy->isAir()) return;

        int toolSlot = getMiningToolSlot(targetBlock);
        if (toolSlot == -1) { notifyToolStop(); return; }

        mPendingVeinBlockName = targetBlock->mLegacy->getmName();

        float dist = glm::distance(*player->getPos(), glm::vec3(target) + glm::vec3(0.5f));
        if (dist > 6.0f)
        {
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
            mCurrentBlockPos = target;
            mCurrentBlockFace = BlockUtils::getExposedFace(target);
            if (mCurrentBlockFace == -1) mCurrentBlockFace = 0;
            mIsMiningBlock = true;
            mBreakingProgress = 0;
            mShouldSpoofSlot = true;
            mWaitingForBreak = false;
            mWaitRetries = 0;

            PacketUtils::spoofSlot(toolSlot, false);
            mShouldSpoofSlot = false;
            mToolSlot = toolSlot;

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
        resetScanner();
        std::lock_guard<std::mutex> lk(mMutex);
        mPacketPositions.clear();
    }
}

// =========================================================
void OreMiner::onBlockChangedEvent(BlockChangedEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    glm::vec3 pp = *player->getPos();
    float r = mRange.mValue + 8.f;
    float dx = event.mBlockPos.x - pp.x;
    float dy = event.mBlockPos.y - pp.y;
    float dz = event.mBlockPos.z - pp.z;
    if (dx * dx + dy * dy + dz * dz > r * r) return;

    bool isTarget = event.mNewBlock && isTargetLegacy(event.mNewBlock->mLegacy);
    if (isTarget)
        addFound(glm::ivec3(event.mBlockPos), event.mNewBlock->mLegacy);
    else
        mFoundBlocks.erase(event.mBlockPos);
}

// =========================================================
void OreMiner::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto drawList = ImGui::GetBackgroundDrawList();

    if (mRenderBlock.mValue && mIsMiningBlock && mCurrentBlockPos.x != INT_MAX)
    {
        float progress = std::clamp(mBreakingProgress / mCurrentDestroySpeed, 0.f, 1.f);
        if (progress > 0.01f)
        {
            glm::vec3 blockPos = glm::vec3(mCurrentBlockPos) + glm::vec3(0.5f - progress / 2.f);
            RenderUtils::drawOutlinedAABB(AABB(blockPos, glm::vec3(progress)), true, ImColor(0, 255, 0, 255));
        }
    }

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

    if (mRenderTargets.mValue)
    {
        glm::vec3 pp = *player->getPos();
        int rendered = 0;
        float maxR = mRange.mValue + 8.f;
        for (auto& pair : mFoundBlocks)
        {
            if (rendered >= 60) break;
            const BlockPos& pos = pair.first;
            if (mProtectedPositions.count(pos)) continue;
            if (glm::distance(pp, glm::vec3(pos)) > maxR) continue;
            if (!isInPlayerFOV(player, glm::vec3(pos) + 0.5f)) continue;

            auto pts = MathUtils::getImBoxPoints(AABB(glm::vec3(pos), glm::vec3(1)));
            if (pts.empty()) continue;
            ImColor c = ColorUtils::getThemedColor(0);
            c.Value.w = 0.5f;
            drawList->AddPolyline(pts.data(), pts.size(), c, true, 1.5f);
            rendered++;
        }
    }

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
