#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <chrono>

class CustomCrosshair : public ModuleBase<CustomCrosshair>
{
public:
    enum class CrosshairStyle { Cross, Circle, CsGo };
    enum class DynamicMode { Off, Attack, Move, Full };

    EnumSettingT<CrosshairStyle> mStyle = EnumSettingT<CrosshairStyle>(
        "Style", "Style of the crosshair", CrosshairStyle::CsGo, "Cross", "Circle", "CsGo");
    
    EnumSettingT<DynamicMode> mDynamic = EnumSettingT<DynamicMode>(
        "Dynamic", "When it expands", DynamicMode::Full, "Off", "Attack", "Move", "Full");

    NumberSetting mScale = NumberSetting("Scale", "Scale", 1.f, 0.5f, 3.f, 0.1f);
    NumberSetting mSize = NumberSetting("Size", "Line length", 4.f, 1.f, 20.f, 0.5f);
    NumberSetting mGap = NumberSetting("Gap", "Gap from center", 4.f, 0.f, 20.f, 0.5f);
    NumberSetting mThickness = NumberSetting("Thickness", "Line thickness", 2.f, 1.f, 6.f, 0.5f);
    NumberSetting mExpandSize = NumberSetting("Expand", "Max expansion", 10.f, 1.f, 30.f, 0.5f);
    
    BoolSetting mDot = BoolSetting("Dot", "Show dot in center", false);
    BoolSetting mOutline = BoolSetting("Outline", "Outline", true);
    BoolSetting mSneakShrink = BoolSetting("SneakShrink", "Shrink gap when sneaking", true);
    BoolSetting mRainbow = BoolSetting("Rainbow", "Rainbow color", false);
    ColorSetting mColor = ColorSetting("Color", "Color", ImColor(255, 255, 255, 220));

    BoolSetting mPresetCsGo = BoolSetting("Preset CS:GO", "Apply CS:GO settings", false);

    // Отслеживание свинга
    bool mWasSwinging = false;
    int mPrevSwingProgress = 0;

    // Отслеживание урона
    bool mWasHurt = false;
    int mPrevHurtTime = 0;

    // Таймеры
    int64_t mLastHitEntityTime = 0;
    int64_t mLastHurtTime = 0;

    // Атака: базовое расширение
    float mAttackExpand = 0.f;      // 0→1 базовое расширение
    bool mWasAttacking = false;      // Был ли атакован в предыдущем кадре
    int64_t mAttackStartTime = 0;   // Когда началась непрерывная атака

    // Атака: пульсация (осцилляция) при удержании
    float mOscPhase = 0.f;          // Фаза синуса
    float mOscAmplitude = 0.f;      // Текущая амплитуда (плавно нарастает/спадает)

    // Остальные анимации
    float mHitAnim = 0.f;
    float mHurtAnim = 0.f;
    float mMoveAnim = 0.f;
    float mAirAnim = 0.f;
    float mSneakAnim = 1.f;

    CustomCrosshair() : ModuleBase("CustomCrosshair", "Custom dynamic crosshair", ModuleCategory::Visual, 0, false)
    {
        gFeatureManager->mDispatcher->listen<RenderEvent, &CustomCrosshair::onRenderEvent>(this);

        addSetting(&mStyle);
        addSetting(&mDynamic);
        addSetting(&mScale);
        addSetting(&mSize);
        addSetting(&mGap);
        addSetting(&mThickness);
        addSetting(&mExpandSize);
        addSetting(&mDot);
        addSetting(&mOutline);
        addSetting(&mSneakShrink);
        addSetting(&mRainbow);
        addSetting(&mColor);
        addSetting(&mPresetCsGo);

        mNames = {
            {Lowercase,       "customcrosshair"},
            {LowercaseSpaced, "custom crosshair"},
            {Normal,          "CustomCrosshair"},
            {NormalSpaced,    "Custom Crosshair"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
};