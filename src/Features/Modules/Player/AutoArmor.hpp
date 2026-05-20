#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class AutoArmor : public ModuleBase<AutoArmor>
{
public:
    NumberSetting mDelay          = NumberSetting("Delay", "Delay between equip actions (ms)", 50, 0, 500, 10);
    BoolSetting   mInstant        = BoolSetting("Instant", "Equip all pieces in one tick", true);
    BoolSetting   mDropWorse      = BoolSetting("Drop Worse", "Drop worse armor from inventory", false);
    BoolSetting   mPreferFireProt = BoolSetting("Prefer Fire Prot", "Keep fire protection armor even if lower tier", false);

    // --- Durability ---
    BoolSetting   mDurabilityCheck   = BoolSetting("Durability Check", "Auto-replace armor below threshold", true);
    NumberSetting mDurabilityThresh  = NumberSetting("Durability %", "Replace when durability below this %", 15.f, 1.f, 50.f, 1.f);
    BoolSetting   mNotifyLow        = BoolSetting("Notify Low", "Notify when no replacement found", true);

    AutoArmor() : ModuleBase("AutoArmor", "Automatically equips best armor",
        ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mDelay, &mInstant, &mDropWorse, &mPreferFireProt,
            &mDurabilityCheck, &mDurabilityThresh, &mNotifyLow
        );

        VISIBILITY_CONDITION(mDelay,            !mInstant.mValue);
        VISIBILITY_CONDITION(mDurabilityThresh,  mDurabilityCheck.mValue);
        VISIBILITY_CONDITION(mNotifyLow,         mDurabilityCheck.mValue);

        mNames = {
            {Lowercase,       "autoarmor"},
            {LowercaseSpaced, "auto armor"},
            {Normal,          "AutoArmor"},
            {NormalSpaced,    "Auto Armor"}
        };
    }

    // State
    uint64_t mLastAction = 0;

    // Трекаем уже отправленные нотификации чтобы не спамить
    // ключ = armor slot (0-3), значение = время последнего уведомления
    uint64_t mLastNotify[4] = {0, 0, 0, 0};

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);

    // Returns inventory slot (0-35) of best armor for given armor slot, or -1
    // ignoreDurability = true когда ищем замену для сломанного
    int findBestArmor(int armorSlot, bool ignoreDurability = false);

    // Returns true если броня в слоте ниже порога прочности
    bool isArmorLow(int armorSlot);

    std::string getSettingDisplay() override {
        return mInstant.mValue ? "Instant" : std::to_string((int)mDelay.mValue) + "ms";
    }
};