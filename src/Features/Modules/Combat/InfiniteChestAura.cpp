#include "InfiniteChestAura.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Player/ChestStealer.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/Actor/Components/AttributesComponent.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/ContainerClosePacket.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/FileUtils.hpp>
#include <algorithm>
#include <cctype>
#include <regex>

static ChestStealer* sChestStealer = nullptr;

// =========================================================
// ENABLE / DISABLE
// =========================================================

void InfiniteChestAura::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &InfiniteChestAura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &InfiniteChestAura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &InfiniteChestAura::onRenderEvent>(this);

    sChestStealer = gFeatureManager->mModuleManager->getModule<ChestStealer>();

    mState = State::Idle;
    mHasTarget = false;
    mIsChestOpen = false;
    mChestStealerWasEnabled = false;
    mStateStartTime = NOW;
    mNeedsQuickScan = true;
    mFoundChests.clear();
    mTiming = {};
    mIsEating = false;
    mEatSlot = -1;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPacketPositions.clear();
    }

    resetScanner();
    loadChestMemory();

    if (mDebug.mValue)
        NotifyUtils::notify("InfiniteChestAura ON", 2.f, Notification::Type::Info);
}

void InfiniteChestAura::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &InfiniteChestAura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &InfiniteChestAura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &InfiniteChestAura::onRenderEvent>(this);

    if (sChestStealer && mChestStealerWasEnabled)
        sChestStealer->setEnabled(false);

    if (mIsChestOpen)
        closeChest();

    saveChestMemory();

    mState = State::Idle;
    mHasTarget = false;
    mIsChestOpen = false;
    mFoundChests.clear();
    mIsEating = false;
    mEatSlot = -1;

    std::lock_guard<std::mutex> lock(mMutex);
    mPacketPositions.clear();
}

// =========================================================
// STATE UTILS
// =========================================================

void InfiniteChestAura::setState(State newState)
{
    if (mDebug.mValue && mState != newState)
        ChatUtils::displayClientMessage("§7[ICA] §e{} §f-> §a{}", getStateName(mState), getStateName(newState));
    mState = newState;
    mStateStartTime = NOW;
}

uint64_t InfiniteChestAura::getStateTime()
{
    return NOW - mStateStartTime;
}

std::string InfiniteChestAura::getStateName(State state)
{
    switch (state) {
        case State::Idle:              return "Idle";
        case State::WaitAfterTP:       return "WaitTP";
        case State::OpeningChest:      return "Opening";
        case State::WaitingForOpen:    return "WaitOpen";
        case State::WaitingForStealer: return "Stealing";
        case State::WaitingForEvent:   return "Waiting";
        default:                       return "?";
    }
}

bool InfiniteChestAura::containsTriggerWord(const std::string& message)
{
    if (message.empty()) return false;
    std::string lower = message;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });
    for (const auto& kw : mTriggerKeywords) {
        std::string lkw = kw;
        std::transform(lkw.begin(), lkw.end(), lkw.begin(),
            [](unsigned char c) { return std::tolower(c); });
        if (lower.find(lkw) != std::string::npos) return true;
    }
    return false;
}

void InfiniteChestAura::logCycleTiming()
{
    if (!mDebug.mValue) return;

    uint64_t tpTo = mTiming.tpToEnd - mTiming.tpToStart;
    uint64_t wait = mTiming.waitEnd - mTiming.tpToEnd;
    uint64_t open = mTiming.openEnd - mTiming.waitEnd;
    uint64_t steal = mTiming.stealEnd - mTiming.openEnd;
    uint64_t tpBack = mTiming.tpBackEnd - mTiming.stealEnd;
    uint64_t total = mTiming.tpBackEnd - mTiming.cycleStart;

    ChatUtils::displayClientMessage("§7[ICA] §6=== CYCLE #{} ===", mTiming.cycleNumber);
    ChatUtils::displayClientMessage("§7[ICA] §fChest: §e({}, {}, {}) §fdist: §a{:.1f}",
        mTiming.chestPos.x, mTiming.chestPos.y, mTiming.chestPos.z, mTiming.distance);
    ChatUtils::displayClientMessage("§7[ICA] §fTP->chest: §e{}ms", tpTo);
    ChatUtils::displayClientMessage("§7[ICA] §fWait:      §e{}ms", wait);
    ChatUtils::displayClientMessage("§7[ICA] §fOpen:      §e{}ms", open);
    ChatUtils::displayClientMessage("§7[ICA] §fSteal:     §e{}ms", steal);
    ChatUtils::displayClientMessage("§7[ICA] §fTP->back:  §e{}ms", tpBack);
    ChatUtils::displayClientMessage("§7[ICA] §fTotal:     §a{}ms", total);
}

// =========================================================
// SCANNER — chunk based (OreMiner style)
// =========================================================

void InfiniteChestAura::resetScanner()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    mScan.center = ChunkPos(*player->getPos());
    mScan.current = mScan.center;
    mScan.subChunkIdx = 0;
    mScan.dirIdx = 0;
    mScan.steps = 1;
    mScan.stepCount = 0;
}

void InfiniteChestAura::moveToNextChunk()
{
    static const std::pair<int, int> dirs[] = {{1,0},{0,1},{-1,0},{0,-1}};
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) { resetScanner(); return; }
    size_t numSubs = (source->getBuildHeight() - source->getBuildDepth()) / 16;
    if ((size_t)mScan.subChunkIdx < numSubs - 1) { mScan.subChunkIdx++; return; }
    mScan.current.x += dirs[mScan.dirIdx].first;
    mScan.current.y += dirs[mScan.dirIdx].second;
    mScan.stepCount++;
    if (mScan.stepCount >= mScan.steps) {
        mScan.stepCount = 0;
        mScan.dirIdx = (mScan.dirIdx + 1) % 4;
        if (mScan.dirIdx % 2 == 0) mScan.steps++;
    }
    mScan.subChunkIdx = 0;
}

bool InfiniteChestAura::isValidChest(int blockId)
{
    if (blockId == CHEST_ID || blockId == BARREL_ID) return true;
    if (blockId == ENDER_CHEST_ID && !mIgnoreEnder.mValue) return true;
    if (blockId == TRAPPED_CHEST_ID && !mIgnoreTrapped.mValue) return true;
    return false;
}

bool InfiniteChestAura::scanSubChunk(ChunkPos chunk, int subIdx)
{
    auto source = ClientInstance::get()->getBlockSource();
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

        int blockId = block->mLegacy->getBlockId();
        if (!isValidChest(blockId)) continue;

        BlockPos pos;
        pos.x = chunk.x * 16 + x;
        pos.z = chunk.y * 16 + z;
        pos.y = y + (sub.subchunkIndex * 16);

        if (mIgnoreOpened.mValue && isChestOpened(pos)) continue;
        if (mFoundChests.find(pos) == mFoundChests.end())
            mFoundChests[pos] = {pos, blockId};
    }
    return true;
}

void InfiniteChestAura::quickScan()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto source = ClientInstance::get()->getBlockSource();
    if (!player || !source) return;

    glm::vec3 pp = *player->getPos();
    int radius = std::min(static_cast<int>(mSearchRadius.mValue), 80);
    int minY = static_cast<int>(pp.y) - 30;
    int maxY = static_cast<int>(pp.y) + 30;

    // Clamp to world bounds
    int buildDepth = static_cast<int>(source->getBuildDepth());
    int buildHeight = static_cast<int>(source->getBuildHeight());
    if (minY < buildDepth) minY = buildDepth;
    if (maxY > buildHeight) maxY = buildHeight;

    for (int x = static_cast<int>(pp.x) - radius; x <= static_cast<int>(pp.x) + radius; x++)
    for (int z = static_cast<int>(pp.z) - radius; z <= static_cast<int>(pp.z) + radius; z++)
    {
        float dxz = static_cast<float>((x - (int)pp.x) * (x - (int)pp.x) +
                                        (z - (int)pp.z) * (z - (int)pp.z));
        if (dxz > radius * radius) continue;

        for (int y = minY; y <= maxY; y++)
        {
            Block* block = source->getBlock(x, y, z);
            if (!block || !block->mLegacy) continue;

            int blockId = block->mLegacy->getBlockId();
            if (!isValidChest(blockId)) continue;

            glm::ivec3 bpos(x, y, z);
            if (mIgnoreOpened.mValue && isChestOpened(bpos)) continue;

            BlockPos bp;
            bp.x = x; bp.y = y; bp.z = z;
            if (mFoundChests.find(bp) == mFoundChests.end())
                mFoundChests[bp] = {bpos, blockId};
        }
    }

    if (mDebug.mValue)
        ChatUtils::displayClientMessage("§7[ICA] §fQuickScan: §e{} §fchests found", mFoundChests.size());
}

void InfiniteChestAura::runChunkScanner()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (glm::distance(glm::vec2(mScan.current), glm::vec2(mScan.center)) > CHUNK_RADIUS) {
        mScan.center = ChunkPos(*player->getPos());
        mScan.current = mScan.center;
        mScan.stepCount = 0; mScan.steps = 1;
        mScan.dirIdx = 0; mScan.subChunkIdx = 0;
    }

    for (int i = 0; i < CHUNKS_PER_TICK; i++) {
        TRY_CALL([&]() { scanSubChunk(mScan.current, mScan.subChunkIdx); });
        moveToNextChunk();
    }
}

void InfiniteChestAura::cleanupFoundChests()
{
    auto source = ClientInstance::get()->getBlockSource();
    if (!source) return;

    for (auto it = mFoundChests.begin(); it != mFoundChests.end();)
    {
        Block* block = source->getBlock(it->first);
        if (!block || !isValidChest(block->mLegacy->getBlockId())) {
            it = mFoundChests.erase(it);
        } else if (mIgnoreOpened.mValue && isChestOpened(it->first)) {
            it = mFoundChests.erase(it);
        } else {
            ++it;
        }
    }
}

// =========================================================
// TP — AutoLoot style (packet-only, setPosition at end)
// =========================================================

std::shared_ptr<MovePlayerPacket> InfiniteChestAura::createPacketForPos(Actor* player, glm::vec3 pos)
{
    auto packet = MinecraftPackets::createPacket<MovePlayerPacket>();
    packet->mPos = pos;
    packet->mPlayerID = player->getRuntimeID();

    auto rot = player->getActorRotationComponent();
    if (rot) {
        packet->mRot = {rot->mPitch, rot->mYaw};
        packet->mYHeadRot = rot->mYaw;
    }

    packet->mResetPosition = PositionMode::Teleport;
    packet->mOnGround = true;
    packet->mRidingID = -1;
    packet->mCause = TeleportationCause::Unknown;
    packet->mSourceEntityType = ActorType::Player;
    packet->mTick = 0;
    return packet;
}

void InfiniteChestAura::straightLineTP(Actor* player, glm::vec3 from, glm::vec3 to, bool saveForRender)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float stepSize = mStepDistance.mValue;
    glm::vec3 diff = to - from;
    float totalDist = glm::length(diff);

    if (totalDist < 0.01f) {
        sender->sendToServer(createPacketForPos(player, to).get());
        return;
    }

    glm::vec3 direction = glm::normalize(diff);
    glm::vec3 currentPos = from;
    std::vector<glm::vec3> positions;

    while (glm::distance(currentPos, to) > stepSize) {
        currentPos += direction * stepSize;
        positions.push_back(currentPos);
        sender->sendToServer(createPacketForPos(player, currentPos).get());
    }

    positions.push_back(to);
    sender->sendToServer(createPacketForPos(player, to).get());

    if (saveForRender) {
        std::lock_guard<std::mutex> lock(mMutex);
        mPacketPositions = positions;
        mLastPathTime = NOW;
    }
}

// =========================================================
// CHEST OPERATIONS
// =========================================================

glm::ivec3 InfiniteChestAura::findNearestChest(Actor* player)
{
    glm::vec3 pp = *player->getPos();
    glm::ivec3 best = {INT_MAX, INT_MAX, INT_MAX};
    float bestDist = FLT_MAX;

    for (auto& [bpos, info] : mFoundChests)
    {
        if (mIgnoreOpened.mValue && isChestOpened(info.position)) continue;

        float dist = glm::distance(glm::vec3(info.position), pp);

        if (dist < bestDist) {
            bestDist = dist;
            best = info.position;
        }
    }
    return best;
}

void InfiniteChestAura::openChest(Actor* player, glm::ivec3 pos)
{
    if (!player) return;

    glm::vec3 playerPos = *player->getPos();
    int bestFace = 1;
    float minDist = FLT_MAX;

    for (auto& [face, offset] : BlockUtils::blockFaceOffsets) {
        glm::vec3 facePos = glm::vec3(pos) + offset;
        float dist = glm::distance(facePos, playerPos);
        if (dist < minDist) { minDist = dist; bestFace = face; }
    }

    player->getGameMode()->buildBlock(pos, bestFace, false);
}

void InfiniteChestAura::closeChest()
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;
    auto packet = MinecraftPackets::createPacket<ContainerClosePacket>();
    packet->mContainerId = ContainerID::Chest;
    packet->mServerInitiatedClose = false;
    sender->sendToServer(packet.get());
    mIsChestOpen = false;
}

// =========================================================
// MEMORY
// =========================================================

bool InfiniteChestAura::isChestOpened(glm::ivec3 pos)
{
    return mOpenedChests.find(pos) != mOpenedChests.end();
}

void InfiniteChestAura::markChestAsOpened(glm::ivec3 pos)
{
    if (isChestOpened(pos)) return;
    mOpenedChests.insert(pos);
    mOpenedChestsOrder.push_back(pos);
    while (mOpenedChestsOrder.size() > MAX_MEMORY) {
        mOpenedChests.erase(mOpenedChestsOrder.front());
        mOpenedChestsOrder.pop_front();
    }
    saveChestMemory();
}

std::string InfiniteChestAura::getMemoryFilePath()
{
    return FileUtils::getSolsticeDir() + "\\chest_memory.json";
}

void InfiniteChestAura::saveChestMemory()
{
    if (!mPersistentMemory.mValue) return;
    try {
        nlohmann::json j;
        j["chests"] = nlohmann::json::array();
        j["totalLooted"] = mChestsLooted;
        for (const auto& pos : mOpenedChests) {
            j["chests"].push_back({{"x", pos.x}, {"y", pos.y}, {"z", pos.z}});
        }
        std::ofstream file(getMemoryFilePath());
        if (file.is_open()) { file << j.dump(2); file.close(); }
    } catch (...) {}
}

void InfiniteChestAura::loadChestMemory()
{
    if (!mPersistentMemory.mValue) return;
    try {
        std::ifstream file(getMemoryFilePath());
        if (!file.is_open()) return;
        nlohmann::json j; file >> j; file.close();

        if (j.contains("totalLooted"))
            mChestsLooted = j["totalLooted"].get<int>();

        if (j.contains("chests")) {
            for (const auto& c : j["chests"]) {
                glm::ivec3 pos(c["x"].get<int>(), c["y"].get<int>(), c["z"].get<int>());
                if (!isChestOpened(pos)) {
                    mOpenedChests.insert(pos);
                    mOpenedChestsOrder.push_back(pos);
                }
            }
            while (mOpenedChestsOrder.size() > MAX_MEMORY) {
                mOpenedChests.erase(mOpenedChestsOrder.front());
                mOpenedChestsOrder.pop_front();
            }
        }

        if (mDebug.mValue)
            ChatUtils::displayClientMessage("§7[ICA] §aLoaded §e{} §achests (looted: §e{}§a)",
                mOpenedChests.size(), mChestsLooted);
    } catch (...) {}
}

// =========================================================
// AUTO EAT
// =========================================================

bool InfiniteChestAura::isFoodItem(const std::string& name)
{
    std::string n = name;
    std::transform(n.begin(), n.end(), n.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // Удочки-приманки не еда
    if (n.find("on_a_stick") != std::string::npos) return false;

    static const std::vector<std::string> foods = {
        "apple", "bread", "porkchop", "beef", "chicken", "mutton", "rabbit",
        "cod", "salmon", "tropical_fish", "cookie", "melon_slice", "carrot",
        "potato", "beetroot", "mushroom_stew", "pumpkin_pie", "dried_kelp",
        "sweet_berries", "glow_berries", "chorus_fruit", "honey_bottle",
        "rotten_flesh", "spider_eye", "suspicious_stew", "pufferfish"
    };
    for (const auto& f : foods) {
        if (n.find(f) != std::string::npos) return true;
    }
    return false;
}

int InfiniteChestAura::findFoodSlot(Actor* player)
{
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    for (int i = 0; i < 9; i++) {
        auto stack = container->getItem(i);
        if (!stack || !stack->mItem) continue;
        if (stack->mCount <= 0) continue;
        auto item = stack->getItem();
        if (!item || item->mItemId == 0) continue;
        if (isFoodItem(item->mName)) return i;
    }
    return -1;
}

void InfiniteChestAura::updateAutoEat(Actor* player)
{
    if (!player) { mIsEating = false; return; }

    auto supplies = player->getSupplies();
    if (!supplies) { mIsEating = false; return; }
    auto container = supplies->getContainer();
    if (!container) { mIsEating = false; return; }

    auto stopEating = [&]() {
        supplies->mSelectedSlot = mEatPrevSlot;
        auto sender = ClientInstance::get()->getPacketSender();
        if (sender)
            sender->sendToServer(PacketUtils::createMobEquipmentPacket(mEatPrevSlot).get());
        mIsEating = false;
    };

    if (!mAutoEat.mValue) {
        if (mIsEating) stopEating();
        return;
    }

    float hunger = 20.f, maxHunger = 20.f;
    auto attr = player->getAttribute(AttributeId::PlayerHunger);
    if (attr) { hunger = attr->mCurrentValue; maxHunger = attr->mMaximumValue; }

    if (mIsEating) {
        auto stack = container->getItem(mEatSlot);
        bool stillFood = stack && stack->mItem && stack->mCount > 0 &&
                         isFoodItem(stack->getItem()->mName);

        if (hunger >= maxHunger || !stillFood || NOW - mEatStartTime > 5000) {
            stopEating();
            return;
        }

        // Продолжаем есть: держим еду в руке и «зажимаем» ПКМ
        supplies->mSelectedSlot = mEatSlot;
        player->getGameMode()->baseUseItem(stack);
        return;
    }

    // Не начинаем есть посреди цикла сундука / с открытым контейнером
    if (mState != State::Idle || mIsChestOpen) return;
    if (hunger > mAutoEatHunger.mValue) return;

    int slot = findFoodSlot(player);
    if (slot == -1) return;

    mEatPrevSlot = supplies->mSelectedSlot;
    mEatSlot = slot;
    mIsEating = true;
    mEatStartTime = NOW;

    supplies->mSelectedSlot = slot;
    auto sender = ClientInstance::get()->getPacketSender();
    if (sender)
        sender->sendToServer(PacketUtils::createMobEquipmentPacket(slot).get());
    player->getGameMode()->baseUseItem(container->getItem(slot));

    if (mDebug.mValue)
        ChatUtils::displayClientMessage("§7[ICA] §fAutoEat: eating from slot §e{}", slot);
}

// =========================================================
// MAIN TICK
// =========================================================

void InfiniteChestAura::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    // === Handle utility buttons ===
    if (mClearMemory.mValue) {
        mClearMemory.mValue = false;
        int count = static_cast<int>(mOpenedChests.size());
        mOpenedChests.clear();
        mOpenedChestsOrder.clear();
        mFoundChests.clear();
        mNeedsQuickScan = true;
        resetScanner();
        NotifyUtils::notify(fmt::format("Cleared {} chests", count), 2.f, Notification::Type::Info);
    }

    if (mForceStart.mValue) {
        mForceStart.mValue = false;
        mNeedsQuickScan = true;
        mFoundChests.clear();
        resetScanner();
        if (mState == State::WaitingForEvent) setState(State::Idle);
        NotifyUtils::notify("Force started!", 2.f, Notification::Type::Info);
    }

    // === AutoEat ===
    updateAutoEat(player);

    // === WaitingForEvent — do nothing, just wait ===
    if (mState == State::WaitingForEvent) return;

    // === Run scanners ===
    if (mNeedsQuickScan) {
        mNeedsQuickScan = false;
        quickScan();
    }
    runChunkScanner();
    cleanupFoundChests();

    // === State Machine ===
    uint64_t stateTime = getStateTime();

    switch (mState) {

    // ---------------------------------------------------------
    case State::Idle: {
        if (mIsEating) return; // не TP'аемся к сундуку посреди еды

        glm::ivec3 target = findNearestChest(player);
        if (target.x == INT_MAX) {
            mHasTarget = false;
            // No chests + event mode + already looted some → wait for event
            if (mEventMode.mValue && mChestsLooted > 0) {
                NotifyUtils::notify("No chests found, waiting for event...", 3.f, Notification::Type::Info);
                setState(State::WaitingForEvent);
            }
            return;
        }

        // === Found a chest! Start cycle ===
        mOriginalPos = *player->getPos();
        mTargetChestBlock = target;
        mTargetChestPos = glm::vec3(target) + glm::vec3(0.5f, PLAYER_HEIGHT, 0.5f);
        mHasTarget = true;
        mIsChestOpen = false; // Reset before cycle

        // Timing
        mTiming = {};
        mTiming.cycleStart = NOW;
        mTiming.tpToStart = NOW;
        mTiming.chestPos = target;
        mTiming.distance = glm::distance(*player->getPos(), mTargetChestPos);
        mTiming.cycleNumber = mChestsLooted + 1;

        if (mDebug.mValue)
            ChatUtils::displayClientMessage("§7[ICA] §fTarget: §e({}, {}, {}) §fdist: §a{:.1f}",
                target.x, target.y, target.z, mTiming.distance);

        // TP to chest (instant, all packets in 1 tick)
        straightLineTP(player, *player->getPos(), mTargetChestPos, true);
        player->setPosition(mTargetChestPos);
        auto sv = player->getStateVectorComponent();
        if (sv) sv->mVelocity = glm::vec3(0.f);

        mTiming.tpToEnd = NOW;
        setState(State::WaitAfterTP);
        break;
    }

    // ---------------------------------------------------------
    case State::WaitAfterTP: {
        if (stateTime < static_cast<uint64_t>(mWaitAfterTP.mValue)) return;

        mTiming.waitEnd = NOW;

        // Verify chest still exists
        auto source = ClientInstance::get()->getBlockSource();
        if (source) {
            Block* block = source->getBlock(mTargetChestBlock);
            if (!block || !isValidChest(block->mLegacy->getBlockId())) {
                if (mDebug.mValue)
                    ChatUtils::displayClientMessage("§7[ICA] §cChest gone after TP!");
                // Remove from found and TP back
                BlockPos bp; bp.x = mTargetChestBlock.x; bp.y = mTargetChestBlock.y; bp.z = mTargetChestBlock.z;
                mFoundChests.erase(bp);

                if (mReturnBack.mValue) {
                    straightLineTP(player, *player->getPos(), mOriginalPos, false);
                    player->setPosition(mOriginalPos);
                    auto sv2 = player->getStateVectorComponent();
                    if (sv2) sv2->mVelocity = glm::vec3(0.f);
                }
                mHasTarget = false;
                setState(State::Idle);
                return;
            }
        }

        setState(State::OpeningChest);
        break;
    }

    // ---------------------------------------------------------
    case State::OpeningChest: {
        // Re-send position to make sure server knows we're next to the chest
        {
            auto sender = ClientInstance::get()->getPacketSender();
            if (sender) {
                auto posPkt = createPacketForPos(player, mTargetChestPos);
                sender->sendToServer(posPkt.get());
            }
        }
        openChest(player, mTargetChestBlock);
        markChestAsOpened(mTargetChestBlock);
        setState(State::WaitingForOpen);
        break;
    }

    // ---------------------------------------------------------
    case State::WaitingForOpen: {
        if (mIsChestOpen) {
            mTiming.openEnd = NOW;

            // Enable ChestStealer
            if (sChestStealer) {
                mChestStealerWasEnabled = sChestStealer->mEnabled;
                if (!sChestStealer->mEnabled)
                    sChestStealer->setEnabled(true);
            }
            setState(State::WaitingForStealer);
            return;
        }

        // Retry opening every 1 second
        if (stateTime > 1000 && stateTime < 3000) {
            if ((stateTime / 1000) * 1000 == stateTime) {
                // Retry: re-send position + open
                auto sender = ClientInstance::get()->getPacketSender();
                if (sender) {
                    auto posPkt = createPacketForPos(player, mTargetChestPos);
                    sender->sendToServer(posPkt.get());
                }
                openChest(player, mTargetChestBlock);
                if (mDebug.mValue)
                    ChatUtils::displayClientMessage("§7[ICA] §eRetrying chest open...");
            }
        }

        // Timeout 4 seconds (was 3, now with retries)
        if (stateTime > 4000) {
            if (mDebug.mValue)
                ChatUtils::displayClientMessage("§7[ICA] §cChest open timeout!");

            mTiming.openEnd = NOW;
            mTiming.stealEnd = NOW;
            mTiming.tpBackEnd = NOW;

            if (mReturnBack.mValue) {
                straightLineTP(player, *player->getPos(), mOriginalPos, false);
                player->setPosition(mOriginalPos);
                auto sv3 = player->getStateVectorComponent();
                if (sv3) sv3->mVelocity = glm::vec3(0.f);
                mTiming.tpBackEnd = NOW;
            }

            logCycleTiming();
            mHasTarget = false;
            setState(State::Idle);
        }
        break;
    }

    // ---------------------------------------------------------
    case State::WaitingForStealer: {
        // Chest closed = stealing complete
        if (!mIsChestOpen) {
            mTiming.stealEnd = NOW;
            mChestsLooted++;

            if (sChestStealer && !mChestStealerWasEnabled)
                sChestStealer->setEnabled(false);

            // TP back
            if (mReturnBack.mValue) {
                straightLineTP(player, *player->getPos(), mOriginalPos, true);
                player->setPosition(mOriginalPos);
                auto sv4 = player->getStateVectorComponent();
                if (sv4) sv4->mVelocity = glm::vec3(0.f);
            }

            mTiming.tpBackEnd = NOW;
            logCycleTiming();
            mHasTarget = false;
            setState(State::Idle);
            return;
        }

        // Timeout 30 seconds
        if (stateTime > 30000) {
            if (mDebug.mValue)
                ChatUtils::displayClientMessage("§7[ICA] §cStealer timeout!");

            if (sChestStealer && !mChestStealerWasEnabled)
                sChestStealer->setEnabled(false);

            closeChest();
            mTiming.stealEnd = NOW;
            mChestsLooted++;

            if (mReturnBack.mValue) {
                straightLineTP(player, *player->getPos(), mOriginalPos, true);
                player->setPosition(mOriginalPos);
                auto sv5 = player->getStateVectorComponent();
                if (sv5) sv5->mVelocity = glm::vec3(0.f);
            }

            mTiming.tpBackEnd = NOW;
            logCycleTiming();
            mHasTarget = false;
            setState(State::Idle);
        }
        break;
    }

    // ---------------------------------------------------------
    case State::WaitingForEvent:
        // Handled at top of function
        break;
    }
}

// =========================================================
// PACKET IN
// =========================================================

void InfiniteChestAura::onPacketInEvent(PacketInEvent& event)
{
    // === Container Open ===
    if (event.mPacket->getId() == PacketID::ContainerOpen) {
        if (mState == State::WaitingForOpen || mState == State::OpeningChest)
            mIsChestOpen = true;
    }

    // === Container Close ===
    else if (event.mPacket->getId() == PacketID::ContainerClose) {
        mIsChestOpen = false;
    }

    // === Cancel server position corrections during active TP ===
    else if (event.mPacket->getId() == PacketID::MovePlayer) {
        if (mState == State::WaitAfterTP || mState == State::OpeningChest ||
            mState == State::WaitingForOpen || mState == State::WaitingForStealer)
        {
            auto player = ClientInstance::get()->getLocalPlayer();
            if (!player) return;
            auto pkt = event.getPacket<MovePlayerPacket>();
            if (pkt->mPlayerID == player->getRuntimeID()) {
                event.cancel();
                // Re-send so server knows we got it
                ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
            }
        }
    }

    // === Dimension change ===
    else if (event.mPacket->getId() == PacketID::ChangeDimension) {
        saveChestMemory();
        mState = State::Idle;
        mHasTarget = false;
        mIsChestOpen = false;
        mFoundChests.clear();
        mIsEating = false;
        mEatSlot = -1;
        resetScanner();
        mNeedsQuickScan = true;
        std::lock_guard<std::mutex> lock(mMutex);
        mPacketPositions.clear();
    }

    // === Chat trigger for Event Mode ===
    else if (event.mPacket->getId() == PacketID::Text) {
        if (!mEventMode.mValue) return;
        if (mState != State::WaitingForEvent) return;

        auto textPacket = event.getPacket<TextPacket>();
        if (!textPacket) return;

        bool triggered = false;
        if (!textPacket->mAuthor.empty() && containsTriggerWord(textPacket->mAuthor))
            triggered = true;
        if (!triggered && !textPacket->mMessage.empty() && containsTriggerWord(textPacket->mMessage))
            triggered = true;
        if (!triggered) {
            for (const auto& param : textPacket->mParams) {
                if (containsTriggerWord(param)) { triggered = true; break; }
            }
        }

        if (triggered) {
            if (mDebug.mValue)
                ChatUtils::displayClientMessage("§7[ICA] §a*** EVENT TRIGGERED! ***");
            NotifyUtils::notify("AirDrop detected!", 3.f, Notification::Type::Info);

            // Try to parse coordinates from the chat message
            // Supports formats like: "x: 123 y: 64 z: -456", "(123, 64, -456)", "123 64 -456"
            bool coordsFound = false;
            glm::ivec3 parsedCoords(0);

            auto tryParseCoords = [&](const std::string& text) -> bool {
                if (text.empty()) return false;
                // Pattern 1: "X: 378 | Y: 65 | Z: 351" or "x: 123 y: 64 z: -456"
                // The pipe | is used as separator in the actual airdrop message
                std::regex rxyz(R"([xX]\s*[:=]\s*(-?\d+)\s*[,\s|]*[yY]\s*[:=]\s*(-?\d+)\s*[,\s|]*[zZ]\s*[:=]\s*(-?\d+))");
                std::smatch m;
                if (std::regex_search(text, m, rxyz) && m.size() == 4) {
                    parsedCoords.x = std::stoi(m[1].str());
                    parsedCoords.y = std::stoi(m[2].str());
                    parsedCoords.z = std::stoi(m[3].str());
                    return true;
                }
                // Pattern 2: "(123, 64, -456)" or "[123, 64, -456]"
                std::regex rparens(R"([\(\[]\s*(-?\d+)\s*[,\s]+\s*(-?\d+)\s*[,\s]+\s*(-?\d+)\s*[\)\]])");
                if (std::regex_search(text, m, rparens) && m.size() == 4) {
                    parsedCoords.x = std::stoi(m[1].str());
                    parsedCoords.y = std::stoi(m[2].str());
                    parsedCoords.z = std::stoi(m[3].str());
                    return true;
                }
                // Pattern 3: three consecutive numbers "123 64 -456"
                std::regex rnums(R"(\b(-?\d+)\s+(-?\d+)\s+(-?\d+)\b)");
                if (std::regex_search(text, m, rnums) && m.size() == 4) {
                    int x = std::stoi(m[1].str());
                    int y = std::stoi(m[2].str());
                    int z = std::stoi(m[3].str());
                    // Sanity check: Y should be reasonable
                    if (y >= -64 && y <= 320) {
                        parsedCoords = { x, y, z };
                        return true;
                    }
                }
                return false;
            };

            // Try parsing coords from all text fields
            if (!coordsFound && tryParseCoords(textPacket->mMessage)) coordsFound = true;
            if (!coordsFound && tryParseCoords(textPacket->mAuthor)) coordsFound = true;
            if (!coordsFound) {
                for (const auto& param : textPacket->mParams) {
                    if (tryParseCoords(param)) { coordsFound = true; break; }
                }
            }

            if (coordsFound) {
                // We have exact coordinates! Add as found chest and start immediately
                ChatUtils::displayClientMessage("§7[ICA] §aParsed coords: §e({}, {}, {})",
                    parsedCoords.x, parsedCoords.y, parsedCoords.z);

                // Add the chest at parsed position directly
                BlockPos bp; bp.x = parsedCoords.x; bp.y = parsedCoords.y; bp.z = parsedCoords.z;
                mFoundChests[bp] = { parsedCoords, CHEST_ID };

                // Also scan nearby (Y offset ±3) in case the chest is at a slightly different Y
                for (int dy = -3; dy <= 3; dy++) {
                    if (dy == 0) continue;
                    BlockPos bp2; bp2.x = parsedCoords.x; bp2.y = parsedCoords.y + dy; bp2.z = parsedCoords.z;
                    mFoundChests[bp2] = { glm::ivec3(parsedCoords.x, parsedCoords.y + dy, parsedCoords.z), CHEST_ID };
                }

                // Override search radius temporarily to reach the chest
                // (scanner uses mSearchRadius, which might be too small)
            }

            // Reset scanner to also find nearby chests
            if (!coordsFound) {
                mFoundChests.clear();
                mNeedsQuickScan = true;
            }
            resetScanner();
            setState(State::Idle);
        }
    }
}

// =========================================================
// RENDER
// =========================================================

void InfiniteChestAura::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto drawList = ImGui::GetBackgroundDrawList();

    // === Draw TP path ===
    if (mDrawPath.mValue) {
        std::lock_guard<std::mutex> lock(mMutex);
        uint64_t now = NOW;
        float alpha = 1.f;

        if (mLastPathTime + 3000 < now) {
            mPacketPositions.clear();
        } else {
            alpha = std::clamp(1.f - float(now - mLastPathTime) / 3000.f, 0.f, 1.f);
        }

        if (!mPacketPositions.empty()) {
            std::vector<ImVec2> points;
            for (auto& pos : mPacketPositions) {
                ImVec2 sp;
                if (RenderUtils::worldToScreen(pos, sp)) points.push_back(sp);
            }
            for (size_t i = 0; i + 1 < points.size(); i++) {
                ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
                c.Value.w *= alpha;
                drawList->AddLine(points[i], points[i + 1], c, 2.0f);
            }
            if (points.size() >= 2) {
                drawList->AddCircleFilled(points[0], 4.0f, ImColor(0.2f, 1.0f, 0.2f, alpha));
                drawList->AddCircleFilled(points.back(), 4.0f, ImColor(1.0f, 1.0f, 0.2f, alpha));
            }
        }
    }

    // === Highlight target chest ===
    if (mRenderTarget.mValue && mHasTarget) {
        AABB aabb(glm::vec3(mTargetChestBlock), glm::vec3(1.0f));
        auto pts = MathUtils::getImBoxPoints(aabb);
        if (!pts.empty()) {
            ImColor col = ColorUtils::getThemedColor(0);
            ImColor fill(col.Value.x, col.Value.y, col.Value.z, 0.3f);
            drawList->AddConvexPolyFilled(pts.data(), pts.size(), fill);
            drawList->AddPolyline(pts.data(), pts.size(), col, true, 2.5f);
        }
    }

    // === Status text ===
    ImVec2 ss = ImGui::GetIO().DisplaySize;
    float centerX = ss.x * 0.5f;

    if (mState == State::WaitingForEvent) {
        const char* text = "Waiting for AirDrop...";
        ImVec2 ts = ImGui::CalcTextSize(text);
        float boxW = ts.x + 20.f;
        float boxH = ts.y + 10.f;
        float boxX = centerX - boxW * 0.5f;
        float boxY = ss.y * 0.28f;
        drawList->AddRectFilled(ImVec2(boxX, boxY), ImVec2(boxX + boxW, boxY + boxH),
            ImColor(0.f, 0.f, 0.f, 0.5f), 6.f);
        drawList->AddText(ImVec2(centerX - ts.x * 0.5f, boxY + 5.f),
            ImColor(1.f, 1.f, 0.4f, 0.9f), text);
    }

    // Chests counter
    {
        std::string text = fmt::format("Found: {} | Looted: {} | Memory: {}",
            mFoundChests.size(), mChestsLooted, mOpenedChests.size());
        ImVec2 ts = ImGui::CalcTextSize(text.c_str());
        drawList->AddText(ImVec2(centerX - ts.x * 0.5f, ss.y * 0.32f),
            ImColor(0.5f, 1.f, 0.5f, 0.7f), text.c_str());
    }
}