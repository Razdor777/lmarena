#include "ArrowRecovery.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

void ArrowRecovery::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<ProjectileHitEvent, &ArrowRecovery::onProjectileHit>(this);
  gFeatureManager->mDispatcher->listen<RenderEvent, &ArrowRecovery::onRender>(
      this);
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &ArrowRecovery::onBaseTick>(this);
  mArrows.clear();
}

void ArrowRecovery::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<ProjectileHitEvent, &ArrowRecovery::onProjectileHit>(this);
  gFeatureManager->mDispatcher->deafen<RenderEvent, &ArrowRecovery::onRender>(
      this);
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &ArrowRecovery::onBaseTick>(this);
  mArrows.clear();
}

void ArrowRecovery::onProjectileHit(ProjectileHitEvent &event) {
  if (event.mResult.mType != HitType::BLOCK)
    return;
  if (!event.mProjectile)
    return;

  // Check if it's an arrow
  std::string name = event.mProjectile->getRawName();
  if (name.find("arrow") == std::string::npos &&
      name.find("trident") == std::string::npos)
    return;

  // Add to list
  mArrows.push_back({event.mProjectile->getRuntimeID(),
                     *event.mProjectile->getPos(), mTimeout.mValue});
}

void ArrowRecovery::onBaseTick(BaseTickEvent &event) {
  if (mArrows.empty())
    return;

  // Remove despawned arrows (picked up or dead)
  auto list = ActorUtils::getActorList(false, true);

  // Create a set of current entity IDs for O(1) lookup
  std::set<int64_t> currentIds;
  for (auto actor : list) {
    currentIds.insert(actor->getRuntimeID());
  }

  auto it = mArrows.begin();
  while (it != mArrows.end()) {
    // If arrow not in the world list, it's gone (picked up)
    if (currentIds.find(it->mEntityId) == currentIds.end()) {
      it = mArrows.erase(it);
    } else {
      // Also check timeout
      it->mTime -= 1.0f / 20.0f; // Approx
      if (it->mTime <= 0) {
        it = mArrows.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void ArrowRecovery::onRender(RenderEvent &event) {
  if (mArrows.empty())
    return;

  for (const auto &arrow : mArrows) {
    if (mShowBox.mValue) {
      // Draw box at position
      // Size of arrow is small, maybe 0.5
      AABB aabb(arrow.mPos - 0.25f, arrow.mPos + 0.25f);
      RenderUtils::drawOutlinedAABB(aabb, true, mColor.getAsImColor());
    }
  }
}
