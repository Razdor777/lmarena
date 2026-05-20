//
// SchematicBuilder - Fixed version with proper render & grass exclusion
//

#include "SchematicBuilder.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <algorithm>

void SchematicBuilder::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &SchematicBuilder::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &SchematicBuilder::onBaseTickEvent>(this);
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        ChatUtils::displayClientMessage("§cYou must be in a world!");
        setEnabled(false);
        return;
    }
    
    ChatUtils::displayClientMessage("§aSchematicBuilder enabled!");
    ChatUtils::displayClientMessage("§7Commands: .spos1, .spos2, .scopy, .spaste, .sbuild, .scancel");
}

void SchematicBuilder::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &SchematicBuilder::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SchematicBuilder::onBaseTickEvent>(this);
    
    mState = State::Idle;
    mIsWalking = false;
}

bool SchematicBuilder::hasValidSelection() 
{ 
    return sPos1Set && sPos2Set; 
}

glm::ivec3 SchematicBuilder::getSelectionMin() 
{
    return glm::ivec3(
        std::min(sPos1.x, sPos2.x),
        std::min(sPos1.y, sPos2.y),
        std::min(sPos1.z, sPos2.z)
    );
}

glm::ivec3 SchematicBuilder::getSelectionMax() 
{
    return glm::ivec3(
        std::max(sPos1.x, sPos2.x),
        std::max(sPos1.y, sPos2.y),
        std::max(sPos1.z, sPos2.z)
    );
}

glm::ivec3 SchematicBuilder::getSelectionSize() 
{
    auto min = getSelectionMin();
    auto max = getSelectionMax();
    return glm::ivec3(max.x - min.x + 1, max.y - min.y + 1, max.z - min.z + 1);
}

// === ПРОВЕРКА ТРАВЫ И РАСТЕНИЙ ===
bool SchematicBuilder::isGrassOrPlant(const std::string& blockName)
{
    std::string name = blockName;
    if (name.starts_with("minecraft:")) {
        name = name.substr(10);
    }
    
    // Полный список травы и растений для исключения
    static const std::vector<std::string> grassPatterns = {
        // Трава
        "grass", "tallgrass", "tall_grass", "short_grass",
        "double_plant", "fern", "large_fern",
        
        // Водные растения
        "seagrass", "kelp", "kelp_plant",
        
        // Мёртвые кусты и лозы
        "dead_bush", "deadbush", "vine", "vines",
        "hanging_roots", "cave_vines", "weeping_vines", "twisting_vines",
        
        // Мох и лишайники  
        "glow_lichen", "moss_carpet", "spore_blossom",
        "sculk_vein", "moss",
        
        // Цветы
        "flower", "dandelion", "poppy", "blue_orchid", "allium",
        "azure_bluet", "tulip", "oxeye_daisy", "cornflower",
        "lily_of_the_valley", "wither_rose", "sunflower",
        "lilac", "rose_bush", "peony", "torchflower", "pitcher",
        
        // Грибы
        "mushroom", "fungus",
        
        // Сахарный тростник и бамбук
        "sugar_cane", "sugarcane", "bamboo", "cactus",
        
        // Листья и саженцы
        "sapling", "azalea", "dripleaf", "spore_blossom",
        
        // Культуры
        "wheat", "carrots", "potatoes", "beetroots", "melon_stem",
        "pumpkin_stem", "sweet_berry", "nether_wart",
        
        // Лилия
        "lily_pad", "waterlily"
    };
    
    for (const auto& pattern : grassPatterns) {
        if (name == pattern) return true;
        if (name.find(pattern) != std::string::npos) return true;
    }
    
    return false;
}

bool SchematicBuilder::copySelection()
{
    if (!hasValidSelection()) {
        ChatUtils::displayClientMessage("§cNo valid selection! Use .spos1 and .spos2 first.");
        return false;
    }
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    if (!blockSource) return false;
    
    mClipboard.clear();
    
    glm::ivec3 min = getSelectionMin();
    glm::ivec3 max = getSelectionMax();
    glm::ivec3 size = getSelectionSize();
    
    mClipboard.origin = min;
    mClipboard.size = size;
    mClipboard.name = fmt::format("copy_{}x{}x{}", size.x, size.y, size.z);
    
    int airBlocks = 0;
    int solidBlocks = 0;
    int excludedGrass = 0;
    
    for (int y = min.y; y <= max.y; y++) {
        for (int x = min.x; x <= max.x; x++) {
            for (int z = min.z; z <= max.z; z++) {
                glm::ivec3 worldPos(x, y, z);
                glm::ivec3 relativePos = worldPos - min;
                
                Block* block = blockSource->getBlock(worldPos);
                if (!block) continue;
                
                BlockLegacy* blockLegacy = block->toLegacy();
                if (!blockLegacy) continue;
                
                // Пропускаем воздух
                if (blockLegacy->isAir()) {
                    airBlocks++;
                    continue;
                }
                
                std::string blockName = blockLegacy->mName;
                
                // Исключаем траву и растения если опция включена
                if (mExcludeGrass.mValue && isGrassOrPlant(blockName)) {
                    excludedGrass++;
                    continue;
                }
                
                unsigned int runtimeId = block->getRuntimeId();
                
                SchematicBlock schematicBlock(relativePos, blockName, runtimeId);
                mClipboard.blocks.push_back(schematicBlock);
                mClipboard.blockCounts[blockName]++;
                solidBlocks++;
            }
        }
    }
    
    ChatUtils::displayClientMessage("§aCopied §f{} §ablocks!", solidBlocks);
    ChatUtils::displayClientMessage("§7  Air skipped: §f{}", airBlocks);
    
    if (excludedGrass > 0) {
        ChatUtils::displayClientMessage("§7  Grass/plants excluded: §f{}", excludedGrass);
    }
    
    // Показываем требуемые блоки
    ChatUtils::displayClientMessage("§7Required blocks:");
    for (const auto& [name, count] : mClipboard.blockCounts) {
        std::string displayName = name;
        if (displayName.starts_with("minecraft:")) {
            displayName = displayName.substr(10);
        }
        
        int slot = findBlockInFullInventory(name);
        std::string status;
        if (slot != -1) {
            if (slot < 9) {
                status = fmt::format("§a✓ hotbar {}", slot + 1);
            } else {
                status = fmt::format("§e✓ inv {}", slot + 1);
            }
        } else {
            status = "§c✗ missing";
        }
        
        ChatUtils::displayClientMessage("§7  {} §f{}: §e{}", status, displayName, count);
    }
    
    return true;
}

bool SchematicBuilder::preparePaste(glm::ivec3 position)
{
    if (mClipboard.isEmpty()) {
        ChatUtils::displayClientMessage("§cClipboard is empty! Use .scopy first.");
        return false;
    }
    
    mPastePosition = position;
    mHasPastePosition = true;
    mState = State::Previewing;
    
    updateAvailableBlocks();
    updateMissingBlocks();
    
    int conflicts = 0;
    int available = 0;
    int missing = 0;
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    
    for (const auto& block : mClipboard.blocks) {
        if (blockSource) {
            glm::ivec3 worldPos = mPastePosition + block.relativePos;
            Block* existingBlock = blockSource->getBlock(worldPos);
            if (existingBlock && existingBlock->toLegacy() && !existingBlock->toLegacy()->isAir()) {
                if (existingBlock->toLegacy()->mName != block.blockName) {
                    conflicts++;
                }
            }
        }
        
        if (findBlockInFullInventory(block.blockName) != -1) {
            available++;
        } else {
            missing++;
        }
    }
    
    ChatUtils::displayClientMessage("§aPaste position: §f{}, {}, {}", position.x, position.y, position.z);
    ChatUtils::displayClientMessage("§7Total: §f{} §7| §aAvailable: §f{} §7| §cMissing: §f{}", 
        mClipboard.blocks.size(), available, missing);
    
    if (conflicts > 0) {
        ChatUtils::displayClientMessage("§e⚠ {} blocks need clearing", conflicts);
    }
    
    ChatUtils::displayClientMessage("§7Use §f.sbuild §7to start");
    
    return true;
}

void SchematicBuilder::updateMissingBlocks()
{
    mMissingBlocksCache.clear();
    
    std::map<std::string, int> needed;
    std::map<std::string, int> have;
    
    // Считаем сколько блоков нужно (только незавершённые)
    for (size_t i = 0; i < mBuildQueue.size(); i++) {
        if (mCompletedIndices.count(i)) continue;
        
        std::string name = mBuildQueue[i].blockName;
        if (name.starts_with("minecraft:")) {
            name = name.substr(10);
        }
        needed[name]++;
    }
    
    // Если очередь пуста, используем clipboard
    if (mBuildQueue.empty()) {
        for (const auto& block : mClipboard.blocks) {
            std::string name = block.blockName;
            if (name.starts_with("minecraft:")) {
                name = name.substr(10);
            }
            needed[name]++;
        }
    }
    
    // Считаем сколько есть в инвентаре
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto supplies = player->getSupplies();
    if (!supplies) return;
    
    auto container = supplies->getContainer();
    if (!container) return;
    
    for (int i = 0; i < 36; i++) {
        ItemStack* stack = container->getItem(i);
        if (!stack || !stack->mItem || stack->mCount <= 0) continue;
        
        std::string itemName = "";
        
        if (stack->mBlock) {
            BlockLegacy* blockLegacy = stack->mBlock->toLegacy();
            if (blockLegacy) {
                itemName = blockLegacy->mName;
            }
        }
        
        if (itemName.empty()) {
            Item* item = stack->getItem();
            if (item) {
                itemName = item->mName;
            }
        }
        
        if (itemName.starts_with("minecraft:")) {
            itemName = itemName.substr(10);
        }
        
        if (!itemName.empty()) {
            have[itemName] += stack->mCount;
        }
    }
    
    // Вычисляем недостающие
    for (const auto& [name, count] : needed) {
        int haveCount = 0;
        
        // Ищем совпадение
        for (const auto& [haveName, haveC] : have) {
            if (haveName == name || 
                haveName.find(name) == 0 || 
                name.find(haveName) == 0) {
                haveCount = haveC;
                break;
            }
        }
        
        int missing = count - haveCount;
        if (missing > 0) {
            mMissingBlocksCache[name] = missing;
        }
    }
    
    mLastMissingBlocksUpdate = NOW;
}

void SchematicBuilder::startBuilding()
{
    if (mClipboard.isEmpty()) {
        ChatUtils::displayClientMessage("§cNothing to build!");
        return;
    }
    
    if (!mHasPastePosition) {
        ChatUtils::displayClientMessage("§cNo paste position! Use .spaste first.");
        return;
    }
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    mBuildQueue.clear();
    mCompletedIndices.clear();
    mSkippedIndices.clear();
    mClearQueue.clear();
    mClearIndex = 0;
    mBuildQueue = mClipboard.blocks;
    
    mTotalBlocksToBuild = static_cast<int>(mBuildQueue.size());
    mBlocksPlaced = 0;
    mBlocksVerified = 0;
    mBlocksCleared = 0;
    mLastActionTime = 0;
    mWaitingForVerification = false;
    mIsWalking = false;
    mWalkTicks = 0;
    mCurrentBuildIdx = -1;
    mCurrentStage = 1;
    mBlocksBuiltInStage = 0;
    
    updateAvailableBlocks();
    updateMissingBlocks();
    
    if (mAutoClear.mValue) {
        prepareClearQueue();
        
        if (!mClearQueue.empty()) {
            ChatUtils::displayClientMessage("§eClearing §f{} §eblocks first...", mClearQueue.size());
            mState = State::Clearing;
            return;
        }
    }
    
    mBlocksInCurrentStage = countRemainingBlocksForCurrentStage();
    
    if (mBlocksInCurrentStage == 0) {
        ChatUtils::displayClientMessage("§cNo blocks available in hotbar!");
        mState = State::WaitingForBlocks;
        return;
    }
    
    mState = State::Building;
    ChatUtils::displayClientMessage("§aStage {}: §f{} §ablocks available", mCurrentStage, mBlocksInCurrentStage);
}

void SchematicBuilder::prepareClearQueue()
{
    mClearQueue.clear();
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    if (!blockSource) return;
    
    std::vector<std::pair<glm::ivec3, int>> blocksToBreak;
    
    for (const auto& block : mBuildQueue) {
        glm::ivec3 worldPos = mPastePosition + block.relativePos;
        Block* existingBlock = blockSource->getBlock(worldPos);
        
        if (existingBlock && existingBlock->toLegacy() && !existingBlock->toLegacy()->isAir()) {
            if (existingBlock->toLegacy()->mName != block.blockName) {
                blocksToBreak.push_back({worldPos, worldPos.y});
            }
        }
    }
    
    std::sort(blocksToBreak.begin(), blocksToBreak.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [pos, y] : blocksToBreak) {
        mClearQueue.push_back(pos);
    }
}

void SchematicBuilder::updateAvailableBlocks()
{
    mAvailableBlocks.clear();
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto supplies = player->getSupplies();
    if (!supplies) return;
    
    auto container = supplies->getContainer();
    if (!container) return;
    
    for (int i = 0; i < 9; i++) {
        ItemStack* stack = container->getItem(i);
        if (!stack || !stack->mItem) continue;
        if (stack->mCount <= 0) continue;
        
        if (stack->mBlock) {
            BlockLegacy* blockLegacy = stack->mBlock->toLegacy();
            if (blockLegacy) {
                mAvailableBlocks.insert(blockLegacy->mName);
                
                std::string name = blockLegacy->mName;
                if (name.starts_with("minecraft:")) {
                    mAvailableBlocks.insert(name.substr(10));
                }
            }
        }
        
        Item* item = stack->getItem();
        if (item) {
            mAvailableBlocks.insert(item->mName);
            mAvailableBlocks.insert("minecraft:" + item->mName);
        }
    }
}

bool SchematicBuilder::isBlockInHotbar(const std::string& blockName)
{
    std::string searchName = blockName;
    if (searchName.starts_with("minecraft:")) {
        searchName = searchName.substr(10);
    }
    
    if (mAvailableBlocks.count(blockName)) return true;
    if (mAvailableBlocks.count(searchName)) return true;
    if (mAvailableBlocks.count("minecraft:" + searchName)) return true;
    
    for (const auto& available : mAvailableBlocks) {
        std::string avName = available;
        if (avName.starts_with("minecraft:")) {
            avName = avName.substr(10);
        }
        
        if (avName == searchName) return true;
        if (searchName.find(avName) == 0) return true;
        if (avName.find(searchName) == 0) return true;
    }
    
    return false;
}

int SchematicBuilder::countRemainingBlocksForCurrentStage()
{
    int count = 0;
    for (size_t i = 0; i < mBuildQueue.size(); i++) {
        if (mCompletedIndices.count(i)) continue;
        if (isBlockInHotbar(mBuildQueue[i].blockName)) {
            count++;
        }
    }
    return count;
}

int SchematicBuilder::countRemainingBlocksTotal()
{
    return mTotalBlocksToBuild - static_cast<int>(mCompletedIndices.size());
}

void SchematicBuilder::startNextStage()
{
    mCurrentStage++;
    mBlocksBuiltInStage = 0;
    
    updateAvailableBlocks();
    updateMissingBlocks();
    
    mBlocksInCurrentStage = countRemainingBlocksForCurrentStage();
    
    if (mBlocksInCurrentStage > 0) {
        ChatUtils::displayClientMessage("§aStage {}: §f{} §ablocks available", mCurrentStage, mBlocksInCurrentStage);
        mState = State::Building;
    } else {
        int remaining = countRemainingBlocksTotal();
        if (remaining > 0) {
            ChatUtils::displayClientMessage("§eWaiting for blocks... §f{} §eremaining", remaining);
            ChatUtils::displayClientMessage("§7Put required blocks in hotbar");
            mState = State::WaitingForBlocks;
        } else {
            ChatUtils::displayClientMessage("§aBuild complete! §f{} §ablocks placed", mBlocksVerified);
            mState = State::Idle;
            mHasPastePosition = false;
        }
    }
}

void SchematicBuilder::stopBuilding()
{
    if (mState == State::Building || mState == State::Walking || mState == State::Clearing) {
        ChatUtils::displayClientMessage("§ePaused. Progress: §f{}/{}", mBlocksVerified, mTotalBlocksToBuild);
    }
    
    mState = State::Previewing;
    mIsWalking = false;
    mWaitingForVerification = false;
}

void SchematicBuilder::cancelPaste()
{
    mBuildQueue.clear();
    mCompletedIndices.clear();
    mSkippedIndices.clear();
    mClearQueue.clear();
    mMissingBlocksCache.clear();
    mHasPastePosition = false;
    mState = State::Idle;
    mIsWalking = false;
    ChatUtils::displayClientMessage("§cCancelled.");
}

void SchematicBuilder::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    std::string screenName = ClientInstance::get()->getScreenName();
    if (!screenName.empty() && screenName != "hud_screen") {
        return;
    }
    
    // Обновляем кэш недостающих блоков каждые 2 секунды
    if (NOW - mLastMissingBlocksUpdate > 2000) {
        updateMissingBlocks();
    }
    
    switch (mState) {
        case State::Clearing:
            processClearing();
            break;
        case State::Building:
            if (mWaitingForVerification) {
                processVerification();
            } else {
                processBuilding();
            }
            break;
        case State::Walking:
            processWalking();
            break;
        case State::WaitingForBlocks:
            processWaitingForBlocks();
            break;
        default:
            break;
    }
}

void SchematicBuilder::processWaitingForBlocks()
{
    uint64_t now = NOW;
    
    if (now - mLastInventoryCheck < 500) return;
    mLastInventoryCheck = now;
    
    updateAvailableBlocks();
    
    int available = countRemainingBlocksForCurrentStage();
    
    if (available > 0) {
        mBlocksInCurrentStage = available;
        ChatUtils::displayClientMessage("§aFound blocks! Continuing Stage {}...", mCurrentStage);
        mState = State::Building;
    }
    
    int remaining = countRemainingBlocksTotal();
    if (remaining == 0) {
        ChatUtils::displayClientMessage("§aBuild complete! §f{} §ablocks placed", mBlocksVerified);
        mState = State::Idle;
        mHasPastePosition = false;
    }
}

void SchematicBuilder::processClearing()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    uint64_t now = NOW;
    if (mBuildDelay.mValue > 0 && now - mLastActionTime < static_cast<uint64_t>(mBuildDelay.mValue)) return;
    
    if (mClearIndex >= static_cast<int>(mClearQueue.size())) {
        ChatUtils::displayClientMessage("§aClearing complete!");
        
        updateAvailableBlocks();
        mBlocksInCurrentStage = countRemainingBlocksForCurrentStage();
        
        if (mBlocksInCurrentStage > 0) {
            mState = State::Building;
            ChatUtils::displayClientMessage("§aStage {}: §f{} §ablocks available", mCurrentStage, mBlocksInCurrentStage);
        } else {
            mState = State::WaitingForBlocks;
            ChatUtils::displayClientMessage("§eWaiting for blocks...");
        }
        return;
    }
    
    glm::ivec3 pos = mClearQueue[mClearIndex];
    
    if (!isInRange(pos)) {
        if (mAutoWalk.mValue) {
            mWalkTarget = findWalkPosition(pos);
            mState = State::Walking;
            mIsWalking = true;
            mWalkTicks = 0;
            return;
        }
        mClearIndex++;
        return;
    }
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    if (!blockSource) return;
    
    Block* block = blockSource->getBlock(pos);
    if (!block || !block->toLegacy() || block->toLegacy()->isAir()) {
        mClearIndex++;
        mBlocksCleared++;
        return;
    }
    
    if (tryBreakBlock(pos)) {
        mLastActionTime = now;
        mBlocksCleared++;
        mClearIndex++;
    }
}

bool SchematicBuilder::tryBreakBlock(glm::ivec3 pos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;
    
    int side = BlockUtils::getExposedFace(pos);
    if (side == -1) side = 0;
    
    if (mSwing.mValue) {
        player->swing();
    }
    
    BlockUtils::destroyBlock(pos, side, false);
    return true;
}

void SchematicBuilder::processVerification()
{
    uint64_t now = NOW;
    if (now - mLastPlacedTime < 100) return;
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    if (!blockSource) {
        mWaitingForVerification = false;
        return;
    }
    
    Block* block = blockSource->getBlock(mLastPlacedPos);
    if (block && block->toLegacy() && !block->toLegacy()->isAir()) {
        mBlocksVerified++;
        mBlocksBuiltInStage++;
        mCompletedIndices.insert(mCurrentBuildIdx);
    }
    
    mWaitingForVerification = false;
    mCurrentBuildIdx = -1;
}

void SchematicBuilder::processBuilding()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    uint64_t now = NOW;
    if (mBuildDelay.mValue > 0 && now - mLastActionTime < static_cast<uint64_t>(mBuildDelay.mValue)) return;
    
    auto blockSource = ClientInstance::get()->getBlockSource();
    if (blockSource) {
        for (size_t i = 0; i < mBuildQueue.size(); i++) {
            if (mCompletedIndices.count(i)) continue;
            
            glm::ivec3 worldPos = mPastePosition + mBuildQueue[i].relativePos;
            Block* block = blockSource->getBlock(worldPos);
            
            if (block && block->toLegacy() && !block->toLegacy()->isAir()) {
                mCompletedIndices.insert(i);
                mBlocksVerified++;
            }
        }
    }
    
    int remaining = countRemainingBlocksTotal();
    if (remaining == 0) {
        ChatUtils::displayClientMessage("§aBuild complete! §f{} §ablocks placed", mBlocksVerified);
        mState = State::Idle;
        mHasPastePosition = false;
        return;
    }
    
    int stageRemaining = countRemainingBlocksForCurrentStage();
    if (stageRemaining == 0) {
        startNextStage();
        return;
    }
    
    int nextIdx = findNextBuildableIndex();
    
    if (nextIdx == -1) {
        if (mAutoWalk.mValue) {
            int walkIdx = findBlockToWalkTo();
            if (walkIdx != -1) {
                glm::ivec3 targetPos = mPastePosition + mBuildQueue[walkIdx].relativePos;
                mWalkTarget = findWalkPosition(targetPos);
                mState = State::Walking;
                mIsWalking = true;
                mWalkTicks = 0;
                return;
            }
        }
        return;
    }
    
    if (tryPlaceBlock(nextIdx)) {
        mBlocksPlaced++;
        mLastActionTime = now;
        mLastPlacedPos = mPastePosition + mBuildQueue[nextIdx].relativePos;
        mLastPlacedBlockName = mBuildQueue[nextIdx].blockName;
        mLastPlacedTime = now;
        mCurrentBuildIdx = nextIdx;
        
        if (mVerifyPlacement.mValue) {
            mWaitingForVerification = true;
        } else {
            mBlocksVerified++;
            mBlocksBuiltInStage++;
            mCompletedIndices.insert(nextIdx);
        }
    }
}

void SchematicBuilder::processWalking()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    glm::vec3 playerPos = *player->getPos();
    float distanceXZ = glm::distance(
        glm::vec2(playerPos.x, playerPos.z), 
        glm::vec2(mWalkTarget.x, mWalkTarget.z)
    );
    
    if (distanceXZ < 2.0f) {
        mIsWalking = false;
        mState = mClearIndex < static_cast<int>(mClearQueue.size()) ? State::Clearing : State::Building;
        mWalkTicks = 0;
        return;
    }
    
    mWalkTicks++;
    if (mWalkTicks > 200) {
        ChatUtils::displayClientMessage("§cCan't reach target, skipping...");
        mIsWalking = false;
        mState = mClearIndex < static_cast<int>(mClearQueue.size()) ? State::Clearing : State::Building;
        mWalkTicks = 0;
        if (mState == State::Clearing) mClearIndex++;
        return;
    }
    
    walkTowards(mWalkTarget);
}

void SchematicBuilder::walkTowards(glm::vec3 target)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto stateVec = player->getStateVectorComponent();
    if (!stateVec) return;
    
    glm::vec3 playerPos = *player->getPos();
    glm::vec3 direction = target - playerPos;
    direction.y = 0;
    
    float length = glm::length(direction);
    if (length < 0.01f) return;
    
    direction = glm::normalize(direction);
    
    float speed = mWalkSpeed.mValue * 0.1f;
    stateVec->mVelocity.x = direction.x * speed;
    stateVec->mVelocity.z = direction.z * speed;
    
    if (player->isOnGround()) {
        auto blockSource = ClientInstance::get()->getBlockSource();
        if (blockSource) {
            glm::ivec3 frontPos = glm::ivec3(
                static_cast<int>(std::floor(playerPos.x + direction.x)),
                static_cast<int>(std::floor(playerPos.y)),
                static_cast<int>(std::floor(playerPos.z + direction.z))
            );
            
            if (!BlockUtils::isAirBlock(frontPos) || target.y > playerPos.y + 0.5f) {
                stateVec->mVelocity.y = 0.42f;
            }
        }
    }
}

glm::vec3 SchematicBuilder::findWalkPosition(glm::ivec3 targetBlock)
{
    glm::vec3 target = glm::vec3(targetBlock) + glm::vec3(0.5f, 0, 0.5f);
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return target;
    
    glm::vec3 playerPos = *player->getPos();
    
    std::vector<glm::vec3> positions = {
        target + glm::vec3(2.5f, 0, 0),
        target + glm::vec3(-2.5f, 0, 0),
        target + glm::vec3(0, 0, 2.5f),
        target + glm::vec3(0, 0, -2.5f),
    };
    
    float closestDist = FLT_MAX;
    glm::vec3 bestPos = target;
    
    for (const auto& pos : positions) {
        float dist = glm::distance(playerPos, pos);
        if (dist < closestDist) {
            closestDist = dist;
            bestPos = pos;
        }
    }
    
    return bestPos;
}

bool SchematicBuilder::isInRange(glm::ivec3 pos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;
    
    glm::vec3 playerPos = *player->getPos();
    glm::vec3 blockCenter = glm::vec3(pos) + glm::vec3(0.5f);
    
    return glm::distance(playerPos, blockCenter) <= mBuildRange.mValue;
}

bool SchematicBuilder::canBuildAt(int index)
{
    if (index < 0 || index >= static_cast<int>(mBuildQueue.size())) return false;
    if (mCompletedIndices.count(index)) return false;
    
    const std::string& blockName = mBuildQueue[index].blockName;
    
    if (!isBlockInHotbar(blockName)) return false;
    
    glm::ivec3 worldPos = mPastePosition + mBuildQueue[index].relativePos;
    
    if (!BlockUtils::isAirBlock(worldPos)) return false;
    if (!isInRange(worldPos)) return false;
    if (BlockUtils::getBlockPlaceFace(worldPos) == -1) return false;
    if (findBlockInInventory(blockName) == -1) return false;
    
    return true;
}

int SchematicBuilder::findNextBuildableIndex()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    
    glm::vec3 playerPos = *player->getPos();
    
    int bestIdx = -1;
    float bestDist = FLT_MAX;
    int bestY = INT_MAX;
    
    for (size_t i = 0; i < mBuildQueue.size(); i++) {
        if (!canBuildAt(i)) continue;
        
        glm::ivec3 worldPos = mPastePosition + mBuildQueue[i].relativePos;
        
        if (worldPos.y < bestY || 
            (worldPos.y == bestY && glm::distance(playerPos, glm::vec3(worldPos)) < bestDist)) {
            bestY = worldPos.y;
            bestDist = glm::distance(playerPos, glm::vec3(worldPos));
            bestIdx = static_cast<int>(i);
        }
    }
    
    return bestIdx;
}

int SchematicBuilder::findBlockToWalkTo()
{
    for (size_t i = 0; i < mBuildQueue.size(); i++) {
        if (mCompletedIndices.count(i)) continue;
        
        const std::string& blockName = mBuildQueue[i].blockName;
        if (!isBlockInHotbar(blockName)) continue;
        
        glm::ivec3 worldPos = mPastePosition + mBuildQueue[i].relativePos;
        
        if (!BlockUtils::isAirBlock(worldPos)) {
            mCompletedIndices.insert(i);
            mBlocksVerified++;
            continue;
        }
        
        if (BlockUtils::getBlockPlaceFace(worldPos) == -1) continue;
        if (findBlockInInventory(blockName) == -1) continue;
        
        return static_cast<int>(i);
    }
    
    return -1;
}

int SchematicBuilder::findBlockInInventory(const std::string& blockName)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    
    auto container = supplies->getContainer();
    if (!container) return -1;
    
    std::string searchName = blockName;
    if (searchName.starts_with("minecraft:")) {
        searchName = searchName.substr(10);
    }
    
    std::string baseSearchName = searchName;
    std::vector<std::string> suffixes = {"_block", "_upper", "_lower", "_top", "_bottom"};
    for (const auto& suffix : suffixes) {
        size_t pos = baseSearchName.find(suffix);
        if (pos != std::string::npos) {
            baseSearchName = baseSearchName.substr(0, pos);
        }
    }
    
    for (int i = 0; i < 9; i++) {
        ItemStack* stack = container->getItem(i);
        if (!stack) continue;
        if (!stack->mItem) continue;
        if (stack->mCount <= 0) continue;
        
        if (stack->mBlock) {
            BlockLegacy* blockLegacy = stack->mBlock->toLegacy();
            if (blockLegacy) {
                std::string itemName = blockLegacy->mName;
                if (itemName.starts_with("minecraft:")) {
                    itemName = itemName.substr(10);
                }
                
                if (itemName == searchName) return i;
                if (itemName == baseSearchName) return i;
                if (itemName.find(baseSearchName) == 0) return i;
            }
        }
        
        Item* item = stack->getItem();
        if (item) {
            std::string itemName = item->mName;
            if (itemName.starts_with("minecraft:")) {
                itemName = itemName.substr(10);
            }
            
            if (itemName == searchName) return i;
            if (itemName == baseSearchName) return i;
            if (itemName.find(baseSearchName) == 0) return i;
            if (baseSearchName.find(itemName) == 0) return i;
        }
    }
    
    return -1;
}

int SchematicBuilder::findBlockInFullInventory(const std::string& blockName)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    
    auto container = supplies->getContainer();
    if (!container) return -1;
    
    std::string searchName = blockName;
    if (searchName.starts_with("minecraft:")) {
        searchName = searchName.substr(10);
    }
    
    std::string baseSearchName = searchName;
    std::vector<std::string> suffixes = {"_block", "_upper", "_lower", "_top", "_bottom"};
    for (const auto& suffix : suffixes) {
        size_t pos = baseSearchName.find(suffix);
        if (pos != std::string::npos) {
            baseSearchName = baseSearchName.substr(0, pos);
        }
    }
    
    for (int i = 0; i < 36; i++) {
        ItemStack* stack = container->getItem(i);
        if (!stack) continue;
        if (!stack->mItem) continue;
        if (stack->mCount <= 0) continue;
        
        if (stack->mBlock) {
            BlockLegacy* blockLegacy = stack->mBlock->toLegacy();
            if (blockLegacy) {
                std::string itemName = blockLegacy->mName;
                if (itemName.starts_with("minecraft:")) {
                    itemName = itemName.substr(10);
                }
                
                if (itemName == searchName) return i;
                if (itemName == baseSearchName) return i;
                if (itemName.find(baseSearchName) == 0) return i;
            }
        }
        
        Item* item = stack->getItem();
        if (item) {
            std::string itemName = item->mName;
            if (itemName.starts_with("minecraft:")) {
                itemName = itemName.substr(10);
            }
            
            if (itemName == searchName) return i;
            if (itemName == baseSearchName) return i;
            if (itemName.find(baseSearchName) == 0) return i;
            if (baseSearchName.find(itemName) == 0) return i;
        }
    }
    
    return -1;
}

bool SchematicBuilder::tryPlaceBlock(int queueIndex)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;
    
    auto supplies = player->getSupplies();
    if (!supplies) return false;
    
    const SchematicBlock& block = mBuildQueue[queueIndex];
    glm::ivec3 worldPos = mPastePosition + block.relativePos;
    
    int slot = findBlockInInventory(block.blockName);
    if (slot == -1 || slot >= 9) return false;
    
    int side = BlockUtils::getBlockPlaceFace(worldPos);
    if (side == -1) return false;
    
    int oldSlot = supplies->mSelectedSlot;
    supplies->mSelectedSlot = slot;
    
    if (mSwing.mValue) {
        player->swing();
    }
    
    BlockUtils::placeBlock(worldPos, side);
    
    supplies->mSelectedSlot = oldSlot;
    
    return true;
}

// ==================== RENDERING ====================

void SchematicBuilder::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    // Selection - красивый 3D бокс
    if (mShowSelection.mValue && hasValidSelection()) {
        renderSelection();
    }
    
    // Preview - ближайшие точки
    if (mShowPreview.mValue && mHasPastePosition && !mClipboard.isEmpty()) {
        renderPreview();
    }
    
    // Clear area
    if (mState == State::Clearing && !mClearQueue.empty()) {
        renderClearArea();
    }
    
    // Missing blocks HUD
    if (mShowMissingBlocks.mValue && 
        (mState == State::Building || mState == State::Walking || 
         mState == State::WaitingForBlocks || mState == State::Previewing)) {
        renderMissingBlocks();
    }
    
    // Progress bar
    if (mShowProgress.mValue && 
        (mState == State::Building || mState == State::Walking || 
         mState == State::Clearing || mState == State::WaitingForBlocks)) {
        renderProgress();
    }
    
    // Walk target
    if (mState == State::Walking && mIsWalking) {
        renderWalkTarget();
    }
}

void SchematicBuilder::renderSelection()
{
    if (!hasValidSelection()) return;
    
    glm::vec3 min = glm::vec3(getSelectionMin());
    glm::vec3 max = glm::vec3(getSelectionMax()) + glm::vec3(1.0f);
    
    AABB selectionBox(min, max, true);
    RenderUtils::drawOutlinedAABB(selectionBox, true, ImColor(0.0f, 0.7f, 1.0f, 0.8f));
}

void SchematicBuilder::renderPreview()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    glm::vec3 playerPos = *player->getPos();
    float maxRadius = mPreviewRadius.mValue;
    int maxRender = static_cast<int>(mPreviewLimit.mValue);
    
    // Собираем блоки с расстоянием для сортировки
    struct BlockWithDist {
        size_t index;
        float distance;
        glm::vec3 worldPos;
    };
    
    std::vector<BlockWithDist> blocksToRender;
    blocksToRender.reserve(mClipboard.blocks.size());
    
    // Определяем источник данных
    const std::vector<SchematicBlock>* sourceBlocks = nullptr;
    
    if (!mBuildQueue.empty()) {
        sourceBlocks = &mBuildQueue;
    } else {
        sourceBlocks = &mClipboard.blocks;
    }
    
    for (size_t i = 0; i < sourceBlocks->size(); i++) {
        // Пропускаем завершённые
        if (mCompletedIndices.count(i)) continue;
        
        const SchematicBlock& block = (*sourceBlocks)[i];
        glm::ivec3 worldPosI = mPastePosition + block.relativePos;
        glm::vec3 worldPos = glm::vec3(worldPosI) + glm::vec3(0.5f);
        
        float dist = glm::distance(playerPos, worldPos);
        
        // Фильтруем по радиусу
        if (dist > maxRadius) continue;
        
        blocksToRender.push_back({i, dist, worldPos});
    }
    
    // Сортируем по расстоянию - ближайшие сначала
    std::sort(blocksToRender.begin(), blocksToRender.end(),
        [](const BlockWithDist& a, const BlockWithDist& b) {
            return a.distance < b.distance;
        });
    
    // Рендерим только ближайшие
    int rendered = 0;
    for (const auto& bd : blocksToRender) {
        if (rendered >= maxRender) break;
        
        ImVec2 screenPos;
        if (!RenderUtils::worldToScreen(bd.worldPos, screenPos)) continue;
        
        const SchematicBlock& block = (*sourceBlocks)[bd.index];
        
        bool inHotbar = isBlockInHotbar(block.blockName);
        bool inFullInv = !inHotbar && findBlockInFullInventory(block.blockName) != -1;
        
        ImColor color;
        float size = 4.0f;
        
        if (inHotbar) {
            color = ImColor(0.0f, 1.0f, 0.0f, 0.9f);  // Зелёный - хотбар
            size = 5.0f;
        } else if (inFullInv) {
            color = ImColor(0.7f, 0.3f, 1.0f, 0.8f);  // Фиолетовый - инвентарь
            size = 4.0f;
        } else {
            color = ImColor(1.0f, 0.5f, 0.0f, 0.8f);  // Оранжевый - нет
            size = 4.0f;
        }
        
        // Размер зависит от расстояния
        float distFactor = 1.0f - (bd.distance / maxRadius) * 0.5f;
        size *= distFactor;
        
        drawList->AddCircleFilled(screenPos, size, color);
        
        // Обводка для ближайших
        if (rendered < 5) {
            drawList->AddCircle(screenPos, size + 2.0f, ImColor(1.0f, 1.0f, 1.0f, 0.5f), 0, 1.5f);
        }
        
        rendered++;
    }
}

void SchematicBuilder::renderClearArea()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    glm::vec3 playerPos = *player->getPos();
    float maxRadius = mPreviewRadius.mValue;
    int maxRender = static_cast<int>(mPreviewLimit.mValue);
    
    // Собираем и сортируем по расстоянию
    struct ClearBlockDist {
        size_t index;
        float distance;
        glm::vec3 pos;
    };
    
    std::vector<ClearBlockDist> blocksToRender;
    
    for (size_t i = mClearIndex; i < mClearQueue.size(); i++) {
        glm::vec3 posF = glm::vec3(mClearQueue[i]) + glm::vec3(0.5f);
        float dist = glm::distance(playerPos, posF);
        
        if (dist > maxRadius) continue;
        
        blocksToRender.push_back({i, dist, posF});
    }
    
    std::sort(blocksToRender.begin(), blocksToRender.end(),
        [](const ClearBlockDist& a, const ClearBlockDist& b) {
            return a.distance < b.distance;
        });
    
    int rendered = 0;
    for (const auto& cbd : blocksToRender) {
        if (rendered >= maxRender) break;
        
        ImVec2 screenPos;
        if (!RenderUtils::worldToScreen(cbd.pos, screenPos)) continue;
        
        float distFactor = 1.0f - (cbd.distance / maxRadius) * 0.3f;
        float size = 6.0f * distFactor;
        
        drawList->AddCircleFilled(screenPos, size, ImColor(1.0f, 0.0f, 0.0f, 0.7f));
        drawList->AddCircle(screenPos, size + 2.0f, ImColor(1.0f, 1.0f, 1.0f, 0.5f), 0, 2.0f);
        
        rendered++;
    }
}

void SchematicBuilder::renderMissingBlocks()
{
    if (mMissingBlocksCache.empty()) return;
    
    auto guiData = ClientInstance::get()->getGuiData();
    if (!guiData) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    float screenWidth = guiData->mResolution.x;
    
    // Позиция HUD - правый верхний угол
    float hudX = screenWidth - 220.0f;
    float hudY = 60.0f;
    float lineHeight = 16.0f;
    float padding = 8.0f;
    
    // Заголовок
    std::string title = "Missing Blocks";
    
    // Подсчёт высоты
    int maxLines = std::min(static_cast<int>(mMissingBlocksCache.size()), 10);
    float hudHeight = padding * 2 + lineHeight * (maxLines + 1);
    float hudWidth = 200.0f;
    
    // Фон
    drawList->AddRectFilled(
        ImVec2(hudX - padding, hudY - padding),
        ImVec2(hudX + hudWidth + padding, hudY + hudHeight),
        ImColor(0.0f, 0.0f, 0.0f, 0.75f), 6.0f
    );
    
    // Рамка
    drawList->AddRect(
        ImVec2(hudX - padding, hudY - padding),
        ImVec2(hudX + hudWidth + padding, hudY + hudHeight),
        ImColor(1.0f, 0.5f, 0.0f, 0.8f), 6.0f, 0, 2.0f
    );
    
    // Заголовок
    drawList->AddText(
        ImVec2(hudX, hudY),
        ImColor(1.0f, 0.7f, 0.0f, 1.0f),
        title.c_str()
    );
    
    // Список блоков
    float currentY = hudY + lineHeight + 4.0f;
    int lineCount = 0;
    
    for (const auto& [name, count] : mMissingBlocksCache) {
        if (lineCount >= maxLines) {
            // Показываем "и ещё..."
            std::string moreText = fmt::format("... and {} more", 
                static_cast<int>(mMissingBlocksCache.size()) - maxLines);
            drawList->AddText(
                ImVec2(hudX, currentY),
                ImColor(0.7f, 0.7f, 0.7f, 1.0f),
                moreText.c_str()
            );
            break;
        }
        
        // Форматируем имя блока
        std::string displayName = name;
        if (displayName.length() > 20) {
            displayName = displayName.substr(0, 17) + "...";
        }
        
        std::string blockLine = fmt::format("{}: {}", displayName, count);
        
        // Цвет в зависимости от количества
        ImColor textColor;
        if (count > 64) {
            textColor = ImColor(1.0f, 0.3f, 0.3f, 1.0f);  // Красный - много
        } else if (count > 16) {
            textColor = ImColor(1.0f, 0.7f, 0.3f, 1.0f);  // Оранжевый - средне
        } else {
            textColor = ImColor(1.0f, 1.0f, 0.5f, 1.0f);  // Жёлтый - мало
        }
        
        drawList->AddText(
            ImVec2(hudX + 8.0f, currentY),
            textColor,
            blockLine.c_str()
        );
        
        currentY += lineHeight;
        lineCount++;
    }
}

void SchematicBuilder::renderProgress()
{
    auto guiData = ClientInstance::get()->getGuiData();
    if (!guiData) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    float screenWidth = guiData->mResolution.x;
    float screenHeight = guiData->mResolution.y;
    
    float barWidth = 300.0f;
    float barHeight = 25.0f;
    float barX = (screenWidth - barWidth) / 2.0f;
    float barY = screenHeight - 120.0f;
    
    drawList->AddRectFilled(
        ImVec2(barX - 3, barY - 3),
        ImVec2(barX + barWidth + 3, barY + barHeight + 3),
        ImColor(0.0f, 0.0f, 0.0f, 0.8f), 8.0f
    );
    
    float progress = 0.0f;
    std::string stateStr;
    ImColor progressColor;
    
    if (mState == State::Clearing) {
        progress = mClearQueue.empty() ? 1.0f : static_cast<float>(mClearIndex) / mClearQueue.size();
        stateStr = "Clearing";
        progressColor = ImColor(1.0f, 0.3f, 0.3f, 0.9f);
    } else if (mState == State::WaitingForBlocks) {
        progress = mTotalBlocksToBuild > 0 
            ? static_cast<float>(mBlocksVerified) / static_cast<float>(mTotalBlocksToBuild) 
            : 0.0f;
        stateStr = fmt::format("Waiting S{}", mCurrentStage);
        progressColor = ImColor(1.0f, 0.8f, 0.0f, 0.9f);
    } else {
        progress = mTotalBlocksToBuild > 0 
            ? static_cast<float>(mBlocksVerified) / static_cast<float>(mTotalBlocksToBuild) 
            : 0.0f;
        stateStr = mState == State::Walking 
            ? fmt::format("Walking S{}", mCurrentStage)
            : fmt::format("Stage {}", mCurrentStage);
        progressColor = mState == State::Walking 
            ? ImColor(1.0f, 0.6f, 0.0f, 0.9f)
            : ImColor(0.0f, 0.8f, 0.2f, 0.9f);
    }
    
    drawList->AddRectFilled(
        ImVec2(barX, barY),
        ImVec2(barX + barWidth * progress, barY + barHeight),
        progressColor, 5.0f
    );
    
    drawList->AddRect(
        ImVec2(barX, barY),
        ImVec2(barX + barWidth, barY + barHeight),
        ImColor(1.0f, 1.0f, 1.0f, 0.5f), 5.0f
    );
    
    std::string text;
    if (mState == State::Clearing) {
        text = fmt::format("{}: {}/{}", stateStr, mClearIndex, mClearQueue.size());
    } else {
        int remaining = countRemainingBlocksTotal();
        text = fmt::format("{}: {}/{} ({} left)", stateStr, mBlocksVerified, mTotalBlocksToBuild, remaining);
    }
    
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    float textX = barX + (barWidth - textSize.x) / 2.0f;
    float textY = barY + (barHeight - textSize.y) / 2.0f;
    
    drawList->AddText(ImVec2(textX + 1, textY + 1), ImColor(0.0f, 0.0f, 0.0f, 1.0f), text.c_str());
    drawList->AddText(ImVec2(textX, textY), ImColor(1.0f, 1.0f, 1.0f, 1.0f), text.c_str());
}

void SchematicBuilder::renderWalkTarget()
{
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(mWalkTarget, screenPos)) {
        drawList->AddCircleFilled(screenPos, 10.0f, ImColor(1.0f, 0.6f, 0.0f, 0.8f));
        drawList->AddCircle(screenPos, 14.0f, ImColor(1.0f, 1.0f, 1.0f, 0.6f), 0, 2.0f);
    }
}