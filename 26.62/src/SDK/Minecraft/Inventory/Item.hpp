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

    // UNVERIFIED для 1.26.62: индексы vtable сняты с 1.21.44.
    // Если HUD прочности показывает нули — перепроверить индексы 35/15.
    int getMaxDamage() const {
        return MemUtils::callVirtualFunc<short>(35, const_cast<Item*>(this));
    }

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