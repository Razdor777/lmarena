#pragma once
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <deque>

struct TracerPoint {
  glm::vec3 mPos;
  float mTime;
};

struct TracerPath {
  std::vector<TracerPoint> mPoints;
  uint64_t mEntityId;
  ImColor mColor;
};

class ProjectileTracer : public ModuleBase<ProjectileTracer> {
public:
  NumberSetting mLineWidth = NumberSetting(
      "Line Width", "Width of the tracer line", 2.0f, 0.5f, 5.0f, 0.1f);
  NumberSetting mFadeTime =
      NumberSetting("Fade Time", "Time before trace fades (seconds)", 3.0f,
                    1.0f, 10.0f, 0.5f);
  ColorSetting mColor =
      ColorSetting("Color", "Tracer Color", ImColor(255, 255, 255, 255));

  // Map of EntityID -> Path
  std::map<uint64_t, TracerPath> mPaths;

  ProjectileTracer()
      : ModuleBase("ProjectileTracer", "Draws lines following projectiles",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mLineWidth, &mFadeTime, &mColor);
    mNames = {{Lowercase, "projectiletracer"},
              {LowercaseSpaced, "projectile tracer"},
              {Normal, "ProjectileTracer"},
              {NormalSpaced, "Projectile Tracer"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onBaseTick(BaseTickEvent &event);
  void onRender(RenderEvent &event);
};
