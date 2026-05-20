#pragma once
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>


class HealthBar : public ModuleBase<HealthBar> {
public:
  NumberSetting mX =
      NumberSetting("X", "X Position", 10.0f, 0.0f, 1000.0f, 1.0f);
  NumberSetting mY =
      NumberSetting("Y", "Y Position", 100.0f, 0.0f, 1000.0f, 1.0f);
  NumberSetting mWidth =
      NumberSetting("Width", "Bar Width", 100.0f, 50.0f, 300.0f, 1.0f);
  NumberSetting mHeight =
      NumberSetting("Height", "Bar Height", 10.0f, 5.0f, 30.0f, 1.0f);
  ColorSetting mColor =
      ColorSetting("Color", "Bar Color", ImColor(255, 0, 0, 255));

  HealthBar()
      : ModuleBase("HealthBar", "Shows your health bar on the HUD",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mX, &mY, &mWidth, &mHeight, &mColor);
    mNames = {{Lowercase, "healthbar"},
              {LowercaseSpaced, "health bar"},
              {Normal, "HealthBar"},
              {NormalSpaced, "Health Bar"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onRender(RenderEvent &event);
};
