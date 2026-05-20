#include "HealthDisplay.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>

void HealthDisplay::onEnable() {
  gFeatureManager->mDispatcher->listen<RenderEvent, &HealthDisplay::onRender>(
      this);
}

void HealthDisplay::onDisable() {
  gFeatureManager->mDispatcher->deafen<RenderEvent, &HealthDisplay::onRender>(
      this);
}

void HealthDisplay::onRender(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  auto list = ActorUtils::getActorList(false, true);

  for (auto actor : list) {
    if (!actor || actor == player)
      continue;

    bool isPlayer = actor->isPlayer();
    if (isPlayer && !mShowPlayers.mValue)
      continue;
    if (!isPlayer && !mShowMobs.mValue && !mShowAnimals.mValue)
      continue; // Simplify check

    // Render
    float health = actor->getHealth();
    if (isPlayer) {
      std::string targetName = actor->getNameTag();
      size_t nl = targetName.find('\n');
      if (nl != std::string::npos) targetName = targetName.substr(0, nl);
      
      float th = health, tmh = 20.f;
      if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
        health = th;
      } else {
        std::string cleanName = actor->getRawName();
        if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
          health = th;
        }
      }
    }
    std::string text = std::to_string((int)health) + " HP";

    glm::vec3 pos = *actor->getPos();
    auto AABB = actor->getAABBShapeComponent();
    if (AABB)
      pos.y += AABB->mHeight + 0.8f; // Higher than TeamHealthBars
    else
      pos.y += 2.3f;

    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(pos, screenPos)) {
      ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
      ImGui::GetBackgroundDrawList()->AddText(
          ImVec2(screenPos.x - textSize.x / 2, screenPos.y - textSize.y / 2),
          mColor.getAsImColor(), text.c_str());
    }
  }
}
