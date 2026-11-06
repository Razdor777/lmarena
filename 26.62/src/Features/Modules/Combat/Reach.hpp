#pragma once
//
// Reach — combat (attack) reach detour.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class Reach : public ModuleBase<Reach> {
public:
    NumberSetting mCombatReach = NumberSetting("Combat Reach Range", "The attack range.", 3.0f, 3.f, 20.0f, 0.01f);
    NumberSetting mBlockReach = NumberSetting("Block Reach Range", "The placing/destroying range.", 5.7f, 5.7f, 12.0f, 0.01f);

    // The vanilla code loads its own reach constant and takes the smaller of the
    // two values (`minss`), so anything above ~7 blocks was swallowed by the
    // game's clamp. The rip-relative constants are repointed at our buffer in
    // onEnable(), so that clamp no longer applies to the client; this switch is
    // kept for parity and only documents the behaviour. Server-side range checks
    // still apply.
    BoolSetting mFullOverride = BoolSetting(
        "Full Override",
        "Bypass the game's own reach clamp (needed for values above ~7).",
        false);

    Reach() : ModuleBase("Reach", "Changes attack range.", ModuleCategory::Combat, 0, false) {
        addSettings(&mCombatReach, &mBlockReach, &mFullOverride);

        mNames = {
                {Lowercase, "reach"},
                {LowercaseSpaced, "reach"},
                {Normal, "Reach"},
                {NormalSpaced, "Reach"}
        };
    }

    static inline unsigned char mOriginalData[18];

    float* mBlockReachPtr = nullptr;
    float* mCombatPtr = nullptr;
    void onEnable() override;
    void onDisable() override;
    void onInit() override;
    void onBaseTickEvent(class BaseTickEvent& event);
};
