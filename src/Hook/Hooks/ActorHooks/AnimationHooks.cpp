//
// Created by vastrakai on 7/18/2024.
//

#include "AnimationHooks.hpp"

#include <Features/Events/SwingDurationEvent.hpp>
#include <Features/Events/BobHurtEvent.hpp>


#include <SDK/Minecraft/ClientInstance.hpp>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: Mob::getCurrentSwingDuration -> Mob::getModifiedSwingDuration  (src/mc/world/actor/Mob.h:338) — в 1.26 это int getModifiedSwingDuration(); Item::getSwingDuration (Item.h:231) — длительность взмаха предмета, НЕ моба: для хука бери Mob::getModifiedSwingDuration
// [1.26] ПЕРЕЕХАЛО В 1.26: bobHurt -> LevelRendererPlayer::bobHurt(Matrix&, float)  (src-client/mc/client/renderer/game/LevelRendererPlayer.h:365) — метод переехал из анонимного класса в LevelRendererPlayer

std::unique_ptr<Detour> AnimationHooks::mSwingDetour;
std::unique_ptr<Detour> AnimationHooks::mBobHurtDetour;

int AnimationHooks::getCurrentSwingDuration(Actor* actor)
{
    auto original = mSwingDetour->getOriginal<&getCurrentSwingDuration>();
    int result = original(actor);

    auto holder = nes::make_holder<SwingDurationEvent>(result);
    gFeatureManager->mDispatcher->trigger(holder);
    result = holder->mSwingDuration;

    return result;
}

void* AnimationHooks::doBobHurt(void* _this, glm::mat4* matrix)
{
    auto original = mBobHurtDetour->getOriginal<&doBobHurt>();
    // Log the address of the matrix
    auto result = original(_this, matrix);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return result;


    auto holder = nes::make_holder<BobHurtEvent>(_this, matrix);
    gFeatureManager->mDispatcher->trigger(holder);

    return result;
}

void AnimationHooks::init()
{
    uintptr_t func = SigManager::Mob_getCurrentSwingDuration;
    mSwingDetour = std::make_unique<Detour>("Mob::getCurrentSwingDuration", reinterpret_cast<void*>(func), &getCurrentSwingDuration);

    // TODO: Replace this with a better method of doing matrix translation
    func = SigManager::BobHurt;
    mBobHurtDetour = std::make_unique<Detour>("bobHurt", reinterpret_cast<void*>(func), &doBobHurt);
}
