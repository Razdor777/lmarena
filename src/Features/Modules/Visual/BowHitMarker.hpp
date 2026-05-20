#pragma once
#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>


struct HitMarker {
  glm::vec3 mPosition;
  float mTime; // Time remaining
};

class BowHitMarker : public ModuleBase<BowHitMarker> {
public:
  // Settings
  BoolSetting mShow3D =
      BoolSetting("3D Marker", "Show 3D marker at hit position", true);
  BoolSetting mPlaySound =
      BoolSetting("Play Sound", "Play 'ding' sound on hit", true);
  NumberSetting mDuration =
      NumberSetting("Duration", "How long the marker stays (seconds)", 2.0f,
                    0.5f, 5.0f, 0.1f);

  std::vector<HitMarker> mMarkers;

  BowHitMarker()
      : ModuleBase("BowHitMarker",
                   "Shows a marker when you hit an entity with a bow",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mShow3D, &mPlaySound, &mDuration);
    mNames = {{Lowercase, "bowhitmarker"},
              {LowercaseSpaced, "bow hit marker"},
              {Normal, "BowHitMarker"},
              {NormalSpaced, "Bow Hit Marker"}};
  }

  void onEnable() override;
  void onDisable() override;
  void onProjectileHit(ProjectileHitEvent &event);
  void onRender(RenderEvent &event);
};
