//
// Created by vastrakai on 7/6/2024.
//

#include "ItemUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <Utils/StringUtils.hpp>
#include <string_view>
#include <utility>

namespace {
    // Most block items expose ItemStackBase::mBlock.  A few Bedrock items,
    // notably campfire/soul_campfire on 1.21.x, only expose their Item name.
    // Keep a conservative name fallback for those items instead of treating
    // every non-empty inventory item as a block.
    constexpr std::string_view kBlockItemNames[] = {
        "stone", "cobblestone", "dirt", "coarse_dirt", "rooted_dirt",
        "grass_block", "podzol", "mycelium", "sand", "red_sand", "gravel",
        "clay", "snow", "ice", "packed_ice", "blue_ice", "obsidian",
        "bedrock", "netherrack", "soul_sand", "soul_soil", "basalt",
        "blackstone", "end_stone", "tuff", "calcite", "dripstone_block",
        "moss_block", "mud", "packed_mud", "sandstone", "red_sandstone",
        "glowstone", "sea_lantern", "prismarine", "sponge", "slime",
        "honey_block", "hay_block", "bookshelf", "chiseled_bookshelf",
        "crafting_table", "furnace", "blast_furnace", "smoker", "anvil",
        "chest", "trapped_chest", "barrel", "hopper", "dispenser", "dropper",
        "brewing_stand", "enchanting_table", "beacon", "conduit",
        "respawn_anchor", "lodestone", "campfire", "soul_campfire", "bed",
        "torch", "soul_torch", "redstone_torch", "lantern", "soul_lantern",
        "flower_pot", "cauldron", "composter", "jukebox", "note_block", "bell",
        "lectern", "smithing_table", "cartography_table", "fletching_table",
        "stonecutter", "grindstone", "loom", "scaffolding", "ladder", "tnt",
        "tripwire_hook", "lever", "daylight_detector", "target", "lightning_rod",
        "bamboo", "cactus", "sugar_cane", "kelp", "seagrass", "dead_bush",
        "glow_lichen", "spore_blossom", "small_dripleaf", "big_dripleaf",
        "pointed_dripstone", "decorated_pot", "amethyst_cluster", "sculk",
        "sculk_sensor", "calibrated_sculk_sensor", "chorus_plant", "nether_wart",
        "wheat", "carrots", "potatoes", "beetroots", "melon_stem", "pumpkin_stem",
        "pitcher_plant", "nether_sprouts", "turtle_egg", "sniffer_egg", "cake",
        "trial_spawner", "vault", "crafter"
    };

    constexpr std::string_view kBlockItemSuffixes[] = {
        "_block", "_slab", "_stairs", "_wall", "_fence", "_fence_gate",
        "_door", "_trapdoor", "_button", "_pressure_plate", "_sign",
        "_hanging_sign", "_banner", "_bed", "_candle", "_leaves", "_log",
        "_wood", "_planks", "_ore", "_concrete", "_concrete_powder",
        "_terracotta", "_glass", "_pane", "_brick", "_bricks", "_tiles",
        "_tile", "_lantern", "_torch", "_rail", "_carpet", "_wool", "_coral",
        "_coral_fan", "_head", "_skull", "_shulker_box", "_sapling", "_mushroom",
        "_fungus", "_roots", "_vines", "_vine", "_bush", "_grass", "_fern",
        "_flower", "_lily", "_pot", "_hive", "_nest", "_egg"
    };

    constexpr std::string_view kBlacklistedBlockNames[] = {
        "netherreactor", "boombox", "lilypad", "torch", "fence"
    };

    std::string normaliseItemName(std::string name)
    {
        // Item names can be either "campfire" or "minecraft:campfire".
        if (const size_t separator = name.rfind(':'); separator != std::string::npos)
            name.erase(0, separator + 1);
        return StringUtils::toLower(std::move(name));
    }

    bool looksLikeBlockItemName(const std::string& rawName)
    {
        const std::string name = normaliseItemName(rawName);
        if (name.empty()) return false;

        for (const auto exactName : kBlockItemNames)
            if (name == exactName) return true;

        for (const auto suffix : kBlockItemSuffixes)
            if (name.ends_with(suffix)) return true;

        return false;
    }

    bool isBlacklistedBlockName(const std::string& rawName)
    {
        for (const auto blacklisted : kBlacklistedBlockNames)
            if (StringUtils::containsIgnoreCase(rawName, std::string(blacklisted))) return true;
        return false;
    }
}

int ItemUtils::getItemValue(ItemStack* item) {
    int value = 0;
    if (!item->mItem) return -1;
    switch (item->getItem()->getItemType())
    {
    case SItemType::Helmet:
    case SItemType::Chestplate:
    case SItemType::Leggings:
    case SItemType::Boots:
        if (item->getItem()->getItemTier() < 4) break;
        value = item->getEnchantValue(Enchant::PROTECTION) + item->getEnchantValue(Enchant::FIRE_PROTECTION);
        break;
    case SItemType::Sword:
        value = item->getEnchantValue(Enchant::SHARPNESS);
        break;
    case SItemType::Pickaxe:
    case SItemType::Axe:
    case SItemType::Shovel:
        value = item->getEnchantValue(Enchant::EFFICIENCY);
        break;
    case SItemType::None:
        break;
    }

    // If the item is armor, add the getArmorValue function
    if (item->getItem()->getItemType() >= SItemType::Helmet && item->getItem()->getItemType() <= SItemType::Boots)
        value += item->getItem()->mProtection;

    // If the item is a weapon, add the getItemTier function
    if (item->getItem()->getItemType() == SItemType::Sword || item->getItem()->getItemType() == SItemType::Pickaxe || item->getItem()->getItemType() == SItemType::Axe || item->getItem()->getItemType() == SItemType::Shovel)
        value += item->getItem()->getItemTier();

    return value;
}

int ItemUtils::getBestItem(SItemType type, bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto supplies = player->getSupplies();
    auto container = supplies->getContainer();

    int bestSlot = supplies->mSelectedSlot;
    int bestValue = 0;

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (!item->mItem) continue;

        if (hotbarOnly && i > 8) break;

        if (item->getItem()->getItemType() == type)
        {
            int value = getItemValue(item);
            if (value > bestValue)
            {
                bestValue = value;
                bestSlot = i;
            }
        }
    }

    return bestSlot;
}

bool ItemUtils::hasItemType(ItemStack* item)
{
    if (!item->mItem) return false;
    return item->getItem()->getItemType() != SItemType::None;
}

std::unordered_map<SItemType, int> ItemUtils::getBestItems()
{
    std::unordered_map<SItemType, int> bestItemsResult;
    std::vector<std::pair<SItemType, int>> bestItems;

    // instead, you need to iterate over the inventory once manually.
    auto player = ClientInstance::get()->getLocalPlayer();
    auto supplies = player->getSupplies();
    auto container = supplies->getContainer();

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (!item->mItem) continue;

        switch (item->getItem()->getItemType())
        {
        case SItemType::Helmet:
        case SItemType::Chestplate:
        case SItemType::Leggings:
        case SItemType::Boots:
        case SItemType::Sword:
        case SItemType::Pickaxe:
        case SItemType::Axe:
        case SItemType::Shovel:
            bestItems.emplace_back(item->getItem()->getItemType(), i);
            break;
        }
    }

    for (const auto& [type, slot] : bestItems)
    {
        auto item = container->getItem(slot);
        if (!item->mItem) continue;

        int value = getItemValue(item);

        if (value > getItemValue(container->getItem(bestItemsResult[type])))
        {
            bestItemsResult[type] = slot;
        }
    }

    return bestItemsResult;
}

std::string ItemUtils::getPlaceableName(ItemStack* stack)
{
    if (!stack || !stack->mItem) return {};

    // Prefer the block registry name when it is available.  The item name is
    // the important fallback for block-items whose mBlock pointer is null.
    if (stack->mBlock) {
        if (BlockLegacy* legacy = stack->mBlock->toLegacy(); legacy && !legacy->isAir())
            return legacy->mName;
    }

    if (Item* item = stack->getItem())
        return item->mName;

    return {};
}

bool ItemUtils::isPlaceableBlock(ItemStack* stack)
{
    if (!stack || !stack->mItem || stack->mCount <= 0) return false;

    if (stack->mBlock) {
        if (BlockLegacy* legacy = stack->mBlock->toLegacy(); legacy && !legacy->isAir())
            return true;
    }

    // Do not require mBlock here: campfires and a number of special block
    // items are valid placement items but have no block pointer in the stack.
    if (Item* item = stack->getItem())
        return looksLikeBlockItemName(item->mName);

    return false;
}

int ItemUtils::getAllPlaceables(bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->getSupplies() || !player->getSupplies()->getContainer()) return 0;

    int placeables = 0;
    auto container = player->getSupplies()->getContainer();

    for (int i = 0; i < 36; i++)
    {
        if (hotbarOnly && i > 8) break;

        ItemStack* stack = container->getItem(i);
        if (!isPlaceableBlock(stack)) continue;
        if (isBlacklistedBlockName(getPlaceableName(stack))) continue;

        placeables += stack->mCount;
    }

    return placeables;
}

int ItemUtils::getFirstPlaceable(bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!container) return -1;

    for (int i = 0; i < 36; i++)
    {
        if (hotbarOnly && i > 8) break;

        ItemStack* stack = container->getItem(i);
        if (!isPlaceableBlock(stack)) continue;
        if (isBlacklistedBlockName(getPlaceableName(stack))) continue;

        return i;
    }

    return -1;
}

int ItemUtils::getPlaceableItemOnBlock(glm::vec3 blockPos, bool hotbarOnly, bool prioHighest)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    auto container = supplies ? supplies->getContainer() : nullptr;
    if (!container) return -1;

    int slot = -1;
    // slot, count
    std::map<int, int> placeables;
    for (int i = 0; i < 36; i++)
    {
        if (hotbarOnly && i > 8) break;

        ItemStack* stack = container->getItem(i);
        if (!isPlaceableBlock(stack)) continue;
        if (isBlacklistedBlockName(getPlaceableName(stack))) continue;

        // Most items have block geometry available.  For the fallback path
        // mBlock is null, so let the server validate the placement instead of
        // dropping the item before a transaction can be sent.
        if (stack->mBlock && stack->mBlock->mLegacy &&
            !stack->mBlock->mLegacy->mayPlaceOn(blockPos))
            continue;

        if (!prioHighest)
        {
            slot = i;
            break;
        }

        placeables[i] = stack->mCount;
    }

    if (prioHighest)
    {
        int highest = 0;
        for (const auto& [pSlot, count] : placeables)
        {
            if (count > highest)
            {
                highest = count;
                slot = pSlot;
            }
        }
    }

    return slot;
}

bool ItemUtils::isUsableBlock(ItemStack* stack)
{
    return isPlaceableBlock(stack) && !isBlacklistedBlockName(getPlaceableName(stack));
}

int ItemUtils::getSwiftnessSpellbook(bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    int slot = -1;

    for (int i = 0; i < 36; i++)
    {
        ItemStack* stack = player->getSupplies()->getContainer()->getItem(i);
        if (!stack->mItem) continue;
        if (hotbarOnly && i > 8) break;
        Item* item = stack->getItem();
        if (StringUtils::containsIgnoreCase(stack->getCustomName(), "Spell of Swiftness"))
        {
            slot = i;
            break;
        }
    }

    return slot;
}

int ItemUtils::getBoombox(bool hotbarOnly, bool tnt)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    int slot = -1;

    std::string name = "boombox";
    if (tnt) name = "tnt";

    for (int i = 0; i < 36; i++)
    {
        ItemStack* stack = player->getSupplies()->getContainer()->getItem(i);
        if (!stack->mItem) continue;
        if (hotbarOnly && i > 8) break;
        Item* item = stack->getItem();
        if (StringUtils::containsIgnoreCase(item->getmName(), name))
        {
            slot = i;
            break;
        }
    }

    return slot;
}

int ItemUtils::getEmptyHotbarSlot()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    for (int i = 0; i < 9; i++)
    {
        ItemStack* stack = player->getSupplies()->getContainer()->getItem(i);
        if (!stack->mItem) return i;
    }

    return -1;
}

void ItemUtils::useItem(int slot)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    ItemStack* stack = player->getSupplies()->getContainer()->getItem(slot);
    if (!stack->mItem) return;

    auto supplies = player->getSupplies();

    int currentSlot = supplies->mSelectedSlot;
    supplies->mSelectedSlot = slot;
    player->getGameMode()->baseUseItem(stack);
    supplies->mSelectedSlot = currentSlot;
}

int ItemUtils::getHardestBlock(int slot, bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();

    int hardestBlockSlot = -1;
    float slowestDestroySpeed = INT_MAX;
    for (int i = 0; i < 36; i++)
    {
        ItemStack* stack = supplies->getContainer()->getItem(i);
        if (!stack->mItem) continue;
        Item* item = stack->getItem();
        if (hotbarOnly && i > 8) continue;
        if (stack->mBlock && stack->mBlock->mLegacy)
        {
            if (isBlacklistedBlockName(stack->mBlock->mLegacy->mName)) continue;
            float destroySpeed = getDestroySpeed(slot, stack->mBlock);
            if (destroySpeed < slowestDestroySpeed) {
                hardestBlockSlot = i;
                slowestDestroySpeed = destroySpeed;
            }
        }
    }

    return hardestBlockSlot;
}

int ItemUtils::getBestBreakingTool(Block* block, bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    auto supplies = player->getSupplies();
    auto container = supplies->getContainer();

    int bestSlot = supplies->mSelectedSlot;
    float bestSpeed = 0.0f;

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (!item->mItem) continue;

        float speed = getDestroySpeed(i, block);
        if (speed > bestSpeed)
        {
            bestSpeed = speed;
            bestSlot = i;
        }

        if (hotbarOnly && i > 8) break;
    }

    return bestSlot;
}

float ItemUtils::getDestroySpeed(const int slot, const Block* block, const float divisor)
{
    float result = 1.0f;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return result;

    int oldSlot = player->getSupplies()->mSelectedSlot;
    player->getSupplies()->mSelectedSlot = slot;
    result = player->getGameMode()->getDestroyRate(*block);
    player->getSupplies()->mSelectedSlot = oldSlot;

    return result / divisor;
}

bool ItemUtils::isFireSword(ItemStack* stack)
{
    if (!stack->mItem) return false;
    return StringUtils::containsAnyIgnoreCase(stack->getCustomName(), {"§6Sword of §eEmbers", "§cFire Sword"});
}

int ItemUtils::getFireSword(bool hotbarOnly)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    int slot = -1;

    for (int i = 0; i < 36; i++)
    {
        ItemStack* stack = player->getSupplies()->getContainer()->getItem(i);
        if (!stack->mItem) continue;
        if (isFireSword(stack))
        {
            slot = i;
            break;
        }

        if (hotbarOnly && i > 8) break;
    }

    return slot;
}