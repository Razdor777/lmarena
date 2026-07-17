#pragma once
//
// AmbientCubes - Floating ambient glow particles
// Soft bokeh-style visuals that keep up with you:
//  - anchored to your view, instantly refill after teleports (pearls)
//  - fast flight no longer leaves the swarm behind
//

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <deque>
#include <cfloat>
#include <glm/glm.hpp>

struct AmbientCube {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    float size;
    float alpha;
    float lifetime;
    float maxLifetime;
    int style;      // 0=orb, 1=bokeh, 2=dust, 3=petal
    bool popIn;     // skip fade-in, appear instantly (teleports/recycles)
    float swayPhase; // per-particle phase for the gentle sway
};

class AmbientCubes : public ModuleBase<AmbientCubes> {
public:
    enum class ParticleStyle {
        Mixed,
        Orbs,
        Bokeh,
        Dust,
        Petals
    };

    enum class SpawnArea {
        Everywhere,
        Above,
        Front,
        Surround
    };

    // General
    NumberSetting mCubeCount = NumberSetting("Count", "Maximum number of particles", 20, 5, 60, 1);
    NumberSetting mSpawnRadius = NumberSetting("Spawn Radius", "How far particles spawn", 12.f, 5.f, 35.f, 1.f);
    NumberSetting mSpeed = NumberSetting("Speed", "Movement speed", 1.5f, 0.3f, 5.f, 0.1f);
    EnumSettingT<SpawnArea> mSpawnArea = EnumSettingT<SpawnArea>(
        "Spawn Area", "Where particles spawn", SpawnArea::Everywhere,
        "Everywhere", "Above", "Front", "Surround");

    // Visual
    EnumSettingT<ParticleStyle> mStyle = EnumSettingT<ParticleStyle>(
        "Style", "Shape of particles", ParticleStyle::Mixed,
        "Mixed", "Orbs", "Bokeh", "Dust", "Petals");
    NumberSetting mCubeSize = NumberSetting("Size", "Particle size", 0.25f, 0.05f, 0.8f, 0.05f);
    NumberSetting mSizeVariation = NumberSetting("Size Variation", "Random size variation", 0.4f, 0.f, 1.f, 0.1f);
    BoolSetting mGlow = BoolSetting("Glow", "Soft glow effect", true);

    // Rotation
    BoolSetting mRotate = BoolSetting("Rotate", "Enable rotation", true);
    NumberSetting mRotationSpeed = NumberSetting("Rotation Speed", "Rotation speed", 1.0f, 0.1f, 3.f, 0.1f);

    // Color
    BoolSetting mUseThemeColor = BoolSetting("Use Theme Color", "Use current GUI theme color", true);
    ColorSetting mCustomColor = ColorSetting("Custom Color", "Custom particle color", 0xFFADD8E6);
    NumberSetting mAlpha = NumberSetting("Alpha", "Transparency", 0.45f, 0.1f, 0.8f, 0.05f);

    // Lifetime
    NumberSetting mLifetime = NumberSetting("Lifetime", "How long particles last (seconds)", 10.f, 3.f, 25.f, 0.5f);
    BoolSetting mFadeInOut = BoolSetting("Fade In/Out", "Smooth fade effect", true);

    AmbientCubes() : ModuleBase("AmbientCubes", "Floating ambient particles around you",
                                ModuleCategory::Visual, 0, false) {
        gFeatureManager->mDispatcher->listen<RenderEvent, &AmbientCubes::onRenderEvent, nes::event_priority::NORMAL>(this);

        addSettings(&mCubeCount, &mSpawnRadius, &mSpeed, &mSpawnArea,
                    &mStyle, &mCubeSize, &mSizeVariation, &mGlow,
                    &mRotate, &mRotationSpeed,
                    &mUseThemeColor, &mCustomColor, &mAlpha,
                    &mLifetime, &mFadeInOut);

        VISIBILITY_CONDITION(mRotationSpeed, mRotate.mValue);
        VISIBILITY_CONDITION(mCustomColor, !mUseThemeColor.mValue);

        mNames = {
            {Lowercase, "ambientcubes"},
            {LowercaseSpaced, "ambient cubes"},
            {Normal, "AmbientCubes"},
            {NormalSpaced, "Ambient Cubes"}
        };
    }

    std::deque<AmbientCube> cubes;
    float spawnTimer = 0;
    glm::vec3 mLastAnchor = { FLT_MAX, FLT_MAX, FLT_MAX };

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    // Anchor = what you actually see through (camera origin), eye-pos fallback
    glm::vec3 getAnchorPos();
    // Fill the whole swarm right now (used on enable + after teleports)
    void reseedAll(const glm::vec3& anchor);
    AmbientCube makeCube(const glm::vec3& anchorPos, bool popIn);
    void spawnCube(const glm::vec3& anchorPos, bool popIn = false);
    void updateCube(AmbientCube& cube, float deltaTime, float timeSec);
    void renderCube(const AmbientCube& cube);
    void renderShape(const glm::vec2& screenPos, float size,
                     const glm::vec3& rotation, float alpha,
                     ImColor color, int style);
    ImColor getColor();
};
