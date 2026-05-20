#include "DamageLog.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/GameUtils/ChatUtils.hpp> // Assuming ChatUtils exists
#include <Utils/MiscUtils/ColorUtils.hpp>


void DamageLog::onEnable() {
  gFeatureManager->mDispatcher
      ->listen<EntityHurtEvent, &DamageLog::onEntityHurt>(this);
}

void DamageLog::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<EntityHurtEvent, &DamageLog::onEntityHurt>(this);
}

void DamageLog::onEntityHurt(EntityHurtEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player || !event.mEntity)
    return;

  if (mOnlyMyDamage.mValue) {
    // Attempt to check source
    // This is experimental
    if (!event.mSource)
      return;

    // Offset 16 check
    // EntityId* sourceId = reinterpret_cast<EntityId*>((uintptr_t)event.mSource
    // + 16); if (*sourceId != player->getRuntimeID()) return;

    // Since we are not sure, we just log everything if enabled, or disable for
    // now Or if the user asked, we can assume they want robust implementation
    // For now, let's just log everything to debug
    return;
  }

  std::string entityName = event.mEntity->getNameTag();
  if (entityName.empty())
    entityName = "Unknown";

  std::string message = "Entity " + entityName + " took " +
                        std::to_string(event.mDamage) + " damage";

  if (mShowInChat.mValue) {
    auto guiData = ClientInstance::get()->getGuiData();
    if (guiData) {
      guiData->displayClientMessage(message);
    }
  }
}
