#pragma once
//
// AmbientCubes — ambient particles that live in the WORLD, not on the camera.
//
// How it works now:
//   * the swarm is filled instantly and the population is kept FULL — a
//     particle that expires or is left behind is immediately reborn around
//     you, so the air is constantly dense instead of thinning out as you move,
//   * every particle is born at a world position and stays there; because the
//     world is the anchor a teleport, pearl or lagback never wipes the swarm,
//   * particles drift with a slow breeze that turns over time plus a per
//     particle sway, and can rise or fall, so nothing moves in a straight line,
//   * Count is a density, not a hard number: the count is scaled by how large
//     Radius is, so spreading them "across the world" also adds more of them.
//
// The body of a particle is configurable with Shape (orn / rounded square /
// spark / snowflake), and Size / Alpha / Glow / Trails / Twinkle tune the look.
//

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>
#include <deque>
#include <glm/glm.hpp>

struct AmbientCube {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 prevPosition;   // for the motion trail
    float size;
    float alpha;              // current (smoothed) alpha, 0..1
    float life;               // seconds left
    float maxLife;
    float phase;              // 0..1 life phase (birth pop / fade)
    float colorIndex;         // own shade inside the theme palette
    float swayPhase;          // per particle sway/twinkle phase
};

class AmbientCubes : public ModuleBase<AmbientCubes> {
public:
    enum class ColorMode {
        ThemeFlow,    // theme colour, every particle on its own shade (перелив)
        ThemeStatic,  // one theme colour for all of them
        Custom,       // your own colour
        Rainbow       // full spectrum over time
    };

    enum class Shape {
        Orb,      // soft glowing dot
        Square,   // rounded square droplet (the hit-particle look)
        Spark,    // streak of light, like a shooting star
        Snow      // soft dot with a faint flake cross
    };

    NumberSetting mCount  = NumberSetting("Count",  "Particle density (scaled by Radius)", 120, 10, 600, 5);
    NumberSetting mRadius = NumberSetting("Radius", "How far from you particles live and spawn", 32.f, 8.f, 160.f, 2.f);
    NumberSetting mSize   = NumberSetting("Size",   "Particle size",                       0.30f, 0.04f, 1.2f, 0.01f);
    NumberSetting mSpeed  = NumberSetting("Drift Speed", "How fast particles float around", 0.9f, 0.f, 3.f, 0.05f);
    NumberSetting mRise   = NumberSetting("Rise / Fall", "Slow upward (+) or downward (-) drift", 0.10f, -1.f, 1.f, 0.05f);

    EnumSettingT<ColorMode> mColorMode = EnumSettingT<ColorMode>(
        "Color Mode", "Where the particle colour comes from", ColorMode::ThemeFlow,
        "Theme Flow", "Theme Static", "Custom", "Rainbow");

    EnumSettingT<Shape> mShape = EnumSettingT<Shape>(
        "Shape", "What a particle looks like", Shape::Orb,
        "Orb", "Square", "Spark", "Snow");

    ColorSetting  mCustomColor = ColorSetting("Custom Color", "Your own particle colour", 0xFFADD8E6);
    NumberSetting mAlpha       = NumberSetting("Alpha", "Particle transparency", 0.65f, 0.05f, 1.f, 0.05f);
    BoolSetting   mGlow        = BoolSetting("Glow",    "Soft halo around every particle", true);
    BoolSetting   mTrails      = BoolSetting("Trails",  "Motion streak behind fast particles", true);
    BoolSetting   mTwinkle     = BoolSetting("Twinkle", "Each particle gently pulses in brightness", true);

    AmbientCubes() : ModuleBase("AmbientCubes", "Ambient particles all over the world around you",
                                ModuleCategory::Visual, 0, false) {
        gFeatureManager->mDispatcher->listen<RenderEvent, &AmbientCubes::onRenderEvent, nes::event_priority::NORMAL>(this);

        addSettings(&mCount, &mRadius, &mSize, &mSpeed, &mRise,
                    &mColorMode, &mShape, &mCustomColor, &mAlpha, &mGlow, &mTrails, &mTwinkle);

        VISIBILITY_CONDITION(mCustomColor, mColorMode.mValue == ColorMode::Custom);

        mNames = {
            {Lowercase, "ambientcubes"},
            {LowercaseSpaced, "ambient cubes"},
            {Normal, "AmbientCubes"},
            {NormalSpaced, "Ambient Cubes"}
        };
    }

    std::deque<AmbientCube> cubes;

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    // Anchor = what you actually see through (camera origin), eye-pos fallback.
    glm::vec3 getAnchorPos();

    // Actual particle count for the current settings (density * radius²).
    int wantedCount() const;

    AmbientCube makeCube(const glm::vec3& anchor);
    void respawn(AmbientCube& cube, const glm::vec3& anchor, float maxDist);
    void updateCube(AmbientCube& cube, float deltaTime, float timeSec);

    void renderCube(const AmbientCube& cube);
    ImColor getColor(float index);
};
