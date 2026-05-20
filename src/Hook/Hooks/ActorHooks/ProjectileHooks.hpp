#pragma once
#include <Hook/Hook.hpp>

class ProjectileHooks : public Hook {
public:
  static std::unique_ptr<Detour> mDetourProjectileHitEntity;
  static std::unique_ptr<Detour> mDetourProjectileHitBlock;

  // Assumptions:
  // RCX: Actor* (Projectile)
  // RDX: HitResult& (or similar)
  static void onProjectileHitEntity(class Actor *projectile,
                                    const class HitResult &hitResult);
  static void onProjectileHitBlock(class Actor *projectile,
                                   const class HitResult &hitResult);

  void init() override;
};
