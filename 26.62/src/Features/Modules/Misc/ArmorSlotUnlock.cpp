//
// ArmorSlotUnlock.cpp
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

void ArmorSlotUnlock::equipToSlot(int inventorySlot, int targetSlot)
{
    auto* client = ClientInstance::get();
    if (!client) return;
    auto* player = client->getLocalPlayer();
    if (!player) return;
    auto* supplies = player->getSupplies();
    if (!supplies) return;
    auto* inventory = supplies->getContainer();
    if (!inventory) return;

    SimpleContainer* targetContainer = nullptr;
    ContainerID      targetContainerId = ContainerID::Armor;
    int              actualSlot = targetSlot;

    if (targetSlot == 4) { // Offhand
        targetContainer   = player->getOffhandContainer();
        targetContainerId = ContainerID::Offhand;
        actualSlot        = 0;
    } else {
        targetContainer   = player->getArmorContainer();
        targetContainerId = ContainerID::Armor;
        actualSlot        = targetSlot;
    }
    if (!targetContainer) return;

    ItemStack* srcItem = inventory->getItem(inventorySlot);
    if (!srcItem || !srcItem->mItem) {
        spdlog::warn("[ArmorSlotUnlock] No item in inventory slot {}", inventorySlot);
        return;
    }

    ItemStack* dstItem = targetContainer->getItem(actualSlot);
    static ItemStack emptyStack = ItemStack();
    if (!dstItem) dstItem = &emptyStack;

    InventoryAction action1(inventorySlot, srcItem, dstItem);
    action1.mSource.mType        = InventorySourceType::ContainerInventory;
    action1.mSource.mContainerId = static_cast<char>(ContainerID::Inventory);

    InventoryAction action2(actualSlot, dstItem, srcItem);
    action2.mSource.mType        = InventorySourceType::ContainerInventory;
    action2.mSource.mContainerId = static_cast<char>(targetContainerId);

    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(action1);
    cit->data.addAction(action2);
    pkt->mTransaction = std::move(cit);

    client->getPacketSender()->sendToServer(pkt.get());

    const char* name = (targetSlot == 4) ? "offhand" : "armor";
    spdlog::info("[ArmorSlotUnlock] Sent: inv[{}] -> {}[{}]", inventorySlot, name, actualSlot);
}

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

void ArmorSlotUnlock::onBaseTickEvent(BaseTickEvent& event)
{
    auto* player = event.mActor;
    if (!player) return;

    int invSlot   = static_cast<int>(mInventorySlot.mValue);
    int armorSlot = static_cast<int>(mTargetSlot.mValue);

    if (mAutoEquip.mValue) {
        if (!mHasEquipped) {
            equipToSlot(invSlot, armorSlot);
            mHasEquipped = true;
            setEnabled(false);
        }
        return;
    }

    bool slotEmpty = false;
    if (armorSlot == 4) {
        auto* off = player->getOffhandContainer();
        if (!off) return;
        ItemStack* it = off->getItem(0);
        slotEmpty = (!it || !it->mItem);
    } else {
        auto* arm = player->getArmorContainer();
        if (!arm) return;
        ItemStack* it = arm->getItem(armorSlot);
        slotEmpty = (!it || !it->mItem);
    }

    if (slotEmpty) {
        equipToSlot(invSlot, armorSlot);
    }
}