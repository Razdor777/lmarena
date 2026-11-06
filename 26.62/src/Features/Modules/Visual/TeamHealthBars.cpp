#include "TeamHealthBars.hpp"
#include <Features/Modules/Misc/Friends.hpp> // Assuming FriendManager is here
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>

void TeamHealthBars::onEnable() {
  gFeatureManager->mDispatcher->listen<RenderEvent, &TeamHealthBars::onRender>(
      this);
}

void TeamHealthBars::onDisable() {
  gFeatureManager->mDispatcher->deafen<RenderEvent, &TeamHealthBars::onRender>(
      this);
}

void TeamHealthBars::onRender(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  auto list = ActorUtils::getActorList(
      false, true); // Players only? Or all? User said "Team", implies players.

  for (auto actor : list) {
    if (!actor || actor == player)
      continue;
    if (!actor->isPlayer())
      continue;

    // Check if friend
    bool isFriend = false;
    if (gFriendManager) {
      isFriend = gFriendManager->isFriend(actor);
    }

    // If not friend, maybe skip? User called it "TeamHealthBars".
    // Let's assume only friends/team.
    if (!isFriend)
      continue;

    float health = actor->getHealth();
    float maxHealth = actor->getMaxHealth();

    std::string targetName = actor->getNameTag();
    size_t nl = targetName.find('\n');
    if (nl != std::string::npos) targetName = targetName.substr(0, nl);

    float th = health, tmh = maxHealth;
    if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
      health = th;
      maxHealth = tmh;
    } else {
      std::string cleanName = ColorUtils::removeColorCodes(actor->getRawName());
      if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
        health = th;
        maxHealth = tmh;
      }
    }

    if (maxHealth <= 0)
      maxHealth = 20.0f;
    float ratio = std::clamp(health / maxHealth, 0.0f, 1.0f);

    // Render above head
    glm::vec3 pos = *actor->getPos();
    auto AABB = actor->getAABBShapeComponent();
    if (AABB)
      pos.y += AABB->mHeight + 0.5f;
    else
      pos.y += 2.0f;

    ImVec2 screenPos;
    if (RenderUtils::worldToScreen(pos, screenPos)) {
      ImVec2 size(40.f, 4.f);

      // Draw background
      ImGui::GetBackgroundDrawList()->AddRectFilled(
          ImVec2(screenPos.x - size.x / 2, screenPos.y - size.y / 2),
          ImVec2(screenPos.x + size.x / 2, screenPos.y + size.y / 2),
          IM_COL32(0, 0, 0, 150));

      // Draw health
      // ratio is already calculated and clamped above

      ImGui::GetBackgroundDrawList()->AddRectFilled(
          ImVec2(screenPos.x - size.x / 2, screenPos.y - size.y / 2),
          ImVec2(screenPos.x - size.x / 2 + size.x * ratio,
                 screenPos.y + size.y / 2),
          mColor.getAsImColor());

      if (mShowValue.mValue) {
        std::string text = std::to_string((int)health);
        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(screenPos.x - textSize.x / 2, screenPos.y - textSize.y - 5),
            IM_COL32(255, 255, 255, 255), text.c_str());
      }
    }
  }
}
