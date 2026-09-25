#include "ProjectileHooks.hpp"
#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Windows.h>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ПРОВЕРИТЬ: projectileHitEntity -> src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:344 — в 1.26 есть scripting-событие onProjectileHitEntity + ProjectileHitEvent | найдено в другом классе — проверь, тот ли это метод
// [1.26] ПРОВЕРИТЬ: projectileHitBlock -> src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:341 — в 1.26 есть scripting-событие onProjectileHitBlock + ProjectileHitEvent | найдено в другом классе — проверь, тот ли это метод

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
