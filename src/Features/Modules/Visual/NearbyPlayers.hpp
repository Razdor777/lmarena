#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>

class NearbyPlayers : public ModuleBase<NearbyPlayers> {
public:
  // ── Settings ───────────────────────────────────────────────────────────
  NumberSetting mMaxDist =
      NumberSetting("Max Distance", "Only show players within N blocks", 64.f,
                    1.f, 200.f, 1.f);
  NumberSetting mMaxCount = NumberSetting("Max Count", "Max players to display",
                                          10.f, 1.f, 32.f, 1.f);
  NumberSetting mScale =
      NumberSetting("Scale", "UI scale", 1.f, 0.5f, 2.f, 0.05f);
  NumberSetting mOpacity =
      NumberSetting("Opacity", "Background opacity", 0.85f, 0.f, 1.f, 0.05f);
  NumberSetting mRounding =
      NumberSetting("Rounding", "Corner rounding", 8.f, 0.f, 20.f, 1.f);
  NumberSetting mFontSize =
      NumberSetting("Font Size", "Text size", 12.5f, 8.f, 20.f, 0.5f);
  BoolSetting mShowDist =
      BoolSetting("Show Distance", "Show blocks distance", true);
  BoolSetting mSortDist =
      BoolSetting("Sort Distance", "Sort by nearest first", true);
  BoolSetting mShowHealth = BoolSetting("Show Health", "Show health bar", true);
  BoolSetting mColorDist =
      BoolSetting("Color Distance", "Color distance red→green", true);
  BoolSetting mGlassStyle =
      BoolSetting("Glass Style", "Glassmorphism background", true);
  BoolSetting mPulseNew =
      BoolSetting("Pulse New", "Pulse animation on new player", true);

  NearbyPlayers()
      : ModuleBase("NearbyPlayers", "Shows nearby players with distance",
                   ModuleCategory::Visual, 0, false) {
    addSettings(&mMaxDist, &mMaxCount, &mScale, &mOpacity, &mRounding,
                &mFontSize, &mShowDist, &mSortDist, &mShowHealth, &mColorDist,
                &mGlassStyle, &mPulseNew);
    mNames = {{Lowercase, "nearbyplayers"},
              {LowercaseSpaced, "nearby players"},
              {Normal, "NearbyPlayers"},
              {NormalSpaced, "Nearby Players"}};
  }

  struct PlayerEntry {
    std::string name;
    float dist;
    float health;
    float maxHealth;
    float introAnim;
    bool isNew;
    float newPulse;
    float animHealth = 0.f;
  };

  std::vector<PlayerEntry> mPlayers;
  std::set<std::string> mKnownNames;
  HudElement *mHudElement = nullptr;

  // ← ДОБАВЛЕНО: Health Calculation (как в TargetHUD)
  struct HealthInfo {
    float health = 20;
    float lastAbsorption = 0;
    float damage = 1;
  };
  std::map<std::string, HealthInfo> mHealths;
  uint64_t mLastHealTime = 0;

  void onEnable() override;
  void onDisable() override;
  void calculateHealths(); // ← ДОБАВЛЕНО
  void onBaseTickEvent(class BaseTickEvent &event);
  void onRenderEvent(class RenderEvent &event);
};