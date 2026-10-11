#pragma once
//
// Combat reach.
//
// The engine computes the attack range and then clamps it with a single SSE
// instruction (`minss xmm15, [3.0f]`). One byte pair of that instruction is
// patched so the constant lives in our own buffer and can be rewritten every
// tick. See Reach.cpp for the details and for the "Force Range" option.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class Reach : public ModuleBase<Reach> {
public:
    // 7.0 was the old ceiling of the slider. The clamp above it did nothing
    // because the value the engine feeds into that instruction is itself never
    // larger than ~7 — which is exactly the "reach stops working above 7"
    // report. "Force Range" below replaces the clamp with a hard overwrite so
    // the value you type is the value the engine sees.
    NumberSetting mCombatReach = NumberSetting("Combat Reach Range", "The attack range.", 3.0f, 1.0f, 20.0f, 0.01f);
    NumberSetting mBlockReach = NumberSetting("Block Reach Range", "The placing/destroying range.", 5.7f, 5.7f, 12.0f, 0.01f);

    // Off by default: while off the module behaves exactly like before (the
    // engine's own range value gets capped at the number you set, so values
    // above ~7 have no effect). While on, the value is written over the
    // engine's instead of clamping it. If hits stop registering, turn it off.
    BoolSetting mForce = BoolSetting("Force Range",
                                     "Experimental: overwrite the engine's own range value instead of clamping it. This is what makes values above 7 work, but it can also make hits stop registering.",
                                     false);

    Reach() : ModuleBase("Reach", "Changes attack range.", ModuleCategory::Combat, 0, false) {
        addSettings(&mCombatReach, &mBlockReach, &mForce);

        mNames = {
                {Lowercase, "reach"},
                {LowercaseSpaced, "reach"},
                {Normal, "Reach"},
                {NormalSpaced, "Reach"}
        };
    }

    static inline unsigned char mOriginalData[11];

    // [0..3]  float constant the instruction below reads (rewritten every tick)
    // [4..12] copy of the engine instruction: minss xmm15, [rip-13]
    //         0xF3 0x44 0x0F 0x5D 0x3D (disp32 = -13 -> back to byte 0)
    static inline unsigned char mDetourBytes[] = {
        0x00, 0x00, 0x40, 0x40,
        0xF3, 0x44, 0x0F, 0x5D, 0x3D, 0xF3, 0xFF, 0xFF, 0xFF
    };

    static inline void* mDetour = nullptr;
    float* mBlockReachPtr = nullptr;

    void onEnable() override;
    void onDisable() override;
    void onInit() override;
    void onBaseTickEvent(class BaseTickEvent& event);

private:
    // Byte 7 of mDetourBytes is the SSE opcode: 0x5D = MINSS (clamp, vanilla
    // behaviour), 0x10 = MOVSS (load our value into the register).
    static constexpr size_t kOpcodeOffset = 7;
    static constexpr unsigned char kOpcodeMin = 0x5D;
    static constexpr unsigned char kOpcodeMov = 0x10;

    unsigned char mAppliedOpcode = 0;

    void applyOpcode(bool force);
};
