#pragma once
//
// JavaInventory — Java-style inventory swapping inside an open inventory screen:
//
//   * hover a slot, press 1..9   -> that stack trades places with the hotbar slot
//   * hover a slot, press F      -> that stack trades places with the offhand
//
// The slot under the cursor is known only to the game, so everything hangs off
// ContainerScreenControllerHook::onContainerSlotHovered, which hands us
// (collectionName, slot) every time the hovered slot changes. Nothing here needs
// its own signature or offset: the two packets are plain InventoryTransaction
// actions, the same shape ArmorSlotUnlock already uses for armour.
//
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>

#include <unordered_map>

class JavaInventory : public ModuleBase<JavaInventory>
{
public:
    BoolSetting mSwapWithNumbers = BoolSetting("Swap With Numbers",
        "Hover a slot and press 1-9 to swap it with that hotbar slot", true);

    BoolSetting mSwapWithF = BoolSetting("Swap With F",
        "Hover a slot and press F to swap it with the offhand", true);

    NumberSetting mCooldown = NumberSetting("Cooldown",
        "Minimum ms between two swaps", 120.f, 0.f, 1000.f, 10.f);

    BoolSetting mNotify = BoolSetting("Notify",
        "Chat notification on every swap", false);

    JavaInventory() : ModuleBase("JavaInventory",
        "Java-style swapping with number keys and F while the inventory is open",
        ModuleCategory::Misc, 0, false)
    {
        addSettings(&mSwapWithNumbers, &mSwapWithF, &mCooldown, &mNotify);

        mNames = {
            {Lowercase,       "javainventory"},
            {LowercaseSpaced, "java inventory"},
            {Normal,          "JavaInventory"},
            {NormalSpaced,    "Java Inventory"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);

private:
    bool isKeyJustPressed(int vk);

    // One transaction, two actions — source slot loses the stack, destination
    // slot gains it and vice versa. Same shape as ArmorSlotUnlock::equipToSlot.
    void sendSwap(int fromSlot, ContainerID fromContainer, class ItemStack* fromItem,
                  int toSlot,   ContainerID toContainer,   class ItemStack* toItem);

    std::unordered_map<int, bool> mKeyStates;
    uint64_t mLastSwap = 0;
};
