#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <random>

class HitParticles : public ModuleBase<HitParticles> {
public:
    HitParticles();

    void onEnable() override;
    void onDisable() override;
    void onTick() override;

    void onEntityHurt(EntityHurtEvent& event);
    void onRenderEvent(RenderEvent& event);

private:
    struct DamageNumber {
        glm::vec3 mPos;
        float mDamage;
        float mAge = 0.f;
        bool mIsCrit = false;
        float mOffsetX = 0.f;
    };

    struct TrailPoint {
        glm::vec3 mPos;
        float mAge = 0.f;
    };

    struct HitParticle {
        glm::vec3 mPos;
        glm::vec3 mVel;
        float mAge = 0.f;
        float mSize;
        ImColor mColor;
    };

    std::vector<DamageNumber> mNumbers;
    std::vector<TrailPoint> mTrailPoints;
    std::vector<HitParticle> mParticles;
    std::mt19937 mRNG{std::random_device{}()};
    int mTrailTickCounter = 0;

    BoolSetting mDamageNumbers{"DamageNumbers", "Show floating damage", true};
    NumberSetting mRiseSpeed{"RiseSpeed", "How fast numbers float up", 0.4f, 0.1f, 2.0f, 0.05f};
    NumberSetting mDmgLifetime{"DmgLifetime", "Number lifetime (sec)", 1.5f, 0.3f, 5.0f, 0.1f};
    NumberSetting mScale{"Scale", "Text size multiplier", 1.0f, 0.3f, 3.0f, 0.1f};
    BoolSetting mShadow{"Shadow", "Draw text shadow", true};
    BoolSetting mColorCrits{"ColorCrits", "Color crits differently", true};
    ColorSetting mNormalColor{"NormalColor", "Normal hit color", 1.0f, 1.0f, 1.0f, 1.0f};
    ColorSetting mCritColor{"CritColor", "Critical hit color", 1.0f, 0.55f, 0.0f, 1.0f};

    BoolSetting mGhostTrail{"GhostTrail", "Ghostly trail behind actors", true};
    NumberSetting mTrailInterval{"TrailInterval", "Ticks between saves", 3.0f, 1.0f, 20.0f, 1.0f};
    NumberSetting mTrailLifetime{"TrailLifetime", "Trail duration (sec)", 3.0f, 0.5f, 10.0f, 0.5f};
    NumberSetting mTrailRadius{"TrailRadius", "Point radius", 3.0f, 1.0f, 8.0f, 0.5f};
    NumberSetting mTrailRange{"TrailRange", "Actor detection range", 15.0f, 5.0f, 50.0f, 1.0f};
    ColorSetting mTrailColor{"TrailColor", "Trail color", 0.4f, 0.75f, 1.0f, 0.7f};
    BoolSetting mOnlyPlayers{"OnlyPlayers", "Only trail players", true};
    NumberSetting mMaxTrailPoints{"MaxTrailPoints", "Max trail points", 500.0f, 100.0f, 2000.0f, 50.0f};

    BoolSetting mHitBurst{"HitBurst", "Particle burst on hit", true};
    NumberSetting mParticleCount{"ParticleCount", "Particles per hit", 8.0f, 1.0f, 30.0f, 1.0f};
    NumberSetting mParticleSpeed{"ParticleSpeed", "Particle speed", 0.3f, 0.05f, 1.0f, 0.05f};
    NumberSetting mParticleLifetime{"ParticleLifetime", "Particle lifetime (sec)", 0.8f, 0.2f, 3.0f, 0.1f};
    NumberSetting mParticleSize{"ParticleSize", "Particle size", 3.0f, 1.0f, 8.0f, 0.5f};
    ColorSetting mParticleColor{"ParticleColor", "Particle color", 1.0f, 0.3f, 0.3f, 0.9f};
    ColorSetting mParticleCritColor{"ParticleCritColor", "Crit particle color", 1.0f, 0.85f, 0.0f, 1.0f};

    NumberSetting mMaxDist{"MaxDist", "Max render distance", 20.0f, 5.0f, 50.0f, 1.0f};

    void spawnDamageNumber(Actor* entity, float damage, bool isCrit);
    void spawnHitBurst(const glm::vec3& pos, bool isCrit);
    void updateTrail();
    void renderNumbers(const glmatrixf& mat, const glm::vec2& fov, const glm::vec2& screenSize, const glm::vec3& origin);
    void renderTrail(const glmatrixf& mat, const glm::vec2& fov, const glm::vec2& screenSize, const glm::vec3& origin);
    void renderParticles(const glmatrixf& mat, const glm::vec2& fov, const glm::vec2& screenSize, const glm::vec3& origin);
};