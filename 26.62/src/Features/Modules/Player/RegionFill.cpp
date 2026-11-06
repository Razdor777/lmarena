#include "RegionFill.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MobEquipmentPacket.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/StringUtils.hpp>
#include <algorithm>

// =========================================================
// ДЕТЕКЦИЯ "ЭТИМ ПРЕДМЕТОМ МОЖНО ПОСТАВИТЬ БЛОК"
// У костра, кровати, торта, дверей, табличек и т.п. ItemStack::mBlock == nullptr
// (это не BlockItem), поэтому нужна проверка по имени.
// =========================================================
namespace
{
    const std::vector<std::string> kBlockPlacingItems = {
        "campfire", "soul_campfire",
        "bed", "cake", "door", "sign", "hanging_sign",
        "kelp", "sugar_cane", "reeds", "nether_wart", "flower_pot",
        "hopper", "brewing_stand", "cauldron", "repeater", "comparator",
        "frame", "glow_frame", "skull", "chain", "banner", "bamboo",
        "candle", "lantern", "soul_lantern", "scaffolding",
        "lily_pad", "waterlily", "redstone", "string", "seagrass", "nether_sprouts",
    };

    std::string normalizeName(std::string n)
    {
        n = StringUtils::toLower(n);
        if (n.rfind("item.", 0) == 0) n = n.substr(5);
        else if (n.rfind("tile.", 0) == 0) n = n.substr(5);
        if (n.size() > 5 && n.compare(n.size() - 5, 5, ".name") == 0) n = n.substr(0, n.size() - 5);
        size_t c = n.find(':');
        if (c != std::string::npos) n = n.substr(c + 1);
        return n;
    }

    std::string getStackName(ItemStack* stack)
    {
        if (!stack || !stack->mItem) return "";
        Item* item = stack->getItem();
        if (!item) return "";

        std::string name;
        TRY_CALL([&]() { name = item->mName; });

        if (stack->mBlock)
        {
            BlockLegacy* bl = stack->mBlock->toLegacy();
            if (bl)
            {
                std::string bn;
                TRY_CALL([&]() { bn = bl->mName; });
                if (!bn.empty()) name = bn;
            }
        }
        return normalizeName(name);
    }

    bool nameIsPlaceable(const std::string& n)
    {
        if (n.empty()) return false;
        for (const auto& p : kBlockPlacingItems)
        {
            if (n == p) return true;
            std::string suffix = "_" + p;
            if (n.size() > suffix.size() &&
                n.compare(n.size() - suffix.size(), suffix.size(), suffix) == 0)
                return true;
        }
        return false;
    }

    bool isBlockPlacingItem(ItemStack* stack)
    {
        if (!stack || !stack->mItem || stack->mCount <= 0) return false;
        Item* item = stack->getItem();
        if (!item) return false;

        // 1) Обычный блок
        if (stack->mBlock)
        {
            BlockLegacy* bl = stack->mBlock->toLegacy();
            if (bl && !bl->isAir()) return true;
        }

        // 2) Инструменты / броня / всё с прочностью — точно не блоки
        if (item->getItemType() != SItemType::None) return false;
        if (stack->hasDurability()) return false;

        // 3) Известный предмет, ставящий блок (костёр и т.д.)
        return nameIsPlaceable(getStackName(stack));
    }
}

// =========================================================
// ENABLE / DISABLE
// =========================================================

void RegionFill::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &RegionFill::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &RegionFill::onPacketOutEvent,
        nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,  &RegionFill::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,    &RegionFill::onRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        ChatUtils::displayClientMessage("§cYou must be in a world!");
        setEnabled(false);
        return;
    }

    if (auto supplies = player->getSupplies())
        mHeldSlot = supplies->mSelectedSlot;
    else
        mHeldSlot = -1;

    if (mDebug.mValue) dumpHotbar();

    if (!hasValidSelection()) {
        ChatUtils::displayClientMessage("§cSet pos1 and pos2 first! (.spos1 / .spos2)");
        setEnabled(false);
        return;
    }

    auto rot = player->getActorRotationComponent();
    if (rot) mRots = {rot->mPitch, rot->mYaw, rot->mYaw};

    mBlocksCleared = 0;
    mBlocksPlaced  = 0;
    mClearIndex    = 0;
    mFillIndex     = 0;
    mIsTPing       = false;
    mStartTime     = NOW;
    mLastNoBlocksWarn = 0;

    {
        std::lock_guard<std::mutex> lk(mMutex);
        mPacketPositions.clear();
    }

    buildQueues();

    glm::ivec3 sz     = getSelectionMax() - getSelectionMin() + glm::ivec3(1);
    int        volume = sz.x * sz.y * sz.z;

    if (mClearFirst.mValue && !mClearQueue.empty()) {
        ChatUtils::displayClientMessage(
            "§eRegionFill: §f{}x{}x{} §e(§f{} §evolume)", sz.x, sz.y, sz.z, volume);
        ChatUtils::displayClientMessage(
            "§eClearing §f{} §eblocks, then filling §f{}", mTotalToClear, mTotalToFill);
        if (findAnyPlaceableSlot() == -1)
            ChatUtils::displayClientMessage("§7(no placeable blocks in hotbar yet — add some before clearing finishes)");
        mState = State::Clearing;
    } else {
        if (findAnyPlaceableSlot() == -1) {
            ChatUtils::displayClientMessage("§cNo placeable blocks in hotbar!");
            ChatUtils::displayClientMessage("§7Tip: enable §fUse Held Slot §7or §fDebug §7to see what the module detects.");
            setEnabled(false);
            return;
        }
        ChatUtils::displayClientMessage(
            "§aRegionFill: §f{}x{}x{} §a— filling §f{} §ablocks", sz.x, sz.y, sz.z, mTotalToFill);
        mState = State::Filling;
    }
}

void RegionFill::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,  &RegionFill::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &RegionFill::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,  &RegionFill::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &RegionFill::onRenderEvent>(this);

    if (mState != State::Idle) {
        uint64_t elapsed = (NOW - mStartTime) / 1000;
        ChatUtils::displayClientMessage(
            "§eStopped: §f{} §eplaced, §f{} §ecleared in §f{}s",
            mBlocksPlaced, mBlocksCleared, elapsed);
    }

    mState   = State::Idle;
    mIsTPing = false;
    mClearQueue.clear();
    mFillQueue.clear();
}

// =========================================================
// SELECTION
// =========================================================

bool RegionFill::hasValidSelection()
{
    return SchematicBuilder::sPos1Set && SchematicBuilder::sPos2Set;
}

glm::ivec3 RegionFill::getSelectionMin()
{
    return glm::ivec3(
        std::min(SchematicBuilder::sPos1.x, SchematicBuilder::sPos2.x),
        std::min(SchematicBuilder::sPos1.y, SchematicBuilder::sPos2.y),
        std::min(SchematicBuilder::sPos1.z, SchematicBuilder::sPos2.z));
}

glm::ivec3 RegionFill::getSelectionMax()
{
    return glm::ivec3(
        std::max(SchematicBuilder::sPos1.x, SchematicBuilder::sPos2.x),
        std::max(SchematicBuilder::sPos1.y, SchematicBuilder::sPos2.y),
        std::max(SchematicBuilder::sPos1.z, SchematicBuilder::sPos2.z));
}

// =========================================================
// BUILD QUEUES
// =========================================================

void RegionFill::buildQueues()
{
    mClearQueue.clear();
    mFillQueue.clear();

    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return;

    glm::ivec3 mn = getSelectionMin();
    glm::ivec3 mx = getSelectionMax();

    for (int y = mx.y; y >= mn.y; y--)
    for (int x = mn.x; x <= mx.x; x++)
    for (int z = mn.z; z <= mx.z; z++)
    {
        glm::ivec3 pos(x, y, z);
        Block* block = source->getBlock(x, y, z);

        bool isAir = true;
        if (block && block->mLegacy) isAir = block->mLegacy->isAir();

        if (!isAir && mClearFirst.mValue) mClearQueue.push_back(pos);
        mFillQueue.push_back(pos);
    }

    std::reverse(mFillQueue.begin(), mFillQueue.end());

    mTotalToClear = static_cast<int>(mClearQueue.size());
    mTotalToFill  = static_cast<int>(mFillQueue.size());
    mClearIndex   = 0;
    mFillIndex    = 0;
}

// =========================================================
// HOTBAR
// =========================================================

bool RegionFill::isPlaceableBlock(ItemStack* stack)
{
    return isBlockPlacingItem(stack);
}

void RegionFill::dumpHotbar()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;

    ChatUtils::displayClientMessage("§e--- RegionFill hotbar dump (selected = {}) ---", supplies->mSelectedSlot);
    for (int i = 0; i < 9; i++)
    {
        ItemStack* s = container->getItem(i);
        if (!s || !s->mItem) {
            ChatUtils::displayClientMessage("§7[{}] empty", i);
            continue;
        }
        ChatUtils::displayClientMessage("§7[{}] §f{} §7x{} mBlock={} id={} -> {}",
            i, getStackName(s), (int)s->mCount,
            s->mBlock ? "§ayes§7" : "§cnull§7",
            (int)s->getItem()->mItemId,
            isBlockPlacingItem(s) ? "§aPLACEABLE" : "§cno");
    }
}

int RegionFill::findAnyPlaceableSlot()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    // Режим "Use Held Slot": доверяем игроку — ставим тем, что было в руке
    if (mUseHeldSlot.mValue)
    {
        if (mHeldSlot < 0 || mHeldSlot > 8) return -1;
        ItemStack* s = container->getItem(mHeldSlot);
        if (s && s->mItem && s->mCount > 0) return mHeldSlot;
        return -1;
    }

    for (int i = 0; i < 9; i++)
        if (isPlaceableBlock(container->getItem(i))) return i;
    return -1;
}

int RegionFill::findMixedSlot()
{
    if (mUseHeldSlot.mValue) return findAnyPlaceableSlot();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return findAnyPlaceableSlot();
    auto supplies = player->getSupplies();
    if (!supplies) return findAnyPlaceableSlot();
    auto container = supplies->getContainer();
    if (!container) return findAnyPlaceableSlot();

    std::vector<int> slots;
    std::vector<std::string> seenNames;

    for (int i = 0; i < 9; i++) {
        ItemStack* stack = container->getItem(i);
        if (!isPlaceableBlock(stack)) continue;

        if (mDiverseOnly.mValue) {
            std::string blockName = getStackName(stack);
            bool duplicate = false;
            for (const auto& n : seenNames)
                if (n == blockName) { duplicate = true; break; }
            if (duplicate) continue;
            seenNames.push_back(blockName);
        }
        slots.push_back(i);
    }

    if (slots.empty()) return -1;
    return slots[rand() % (int)slots.size()];
}

// =========================================================
// TP
// =========================================================

std::shared_ptr<MovePlayerPacket> RegionFill::createPacketForPos(glm::vec3 pos)
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

void RegionFill::straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float step = mStepDistance.mValue;
    float dist = glm::length(to - from);

    if (dist < 0.01f) {
        sender->sendToServer(createPacketForPos(to).get());
        return;
    }

    glm::vec3              dir = glm::normalize(to - from);
    glm::vec3              cur = from;
    std::vector<glm::vec3> positions;

    while (glm::distance(cur, to) > step) {
        cur += dir * step;
        positions.push_back(cur);
        sender->sendToServer(createPacketForPos(cur).get());
    }
    positions.push_back(to);
    sender->sendToServer(createPacketForPos(to).get());

    if (saveForRender) {
        std::lock_guard<std::mutex> lk(mMutex);
        mPacketPositions = positions;
        mLastPathTime    = NOW;
    }
}

// =========================================================
// BREAK BLOCK
// =========================================================

void RegionFill::breakBlockAtPos(glm::ivec3 pos, Actor* player)
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

    glm::vec3 playerPos = *player->getPos();
    glm::vec3 standPos  = glm::vec3(pos.x + 0.5f, pos.y + 2.62f, pos.z + 0.5f);

    mIsTPing = true;
    straightLineTP(playerPos, standPos, true);

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(bestTool).get());

    if (mSwing.mValue) player->swing();

    {
        auto pkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        pkt->mPos        = pos;
        pkt->mResultPos  = pos;
        pkt->mFace       = face;
        pkt->mAction     = static_cast<PlayerActionType>(0);
        pkt->mRuntimeId  = player->getRuntimeID();
        pkt->mtIsFromServerPlayerMovementSystem = false;
        sender->sendToServer(pkt.get());
    }
    {
        auto pkt = MinecraftPackets::createPacket<PlayerActionPacket>();
        pkt->mPos        = pos;
        pkt->mResultPos  = pos;
        pkt->mFace       = face;
        pkt->mAction     = PlayerActionType::StopDestroyBlock;
        pkt->mRuntimeId  = player->getRuntimeID();
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
        cit->mPlayerPos            = standPos;
        cit->mClickPos             = {0.5f, 1.0f, 0.5f};
        txn->mTransaction          = std::move(cit);
        sender->sendToServer(txn.get());
    }

    if (bestTool != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(oldSlot).get());

    TRY_CALL([&]() { BlockUtils::clearBlock(pos); });

    straightLineTP(standPos, playerPos, false);
    mIsTPing = false;
}

// =========================================================
// PLACE BLOCK
// =========================================================

bool RegionFill::placeAnyBlockAtPos(glm::ivec3 blockPos, Actor* player)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return false;

    int side = BlockUtils::getBlockPlaceFace(blockPos);
    bool airPlace = false;
    if (side == -1) {
        if (!mAirPlace.mValue) return false;
        airPlace = true;
        side = 1;
    }

    int slot = mMixedBlocks.mValue ? findMixedSlot() : findAnyPlaceableSlot();
    if (slot == -1) return false;

    auto supplies  = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!supplies || !container) return false;

    int       oldSlot   = supplies->mSelectedSlot;
    glm::vec3 playerPos = *player->getPos();
    glm::vec3 standPos  = glm::vec3(blockPos.x + 0.5f, blockPos.y + 2.62f, blockPos.z + 0.5f);

    mIsTPing = true;
    straightLineTP(playerPos, standPos, true);

    if (slot != oldSlot)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());

    if (mSwing.mValue) player->swing();

    {
        auto txn = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ItemUseInventoryTransaction>();
        cit->mActionType           = ItemUseInventoryTransaction::ActionType::Place;
        cit->mSlot                 = slot;
        cit->mItemInHand           = NetworkItemStackDescriptor(*container->getItem(slot));
        cit->mBlockPos             = airPlace ? blockPos : blockPos + glm::ivec3(BlockUtils::blockFaceOffsets[side]);
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

    straightLineTP(standPos, playerPos, false);
    mIsTPing = false;
    return true;
}

// =========================================================
// MAIN TICK
// =========================================================

void RegionFill::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;
    if (mState == State::Idle) return;

    if (NOW - mLastActionTime < static_cast<uint64_t>(mDelay.mValue)) return;

    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return;

    // ---------------- CLEARING ----------------
    if (mState == State::Clearing)
    {
        while (mClearIndex < mClearQueue.size()) {
            glm::ivec3 pos   = mClearQueue[mClearIndex];
            Block*     block = source->getBlock(pos);

            bool isAir = (!block || !block->mLegacy || block->mLegacy->isAir());
            if (isAir) {
                mClearIndex++;
                mBlocksCleared++;
                continue;
            }

            breakBlockAtPos(pos, player);
            mBlocksCleared++;
            mClearIndex++;
            mLastActionTime = NOW;
            return;
        }

        ChatUtils::displayClientMessage("§aCleared §f{} §ablocks! Now filling...", mBlocksCleared);

        if (findAnyPlaceableSlot() == -1) {
            ChatUtils::displayClientMessage("§cNo placeable blocks in hotbar! Add blocks to continue.");
            ChatUtils::displayClientMessage("§7Tip: enable §fUse Held Slot §7or §fDebug§7.");
            setEnabled(false);
            return;
        }

        mState = State::Filling;
        return;
    }

    // ---------------- FILLING ----------------
    if (mState == State::Filling)
    {
        if (findAnyPlaceableSlot() == -1) {
            if (NOW - mLastNoBlocksWarn > 3000) {
                mLastNoBlocksWarn = NOW;
                ChatUtils::displayClientMessage("§cNo placeable blocks in hotbar! Put blocks and wait...");
            }
            return;
        }

        while (mFillIndex < mFillQueue.size()) {
            glm::ivec3 pos   = mFillQueue[mFillIndex];
            Block*     block = source->getBlock(pos);

            bool filled = (block && block->mLegacy && !block->mLegacy->isAir());
            if (filled) { mFillIndex++; continue; }

            if (!mAirPlace.mValue && BlockUtils::getBlockPlaceFace(pos) == -1) { mFillIndex++; continue; }

            if (placeAnyBlockAtPos(pos, player)) {
                mBlocksPlaced++;
                mFillIndex++;
                mLastActionTime = NOW;
                return;
            }
            mFillIndex++;
        }

        bool allFilled = true;
        for (auto& pos : mFillQueue) {
            Block* block = source->getBlock(pos);
            if (!block || !block->mLegacy || block->mLegacy->isAir()) { allFilled = false; break; }
        }

        if (allFilled) {
            uint64_t elapsed = (NOW - mStartTime) / 1000;
            ChatUtils::displayClientMessage(
                "§aDone! §f{} §aplaced, §f{} §acleared in §f{}s",
                mBlocksPlaced, mBlocksCleared, elapsed);
            mState = State::Idle;
            setEnabled(false);
        } else {
            mFillIndex = 0;
        }
    }
}

// =========================================================
// PACKETS
// =========================================================

void RegionFill::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() == PacketID::MovePlayer) {
        auto pkt = event.getPacket<MovePlayerPacket>();
        mRots = {pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot};
    }
}

void RegionFill::onPacketInEvent(PacketInEvent& event)
{
    if (!mSilentAccept.mValue) return;
    if (!mIsTPing) return;

    if (event.mPacket->getId() == PacketID::MovePlayer) {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return;
        auto pkt = event.getPacket<MovePlayerPacket>();
        if (pkt->mPlayerID == player->getRuntimeID()) {
            event.cancel();
            ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
        }
    }
}

// =========================================================
// RENDER
// =========================================================

void RegionFill::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (mShowSelection.mValue && hasValidSelection()) renderSelection();
    if (mShowProgress.mValue && mState != State::Idle)  renderProgress();

    {
        std::lock_guard<std::mutex> lk(mMutex);
        uint64_t now = NOW;
        if (mLastPathTime + 500 < now) { mPacketPositions.clear(); return; }

        float alpha = std::clamp(1.f - float(now - mLastPathTime) / 500.f, 0.f, 1.f);
        if (mPacketPositions.empty()) return;

        auto                drawList = ImGui::GetBackgroundDrawList();
        std::vector<ImVec2> pts;
        for (auto& pos : mPacketPositions) {
            ImVec2 sp;
            if (RenderUtils::worldToScreen(pos, sp)) pts.push_back(sp);
        }
        for (size_t i = 0; i + 1 < pts.size(); i++) {
            ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
            c.Value.w *= alpha;
            drawList->AddLine(pts[i], pts[i + 1], c, 2.f);
        }
    }
}

void RegionFill::renderSelection()
{
    glm::vec3 mn = glm::vec3(getSelectionMin());
    glm::vec3 mx = glm::vec3(getSelectionMax()) + glm::vec3(1.0f);
    AABB box(mn, mx, true);
    RenderUtils::drawOutlinedAABB(box, true, ImColor(0.0f, 0.7f, 1.0f, 0.8f));
}

void RegionFill::renderProgress()
{
    auto   drawList = ImGui::GetBackgroundDrawList();
    ImVec2 ss       = ImGui::GetIO().DisplaySize;

    float barW = 300.f, barH = 22.f;
    float barX = (ss.x - barW) / 2.f;
    float barY = ss.y - 100.f;

    drawList->AddRectFilled(
        {barX - 2, barY - 2}, {barX + barW + 2, barY + barH + 2},
        ImColor(0.f, 0.f, 0.f, 0.8f), 6.f);

    float       progress = 0.f;
    std::string text;
    ImColor     color;

    if (mState == State::Clearing) {
        progress = mTotalToClear > 0 ? float(mClearIndex) / float(mTotalToClear) : 1.f;
        text  = fmt::format("Clearing: {}/{}", mClearIndex, mTotalToClear);
        color = ImColor(1.f, 0.3f, 0.3f, 0.9f);
    } else {
        progress = mTotalToFill > 0 ? float(mBlocksPlaced) / float(mTotalToFill) : 1.f;
        text  = fmt::format("Filling: {}/{}", mBlocksPlaced, mTotalToFill);
        color = ImColor(0.2f, 0.8f, 0.2f, 0.9f);
    }

    drawList->AddRectFilled({barX, barY}, {barX + barW * progress, barY + barH}, color, 4.f);
    drawList->AddRect({barX, barY}, {barX + barW, barY + barH}, ImColor(1.f, 1.f, 1.f, 0.5f), 4.f);

    ImVec2 ts = ImGui::CalcTextSize(text.c_str());
    drawList->AddText({barX + (barW - ts.x) / 2.f, barY + (barH - ts.y) / 2.f},
        ImColor(1.f, 1.f, 1.f, 1.f), text.c_str());

    if (mStartTime > 0) {
        uint64_t    elapsed = (NOW - mStartTime) / 1000;
        std::string t       = fmt::format("{}s", elapsed);
        ImVec2      tts     = ImGui::CalcTextSize(t.c_str());
        drawList->AddText({barX + barW - tts.x, barY - tts.y - 4},
            ImColor(0.7f, 0.7f, 0.7f, 0.8f), t.c_str());
    }
}
