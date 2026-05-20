#include "SpawnerAura.hpp"

#include <cmath>
#include <algorithm>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

static constexpr int SPAWNER_BLOCK_ID = 52;

// =========================================================
// ENABLE / DISABLE
// =========================================================
void SpawnerAura::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &SpawnerAura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &SpawnerAura::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &SpawnerAura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &SpawnerAura::onRenderEvent>(this);

    mScanState = ScanState::Idle;
    mHasTarget = false;
    mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
    mLastScanEnd = 0;
    mLastInteract = 0;
    mPacketPositions.clear();
    mAllSpawners.clear();
    mCurrentSpawnerIdx = 0;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player)
    {
        auto rot = player->getActorRotationComponent();
        if (rot) mRots = {rot->mPitch, rot->mYaw, rot->mYaw};
    }
}

void SpawnerAura::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SpawnerAura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &SpawnerAura::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &SpawnerAura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &SpawnerAura::onRenderEvent>(this);
    mHasTarget = false;
    mScanState = ScanState::Idle;
    mPacketPositions.clear();
}

// =========================================================
// CENTER-OUTWARD X OFFSET: 0, +1, -1, +2, -2, ...
// =========================================================
int SpawnerAura::getXOffset(int slice)
{
    if (slice == 0) return 0;
    int r = (slice + 1) / 2;
    return (slice % 2 == 1) ? r : -r;
}

// =========================================================
// INCREMENTAL SCAN
// Only runs when mHasTarget == false
// Scans center-outward, stops on FIRST spawner found
// Resumes across ticks, max blocksPerTick per tick
// =========================================================
void SpawnerAura::scanTick(Actor* player)
{
    // --- Validate current target ---
    if (mHasTarget)
    {
        auto blockSource = ClientInstance::get()->getBlockSource();
        if (blockSource)
        {
            try {
                auto block = blockSource->getBlock(mCurrentTarget);
                if (block && block->toLegacy() &&
                    block->toLegacy()->getBlockId() == SPAWNER_BLOCK_ID)
                {
                    if (mRepeatMode.mValue) return; // keep hitting same spawner
                    return; // still valid — no scanning needed
                }
            } catch (...) {}
        }
        // spawner gone (broken / unloaded)
        mHasTarget = false;
        mCurrentTarget = {INT_MAX, INT_MAX, INT_MAX};
        mScanState = ScanState::Idle;
        mLastScanEnd = NOW;
    }

    // --- Idle: wait before re-scan ---
    if (mScanState == ScanState::Idle)
    {
        uint64_t cooldown = mRepeatMode.mValue ? 1000 : 3000;
        if (NOW - mLastScanEnd < cooldown) return;
        mScanState = ScanState::Scanning;
        mScanCenter = glm::ivec3(glm::floor(*player->getPos()));
        mScanSlice = 0;
        mScanZ = 0;
        mScanY = 0;
        int range = static_cast<int>(mRange.mValue);
        mTotalSlices = 2 * range + 1;
        if (mMultiSpawner.mValue) mAllSpawners.clear();
    }

    if (mScanState != ScanState::Scanning) return;

    auto blockSource = ClientInstance::get()->getBlockSource();
    if (!blockSource) return;

    int range = static_cast<int>(mRange.mValue);
    int blocksPerTick = static_cast<int>(mBlocksPerTick.mValue) * 1000;
    int yMin = std::max(mScanCenter.y - 50, -64);
    int yMax = std::min(mScanCenter.y + 50, 320);
    int yRange = yMax - yMin + 1;
    int zRange = 2 * range + 1;
    int processed = 0;

    while (processed < blocksPerTick && mScanSlice < mTotalSlices)
    {
        int xOff = getXOffset(mScanSlice);
        int worldX = mScanCenter.x + xOff;
        int worldZ = mScanCenter.z - range + mScanZ;
        int worldY = yMin + mScanY;

        try {
            auto block = blockSource->getBlock({worldX, worldY, worldZ});
            if (block)
            {
                auto legacy = block->toLegacy();
                if (legacy && legacy->getBlockId() == SPAWNER_BLOCK_ID)
                {
                    if (mMultiSpawner.mValue) {
                        // Collect all spawners, don't stop
                        mAllSpawners.push_back({worldX, worldY, worldZ});
                    } else {
                        mCurrentTarget = {worldX, worldY, worldZ};
                        mHasTarget = true;
                        mScanState = ScanState::Found;
                        return;  // done! stop scanning
                    }
                }
            }
        } catch (...) {}

        processed++;

        // advance: Y → Z → Slice
        mScanY++;
        if (mScanY >= yRange)
        {
            mScanY = 0;
            mScanZ++;
            if (mScanZ >= zRange)
            {
                mScanZ = 0;
                mScanSlice++;
            }
        }
    }

    // full scan done
    if (mScanSlice >= mTotalSlices)
    {
        if (mMultiSpawner.mValue && !mAllSpawners.empty()) {
            // Multi mode: use first spawner as current target
            mCurrentSpawnerIdx = 0;
            mCurrentTarget = mAllSpawners[0];
            mHasTarget = true;
            mScanState = ScanState::Found;
        } else {
            mScanState = ScanState::Idle;
            mLastScanEnd = NOW;
        }
    }
}

// =========================================================
// MOVE PACKET (same as InfiniteAura)
// =========================================================
std::shared_ptr<MovePlayerPacket> SpawnerAura::createPacketForPos(glm::vec3 pos)
{
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

// =========================================================
// CHAIN TP (same as InfiniteAura)
// =========================================================
void SpawnerAura::straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float stepSize = mStepDistance.mValue;
    glm::vec3 diff = to - from;
    float totalDist = glm::length(diff);

    if (totalDist < 0.01f)
    {
        sender->sendToServer(createPacketForPos(to).get());
        return;
    }

    glm::vec3 dir = glm::normalize(diff);
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

    if (saveForRender)
    {
        mPacketPositions = positions;
        mLastPathTime = NOW;
    }
}

// =========================================================
// MAIN TICK
// =========================================================
void SpawnerAura::onBaseTickEvent(BaseTickEvent& event)
{
    std::lock_guard<std::mutex> lock(mMutex);
    auto player = event.mActor;
    if (!player) return;

    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;

    // --- scan (only when no target) ---
    scanTick(player);

    // --- APS delay ---
    uint64_t delay = static_cast<uint64_t>(1000.f / mAPS.mValue);
    if (NOW - mLastInteract < delay) return;

    if (!mHasTarget) return;

    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    glm::vec3 playerPos = *player->getPos();
    int slot = supplies->mSelectedSlot;
    auto* item = container->getItem(slot);
    if (!item) return;

    // eye position on top of spawner (feet = spawner.y+1, eyes = +1.62)
    glm::vec3 interactPos = glm::vec3(
        mCurrentTarget.x + 0.5f,
        mCurrentTarget.y + 2.62f,
        mCurrentTarget.z + 0.5f
    );

    // === STEP 1: chain TP to spawner ===
    straightLineTP(playerPos, interactPos, true);

    // === STEP 2: swing ===
    player->swing();

    // === STEP 3: right-click (Place transaction) ===
    if (mClickMode.mValue == ClickMode::RightClick ||
        mClickMode.mValue == ClickMode::Both)
    {
        auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ItemUseInventoryTransaction>();
        cit->mActionType = ItemUseInventoryTransaction::ActionType::Place;
        cit->mSlot = slot;
        cit->mItemInHand = NetworkItemStackDescriptor(*item);
        cit->mBlockPos = mCurrentTarget;
        cit->mFace = 1;  // top face (Y+)
        cit->mTargetBlockRuntimeId = 0;
        cit->mPlayerPos = interactPos;
        cit->mClickPos = glm::vec3(0.5f, 1.0f, 0.5f); // center of top face
        pkt->mTransaction = std::move(cit);
        sender->sendToServer(pkt.get());
    }

    // === STEP 3b: left-click (Start + Abort destroy) ===
    if (mClickMode.mValue == ClickMode::LeftClick ||
        mClickMode.mValue == ClickMode::Both)
    {
        // StartDestroyBlock (action 0)
        auto startPkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        startPkt->mPos = mCurrentTarget;
        startPkt->mResultPos = mCurrentTarget;
        startPkt->mFace = 1;
        startPkt->mAction = static_cast<PlayerActionType>(0); // StartDestroyBlock
        startPkt->mRuntimeId = player->getRuntimeID();
        startPkt->mtIsFromServerPlayerMovementSystem = false;
        sender->sendToServer(startPkt.get());

        // AbortDestroyBlock (action 1) — don't actually break it
        auto abortPkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        abortPkt->mPos = mCurrentTarget;
        abortPkt->mResultPos = mCurrentTarget;
        abortPkt->mFace = 1;
        abortPkt->mAction = static_cast<PlayerActionType>(1); // AbortDestroyBlock
        abortPkt->mRuntimeId = player->getRuntimeID();
        abortPkt->mtIsFromServerPlayerMovementSystem = false;
        sender->sendToServer(abortPkt.get());
    }

    // === STEP 4: chain TP back ===
    straightLineTP(interactPos, playerPos, false);

    mLastInteract = NOW;

    // === STEP 5: Multi-spawner round-robin ===
    if (mMultiSpawner.mValue && mAllSpawners.size() > 1) {
        mCurrentSpawnerIdx = (mCurrentSpawnerIdx + 1) % (int)mAllSpawners.size();
        mCurrentTarget = mAllSpawners[mCurrentSpawnerIdx];
    }
}

// =========================================================
// PACKET OUT — capture rotations
// =========================================================
void SpawnerAura::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() == PacketID::MovePlayer)
    {
        auto pkt = event.getPacket<MovePlayerPacket>();
        mRots = {pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot};
    }
}

// =========================================================
// PACKET IN — silent accept (anti rubber band)
// =========================================================
void SpawnerAura::onPacketInEvent(PacketInEvent& event)
{
    if (!mSilentAccept.mValue) return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt->mPlayerID != player->getRuntimeID()) return;

    event.cancel();
    ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
}

// =========================================================
// RENDER — path + target highlight
// =========================================================
void SpawnerAura::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::lock_guard<std::mutex> lock(mMutex);  // ← СРАЗУ в начало

    float alpha = 1.0f;
    uint64_t fadeTime = 500;
    uint64_t now = NOW;

    if (mLastPathTime + fadeTime < now)
        mPacketPositions.clear();
    else
    {
        alpha = 1.0f - static_cast<float>(now - mLastPathTime)
                     / static_cast<float>(fadeTime);
        alpha = std::clamp(alpha, 0.0f, 1.0f);
    }

    auto drawList = ImGui::GetBackgroundDrawList();

    // --- PATH ---
    if (mDrawPath.mValue && !mPacketPositions.empty())
    {
        std::vector<ImVec2> points;
        for (auto& pos : mPacketPositions)
        {
            ImVec2 pt;
            if (RenderUtils::worldToScreen(pos, pt))
                points.emplace_back(pt);
        }
        for (size_t i = 0; i + 1 < points.size(); i++)
        {
            ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
            c.Value.w *= alpha;
            drawList->AddLine(points[i], points[i + 1], c, 2.0f);
        }
    }

    // --- TARGET BLOCK ---
    if (mDrawTarget.mValue && mHasTarget && mCurrentTarget.x != INT_MAX)
    {
        AABB box;
        box.mMin = glm::vec3(mCurrentTarget);
        box.mMax = glm::vec3(mCurrentTarget) + glm::vec3(1.f);
        auto pts = MathUtils::getImBoxPoints(box);
        if (!pts.empty())
        {
            ImColor fill = ColorUtils::getThemedColor(0);
            fill.Value.w = 0.15f * alpha;
            ImColor outline = ColorUtils::getThemedColor(0);
            outline.Value.w = 0.8f * alpha;
            drawList->AddConvexPolyFilled(pts.data(), pts.size(), fill);
            drawList->AddPolyline(pts.data(), pts.size(), outline, true, 2.f);
        }

        // line from player to spawner
        ImVec2 ps, ts;
        glm::vec3 center = glm::vec3(mCurrentTarget) + glm::vec3(0.5f);
        if (RenderUtils::worldToScreen(*player->getPos(), ps) &&
            RenderUtils::worldToScreen(center, ts))
        {
            ImColor lc = ColorUtils::getThemedColor(0);
            lc.Value.w = 0.4f * alpha;
            drawList->AddLine(ps, ts, lc, 1.5f);
        }
    }
}