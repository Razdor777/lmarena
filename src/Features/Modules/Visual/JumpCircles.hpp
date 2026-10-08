#pragma once

//
// Created by alteik on 26/10/2024.
// Rewritten: movement visuals — a clean expanding ground ring on jump/land,
// a soft dust burst, an optional world-anchored trail and ground shadows.
//

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

class JumpCircles : public ModuleBase<JumpCircles>
{
public:

    struct Circle {
        glm::vec3 position;
        float radius;
        float maxRadius;
        ImVec4 color;
        float glowAmount;
        float opacity;
        uint64_t startTime;
    };

    // One sampled point of the movement trail (world anchored, so it stays
    // where it was left instead of following the camera).
    struct TrailPoint {
        glm::vec3 pos;
        uint64_t time;
    };

    // Short lived dust particle used for the jump / landing burst.
    struct Puff {
        glm::vec3 pos;
        glm::vec3 vel;
        float size;
        float life;
        float maxLife;
    };

    enum class Style {
        Ring,       // One crisp expanding ring
        Ripple,     // A few concentric rings trailing the wave
        Shockwave,  // Fast, thin and bright with a wide glow
        Disc        // A translucent filled disc that fades out
    };

    enum class TrailStyle {
        Ribbon,   // One connected ribbon behind you
        Beads     // Separate glowing dots
    };

    // ---- rings ----
    // Defaults match the tuned look (Shockwave / 0.19 / 1.0 / 4 rings / 1.5
    // thickness / trail on) so a fresh config looks like the screenshots.
    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "How the circle is drawn", Style::Shockwave, "Ring", "Ripple", "Shockwave", "Disc");
    NumberSetting mSpeed = NumberSetting("Speed", "How fast the ring expands", 0.19f, 0.01f, 0.20f, 0.01f);
    NumberSetting mMaxRadius = NumberSetting("Max Radius", "Maximum ring radius", 1.0f, 0.5f, 6.0f, 0.1f);
    NumberSetting mRings = NumberSetting("Rings", "Number of concentric rings", 4, 1, 12, 1);
    NumberSetting mGlowAmount = NumberSetting("Glow", "Glow intensity", 30, 0, 100, 1);
    NumberSetting mOpacity = NumberSetting("Opacity", "Base opacity", 0.70f, 0.f, 1.f, 0.01f);
    NumberSetting mLifeTime = NumberSetting("Life time (ms)", "Time to render circle", 1400, 300, 10000, 100);
    NumberSetting mThickness = NumberSetting("Thickness", "Ring line thickness", 1.5f, 0.5f, 6.0f, 0.5f);
    BoolSetting   mFill = BoolSetting("Fill", "Fill the inside of the ring", true);

    // ---- trail ----
    BoolSetting   mTrail = BoolSetting("Trail", "Ribbon of light behind you while you move", true);
    EnumSettingT<TrailStyle> mTrailStyle = EnumSettingT<TrailStyle>("Trail Style", "How the trail is drawn",
        TrailStyle::Ribbon, "Ribbon", "Beads");
    NumberSetting mTrailLength = NumberSetting("Trail Length", "How long the trail lingers (seconds)", 1.25f, 0.2f, 3.f, 0.05f);
    NumberSetting mTrailWidth = NumberSetting("Trail Width", "Ribbon thickness", 2.4f, 0.5f, 8.f, 0.1f);

    // ---- shadow ----
    BoolSetting   mShadow = BoolSetting("Shadow", "Soft shadow on the ground under you", false);
    NumberSetting mShadowSize = NumberSetting("Shadow Size", "Shadow radius", 0.6f, 0.2f, 2.f, 0.05f);
    NumberSetting mShadowAlpha = NumberSetting("Shadow Alpha", "How dark the shadow is", 0.3f, 0.05f, 0.8f, 0.05f);

    // ---- extra ----
    BoolSetting mJumpBurst = BoolSetting("Jump Burst", "Dust burst when you leave the ground", true);
    BoolSetting mOthers = BoolSetting("Others", "Also draw ground shadows for other players", true);

    JumpCircles() : ModuleBase("JumpCircles", "Jump rings, movement trail and ground shadows", ModuleCategory::Visual, 0, false) {

        addSettings(
            &mStyle,
            &mSpeed,
            &mMaxRadius,
            &mRings,
            &mGlowAmount,
            &mOpacity,
            &mLifeTime,
            &mThickness,
            &mFill,
            &mTrail,
            &mTrailStyle,
            &mTrailLength,
            &mTrailWidth,
            &mShadow,
            &mShadowSize,
            &mShadowAlpha,
            &mJumpBurst,
            &mOthers
        );

        VISIBILITY_CONDITION(mTrailStyle, mTrail.mValue);
        VISIBILITY_CONDITION(mTrailLength, mTrail.mValue);
        VISIBILITY_CONDITION(mTrailWidth, mTrail.mValue);
        VISIBILITY_CONDITION(mShadowSize, mShadow.mValue);
        VISIBILITY_CONDITION(mShadowAlpha, mShadow.mValue);

        mNames = {
            {Lowercase, "jumpcircles"},
            {LowercaseSpaced, "jump circles"},
            {Normal, "JumpCircles"},
            {NormalSpaced, "Jump Circles"}
        };
    }

    std::vector<Circle> circles;
    std::vector<TrailPoint> trail;
    std::vector<Puff> puffs;

    uint64_t lastAddTime = 0;
    uint64_t lastTrailSample = 0;
    uint64_t lastBurst = 0;

    // Ground height the shadow is drawn at. Tracked while you are standing so
    // the shadow stays on the ground while you are in the air.
    float mGroundY = 0.f;
    bool  mGroundKnown = false;
    bool  mWasFalling = false;

    std::unordered_map<int64_t, float> mOtherGroundY;

    void addCircle(const glm::vec3& pos);
    void addBurst(const glm::vec3& pos, int count, float power);
    glm::vec3 feetPos(Actor* actor) const;
    void drawShadow(const glm::vec3& feet, float groundY, float alphaScale);
    void updatePuffs(float dt);
    void drawPuffs();

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        std::string r = mStyle.mValues[mStyle.as<int>()];
        if (mTrail.mValue)  r += " +T";
        if (mShadow.mValue) r += " +S";
        return r;
    }

private:
    void drawRing(const Circle& circle, float radius, float thickness, float alpha, const ImVec4& baseColor);
    std::vector<ImVec2> projectRing(const Circle& circle, float radius, std::vector<bool>& valid) const;
};
