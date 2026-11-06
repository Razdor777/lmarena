//
// Created by vastrakai on 7/5/2024.
//

#pragma once

#include <string>
#include <Utils/MemUtils.hpp>

enum class SItemType
{
    Helmet,
    Chestplate,
    Leggings,
    Boots,
    Sword,
    Pickaxe,
    Axe,
    Shovel,
    None
};

class Item {
public:
    CLASS_FIELD(short, mItemId, 0xAA);
    CLASS_FIELD(std::string, mName, 0xD8);

    // Carried over from the 1.21.44 SDK. Offsets 0x244 / 0x264 were at least one
    // content update old there -- re-verify with a dump before trusting them.
    CLASS_FIELD(int, mArmorItemType, 0x244);
    CLASS_FIELD(int, mProtection, 0x264);

    // vtable index 35 = Item::getMaxDamage() (returns short). Index unchanged in
    // the 1.26.x SDK, but confirm in game: wrong index => garbage in ArmorHUD.
    int getMaxDamage() const {
        return MemUtils::callVirtualFunc<short>(35, const_cast<Item*>(this));
    }

    // vtable index 15 = Item::isBlockItem() -- the only reliable "is it a block"
    // check: some block items (campfire) leave ItemStack::mBlock empty.
    bool isBlockItem() const {
        return MemUtils::callVirtualFunc<bool>(15, const_cast<Item*>(this));
    }

    int getArmorSlot();
    bool isHelmet();
    bool isChestplate();
    bool isLeggings();
    bool isBoots();
    bool isSword();
    bool isPickaxe();
    bool isAxe();
    bool isShovel();
    int getItemTier();
    int getArmorTier();
    SItemType getItemType();

};