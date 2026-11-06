#pragma once
#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/Modules/Module.hpp>


class DamageLog : public ModuleBase<DamageLog> {
public:
  BoolSetting mOnlyMyDamage =
      BoolSetting("Only My Damage", "Only log damage dealt by you",
                  false); // Default false until we verify source check
  BoolSetting mShowInChat =
      BoolSetting("Show In Chat", "Print damage to chat", true);
  BoolSetting mShowOnScreen =
      BoolSetting("Show On Screen", "Show damage on screen (popup)", false);

  DamageLog()
      : ModuleBase("DamageLog", "Logs damage events", ModuleCategory::Visual, 0,
                   false) {
    addSettings(&mOnlyMyDamage, &mShowInChat, &mShowOnScreen);
    mNames = {{Lowercase, "damagelog"},
              {LowercaseSpaced, "damage log"},
              {Normal, "DamageLog"},
              {NormalSpaced, "Damage Log"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onEntityHurt(EntityHurtEvent &event);
};
