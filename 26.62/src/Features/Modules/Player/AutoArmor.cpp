#include "AutoArmor.hpp"

#include <algorithm>

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

// =========================================================
// Имена слотов для уведомлений
// =========================================================
static const char* sArmorSlotName(int slot)
{
    switch (slot) {
        case 0: return "Helmet";
        case 1: return "Chestplate";
        case 2: return "Leggings";
        case 3: return "Boots";
        default: return "Armor";
    }
}

// =========================================================
// ENABLE / DISABLE
// =========================================================
void AutoArmor::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent,
        &AutoArmor::onBaseTickEvent>(this);
    mLastAction = 0;
    for (int i = 0; i < 4; i++) mLastNotify[i] = 0;
}

void AutoArmor::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,
        &AutoArmor::onBaseTickEvent>(this);
}

// =========================================================
// IS ARMOR LOW — проверяем прочность надетой брони
// =========================================================
bool AutoArmor::isArmorLow(int armorSlot)
{
    if (!mDurabilityCheck.mValue) return false;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;

    auto armorContainer = player->getArmorContainer();
    if (!armorContainer) return false;

    ItemStack* equipped = armorContainer->getItem(armorSlot);
    if (!equipped || !equipped->mItem) return false;

    // Если у предмета нет прочности (например зачарованная книга) — пропускаем
    if (!equipped->hasDurability()) return false;

    float durPct = equipped->getDurabilityPercent();

    // durPct: 1.0 = новый, 0.0 = сломан
    // mDurabilityThresh хранится в процентах (15 = 15%)
    float threshold = mDurabilityThresh.mValue / 100.f;

    return durPct <= threshold;
}

// =========================================================
// FIND BEST ARMOR
//
// armorSlot: 0=helmet, 1=chestplate, 2=leggings, 3=boots
// ignoreDurability: если true — ищем любую замену,
//   даже хуже по значению (используется при низкой прочности)
// =========================================================
int AutoArmor::findBestArmor(int armorSlot, bool ignoreDurability)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;

    auto armorContainer = player->getArmorContainer();
    if (!armorContainer) return -1;

    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    SItemType targetType;
    switch (armorSlot) {
        case 0: targetType = SItemType::Helmet;     break;
        case 1: targetType = SItemType::Chestplate; break;
        case 2: targetType = SItemType::Leggings;   break;
        case 3: targetType = SItemType::Boots;       break;
        default: return -1;
    }

    // Текущая надетая броня
    ItemStack* equipped       = armorContainer->getItem(armorSlot);
    int        equippedValue  = 0;
    bool       equippedFireProt = false;

    if (equipped && equipped->mItem)
    {
        equippedValue    = ItemUtils::getItemValue(equipped);
        equippedFireProt = equipped->getEnchantValue(Enchant::FIRE_PROTECTION) > 0;
    }

    int  bestSlot        = -1;
    int  bestValue       = ignoreDurability ? -1 : equippedValue;
    bool bestHasFireProt = equippedFireProt;

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (!item || !item->mItem) continue;

        // Тип должен совпадать
        if (item->getItem()->getItemType() != targetType) continue;

        // Не рассматриваем предметы с низкой прочностью как замену
        // (чтобы не менять плохое на плохое)
        if (mDurabilityCheck.mValue && item->hasDurability())
        {
            float durPct   = item->getDurabilityPercent();
            float threshold= mDurabilityThresh.mValue / 100.f;
            if (durPct <= threshold) continue;
        }

        int  itemValue    = ItemUtils::getItemValue(item);
        bool hasFireProt  = item->getEnchantValue(Enchant::FIRE_PROTECTION) > 0;

        // Fire Protection preference
        if (mPreferFireProt.mValue)
        {
            // Текущий лучший имеет FireProt, кандидат нет — пропускаем
            if (bestHasFireProt && !hasFireProt && bestValue > 0) continue;

            // Кандидат имеет FireProt, текущий нет — предпочитаем
            if (hasFireProt && !bestHasFireProt)
            {
                bestSlot        = i;
                bestValue       = itemValue;
                bestHasFireProt = true;
                continue;
            }
        }

        if (itemValue > bestValue)
        {
            bestSlot        = i;
            bestValue       = itemValue;
            bestHasFireProt = hasFireProt;
        }
    }

    return bestSlot;
}

// =========================================================
// OFFHAND HELPERS
// =========================================================

static bool nameContains(ItemStack* stack, const std::string& sub)
{
    if (!stack || !stack->mItem) return false;
    std::string name = stack->getItem()->mName;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return name.find(sub) != std::string::npos;
}

int AutoArmor::findTotem()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (nameContains(item, "totem")) return i;
    }
    return -1;
}

int AutoArmor::findShield()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    auto supplies = player->getSupplies();
    if (!supplies) return -1;
    auto container = supplies->getContainer();
    if (!container) return -1;

    for (int i = 0; i < 36; i++)
    {
        auto item = container->getItem(i);
        if (nameContains(item, "shield")) return i;
    }
    return -1;
}

void AutoArmor::equipOffhand(int inventorySlot)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto inventory = supplies->getContainer();
    if (!inventory) return;

    auto* offhandContainer = player->getOffhandContainer();
    if (!offhandContainer) return;

    ItemStack* srcItem = inventory->getItem(inventorySlot);
    if (!srcItem || !srcItem->mItem) return;

    ItemStack* dstItem = offhandContainer->getItem(0);
    static ItemStack emptyStack = ItemStack();
    if (!dstItem) dstItem = &emptyStack;

    InventoryAction action1(inventorySlot, srcItem, dstItem);
    action1.mSource.mType        = InventorySourceType::ContainerInventory;
    action1.mSource.mContainerId = static_cast<char>(ContainerID::Inventory);

    InventoryAction action2(0, dstItem, srcItem);
    action2.mSource.mType        = InventorySourceType::ContainerInventory;
    action2.mSource.mContainerId = static_cast<char>(ContainerID::Offhand);

    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(action1);
    cit->data.addAction(action2);
    pkt->mTransaction = std::move(cit);

    ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
}

// =========================================================
// MAIN TICK
// =========================================================
void AutoArmor::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;
    auto armorContainer = player->getArmorContainer();
    if (!armorContainer) return;

    // Delay check
    if (!mInstant.mValue)
    {
        if (NOW - mLastAction < static_cast<uint64_t>(mDelay.mValue))
            return;
    }

    // -------------------------------------------------------
    // DURABILITY CHECK — первый проход
    // Если броня ниже порога — сразу ищем замену
    // -------------------------------------------------------
    if (mDurabilityCheck.mValue)
    {
        for (int slot = 0; slot < 4; slot++)
        {
            if (!isArmorLow(slot)) continue;

            // Ищем замену (ignoreDurability=true — берём что угодно лучше -1)
            int replacementSlot = findBestArmor(slot, true);

            if (replacementSlot != -1)
            {
                // Нашли замену — надеваем
                container->equipArmor(replacementSlot);
                mLastAction = NOW;

                if (!mInstant.mValue) return;
            }
            else
            {
                // Замены нет — уведомляем (не чаще раза в 10 секунд)
                if (mNotifyLow.mValue && NOW - mLastNotify[slot] > 10000)
                {
                    mLastNotify[slot] = NOW;

                    // Получаем текущий процент прочности для уведомления
                    ItemStack* equipped = armorContainer->getItem(slot);
                    std::string pctStr = "?";
                    if (equipped && equipped->mItem && equipped->hasDurability())
                    {
                        int pct = (int)(equipped->getDurabilityPercent() * 100.f);
                        pctStr  = std::to_string(pct);
                    }

                    NotifyUtils::notify(
                        "§c" + std::string(sArmorSlotName(slot))
                        + " §elow durability (" + pctStr + "%)! §cNo replacement found.",
                        4.f,
                        Notification::Type::Warning
                    );
                }
            }
        }
    }

    // -------------------------------------------------------
    // NORMAL CHECK — второй проход
    // Ищем лучшую броню независимо от прочности
    // -------------------------------------------------------
    for (int slot = 0; slot < 4; slot++)
    {
        // findBestArmor с ignoreDurability=false —
        // вернёт слот только если там что-то лучше текущего
        int bestInvSlot = findBestArmor(slot, false);

        if (bestInvSlot != -1)
        {
            container->equipArmor(bestInvSlot);
            mLastAction = NOW;

            if (!mInstant.mValue) return;
        }
    }

    // -------------------------------------------------------
    // DROP WORSE — выбрасываем хлам
    // -------------------------------------------------------
    if (mDropWorse.mValue)
    {
        for (int i = 0; i < 36; i++)
        {
            auto item = container->getItem(i);
            if (!item || !item->mItem) continue;

            auto itemType = item->getItem()->getItemType();

            int armorSlot = -1;
            switch (itemType) {
                case SItemType::Helmet:     armorSlot = 0; break;
                case SItemType::Chestplate: armorSlot = 1; break;
                case SItemType::Leggings:   armorSlot = 2; break;
                case SItemType::Boots:      armorSlot = 3; break;
                default: continue;
            }

            ItemStack* equipped = armorContainer->getItem(armorSlot);
            if (!equipped || !equipped->mItem) continue;

            // Не выбрасываем FireProt если включена настройка
            if (mPreferFireProt.mValue &&
                item->getEnchantValue(Enchant::FIRE_PROTECTION) > 0)
                continue;

            // Не выбрасываем броню с хорошей прочностью
            // (вдруг надетая сломается — она пригодится)
            if (mDurabilityCheck.mValue && item->hasDurability())
            {
                float durPct    = item->getDurabilityPercent();
                float threshold = mDurabilityThresh.mValue / 100.f;
                if (durPct > threshold) continue;
            }

            int equippedValue = ItemUtils::getItemValue(equipped);
            int itemValue     = ItemUtils::getItemValue(item);

            if (itemValue < equippedValue)
            {
                container->dropSlot(i);
                mLastAction = NOW;

                if (!mInstant.mValue) return;
            }
        }
    }

    // -------------------------------------------------------
    // OFFHAND — Totem / Shield
    // -------------------------------------------------------
    if (mAutoTotem.mValue || mAutoShield.mValue)
    {
        auto* offhandContainer = player->getOffhandContainer();
        if (offhandContainer)
        {
            auto* offhandItem = offhandContainer->getItem(0);
            bool hasTotem  = nameContains(offhandItem, "totem");
            bool hasShield = nameContains(offhandItem, "shield");

            int invSlot = -1;

            // Приоритет: Totem > Shield
            if (mAutoTotem.mValue && !hasTotem)
            {
                invSlot = findTotem();
            }

            if (invSlot == -1 && mAutoShield.mValue && !hasShield)
            {
                invSlot = findShield();
            }

            if (invSlot != -1)
            {
                equipOffhand(invSlot);
                mLastAction = NOW;
                if (!mInstant.mValue) return;
            }
        }
    }
}