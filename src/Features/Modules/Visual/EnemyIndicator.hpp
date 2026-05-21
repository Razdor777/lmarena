#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>

class EnemyIndicator : public ModuleBase<EnemyIndicator>
{
public:
    enum class IndicatorMode { Arrows, Radar, Compass };

    EnumSettingT<IndicatorMode> mMode = EnumSettingT<IndicatorMode>(
        "Mode", "Arrows = ring chevrons | Radar = circle+pointer | Compass = single arrow above crosshair",
        IndicatorMode::Arrows, "Arrows", "Radar", "Compass");

    NumberSetting mRange     = NumberSetting("Range",      "Detection range (blocks)", 50.f,  1.f, 200.f, 1.f);
    NumberSetting mRadius    = NumberSetting("Radius",     "Ring/circle radius (px)",  90.f, 30.f, 400.f, 5.f);
    NumberSetting mSize      = NumberSetting("Size",       "Arrow/pointer size",       13.f,  4.f,  30.f, 1.f);
    NumberSetting mOpacity   = NumberSetting("Opacity",    "Opacity",                  0.9f,  0.1f,  1.f, 0.05f);
    NumberSetting mMaxCount  = NumberSetting("Max",        "Max enemies shown",         5.f,   1.f,  16.f, 1.f);
    NumberSetting mOffset    = NumberSetting("Offset",     "[Compass] Distance above crosshair (px)", 80.f, 20.f, 300.f, 5.f);
    BoolSetting   mShowDist  = BoolSetting  ("Show Dist",  "Show distance label",      true);
    BoolSetting   mShowCount = BoolSetting  ("Show Count", "Show enemy count",         true);
    BoolSetting   mGlow      = BoolSetting  ("Glow",       "Glow effect",              true);

    EnemyIndicator() : ModuleBase("EnemyIndicator",
        "Enemy direction indicator. Themed animated color.",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mMode, &mRange, &mRadius, &mSize, &mOpacity, &mMaxCount,
                    &mOffset, &mShowDist, &mShowCount, &mGlow);

        VISIBILITY_CONDITION(mRadius,   mMode.mValue != IndicatorMode::Compass);
        VISIBILITY_CONDITION(mMaxCount, mMode.mValue != IndicatorMode::Compass);
        VISIBILITY_CONDITION(mOffset,   mMode.mValue == IndicatorMode::Compass);

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
