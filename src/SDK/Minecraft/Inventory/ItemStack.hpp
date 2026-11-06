//
// Created by vastrakai on 7/5/2024.
//

#pragma once

#include <regex>
#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/heap_rd.hpp>
#include <SDK/Minecraft/ptr/WeakPtr.hpp>
#include <Utils/MemUtils.hpp>

#include "CompoundTag.hpp"


// Layout re-pinned for the 26.62 build.
//
// 1.21.44:  Item** mItem; CompoundTag*; Block*; short; int8_t count; bool valid; PAD(0x60)
// 26.62  :  WeakPtr<Item> (ref-counted, 16 bytes); CompoundTag*; Block*; short;
//           uint8_t count; bool valid; PAD(0x58)
// sizeof(ItemStackBase) stays 0x88 and sizeof(ItemStack) stays 0x98, which is
// what the static_asserts below check.  WeakPtr keeps a SharedCounter* instead
// of a raw Item**, so every plain `*mItem` dereference in the old code had to go.
class ItemStackBase {
public:
    uintptr_t** mVfTable;
    WeakPtr<class Item> mItem;
    CompoundTag* mCompoundTag;
    class Block* mBlock;
    short mAuxValue;
    uint8_t mCount;
    // Kept so existing call sites that test `valid` still compile. The engine no
    // longer uses it as a validity flag - always ask getItem() instead.
    bool valid;

    Item* getItem() const {
        return mItem.get();
    }

    bool isValid() const {
        return getItem() != nullptr;
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
    // 26.62 adds 24 bytes of trailing state after the base class; mStackNetId
    // no longer sits at 0x88, so it is kept as a named tail only for size.
    PAD(24);

    void reinit(Item* item, int count, int itemData) {
        mVfTable = reinterpret_cast<uintptr_t**>(SigManager::ItemStack_vTable);
        MemUtils::callVirtualFunc<void>(OffsetProvider::ItemStack_reInit, this, item, count, itemData);
    }

    std::string getCustomName() {
        static auto func = SigManager::ItemStack_getCustomName;
        std::string str;
        if (!func) return {};
        MemUtils::callFastcall<void, ItemStack*, std::string*>(func, this, &str);
        // The engine hands the text back in a buffer it owns; copy it out and
        // release it through the game heap wrapper (mimalloc or the process heap).
        std::string out(str.c_str(), str.size());
        if (str.capacity() > 15) {
            void* raw = &str;
            if (mr::ensure()) {
                mr::freep(raw);
            } else {
                HeapFree(GetProcessHeap(), 0, raw);
            }
        }
        return out;
    }

    std::map<int, int> gatherEnchants()
    {
        if (!mCompoundTag) return {};

        std::map<int, int> enchants;
        for (auto& [first, second] : mCompoundTag->data) {
            if (second.type != Tag::Type::List || first != "ench") continue;

            for (const auto list = second.asListTag(); const auto& entry : list->val) {
                if (entry->getId() != Tag::Type::Compound) continue;

                const auto comp = reinterpret_cast<CompoundTag*>(entry);
                int id = comp->get("id")->asShortTag()->val;
                const int lvl = comp->get("lvl")->asShortTag()->val;
                enchants[id] = lvl;
            }
        }

        return enchants;
    }

    int getEnchantValue(int id)
    {
        auto enchants = gatherEnchants();
        return enchants.contains(id) ? enchants[id] : 0;
    }

    int getEnchantValue(Enchant enchant)
    {
        return getEnchantValue(static_cast<int>(enchant));
    }

    // =====================================================
    // DURABILITY / DAMAGE METHODS
    // =====================================================
    // ItemStackBase::mAuxValue offset: 0x20 (содержит damage)
    
    int getDamage() const {
        return static_cast<int>(mAuxValue);
    }

    int getMaxDamage() const {
        Item* item = getItem();
        if (!item) return 0;
        return item->getMaxDamage(); // вызываем виртуальную функцию из vtable Item
    }

    int getRemainingDurability() const {
        int maxDmg = getMaxDamage();
        if (maxDmg <= 0) return 0;
        return maxDmg - getDamage();
    }

    float getDurabilityPercent() const {
        int maxDmg = getMaxDamage();
        if (maxDmg <= 0) return 1.0f;
        return static_cast<float>(getRemainingDurability()) / static_cast<float>(maxDmg);
    }

    bool isArmorEnchantable() const {
        int maxDmg = getMaxDamage();
        return maxDmg > 0;
    }

    bool hasDurability() const {
        return getMaxDamage() > 0;
    }

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

    static ItemStack fromDescriptor(class NetworkItemStackDescriptor const& desc);
};

static_assert(sizeof(ItemStack) == 0x98, "ItemStack size invalid");