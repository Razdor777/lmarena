#include "Cage.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <glm/glm.hpp>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════
// Enable / Disable
// ═══════════════════════════════════════════════════════════════════════════

void Cage::onEnable()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) { setEnabled(false); return; }

    auto rot = player->getActorRotationComponent();
    if (rot) mRots = {rot->mPitch, rot->mYaw, rot->mYaw};

    mTarget     = nullptr;
    mTargetPrev = {};
    mTargetVel  = {};
    mPlaceQueue.clear();
    mCagePositions.clear();
    mLastPlace = 0;

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Cage::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &Cage::onRenderEvent>(this);
}

void Cage::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Cage::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &Cage::onRenderEvent>(this);
    mTarget = nullptr;
    mPlaceQueue.clear();
    mCagePositions.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

bool Cage::isAirAt(glm::ivec3 pos)
{
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return false;
    auto block = source->getBlock(pos);
    return (!block || !block->mLegacy || block->mLegacy->isAir());
}

int Cage::findBlockSlot()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!container) return -1;

    for (int i = 0; i < 9; i++) {
        auto* stack = container->getItem(i);
        if (!stack || !stack->mItem || stack->mCount <= 0) continue;
        if (stack->mBlock) {
            auto* bl = stack->mBlock->toLegacy();
            if (bl && !bl->isAir()) return i;
        }
    }
    return -1;
}

Actor* Cage::findTarget()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return nullptr;

    glm::vec3 myPos = *player->getPos();
    float maxRange  = mRange.mValue;
    Actor* best     = nullptr;
    float  bestDist = maxRange + 1.f;

    for (auto* a : ActorUtils::getActorList(true, true)) {
        if (a == player) continue;
        float d = glm::distance(myPos, *a->getPos());
        if (d < bestDist) { bestDist = d; best = a; }
    }
    return best;
}

// ── TP Packets (identical to RegionFill pattern) ──────────────────────────

std::shared_ptr<MovePlayerPacket> Cage::makeTPPacket(glm::vec3 pos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto pkt    = MinecraftPackets::createPacket<MovePlayerPacket>();
    pkt->mPos              = pos;
    pkt->mPlayerID         = player->getRuntimeID();
    pkt->mRot              = {mRots.x, mRots.y};
    pkt->mYHeadRot         = mRots.z;
    pkt->mResetPosition    = PositionMode::Teleport;
    pkt->mOnGround         = true;
    pkt->mRidingID         = -1;
    pkt->mCause            = TeleportationCause::Unknown;
    pkt->mSourceEntityType = ActorType::Player;
    pkt->mTick             = 0;
    return pkt;
}

void Cage::tpBetween(glm::vec3 from, glm::vec3 to)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float step = mStepDist.mValue;
    float dist = glm::length(to - from);
    glm::vec3 dir = dist > 0.001f ? glm::normalize(to - from) : glm::vec3(0.f);

    for (float d = 0.f; d < dist; d += step)
        sender->sendToServer(makeTPPacket(from + dir * d).get());

    sender->sendToServer(makeTPPacket(to).get());
}

// ── Place one block (RegionFill approach: TP over block + txn) ────────────

void Cage::placeBlock(glm::ivec3 blockPos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    int side = BlockUtils::getBlockPlaceFace(blockPos);
    if (side == -1) return;

    int slot = findBlockSlot();
    if (slot == -1) {
        ChatUtils::displayClientMessage("§c[Cage] No blocks in hotbar!");
        setEnabled(false);
        return;
    }

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return;

    int       oldSlot  = supplies->mSelectedSlot;
    glm::vec3 myPos    = *player->getPos();
    glm::vec3 standPos = glm::vec3(blockPos.x + 0.5f, blockPos.y + 2.62f, blockPos.z + 0.5f);

    // TP to position above block
    tpBetween(myPos, standPos);

    // Switch slot if needed
    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());

    if (mSwing.mValue) player->swing();

    // Send place transaction
    {
        auto txn = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ItemUseInventoryTransaction>();
        cit->mActionType           = ItemUseInventoryTransaction::ActionType::Place;
        cit->mSlot                 = slot;
        cit->mItemInHand           = NetworkItemStackDescriptor(*container->getItem(slot));
        cit->mBlockPos             = blockPos + glm::ivec3(BlockUtils::blockFaceOffsets[side]);
        cit->mFace                 = side;
        cit->mTargetBlockRuntimeId = 0;
        cit->mPlayerPos            = standPos;
        cit->mClickPos             = BlockUtils::clickPosOffsets[side];

        // Randomize click pos (anti-cheat)
        for (int i = 0; i < 3; i++)
            if (cit->mClickPos[i] == 0.5f)
                cit->mClickPos[i] = MathUtils::randomFloat(-0.49f, 0.49f);

        txn->mTransaction = std::move(cit);
        sender->sendToServer(txn.get());
    }

    // Restore slot
    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    // TP back
    tpBetween(standPos, myPos);
}

// ═══════════════════════════════════════════════════════════════════════════
// Build cage queue with smart prediction
// ═══════════════════════════════════════════════════════════════════════════

void Cage::rebuildQueue()
{
    if (!mTarget) return;

    glm::vec3 tPos = *mTarget->getPos();

    // ── Predict where the target will be ──────────────────────────────────
    glm::vec3 predictedPos = tPos;
    if (mPredictMotion.mValue) {
        float ticks = mPredictTicks.mValue;
        // mTargetVel is updated each tick
        predictedPos = tPos + mTargetVel * ticks;
    }

    int R = (int)mCageRadius.mValue;
    int H = (int)mCageHeight.mValue;

    // ── Generate ALL cage block positions around predicted location ────────
    // Use round() for X/Z so cage wraps around the target properly
    // even when standing on block edges (between 2-4 blocks)
    // Y uses feet level (pos.y is eye height, subtract 1.62 for feet)
    glm::ivec3 center = glm::ivec3(
        (int)std::round(predictedPos.x - 0.5f),
        (int)std::floor(predictedPos.y - 1.62f),
        (int)std::round(predictedPos.z - 0.5f));

    std::vector<glm::ivec3> walls, ceiling, floor;

    // Walls: perimeter of XZ ring at each Y level
    for (int y = 0; y < H; y++) {
        for (int x = -R; x <= R; x++) {
            for (int z = -R; z <= R; z++) {
                if (!mHollowWalls.mValue) {
                    // Solid — all blocks inside ring
                    if (x == -R || x == R || z == -R || z == R) {
                        walls.push_back({center.x + x, center.y + y, center.z + z});
                    }
                } else {
                    // Perimeter only
                    if (std::abs(x) == R || std::abs(z) == R) {
                        walls.push_back({center.x + x, center.y + y, center.z + z});
                    }
                }
            }
        }
    }

    // Ceiling: solid cap above
    if (mPlaceCeiling.mValue) {
        for (int x = -R; x <= R; x++)
            for (int z = -R; z <= R; z++)
                ceiling.push_back({center.x + x, center.y + H, center.z + z});
    }

    // Floor: one below feet
    if (mPlaceFloor.mValue) {
        for (int x = -R; x <= R; x++)
            for (int z = -R; z <= R; z++)
                floor.push_back({center.x + x, center.y - 1, center.z + z});
    }

    // ── Sort by priority: cut-off mode puts predicted-path blocks first ───
    auto toCurrent = *mTarget->getPos();
    auto velocity  = mTargetVel;

    // Priority function: blocks in front of the target's motion get placed first
    auto priority = [&](const glm::ivec3& bp) -> float {
        glm::vec3 bpf(bp.x + 0.5f, bp.y, bp.z + 0.5f);
        if (mCutoffMode.mValue && glm::length(velocity) > 0.01f) {
            // Dot product: how much is this block "ahead" of target's movement?
            glm::vec3 dir = glm::normalize(velocity);
            glm::vec3 toBlock = bpf - toCurrent;
            float dot = glm::dot(dir, glm::vec3(toBlock.x, 0.f, toBlock.z));
            // Negate: higher dot = in front = higher priority
            return -dot;
        }
        // Default: distance to target (closest first)
        return glm::distance(bpf, toCurrent);
    };

    // Merge: walls first, then ceiling, then floor
    std::vector<glm::ivec3> all;
    all.insert(all.end(), walls.begin(),   walls.end());
    all.insert(all.end(), ceiling.begin(), ceiling.end());
    all.insert(all.end(), floor.begin(),   floor.end());

    // Filter: only air positions
    std::vector<glm::ivec3> needed;
    for (auto& p : all) {
        if (isAirAt(p)) needed.push_back(p);
    }

    // Sort by priority
    std::sort(needed.begin(), needed.end(), [&](const glm::ivec3& a, const glm::ivec3& b){
        return priority(a) < priority(b);
    });

    mPlaceQueue    = std::move(needed);
    mCagePositions = all; // for ESP (all, not just air)
}

// ═══════════════════════════════════════════════════════════════════════════
// Tick
// ═══════════════════════════════════════════════════════════════════════════

void Cage::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    // ── Select target ─────────────────────────────────────────────────────
    if (mAutoTarget.mValue) {
        mTarget = findTarget();
    }
    if (!mTarget || mTarget->isDead()) {
        mTarget = nullptr;
        mPlaceQueue.clear();
        return;
    }

    // ── Track velocity ────────────────────────────────────────────────────
    glm::vec3 curPos = *mTarget->getPos();
    mTargetVel  = curPos - mTargetPrev;
    mTargetPrev = curPos;

    // ── Check distance ────────────────────────────────────────────────────
    float dist = glm::distance(*player->getPos(), curPos);
    if (dist > mRange.mValue) {
        mPlaceQueue.clear();
        return;
    }

    // ── Rebuild queue every tick (target moves) ───────────────────────────
    rebuildQueue();

    if (mPlaceQueue.empty()) return;

    // ── Delay ─────────────────────────────────────────────────────────────
    if (NOW - mLastPlace < static_cast<uint64_t>(mDelay.mValue)) return;

    // ── Place N blocks per tick ───────────────────────────────────────────
    int maxPlace = (int)mBlocksPerTick.mValue;
    int placed   = 0;

    while (placed < maxPlace && !mPlaceQueue.empty()) {
        glm::ivec3 pos = mPlaceQueue.front();
        mPlaceQueue.erase(mPlaceQueue.begin());

        // Skip if already filled (another player placed / block update)
        if (!isAirAt(pos)) continue;

        placeBlock(pos, player);
        placed++;
    }

    if (placed > 0) mLastPlace = NOW;
}

// ═══════════════════════════════════════════════════════════════════════════
// Render — ESP highlight for cage positions
// ═══════════════════════════════════════════════════════════════════════════

void Cage::onRenderEvent(RenderEvent& event)
{
    if (!mShowESP.mValue || !mTarget) return;

    float now = (float)ImGui::GetTime();


    for (auto& pos : mCagePositions) {
        if (!isAirAt(pos)) continue;
        AABB box(
            glm::vec3(pos.x,     pos.y,     pos.z),
            glm::vec3(pos.x + 1, pos.y + 1, pos.z + 1)
        );
        // filled tint
        ImColor fill = ColorUtils::getThemedColor(now * 50.f);
        fill.Value.w  = 0.10f + 0.05f * sinf(now * 4.f);
        ImColor outline = ColorUtils::getThemedColor(now * 50.f + 45.f);
        outline.Value.w = 0.75f;
        RenderUtils::drawOutlinedAABB(box, true, fill);
        RenderUtils::drawOutlinedAABB(box, false, outline);
    }

    // Highlight target
    if (mTarget) {
        AABB tbox = mTarget->getAABB();
        float pulse = 0.5f + 0.25f * sinf(now * 6.f);
        ImColor fill = ImColor(1.f, 0.3f, 0.3f, pulse * 0.5f);
        ImColor out  = ImColor(1.f, 0.2f, 0.2f, 0.9f);
        RenderUtils::drawOutlinedAABB(tbox, true,  fill);
        RenderUtils::drawOutlinedAABB(tbox, false, out);
    }
}
