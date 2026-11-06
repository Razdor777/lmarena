#include "HealthHooks.hpp"
#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> HealthHooks::mHurtDetour;

bool HealthHooks::onEntityHurt(Actor* _this, ActorDamageSource* src, float damage, bool knock, bool ignite)
{
    auto original = mHurtDetour->getOriginal<&HealthHooks::onEntityHurt>();
    bool result = original(_this, src, damage, knock, ignite);

    if (result && gFeatureManager && gFeatureManager->mDispatcher)
    {
        auto holder = nes::make_holder<EntityHurtEvent>(_this, src, static_cast<int>(damage), knock, ignite);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return result;
}

void HealthHooks::init()
{
    uintptr_t hurtAddr = SigManager::Health_entityHurt;
    if (!hurtAddr)
    {
        // BaseTickHook natively dispatches EntityHurtEvent via MobHurtTimeComponent on 26.62.
        return;
    }

    mHurtDetour = std::make_unique<Detour>("Mob::_hurt", reinterpret_cast<void*>(hurtAddr), &HealthHooks::onEntityHurt);
    mHurtDetour->enable();
}
