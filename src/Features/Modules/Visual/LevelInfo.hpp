#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>
//
// Created by vastrakai on 7/2/2024.
// Redesigned with premium horizontal bar + HudEditor integration.
//

class LevelInfo : public ModuleBase<LevelInfo>
{
public:
    // ── What to show ───────────────────────────────────────────────────────
    BoolSetting mShowFPS       = BoolSetting("Show FPS",    "Show the current FPS",              true);
    BoolSetting mShowPing      = BoolSetting("Show Ping",   "Show the current ping",             true);
    BoolSetting mShowName      = BoolSetting("Show Name",   "Show your username",                true);
    BoolSetting mShowXYZ       = BoolSetting("Show XYZ",    "Show XYZ coordinates",              true);
    BoolSetting mShowBPS       = BoolSetting("Show BPS",    "Show blocks per second",            false);
    BoolSetting mShowArrows    = BoolSetting("Show Arrows", "Show arrow count",                  false);
    BoolSetting mShowEnderPearls = BoolSetting("Show Pearls","Show ender pearl count",           false);
    BoolSetting mShowKicksAmount = BoolSetting("Kicks Counter","Show times kicked",              false);
    BoolSetting mShowSpells    = BoolSetting("Show Spells", "Show spells from inventory",        false);
    BoolSetting mShowHealthSpells    = BoolSetting("Health Spells","Show health spells",         false);
    BoolSetting mShowSpeedSpells     = BoolSetting("Speed Spells", "Show speed spells",          false);
    BoolSetting mShowFireTrailSpells = BoolSetting("FireTrail Spells","Show fire trail spells",  false);

    // ── Visual ─────────────────────────────────────────────────────────────
    NumberSetting mScale     = NumberSetting("Scale",    "Bar scale",             1.f,  0.5f, 2.f,  0.05f);
    NumberSetting mOpacity   = NumberSetting("Opacity",  "Bar opacity",           0.88f,0.f,  1.f,  0.05f);
    NumberSetting mRounding  = NumberSetting("Rounding", "Corner rounding",       8.f,  0.f,  30.f, 1.f);
    NumberSetting mFontSize  = NumberSetting("Font Size","Label font size",        11.5f,7.f,  18.f, 0.5f);
    BoolSetting   mGlass     = BoolSetting  ("Glass",    "Glassmorphism blur bg", true);
    BoolSetting   mDividers  = BoolSetting  ("Dividers", "Show dividers between items", true);
    BoolSetting   mIcons     = BoolSetting  ("Icons",    "Show icon prefix",      true);
    BoolSetting   mColorPing = BoolSetting  ("Color Ping","Color ping red when high", true);

    LevelInfo() : ModuleBase("LevelInfo", "Premium horizontal info bar.", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mShowFPS, &mShowPing, &mShowName, &mShowXYZ,
            &mShowBPS, &mShowArrows, &mShowEnderPearls, &mShowKicksAmount,
            &mShowSpells, &mShowHealthSpells, &mShowSpeedSpells, &mShowFireTrailSpells,
            &mScale, &mOpacity, &mRounding, &mFontSize,
            &mGlass, &mDividers, &mIcons, &mColorPing
        );

        VISIBILITY_CONDITION(mShowHealthSpells,    mShowSpells.mValue);
        VISIBILITY_CONDITION(mShowSpeedSpells,     mShowSpells.mValue);
        VISIBILITY_CONDITION(mShowFireTrailSpells, mShowSpells.mValue);

        mNames = {
            {Lowercase,       "levelinfo"},
            {LowercaseSpaced, "level info"},
            {Normal,          "LevelInfo"},
            {NormalSpaced,    "Level Info"}
        };
    }

    // ── Data ───────────────────────────────────────────────────────────────
    int   mArrows = 0, mPearls = 0;
    int   mHealthSpells = 0, mHearts = 0;
    int   mSpeedSpells  = 0, mSeconds = 0;
    int   mFireTrailSpells = 0, mBlocks = 0;
    int   mKicksAmount = 0;
    float mBps = 0.f, mAveragedBps = 0.f;
    std::map<uint64_t, float> mBpsHistory;
    __int64 mPing = 0, mEventDelay = 0;

    HudElement* mHudElement = nullptr;

    // ── Methods ────────────────────────────────────────────────────────────
    int  getPearlsAmount();
    int  getArrowsAmount();
    int  getSpellsAmount(int idx);
    void calculateValue(int idx);
    void spellsUpdate();

    void onEnable()  override;
    void onDisable() override;
    void onSendImmediateEvent(class SendImmediateEvent& event);
    void onPingUpdateEvent(class PingUpdateEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
};