#include "HealthBar.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

void HealthBar::onEnable() {
  gFeatureManager->mDispatcher->listen<RenderEvent, &HealthBar::onRender>(this);
}

void HealthBar::onDisable() {
  gFeatureManager->mDispatcher->deafen<RenderEvent, &HealthBar::onRender>(this);
}

void HealthBar::onRender(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;

  float health = player->getHealth();
  float maxHealth = player->getMaxHealth();
  if (maxHealth <= 0)
    maxHealth = 20.0f;

  float ratio = std::clamp(health / maxHealth, 0.0f, 1.0f);

  ImVec2 pos(mX.mValue, mY.mValue);
  ImVec2 size(mWidth.mValue, mHeight.mValue);

  // Background
  ImGui::GetBackgroundDrawList()->AddRectFilled(
      pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(0, 0, 0, 150));

  // Bar
  ImGui::GetBackgroundDrawList()->AddRectFilled(
      pos, ImVec2(pos.x + size.x * ratio, pos.y + size.y),
      mColor.getAsImColor());

  // Text
  std::string text =
      std::to_string((int)health) + "/" + std::to_string((int)maxHealth);
  ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
  ImGui::GetBackgroundDrawList()->AddText(
      ImVec2(pos.x + (size.x - textSize.x) / 2,
             pos.y + (size.y - textSize.y) / 2),
      IM_COL32(255, 255, 255, 255), text.c_str());
}
