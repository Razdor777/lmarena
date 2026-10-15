//
// Created by vastrakai on 7/7/2024.
//
// Ported from Combatant BlockESP. See BlockESP.hpp for the mapping notes.
//

#include "BlockESP.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <mutex>

static std::mutex blockMutex = {};

// =========================================================
// ORE IDS
// =========================================================

namespace BlockEspImpl
{
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

    constexpr int SCAN_RADIUS_UNLIMITED = 256;
    constexpr int RECONCILE_BUDGET_PER_TICK = 512;
    constexpr int DEOB_DELAY_TICKS = 1;

    static const glm::ivec3 kNeighbours[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
}

// =========================================================
// SCAN: Current scan radius in blocks
// =========================================================

int BlockESP::getScanRadius()
{
    return mLimitDistance.mValue ? static_cast<int>(mMaxDistance.mValue) : BlockEspImpl::SCAN_RADIUS_UNLIMITED;
}

bool BlockESP::withinDistance(const glm::vec3& pos)
{
    if (!mLimitDistance.mValue) return true;

    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci ? ci->getLocalPlayer() : nullptr;
    if (!player) return false;

    glm::vec3 eye = *player->getPos();
    glm::vec3 diff = pos - eye;
    float maxD = mMaxDistance.mValue;
    return glm::dot(diff, diff) <= maxD * maxD;
}

bool BlockESP::acceptsMode(bool visible)
{
    switch (mScanMode.mValue) {
        case BlockScanMode::LOS: return visible;
        case BlockScanMode::Unrestricted: return true;
    }
    return visible;
}

// =========================================================
// CLUSTERS (6-neighbour connectivity, like Combatant buildClusters)
// =========================================================

std::vector<std::vector<BlockPos>> BlockESP::buildClusters(const std::vector<BlockPos>& positions)
{
    std::unordered_set<BlockPos> remaining(positions.begin(), positions.end());
    std::unordered_set<BlockPos> visited;
    std::vector<std::vector<BlockPos>> clusters;

    for (const BlockPos& start : positions) {
        if (visited.contains(start)) continue;

        std::vector<BlockPos> cluster;
        std::deque<BlockPos> queue;
        queue.push_back(start);
        visited.insert(start);

        while (!queue.empty()) {
            BlockPos current = queue.front();
            queue.pop_front();
            cluster.push_back(current);

            for (const glm::ivec3& d : BlockEspImpl::kNeighbours) {
                BlockPos next = {current.x + d.x, current.y + d.y, current.z + d.z};
                if (visited.contains(next) || !remaining.contains(next)) continue;
                visited.insert(next);
                queue.push_back(next);
            }
        }

        clusters.push_back(std::move(cluster));
    }

    return clusters;
}

// =========================================================
// SCAN: Move spiral to next position
// =========================================================

void BlockESP::moveToNext()
{
    ClientInstance* ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) {
        return;
    }
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
    ClientInstance* ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) {
        return false;
    }
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
    const size_t maxTargets = static_cast<size_t>(mMaxTargets.mValue);

    for (uint16_t x = 0; x < 16; x++) {
        for (uint16_t z = 0; z < 16; z++) {
            for (uint16_t y = 0; y < (blockSource->getBuildHeight() - blockSource->getBuildDepth()) / chunk->getSubChunks()->size(); y++) {
                uint16_t elementId = (x * 0x10 + z) * 0x10 + (y & 0xf);
                const Block* found = blockReader->getElement(elementId);

                BlockPos pos;
                pos.x = (processChunk.x * 16) + x;
                pos.z = (processChunk.y * 16) + z;
                pos.y = y + (subChunk.subchunkIndex * 16);

                const int blockId = (found && found->mLegacy) ? found->mLegacy->getBlockId() : 0;
                if (blockId == 0 || std::ranges::find(enabledBlocks, blockId) == enabledBlocks.end()) {
                    // Most blocks are not targets: only touch the map when it has entries
                    if (!mFoundBlocks.empty()) mFoundBlocks.erase(pos);
                    continue;
                }

                if (!withinDistance(glm::vec3(pos) + glm::vec3(0.5f))) {
                    mFoundBlocks.erase(pos);
                    continue;
                }

                // Already tracked: reconcile handles visibility updates
                if (mFoundBlocks.contains(pos)) continue;
                if (mFoundBlocks.size() >= maxTargets) continue;

                bool visible = BlockUtils::getExposedFace(pos) != -1;
                mFoundBlocks[pos] = { found, blockId, AABB(pos, glm::vec3(1.f, 1.f, 1.f)), visible };
            }
        }
    }

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
    mReconcileKeys.clear();
    mReconcileIndex = 0;
    mDeobQueue.clear();
    mDeobBuilt = false;
    mDeobDelayTicks = 0;
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
    using namespace BlockEspImpl;
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
// RECONCILE: re-check a few tracked blocks per tick
// (still a target? still in range? visible?)
// =========================================================

void BlockESP::reconcileTargets(int budget)
{
    ClientInstance* ci = ClientInstance::get();
    BlockSource* blockSource = ci->getBlockSource();
    if (!blockSource) return;

    if (mFoundBlocks.empty()) {
        mReconcileKeys.clear();
        mReconcileIndex = 0;
        return;
    }

    if (mReconcileIndex >= mReconcileKeys.size()) {
        mReconcileKeys.clear();
        mReconcileKeys.reserve(mFoundBlocks.size());
        for (const auto& [pos, _] : mFoundBlocks) mReconcileKeys.push_back(pos);
        mReconcileIndex = 0;
    }

    std::vector<int> enabledBlocks = getEnabledBlocks();
    int processed = 0;
    while (processed < budget && mReconcileIndex < mReconcileKeys.size()) {
        BlockPos pos = mReconcileKeys[mReconcileIndex++];
        processed++;

        auto it = mFoundBlocks.find(pos);
        if (it == mFoundBlocks.end()) continue;

        if (!withinDistance(glm::vec3(pos) + glm::vec3(0.5f))) {
            mFoundBlocks.erase(it);
            continue;
        }

        Block* current = blockSource->getBlock(pos);
        if (!current || current->mLegacy == nullptr) {
            mFoundBlocks.erase(it);
            continue;
        }

        const int id = current->mLegacy->getBlockId();
        if (std::ranges::find(enabledBlocks, id) == enabledBlocks.end()) {
            mFoundBlocks.erase(it);
            continue;
        }

        it->second.block = current;
        it->second.blockId = id;
        it->second.visible = BlockUtils::getExposedFace(pos) != -1;
    }
}

// =========================================================
// DEOBFUSCATION (Combatant BlockDeobfuscationScanner)
// Briefly "mines" nearby blocks so the server reveals real states.
// =========================================================

void BlockESP::rebuildDeobQueue()
{
    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci->getLocalPlayer();
    BlockSource* blockSource = ci->getBlockSource();
    if (!player || !blockSource) return;

    const int r = std::max(1, static_cast<int>(mDeobfuscationRadius.mValue));
    const int up = std::max(1, static_cast<int>(mDeobfuscationUp.mValue));
    const int down = std::max(1, static_cast<int>(mDeobfuscationDown.mValue));
    const bool fast = mDeobfuscationFast.mValue;

    glm::ivec3 feet = glm::ivec3(glm::floor(*player->getPos() - PLAYER_HEIGHT_VEC));
    const int minY = blockSource->getBuildDepth();
    const int maxY = blockSource->getBuildHeight() - 1;

    mDeobQueue.clear();
    for (int x = feet.x - r; x <= feet.x + r; x++) {
        for (int y = std::max(minY, feet.y - down); y <= std::min(maxY, feet.y + up); y++) {
            for (int z = feet.z - r; z <= feet.z + r; z++) {
                if (fast && (x % 2 == 0 || y % 2 == 0 || z % 2 == 0)) continue;

                Block* state = blockSource->getBlock(glm::ivec3(x, y, z));
                if (!state || !state->mLegacy || state->mLegacy->getBlockId() == 0) continue;

                mDeobQueue.push_back(BlockPos{x, y, z});
            }
        }
    }

    mDeobCenter = feet;
    mDeobBuilt = true;
}

void BlockESP::tickDeobfuscation()
{
    if (!mDeobfuscation.mValue) return;

    ClientInstance* ci = ClientInstance::get();
    Actor* player = ci->getLocalPlayer();
    if (!player || !ci->getBlockSource()) return;
    GameMode* gameMode = player->getGameMode();
    if (!gameMode) return;

    const glm::ivec3 feet = glm::ivec3(glm::floor(*player->getPos() - PLAYER_HEIGHT_VEC));
    const int r = std::max(1, static_cast<int>(mDeobfuscationRadius.mValue));
    const int up = std::max(1, static_cast<int>(mDeobfuscationUp.mValue));
    const int down = std::max(1, static_cast<int>(mDeobfuscationDown.mValue));

    const bool moved = std::abs(feet.x - mDeobCenter.x) > r ||
                       std::abs(feet.z - mDeobCenter.z) > r ||
                       (feet.y - mDeobCenter.y) > up ||
                       (mDeobCenter.y - feet.y) > down;
    if (!mDeobBuilt || moved) {
        rebuildDeobQueue();
    }

    if (mDeobQueue.empty()) return;
    if (++mDeobDelayTicks < BlockEspImpl::DEOB_DELAY_TICKS) return;
    mDeobDelayTicks = 0;

    const size_t index = mDeobQueue.size() <= 1 ? 0 : static_cast<size_t>(std::rand()) % mDeobQueue.size();
    BlockPos target = mDeobQueue[index];
    mDeobQueue[index] = mDeobQueue.back();
    mDeobQueue.pop_back();

    // Use GameMode directly: BlockUtils::startDestroyBlock rewrites the crosshair hit result.
    int face = BlockUtils::getExposedFace(target);
    if (face == -1) face = 1;

    glm::ivec3 position = target;
    bool destroyed = false;
    gameMode->startDestroyBlock(&position, face, destroyed);
    gameMode->stopDestroyBlock(position);
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
    if (!ClientInstance::get()->getLocalPlayer()) return;

    std::lock_guard<std::mutex> lock(blockMutex);

    const BlockPos pos = event.mBlockPos;
    if (!withinDistance(glm::vec3(pos) + glm::vec3(0.5f))) {
        mFoundBlocks.erase(pos);
        return;
    }

    if (!event.mNewBlock || !event.mNewBlock->mLegacy) {
        mFoundBlocks.erase(pos);
        return;
    }

    const int newBlockId = event.mNewBlock->mLegacy->getBlockId();
    std::vector<int> enabledBlocks = getEnabledBlocks();

    if (std::ranges::find(enabledBlocks, newBlockId) != enabledBlocks.end()) {
        if (!mFoundBlocks.contains(pos) && mFoundBlocks.size() >= static_cast<size_t>(mMaxTargets.mValue)) {
            return;
        }
        bool visible = BlockUtils::getExposedFace(pos) != -1;
        mFoundBlocks[pos] = { event.mNewBlock, newBlockId, AABB(pos, glm::vec3(1.f, 1.f, 1.f)), visible };
    } else {
        mFoundBlocks.erase(pos);
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

    static uint64_t lastUpdate = 0;
    uint64_t freq = static_cast<uint64_t>(mUpdateFrequency.mValue * 50.f);
    uint64_t now = NOW;

    if (lastUpdate + freq > now) return;
    lastUpdate = now;

    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();
    if (!player) return;
    auto blockSource = ci->getBlockSource();

    std::lock_guard<std::mutex> lock(blockMutex);

    // Restart the spiral when it leaves the scan radius
    const int chunkRadius = std::max(1, (getScanRadius() + 15) >> 4);
    bool spiralComplete = glm::distance(glm::vec2(mCurrentChunkPos), glm::vec2(mSearchCenter)) > chunkRadius;

    if (spiralComplete) {
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

    reconcileTargets(BlockEspImpl::RECONCILE_BUDGET_PER_TICK);
    tickDeobfuscation();
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

void BlockESP::renderTracers()
{
    if (!mUseTracers.mValue) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 origin(display.x * 0.5f, display.y);

    std::vector<BlockPos> positions;
    positions.reserve(mFoundBlocks.size());
    for (const auto& [pos, block] : mFoundBlocks) {
        if (!acceptsMode(block.visible)) continue;
        if (!withinDistance(glm::vec3(pos) + glm::vec3(0.5f))) continue;
        positions.push_back(pos);
    }

    auto drawTo = [&](const glm::vec3& point, ImColor color) {
        ImVec2 screen;
        if (!RenderUtils::worldToScreen(point, screen)) return;
        drawList->AddLine(origin, screen, color, 1.5f);
    };

    if (!mClusterTracers.mValue) {
        for (const BlockPos& pos : positions) {
            bool visible = mFoundBlocks.at(pos).visible;
            drawTo(glm::vec3(pos) + glm::vec3(0.5f), visible ? mVisibleColor.getAsImColor() : mChunkColor.getAsImColor());
        }
        return;
    }

    for (const auto& cluster : buildClusters(positions)) {
        glm::vec3 center(0.f);
        bool anyVisible = false;
        for (const BlockPos& pos : cluster) {
            center += glm::vec3(pos) + glm::vec3(0.5f);
            anyVisible = anyVisible || mFoundBlocks.at(pos).visible;
        }
        center /= static_cast<float>(cluster.size());
        drawTo(center, anyVisible ? mVisibleColor.getAsImColor() : mChunkColor.getAsImColor());
    }
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

    // Render current chunk debug
    if (mRenderCurrentChunk.mValue) {
        ChunkPos currentChunkPos = ChunkPos(mCurrentChunkPos);
        glm::vec3 pos = glm::vec3(currentChunkPos.x * 16, 0, currentChunkPos.y * 16);
        AABB chunkAABB = AABB(pos, glm::vec3(16.f, 1.f, 16.f));
        std::vector<ImVec2> chunkPoints = MathUtils::getImBoxPoints(chunkAABB);

        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Outline)
            drawList->AddPolyline(chunkPoints.data(), chunkPoints.size(), mChunkColor.getAsImColor(), 0, 2.0f);
        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Filled)
            drawList->AddConvexPolyFilled(chunkPoints.data(), chunkPoints.size(), ImColor(1.f, 1.f, 1.f, 0.25f));
    }

    const float lineWidth = std::max(0.5f, mLineWidth.mValue);
    const size_t maxRendered = static_cast<size_t>(mMaxTargets.mValue);
    size_t rendered = 0;

    for (auto& [pos, block] : mFoundBlocks) {
        if (rendered >= maxRendered) break;
        if (!acceptsMode(block.visible)) continue;

        glm::vec3 center = glm::vec3(pos) + glm::vec3(0.5f);
        if (!withinDistance(center)) continue;

        // Skip blocks behind camera
        ImVec2 centerScreen;
        if (!RenderUtils::worldToScreen(center, centerScreen)) continue;

        std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(block.aabb);
        if (imPoints.empty()) continue;

        ImColor color = block.visible ? mVisibleColor.getAsImColor() : mChunkColor.getAsImColor();

        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Outline)
            drawList->AddPolyline(imPoints.data(), imPoints.size(), color, 0, lineWidth);
        if (mRenderMode.mValue == BlockRenderMode::Both || mRenderMode.mValue == BlockRenderMode::Filled)
            drawList->AddConvexPolyFilled(imPoints.data(), imPoints.size(),
                ImColor(color.Value.x, color.Value.y, color.Value.z, 0.25f));

        rendered++;
    }

    renderTracers();
}
