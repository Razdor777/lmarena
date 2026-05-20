#include "ItemDupe.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

void ItemDupe::onEnable() {
    mLastAttempt = 0;
    mAttemptCount = 0;
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &ItemDupe::onBaseTickEvent>(this);
    NotifyUtils::notify("§eItemDupe enabled — experimental!", 3.f, Notification::Type::Warning);
}

void ItemDupe::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &ItemDupe::onBaseTickEvent>(this);
    NotifyUtils::notify("§eItemDupe: " + std::to_string(mAttemptCount) + " attempts made",
        2.f, Notification::Type::Info);
}

void ItemDupe::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    uint64_t now = NOW;
    uint64_t delayMs = static_cast<uint64_t>(mDelay.mValue) * 50;
    if (now - mLastAttempt < delayMs) return;
    mLastAttempt = now;

    switch (mMethod.mValue) {
        case 0: tryDoubleTransaction(player); break;
        case 1: trySwapExploit(player); break;
        case 2: tryDropDupe(player); break;
    }
    mAttemptCount++;
}

// Method 1: Send the same inventory transaction twice
// The idea: server processes item move, then processes it again = item exists in both slots
void ItemDupe::tryDoubleTransaction(Actor* player)
{
    if (!player) return;
    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;

    int srcSlot = static_cast<int>(mSlot.mValue);
    ItemStack* srcItem = container->getItem(srcSlot);
    if (!srcItem || !srcItem->mItem) return;

    // Find empty slot for destination
    int dstSlot = -1;
    for (int i = 0; i < 36; i++) {
        if (i == srcSlot) continue;
        ItemStack* item = container->getItem(i);
        if (!item || !item->mItem) { dstSlot = i; break; }
    }
    if (dstSlot == -1) return;

    static ItemStack emptyStack = ItemStack();

    // Create swap transaction
    InventoryAction action1(srcSlot, srcItem, &emptyStack);
    action1.mSource.mType = InventorySourceType::ContainerInventory;
    action1.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);

    InventoryAction action2(dstSlot, &emptyStack, srcItem);
    action2.mSource.mType = InventorySourceType::ContainerInventory;
    action2.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);

    auto ci = ClientInstance::get();
    if (!ci) return;
    auto sender = ci->getPacketSender();
    if (!sender) return;

    // Send same transaction TWICE in rapid succession
    for (int i = 0; i < 2; i++) {
        auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ComplexInventoryTransaction>();
        cit->data.addAction(action1);
        cit->data.addAction(action2);
        pkt->mTransaction = std::move(cit);
        sender->sendToServer(pkt.get());
    }

    spdlog::info("[ItemDupe] DoubleTransaction: slot {} -> {}", srcSlot, dstSlot);
}

// Method 2: Swap item to two different slots simultaneously
void ItemDupe::trySwapExploit(Actor* player)
{
    if (!player) return;
    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;

    int srcSlot = static_cast<int>(mSlot.mValue);
    ItemStack* srcItem = container->getItem(srcSlot);
    if (!srcItem || !srcItem->mItem) return;

    // Find two empty slots
    int dst1 = -1, dst2 = -1;
    for (int i = 0; i < 36; i++) {
        if (i == srcSlot) continue;
        ItemStack* item = container->getItem(i);
        if (!item || !item->mItem) {
            if (dst1 == -1) dst1 = i;
            else if (dst2 == -1) { dst2 = i; break; }
        }
    }
    if (dst1 == -1 || dst2 == -1) return;

    static ItemStack emptyStack = ItemStack();
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto sender = ci->getPacketSender();
    if (!sender) return;

    // Packet 1: move src -> dst1
    {
        auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ComplexInventoryTransaction>();
        InventoryAction a1(srcSlot, srcItem, &emptyStack);
        a1.mSource.mType = InventorySourceType::ContainerInventory;
        a1.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);
        InventoryAction a2(dst1, &emptyStack, srcItem);
        a2.mSource.mType = InventorySourceType::ContainerInventory;
        a2.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);
        cit->data.addAction(a1);
        cit->data.addAction(a2);
        pkt->mTransaction = std::move(cit);
        sender->sendToServer(pkt.get());
    }

    // Packet 2: move src -> dst2 (same source, different destination)
    {
        auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
        auto cit = std::make_unique<ComplexInventoryTransaction>();
        InventoryAction a1(srcSlot, srcItem, &emptyStack);
        a1.mSource.mType = InventorySourceType::ContainerInventory;
        a1.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);
        InventoryAction a2(dst2, &emptyStack, srcItem);
        a2.mSource.mType = InventorySourceType::ContainerInventory;
        a2.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);
        cit->data.addAction(a1);
        cit->data.addAction(a2);
        pkt->mTransaction = std::move(cit);
        sender->sendToServer(pkt.get());
    }

    spdlog::info("[ItemDupe] SwapExploit: slot {} -> {}, {}", srcSlot, dst1, dst2);
}

// Method 3: Drop and pick up timing exploit
void ItemDupe::tryDropDupe(Actor* player)
{
    if (!player) return;
    auto supplies = player->getSupplies();
    if (!supplies) return;
    auto container = supplies->getContainer();
    if (!container) return;

    int srcSlot = static_cast<int>(mSlot.mValue);
    ItemStack* srcItem = container->getItem(srcSlot);
    if (!srcItem || !srcItem->mItem) return;

    static ItemStack emptyStack = ItemStack();
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto sender = ci->getPacketSender();
    if (!sender) return;

    // Drop the item
    InventoryAction dropAction(srcSlot, srcItem, &emptyStack);
    dropAction.mSource.mType = InventorySourceType::ContainerInventory;
    dropAction.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);

    InventoryAction worldAction(0, &emptyStack, srcItem);
    worldAction.mSource.mType = InventorySourceType::WorldInteraction;
    worldAction.mSource.mContainerId = static_cast<int>(ContainerID::Inventory);

    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(dropAction);
    cit->data.addAction(worldAction);
    pkt->mTransaction = std::move(cit);

    // Send drop twice
    sender->sendToServer(pkt.get());
    sender->sendToServer(pkt.get());

    spdlog::info("[ItemDupe] DropDupe: slot {}", srcSlot);
}
