#include "Reach.hpp"
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MemUtils.hpp>

#include <algorithm>

static uintptr_t func = 0x0;

void Reach::onInit() {
    func = SigManager::Reach;

    // Same guard as the combat signature: reading the rel32 out of a failed
    // signature would dereference address 4 and crash at startup.
    const uintptr_t addr2 = SigManager::BlockReach;
    if (addr2 != 0) {
        const uintptr_t ripAddr = addr2 + 4;
        const int offset2 = *reinterpret_cast<const int*>(ripAddr);
        mBlockReachPtr = reinterpret_cast<float*>(ripAddr + offset2 + 4);
    } else {
        mBlockReachPtr = nullptr;
        spdlog::error("Reach: block reach signature not found, block reach stays vanilla");
    }
}

void Reach::onEnable() {
    if (func == 0) {
        // Signature did not resolve (game build mismatch) — enabling the module
        // would write to address 0, so stay off and say why.
        spdlog::error("Reach: engine signature not found on this build, module stays disabled");
        ChatUtils::displayClientMessage("§cReach: engine signature not found on this game build.");
        mEnabled = false;
        mWantedState = false;
        return;
    }

    MemUtils::ReadBytes((void*)func, mOriginalData, sizeof(mOriginalData));

    mDetour = AllocateBuffer((void*)func);
    MemUtils::writeBytes((uintptr_t)mDetour, mDetourBytes, sizeof(mDetourBytes));

    auto toOriginalAddrRip1 = MemUtils::GetRelativeAddress((uintptr_t)mDetour + sizeof(mDetourBytes) + 1, func + 11);

    MemUtils::writeBytes((uintptr_t)mDetour + sizeof(mDetourBytes), "\xE9", 1);
    MemUtils::writeBytes((uintptr_t)mDetour + sizeof(mDetourBytes) + 1, &toOriginalAddrRip1, sizeof(int32_t));

    auto newRelRip1 = MemUtils::GetRelativeAddress(func + 1, (uintptr_t)mDetour + 4);

    MemUtils::writeBytes(func, "\xE9", 1);
    MemUtils::writeBytes(func + 1, &newRelRip1, sizeof(int32_t));
    MemUtils::NopBytes(func + 5, 6);

    // Fresh trampoline: the opcode byte has to be applied again.
    mAppliedOpcode = 0;
    applyOpcode(mForce.mValue);

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Reach::onBaseTickEvent>(this);
}

void Reach::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Reach::onBaseTickEvent>(this);

    if (func != 0) {
        MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));
    }
    if (mDetour) {
        FreeBuffer(mDetour);
        mDetour = nullptr;
    }

    if (mBlockReachPtr) {
        MemUtils::Write((uintptr_t) mBlockReachPtr, 5.7f);
    }
}

// Switches the trampoline between "cap the engine value" and "write our value".
// Both encodings are the same length, so nothing else has to move.
void Reach::applyOpcode(bool force) {
    if (!mDetour) return;

    const unsigned char op = force ? kOpcodeMov : kOpcodeMin;
    if (mAppliedOpcode == op) return;

    MemUtils::writeBytes((uintptr_t)mDetour + kOpcodeOffset, &op, 1);
    mAppliedOpcode = op;
    mDetourBytes[kOpcodeOffset] = op;
}

void Reach::onBaseTickEvent(class BaseTickEvent &event) {
    if (!mDetour) return;

    // A config written by an older build could hold anything (the old slider
    // went up to 100000). Clamp once, write the same value everywhere.
    const float combat = std::clamp(mCombatReach.mValue, mCombatReach.mMin, mCombatReach.mMax);
    const float block  = std::clamp(mBlockReach.mValue, mBlockReach.mMin, mBlockReach.mMax);

    applyOpcode(mForce.mValue);

    MemUtils::Write((uintptr_t)mDetour, combat);
    if (mBlockReachPtr) MemUtils::Write((uintptr_t)mBlockReachPtr, block);
}
