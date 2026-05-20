#pragma once
//
// AmbientCubes - Floating ambient particle shapes
// Soft, small, non-intrusive visual effect
//

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <deque>
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
    int style; // 0=diamond, 1=ring, 2=dot, 3=star, 4=triangle
};

class AmbientCubes : public ModuleBase<AmbientCubes> {
public:
    enum class ParticleStyle {
        Mixed,
        Diamonds,
        Rings,
        Dots,
        Stars,
        Triangles
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
        "Mixed", "Diamonds", "Rings", "Dots", "Stars", "Triangles");
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

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    void spawnCube(const glm::vec3& playerPos);
    void updateCube(AmbientCube& cube, float deltaTime);
    void renderCube(const AmbientCube& cube);
    void renderShape(const glm::vec2& screenPos, float size,
                     const glm::vec3& rotation, float alpha,
                     ImColor color, int style);
    ImColor getColor();
};
