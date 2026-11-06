//
// Created by alteik on 04/09/2024.
//

#include "Reach.hpp"
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Utils/MemUtils.hpp>

static uintptr_t func = 0x0;

void Reach::onInit() {
    func = SigManager::Reach;

    if (SigManager::BlockReach) {
        uintptr_t addr2 = SigManager::BlockReach + 4;
        int offset2 = *reinterpret_cast<int*>(addr2);
        mBlockReachPtr = reinterpret_cast<float*>(addr2 + offset2 + 4);
        if (mBlockReachPtr) {
            mOrigBlockReach = *mBlockReachPtr;
        }
    }
}

void Reach::onEnable() {
    if (!func) return;

    auto* buf = AllocateBuffer((void*)func);
    if (!buf) {
        return;
    }
    uintptr_t adr = (uintptr_t)buf;
    if (adr % alignof(float) != 0) {
        FreeBuffer(buf);
        return;
    }
    mCombatPtr = (float*)buf;
    *mCombatPtr = mCombatReach.mValue;

    MemUtils::ReadBytes((void*)func, mOriginalData, sizeof(mOriginalData));
    uintptr_t mov = func + 10;

    auto cr = (int32_t)((uintptr_t)mCombatPtr - (func + 8));
    auto mr = (int32_t)((uintptr_t)mCombatPtr - (mov + 8));
    MemUtils::writeBytes((uintptr_t)func + 4, &cr, sizeof(int32_t));
    MemUtils::writeBytes((uintptr_t)mov + 4, &mr, sizeof(int32_t));

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Reach::onBaseTickEvent>(this);
}

void Reach::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Reach::onBaseTickEvent>(this);

    if (func && mCombatPtr) {
        MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));
        FreeBuffer(mCombatPtr);
        mCombatPtr = nullptr;
    }

    if (mBlockReachPtr) {
        MemUtils::Write((uintptr_t)mBlockReachPtr, mOrigBlockReach);
    }
}

void Reach::onBaseTickEvent(class BaseTickEvent& event) {
    if (mCombatPtr) {
        *mCombatPtr = mCombatReach.mValue;
    }
    if (mBlockReachPtr) {
        MemUtils::Write((uintptr_t)mBlockReachPtr, mBlockReach.mValue);
    }
}
