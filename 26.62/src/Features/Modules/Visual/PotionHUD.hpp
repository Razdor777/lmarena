#pragma once
#include <Features/Modules/Module.hpp>
#include <SDK/Minecraft/Network/Packets/MobEffectPacket.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>
#include <map>

class PotionHUD : public ModuleBase<PotionHUD>
{
public:
    NumberSetting mScale       = NumberSetting("Scale",       "UI scale",                   1.f,  0.5f, 2.f,  0.05f);
    NumberSetting mOpacity     = NumberSetting("Opacity",     "Background opacity",          0.85f,0.f,  1.f,  0.05f);
    NumberSetting mRounding    = NumberSetting("Rounding",    "Corner rounding",             8.f,  0.f,  20.f, 1.f);
    NumberSetting mFontSize    = NumberSetting("Font Size",   "Text size",                   12.5f,8.f,  20.f, 0.5f);
    BoolSetting   mShowLevel   = BoolSetting  ("Show Level",  "Show amplifier (I, II, III)", true);
    BoolSetting   mShowTimer   = BoolSetting  ("Show Timer",  "Show remaining duration",     true);
    BoolSetting   mBlinkLow    = BoolSetting  ("Blink Low",   "Blink when < 5s remaining",   true);
    BoolSetting   mColorBars   = BoolSetting  ("Color Bars",  "Colored left accent bar",     false); // kept for compat
    BoolSetting   mHideNegative= BoolSetting  ("Hide Negative","Hide debuffs (poison, etc)", false);
    BoolSetting   mSortByTime  = BoolSetting  ("Sort by Time","Sort effects by time left",   true);

    PotionHUD() : ModuleBase("PotionHUD",
        "Displays active potion effects with icons and timers",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mScale, &mOpacity, &mRounding, &mFontSize,
            &mShowLevel, &mShowTimer,
            &mBlinkLow, &mHideNegative, &mSortByTime
        );
        mNames = {
            {Lowercase,       "potionhud"},
            {LowercaseSpaced, "potion hud"},
            {Normal,          "PotionHUD"},
            {NormalSpaced,    "Potion HUD"}
        };
    }

    struct ActiveEffect {
        EffectType type;
        int        amplifier;
        int        durationTicks;
        float      displayTime;
        float      introAnim;
        bool       negative;
    };

    std::map<EffectType, ActiveEffect> mEffects;
    HudElement* mHudElement = nullptr;

    static bool isNegativeEffect(EffectType t);
    static ImColor effectColor(EffectType t);
    static const char* getEffectTextureName(EffectType t);
    static std::string romanLevel(int amplifier);
    static std::string formatTime(int ticks);

    void onEnable()  override;
    void onDisable() override;
    void onPacketInEvent(class PacketInEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent (class RenderEvent&    event);
};