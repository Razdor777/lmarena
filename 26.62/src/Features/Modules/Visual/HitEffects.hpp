#pragma once
//
// HitEffects - Visual effects when hitting entities
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>

enum class HitState {
    Spawning,   // Появляется (size растёт)
    Active,     // Активна (вращается)
    Despawning  // Исчезает (size уменьшается)
};

struct HitInfo {
    Actor* target;
    int64_t runtimeId;
    uint64_t lastHitTime;      // Время последнего удара (для продления)
    uint64_t stateStartTime;   // Время начала текущего состояния
    glm::vec3 hitPosition;
    float rotation;
    float rotationDirection;
    int rotationCycles;
    float pauseTimer;
    bool isPaused;
    HitState state;
    float animProgress;        // 0-1, прогресс анимации spawn/despawn
};

class HitEffects : public ModuleBase<HitEffects>
{
public:
    // === PRESETS (MAIN CONTROL) ===
    enum class Preset {
        Custom,        // Manual settings
        AnimeStar,     // Anime-style rotating star, rainbow, bounce spawn
        PVPDiamond,    // Clean PVP diamond, fast rotation, red
        SakuraPetal,   // Soft pink corners, slow wave rotation
        FireStar,      // Orange/yellow diamond, pulse rotation, glow
        NeonPulse,     // Cyan neon both styles, custom rotation
        MinimalDot     // Tiny white diamond, no glow, minimal
    };

    EnumSettingT<Preset> mPreset = EnumSettingT<Preset>("Preset", "Quick setup - overrides all settings", Preset::PVPDiamond,
        "Custom", "AnimeStar", "PVPDiamond", "SakuraPetal", "FireStar", "NeonPulse", "MinimalDot");

    enum class Style {
        Diamond,
        Corners,
        Both
    };

    enum class RotationMode {
        Normal,
        PingPong,
        Pulse,
        Wave,
        Custom
    };

    enum class SpawnStyle {
        Grow,           // Вырастает из центра
        GrowRotate,     // Вырастает + быстро вращается в начале
        Explode,        // Быстро появляется с "взрывом"
        Fade,           // Просто fade in
        Bounce,         // Появляется с эффектом пружины
        Spiral          // Появляется по спирали (маленький размер + много вращений)
    };

    enum class DespawnStyle {
        Shrink,         // Уменьшается к центру
        ShrinkRotate,   // Уменьшается + быстро вращается
        Implode,        // Быстро схлопывается
        Fade,           // Просто fade out
        Spin,           // Быстро крутится и исчезает
        Scatter         // "Разлетается" (увеличивается + fade out)
    };

    // === ОСНОВНЫЕ НАСТРОЙКИ ===
    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "The style of hit effect", Style::Diamond, "Diamond", "Corners", "Both");
    NumberSetting mDuration = NumberSetting("Duration", "How long the effect lasts (seconds)", 2.0f, 0.5f, 20.0f, 0.5f);
    NumberSetting mThickness = NumberSetting("Thickness", "Line thickness", 2.0f, 1.0f, 5.0f, 0.5f);
    
    // === НАСТРОЙКИ АНИМАЦИЙ ===
    EnumSettingT<SpawnStyle> mSpawnStyle = EnumSettingT<SpawnStyle>("Spawn Style", "How effect appears", SpawnStyle::GrowRotate, "Grow", "Grow+Rotate", "Explode", "Fade", "Bounce", "Spiral");
    EnumSettingT<DespawnStyle> mDespawnStyle = EnumSettingT<DespawnStyle>("Despawn Style", "How effect disappears", DespawnStyle::ShrinkRotate, "Shrink", "Shrink+Rotate", "Implode", "Fade", "Spin", "Scatter");
    NumberSetting mSpawnTime = NumberSetting("Spawn Time", "Duration of spawn animation (sec)", 0.4f, 0.1f, 2.0f, 0.05f);
    NumberSetting mDespawnTime = NumberSetting("Despawn Time", "Duration of despawn animation (sec)", 0.3f, 0.1f, 2.0f, 0.05f);
    BoolSetting mSpawnEasing = BoolSetting("Spawn Easing", "Smooth spawn animation", true);
    BoolSetting mDespawnEasing = BoolSetting("Despawn Easing", "Smooth despawn animation", true);
    
    // === НАСТРОЙКИ РАЗМЕРА ===
    NumberSetting mDiamondSize = NumberSetting("Diamond Size", "Size of diamond effect", 15.0f, 5.0f, 50.0f, 1.0f);
    NumberSetting mDiamondInnerSize = NumberSetting("Diamond Inner", "Size of inner circle", 0.5f, 0.1f, 1.0f, 0.05f);
    NumberSetting mCornersSize = NumberSetting("Corners Size", "Size of corners effect", 20.0f, 5.0f, 50.0f, 1.0f);
    NumberSetting mCornerLength = NumberSetting("Corner Length", "Length of corner lines", 0.3f, 0.1f, 0.8f, 0.05f);
    NumberSetting mCornerGap = NumberSetting("Corner Gap", "Gap between corners", 1.0f, 0.5f, 2.0f, 0.1f);
    
    // === НАСТРОЙКИ ПОЗИЦИИ ===
    NumberSetting mPositionY = NumberSetting("Position Y", "Vertical position offset", 0.0f, -2.0f, 2.0f, 0.1f);
    NumberSetting mHeightPercent = NumberSetting("Height %", "Position as % of hitbox (0=feet, 100=head)", 50.0f, 0.0f, 150.0f, 5.0f);
    
    // === НАСТРОЙКИ ВРАЩЕНИЯ ===
    EnumSettingT<RotationMode> mRotationMode = EnumSettingT<RotationMode>("Rotation Mode", "Type of rotation", RotationMode::Normal, "Normal", "PingPong", "Pulse", "Wave", "Custom");
    NumberSetting mRotationSpeed = NumberSetting("Rotation Speed", "Speed of rotation", 3.0f, 0.0f, 10.0f, 0.1f);
    
    // === КАСТОМНОЕ ВРАЩЕНИЕ ===
    NumberSetting mCustomCycles = NumberSetting("Cycles", "Rotations before direction change", 2.0f, 0.5f, 10.0f, 0.5f);
    NumberSetting mCustomPause = NumberSetting("Pause Time", "Pause between direction changes (sec)", 0.3f, 0.0f, 2.0f, 0.1f);
    NumberSetting mCustomSpeed = NumberSetting("Custom Speed", "Rotation speed for custom mode", 4.0f, 1.0f, 15.0f, 0.5f);
    BoolSetting mCustomEasing = BoolSetting("Smooth Easing", "Smooth acceleration/deceleration", true);
    
    // === PINGPONG НАСТРОЙКИ ===
    NumberSetting mPingPongAngle = NumberSetting("PingPong Angle", "Max angle for ping-pong", 45.0f, 10.0f, 180.0f, 5.0f);
    NumberSetting mPingPongSpeed = NumberSetting("PingPong Speed", "Speed of ping-pong", 3.0f, 0.5f, 10.0f, 0.5f);
    
    // === WAVE НАСТРОЙКИ ===
    NumberSetting mWaveAmplitude = NumberSetting("Wave Amplitude", "Amplitude of wave rotation", 30.0f, 5.0f, 90.0f, 5.0f);
    NumberSetting mWaveFrequency = NumberSetting("Wave Frequency", "Frequency of wave", 2.0f, 0.5f, 10.0f, 0.5f);
    
    // === PULSE НАСТРОЙКИ ===
    NumberSetting mPulseMinSpeed = NumberSetting("Pulse Min Speed", "Minimum speed", 1.0f, 0.0f, 5.0f, 0.5f);
    NumberSetting mPulseMaxSpeed = NumberSetting("Pulse Max Speed", "Maximum speed", 6.0f, 2.0f, 15.0f, 0.5f);
    NumberSetting mPulseCycleTime = NumberSetting("Pulse Cycle", "Time for one pulse cycle", 1.0f, 0.2f, 3.0f, 0.1f);

    // === НАСТРОЙКИ ЦВЕТА ===
    BoolSetting mRainbow = BoolSetting("Rainbow", "Use rainbow color", false);
    NumberSetting mRainbowSpeed = NumberSetting("Rainbow Speed", "Speed of rainbow cycle", 2.0f, 0.5f, 10.0f, 0.1f);
    NumberSetting mColorR = NumberSetting("Red", "Red color component", 1.0f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorG = NumberSetting("Green", "Green color component", 0.3f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorB = NumberSetting("Blue", "Blue color component", 0.3f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorA = NumberSetting("Alpha", "Transparency", 0.9f, 0.1f, 1.0f, 0.01f);
    
    // === ЭФФЕКТЫ ===
    BoolSetting mGlow = BoolSetting("Glow", "Add glow effect", true);
    NumberSetting mGlowIntensity = NumberSetting("Glow Intensity", "Intensity of glow", 0.3f, 0.1f, 1.0f, 0.05f);

    HitEffects() : ModuleBase("HitEffects", "Visual effects when hitting entities", ModuleCategory::Visual, 0, false) {
        addSettings(
            // PRESET - FIRST!
            &mPreset,
            
            // Основные (visible only in Custom)
            &mStyle,
            &mDuration,
            &mThickness,
            
            // Анимации
            &mSpawnStyle,
            &mDespawnStyle,
            &mSpawnTime,
            &mDespawnTime,
            &mSpawnEasing,
            &mDespawnEasing,
            
            // Размеры
            &mDiamondSize,
            &mDiamondInnerSize,
            &mCornersSize,
            &mCornerLength,
            &mCornerGap,
            
            // Позиция
            &mPositionY,
            &mHeightPercent,
            
            // Вращение
            &mRotationMode,
            &mRotationSpeed,
            
            // Custom rotation
            &mCustomCycles,
            &mCustomPause,
            &mCustomSpeed,
            &mCustomEasing,
            
            // PingPong
            &mPingPongAngle,
            &mPingPongSpeed,
            
            // Wave
            &mWaveAmplitude,
            &mWaveFrequency,
            
            // Pulse
            &mPulseMinSpeed,
            &mPulseMaxSpeed,
            &mPulseCycleTime,
            
            // Цвет
            &mRainbow,
            &mRainbowSpeed,
            &mColorR,
            &mColorG,
            &mColorB,
            &mColorA,
            
            // Эффекты
            &mGlow,
            &mGlowIntensity
        );

        // === HIDE ALL SETTINGS WHEN PRESET != CUSTOM ===
        auto isCustom = [this]() { return mPreset.mValue == Preset::Custom; };
        
        VISIBILITY_CONDITION(mStyle, isCustom());
        VISIBILITY_CONDITION(mDuration, isCustom());
        VISIBILITY_CONDITION(mThickness, isCustom());
        VISIBILITY_CONDITION(mSpawnStyle, isCustom());
        VISIBILITY_CONDITION(mDespawnStyle, isCustom());
        VISIBILITY_CONDITION(mSpawnTime, isCustom());
        VISIBILITY_CONDITION(mDespawnTime, isCustom());
        VISIBILITY_CONDITION(mSpawnEasing, isCustom());
        VISIBILITY_CONDITION(mDespawnEasing, isCustom());
        VISIBILITY_CONDITION(mPositionY, isCustom());
        VISIBILITY_CONDITION(mHeightPercent, isCustom());
        VISIBILITY_CONDITION(mRotationMode, isCustom());
        VISIBILITY_CONDITION(mRainbow, isCustom());
        VISIBILITY_CONDITION(mGlow, isCustom());
        
        // Size settings: custom + correct style
        VISIBILITY_CONDITION(mDiamondSize, isCustom() && (mStyle.mValue == Style::Diamond || mStyle.mValue == Style::Both));
        VISIBILITY_CONDITION(mDiamondInnerSize, isCustom() && (mStyle.mValue == Style::Diamond || mStyle.mValue == Style::Both));
        VISIBILITY_CONDITION(mCornersSize, isCustom() && (mStyle.mValue == Style::Corners || mStyle.mValue == Style::Both));
        VISIBILITY_CONDITION(mCornerLength, isCustom() && (mStyle.mValue == Style::Corners || mStyle.mValue == Style::Both));
        VISIBILITY_CONDITION(mCornerGap, isCustom() && (mStyle.mValue == Style::Corners || mStyle.mValue == Style::Both));
        
        // Rotation sub-settings: custom + correct rotation mode
        VISIBILITY_CONDITION(mRotationSpeed, isCustom() && mRotationMode.mValue == RotationMode::Normal);
        VISIBILITY_CONDITION(mCustomCycles, isCustom() && mRotationMode.mValue == RotationMode::Custom);
        VISIBILITY_CONDITION(mCustomPause, isCustom() && mRotationMode.mValue == RotationMode::Custom);
        VISIBILITY_CONDITION(mCustomSpeed, isCustom() && mRotationMode.mValue == RotationMode::Custom);
        VISIBILITY_CONDITION(mCustomEasing, isCustom() && mRotationMode.mValue == RotationMode::Custom);
        VISIBILITY_CONDITION(mPingPongAngle, isCustom() && mRotationMode.mValue == RotationMode::PingPong);
        VISIBILITY_CONDITION(mPingPongSpeed, isCustom() && mRotationMode.mValue == RotationMode::PingPong);
        VISIBILITY_CONDITION(mWaveAmplitude, isCustom() && mRotationMode.mValue == RotationMode::Wave);
        VISIBILITY_CONDITION(mWaveFrequency, isCustom() && mRotationMode.mValue == RotationMode::Wave);
        VISIBILITY_CONDITION(mPulseMinSpeed, isCustom() && mRotationMode.mValue == RotationMode::Pulse);
        VISIBILITY_CONDITION(mPulseMaxSpeed, isCustom() && mRotationMode.mValue == RotationMode::Pulse);
        VISIBILITY_CONDITION(mPulseCycleTime, isCustom() && mRotationMode.mValue == RotationMode::Pulse);
        
        // Color sub-settings
        VISIBILITY_CONDITION(mRainbowSpeed, isCustom() && mRainbow.mValue);
        VISIBILITY_CONDITION(mColorR, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorG, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorB, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorA, isCustom());
        VISIBILITY_CONDITION(mGlowIntensity, isCustom() && mGlow.mValue);

        mNames = {
            {Lowercase, "hiteffects"},
            {LowercaseSpaced, "hit effects"},
            {Normal, "HitEffects"},
            {NormalSpaced, "Hit Effects"}
        };
    }

    // Только одна активная цель!
    std::optional<HitInfo> mActiveHit;
    std::mutex mHitMutex;
    Preset mLastPreset = Preset::Custom;

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void applyPreset(Preset preset);

private:
    void processHit(Actor* target);
    void updateState(HitInfo& hit);
    void updateRotation(HitInfo& hit, float deltaTime);
    void renderDiamond(const HitInfo& hit, const glm::vec2& screenPos, float scale, float alpha, ImColor color, float extraRotation);
    void renderCorners(const HitInfo& hit, const glm::vec2& screenPos, float scale, float alpha, ImColor color, float extraRotation);
    ImColor getColor(float timeOffset = 0.0f);
    
    // Easing функции
    float easeOutElastic(float t);
    float easeOutBounce(float t);
    float easeOutBack(float t);
    float easeOutExpo(float t);
    float easeInExpo(float t);
    float easeInOutCubic(float t);
    float easeInOutSine(float t);
    float easeOutCubic(float t);
    float easeInCubic(float t);
    
    // Функции анимации
    float getSpawnScale(float progress);
    float getSpawnAlpha(float progress);
    float getSpawnExtraRotation(float progress);
    float getDespawnScale(float progress);
    float getDespawnAlpha(float progress);
    float getDespawnExtraRotation(float progress);

    std::string getSettingDisplay() override {
        return mPreset.mValues[mPreset.as<int>()];
    }
};