#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>

// Shows directional arrows pointing toward nearby enemies
// Arrows: navigation chevron shape, pink->red->purple animated gradient
class EnemyIndicator : public ModuleBase<EnemyIndicator>
{
public:
    NumberSetting mRange      = NumberSetting("Range",       "Detection range in blocks",         30.f, 1.f, 100.f, 1.f);
    NumberSetting mRadius     = NumberSetting("Radius",      "Indicator ring radius (px)",        80.f, 30.f, 200.f, 5.f);
    NumberSetting mArrowSize  = NumberSetting("Arrow Size",  "Size of each arrow",                14.f, 6.f, 30.f,  1.f);
    NumberSetting mMaxArrows  = NumberSetting("Max Arrows",  "Max enemies to show arrows for",    5.f,  1.f, 16.f,  1.f);
    NumberSetting mOpacity    = NumberSetting("Opacity",     "Arrow opacity",                     0.9f, 0.1f, 1.f,  0.05f);
    BoolSetting   mShowDist   = BoolSetting  ("Show Dist",   "Show distance label near arrow",    true);
    BoolSetting   mColorDist  = BoolSetting  ("Color by Dist","Pink=close, Purple=far",           true);
    BoolSetting   mPulseClose = BoolSetting  ("Pulse Close", "Pulse arrow if enemy <10 blocks",  true);
    BoolSetting   mShowCount  = BoolSetting  ("Show Count",  "Show total enemy count at center",  true);
    BoolSetting   mGlow       = BoolSetting  ("Glow",        "Glow effect around arrows",         true);

    EnemyIndicator() : ModuleBase("EnemyIndicator",
        "Directional arrows toward nearby enemies (pink/red/purple chevrons)",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mRange, &mRadius, &mArrowSize, &mMaxArrows, &mOpacity,
            &mShowDist, &mColorDist, &mPulseClose, &mShowCount, &mGlow
        );
        mNames = {
            {Lowercase,       "enemyindicator"},
            {LowercaseSpaced, "enemy indicator"},
            {Normal,          "EnemyIndicator"},
            {NormalSpaced,    "Enemy Indicator"}
        };
    }

    HudElement* mHudElement = nullptr;

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
};
