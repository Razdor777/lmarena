#include "HealthHooks.hpp"
#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Windows.h>


std::unique_ptr<Detour> HealthHooks::mDetourEntityHurt;
std::unique_ptr<Detour> HealthHooks::mDetourEntityHealthChanged;

bool HealthHooks::onEntityHurt(Actor *actor, ActorDamageSource *source,
                               int damage, bool knockback, bool ignites) {
  auto original = mDetourEntityHurt->getOriginal<&HealthHooks::onEntityHurt>();

  if (actor) {
    auto holder = nes::make_holder<EntityHurtEvent>(actor, source, damage,
                                                    knockback, ignites);
    gFeatureManager->mDispatcher->trigger(holder);
  }

  return original(actor, source, damage, knockback, ignites);
}

void HealthHooks::onEntityHealthChanged(Actor *actor, int oldHealth,
                                        int newHealth,
                                        const AttributeModifier &modifier) {
  // We can guess signature for now, likely just void
  // Or maybe just void(Actor*)?
  // User provided no sig.
  // If we detour with wrong sig, we crash.
  // Safest is to skip healthChanged if we have hurt, unless required.
  // DamageLog can use hurt.

  auto original = mDetourEntityHealthChanged
                      ->getOriginal<&HealthHooks::onEntityHealthChanged>();
  original(actor, oldHealth, newHealth, modifier);
}

void HealthHooks::init() {
  uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

  // entityHurt sub_1423A09B0
  // Offset: 0x23A09B0
  uintptr_t hurtAddr = base + 0x23A09B0;

  // entityHealthChanged sub_1423A0B80
  // Offset: 0x23A0B80
  uintptr_t healthChangedAddr = base + 0x23A0B80;

  mDetourEntityHurt = std::make_unique<Detour>("entityHurt", (void *)hurtAddr,
                                               &HealthHooks::onEntityHurt);

  // Temporarily disabled healthChanged hook until signature is confirmed to
  // avoid crashes mDetourEntityHealthChanged =
  // std::make_unique<Detour>("entityHealthChanged", (void*)healthChangedAddr,
  // &HealthHooks::onEntityHealthChanged);
}
