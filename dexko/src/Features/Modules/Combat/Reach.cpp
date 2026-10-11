//
// Reach — combat (attack) reach detour.
//
// The game computes its own reach and then clamps it with
//     minss xmm15, [reach constant]
// The constant is what the old patch overwrote with the user's value, so the
// result was always min(game value, user value) — a hard ceiling around 7 that
// made `.reach 10` behave exactly like `.reach 7`. `.reach 1` looked fine
// simply because 1 was below that ceiling.
//
// Full Override rewrites the copied instruction's opcode byte from MINSS (0x5D)
// to MOVSS (0x10): the very same instruction then copies our value into xmm15
// instead of comparing it, so the configured range is used verbatim. It stays
// opt-in because it changes the semantics of the function we are patching.
//

#include "Reach.hpp"
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Utils/MemUtils.hpp>

static uintptr_t func = 0x0;

namespace
{
    // `mDetourBytes` = [float 0..3][F3 44 0F 5D 3D disp32][E9 rel32]
    //                                            ^ opcode lives at offset 7
    constexpr uintptr_t kOpcodeOffset = 7;
    constexpr unsigned char kMinss = 0x5D; // 0F 5D -> minss xmm, m32
    constexpr unsigned char kMovss = 0x10; // 0F 10 -> movss xmm, m32
}

void Reach::onInit() {
    func = SigManager::Reach;

    uintptr_t addr2 = SigManager::BlockReach;
    addr2 += 4;
    int offset2 = *reinterpret_cast<int*>(addr2);
    mBlockReachPtr = reinterpret_cast<float*>(addr2 + offset2 + 4);
}

void Reach::onEnable() {
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

    // Fresh buffer -> whatever mode was applied to the previous one is gone.
    mAppliedOpcode = -1;

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Reach::onBaseTickEvent>(this);
}

void Reach::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Reach::onBaseTickEvent>(this);

    MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));
    FreeBuffer(mDetour);
    mDetour = nullptr;
    mAppliedOpcode = -1;

    MemUtils::Write((uintptr_t) mBlockReachPtr, 5.7f);
}

// Only the opcode byte of the copied instruction is rewritten, and only when the
// selected mode actually changed — this runs every tick, so the hot path is a
// single integer compare.
void Reach::applyOverrideMode()
{
    if (!mDetour) return;

    const unsigned char wanted = mFullOverride.mValue ? kMovss : kMinss;
    if (mAppliedOpcode == static_cast<int>(wanted)) return;

    MemUtils::Write((uintptr_t)mDetour + kOpcodeOffset, wanted);
    mAppliedOpcode = static_cast<int>(wanted);
}

void Reach::onBaseTickEvent(class BaseTickEvent &event) {
    applyOverrideMode();

    MemUtils::Write((uintptr_t) mDetour, mCombatReach.mValue);
    MemUtils::Write((uintptr_t) mBlockReachPtr, mBlockReach.mValue);
}
