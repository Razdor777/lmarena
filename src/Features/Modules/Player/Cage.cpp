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
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
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

std::shared_ptr<MovePlayerPacket> Cage::makeTPPacket(glm::vec3 pos, const glm::vec2& rots)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto pkt    = MinecraftPackets::createPacket<MovePlayerPacket>();
    pkt->mPos              = pos;
    pkt->mPlayerID         = player->getRuntimeID();
    // Смотрим туда, куда надо для клика: сервер валидирует луч от глаз
    pkt->mRot              = {rots.x, rots.y};
    pkt->mYHeadRot         = rots.y;
    pkt->mResetPosition    = PositionMode::Teleport;
    pkt->mOnGround         = true;
    pkt->mRidingID         = -1;
    pkt->mCause            = TeleportationCause::Unknown;
    pkt->mSourceEntityType = ActorType::Player;
    pkt->mTick             = 0;
    return pkt;
}

void Cage::tpBetween(glm::vec3 from, glm::vec3 to, const glm::vec2& rots)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float step = mStepDist.mValue;
    float dist = glm::length(to - from);
    glm::vec3 dir = dist > 0.001f ? glm::normalize(to - from) : glm::vec3(0.f);

    for (float d = step; d < dist; d += step)
        sender->sendToServer(makeTPPacket(from + dir * d, rots).get());
    sender->sendToServer(makeTPPacket(to, rots).get());
}

// ═══════════════════════════════════════════════════════════════
// Placement plan
//
// Главная причина, почему потолок и пол «не ставились»: и старый,
// и текущий код всегда вставал ПРЯМО НАД клеткой. Оттуда луч бьёт
// в верхнюю грань блока снизу и мимо боковых граней — а сервер
// сверяет грань, которую мы отправили, с той, в которую реально
// попал луч, и отбрасывает блок.
//
// Теперь позиция фейкового игрока считается ПОД конкретную грань:
//   face 1  — блок снизу, клик по верхней грани → стоим сверху
//   face 0  — блок сверху, клик по нижней    → стоим снизу
//   face 2-5- боковой сосед, клик по боку    → стоим с его стороны
// ═══════════════════════════════════════════════════════════════

Cage::PlacePlan Cage::buildPlacePlan(glm::ivec3 cell)
{
    PlacePlan plan;
    glm::vec3 center = glm::vec3(cell) + glm::vec3(0.5f);

    static const std::vector<int> faceOrder = { 1, 2, 3, 4, 5, 0 };

    for (int f : faceOrder)
    {
        glm::vec3  offset    = glm::vec3(BlockUtils::blockFaceOffsets[f]);
        glm::ivec3 neighbour = cell + glm::ivec3(BlockUtils::blockFaceOffsets[f]);
        if (isAirAt(neighbour)) continue;

        glm::vec3 stand;
        if (f == 1)
        {
            // Стоим НА клетке (ровно как RegionFill) и смотрим вниз —
            // луч бьёт в верхнюю грань блока под клеткой.
            stand = { center.x, cell.y + 2.62f, center.z };
        }
        else
        {
            // Встаём чуть ниже клетки со стороны, ОТКУДА видно её соседа,
            // и целиком под клеткой — тогда тело не пересекает ставимый
            // блок, а луч доходит до нужной грани и не попадает в другой.
            stand   = center - offset * 0.5f;
            stand.y = cell.y - 0.18f;
        }

        // Точка на грани кликаемого блока, повёрнутая к нашей клетке
        glm::vec3 faceCenter = glm::vec3(neighbour) + glm::vec3(0.5f) - offset * 0.5f;

        plan.rots = MathUtils::getRots(stand, faceCenter);
        // Лёгкий разброс, чтобы взгляд не был идеально механическим
        plan.rots.x = MathUtils::clamp(plan.rots.x + MathUtils::randomFloat(-1.1f, 1.1f), -90.f, 90.f);
        plan.rots.y = MathUtils::wrap(plan.rots.y  + MathUtils::randomFloat(-1.1f, 1.1f), -180.f, 180.f);

        plan.valid   = true;
        plan.face    = f;
        plan.clicked = neighbour;
        plan.stand   = stand;
        break;
    }

    if (!plan.valid)
    {
        // Ни одного соседа — AirPlace (принимают лояльные сервера)
        plan.valid    = true;
        plan.airPlace = true;
        plan.face     = 1;
        plan.clicked  = cell;
        plan.stand    = { center.x, cell.y + 2.62f, center.z };
        plan.rots     = { 89.5f, mRots.y };
    }

    return plan;
}

bool Cage::placeAt(glm::ivec3 cell, Actor* player, int slot)
{
    if (slot < 0) return false;

    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return false;

    PlacePlan plan = buildPlacePlan(cell);
    if (!plan.valid) return false;

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return false;

    auto* stack = container->getItem(slot);
    if (!stack || !stack->mItem || stack->mCount <= 0) return false;

    int       oldSlot = supplies->mSelectedSlot;
    glm::vec3 myPos   = *player->getPos();
    glm::vec3 oldRots = mRots;

    tpBetween(myPos, plan.stand, plan.rots);

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());

    if (mSwing.mValue) player->swing();

    {
        auto txn = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ItemUseInventoryTransaction>();
        cit->mActionType           = ItemUseInventoryTransaction::ActionType::Place;
        cit->mSlot                 = slot;
        cit->mItemInHand           = NetworkItemStackDescriptor(*stack);
        cit->mBlockPos             = plan.clicked;
        cit->mFace                 = plan.face;
        cit->mTargetBlockRuntimeId = 0;
        cit->mPlayerPos            = plan.stand;
        cit->mClickPos             = BlockUtils::clickPosOffsets[plan.face];

        for (int i = 0; i < 3; i++)
            if (cit->mClickPos[i] == 0.5f)
                cit->mClickPos[i] = MathUtils::randomFloat(-0.49f, 0.49f);

        txn->mTransaction = std::move(cit);
        sender->sendToServer(txn.get());
    }

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    tpBetween(plan.stand, myPos, oldRots);
    mRots = oldRots;

    return true;
}

bool Cage::placeBlockAt(glm::ivec3 blockPos, Actor* player)
{
    return placeAt(blockPos, player, findBlockSlot());
}

void Cage::placeWebAt(glm::ivec3 blockPos, Actor* player)
{
    placeAt(blockPos, player, findWebSlot());
}

// ═══════════════════════════════════════════════════════════════
// Break block (под целью / под полом)
// ═══════════════════════════════════════════════════════════════

void Cage::breakBlockAt(glm::ivec3 pos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    auto source = ClientInstance::get()->getBlockSource();
    if (!sender || !source) return;

    Block* block = source->getBlock(pos);
    if (!block || !block->mLegacy || block->mLegacy->isAir()) return;

    int face = BlockUtils::getExposedFace(pos);
    if (face == -1) face = 1;

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return;

    int oldSlot  = supplies->mSelectedSlot;
    int bestTool = ItemUtils::getBestBreakingTool(block, false);
    if (bestTool < 0) bestTool = oldSlot;

    glm::vec3 stand = glm::vec3(pos) + glm::vec3(0.5f, 2.62f, 0.5f);
    glm::vec2 rots  = MathUtils::getRots(stand, glm::vec3(pos) + glm::vec3(0.5f));

    glm::vec3 myPos   = *player->getPos();
    glm::vec3 oldRots = mRots;

    tpBetween(myPos, stand, rots);

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(bestTool).get());

    if (mSwing.mValue) player->swing();

    {
        auto pkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        pkt->mPos       = pos;
        pkt->mResultPos = pos;
        pkt->mFace      = face;
        pkt->mAction    = static_cast<PlayerActionType>(0); // StartDestroyBlock
        pkt->mRuntimeId = player->getRuntimeID();
        pkt->mtIsFromServerPlayerMovementSystem = false;
        sender->sendToServer(pkt.get());
    }
    {
        auto pkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        pkt->mPos       = pos;
        pkt->mResultPos = pos;
        pkt->mFace      = face;
        pkt->mAction    = PlayerActionType::StopDestroyBlock;
        pkt->mRuntimeId = player->getRuntimeID();
        pkt->mtIsFromServerPlayerMovementSystem = false;
        sender->sendToServer(pkt.get());
    }
    {
        auto txn = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ItemUseInventoryTransaction>();
        cit->mActionType           = ItemUseInventoryTransaction::ActionType::Destroy;
        cit->mSlot                 = bestTool;
        cit->mItemInHand           = NetworkItemStackDescriptor(*container->getItem(bestTool));
        cit->mBlockPos             = pos;
        cit->mFace                 = face;
        cit->mTargetBlockRuntimeId = 0;
        cit->mPlayerPos            = stand;
        cit->mClickPos             = {0.5f, 1.0f, 0.5f};
        txn->mTransaction          = std::move(cit);
        sender->sendToServer(txn.get());
    }

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    // Локально сразу убираем блок — иначе модуль будет бить его повторно
    BlockUtils::clearBlock(pos);

    tpBetween(stand, myPos, oldRots);
    mRots = oldRots;
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
// Полноценная коробка, ставится снизу вверх, чтобы каждая следующая
// клетка имела соседа, по которому можно кликнуть:
//   1. стены (feetY, feetY + 1)
//   2. кольцо над стенами (feetY + 2)
//   3. центр потолка (feetY + 2) — кликается по боковой грани кольца
//   4. пол (feetY - 1) — после расчистки блока под целью
// ═══════════════════════════════════════════════════════════════

void Cage::rebuildQueue()
{
    if (!mTarget) return;

    glm::vec3 tPos = *mTarget->getPos();

    int feetY = (int)std::floor(tPos.y - 1.62f);

    int minBX = (int)std::floor(tPos.x - 0.3f);
    int maxBX = (int)std::floor(tPos.x + 0.3f);
    int minBZ = (int)std::floor(tPos.z - 0.3f);
    int maxBZ = (int)std::floor(tPos.z + 0.3f);

    auto inFootprint = [&](int bx, int bz) {
        return bx >= minBX && bx <= maxBX && bz >= minBZ && bz <= maxBZ;
    };

    bool hasWeb    = (findWebSlot()   != -1);
    bool hasBlocks = (findBlockSlot() != -1);

    std::set<glm::ivec3, Ivec3Cmp> seen;
    std::vector<std::vector<glm::ivec3>> stages;

    auto push = [&](std::vector<glm::ivec3>& stage, glm::ivec3 p) {
        if (seen.insert(p).second) stage.push_back(p);
    };

    if (hasWeb)
    {
        // Паутина ставится прямо в клетки цели — она проходит сквозь игрока
        std::vector<glm::ivec3> web;
        for (int bx = minBX; bx <= maxBX; bx++)
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                push(web, {bx, feetY,     bz});
                push(web, {bx, feetY + 1, bz});
            }
        stages.push_back(std::move(web));
    }
    else if (hasBlocks)
    {
        // 1. Стены: сначала нижний уровень (на нём держатся верхние)
        std::vector<glm::ivec3> walls;
        for (int y = 0; y < 2; y++)
            for (int bx = minBX - 1; bx <= maxBX + 1; bx++)
                for (int bz = minBZ - 1; bz <= maxBZ + 1; bz++) {
                    if (inFootprint(bx, bz)) continue;
                    push(walls, {bx, feetY + y, bz});
                }
        stages.push_back(std::move(walls));

        if (mPlaceCeiling.mValue)
        {
            // 2. Кольцо над стенами — по нему потом закроется центр
            std::vector<glm::ivec3> ring;
            for (int bx = minBX - 1; bx <= maxBX + 1; bx++)
                for (int bz = minBZ - 1; bz <= maxBZ + 1; bz++) {
                    if (inFootprint(bx, bz)) continue;
                    push(ring, {bx, feetY + 2, bz});
                }
            stages.push_back(std::move(ring));

            // 3. Центр потолка
            std::vector<glm::ivec3> ceiling;
            for (int bx = minBX; bx <= maxBX; bx++)
                for (int bz = minBZ; bz <= maxBZ; bz++)
                    push(ceiling, {bx, feetY + 2, bz});
            stages.push_back(std::move(ceiling));
        }

        if (mPlaceFloor.mValue)
        {
            // 4. Пол под целью
            std::vector<glm::ivec3> floor;
            for (int bx = minBX; bx <= maxBX; bx++)
                for (int bz = minBZ; bz <= maxBZ; bz++)
                    push(floor, {bx, feetY - 1, bz});
            stages.push_back(std::move(floor));
        }
    }

    // Приоритет: сначала перекрываем путь отхода, потом ближайшие
    glm::vec3 vel    = mTargetVel;
    bool      moving = glm::length(glm::vec3(vel.x, 0, vel.z)) > 0.01f;
    glm::vec3 priorityDir(0);

    if (moving) {
        priorityDir = glm::normalize(glm::vec3(vel.x, 0, vel.z));
    } else {
        auto rot = mTarget->getActorRotationComponent();
        if (rot) {
            float yaw = rot->mYaw * (PI / 180.f);
            priorityDir = glm::vec3(-sinf(yaw), 0, cosf(yaw));
        }
    }

    bool      hasPriority = glm::length(priorityDir) > 0.01f;
    glm::vec3 center(tPos.x, 0, tPos.z);

    auto cmp = [&](const glm::ivec3& a, const glm::ivec3& b) {
        if (a.y != b.y) return a.y < b.y; // нижние первыми — опора для верхних

        if (hasPriority) {
            glm::vec3 ac(a.x + 0.5f, 0, a.z + 0.5f);
            glm::vec3 bc(b.x + 0.5f, 0, b.z + 0.5f);
            float da = glm::dot(ac - center, priorityDir);
            float db = glm::dot(bc - center, priorityDir);
            if (std::abs(da - db) > 0.1f) return da > db;
        }

        return glm::distance(glm::vec3(a), tPos) < glm::distance(glm::vec3(b), tPos);
    };

    mCagePositions.clear();
    mPlaceQueue.clear();

    for (auto& stage : stages)
    {
        std::sort(stage.begin(), stage.end(), cmp);
        for (auto& p : stage)
        {
            mCagePositions.push_back(p);
            if (isAirAt(p)) mPlaceQueue.push_back(p);
        }
    }
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
        mCagePositions.clear();
        return;
    }

    glm::vec3 curPos = *mTarget->getPos();
    mTargetVel  = curPos - mTargetPrev;
    mTargetPrev = curPos;

    auto rot = player->getActorRotationComponent();
    if (rot) mRots = {rot->mPitch, rot->mYaw, rot->mYaw};

    if (NOW - mLastPlace < static_cast<uint64_t>(mDelay.mValue)) return;

    // ── Пол: сначала сносим блок, на котором стоит цель ──────────────
    if (mPlaceFloor.mValue && mBreakUnder.mValue)
    {
        int feetY = (int)std::floor(curPos.y - 1.62f);
        glm::ivec3 under = {
            (int)std::floor(curPos.x),
            feetY - 1,
            (int)std::floor(curPos.z)
        };

        if (!isAirAt(under))
        {
            breakBlockAt(under, player);
            mLastPlace = NOW;
            return;
        }
    }

    rebuildQueue();

    if (mPlaceQueue.empty()) return;

    bool hasWeb    = (findWebSlot()   != -1);
    bool hasBlocks = (findBlockSlot() != -1);
    if (!hasWeb && !hasBlocks) return;

    int maxPlace = (int)mBlocksPerTick.mValue;
    int placed   = 0;

    while (placed < maxPlace && !mPlaceQueue.empty()) {
        glm::ivec3 pos = mPlaceQueue.front();
        mPlaceQueue.erase(mPlaceQueue.begin());

        if (!isAirAt(pos)) continue;

        if (hasWeb) { if (!placeAt(pos, player, findWebSlot()))   continue; }
        else        { if (!placeAt(pos, player, findBlockSlot())) continue; }

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
