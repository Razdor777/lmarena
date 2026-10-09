#pragma once
//
// Dexko Reach — combat-only форк движкового Reach.
// Отличия: убран block reach (остался только attack range), управление —
// через команду .reach.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class Reach : public ModuleBase<Reach> {
public:
    NumberSetting mCombatReach = NumberSetting("Combat Reach Range", "The attack range.", 3.0f, 3.f, 7.0f, 0.01f);

    Reach() : ModuleBase("Reach", "Changes attack range.", ModuleCategory::Combat, 0, false) {
        addSettings(&mCombatReach);

        mNames = {
                {Lowercase, "reach"},
                {LowercaseSpaced, "reach"},
                {Normal, "Reach"},
                {NormalSpaced, "Reach"}
        };
    }

    static inline unsigned char mOriginalData[11];

    static inline unsigned char mDetourBytes[] = {
        0x00, 0x00, 0x40, 0x40,
        0xF3, 0x44, 0x0F, 0x5D, 0x3D, 0xF3, 0xFF, 0xFF, 0xFF
    };

    static inline void* mDetour = nullptr;

    void onEnable() override;
    void onDisable() override;
    void onInit() override;
    void onBaseTickEvent(class BaseTickEvent& event);
};
