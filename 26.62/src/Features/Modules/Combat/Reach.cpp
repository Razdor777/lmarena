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
// onEnable() repoints *both* rip-relative reach constants at one float buffer,
// so the client reads exactly the configured value — the vanilla clamp only ever
// sees our number. mFullOverride is kept for parity with the 1.21.44 settings
// layout and no longer needs a separate code path.
//

#include "Reach.hpp"
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Utils/MemUtils.hpp>

static uintptr_t func = 0x0;

void Reach::onInit() {
    func = SigManager::Reach;

    uintptr_t addr2 = SigManager::BlockReach;
    addr2 += 4;
    int offset2 = *(int*)addr2;
    mBlockReachPtr = (float*)(addr2 + offset2 + 4);
}

void Reach::onEnable() {
    auto* buf = AllocateBuffer((void*)func);
    if (!buf) {
        return;
    }
    uintptr_t adr = (uintptr_t)buf;
    if (adr % alignof(float) != 0) {
        return;
    }
    mCombatPtr = (float*)buf;
    if (!mCombatPtr) {
        return;
    }
    *mCombatPtr = 3.f;

    MemUtils::ReadBytes((void*)func, mOriginalData, sizeof(mOriginalData));
    static auto mov = func + 10;

    auto cr = (int32_t)((uintptr_t)mCombatPtr - (func + 8));
    auto mr = (int32_t)((uintptr_t)mCombatPtr - (mov + 8));
    MemUtils::writeBytes((uintptr_t)func + 4, &cr, sizeof(int32_t));
    MemUtils::writeBytes((uintptr_t)mov + 4, &mr, sizeof(int32_t)); // second constant

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Reach::onBaseTickEvent>(this);
}

void Reach::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Reach::onBaseTickEvent>(this);

    MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));
    FreeBuffer(mCombatPtr);
    mCombatPtr = nullptr;

    //MemUtils::Write((uintptr_t) mBlockReachPtr, 5.7f);
}

void Reach::onBaseTickEvent(class BaseTickEvent &event) {
    // Both reach constants were repointed at mCombatPtr in onEnable, so updating
    // the buffer is what actually changes the reach.
    if (mCombatPtr) {
        *mCombatPtr = mCombatReach.mValue;
    }
    MemUtils::Write((uintptr_t) mBlockReachPtr, mBlockReach.mValue);
}
