#pragma once
//
// ModuleToggle.hpp — где вызываются onEnable()/onDisable() модулей.
//
// Раньше их звал ModuleManager::onClientTick() из фонового потока Solstice
// (Solstice::shutdownThread, раз в 1 мс). А модули в onEnable/onDisable
// трогают игру: ставят флаги игроку, скорость, таймер, перебирают сущности
// (HitBoxes, GhostMode, NoClip, Fly, Freecam...). Делать это из чужого потока,
// пока поток игры сам меняет те же данные, — гонка: изредка и «просто так»
// игра падала при включении/выключении модулей (клавишей, в ClickGui, при
// загрузке конфига).
//
// Теперь: пока игра тикает — только из Actor::baseTick (поток игры, до
// событий тика). В меню/на загрузке, когда тиков нет, — как раньше, из
// фонового потока (мира нет — гоняться не с чем). Мьютекс не даёт обоим
// потокам обработать один и тот же модуль одновременно.
//

#include <Features/FeatureManager.hpp>
#include <Solstice.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>

struct ModuleToggle
{
    // Из BaseTickHook::onBaseTick (поток игры), до BaseTickEvent: ни один
    // trigger() в этот момент не выполняется, listen/deafen безопасны.
    static void fromGameThread()
    {
        sLastGameTick.store(GetTickCount64());
        process();
    }

    // Из Solstice::shutdownThread: только если игра не тикает больше секунды
    // (меню, загрузка мира, пауза в одиночной игре).
    static void fromBackgroundThread()
    {
        if (GetTickCount64() - sLastGameTick.load() < 1000) return;
        process();
    }

private:
    static void process()
    {
        // Выгрузка (END): модули сейчас выключает и удаляет сам Solstice.
        if (Solstice::mUnloading.load() || !gFeatureManager || !gFeatureManager->mModuleManager) return;
        std::lock_guard<std::mutex> lock(sMutex);
        gFeatureManager->mModuleManager->onClientTick();
    }

    static inline std::mutex sMutex;
    static inline std::atomic<uint64_t> sLastGameTick{0};
};
