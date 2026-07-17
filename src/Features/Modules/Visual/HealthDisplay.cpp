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

// TargetHUD-style health calculation
static float calculateActorHealth(Actor* actor) {
    if (!actor) return 0.f;
    float health = actor->getHealth();
    float maxHealth = actor->getMaxHealth();
    
    if (actor->isPlayer()) {
        std::string targetName = actor->getNameTag();
        size_t nl = targetName.find('\n');
        if (nl != std::string::npos) targetName = targetName.substr(0, nl);
        
        std::string cleanName = ColorUtils::removeColorCodes(actor->getRawName());
        
        float th = health, tmh = maxHealth;
        bool tracked = false;
        if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
            health = th;
            maxHealth = tmh;
            tracked = true;
        } else if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
            health = th;
            maxHealth = tmh;
            tracked = true;
        }
    }
    return health;
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
      continue;

    float health = calculateActorHealth(actor);
    float maxHealth = actor->getMaxHealth();
    
    std::string text = std::to_string((int)health) + " HP";

    glm::vec3 pos = *actor->getPos();
    auto AABB = actor->getAABBShapeComponent();
    if (AABB)
      pos.y += AABB->mHeight + 0.8f;
    else
      pos.y += 2.3f;

    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(pos, screenPos)) {
      ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
      ImColor col = mColor.getAsImColor();
      // Color based on health percentage
      float hpPerc = maxHealth > 0 ? health / maxHealth : 1.f;
      if (hpPerc > 0.6f) col = ImColor(100, 255, 100, 255);
      else if (hpPerc > 0.3f) col = ImColor(255, 255, 100, 255);
      else col = ImColor(255, 80, 80, 255);
      
      ImGui::GetBackgroundDrawList()->AddText(
          ImVec2(screenPos.x - textSize.x / 2, screenPos.y - textSize.y / 2),
          col, text.c_str());
    }
  }
}
