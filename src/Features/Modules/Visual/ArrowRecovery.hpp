#pragma once
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <SDK/Minecraft/Actor/EntityId.hpp>

struct RecoverableArrow {
  int64_t mEntityId;
  glm::vec3 mPos;
  float mTime;
};

class ArrowRecovery : public ModuleBase<ArrowRecovery> {
public:
  BoolSetting mShowBox = BoolSetting("Show Box", "Draw box around arrow", true);
  ColorSetting mColor =
      ColorSetting("Color", "ESP Color", ImColor(0, 255, 255, 100));
  NumberSetting mTimeout = NumberSetting(
      "Timeout", "Max time to show (seconds)", 60.0f, 10.0f, 300.0f, 10.0f);

  std::vector<RecoverableArrow> mArrows;

  ArrowRecovery()
      : ModuleBase("ArrowRecovery", "Highlights fired arrows for recovery",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mShowBox, &mColor, &mTimeout);
    mNames = {{Lowercase, "arrowrecovery"},
              {LowercaseSpaced, "arrow recovery"},
              {Normal, "ArrowRecovery"},
              {NormalSpaced, "Arrow Recovery"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onProjectileHit(ProjectileHitEvent &event);
  void onRender(RenderEvent &event);
  void onBaseTick(BaseTickEvent &event);
};
