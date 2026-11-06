//
// ProjectileHooks — источник ProjectileHitEvent в 26.62.
//
// Раньше стояли два адреса (0x23A0270 «в сущность» и 0x23A0440 «в блок»).
// Теперь одна сигнатура SigManager::Projectile_onHit; тип попадания модули
// читают из HitResult::mType (HitType::BLOCK / ENTITY), поэтому один детур
// закрывает оба случая.
//

#include "ProjectileHooks.hpp"

#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> ProjectileHooks::mDetourProjectileHit;

void ProjectileHooks::onProjectileHit(Actor* projectile,
                                      const HitResult& hitResult) {
    auto original =
        mDetourProjectileHit->getOriginal<&ProjectileHooks::onProjectileHit>();

    if (projectile) {
        auto holder = nes::make_holder<ProjectileHitEvent>(projectile, hitResult);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    original(projectile, hitResult);
}

void ProjectileHooks::init() {
    if (!SigManager::Projectile_onHit) {
        spdlog::warn("[BowHitMarker] Projectile::onHit signature not resolved - ProjectileHitEvent disabled");
        return;
    }

    mDetourProjectileHit = std::make_unique<Detour>(
        "Projectile::onHit", reinterpret_cast<void*>(SigManager::Projectile_onHit),
        &ProjectileHooks::onProjectileHit);
    mDetourProjectileHit->enable();
}
