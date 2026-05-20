#pragma once
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class TeamHealthBars : public ModuleBase<TeamHealthBars> {
public:
  ColorSetting mColor =
      ColorSetting("Color", "Bar Color", ImColor(0, 255, 0, 255));
  BoolSetting mShowValue =
      BoolSetting("Show Value", "Show numeric value", true);

  TeamHealthBars()
      : ModuleBase("TeamHealthBars", "Renders health bars above teammates",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mColor, &mShowValue);
    mNames = {{Lowercase, "teamhealthbars"},
              {LowercaseSpaced, "team health bars"},
              {Normal, "TeamHealthBars"},
              {NormalSpaced, "Team Health Bars"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onRender(RenderEvent &event);
};
