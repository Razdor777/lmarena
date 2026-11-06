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
    // Из мастер-ветки (1.21.44): vtable index 35 = Item::getMaxDamage(), возвращает short.
    int getMaxDamage() const {
        return MemUtils::callVirtualFunc<short>(35, const_cast<Item*>(this));
    }
    // vtable index 15 = Item::isBlockItem()
    bool isBlockItem() const {
        return MemUtils::callVirtualFunc<bool>(15, const_cast<Item*>(this));
    }
    CLASS_FIELD(std::string, mName, 0xD8);

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