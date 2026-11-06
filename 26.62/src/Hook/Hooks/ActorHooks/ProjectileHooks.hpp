#pragma once
#include <Hook/Hook.hpp>
//
// Хуки попаданий снарядов: ProjectileHitEvent. На нём висят BowHitMarker и
// ArrowRecovery.
//

class ProjectileHooks : public Hook {
public:
    ProjectileHooks() : Hook() {
        mName = "Projectile::onHit";
    }

    static std::unique_ptr<Detour> mDetourProjectileHit;

    // По прологу: RCX = Actor* (снаряд), RDX = const HitResult&.
    // Разделения «в сущность / в блок» на уровне детура больше нет — тип
    // попадания лежит в HitResult::mType, и модули смотрят именно туда.
    static void onProjectileHit(class Actor* projectile,
                                const class HitResult& hitResult);

    void init() override;
};
