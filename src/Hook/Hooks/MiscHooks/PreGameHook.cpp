//
// Created by vastrakai on 11/5/2024.
//

#include "PreGameHook.hpp"
#include <Features/Events/PreGameCheckEvent.hpp>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ОБНОВЛЕНО: ClientInstance::isPreGame -> src-client/mc/client/game/ClientInstance.h:1755 — готовый символ + транк $isPreGame

std::unique_ptr<Detour> PreGameHook::mDetour = nullptr;

bool PreGameHook::onPreGame(void* _this)
{
    auto original = mDetour->getOriginal<&onPreGame>();

    auto holder = nes::make_holder<PreGameCheckEvent>(false);
    gFeatureManager->mDispatcher->trigger(holder);

    if (holder->mOverride) return holder->mIsPreGame;
    return original(_this);
}

void PreGameHook::init()
{
    mDetour = std::make_unique<Detour>("ClientInstance::isPreGame", reinterpret_cast<void*>(SigManager::ClientInstance_isPreGame), reinterpret_cast<void*>(&onPreGame));
}
