#include "ProjectileTracer.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

void ProjectileTracer::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &ProjectileTracer::onBaseTick>(this);
  gFeatureManager->mDispatcher
      ->listen<RenderEvent, &ProjectileTracer::onRender>(this);
  mPaths.clear();
}

void ProjectileTracer::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &ProjectileTracer::onBaseTick>(this);
  gFeatureManager->mDispatcher
      ->deafen<RenderEvent, &ProjectileTracer::onRender>(this);
  mPaths.clear();
}

void ProjectileTracer::onBaseTick(BaseTickEvent &event) {
  auto list = ActorUtils::getActorList(
      false, true); // Don't include local player, include entities

  for (auto actor : list) {
    if (!actor)
      continue;

    // Simple check: Is it a projectile?
    // We can check if it has a ProjectileComponent
    // Or check runtime ID ranges, or name
    // Actor::isType(ActorType::Projectile) if available

    // For now, assume anything with "arrow", "pearl", "snowball", "trident" in
    // name? OR check components. Let's rely on ProjectileComponent if
    // accessible. If not, we will try to detect by movement or name.

    // Checking name is slow but safe for now given absence of component checks
    std::string name = actor->getRawName(); // or EntityIdentifier
    bool isProjectile = name.find("arrow") != std::string::npos ||
                        name.find("pearl") != std::string::npos ||
                        name.find("snowball") != std::string::npos ||
                        name.find("trident") != std::string::npos;

    if (isProjectile) {
      uint64_t id = actor->getRuntimeID();
      glm::vec3 pos = *actor->getPos();

      if (mPaths.find(id) == mPaths.end()) {
        mPaths[id] = TracerPath{{}, id, mColor.getAsImColor()};
      }

      mPaths[id].mPoints.push_back({pos, mFadeTime.mValue});
    }
  }
}

void ProjectileTracer::onRender(RenderEvent &event) {
  if (mPaths.empty())
    return;

  float dt = 1.0f / 60.0f;

  auto it = mPaths.begin();
  while (it != mPaths.end()) {
    auto &path = it->second;

    if (path.mPoints.empty()) {
      it = mPaths.erase(it);
      continue;
    }

    // Update points
    bool anyAlive = false;
    for (auto &pt : path.mPoints) {
      pt.mTime -= dt;
      if (pt.mTime > 0)
        anyAlive = true;
    }

    // Render lines
    if (path.mPoints.size() > 1) {
      ImDrawList *drawList = ImGui::GetBackgroundDrawList();
      ImColor color = mColor.getAsImColor(); // Use helper

      for (size_t i = 0; i < path.mPoints.size() - 1; i++) {
        if (path.mPoints[i].mTime <= 0 && path.mPoints[i + 1].mTime <= 0)
          continue;

        glm::vec3 p1 = path.mPoints[i].mPos;
        glm::vec3 p2 = path.mPoints[i + 1].mPos; // Connect points

        ImVec2 s1, s2;
        if (RenderUtils::worldToScreen(p1, s1) &&
            RenderUtils::worldToScreen(p2, s2)) {
          drawList->AddLine(s1, s2, color, mLineWidth.mValue);
        }
      }
    }

    if (!anyAlive) {
      it = mPaths.erase(it);
    } else {
      ++it;
    }
  }
}
