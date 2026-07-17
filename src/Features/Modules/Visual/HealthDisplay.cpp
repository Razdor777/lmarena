#include "HealthDisplay.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>
#include <Features/Events/BaseTickEvent.hpp>

void HealthDisplay::onEnable() {
  gFeatureManager->mDispatcher->listen<RenderEvent, &HealthDisplay::onRender>(
      this);
  gFeatureManager->mDispatcher->listen<BaseTickEvent,
      &HealthDisplay::onBaseTickEvent>(this);
  mHealths.clear();
  mLastHealTime = NOW;
}

void HealthDisplay::onDisable() {
  gFeatureManager->mDispatcher->deafen<RenderEvent, &HealthDisplay::onRender>(
      this);
  gFeatureManager->mDispatcher->deafen<BaseTickEvent,
      &HealthDisplay::onBaseTickEvent>(this);
  mHealths.clear();
}

// Absorption-based health calculation (ported from NearbyPlayers)
void HealthDisplay::onBaseTickEvent(BaseTickEvent& event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player) return;

  auto actors = ActorUtils::getActorList(true, true);

  bool heal = 4000 <= NOW - mLastHealTime;
  if (heal) mLastHealTime = NOW;

  for (auto actor : actors) {
    if (!actor || actor == player) continue;

    try {
      if (!actor->getMobHurtTimeComponent()) continue;

      std::string rawName = ColorUtils::removeColorCodes(actor->getRawName());
      auto& info = mHealths[rawName];
      float absorption = actor->getAbsorption();
      int hurtTime = actor->getMobHurtTimeComponent()->mHurtTime;

      if (0 < hurtTime) {
        float damage = 0;
        if (absorption < info.lastAbsorption) {
          if (0 < absorption) {
            info.damage = abs(info.lastAbsorption - absorption);
            damage = 0;
          }
          else if (0 < info.lastAbsorption) {
            damage = abs(info.damage - info.lastAbsorption);
          }
        }
        else if (hurtTime == 9) {
          damage = info.damage;
        }

        if (absorption == 0 && 0 < damage) {
          if (info.health - damage < 0) info.health = 0;
          else info.health -= damage;
        }
      }

      if (heal) {
        if (info.health + 1 > 20) info.health = 20;
        else info.health++;
      }

      info.lastAbsorption = absorption;
    } catch (...) {
      continue;
    }
  }
}

// Get tracked health for an actor (NearbyPlayers logic)
static float getTrackedHealth(Actor* actor,
    std::map<std::string, HealthDisplay::HealthInfo>& healths)
{
  if (!actor) return 0.f;
  float health = actor->getHealth();
  float maxHealth = actor->getMaxHealth();

  if (actor->isPlayer()) {
    std::string targetName = actor->getNameTag();
    size_t nl = targetName.find('\n');
    if (nl != std::string::npos) targetName = targetName.substr(0, nl);

    float th = health, tmh = maxHealth;
    bool tracked = false;
    if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
      health = th;
      maxHealth = tmh;
      tracked = true;
    } else {
      std::string cleanName = actor->getRawName();
      if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
        health = th;
        maxHealth = tmh;
        tracked = true;
      }
    }

    // Fallback to absorption-based tracking
    if (!tracked) {
      std::string rawName = ColorUtils::removeColorCodes(actor->getRawName());
      auto it = healths.find(rawName);
      if (it != healths.end()) {
        health = it->second.health;
      }
    }
  }
  return health;
}

void HealthDisplay::onRender(RenderEvent& event) {
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

    float health = getTrackedHealth(actor, mHealths);
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

      // Color based on health percentage — soft, modern palette
      float hpPerc = maxHealth > 0 ? health / maxHealth : 1.f;
      ImColor col;
      if (hpPerc > 0.6f)      col = ImColor(120, 220, 150, 240); // soft green
      else if (hpPerc > 0.3f) col = ImColor(230, 200, 90, 240);  // soft amber
      else                    col = ImColor(220, 100, 100, 240);  // soft red

      ImGui::GetBackgroundDrawList()->AddText(
          ImVec2(screenPos.x - textSize.x / 2, screenPos.y - textSize.y / 2),
          col, text.c_str());
    }
  }
}
