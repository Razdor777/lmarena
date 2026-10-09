//
// Dexko Reach — combat-only.
//
// Как это работает (как в движке): на адрес SigManager::Reach ставится jmp на
// выделенный буфер-трамплин. Первые 4 байта трамплина — float с текущим
// радиусом атаки (в оригинале там захардкожено 3.0f = 0x40400000), дальше
// minss + обратный jmp в оригинал. onBaseTickEvent просто пишет нужное
// значение в первые 4 байта трамплина.
//

#include "Reach.hpp"
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Utils/MemUtils.hpp>

static uintptr_t func = 0x0;

void Reach::onInit() {
    func = SigManager::Reach;
}

void Reach::onEnable() {
    if (func == 0x0) {
        spdlog::error("[Reach] signature not found, cannot enable");
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

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Reach::onBaseTickEvent>(this);
}

void Reach::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Reach::onBaseTickEvent>(this);

    if (func == 0x0) return;

    MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));

    if (mDetour) {
        FreeBuffer(mDetour);
        mDetour = nullptr;
    }
}

void Reach::onBaseTickEvent(class BaseTickEvent &event) {
    MemUtils::Write((uintptr_t) mDetour, mCombatReach.mValue);
}
