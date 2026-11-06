#pragma once
#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Events/PreGameCheckEvent.hpp>
#include <Features/Events/LevelRenderEvent.hpp>
//
// Created by vastrakai on 7/1/2024.
//


class Interface : public ModuleBase<Interface>
{
public:
    // Палитры из базы 1.21.44: Aurora/Abyss/Ember/Blossom/Prism. Раньше здесь
    // были Trans/Rainbow/Bubblegum/... — они рисовались статичными цветами,
    // теперь цвет живой градиент (см. ColorUtils::getThemedColor).
    enum ColorTheme {
        Aurora,
        Abyss,
        Ember,
        Blossom,
        Prism,
        Spectrum = Prism,
        Rainbow = Prism,
        // Custom остаётся последним: им пользуются VISIBILITY_CONDITION и
        // MathUtils::random(0, ColorTheme::Custom) ниже.
        Custom
    };

    // Клиент грузит ровно один текстовый шрифт — Mntsb (см. FontHelper::load()).
    // Остальные шрифты удалены из resources: они дублировали кириллицу, ломали
    // вёрстку и раздували атлас. Nurik сюда осознанно не входит: это набор
    // иконок, а не текстовый шрифт.
    enum class FontType {
        Mntsb,
    };

    ColorTheme randomTheme = (ColorTheme)MathUtils::random(0, ColorTheme::Custom);

    EnumSettingT<NamingStyle> mNamingStyle = EnumSettingT<NamingStyle>("Naming", "The style of the module names.", NamingStyle::NormalSpaced, "lowercase", "lower spaced", "Normal", "Spaced");
    EnumSettingT<ColorTheme> mMode = EnumSettingT<ColorTheme>("Theme", "Color palette used by the whole client.", Aurora, "Aurora", "Abyss", "Ember", "Blossom", "Prism");
    EnumSettingT<FontType> mFont = EnumSettingT<FontType>("Font", "The font of the interface.", FontType::Mntsb, "Mntsb");
    NumberSetting mColors = NumberSetting("Colors", "The amount of colors in the interface.", 3, 1, 6, 1);
    // Colors will be Fire-themed by default
    ColorSetting mColor1 = ColorSetting("Color 1", "The first color of the interface.", 0xFFFF0000);
    ColorSetting mColor2 = ColorSetting("Color 2", "The second color of the interface.", 0xFFFF7F00);
    ColorSetting mColor3 = ColorSetting("Color 3", "The third color of the interface.", 0xFFFFD600);
    ColorSetting mColor4 = ColorSetting("Color 4", "The fourth color of the interface.", 0xFF00FF00);
    ColorSetting mColor5 = ColorSetting("Color 5", "The fifth color of the interface.", 0xFF0000FF);
    ColorSetting mColor6 = ColorSetting("Color 6", "The sixth color of the interface.", 0xFF8B00FF);
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "The speed of the color change.", 3.f, 0.01f, 20.f, 0.01);
    // ── Живой градиент темы (перенесено из базы 1.21.44) ────────────────────
    // Палитра морфит между цветами, а не переключается: gradientFlow — дыхание,
    // colorWave — волна по элементам, pulse — яркость, shimmer — редкие блики,
    // glowBoost — сила свечения (её читает ModernDropdown для ореолов).
    BoolSetting mGradientFlow = BoolSetting("Gradient Flow", "Smoothly morphs between colors in the selected palette", true);
    NumberSetting mFlowDepth = NumberSetting("Flow Depth", "How strongly the moving gradient affects the base color", 0.82f, 0.f, 1.f, 0.02f);
    BoolSetting mColorWave = BoolSetting("Color Wave", "Waves the hue across elements in sync", true);
    NumberSetting mWaveSpeed = NumberSetting("Wave Speed", "Speed of the color wave", 1.0f, 0.1f, 5.0f, 0.05f);
    NumberSetting mWaveSpacing = NumberSetting("Wave Spacing", "How far apart the wave pattern is", 28.f, 5.f, 120.f, 1.f);
    BoolSetting mPulse = BoolSetting("Pulse", "Breathing brightness pulse on accents", true);
    NumberSetting mPulseSpeed = NumberSetting("Pulse Speed", "Speed of the brightness pulse", 2.0f, 0.5f, 8.0f, 0.1f);
    NumberSetting mPulseStrength = NumberSetting("Pulse Strength", "Depth of the brightness pulse", 0.25f, 0.f, 0.8f, 0.05f);
    BoolSetting mShimmer = BoolSetting("Shimmer", "Occasional bright sparkle sweeping through colors", true);
    NumberSetting mGlowBoost = NumberSetting("Glow Boost", "Multiplies glow strength of themed elements", 1.f, 0.2f, 2.f, 0.05f);
    NumberSetting mSaturation = NumberSetting("Saturation", "The saturation of the interface.", 1.f, 0.f, 1.f, 0.01);
    BoolSetting mHoveredItem = BoolSetting("Custom Hover", "Customizes the hovered item gui.", false);
    BoolSetting mSlotEasing = BoolSetting("Slot Easing", "Eases the selection of slots", true);
    NumberSetting mSlotEasingSpeed = NumberSetting("Easing Speed", "The speed of the slot easing", 20.f, 0.1f, 20.f, 0.01f);
#ifdef __DEBUG__
    BoolSetting mForcePackSwitching = BoolSetting("Force Pack Switching", "Allows pack switching in-game", false);
#endif
    //BoolSetting mShowRotations = BoolSetting("Show Rotations", "Shows normally invisible server-sided rotations", false);


    Interface() : ModuleBase("Interface", "Customize the visuals!", ModuleCategory::Visual, 0, true) {
        gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent, &Interface::onModuleStateChange, nes::event_priority::FIRST>(this);
        gFeatureManager->mDispatcher->listen<RenderEvent, &Interface::onRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<ActorRenderEvent, &Interface::onActorRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<LevelRenderEvent, &Interface::onLevelRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<BaseTickEvent, &Interface::onBaseTickEvent>(this);
        gFeatureManager->mDispatcher->listen<PacketOutEvent, &Interface::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
        gFeatureManager->mDispatcher->listen<DrawImageEvent, &Interface::onDrawImageEvent>(this);
        gFeatureManager->mDispatcher->listen<PreGameCheckEvent, &Interface::onPregameCheckEvent>(this);

        addSettings(
            &mNamingStyle,
            &mMode,
            &mFont,
            &mColors,
            &mColor1,
            &mColor2,
            &mColor3,
            &mColor4,
            &mColor5,
            &mColor6,
            &mColorSpeed,
            &mGradientFlow,
            &mFlowDepth,
            &mColorWave,
            &mWaveSpeed,
            &mWaveSpacing,
            &mPulse,
            &mPulseSpeed,
            &mPulseStrength,
            &mShimmer,
            &mGlowBoost,
            &mSaturation,
            //&mHoveredItem,
            &mSlotEasing,
            &mSlotEasingSpeed
#ifdef __DEBUG__
            ,&mForcePackSwitching
#endif
        );

        VISIBILITY_CONDITION(mColors, mMode.mValue == Custom);
        VISIBILITY_CONDITION(mColor1, mMode.mValue == Custom && mColors.mValue >= 1);
        VISIBILITY_CONDITION(mColor2, mMode.mValue == Custom && mColors.mValue >= 2);
        VISIBILITY_CONDITION(mColor3, mMode.mValue == Custom && mColors.mValue >= 3);
        VISIBILITY_CONDITION(mColor4, mMode.mValue == Custom && mColors.mValue >= 4);
        VISIBILITY_CONDITION(mColor5, mMode.mValue == Custom && mColors.mValue >= 5);
        VISIBILITY_CONDITION(mColor6, mMode.mValue == Custom && mColors.mValue >= 6);

        VISIBILITY_CONDITION(mSlotEasingSpeed, mSlotEasing.mValue);

        mNames = {
            {Lowercase, "interface"},
            {LowercaseSpaced, "interface"},
            {Normal, "Interface"},
            {NormalSpaced, "Interface"}
        };
    }

    static inline std::unordered_map<int, std::vector<ImColor>> ColorThemes = {
        {Trans,     {
            ImColor(91, 206, 250, 255),
            ImColor(245, 169, 184, 255),
            ImColor(255, 255, 255, 255),
            ImColor(245, 169, 184, 255),
    }},
{Rainbow,   {}},
{Bubblegum, {
    ImColor(255, 99, 202, 255),
    ImColor(255, 195, 195, 255),
    ImColor(146, 245, 255, 255),
    ImColor(249, 255, 148, 255),
    ImColor(135, 255, 176, 255),
}},
{Watermelon, {
    ImColor(255, 70, 70, 255),
    ImColor(139, 0, 0, 255),
    ImColor(144, 238, 144, 255),
    ImColor(34, 139, 34, 255),
    ImColor(204, 255, 204, 255),
}},
{Sunset, {
    ImColor(213,32,0, 255),
    ImColor(239, 118, 39, 255),
    ImColor(255, 154, 86, 255),
    ImColor(255, 255, 255, 255),
    ImColor(209, 98, 164, 255),
    ImColor(181, 86, 144, 255),
}},
{Poison, {
    ImColor(115,222,70, 255),
    ImColor(67, 201, 89, 255),
    ImColor(41, 230, 94, 255),
    ImColor(12, 210, 83, 255),
    ImColor(87, 211, 72, 255),
    ImColor(57, 210, 124, 255),
}},
{Custom,    {}}
    };

    std::vector<ImColor> getCustomColors() {
        auto result = std::vector<ImColor>();
        if (mColors.mValue >= 1) result.push_back(mColor1.getAsImColor());
        if (mColors.mValue >= 2) result.push_back(mColor2.getAsImColor());
        if (mColors.mValue >= 3) result.push_back(mColor3.getAsImColor());
        if (mColors.mValue >= 4) result.push_back(mColor4.getAsImColor());
        if (mColors.mValue >= 5) result.push_back(mColor5.getAsImColor());
        if (mColors.mValue >= 6) result.push_back(mColor6.getAsImColor());
        return result;
    }




    void onEnable() override;
    void onDisable() override;
    void renderHoverText();
    void onModuleStateChange(ModuleStateChangeEvent& event);
    void onPregameCheckEvent(class PreGameCheckEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onActorRenderEvent(class ActorRenderEvent& event);
    void onLevelRenderEvent(class LevelRenderEvent& event);
    void onDrawImageEvent(class DrawImageEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);

};
