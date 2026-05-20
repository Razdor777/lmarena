//
// Created by vastrakai on 7/7/2024.
//

#include "BlockESP.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>

static std::mutex blockMutex = {};

// =========================================================
// ORE IDS
// =========================================================

constexpr int REDSTONE_ORE = 73;
constexpr int REDSTONE_ORE_LIT = 74;
constexpr int DIAMOND_ORE = 56;
constexpr int EMERALD_ORE = 129;
constexpr int GOLD_ORE = 14;
constexpr int IRON_ORE = 15;
constexpr int LAPIS_ORE = 21;
constexpr int COAL_ORE = 16;
constexpr int DEEPSLATE_REDSTONE_ORE = 658;
constexpr int DEEPSLATE_LIT_REDSTONE_ORE = 659;
constexpr int DEEPSLATE_DIAMOND_ORE = 660;
constexpr int DEEPSLATE_EMERALD_ORE = 662;
constexpr int DEEPSLATE_GOLD_ORE = 657;
constexpr int DEEPSLATE_IRON_ORE = 656;
constexpr int DEEPSLATE_LAPIS_ORE = 655;
constexpr int DEEPSLATE_COAL_ORE = 661;
constexpr int PORTAL = 90;
constexpr int CHEST = 54;
constexpr int ENDER_CHEST = 130;
constexpr int TRAPPED_CHEST = 146;
constexpr int BARREL = 458;

// =========================================================
// COLORS
// =========================================================

static std::unordered_map<int, ImColor> blockColors = {
    { REDSTONE_ORE,               ImColor(1.f, 0.f, 0.f, 1.f) },
    { REDSTONE_ORE_LIT,           ImColor(1.f, 0.f, 0.f, 1.f) },
    { DEEPSLATE_REDSTONE_ORE,     ImColor(1.f, 0.f, 0.f, 1.f) },
    { DEEPSLATE_LIT_REDSTONE_ORE, ImColor(1.f, 0.f, 0.f, 1.f) },
    { DIAMOND_ORE,                ImColor(0.f, 1.f, 1.f, 1.f) },
    { DEEPSLATE_DIAMOND_ORE,      ImColor(0.f, 1.f, 1.f, 1.f) },
    { EMERALD_ORE,                ImColor(0.f, 1.f, 0.f, 1.f) },
    { DEEPSLATE_EMERALD_ORE,      ImColor(0.f, 1.f, 0.f, 1.f) },
    { GOLD_ORE,                   ImColor(1.f, 1.f, 0.f, 1.f) },
    { DEEPSLATE_GOLD_ORE,         ImColor(1.f, 1.f, 0.f, 1.f) },
    { IRON_ORE,                   ImColor(1.f, 0.5f, 0.f, 1.f) },
    { DEEPSLATE_IRON_ORE,         ImColor(1.f, 0.5f, 0.f, 1.f) },
    { LAPIS_ORE,                  ImColor(0.f, 0.f, 1.f, 1.f) },
    { DEEPSLATE_LAPIS_ORE,        ImColor(0.f, 0.f, 1.f, 1.f) },
    { COAL_ORE,                   ImColor(0.f, 0.f, 0.f, 1.f) },
    { DEEPSLATE_COAL_ORE,         ImColor(0.f, 0.f, 0.f, 1.f) },
    { PORTAL,                     ImColor(0.5f, 0.f, 0.5f, 1.f) },
    { ENDER_CHEST,                ImColor(0.5f, 0.f, 0.5f, 1.f) },
    { CHEST,                      ImColor(255, 165, 0) },
    { TRAPPED_CHEST,              ImColor(255, 165, 0) },
    { BARREL,                     ImColor(255, 165, 0) }
};

static bool isValidBlock(int id)
{
    return blockColors.contains(id);
}

static ImColor getColorFromId(int id)
{
    if (blockColors.contains(id)) return blockColors[id];
    return ImColor(1.f, 1.f, 1.f, 1.f);
}

// =========================================================
// CLUSTER: getOreGroup
// =========================================================

static int getOreGroup(int id)
{
    switch (id) {
        case COAL_ORE: case DEEPSLATE_COAL_ORE: return 1;
        case IRON_ORE: case DEEPSLATE_IRON_ORE: return 2;
        case GOLD_ORE: case DEEPSLATE_GOLD_ORE: return 3;
        case DIAMOND_ORE: case DEEPSLATE_DIAMOND_ORE: return 4;
        case EMERALD_ORE: case DEEPSLATE_EMERALD_ORE: return 5;
        case REDSTONE_ORE: case REDSTONE_ORE_LIT:
        case DEEPSLATE_REDSTONE_ORE: case DEEPSLATE_LIT_REDSTONE_ORE: return 6;
        case LAPIS_ORE: case DEEPSLATE_LAPIS_ORE: return 7;
        case PORTAL: return 8;
        case CHEST: case ENDER_CHEST: case TRAPPED_CHEST: case BARREL: return 9;
        default: return 0;
    }
}

// =========================================================
// CLUSTER: isSameOreType
// =========================================================

bool BlockESP::isSameOreType(int id1, int id2)
{
    int g1 = getOreGroup(id1);
    int g2 = getOreGroup(id2);
    return g1 != 0 && g1 == g2;
}

// =========================================================
// CLUSTER: isOreBlock (not chest/portal)
// =========================================================

static bool isOreBlock(int id)
{
    int g = getOreGroup(id);
    return g >= 1 && g <= 7;
}

// =========================================================
// CLUSTER: BFS to find connected blocks of same ore type
// =========================================================

std::vector<BlockPos> BlockESP::getCluster(const BlockPos& startPos,
    const std::unordered_map<BlockPos, FoundBlock>& blocks)
{
    std::vector<BlockPos> cluster;
    std::unordered_set<BlockPos> visited;
    std::queue<BlockPos> queue;

    auto it = blocks.find(startPos);
    if (it == blocks.end()) return cluster;

    int startId = it->second.block->mLegacy->getBlockId();

    // Only cluster ore blocks, not chests/portals
    if (!isOreBlock(startId)) {
        cluster.push_back(startPos);
        return cluster;
    }

    queue.push(startPos);
    visited.insert(startPos);

    static const BlockPos directions[] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    while (!queue.empty()) {
        BlockPos current = queue.front();
        queue.pop();
        cluster.push_back(current);

        for (const auto& dir : directions) {
            BlockPos neighbor = {current.x + dir.x, current.y + dir.y, current.z + dir.z};

            if (visited.contains(neighbor)) continue;

            auto nit = blocks.find(neighbor);
            if (nit == blocks.end()) continue;

            int neighborId = nit->second.block->mLegacy->getBlockId();
            if (!isSameOreType(startId, neighborId)) continue;

            visited.insert(neighbor);
            queue.push(neighbor);
        }
    }

    return cluster;
}

// =========================================================
// CLUSTER: Full recompute (called rarely)
// =========================================================

void BlockESP::updateClusters()
{
    if (!mClusterFilter.mValue) {
        mFilteredBlocks = mFoundBlocks;
        mClustersValid = true;
        return;
    }

    mFilteredBlocks.clear();

    std::unordered_set<BlockPos> processed;
    int minSize = static_cast<int>(mMinClusterSize.mValue);
    int maxSize = static_cast<int>(mMaxClusterSize.mValue);

    for (auto& [pos, block] : mFoundBlocks) {
        if (processed.contains(pos)) continue;
        if (mBlacklistedPositions.contains(pos)) continue;

        int blockId = block.block->mLegacy->getBlockId();

        // Non-ore blocks (chests, portals) always pass through
        if (!isOreBlock(blockId)) {
            mFilteredBlocks[pos] = block;
            processed.insert(pos);
            continue;
        }

        std::vector<BlockPos> cluster = getCluster(pos, mFoundBlocks);

        for (const auto& cp : cluster)
            processed.insert(cp);

        int clusterSize = static_cast<int>(cluster.size());

        if (clusterSize >= minSize && clusterSize <= maxSize) {
            bool blacklisted = false;
            for (const auto& cp : cluster) {
                if (mBlacklistedPositions.contains(cp)) {
                    blacklisted = true;
                    break;
                }
            }
            if (blacklisted) continue;

            for (const auto& cp : cluster) {
                auto fit = mFoundBlocks.find(cp);
                if (fit != mFoundBlocks.end()) {
                    mFilteredBlocks[cp] = fit->second;
                }
            }
        }
    }

    mClustersValid = true;
}

// =========================================================
// CLUSTER: Remove single block incrementally
// (No full recompute needed — just remove from both maps)
// =========================================================

void BlockESP::removeBlockIncremental(const BlockPos& pos)
{
    mFoundBlocks.erase(pos);
    mFilteredBlocks.erase(pos);
}

// =========================================================
// CLUSTER: Blacklist single cluster by any block in it
// =========================================================

void BlockESP::blacklistCluster(const BlockPos& anyBlockInCluster)
{
    std::lock_guard<std::mutex> lock(blockMutex);

    auto cluster = getCluster(anyBlockInCluster, mFoundBlocks);
    for (const auto& pos : cluster) {
        mBlacklistedPositions.insert(pos);
        mFoundBlocks.erase(pos);
        mFilteredBlocks.erase(pos);
    }
}

// =========================================================
// CLUSTER: Blacklist list of positions (for OreMinerV2)
// =========================================================

void BlockESP::blacklistPositions(const std::vector<BlockPos>& positions)
{
    std::lock_guard<std::mutex> lock(blockMutex);
    for (const auto& pos : positions) {
        mBlacklistedPositions.insert(pos);
        mFoundBlocks.erase(pos);
        mFilteredBlocks.erase(pos);
    }
}

// =========================================================
// Thread-safe copy of found blocks (for OreMinerV2)
// =========================================================

std::unordered_map<BlockPos, BlockESP::FoundBlock> BlockESP::getFoundBlocksCopy()
{
    std::lock_guard<std::mutex> lock(blockMutex);
    std::unordered_map<BlockPos, FoundBlock> copy;
    for (auto& [pos, block] : mFoundBlocks) {
        if (!mBlacklistedPositions.contains(pos)) {
            copy[pos] = block;
        }
    }
    return copy;
}

// =========================================================
// SCAN: Move spiral to next position
// =========================================================

void BlockESP::moveToNext()
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }
    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci->getLocalPlayer();
    if (!player) return;
    BlockSource* blockSource = ci->getBlockSource();

    static const std::vector<std::pair<int, int>> directions = {
        { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }
    };

    size_t numSubchunks = (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / 16;
    if (numSubchunks - 1 > mSubChunkIndex) {
        mSubChunkIndex++;
        return;
    }

    mCurrentChunkPos.x += directions[mDirectionIndex].first;
    mCurrentChunkPos.y += directions[mDirectionIndex].second;

    mStepsCount++;
    if (mStepsCount >= mSteps) {
        mStepsCount = 0;
        mDirectionIndex = (mDirectionIndex + 1) % directions.size();
        if (mDirectionIndex % 2 == 0) {
            mSteps++;
        }
    }

    mSubChunkIndex = 0;
}

// =========================================================
// SCAN: Try process subchunk (safe wrapper)
// =========================================================

void BlockESP::tryProcessSub(bool& processed, ChunkPos currentChunkPos, int subChunkIndex)
{
    TRY_CALL([&]() {
        if (processSub(currentChunkPos, subChunkIndex))
            processed = true;
    });
}

// =========================================================
// SCAN: Process one subchunk
// =========================================================

bool BlockESP::processSub(ChunkPos processChunk, int index)
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return false;
    }
    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci->getLocalPlayer();
    if (!player) return false;
    BlockSource* blockSource = ci->getBlockSource();

    size_t numSubchunks = (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / 16;
    if (index < 0 || index >= numSubchunks) return false;

    LevelChunk* chunk = blockSource->getChunk(processChunk);
    if (!chunk) return false;

    auto subChunk = (*chunk->getSubChunks())[index];
    SubChunkBlockStorage* blockReader = subChunk.blockReadPtr;
    if (!blockReader) return false;

    std::vector<int> enabledBlocks = getEnabledBlocks();
    bool changed = false;

    for (uint16_t x = 0; x < 16; x++) {
        for (uint16_t z = 0; z < 16; z++) {
            for (uint16_t y = 0; y < (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / chunk->getSubChunks()->size(); y++) {
                uint16_t elementId = (x * 0x10 + z) * 0x10 + (y & 0xf);
                const Block* found = blockReader->getElement(elementId);

                BlockPos pos;
                pos.x = (processChunk.x * 16) + x;
                pos.z = (processChunk.y * 16) + z;
                pos.y = y + (subChunk.subchunkIndex * 16);

                if (found->mLegacy->getBlockId() == 0) {
                    if (mFoundBlocks.erase(pos)) {
                        mFilteredBlocks.erase(pos);
                        changed = true;
                    }
                    continue;
                }

                if (mBlacklistedPositions.contains(pos)) continue;

                if (std::ranges::find(enabledBlocks, found->mLegacy->getBlockId()) == enabledBlocks.end()) continue;

                int exposedFace = BlockUtils::getExposedFace(pos);
                if (mOnlyExposedOres.mValue && exposedFace == -1) continue;

                if (!mFoundBlocks.contains(pos)) {
                    mFoundBlocks[pos] = { found, AABB(pos, glm::vec3(1.f, 1.f, 1.f)), getColorFromId(found->mLegacy->getBlockId()) };
                    changed = true;
                }
            }
        }
    }

    // Mark that scan data changed — cluster recompute needed eventually
    if (changed) mScanDirty = true;
    return true;
}

// =========================================================
// RESET
// =========================================================

void BlockESP::reset()
{
    std::lock_guard<std::mutex> lock(blockMutex);

    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci->getLocalPlayer();
    mSearchStart = NOW;
    mFoundBlocks.clear();
    mFilteredBlocks.clear();
    mBlacklistedPositions.clear();
    mClustersValid = false;
    mScanDirty = false;
    mLastClusterUpdate = 0;
    mScanComplete = false;
    mStepsCount = 0;
    mSteps = 1;
    mDirectionIndex = 0;
    mSubChunkIndex = 0;
    if (!player) return;

    mSearchCenter = ChunkPos(*player->getPos());
    mCurrentChunkPos = mSearchCenter;
}

// =========================================================
// ENABLE / DISABLE
// =========================================================

std::vector<BlockPos> BlockESP::findNearestCluster(
    glm::vec3 playerPos,
    const std::unordered_set<BlockPos>& exclude)
{
    std::lock_guard<std::mutex> lock(blockMutex);

    auto& source = mClusterFilter.mValue ? mFilteredBlocks : mFoundBlocks;

    BlockPos nearestPos;
    float nearestDist = FLT_MAX;
    bool found = false;

    for (auto& [pos, block] : source) {
        if (exclude.contains(pos)) continue;
        if (mBlacklistedPositions.contains(pos)) continue;

        float dist = glm::distance(playerPos, glm::vec3(pos) + glm::vec3(0.5f));
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestPos = pos;
            found = true;
        }
    }

    if (!found) return {};

    // BFS — но нам нужна копия source без excluded для getCluster
    // getCluster принимает const ref map, так что строим временную
    std::unordered_map<BlockPos, FoundBlock> validBlocks;
    for (auto& [pos, block] : source) {
        if (!exclude.contains(pos) && !mBlacklistedPositions.contains(pos))
            validBlocks[pos] = block;
    }

    auto fullCluster = getCluster(nearestPos, validBlocks);

    // Sort by distance
    std::sort(fullCluster.begin(), fullCluster.end(),
        [&playerPos](const BlockPos& a, const BlockPos& b) {
            return glm::distance(playerPos, glm::vec3(a) + glm::vec3(0.5f))
                 < glm::distance(playerPos, glm::vec3(b) + glm::vec3(0.5f));
        });

    return fullCluster;
}

void BlockESP::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &BlockESP::onRenderEvent, nes::event_priority::VERY_FIRST>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &BlockESP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<BlockChangedEvent, &BlockESP::onBlockChangedEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &BlockESP::onPacketInEvent>(this);
    reset();
}

void BlockESP::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &BlockESP::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &BlockESP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<BlockChangedEvent, &BlockESP::onBlockChangedEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &BlockESP::onPacketInEvent>(this);
    reset();
}

// =========================================================
// ENABLED BLOCKS LIST
// =========================================================

std::vector<int> BlockESP::getEnabledBlocks()
{
    std::vector<int> enabledBlocks = {};

    if (mEmerald.mValue) {
        enabledBlocks.push_back(EMERALD_ORE);
        enabledBlocks.push_back(DEEPSLATE_EMERALD_ORE);
    }
    if (mDiamond.mValue) {
        enabledBlocks.push_back(DIAMOND_ORE);
        enabledBlocks.push_back(DEEPSLATE_DIAMOND_ORE);
    }
    if (mGold.mValue) {
        enabledBlocks.push_back(GOLD_ORE);
        enabledBlocks.push_back(DEEPSLATE_GOLD_ORE);
    }
    if (mIron.mValue) {
        enabledBlocks.push_back(IRON_ORE);
        enabledBlocks.push_back(DEEPSLATE_IRON_ORE);
    }
    if (mCoal.mValue) {
        enabledBlocks.push_back(COAL_ORE);
        enabledBlocks.push_back(DEEPSLATE_COAL_ORE);
    }
    if (mRedstone.mValue) {
        enabledBlocks.push_back(REDSTONE_ORE);
        enabledBlocks.push_back(REDSTONE_ORE_LIT);
        enabledBlocks.push_back(DEEPSLATE_REDSTONE_ORE);
        enabledBlocks.push_back(DEEPSLATE_LIT_REDSTONE_ORE);
    }
    if (mLapis.mValue) {
        enabledBlocks.push_back(LAPIS_ORE);
        enabledBlocks.push_back(DEEPSLATE_LAPIS_ORE);
    }
    if (mPortal.mValue) {
        enabledBlocks.push_back(PORTAL);
    }
    if (mChests.mValue) {
        enabledBlocks.push_back(CHEST);
        enabledBlocks.push_back(ENDER_CHEST);
        enabledBlocks.push_back(TRAPPED_CHEST);
        enabledBlocks.push_back(BARREL);
    }

    return enabledBlocks;
}

// =========================================================
// EVENT: Block changed
// =========================================================

void BlockESP::onBlockChangedEvent(BlockChangedEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }
    std::lock_guard<std::mutex> lock(blockMutex);

    auto dabl = BlockInfo(event.mNewBlock, event.mBlockPos);
    if (dabl.getDistance(*ClientInstance::get()->getLocalPlayer()->getPos()) > mRadius.mValue) return;

    auto enabledBlocks = getEnabledBlocks();
    int newBlockId = event.mNewBlock->mLegacy->getBlockId();

    if (mBlacklistedPositions.contains(event.mBlockPos)) {
        // Block was blacklisted — don't re-add
        return;
    }

    if (isValidBlock(newBlockId) &&
        std::ranges::find(enabledBlocks, newBlockId) != enabledBlocks.end())
    {
        // New ore placed/appeared — add it
        const Block* block = event.mNewBlock;
        FoundBlock fb = { block, AABB(event.mBlockPos, glm::vec3(1.f, 1.f, 1.f)), getColorFromId(newBlockId) };
        mFoundBlocks[event.mBlockPos] = fb;

        // If cluster filter is off, also add to filtered immediately
        if (!mClusterFilter.mValue) {
            mFilteredBlocks[event.mBlockPos] = fb;
        } else {
            // Mark for eventual cluster recompute
            mScanDirty = true;
        }
    }
    else
    {
        // Block removed/replaced with non-ore — remove incrementally
        removeBlockIncremental(event.mBlockPos);
    }
}

// =========================================================
// EVENT: Base tick
// =========================================================

void BlockESP::onBaseTickEvent(BaseTickEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }
    std::lock_guard<std::mutex> lock(blockMutex);

    static uint64_t lastUpdate = 0;
    uint64_t freq = mUpdateFrequency.mValue * 50.f;
    uint64_t now = NOW;

    if (lastUpdate + freq > now) return;
    lastUpdate = now;

    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();
    if (!player) return;
    auto blockSource = ci->getBlockSource();

    // Check if we need to restart scan spiral
    bool spiralComplete = glm::distance(glm::vec2(mCurrentChunkPos), glm::vec2(mSearchCenter)) > mChunkRadius.mValue;

    if (spiralComplete) {
        // Full scan cycle completed
        if (!mScanComplete) {
            mScanComplete = true;
            // Do one cluster recompute now that initial scan is done
            if (mClusterFilter.mValue && mScanDirty) {
                updateClusters();
                mLastClusterUpdate = now;
                mScanDirty = false;
            }
        }

        // Reset spiral for next cycle
        mSearchStart = NOW;
        mSearchCenter = ChunkPos(*player->getPos());
        mCurrentChunkPos = mSearchCenter;
        mStepsCount = 0;
        mSteps = 1;
        mDirectionIndex = 0;
        mSubChunkIndex = 0;
    }

    // Scan chunks
    for (int i = 0; i < mChunkUpdatesPerTick.mValue; i++) {
        bool processed = false;
        tryProcessSub(processed, mCurrentChunkPos, mSubChunkIndex);
        moveToNext();
    }

    // Also scan player's current chunk
    BlockPos playerPos = *player->getPos();
    int subChunk = (playerPos.y - blockSource->getBuildDepth()) >> 4;
    bool result = false;
    tryProcessSub(result, ChunkPos(playerPos), subChunk);

    // =========================================================
    // CLUSTER UPDATE LOGIC (throttled)
    // =========================================================
    if (mClusterFilter.mValue && mScanDirty) {
        // During initial scan: don't recompute clusters every tick
        // After initial scan: recompute at most every 2 seconds
        uint64_t clusterCooldown = mScanComplete ? 2000 : 5000;

        if (now - mLastClusterUpdate > clusterCooldown) {
            updateClusters();
            mLastClusterUpdate = now;
            mScanDirty = false;
        }
    }

    // If cluster filter is off, just mirror found -> filtered
    if (!mClusterFilter.mValue && !mClustersValid) {
        mFilteredBlocks = mFoundBlocks;
        mClustersValid = true;
    }
}

// =========================================================
// EVENT: Packet in
// =========================================================

void BlockESP::onPacketInEvent(PacketInEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }

    if (event.mPacket->getId() == PacketID::ChangeDimension) {
        reset();
    }

    if (event.mPacket->getId() == PacketID::PlayerAction) {
        auto packet = event.getPacket<PlayerActionPacket>();
        if (packet->mAction == PlayerActionType::Respawn) reset();
    }
}

// =========================================================
// EVENT: Render
// =========================================================
std::unordered_map<BlockPos, BlockESP::FoundBlock> BlockESP::getRenderedBlocksCopy()
{
    std::lock_guard<std::mutex> lock(blockMutex);
    auto& source = mClusterFilter.mValue ? mFilteredBlocks : mFoundBlocks;
    std::unordered_map<BlockPos, FoundBlock> copy;
    for (auto& [pos, block] : source) {
        if (!mBlacklistedPositions.contains(pos)) {
            copy[pos] = block;
        }
    }
    return copy;
}

void BlockESP::onRenderEvent(RenderEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }

    if (ClientInstance::get()->getMouseGrabbed()) return;

    std::lock_guard<std::mutex> lock(blockMutex);

    auto drawList = ImGui::GetBackgroundDrawList();
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !ClientInstance::get()->getLevelRenderer()) {
        reset();
        return;
    }

    glm::ivec3 playerPos = *player->getPos();

    // Render current chunk debug
    if (mRenderCurrentChunk.mValue) {
        ChunkPos currentChunkPos = ChunkPos(mCurrentChunkPos);
        glm::vec3 pos = glm::vec3(currentChunkPos.x * 16, 0, currentChunkPos.y * 16);
        AABB chunkAABB = AABB(pos, glm::vec3(16.f, 1.f, 16.f));
        std::vector<ImVec2> chunkPoints = MathUtils::getImBoxPoints(chunkAABB);

        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Outline)
            drawList->AddPolyline(chunkPoints.data(), chunkPoints.size(), ImColor(1.f, 1.f, 1.f), 0, 2.0f);
        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Filled)
            drawList->AddConvexPolyFilled(chunkPoints.data(), chunkPoints.size(), ImColor(1.f, 1.f, 1.f, 0.25f));
    }

    // Pre-compute for optimization
    auto enabledVec = getEnabledBlocks();
    std::unordered_set<int> enabledSet(enabledVec.begin(), enabledVec.end());
    float radiusSq = mRadius.mValue * mRadius.mValue;

    auto& renderBlocks = mClusterFilter.mValue ? mFilteredBlocks : mFoundBlocks;

    int rendered = 0;
    static constexpr int MAX_RENDER = 500;

    for (auto& [pos, block] : renderBlocks) {
        if (rendered >= MAX_RENDER) break;

        // Squared distance — no sqrt
        glm::vec3 diff = glm::vec3(pos) - glm::vec3(playerPos);
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (distSq > radiusSq) continue;

        // O(1) lookup instead of O(n) find
        if (!enabledSet.contains(block.block->mLegacy->getBlockId())) continue;

        // Skip blocks behind camera — 1 check saves 7 projections
        ImVec2 centerScreen;
        if (!RenderUtils::worldToScreen(glm::vec3(pos) + glm::vec3(0.5f), centerScreen)) continue;

        std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(block.aabb);
        if (imPoints.empty()) continue;

        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Outline)
            drawList->AddPolyline(imPoints.data(), imPoints.size(), block.color, 0, 2.0f);
        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Filled)
            drawList->AddConvexPolyFilled(imPoints.data(), imPoints.size(),
                ImColor(block.color.Value.x, block.color.Value.y, block.color.Value.z, 0.25f));

        rendered++;
    }
}