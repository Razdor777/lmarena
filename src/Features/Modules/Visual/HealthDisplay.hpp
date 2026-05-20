#pragma once
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class HealthDisplay : public ModuleBase<HealthDisplay> {
public:
  BoolSetting mShowPlayers = BoolSetting("Players", "Show on players", true);
  BoolSetting mShowMobs = BoolSetting("Mobs", "Show on mobs", false);
  BoolSetting mShowAnimals = BoolSetting("Animals", "Show on animals", false);
  ColorSetting mColor =
      ColorSetting("Color", "Text Color", ImColor(255, 255, 255, 255));

  HealthDisplay()
      : ModuleBase("HealthDisplay", "Displays health numbers on entities",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mShowPlayers, &mShowMobs, &mShowAnimals, &mColor);
    mNames = {{Lowercase, "healthdisplay"},
              {LowercaseSpaced, "health display"},
              {Normal, "HealthDisplay"},
              {NormalSpaced, "Health Display"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onRender(RenderEvent &event);
};
