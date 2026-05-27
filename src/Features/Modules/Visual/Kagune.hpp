#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>

class Kagune : public ModuleBase<Kagune>
{
public:
    // ── Атака ─────────────────────────────────────────────────────────────
    enum class AttackMode { Alternate, Sting, Slap };
    EnumSettingT<AttackMode> mAttackMode = EnumSettingT<AttackMode>(
        "Attack Mode", "Attack animation type", AttackMode::Alternate,
        "Alternate", "Sting", "Slap");

    // ── Частицы ───────────────────────────────────────────────────────────
    enum class ParticleColor { DarkRed, BrightRed, Gradient };
    EnumSettingT<ParticleColor> mParticleColor = EnumSettingT<ParticleColor>(
        "Blood Color", "Hemolymph particle color", ParticleColor::Gradient,
        "Dark Red", "Bright Red", "Gradient");

    // ── Основные параметры ────────────────────────────────────────────────
    NumberSetting mLength    = NumberSetting("Length",     "Max tentacle length", 2.50f, 1.0f,  8.0f,  0.1f);
    NumberSetting mThickness = NumberSetting("Thickness",  "Tentacle thickness",  8.00f, 2.0f,  20.0f, 0.5f);
    NumberSetting mCount     = NumberSetting("Count",      "Number of tentacles", 4.00f, 1.0f,  8.0f,  1.0f);
    NumberSetting mSegments  = NumberSetting("Segments",   "Spine segments",      16.0f, 4.0f,  20.0f, 1.0f);
    NumberSetting mRingRes   = NumberSetting("Ring Res",   "Tube sides",          7.00f, 3.0f,  10.0f, 1.0f);
    NumberSetting mAnimTime  = NumberSetting("Anim Time",  "Attack duration",     0.50f, 0.1f,  2.0f,  0.05f);
    NumberSetting mHideDelay = NumberSetting("Hide Delay", "Auto-hide (sec)",    20.00f, 10.0f, 60.0f, 1.0f);

    // ── Визуал ────────────────────────────────────────────────────────────
    BoolSetting mSpikes  = BoolSetting("Spikes",   "Bone crystal spikes",  true);
    BoolSetting mGlow    = BoolSetting("Glow",     "Blood particles",      true);
    BoolSetting mShowFPV = BoolSetting("FPV Show", "Show in first person", true);

    // ── Цвета ─────────────────────────────────────────────────────────────
    ColorSetting mColOuter = ColorSetting("Flesh Color", "Outer flesh", 0.45f, 0.02f, 0.03f, 1.0f);
    ColorSetting mColCore  = ColorSetting("Core Color",  "Inner core",  1.00f, 0.15f, 0.15f, 1.0f);
    ColorSetting mColSpike = ColorSetting("Spike Color", "Bone spikes", 0.85f, 0.78f, 0.72f, 1.0f);

    // ── Константы позиций ─────────────────────────────────────────────────
    static constexpr float kOffsetY   = -1.70f;
    static constexpr float kOffsetZ   =  0.10f;
    static constexpr float kSpreadX   =  0.20f;
    static constexpr float kUpperY    =  0.30f;
    static constexpr float kLowerY    = -0.05f;
    static constexpr float kHeightFac =  0.57f;

    uint64_t mEnableTime = 0;

    // ── Spawn/despawn ──────────────────────────────────────────────────────
    enum class SpawnState { Hidden, Spawning, Visible, Despawning };
    SpawnState mSpawnState      = SpawnState::Hidden;
    uint64_t   mSpawnStart      = 0;
    uint64_t   mLastHit         = 0;
    float      mDespawnProgress = 0.f;

    bool      mPendingHit    = false;
    glm::vec3 mPendingTarget = {};

    // ── Частицы ───────────────────────────────────────────────────────────
    struct Particle {
        glm::vec3 pos{};
        glm::vec3 vel{};
        float     life    = 0.f;
        float     maxLife = 1.f;
        float     size    = 3.f;
        ImColor   col     = ImColor(200, 10, 10, 255);
    };
    std::vector<Particle> mParticles;

    // ── Очередь ударов ────────────────────────────────────────────────────
    struct HitRecord {
        uint64_t  hitTime = 0;
        glm::vec3 target  = {};
        uint8_t   animId  = 0;
        bool      isSlap  = false;
        float     stretch = 1.f;
    };

    // ── Состояние щупальца ────────────────────────────────────────────────
    struct TentacleState {
        std::vector<HitRecord> hitQueue;
        HitRecord currentHit   = {};
        bool      hasActiveHit = false;

        glm::vec3 target     = {};
        uint8_t   animId     = 0;
        float     stretch    = 1.f;
        bool      isSlap     = false;
        uint32_t  hitCount   = 0;
        glm::vec3 restTip    = {};
        bool      restInited = false;
        float     stabAngleH = 0.f;
        float     stabAngleV = 0.f;

        uint64_t  hardenTime = 0;
        uint64_t  hitTime    = 0;
    };

    static constexpr int kMax = 8;
    std::array<TentacleState, kMax> mTentacles{};

    int      mStriker     = 0;
    int      mLastStriker = -1;
    uint32_t mHits        = 0;

    // ── Прыжок ────────────────────────────────────────────────────────────
    float    mJumpVel     = 0.f;
    bool     mWasOnGround = true;
    float    mJumpPush    = 0.f;
    uint64_t mJumpStart   = 0;

    // ── Сглаженный body yaw (вычисляем сами) ──────────────────────────────
    // -999.f = не инициализирован
    float mSmoothedBodyYaw = -999.f;

    Kagune() : ModuleBase("Kagune", "Tokyo Ghoul kagune cosmetic", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mAttackMode,
            &mParticleColor,
            &mLength, &mThickness, &mCount, &mSegments, &mRingRes, &mAnimTime,
            &mHideDelay,
            &mSpikes, &mGlow, &mShowFPV,
            &mColOuter, &mColCore, &mColSpike
        );

        mNames = {
            {Lowercase,       "kagune"},
            {LowercaseSpaced, "kagune"},
            {Normal,          "Kagune"},
            {NormalSpaced,    "Kagune"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
};