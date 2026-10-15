#pragma once
//
// Ported from Combatant WorldParticles (visuals).
// Drawn in 2D on the ImGui background draw list: sprites become glow dots,
// cubes become projected wireframes, bubbles and the funnel become rings.
//

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

#include <cstdint>
#include <vector>

class WorldParticles : public ModuleBase<WorldParticles>
{
public:
    enum class Mode {
        Quads,
        AirBubbles,
        Funnel
    };

    enum class ColorMode {
        Static,
        Rainbow,
        DoubleColor,
        Theme
    };

    EnumSettingT<Mode> mMode = EnumSettingT("Mode", "Quads: glow cubes. Air Bubbles: bubbles. Funnel: swirling funnel",
                                            Mode::Quads, "Quads", "Air Bubbles", "Funnel");
    ColorSetting mColor = ColorSetting("Color", "Particle color", 0xFF8ED4FFULL);
    EnumSettingT<ColorMode> mColorMode = EnumSettingT("Color Mode", "How particles are colored", ColorMode::Static,
                                                      "Static", "Rainbow", "Double Color", "Theme");
    ColorSetting mColor2 = ColorSetting("Color 2", "Second color (Double Color mode)", 0xFF55FFFFULL);
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "Speed of animated colors", 18.f, 2.f, 54.f, 1.f);
    NumberSetting mColorPhase = NumberSetting("Color Phase", "Phase offset of animated colors", 0.f, 0.f, 360.f, 1.f);
    NumberSetting mColorSpread = NumberSetting("Color Spread", "How much color varies between particles", 90.f, -720.f, 720.f, 1.f);
    NumberSetting mMaxParticles = NumberSetting("Max Particles", "Maximum number of particles alive", 100.f, 1.f, 400.f, 1.f);
    NumberSetting mSpawnPerTick = NumberSetting("Spawn Per Tick", "Particles spawned each tick", 1.f, 1.f, 20.f, 1.f);
    NumberSetting mRadius = NumberSetting("Radius", "Horizontal spawn radius", 20.f, 1.f, 64.f, 0.5f);
    NumberSetting mHeight = NumberSetting("Height", "Vertical spawn range above feet", 5.f, 0.5f, 16.f, 0.1f);
    NumberSetting mLifeMinMs = NumberSetting("Life Min (ms)", "Minimum lifetime", 1500.f, 100.f, 10000.f, 50.f);
    NumberSetting mLifeMaxMs = NumberSetting("Life Max (ms)", "Maximum lifetime", 4500.f, 100.f, 15000.f, 50.f);
    NumberSetting mSizeMin = NumberSetting("Size Min", "Minimum particle size", 0.10f, 0.02f, 1.0f, 0.01f);
    NumberSetting mSizeMax = NumberSetting("Size Max", "Maximum particle size", 0.30f, 0.02f, 1.5f, 0.01f);
    NumberSetting mLineWidth = NumberSetting("Line Width", "Cube outline thickness (Quads mode)", 1.0f, 0.5f, 4.0f, 0.1f);

    WorldParticles() : ModuleBase("WorldParticles", "Floating particles around you", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mMode,
            &mColor,
            &mColorMode,
            &mColor2,
            &mColorSpeed,
            &mColorPhase,
            &mColorSpread,
            &mMaxParticles,
            &mSpawnPerTick,
            &mRadius,
            &mHeight,
            &mLifeMinMs,
            &mLifeMaxMs,
            &mSizeMin,
            &mSizeMax,
            &mLineWidth
        );

        VISIBILITY_CONDITION(mColor2, mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mColorSpeed, mColorMode.mValue == ColorMode::Rainbow || mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mLineWidth, mMode.mValue == Mode::Quads);

        mNames = {
            {Lowercase, "worldparticles"},
            {LowercaseSpaced, "world particles"},
            {Normal, "WorldParticles"},
            {NormalSpaced, "World Particles"}
        };
    }

    struct Particle {
        glm::vec3 prevPos = {0.f, 0.f, 0.f};
        glm::vec3 pos = {0.f, 0.f, 0.f};
        glm::vec3 prevRotation = {0.f, 0.f, 0.f};
        glm::vec3 rotation = {0.f, 0.f, 0.f};
        glm::vec3 motion = {0.f, 0.f, 0.f};
        glm::vec3 rotationMotion = {0.f, 0.f, 0.f};
        uint64_t spawnMs = 0;
        uint64_t lifeMs = 1;
        float size = 0.1f;
        float visualPhase = 0.f;
        float eyeSpin = 0.5f;
        float distortionSpin = -0.2f;
    };

    std::vector<Particle> mParticles = {};

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);

    Particle createParticle(glm::vec3 feet);
};
