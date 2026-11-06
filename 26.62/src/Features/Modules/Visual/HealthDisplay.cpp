#include "HealthDisplay.hpp"
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>

void HealthDisplay::onEnable() {
  gFeatureManager->mDispatcher->listen<RenderEvent, &HealthDisplay::onRender>(
      this);
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &HealthDisplay::onBaseTickEvent>(this);

  mHealths.clear();
  mLastHealTime = NOW;
}

void HealthDisplay::onDisable() {
  gFeatureManager->mDispatcher->deafen<RenderEvent, &HealthDisplay::onRender>(
      this);
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &HealthDisplay::onBaseTickEvent>(this);
}

// NearbyPlayers-style damage tracking (identical logic):
// watches absorption/hurt-time deltas to compute real damage, then simulates
// slow passive regeneration (+1 hp every 4s, capped at 20).
void HealthDisplay::calculateHealths() {
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
                else if (hurtTime == 9)
                {
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

void HealthDisplay::onBaseTickEvent(BaseTickEvent &event) {
  if (!event.mActor) return;
  calculateHealths();
}

// NearbyPlayers-style health resolution — EXACTLY the same flow:
// 1) HealthTracker data parsed from server text (nametag, then clean raw
//    name) — also gives us the real maxHealth
// 2) damage-tracking fallback (mHealths, kept by calculateHealths)
// 3) raw vanilla values
void HealthDisplay::resolveHealth(Actor* actor, float& health, float& maxHealth) {
    if (!actor) return;

    health = actor->getHealth();
    maxHealth = actor->getMaxHealth();

    if (!actor->isPlayer()) return;

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
    } else {
        if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
            health = th;
            maxHealth = tmh;
            tracked = true;
        }
    }

    // NearbyPlayers fallback: damage simulation when the server
    // doesn't expose health at all
    if (!tracked && mHealths.count(cleanName)) {
        health = mHealths[cleanName].health;
    }
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

    float health = 0.f;
    float maxHealth = 20.f;
    resolveHealth(actor, health, maxHealth);

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
