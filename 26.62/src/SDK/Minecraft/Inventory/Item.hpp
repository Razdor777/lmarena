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
    // 26.62: оффсеты mItemId/mName обновлены по новой базе Dexko
    CLASS_FIELD(short, mItemId, 0xAA);
    CLASS_FIELD(std::string, mName, 0xD8);
    // TODO(26.62): оффсеты ниже остались от 1.21.44 — перепроверить на новой версии!
    CLASS_FIELD(int, mArmorItemType, 0x244);
    CLASS_FIELD(int, mProtection, 0x264);

    // vtable index 35 = Item::getMaxDamage() (снято на 1.21.44 — TODO перепроверить)
    int getMaxDamage() const {
        return MemUtils::callVirtualFunc<short>(35, const_cast<Item*>(this));
    }

    // vtable index 15 = Item::isBlockItem() — "является ли предмет блоком".
    // Надёжный способ отличить блоки (в т.ч. campfire) от инструментов.
    // (снято на 1.21.44 — TODO перепроверить индекс на 26.62)
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