#pragma once

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>

struct ProjectileHitEvent {
  class Actor *mProjectile;
  const class HitResult &mResult;
  // Type of hit (block or entity) is in HitResult::mType
};
