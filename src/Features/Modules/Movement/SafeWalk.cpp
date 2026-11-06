//
// Created by vastrakai on 7/18/2024.
//

#include "SafeWalk.hpp"

#include <Features/Events/BaseTickEvent.hpp>

static unsigned char gSafeWalkBytes1[] = {
    0xC6, 0x40, 0x55, 0x01,
    0xE9, 0x6F, 0, 0, 0,
    0x90, 0x90, 0x90, 0x90
};

static unsigned char gSafeWalkBytes2[] = {
    0xC6, 0x40, 0x55, 0x01,
    0xE9, 0, 0, 0, 0,
    0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90
};

static void patchSafeWalk(bool v) {
    static bool current = false;
    uintptr_t adr = SigManager::SneakMovementSystem_tickSneakMovementSystem;
    uintptr_t adr2 = SigManager::SneakMovementSystem_tickSneakMovementSystem2;
    if (!adr || !adr2) return;
    if (current == v) {
        return;
    }
    if (v) {
        MemUtils::ReadBytes((void*)adr, SafeWalk::mOriginalData, sizeof(SafeWalk::mOriginalData));
        MemUtils::ReadBytes((void*)adr2, SafeWalk::mOriginalData2, sizeof(SafeWalk::mOriginalData2));

        *(int32_t*)(gSafeWalkBytes1 + 5) = (int32_t)((adr + 0x78) - (adr + 9));
        MemUtils::writeBytes(adr, gSafeWalkBytes1, sizeof(gSafeWalkBytes1));

        *(int32_t*)(gSafeWalkBytes2 + 5) = (int32_t)((adr2 + 0x19) - (adr2 + 9));
        MemUtils::writeBytes(adr2, gSafeWalkBytes2, sizeof(gSafeWalkBytes2));
        current = true;
    }
    else {
        MemUtils::writeBytes(adr, SafeWalk::mOriginalData, sizeof(SafeWalk::mOriginalData));
        MemUtils::writeBytes(adr2, SafeWalk::mOriginalData2, sizeof(SafeWalk::mOriginalData2));
        current = false;
    }
}

void SafeWalk::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &SafeWalk::onBaseTickEvent>(this);
}

void SafeWalk::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SafeWalk::onBaseTickEvent>(this);
    patchSafeWalk(false);
}

void SafeWalk::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (player && player->isOnGround()) {
        patchSafeWalk(mEnabled);
    }
    else {
        patchSafeWalk(false);
    }
}
