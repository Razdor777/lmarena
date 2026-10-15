#pragma once
//
// Reach — combat (attack) reach detour.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class Reach : public ModuleBase<Reach> {
public:
    NumberSetting mCombatReach = NumberSetting("Combat Reach Range", "The attack range.", 3.0f, 0.0f, 100000.0f, 0.01f);
    NumberSetting mBlockReach = NumberSetting("Block Reach Range", "The placing/destroying range.", 5.7f, 0.0f, 100000.0f, 0.01f);

    // The patched instruction loads the game's own reach and takes the smaller
    // of the two values (`minss`), which is why anything above ~7 blocks used to
    // be swallowed by the game's clamp. With this off nothing changes; with it
    // on the same instruction copies our value instead (`movss`), so the client
    // uses the configured range as-is. Server-side range checks still apply.
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

    static inline unsigned char mOriginalData[11];

    // [our float][copied rip-relative instruction][jmp back]
    //
    //   F3 44 0F 5D 3D F3 FF FF FF  =  minss xmm15, [rip-13]   <- reads our float
    //   F3 44 0F 10 3D F3 FF FF FF  =  movss xmm15, [rip-13]   <- Full Override
    //
    // Only the opcode byte differs, so switching modes is a one byte patch
    // (see kOpcodeOffset in the .cpp).
    static inline unsigned char mDetourBytes[] = {
        0x00, 0x00, 0x40, 0x40,
        0xF3, 0x44, 0x0F, 0x5D, 0x3D, 0xF3, 0xFF, 0xFF, 0xFF
    };

    static inline void* mDetour = nullptr;
    float* mBlockReachPtr = nullptr;

    void onEnable() override;
    void onDisable() override;
    void onInit() override;
    void applyOverrideMode();
    void onBaseTickEvent(class BaseTickEvent& event);

private:
    // -1 = nothing applied to the current detour buffer yet.
    int mAppliedOpcode = -1;
};
