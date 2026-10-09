#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <chrono>

class CustomCrosshair : public ModuleBase<CustomCrosshair>
{
public:
    enum class CrosshairStyle { Cross, Circle, CsGo };
    enum class DynamicMode { Off, Attack, Move, Full };

    // One-shot look presets. Selecting one writes the sliders below and then
    // resets back to Custom, so the preset never fights your own tweaking.
    enum class Preset { Custom, CsGo, Valorant, Dot, Classic };

    EnumSettingT<CrosshairStyle> mStyle = EnumSettingT<CrosshairStyle>(
        "Style", "Style of the crosshair", CrosshairStyle::CsGo, "Cross", "Circle", "CsGo");

    EnumSettingT<DynamicMode> mDynamic = EnumSettingT<DynamicMode>(
        "Dynamic", "When it expands", DynamicMode::Full, "Off", "Attack", "Move", "Full");

    EnumSettingT<Preset> mPreset = EnumSettingT<Preset>(
        "Preset", "Apply a ready-made look, then keeps the sliders editable",
        Preset::Custom, "Custom", "CS:GO", "Valorant", "Dot", "Classic");

    NumberSetting mScale = NumberSetting("Scale", "Scale", 0.8f, 0.5f, 3.f, 0.1f);
    NumberSetting mSize = NumberSetting("Size", "Line length", 3.5f, 1.f, 20.f, 0.5f);
    NumberSetting mGap = NumberSetting("Gap", "Gap from center", 6.5f, 0.f, 20.f, 0.5f);
    // Step is 0.01 so the line can be tuned exactly (1.10, 1.25, ...) and 0 is
    // allowed, which hides the crosshair completely. This slider also drives the
    // dark outline drawn around the lines, so no separate outline settings.
    NumberSetting mThickness = NumberSetting("Thickness", "Line thickness and the outline around it", 1.28f, 0.f, 6.f, 0.01f);
    NumberSetting mExpandSize = NumberSetting("Expand", "Max expansion", 6.f, 1.f, 30.f, 0.5f);

    BoolSetting mDot = BoolSetting("Dot", "Show dot in center", false);
    BoolSetting mOutline = BoolSetting("Outline", "Draw a dark outline around the crosshair", false);
    BoolSetting mSneakShrink = BoolSetting("SneakShrink", "Shrink gap when sneaking", true);
    BoolSetting mRainbow = BoolSetting("Rainbow", "Rainbow color", false);
    ColorSetting mColor = ColorSetting("Color", "Color", ImColor(255, 255, 255, 220));

    // ── Sway: the reticle physically walks with you instead of being nailed
    //    to the exact center of the screen. Small by default on purpose.
    BoolSetting mSway = BoolSetting("Sway", "Let the reticle drift while you move", true);
    NumberSetting mSwayAmount = NumberSetting("Sway Amount", "How far the reticle drifts", 3.5f, 0.f, 14.f, 0.5f);

    // ── Rotation sway: Roblox-style camera lag. The reticle is dragged behind
    //    the camera when you flick it and springs back to the center.
    BoolSetting mRotSway = BoolSetting("Rotation Sway", "Reticle lags behind when you turn the camera", true);
    NumberSetting mRotSwayAmount = NumberSetting("Rotation Sway Amount", "How far the reticle trails behind a camera flick", 1.f, 0.f, 4.f, 0.05f);
    NumberSetting mRotSwayLag = NumberSetting("Rotation Lag", "How long the reticle keeps trailing (lower = snappier)", 0.45f, 0.05f, 1.f, 0.05f);

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

    // Sway animation
    ImVec2 mSwayOffset = ImVec2(0.f, 0.f);
    float  mSwayPhase = 0.f;

    // Rotation sway: spring state (driven by camera yaw/pitch velocity)
    float  mRotOffsetX = 0.f;
    float  mRotOffsetY = 0.f;
    float  mRotVelX = 0.f;
    float  mRotVelY = 0.f;
    float  mPrevCamYaw = 0.f;
    float  mPrevCamPitch = 0.f;
    bool   mHasPrevCamRot = false;

    CustomCrosshair() : ModuleBase("CustomCrosshair", "Custom dynamic crosshair", ModuleCategory::Visual, 0, true)
    {
        gFeatureManager->mDispatcher->listen<RenderEvent, &CustomCrosshair::onRenderEvent>(this);

        addSettings(
            &mStyle,
            &mPreset,
            &mDynamic,
            &mScale,
            &mSize,
            &mGap,
            &mThickness,
            &mExpandSize,
            &mDot,
            &mOutline,
            &mSneakShrink,
            &mRainbow,
            &mColor,
            &mSway,
            &mSwayAmount,
            &mRotSway,
            &mRotSwayAmount,
            &mRotSwayLag
        );

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

private:
    void applyPreset(Preset preset);
    std::string getSettingDisplay() override;
};
