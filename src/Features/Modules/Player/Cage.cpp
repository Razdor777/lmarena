#include "Cage.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
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
#include <set>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ═══════════════════════════════════════════════════════════════
// Enable / Disable
// ═══════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════

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
            if (bl && !bl->isAir() && bl->getBlockId() != 30) return i;
        }
    }
    return -1;
}

int Cage::findWebSlot()
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
            if (bl && bl->getBlockId() == 30) return i;
        }
    }
    return -1;
}

static int getArmorCount(Actor* a)
{
    auto* armor = a->getArmorContainer();
    if (!armor) return 0;
    int count = 0;
    for (int s = 0; s < 4; s++) {
        auto* item = armor->getItem(s);
        if (item && item->mItem) count++;
    }
    return count;
}

Actor* Cage::findTarget()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return nullptr;

    glm::vec3 myPos = *player->getPos();
    Actor* best  = nullptr;
    float  bestVal = FLT_MAX;

    for (auto* a : ActorUtils::getActorList(true, true)) {
        if (a == player) continue;
        if (a->isDead()) continue;
        if (a->getHealth() <= 0.f) continue;

        bool isPlayer = a->isPlayer();
        bool isMob    = !isPlayer;
        if (mTargetMode.mValue == TargetMode::Players && !isPlayer) continue;
        if (mTargetMode.mValue == TargetMode::Mobs    && !isMob)    continue;

        if (mIgnoreFriends.mValue && gFriendManager && gFriendManager->isFriend(a)) continue;

        float val = 0.f;
        if (mPriority.mValue == Priority::Closest) {
            val = glm::distance(myPos, *a->getPos());
        } else {
            val = (float)(4 - getArmorCount(a));
        }

        if (val < bestVal) { bestVal = val; best = a; }
    }
    return best;
}

// ═══════════════════════════════════════════════════════════════
// TP
// ═══════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════
// Place block
// ═══════════════════════════════════════════════════════════════

void Cage::placeBlockAt(glm::ivec3 blockPos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    int side = BlockUtils::getBlockPlaceFace(blockPos);
    if (side == -1) return;

    int slot = findBlockSlot();
    if (slot == -1) return;

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return;

    int       oldSlot  = supplies->mSelectedSlot;
    glm::vec3 myPos    = *player->getPos();
    glm::vec3 standPos = glm::vec3(blockPos.x + 0.5f, blockPos.y + 2.62f, blockPos.z + 0.5f);

    tpBetween(myPos, standPos);

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());

    if (mSwing.mValue) player->swing();

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

        for (int i = 0; i < 3; i++)
            if (cit->mClickPos[i] == 0.5f)
                cit->mClickPos[i] = MathUtils::randomFloat(-0.49f, 0.49f);

        txn->mTransaction = std::move(cit);
        sender->sendToServer(txn.get());
    }

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    tpBetween(standPos, myPos);
}

// ═══════════════════════════════════════════════════════════════
// Place web
// ═══════════════════════════════════════════════════════════════

void Cage::placeWebAt(glm::ivec3 blockPos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    int slot = findWebSlot();
    if (slot == -1) return;

    int side = BlockUtils::getBlockPlaceFace(blockPos);
    if (side == -1) {
        glm::ivec3 below = blockPos - glm::ivec3(0, 1, 0);
        side = BlockUtils::getBlockPlaceFace(below);
        if (side != -1) blockPos = below;
        else return;
    }

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return;

    int       oldSlot  = supplies->mSelectedSlot;
    glm::vec3 myPos    = *player->getPos();
    glm::vec3 standPos = glm::vec3(blockPos.x + 0.5f, blockPos.y + 2.62f, blockPos.z + 0.5f);

    tpBetween(myPos, standPos);

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());

    if (mSwing.mValue) player->swing();

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

        for (int i = 0; i < 3; i++)
            if (cit->mClickPos[i] == 0.5f)
                cit->mClickPos[i] = MathUtils::randomFloat(-0.49f, 0.49f);

        txn->mTransaction = std::move(cit);
        sender->sendToServer(txn.get());
    }

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    tpBetween(standPos, myPos);
}

// ═══════════════════════════════════════════════════════════════
// Ivec3 comparator for std::set
// ═══════════════════════════════════════════════════════════════

struct Ivec3Cmp {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

// ═══════════════════════════════════════════════════════════════
// Build cage queue
//
// Логика:
// 1. Вычисляем "занятые" блоки — где хитбокс цели пересекается
//    (using 0.3 hitbox margin → up to 4 blocks at feet level)
// 2. Для блоков: стены = соседние блокы НЕ из занятого множества
//    (outer ring — даже если цель между 4 блоками, ставим вокруг них)
// 3. Для паутины: ставим прямо в занятые позиции (web проходит сквозь игрока)
// 4. Сортировка: сначала блокируем путь отхода (velocity > look dir),
//    затем стороны, затем спина. Нижние Y первыми (опора для верхних).
// ═══════════════════════════════════════════════════════════════

void Cage::rebuildQueue()
{
    if (!mTarget) return;

    glm::vec3 tPos = *mTarget->getPos();

    // === Occupied footprint (player hitbox 0.6 wide → 0.3 margin) ===
    int feetY = (int)std::floor(tPos.y - 1.62f);

    int minBX = (int)std::floor(tPos.x - 0.3f);
    int maxBX = (int)std::floor(tPos.x + 0.3f);
    int minBZ = (int)std::floor(tPos.z - 0.3f);
    int maxBZ = (int)std::floor(tPos.z + 0.3f);

    // Occupied blocks at feet AND body level (feetY and feetY+1)
    std::set<glm::ivec3, Ivec3Cmp> occupied;
    for (int bx = minBX; bx <= maxBX; bx++)
        for (int bz = minBZ; bz <= maxBZ; bz++) {
            occupied.insert({bx, feetY,     bz});
            occupied.insert({bx, feetY + 1, bz});
        }

    bool hasWeb    = (findWebSlot()   != -1);
    bool hasBlocks = (findBlockSlot() != -1);

    std::vector<glm::ivec3> positions;
    std::set<glm::ivec3, Ivec3Cmp> seen;

    if (hasWeb) {
        // ═══ Web: place directly at occupied positions ═══
        // Web can be placed even if player stands there
        for (int bx = minBX; bx <= maxBX; bx++)
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                glm::ivec3 p1 = {bx, feetY,     bz};
                glm::ivec3 p2 = {bx, feetY + 1, bz};
                if (seen.insert(p1).second) positions.push_back(p1);
                if (seen.insert(p2).second) positions.push_back(p2);
            }
    } else if (hasBlocks) {
        // ═══ Blocks: outer ring (adjacent to occupied, NOT occupied) ═══

        // Scan expanded area (1 block larger in each XZ direction)
        for (int bx = minBX - 1; bx <= maxBX + 1; bx++)
            for (int bz = minBZ - 1; bz <= maxBZ + 1; bz++) {
                // Skip if this XZ column is occupied at any wall Y level
                bool isOccXZ = false;
                for (int y = 0; y < 2; y++) {
                    if (occupied.count({bx, feetY + y, bz})) {
                        isOccXZ = true;
                        break;
                    }
                }
                if (isOccXZ) continue;

                // Wall blocks at feet + head level
                for (int y = 0; y < 2; y++) {
                    glm::ivec3 bp = {bx, feetY + y, bz};
                    if (seen.insert(bp).second)
                        positions.push_back(bp);
                }
            }

        // Ceiling (above occupied area — at feetY + 2)
        if (mPlaceCeiling.mValue) {
            for (int bx = minBX; bx <= maxBX; bx++)
                for (int bz = minBZ; bz <= maxBZ; bz++) {
                    glm::ivec3 ceil = {bx, feetY + 2, bz};
                    if (seen.insert(ceil).second)
                        positions.push_back(ceil);
                }
        }

        // Floor (below occupied area — at feetY - 1)
        if (mPlaceFloor.mValue) {
            for (int bx = minBX; bx <= maxBX; bx++)
                for (int bz = minBZ; bz <= maxBZ; bz++) {
                    glm::ivec3 floor = {bx, feetY - 1, bz};
                    if (seen.insert(floor).second)
                        positions.push_back(floor);
                }
        }
    }

    mCagePositions = positions;

    // === Sort: escape path first, lower Y first for support ===

    glm::vec3 vel = mTargetVel;
    bool isMoving = glm::length(glm::vec3(vel.x, 0, vel.z)) > 0.01f;
    glm::vec3 priorityDir(0);

    if (isMoving) {
        priorityDir = glm::normalize(glm::vec3(vel.x, 0, vel.z));
    } else {
        auto rot = mTarget->getActorRotationComponent();
        if (rot) {
            float yaw = rot->mYaw * (PI / 180.f);
            priorityDir = glm::vec3(-sinf(yaw), 0, cosf(yaw));
        }
    }

    bool hasPriority = glm::length(priorityDir) > 0.01f;
    glm::vec3 center(tPos.x, 0, tPos.z);

    std::sort(positions.begin(), positions.end(), [&](const glm::ivec3& a, const glm::ivec3& b) {
        // Lower Y first — lower blocks support upper ones
        if (a.y != b.y) return a.y < b.y;

        if (hasPriority) {
            glm::vec3 ac(a.x + 0.5f, 0, a.z + 0.5f);
            glm::vec3 bc(b.x + 0.5f, 0, b.z + 0.5f);
            float da = glm::dot(ac - center, priorityDir);
            float db = glm::dot(bc - center, priorityDir);
            if (std::abs(da - db) > 0.1f) return da > db; // ahead first
        }

        return glm::distance(glm::vec3(a), tPos) < glm::distance(glm::vec3(b), tPos);
    });

    // Filter: only air blocks
    std::vector<glm::ivec3> needed;
    for (auto& p : positions) {
        if (isAirAt(p)) needed.push_back(p);
    }

    mPlaceQueue = std::move(needed);
}

// ═══════════════════════════════════════════════════════════════
// Tick
// ═══════════════════════════════════════════════════════════════

void Cage::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    mTarget = findTarget();
    if (!mTarget || mTarget->isDead()) {
        mTarget = nullptr;
        mPlaceQueue.clear();
        return;
    }

    glm::vec3 curPos = *mTarget->getPos();
    mTargetVel  = curPos - mTargetPrev;
    mTargetPrev = curPos;

    auto rot = player->getActorRotationComponent();
    if (rot) mRots = {rot->mPitch, rot->mYaw, rot->mYaw};

    rebuildQueue();

    if (mPlaceQueue.empty()) return;
    if (NOW - mLastPlace < static_cast<uint64_t>(mDelay.mValue)) return;

    bool hasWeb    = (findWebSlot()   != -1);
    bool hasBlocks = (findBlockSlot() != -1);

    if (!hasWeb && !hasBlocks) return;

    int maxPlace = (int)mBlocksPerTick.mValue;
    int placed   = 0;

    while (placed < maxPlace && !mPlaceQueue.empty()) {
        glm::ivec3 pos = mPlaceQueue.front();
        mPlaceQueue.erase(mPlaceQueue.begin());

        if (!isAirAt(pos)) continue;

        if (hasWeb) placeWebAt(pos, player);
        else        placeBlockAt(pos, player);

        placed++;
    }

    if (placed > 0) mLastPlace = NOW;
}

// ═══════════════════════════════════════════════════════════════
// Render
// ═══════════════════════════════════════════════════════════════

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
        ImColor fill    = ColorUtils::getThemedColor(now * 50.f);
        fill.Value.w    = 0.10f + 0.05f * sinf(now * 4.f);
        ImColor outline = ColorUtils::getThemedColor(now * 50.f + 45.f);
        outline.Value.w = 0.75f;
        RenderUtils::drawOutlinedAABB(box, true,  fill);
        RenderUtils::drawOutlinedAABB(box, false, outline);
    }

    if (mTarget) {
        AABB tbox  = mTarget->getAABB();
        float pulse = 0.5f + 0.25f * sinf(now * 6.f);
        ImColor fill = ImColor(1.f, 0.3f, 0.3f, pulse * 0.5f);
        ImColor out  = ImColor(1.f, 0.2f, 0.2f, 0.9f);
        RenderUtils::drawOutlinedAABB(tbox, true,  fill);
        RenderUtils::drawOutlinedAABB(tbox, false, out);
    }
}