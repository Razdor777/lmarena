#pragma once
//
// AmbientCubes - Screen-space ambient particles
// Soft, subtle circles that float on screen — no world-space dependency.
// Always visible regardless of camera position or speed.
//

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <deque>
#include <glm/glm.hpp>

struct AmbientParticle {
    glm::vec2 position;       // screen-space position (pixels)
    glm::vec2 velocity;       // screen-space velocity (pixels/sec)
    float     size;           // radius in pixels
    float     alpha;
    float     lifetime;
    float     maxLifetime;
    float     pulsePhase;     // for gentle pulse animation
};

class AmbientCubes : public ModuleBase<AmbientCubes> {
public:
    enum class SpawnArea {
        Center,
        Edges,
        Random,
        Corners
    };

    // General
    NumberSetting mCount = NumberSetting("Count", "Maximum number of particles", 25, 5, 80, 1);
    NumberSetting mSpeed = NumberSetting("Speed", "Movement speed", 1.0f, 0.2f, 4.f, 0.1f);
    EnumSettingT<SpawnArea> mSpawnArea = EnumSettingT<SpawnArea>(
        "Spawn Area", "Where particles spawn on screen", SpawnArea::Random,
        "Center", "Edges", "Random", "Corners");

    // Visual
    NumberSetting mSize = NumberSetting("Size", "Particle radius (px)", 6.f, 2.f, 20.f, 0.5f);
    NumberSetting mSizeVariation = NumberSetting("Size Variation", "Random size variation", 0.5f, 0.f, 1.f, 0.1f);
    BoolSetting mPulse = BoolSetting("Pulse", "Gentle size pulse", true);
    NumberSetting mPulseSpeed = NumberSetting("Pulse Speed", "Pulse animation speed", 1.5f, 0.3f, 4.f, 0.1f);
    BoolSetting mGlow = BoolSetting("Glow", "Soft glow around particles", true);

    // Color
    BoolSetting mUseThemeColor = BoolSetting("Use Theme Color", "Use current GUI theme color", true);
    ColorSetting mCustomColor = ColorSetting("Custom Color", "Custom particle color", 0xFFADD8E6);
    NumberSetting mAlpha = NumberSetting("Alpha", "Transparency", 0.3f, 0.05f, 0.7f, 0.05f);

    // Lifetime
    NumberSetting mLifetime = NumberSetting("Lifetime", "How long particles last (seconds)", 12.f, 3.f, 30.f, 0.5f);
    BoolSetting mFadeInOut = BoolSetting("Fade In/Out", "Smooth fade effect", true);

    AmbientCubes() : ModuleBase("AmbientCubes", "Soft ambient particles on screen",
                                ModuleCategory::Visual, 0, false) {
        gFeatureManager->mDispatcher->listen<RenderEvent, &AmbientCubes::onRenderEvent, nes::event_priority::NORMAL>(this);

        addSettings(&mCount, &mSpeed, &mSpawnArea,
                    &mSize, &mSizeVariation, &mPulse, &mPulseSpeed, &mGlow,
                    &mUseThemeColor, &mCustomColor, &mAlpha,
                    &mLifetime, &mFadeInOut);

        VISIBILITY_CONDITION(mPulseSpeed, mPulse.mValue);
        VISIBILITY_CONDITION(mCustomColor, !mUseThemeColor.mValue);

        mNames = {
            {Lowercase, "ambientcubes"},
            {LowercaseSpaced, "ambient cubes"},
            {Normal, "AmbientCubes"},
            {NormalSpaced, "Ambient Cubes"}
        };
    }

    std::deque<AmbientParticle> particles;
    float spawnTimer = 0;

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    void spawnParticle(float screenW, float screenH);
    void updateParticle(AmbientParticle& p, float dt, float screenW, float screenH);
    void renderParticle(const AmbientParticle& p, ImColor color);
    ImColor getColor();
};
