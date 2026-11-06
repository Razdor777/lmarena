#include "BowHitMarker.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

void BowHitMarker::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<ProjectileHitEvent, &BowHitMarker::onProjectileHit>(this);
  gFeatureManager->mDispatcher->listen<RenderEvent, &BowHitMarker::onRender>(
      this);
  mMarkers.clear();
}

void BowHitMarker::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<ProjectileHitEvent, &BowHitMarker::onProjectileHit>(this);
  gFeatureManager->mDispatcher->deafen<RenderEvent, &BowHitMarker::onRender>(
      this);
  mMarkers.clear();
}

void BowHitMarker::onProjectileHit(ProjectileHitEvent &event) {
  if (event.mResult.mType != HitType::ENTITY)
    return;

  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player || !event.mProjectile)
    return;

  // Check if projectile owner is local player
  // This requires getting the owner ID from the projectile actor
  // ProjectileComponent usually has owner ID.
  // Or we can check if it was spawned by us?
  // For now, assume if we are hooking, we might get all hits.
  // Ideally we check owner.

  // Simplification: always show for now, or check distance/direction?
  // Better: Check if we are holding a bow? No, arrow could be in flight.

  // Add marker
  mMarkers.push_back({event.mResult.mPos, mDuration.mValue});

  if (mPlaySound.mValue) {
    // Play sound (using ClientInstance or Level)
    // ClientInstance::get()->playUi("random.orb", 1.0f, 1.0f);
  }
}

void BowHitMarker::onRender(RenderEvent &event) {
  if (mMarkers.empty())
    return;

  float deltaTime = 1.0f / 60.0f; // Approx
  // Or use global delta time if available

  auto it = mMarkers.begin();
  while (it != mMarkers.end()) {
    it->mTime -= deltaTime;
    if (it->mTime <= 0) {
      it = mMarkers.erase(it);
    } else {
      // Render
      if (mShow3D.mValue) {
        ImVec2 screenPos;
        if (RenderUtils::worldToScreen(it->mPosition, screenPos)) {
          ImGui::GetBackgroundDrawList()->AddLine(
              ImVec2(screenPos.x - 5, screenPos.y - 5),
              ImVec2(screenPos.x + 5, screenPos.y + 5),
              IM_COL32(255, 255, 255, 255), 2.0f);
          ImGui::GetBackgroundDrawList()->AddLine(
              ImVec2(screenPos.x + 5, screenPos.y - 5),
              ImVec2(screenPos.x - 5, screenPos.y + 5),
              IM_COL32(255, 255, 255, 255), 2.0f);
        }
      }
      ++it;
    }
  }
}
