//
//

#pragma once

#include <SDK/heap_rd.hpp>

#include <algorithm>
#include <map>
#include <regex>
#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/MemUtils.hpp>
#include <SDK/Minecraft/ptr/WeakPtr.hpp>
#include "CompoundTag.hpp"
#include "Item.hpp"

class NetworkItemStackDescriptor;

class ItemStackBase {
public:
    uintptr_t** mVfTable;
    WeakPtr<class Item> mItem;
    CompoundTag* mCompoundTag;
    class Block* mBlock;
    short mAuxValue;
    uint8_t mCount;
    bool mValid_DeprecatedSeeComment;
    PAD(0x58);

    Item* getItem() const {
        if (this && mItem)
        {
            return mItem.get();
        }
        return nullptr;
    }

    bool isValid() const {
        return this != nullptr && getItem() != nullptr && mCount > 0;
    }
};

enum class Enchant : int {
    PROTECTION = 0,
    FIRE_PROTECTION = 1,
    FEATHER_FALLING = 2,
    BLAST_PROTECTION = 3,
    PROJECTILE_PROTECTION = 4,
    THORNS = 5,
    RESPIRATION = 6,
    DEPTH_STRIDER = 7,
    AQUA_AFFINITY = 8,
    SHARPNESS = 9,
    SMITE = 10,
    BANE_OF_ARTHROPODS = 11,
    KNOCKBACK = 12,
    FIRE_ASPECT = 13,
    LOOTING = 14,
    EFFICIENCY = 15,
    SILK_TOUCH = 16,
    UNBREAKING = 17,
    FORTUNE = 18,
    POWER = 19,
    PUNCH = 20,
    FLAME = 21,
    BOW_INFINITY = 22,
    LUCK_OF_THE_SEA = 23,
    LURE = 24,
    FROST_WALKER = 25,
    MENDING = 26,
    BINDING = 27,
    VANISHING = 28,
    IMPALING = 29,
    RIPTIDE = 30,
    LOYALTY = 31,
    CHANNELING = 32,
    MULTISHOT = 33,
    PIERCING = 34,
    QUICK_CHARGE = 35,
    SOUL_SPEED = 36,
    SWIFT_SNEAK = 37
};

class ItemStack : public ItemStackBase
{
public:
    PAD(24);
    void reinit(Item* item, int count, int itemData) {
        mVfTable = reinterpret_cast<uintptr_t**>(SigManager::ItemStack_vTable);
        MemUtils::callVirtualFunc<void>(OffsetProvider::ItemStack_reInit, this, item, count, itemData);
    }

    std::string getCustomName() {
        static auto func = SigManager::ItemStack_getCustomName;
        if (!func) return "";
        std::string str;
        MemUtils::callFastcall<std::string*, ItemStack*, std::string*>(func, this, &str);
        return std::string(str.data(), str.size());
    }

    std::map<int, int> gatherEnchants() const
    {
        if (!mCompoundTag) return {};

        std::map<int, int> enchants;
        for (auto& [first, second] : mCompoundTag->data) {
            if (second.type != Tag::Type::List || first != "ench") continue;

            for (const auto list = second.asListTag(); const auto& entry : list->val) {
                if (entry->getId() != Tag::Type::Compound) continue;

                const auto comp = reinterpret_cast<CompoundTag*>(entry);
                auto idTag = comp->get("id");
                auto lvlTag = comp->get("lvl");
                if (!idTag || !lvlTag) continue;
                int id = idTag->asShortTag()->val;
                const int lvl = lvlTag->asShortTag()->val;
                enchants[id] = lvl;
            }
        }

        return enchants;
    }

    int getEnchantValue(int id) const
    {
        auto enchants = gatherEnchants();
        return enchants.contains(id) ? enchants[id] : 0;
    }

    int getEnchantValue(Enchant enchant) const
    {
        return getEnchantValue(static_cast<int>(enchant));
    }

    int getDamage() const
    {
        if (mCompoundTag) {
            if (auto dmgVar = mCompoundTag->get("Damage")) {
                if (dmgVar->type == Tag::Type::Int && dmgVar->asIntTag()) {
                    return dmgVar->asIntTag()->val;
                }
                if (dmgVar->type == Tag::Type::Short && dmgVar->asShortTag()) {
                    return dmgVar->asShortTag()->val;
                }
            }
        }
        return mAuxValue > 0 ? static_cast<int>(mAuxValue) : 0;
    }

    int getMaxDamage() const
    {
        Item* item = getItem();
        return item ? item->getMaxDamage() : 0;
    }

    int getRemainingDurability() const
    {
        int maxDmg = getMaxDamage();
        if (maxDmg <= 0) return 0;
        return std::max(0, maxDmg - getDamage());
    }

    float getDurabilityPercent() const
    {
        int maxDmg = getMaxDamage();
        if (maxDmg <= 0) return 1.0f;
        return std::clamp(static_cast<float>(maxDmg - getDamage()) / static_cast<float>(maxDmg), 0.0f, 1.0f);
    }

    bool hasDurability() const
    {
        return getMaxDamage() > 0;
    }

    static ItemStack fromDescriptor(NetworkItemStackDescriptor const& desc);

    ItemStack()
    {
        memset(this, 0, sizeof(ItemStack));
        mVfTable = reinterpret_cast<uintptr_t**>(SigManager::ItemStack_vTable);
    }

    ItemStack(Item* item, int count, int itemData) {
        memset(this, 0x0, sizeof(ItemStack));
        reinit(item, count, itemData);
    }

    std::string getEnchantName(Enchant enchant) const {
        std::string name = std::string(magic_enum::enum_name(enchant));
        name = StringUtils::toLower(name);
        name = StringUtils::replaceAll(name, "_", " ");
        return name;
    }
};

static_assert(sizeof(ItemStack) == 0x98, "ItemStack size invalid");
