//
//

#include "Item.hpp"


int Item::getArmorSlot()
{
    // Use the name of the item to determine the armor slot
    static constexpr std::array<std::string_view, 4> armorSlots = {
        "_helmet",
        "_chestplate",
        "_leggings",
        "_boots"
    };

    for (int i = 0; i < armorSlots.size(); i++) {
        if (mName.find(armorSlots[i]) != std::string::npos) {
            return i;
        }
    }

    return -1;
}

bool Item::isHelmet()
{
    static constexpr std::string_view helmet = "_helmet";
    return mName.ends_with(helmet);
}

bool Item::isChestplate()
{
    static constexpr std::string_view chestplate = "_chestplate";
    return mName.ends_with(chestplate);
}

bool Item::isLeggings()
{
    static constexpr std::string_view leggings = "_leggings";
    return mName.ends_with(leggings);
}

bool Item::isBoots()
{
    static constexpr std::string_view boots = "_boots";
    return mName.ends_with(boots);
}

bool Item::isSword()
{
    static constexpr std::string_view sword = "_sword";
    return mName.ends_with(sword);
}

bool Item::isPickaxe()
{
    static constexpr std::string_view pickaxe = "_pickaxe";
    return mName.ends_with(pickaxe);
}

bool Item::isAxe()
{
    static constexpr std::string_view axe = "_axe";
    return mName.ends_with(axe);
}

bool Item::isShovel()
{
    static constexpr std::string_view shovel = "_shovel";
    return mName.ends_with(shovel);
}

int Item::getItemTier()
{
    // Use the name of the item to determine the tier
    static constexpr std::array<std::string_view, 7> tiers = {
        "wooden_",
        "chainmail_",
        "stone_",
        "iron_",
        "golden_",
        "diamond_",
        "netherite_"
    };

    for (int i = 0; i < tiers.size(); i++)
    {
        if (mName.starts_with(tiers[i])) {
            return i;
        }
    }

    return 0;
}

int Item::getArmorTier()
{
    // Use the name of the item to determine the tier
    static constexpr std::array<std::string_view, 7> tiers = {
        "leather_",
        "chainmail_",
        "iron_",
        "golden_",
        "diamond_",
        "netherite_"
    };

    for (int i = 0; i < tiers.size(); i++)
    {
        if (mName.starts_with(tiers[i])) {
            return i;
        }
    }

    return 0;
}

SItemType Item::getItemType()
{
    if (isHelmet()) return SItemType::Helmet;
    if (isChestplate()) return SItemType::Chestplate;
    if (isLeggings()) return SItemType::Leggings;
    if (isBoots()) return SItemType::Boots;
    if (isSword()) return SItemType::Sword;
    if (isPickaxe()) return SItemType::Pickaxe;
    if (isAxe()) return SItemType::Axe;
    if (isShovel()) return SItemType::Shovel;

    return SItemType::None;
}

int Item::getMaxDamage() const
{
    if (mName.empty()) return 0;

    // Exact vanilla Bedrock max durability table by item name (safe across vtable changes)
    if (mName.ends_with("_helmet")) {
        if (mName.starts_with("leather_"))   return 55;
        if (mName.starts_with("golden_"))    return 77;
        if (mName.starts_with("chainmail_")) return 165;
        if (mName.starts_with("iron_"))      return 165;
        if (mName.starts_with("turtle_"))    return 275;
        if (mName.starts_with("diamond_"))   return 363;
        if (mName.starts_with("netherite_")) return 407;
    }
    if (mName.ends_with("_chestplate")) {
        if (mName.starts_with("leather_"))   return 80;
        if (mName.starts_with("golden_"))    return 112;
        if (mName.starts_with("chainmail_")) return 240;
        if (mName.starts_with("iron_"))      return 240;
        if (mName.starts_with("diamond_"))   return 528;
        if (mName.starts_with("netherite_")) return 592;
    }
    if (mName.ends_with("_leggings")) {
        if (mName.starts_with("leather_"))   return 75;
        if (mName.starts_with("golden_"))    return 105;
        if (mName.starts_with("chainmail_")) return 225;
        if (mName.starts_with("iron_"))      return 225;
        if (mName.starts_with("diamond_"))   return 495;
        if (mName.starts_with("netherite_")) return 555;
    }
    if (mName.ends_with("_boots")) {
        if (mName.starts_with("leather_"))   return 65;
        if (mName.starts_with("golden_"))    return 91;
        if (mName.starts_with("chainmail_")) return 195;
        if (mName.starts_with("iron_"))      return 195;
        if (mName.starts_with("diamond_"))   return 429;
        if (mName.starts_with("netherite_")) return 481;
    }
    if (mName.ends_with("_sword") || mName.ends_with("_pickaxe") ||
        mName.ends_with("_axe") || mName.ends_with("_shovel") || mName.ends_with("_hoe")) {
        if (mName.starts_with("wooden_"))    return 59;
        if (mName.starts_with("golden_"))    return 32;
        if (mName.starts_with("stone_"))     return 131;
        if (mName.starts_with("copper_"))    return 190;
        if (mName.starts_with("iron_"))      return 250;
        if (mName.starts_with("diamond_"))   return 1561;
        if (mName.starts_with("netherite_")) return 2031;
    }
    if (mName.find("elytra") != std::string::npos)          return 432;
    if (mName.find("shield") != std::string::npos)          return 336;
    if (mName.find("bow") != std::string::npos && mName.find("crossbow") == std::string::npos && mName.find("bowl") == std::string::npos) return 384;
    if (mName.find("crossbow") != std::string::npos)        return 465;
    if (mName.find("trident") != std::string::npos)         return 250;
    if (mName.find("mace") != std::string::npos)            return 500;
    if (mName.find("shears") != std::string::npos)          return 238;
    if (mName.find("flint_and_steel") != std::string::npos) return 64;
    if (mName.find("fishing_rod") != std::string::npos)     return 384;
    if (mName.find("brush") != std::string::npos)           return 64;

    return 0;
}

bool Item::isFood() const
{
    if (mName.empty()) return false;

    std::string_view name = mName;
    if (auto colon = name.find(':'); colon != std::string_view::npos) {
        name.remove_prefix(colon + 1);
    }

    static constexpr std::array<std::string_view, 40> kFoods = {
        "apple", "golden_apple", "enchanted_golden_apple", "appleenchanted",
        "melon_slice", "melon", "sweet_berries", "glow_berries", "chorus_fruit",
        "carrot", "golden_carrot", "potato", "baked_potato", "poisonous_potato",
        "beetroot", "dried_kelp",
        "beef", "cooked_beef", "porkchop", "cooked_porkchop",
        "mutton", "cooked_mutton", "chicken", "cooked_chicken",
        "rabbit", "cooked_rabbit", "cod", "cooked_cod",
        "salmon", "cooked_salmon", "tropical_fish", "pufferfish", "clownfish", "fish", "cooked_fish",
        "bread", "cookie", "pumpkin_pie", "rotten_flesh", "spider_eye"
    };
    static constexpr std::array<std::string_view, 5> kBowlFoods = {
        "mushroom_stew", "beetroot_soup", "rabbit_stew", "suspicious_stew", "honey_bottle"
    };

    for (auto f : kFoods) {
        if (name == f) return true;
    }
    for (auto f : kBowlFoods) {
        if (name == f) return true;
    }
    return false;
}

