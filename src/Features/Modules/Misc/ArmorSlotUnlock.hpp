#pragma once
//
// ArmorSlotUnlock — equip any item into any armor/offhand slot
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class ArmorSlotUnlock : public ModuleBase<ArmorSlotUnlock> {
public:
    EnumSetting mTargetSlot = EnumSetting("Target Slot",
        "Armor/offhand slot to equip item to", 0,
        {"Head", "Chest", "Legs", "Feet", "Offhand"});

    NumberSetting mInventorySlot = NumberSetting("Inventory Slot",
        "Inventory slot to take item from (0-35)",
        0.f, 0.f, 35.f, 1.f);

    BoolSetting mAutoEquip = BoolSetting("Auto Equip",
        "Equip once on enable then auto-disable", true);

    ArmorSlotUnlock() : ModuleBase("ArmorSlotUnlock",
        "Equip any item to any armor/offhand slot", ModuleCategory::Misc, 0, false)
    {
        addSettings(&mTargetSlot, &mInventorySlot, &mAutoEquip);
        mNames = {
            {Lowercase,       "armorslotunlock"},
            {LowercaseSpaced, "armor slot unlock"},
            {Normal,          "ArmorSlotUnlock"},
            {NormalSpaced,    "Armor Slot Unlock"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);

private:
    bool mHasEquipped = false;

    void equipToSlot(int inventorySlot, int targetSlot);
};