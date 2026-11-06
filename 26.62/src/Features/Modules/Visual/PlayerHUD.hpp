#pragma once
//
// PlayerHUD — красивая информационная панель как на скрине:
// Сверху слева: [ник] [FPS] [пинг] [XYZ] [скорость]
// Каждый элемент — отдельный HudElement (перетаскивается)
// По дефолту все в одной строке сверху слева (как на скрине)
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PingUpdateEvent.hpp>
#include <Features/Events/SendImmediateEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <map>

class PlayerHUD : public ModuleBase<PlayerHUD>
{
public:
    // Что показывать
    BoolSetting mShowName    = BoolSetting("Name",    "Show your username",         true);
    BoolSetting mShowFPS     = BoolSetting("FPS",     "Show FPS",                   true);
    BoolSetting mShowPing    = BoolSetting("Ping",    "Show ping",                  true);
    BoolSetting mShowXYZ     = BoolSetting("XYZ",     "Show coordinates",           true);
    BoolSetting mShowSpeed   = BoolSetting("Speed",   "Show speed (BPS)",           true);
    BoolSetting mShowPearls  = BoolSetting("Pearls",  "Show ender pearl count",     false);
    BoolSetting mShowArrows  = BoolSetting("Arrows",  "Show arrow count",           false);

    // Стиль
    NumberSetting mScale     = NumberSetting("Scale",    "Overall scale",        1.f,  0.5f, 2.f,  0.05f);
    NumberSetting mFontSize  = NumberSetting("Font Size","Text size",           11.f,  7.f, 18.f,  0.5f);
    NumberSetting mOpacity   = NumberSetting("Opacity",  "Background opacity",  0.82f, 0.f,  1.f,  0.05f);
    NumberSetting mRounding  = NumberSetting("Rounding", "Corner rounding",     6.f,   0.f, 20.f,  1.f);
    NumberSetting mSpacing   = NumberSetting("Spacing",  "Gap between chips",   4.f,   0.f, 20.f,  1.f);
    BoolSetting   mColorPing = BoolSetting  ("Color Ping","Red ping when high", true);
    BoolSetting   mRainbowName=BoolSetting  ("Rainbow Name","Animated name color",true);
    BoolSetting   mOneRow    = BoolSetting  ("One Row",  "All chips in one row (lock together)", true);

    PlayerHUD() : ModuleBase("PlayerHUD",
        "Красивая информационная панель: ник, FPS, пинг, XYZ, скорость",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mShowName, &mShowFPS, &mShowPing, &mShowXYZ, &mShowSpeed,
            &mShowPearls, &mShowArrows,
            &mScale, &mFontSize, &mOpacity, &mRounding, &mSpacing,
            &mColorPing, &mRainbowName, &mOneRow
        );
        mNames = {
            {Lowercase,       "playerhud"},
            {LowercaseSpaced, "player hud"},
            {Normal,          "PlayerHUD"},
            {NormalSpaced,    "Player HUD"}
        };
    }

    // Data
    int64_t mPing      = 0;
    int64_t mDelay     = 0;
    float   mBps       = 0.f;
    int     mPearls    = 0;
    int     mArrows    = 0;
    std::map<uint64_t, float> mBpsHistory;

    // HUD element — одна точка якоря для всей панели
    HudElement* mHudElem = nullptr;

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onBaseTickEvent(BaseTickEvent& event);
    void onPingUpdateEvent(PingUpdateEvent& event);
    void onSendImmediateEvent(SendImmediateEvent& event);
};
