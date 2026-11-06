#pragma once
//
// Ported from Combatant Trails (visuals).
// Drawn in 2D on the ImGui background draw list. See Trails.cpp for dropped parts.
//

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

class Trails : public ModuleBase<Trails>
{
public:
    enum class TrailMode {
        Tail,
        Trail
    };

    enum class ColorMode {
        Static,
        Rainbow,
        DoubleColor,
        Theme
    };

    BoolSetting mOnlySelf = BoolSetting("Only Self", "Only draw your own trail", false);
    BoolSetting mHideFirstPerson = BoolSetting("Hide First Person", "Hide your own trail in first person", true);
    EnumSettingT<TrailMode> mTrailMode = EnumSettingT("Trail Mode", "Tail: glowing dots. Trail: ribbon", TrailMode::Tail, "Tail", "Trail");
    ColorSetting mColor = ColorSetting("Color", "Trail color", 0x8800FF00ULL);
    EnumSettingT<ColorMode> mColorMode = EnumSettingT("Color Mode", "How the trail is colored", ColorMode::Static, "Static", "Rainbow", "Double Color", "Theme");
    ColorSetting mColor2 = ColorSetting("Color 2", "Second color (Double Color mode)", 0xFF55FFFFULL);
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "Speed of animated colors", 18.f, 2.f, 54.f, 1.f);
    NumberSetting mColorPhase = NumberSetting("Color Phase", "Phase offset of animated colors", 0.f, 0.f, 360.f, 1.f);
    NumberSetting mColorLengthSpread = NumberSetting("Color Length Spread", "How much color changes along the trail", 180.f, -720.f, 720.f, 1.f);
    NumberSetting mColorAgeSpread = NumberSetting("Color Age Spread", "How much color changes with age", 90.f, -720.f, 720.f, 1.f);
    NumberSetting mLifeTimeTicks = NumberSetting("Life Time Ticks", "How long each trail point lasts", 10.f, 1.f, 40.f, 1.f);
    NumberSetting mScale = NumberSetting("Scale", "Size of tail dots", 0.6f, 0.2f, 1.2f, 0.01f);
    NumberSetting mTrailDown = NumberSetting("Trail Down", "Ribbon bottom below feet", 0.5f, 0.f, 2.f, 0.01f);
    NumberSetting mTrailHeight = NumberSetting("Trail Height", "Ribbon height", 1.3f, 0.1f, 2.f, 0.01f);
    BoolSetting mTrailStroke = BoolSetting("Trail Stroke", "Outline the ribbon edges", true);
    NumberSetting mTrailStrokeWidth = NumberSetting("Trail Stroke Width", "Outline thickness", 2.f, 0.5f, 8.f, 0.1f);
    NumberSetting mTrailStrokeAlpha = NumberSetting("Trail Stroke Alpha", "Outline opacity", 0.75f, 0.05f, 1.f, 0.01f);
    BoolSetting mConnectedPath = BoolSetting("Connected Path", "Fill gaps between tail dots", true);
    NumberSetting mConnectStep = NumberSetting("Connect Step", "Distance between filled dots", 0.35f, 0.1f, 1.f, 0.01f);

    BoolSetting mParticlesEnabled = BoolSetting("Particles", "Spawn sparks on walk, jump and projectiles", false);
    BoolSetting mParticleWalk = BoolSetting("Particle Walk", "Sparks while walking", true);
    BoolSetting mParticleJump = BoolSetting("Particle Jump", "Sparks when leaving the ground", true);
    NumberSetting mParticleWalkCount = NumberSetting("Particle Walk Count", "Sparks per step", 3.f, 1.f, 8.f, 1.f);
    NumberSetting mParticleJumpCount = NumberSetting("Particle Jump Count", "Sparks per jump", 12.f, 1.f, 30.f, 1.f);
    BoolSetting mParticleRandomColor = BoolSetting("Particle Random Color", "Random hue for sparks", false);
    NumberSetting mParticleSize = NumberSetting("Particle Size", "Spark radius", 0.12f, 0.04f, 0.4f, 0.01f);
    NumberSetting mParticleLifeMs = NumberSetting("Particle Life (ms)", "Spark lifetime", 1600.f, 200.f, 5000.f, 50.f);

    Trails() : ModuleBase("Trails", "Glowing trail behind players", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mOnlySelf,
            &mHideFirstPerson,
            &mTrailMode,
            &mColor,
            &mColorMode,
            &mColor2,
            &mColorSpeed,
            &mColorPhase,
            &mColorLengthSpread,
            &mColorAgeSpread,
            &mLifeTimeTicks,
            &mScale,
            &mTrailDown,
            &mTrailHeight,
            &mTrailStroke,
            &mTrailStrokeWidth,
            &mTrailStrokeAlpha,
            &mConnectedPath,
            &mConnectStep,
            &mParticlesEnabled,
            &mParticleWalk,
            &mParticleJump,
            &mParticleWalkCount,
            &mParticleJumpCount,
            &mParticleRandomColor,
            &mParticleSize,
            &mParticleLifeMs
        );

        VISIBILITY_CONDITION(mColor2, mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mColorSpeed, mColorMode.mValue == ColorMode::Rainbow || mColorMode.mValue == ColorMode::DoubleColor);
        VISIBILITY_CONDITION(mScale, mTrailMode.mValue == TrailMode::Tail);
        VISIBILITY_CONDITION(mConnectedPath, mTrailMode.mValue == TrailMode::Tail);
        VISIBILITY_CONDITION(mConnectStep, mTrailMode.mValue == TrailMode::Tail && mConnectedPath.mValue);
        VISIBILITY_CONDITION(mTrailDown, mTrailMode.mValue == TrailMode::Trail);
        VISIBILITY_CONDITION(mTrailHeight, mTrailMode.mValue == TrailMode::Trail);
        VISIBILITY_CONDITION(mTrailStroke, mTrailMode.mValue == TrailMode::Trail);
        VISIBILITY_CONDITION(mTrailStrokeWidth, mTrailMode.mValue == TrailMode::Trail && mTrailStroke.mValue);
        VISIBILITY_CONDITION(mTrailStrokeAlpha, mTrailMode.mValue == TrailMode::Trail && mTrailStroke.mValue);
        VISIBILITY_CONDITION(mParticleWalk, mParticlesEnabled.mValue);
        VISIBILITY_CONDITION(mParticleJump, mParticlesEnabled.mValue);
        VISIBILITY_CONDITION(mParticleWalkCount, mParticlesEnabled.mValue && mParticleWalk.mValue);
        VISIBILITY_CONDITION(mParticleJumpCount, mParticlesEnabled.mValue && mParticleJump.mValue);
        VISIBILITY_CONDITION(mParticleRandomColor, mParticlesEnabled.mValue);
        VISIBILITY_CONDITION(mParticleSize, mParticlesEnabled.mValue);
        VISIBILITY_CONDITION(mParticleLifeMs, mParticlesEnabled.mValue);

        mNames = {
            {Lowercase, "trails"},
            {LowercaseSpaced, "trails"},
            {Normal, "Trails"},
            {NormalSpaced, "Trails"}
        };
    }

    // One step of a player's movement (from previous tick to this tick)
    struct TrailPoint {
        glm::vec3 from = {0.f, 0.f, 0.f};
        glm::vec3 to = {0.f, 0.f, 0.f};
        uint32_t color = 0;
        int maxTicks = 1;
        int ticks = 1;
        int prevTicks = 1;
    };

    // Short-lived spark
    struct Particle {
        glm::vec3 pos = {0.f, 0.f, 0.f};
        glm::vec3 velocity = {0.f, 0.f, 0.f};
        uint64_t spawnedAtMs = 0;
        uint64_t lifeMs = 1;
        uint64_t fadeInMs = 0;
        uint64_t fadeOutMs = 0;
        float size = 0.1f;
        uint32_t color = 0;
    };

    std::unordered_map<int64_t, std::deque<TrailPoint>> mTrails = {};
    std::unordered_map<int64_t, glm::vec3> mLastPos = {};
    std::unordered_map<int64_t, bool> mGroundState = {};
    std::vector<Particle> mParticles = {};

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);

    void spawnParticlesAround(glm::vec3 feet, int count, float radius, float height, float motionScale, int colorIndex);
    void spawnParticle(glm::vec3 pos, float motionScale, int colorIndex);
};
