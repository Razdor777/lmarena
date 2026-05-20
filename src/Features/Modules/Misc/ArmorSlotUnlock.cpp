//
// ArmorSlotUnlock.cpp
// Equip any item into any armor slot via InventoryTransactionPacket
//

#include "ArmorSlotUnlock.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Features/FeatureManager.hpp>

// ============================================================
// equipToSlot
//
// Sends an InventoryTransactionPacket to move an item from
// any inventory slot to any armor slot, ignoring item type.
//
// This is the same approach as Inventory::equipArmor() but
// we specify the target armor slot manually instead of using
// item->getArmorSlot().
// ============================================================
void ArmorSlotUnlock::equipToSlot(int inventorySlot, int armorSlot)
{
    auto* client = ClientInstance::get();
    if (!client) return;

    auto* player = client->getLocalPlayer();
    if (!player) return;

    auto* supplies = player->getSupplies();
    if (!supplies) return;

    auto* inventory = supplies->getContainer();
    if (!inventory) return;

    auto* armorContainer = player->getArmorContainer();
    if (!armorContainer) return;

    // Source item from inventory
    ItemStack* srcItem = inventory->getItem(inventorySlot);
    if (!srcItem || !srcItem->mItem)
    {
        spdlog::warn("[ArmorSlotUnlock] No item in inventory slot {}", inventorySlot);
        return;
    }

    // Current item in target armor slot (may be empty)
    ItemStack* dstItem = armorContainer->getItem(armorSlot);

    // Fallback empty stack if armor slot is empty
    static ItemStack emptyStack = ItemStack();
    if (!dstItem) dstItem = &emptyStack;

    // ---- Action 1: inventory loses the item ----
    InventoryAction action1(inventorySlot, srcItem, dstItem);
    action1.mSource.mType        = InventorySourceType::ContainerInventory;
    action1.mSource.mContainerId = static_cast<char>(ContainerID::Inventory);

    // ---- Action 2: armor slot receives the item ----
    // Key: we use armorSlot (0-3) directly, NOT item->getArmorSlot()
    InventoryAction action2(armorSlot, dstItem, srcItem);
    action2.mSource.mType        = InventorySourceType::ContainerInventory;
    action2.mSource.mContainerId = static_cast<char>(ContainerID::Armor);

    // ---- Build and send the packet ----
    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(action1);
    cit->data.addAction(action2);
    pkt->mTransaction = std::move(cit);

    client->getPacketSender()->sendToServer(pkt.get());

    spdlog::info("[ArmorSlotUnlock] Sent transaction: inv[{}] -> armor[{}]",
        inventorySlot, armorSlot);
}

// ============================================================
// onEnable / onDisable
// ============================================================
void ArmorSlotUnlock::onEnable()
{
    mHasEquipped = false;
    gFeatureManager->mDispatcher->listen<BaseTickEvent,
        &ArmorSlotUnlock::onBaseTickEvent>(this);
}

void ArmorSlotUnlock::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,
        &ArmorSlotUnlock::onBaseTickEvent>(this);
}

// ============================================================
// onBaseTickEvent
// ============================================================
void ArmorSlotUnlock::onBaseTickEvent(BaseTickEvent& event)
{
    auto* player = event.mActor;
    if (!player) return;

    int invSlot   = static_cast<int>(mInventorySlot.mValue);
    int armorSlot = static_cast<int>(mTargetSlot.mValue);

    if (mAutoEquip.mValue)
    {
        // One-shot: equip once then disable
        if (!mHasEquipped)
        {
            equipToSlot(invSlot, armorSlot);
            mHasEquipped = true;

            // Auto-disable after equipping
            setEnabled(false);
        }
        return;
    }

    // Continuous mode: keep item in armor slot every tick
    auto* armorContainer = player->getArmorContainer();
    if (!armorContainer) return;

    ItemStack* armorItem = armorContainer->getItem(armorSlot);
    bool slotEmpty = (!armorItem || !armorItem->mItem);

    if (slotEmpty)
    {
        equipToSlot(invSlot, armorSlot);
    }
}
