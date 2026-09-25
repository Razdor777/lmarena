#include "ProjectileHooks.hpp"
#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Windows.h>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: projectileHitEntity -> onProjectileHitEntity  (IScriptWorldAfterEvents.h:344) + ProjectileHitEvent — см. projectileHitBlock
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: projectileHitBlock -> onProjectileHitBlock  (IScriptWorldAfterEvents.h:341) + ProjectileHitEvent — scripting-событие; нативный вариант — хук ProjectileComponent / Actor::_onHit

std::unique_ptr<Detour> ProjectileHooks::mDetourProjectileHitEntity;
std::unique_ptr<Detour> ProjectileHooks::mDetourProjectileHitBlock;

void ProjectileHooks::onProjectileHitEntity(Actor *projectile,
                                            const HitResult &hitResult) {
  auto original = mDetourProjectileHitEntity
                      ->getOriginal<&ProjectileHooks::onProjectileHitEntity>();

  // Trigger event
  if (projectile) {
    auto holder = nes::make_holder<ProjectileHitEvent>(projectile, hitResult);
    gFeatureManager->mDispatcher->trigger(holder);
  }

  original(projectile, hitResult);
}

void ProjectileHooks::onProjectileHitBlock(Actor *projectile,
                                           const HitResult &hitResult) {
  auto original = mDetourProjectileHitBlock
                      ->getOriginal<&ProjectileHooks::onProjectileHitBlock>();

  if (projectile) {
    auto holder = nes::make_holder<ProjectileHitEvent>(projectile, hitResult);
    gFeatureManager->mDispatcher->trigger(holder);
  }

  original(projectile, hitResult);
}

void ProjectileHooks::init() {
  uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

  // projectileHitEntity sub_1423A0270
  // Offset: 0x23A0270
  uintptr_t hitEntityAddr = base + 0x23A0270;

  // projectileHitBlock sub_1423A0440
  // Offset: 0x23A0440
  uintptr_t hitBlockAddr = base + 0x23A0440;

  mDetourProjectileHitEntity =
      std::make_unique<Detour>("projectileHitEntity", (void *)hitEntityAddr,
                               &ProjectileHooks::onProjectileHitEntity);
  mDetourProjectileHitBlock =
      std::make_unique<Detour>("projectileHitBlock", (void *)hitBlockAddr,
                               &ProjectileHooks::onProjectileHitBlock);
}
