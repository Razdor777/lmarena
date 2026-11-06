//
// JavaInventory.cpp
//
// Java-style inventory swapping. See JavaInventory.hpp for the idea.
//

#include "JavaInventory.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.hpp>
#include <Features/FeatureManager.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

void JavaInventory::onEnable()
{
    mKeyStates.clear();
    mLastSwap = 0;

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &JavaInventory::onBaseTickEvent>(this);
}

void JavaInventory::onDisable()
{
    mKeyStates.clear();
    mLastSwap = 0;

    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &JavaInventory::onBaseTickEvent>(this);
}

bool JavaInventory::isKeyJustPressed(int vk)
{
    if (vk <= 0) return false;

    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool was  = mKeyStates[vk];
    mKeyStates[vk]  = down;

    return down && !was;
}

void JavaInventory::sendSwap(int fromSlot, ContainerID fromContainer, ItemStack* fromItem,
                             int toSlot,   ContainerID toContainer,   ItemStack* toItem)
{
    auto* client = ClientInstance::get();
    if (!client) return;

    // An empty slot still has to be described in the transaction, otherwise the
    // server sees a half-swap. ArmorSlotUnlock does the same with one shared
    // empty stack.
    static ItemStack emptyStack = ItemStack();
    if (!fromItem) fromItem = &emptyStack;
    if (!toItem)   toItem   = &emptyStack;

    InventoryAction action1(fromSlot, fromItem, toItem);
    action1.mSource.mType        = InventorySourceType::ContainerInventory;
    action1.mSource.mContainerId = static_cast<char>(fromContainer);

    InventoryAction action2(toSlot, toItem, fromItem);
    action2.mSource.mType        = InventorySourceType::ContainerInventory;
    action2.mSource.mContainerId = static_cast<char>(toContainer);

    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(action1);
    cit->data.addAction(action2);
    pkt->mTransaction = std::move(cit);

    client->getPacketSender()->sendToServer(pkt.get());
}

void JavaInventory::onBaseTickEvent(BaseTickEvent& event)
{
    auto* player = event.mActor;
    if (!player) return;

    // Poll every key each tick, even when nothing is hovered, so the edge state
    // can never go stale.
    int pressedNumber = -1;
    for (int n = 0; n < 9; ++n)
        if (isKeyJustPressed('1' + n)) pressedNumber = n;

    const bool pressedF = isKeyJustPressed('F');

    if (pressedNumber < 0 && !pressedF) return;

    // Only the game knows the hovered slot, and only while a container screen
    // is open.
    if (!ContainerScreenControllerHook::isScreenActive()) return;

    const int hovered = ContainerScreenControllerHook::sHoveredInvSlot.load();
    if (hovered < 0) return;

    const uint64_t now = GetTickCount64();
    if (mLastSwap != 0 && now - mLastSwap < static_cast<uint64_t>(mCooldown.mValue)) return;

    auto* supplies = player->getSupplies();
    if (!supplies) return;
    auto* inventory = supplies->getContainer();
    if (!inventory) return;

    ItemStack* hoveredStack = inventory->getItem(hovered);

    // ---- 1..9: hovered slot <-> that hotbar slot ----
    if (pressedNumber >= 0 && mSwapWithNumbers.mValue)
    {
        const int hotbarSlot = pressedNumber;   // inventory 0..8 is the hotbar
        if (hovered == hotbarSlot) return;      // already there, nothing to do

        ItemStack* hotbarStack = inventory->getItem(hotbarSlot);

        const bool hoveredEmpty = (!hoveredStack || !hoveredStack->mItem);
        const bool hotbarEmpty  = (!hotbarStack  || !hotbarStack->mItem);
        if (hoveredEmpty && hotbarEmpty) return;

        sendSwap(hovered,    ContainerID::Inventory, hoveredStack,
                 hotbarSlot, ContainerID::Inventory, hotbarStack);

        mLastSwap = now;

        if (mNotify.mValue)
        {
            const bool moved = hotbarEmpty;
            NotifyUtils::notify(
                (moved ? "§aMoved to hotbar §f" : "§aSwapped with hotbar §f") +
                    std::to_string(hotbarSlot + 1),
                2.f, Notification::Type::Info);
        }
        return;
    }

    // ---- F: hovered slot <-> offhand ----
    if (pressedF && mSwapWithF.mValue)
    {
        auto* offhand = player->getOffhandContainer();
        if (!offhand) return;

        ItemStack* offhandStack = offhand->getItem(0);

        const bool hoveredEmpty = (!hoveredStack || !hoveredStack->mItem);
        const bool offhandEmpty = (!offhandStack || !offhandStack->mItem);
        if (hoveredEmpty && offhandEmpty) return;

        sendSwap(hovered, ContainerID::Inventory, hoveredStack,
                 0,       ContainerID::Offhand,   offhandStack);

        mLastSwap = now;

        if (mNotify.mValue)
        {
            NotifyUtils::notify(
                (offhandEmpty ? "§aMoved to offhand §f" : "§aSwapped with offhand §f") +
                    std::to_string(hovered),
                2.f, Notification::Type::Info);
        }
    }
}
