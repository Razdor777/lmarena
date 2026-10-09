#pragma once
#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Events/PreGameCheckEvent.hpp>

class Interface : public ModuleBase<Interface>
{
public:
    enum ColorTheme {
        Aurora,
        Abyss,
        Ember,
        Blossom,
        Prism,
        Spectrum = Prism,
        Rainbow = Prism
    };

    // Клиент грузит ровно один текстовый шрифт — Mntsb (см. FontHelper::load()).
    // Остальные шрифты удалены из resources: они дублировали кириллицу, ломали
    // вёрстку и раздували атлас. Nurik убран отсюда осознанно — это НЕ текстовый
    // шрифт, а набор иконок, поэтому он больше не выбирается как Font.
    enum class FontType {
        Mntsb,
    };

    EnumSettingT<NamingStyle> mNamingStyle = EnumSettingT<NamingStyle>("Naming", "The style of the module names.", NamingStyle::NormalSpaced, "lowercase", "lower spaced", "Normal", "Spaced");
    EnumSettingT<ColorTheme> mMode = EnumSettingT<ColorTheme>(
        "Theme", "Color palette used by the whole client.",
        Aurora, "Aurora", "Abyss", "Ember", "Blossom", "Prism");
    EnumSettingT<FontType> mFont = EnumSettingT<FontType>("Font", "The font of the interface.", FontType::Mntsb, "Mntsb");
    BoolSetting mGradientFlow = BoolSetting("Gradient Flow", "Smoothly morphs between colors in the selected palette", true);
    NumberSetting mColorSpeed = NumberSetting("Flow Speed", "Speed of the palette morphing", 2.4f, 0.1f, 10.f, 0.05f);
    NumberSetting mFlowDepth = NumberSetting("Flow Depth", "How strongly the moving gradient affects the base color", 0.82f, 0.f, 1.f, 0.02f);
    NumberSetting mSaturation = NumberSetting("Saturation", "Global saturation of themed accents", 0.92f, 0.f, 1.f, 0.01f);

    // ── Effects (what actually makes it look alive) ────────────────────────
    BoolSetting mColorWave = BoolSetting("Color Wave", "Waves the hue across elements in sync", true);
    NumberSetting mWaveSpeed = NumberSetting("Wave Speed", "Speed of the color wave", 1.0f, 0.1f, 5.0f, 0.05f);
    NumberSetting mWaveSpacing = NumberSetting("Wave Spacing", "How far apart the wave pattern is", 28.f, 5.f, 120.f, 1.f);
    BoolSetting mPulse = BoolSetting("Pulse", "Breathing brightness pulse on accents", true);
    NumberSetting mPulseSpeed = NumberSetting("Pulse Speed", "Speed of the brightness pulse", 2.0f, 0.5f, 8.0f, 0.1f);
    NumberSetting mPulseStrength = NumberSetting("Pulse Strength", "Depth of the brightness pulse", 0.25f, 0.f, 0.8f, 0.05f);
    BoolSetting mShimmer = BoolSetting("Shimmer", "Occasional bright sparkle sweeping through colors", true);
    NumberSetting mGlowBoost = NumberSetting("Glow Boost", "Multiplies glow strength of themed elements", 1.f, 0.2f, 2.f, 0.05f);
    BoolSetting mSlotEasing = BoolSetting("Slot Easing", "Eases the selection of slots", true);
    NumberSetting mSlotEasingSpeed = NumberSetting("Easing Speed", "The speed of the slot easing", 20.f, 0.1f, 20.f, 0.01f);
#ifdef __DEBUG__
    BoolSetting mForcePackSwitching = BoolSetting("Force Pack Switching", "Allows pack switching in-game", false);
#endif

    Interface() : ModuleBase("Interface", "Customize the visuals!", ModuleCategory::Visual, 0, true) {
        gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent, &Interface::onModuleStateChange, nes::event_priority::FIRST>(this);
        gFeatureManager->mDispatcher->listen<RenderEvent, &Interface::onRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<ActorRenderEvent, &Interface::onActorRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<BaseTickEvent, &Interface::onBaseTickEvent>(this);
        gFeatureManager->mDispatcher->listen<PacketOutEvent, &Interface::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
        gFeatureManager->mDispatcher->listen<DrawImageEvent, &Interface::onDrawImageEvent>(this);
        gFeatureManager->mDispatcher->listen<PreGameCheckEvent, &Interface::onPregameCheckEvent>(this);

        addSettings(
            &mNamingStyle,
            &mMode,
            &mFont,
            &mGradientFlow,
            &mColorSpeed,
            &mFlowDepth,
            &mSaturation,
            &mColorWave,
            &mWaveSpeed,
            &mWaveSpacing,
            &mPulse,
            &mPulseSpeed,
            &mPulseStrength,
            &mShimmer,
            &mGlowBoost,
            &mSlotEasing,
            &mSlotEasingSpeed
#ifdef __DEBUG__
            ,&mForcePackSwitching
#endif
        );

        VISIBILITY_CONDITION(mColorSpeed, mGradientFlow.mValue);
        VISIBILITY_CONDITION(mFlowDepth, mGradientFlow.mValue);
        VISIBILITY_CONDITION(mWaveSpeed, mColorWave.mValue);
        VISIBILITY_CONDITION(mWaveSpacing, mColorWave.mValue);
        VISIBILITY_CONDITION(mPulseSpeed, mPulse.mValue);
        VISIBILITY_CONDITION(mPulseStrength, mPulse.mValue);
        VISIBILITY_CONDITION(mSlotEasingSpeed, mSlotEasing.mValue);

        mNames = {
            {Lowercase, "interface"},
            {LowercaseSpaced, "interface"},
            {Normal, "Interface"},
            {NormalSpaced, "Interface"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void renderHoverText();
    void onModuleStateChange(ModuleStateChangeEvent& event);
    void onPregameCheckEvent(class PreGameCheckEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onActorRenderEvent(class ActorRenderEvent& event);
    void onDrawImageEvent(class DrawImageEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
};

class BodyYaw
{
public:
    static inline float bodyYaw = 0.f;
    static inline glm::vec3 posOld = glm::vec3(0, 0, 0);
    static inline glm::vec3 pos = glm::vec3(0, 0, 0);

    static inline void updateRenderAngles(Actor* plr, float headYaw)
    {
        posOld = pos;
        pos = *plr->getPos();
        float diffX = pos.x - posOld.x;
        float diffZ = pos.z - posOld.z;
        float diff = diffX * diffX + diffZ * diffZ;
        float body = bodyYaw;
        if (diff > 0.0025000002F)
        {
            float anglePosDiff = atan2f(diffZ, diffX) * 180.f / 3.14159265358979323846f - 90.f;
            float degrees = abs(wrapAngleTo180_float(headYaw) - anglePosDiff);
            if (95.f < degrees && degrees < 265.f)
                body = anglePosDiff - 180.f;
            else
                body = anglePosDiff;
        }
        turnBody(body, headYaw);
    };

    static inline void turnBody(float bodyRot, float headYaw)
    {
        float amazingDegreeDiff = wrapAngleTo180_float(bodyRot - bodyYaw);
        bodyYaw += amazingDegreeDiff * 0.3f;
        float bodyDiff = wrapAngleTo180_float(headYaw - bodyYaw);
        if (bodyDiff < -75.f) bodyDiff = -75.f;
        if (bodyDiff >= 75.f) bodyDiff = 75.f;
        bodyYaw = headYaw - bodyDiff;
        if (bodyDiff * bodyDiff > 2500.f)
            bodyYaw += bodyDiff * 0.2f;
    };

    static inline float wrapAngleTo180_float(float value)
    {
        value = fmodf(value, 360.f);
        if (value >= 180.0F) value -= 360.0F;
        if (value < -180.0F) value += 360.0F;
        return value;
    };
};
