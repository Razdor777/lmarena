#pragma once
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <map>
#include <string>

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

  // ── NearbyPlayers-style damage tracking state ───────────────────────
  // Same idea as NearbyPlayers::mHealths: tracks damage per player from
  // hurt-time + absorption deltas, with slow passive regen simulation.
  struct HealthInfo {
      float health = 20.f;
      float lastAbsorption = 0.f;
      float damage = 0.f;
  };
  std::map<std::string, HealthInfo> mHealths;
  uint64_t mLastHealTime = 0;

  void calculateHealths();

  // Resolves health/maxHealth for an actor the same way NearbyPlayers does:
  // HealthTracker (nametag, then clean raw name) -> damage-tracking fallback.
  void resolveHealth(class Actor* actor, float& health, float& maxHealth);

  void onEnable() override;
  void onDisable() override;
  void onRender(RenderEvent &event);
  void onBaseTickEvent(class BaseTickEvent &event);
};
