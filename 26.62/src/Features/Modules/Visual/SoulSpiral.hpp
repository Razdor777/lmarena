#pragma once
//
// SoulSpiral - Spirit effect that spirals around hit entities
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <deque>

struct TrailPoint {
    glm::vec3 worldPos;
    float alpha;
    float size;
    uint64_t creationTime;
};

struct SoulInfo {
    Actor* target;
    int64_t runtimeId;
    uint64_t startTime;
    uint64_t lastHitTime;
    glm::vec3 lastTargetPos;
    float progress;           // 0-1, от ног до головы
    float currentAngle;       // Текущий угол спирали
    std::deque<TrailPoint> trail;  // След за душой
    bool isActive;
    float spawnScale;         // Для анимации появления
    float despawnAlpha;       // Для анимации исчезновения
    bool isDespawning;
};

class SoulSpiral : public ModuleBase<SoulSpiral>
{
public:
    enum class SoulStyle {
        Orb,            // Круглая сфера
        Flame,          // Огонёк
        Star,           // Звёздочка
        Diamond         // Ромбик
    };

    enum class TrailStyle {
        Solid,          // Сплошной след
        Dots,           // Точки
        Fading,         // Затухающий
        Smoke,          // Дымка
        None            // Без следа
    };

    enum class MovementStyle {
        SpiralUp,       // Спираль вверх
        SpiralDown,     // Спираль вниз
        SpiralBoth,     // Вверх потом вниз
        Orbit,          // Просто крутится на одной высоте
        Chaotic         // Хаотичное движение
    };

    // === ОСНОВНЫЕ НАСТРОЙКИ ===
    EnumSettingT<SoulStyle> mSoulStyle = EnumSettingT<SoulStyle>("Soul Style", "Shape of the soul", SoulStyle::Orb, "Orb", "Flame", "Star", "Diamond");
    EnumSettingT<TrailStyle> mTrailStyle = EnumSettingT<TrailStyle>("Trail Style", "Style of trail behind soul", TrailStyle::Fading, "Solid", "Dots", "Fading", "Smoke", "None");
    EnumSettingT<MovementStyle> mMovementStyle = EnumSettingT<MovementStyle>("Movement", "How the soul moves", MovementStyle::SpiralUp, "Spiral Up", "Spiral Down", "Spiral Both", "Orbit", "Chaotic");
    
    NumberSetting mDuration = NumberSetting("Duration", "How long effect lasts (seconds)", 2.0f, 0.5f, 10.0f, 0.1f);
    BoolSetting mLooping = BoolSetting("Looping", "Soul repeats animation while hitting", true);
    
    // === НАСТРОЙКИ ДУШИ ===
    NumberSetting mSoulSize = NumberSetting("Soul Size", "Size of the soul", 8.0f, 3.0f, 25.0f, 1.0f);
    NumberSetting mSoulGlow = NumberSetting("Soul Glow", "Glow intensity", 0.5f, 0.0f, 1.0f, 0.05f);
    NumberSetting mSoulPulse = NumberSetting("Soul Pulse", "Pulsing intensity", 0.3f, 0.0f, 1.0f, 0.05f);
    NumberSetting mSoulPulseSpeed = NumberSetting("Pulse Speed", "Speed of pulsing", 3.0f, 1.0f, 10.0f, 0.5f);
    
    // === НАСТРОЙКИ СПИРАЛИ ===
    NumberSetting mSpiralRadius = NumberSetting("Spiral Radius", "Radius of spiral", 0.8f, 0.2f, 2.0f, 0.1f);
    NumberSetting mSpiralSpeed = NumberSetting("Spiral Speed", "Rotation speed", 4.0f, 1.0f, 15.0f, 0.5f);
    NumberSetting mSpiralLoops = NumberSetting("Spiral Loops", "Number of loops", 3.0f, 1.0f, 10.0f, 0.5f);
    NumberSetting mStartHeight = NumberSetting("Start Height %", "Start position (0=feet)", 0.0f, -20.0f, 50.0f, 5.0f);
    NumberSetting mEndHeight = NumberSetting("End Height %", "End position (100=head)", 90.0f, 50.0f, 150.0f, 5.0f);
    
    // === НАСТРОЙКИ СЛЕДА ===
    NumberSetting mTrailLength = NumberSetting("Trail Length", "Length of trail", 15.0f, 5.0f, 50.0f, 1.0f);
    NumberSetting mTrailWidth = NumberSetting("Trail Width", "Width of trail", 4.0f, 1.0f, 15.0f, 0.5f);
    NumberSetting mTrailFade = NumberSetting("Trail Fade", "How fast trail fades", 0.8f, 0.1f, 1.0f, 0.05f);
    BoolSetting mTrailGlow = BoolSetting("Trail Glow", "Add glow to trail", true);
    
    // === НАСТРОЙКИ ЦВЕТА ===
    BoolSetting mRainbow = BoolSetting("Rainbow", "Use rainbow colors", false);
    NumberSetting mRainbowSpeed = NumberSetting("Rainbow Speed", "Speed of rainbow", 2.0f, 0.5f, 10.0f, 0.1f);
    NumberSetting mColorR = NumberSetting("Red", "Red component", 0.6f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorG = NumberSetting("Green", "Green component", 0.8f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorB = NumberSetting("Blue", "Blue component", 1.0f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorA = NumberSetting("Alpha", "Transparency", 0.9f, 0.1f, 1.0f, 0.01f);
    
    // === ВТОРИЧНЫЙ ЦВЕТ (для градиента) ===
    BoolSetting mUseGradient = BoolSetting("Gradient", "Use gradient colors", true);
    NumberSetting mColor2R = NumberSetting("Red 2", "Secondary red", 1.0f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColor2G = NumberSetting("Green 2", "Secondary green", 0.4f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColor2B = NumberSetting("Blue 2", "Secondary blue", 0.8f, 0.0f, 1.0f, 0.01f);
    
    // === АНИМАЦИИ ===
    NumberSetting mSpawnTime = NumberSetting("Spawn Time", "Spawn animation duration", 0.3f, 0.1f, 1.0f, 0.05f);
    NumberSetting mDespawnTime = NumberSetting("Despawn Time", "Despawn animation duration", 0.4f, 0.1f, 1.0f, 0.05f);

    SoulSpiral() : ModuleBase("SoulSpiral", "Spirit that spirals around hit entities", ModuleCategory::Visual, 0, false) {
        addSettings(
            // Основные
            &mSoulStyle,
            &mTrailStyle,
            &mMovementStyle,
            &mDuration,
            &mLooping,
            
            // Душа
            &mSoulSize,
            &mSoulGlow,
            &mSoulPulse,
            &mSoulPulseSpeed,
            
            // Спираль
            &mSpiralRadius,
            &mSpiralSpeed,
            &mSpiralLoops,
            &mStartHeight,
            &mEndHeight,
            
            // След
            &mTrailLength,
            &mTrailWidth,
            &mTrailFade,
            &mTrailGlow,
            
            // Цвет
            &mRainbow,
            &mRainbowSpeed,
            &mColorR,
            &mColorG,
            &mColorB,
            &mColorA,
            
            // Градиент
            &mUseGradient,
            &mColor2R,
            &mColor2G,
            &mColor2B,
            
            // Анимации
            &mSpawnTime,
            &mDespawnTime
        );

        // Visibility conditions
        VISIBILITY_CONDITION(mRainbowSpeed, mRainbow.mValue);
        VISIBILITY_CONDITION(mColorR, !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorG, !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorB, !mRainbow.mValue);
        
        VISIBILITY_CONDITION(mColor2R, mUseGradient.mValue && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColor2G, mUseGradient.mValue && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColor2B, mUseGradient.mValue && !mRainbow.mValue);
        
        VISIBILITY_CONDITION(mTrailLength, mTrailStyle.mValue != TrailStyle::None);
        VISIBILITY_CONDITION(mTrailWidth, mTrailStyle.mValue != TrailStyle::None);
        VISIBILITY_CONDITION(mTrailFade, mTrailStyle.mValue != TrailStyle::None);
        VISIBILITY_CONDITION(mTrailGlow, mTrailStyle.mValue != TrailStyle::None);
        
        VISIBILITY_CONDITION(mSoulPulseSpeed, mSoulPulse.mValue > 0.01f);
        
        VISIBILITY_CONDITION(mSpiralLoops, mMovementStyle.mValue != MovementStyle::Orbit);
        VISIBILITY_CONDITION(mStartHeight, mMovementStyle.mValue != MovementStyle::Orbit);
        VISIBILITY_CONDITION(mEndHeight, mMovementStyle.mValue != MovementStyle::Orbit);

        mNames = {
            {Lowercase, "soulspiral"},
            {LowercaseSpaced, "soul spiral"},
            {Normal, "SoulSpiral"},
            {NormalSpaced, "Soul Spiral"}
        };
    }

    std::optional<SoulInfo> mActiveSoul;
    std::mutex mSoulMutex;

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);

private:
    void processHit(Actor* target);
    void updateSoul(SoulInfo& soul, float deltaTime);
    glm::vec3 calculateSoulPosition(const SoulInfo& soul, const glm::vec3& targetPos, float hitboxHeight);
    void renderSoul(const SoulInfo& soul, const glm::vec2& screenPos, float scale, float alpha, ImColor color);
    void renderTrail(const SoulInfo& soul, float alpha);
    ImColor getColor(float progress, float timeOffset = 0.0f);
    ImColor lerpColor(ImColor a, ImColor b, float t);
    
    // Easing
    float easeOutCubic(float t);
    float easeInCubic(float t);
    float easeInOutSine(float t);
    float easeOutElastic(float t);
};