#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/LookInputEvent.hpp>

class Kagune : public ModuleBase<Kagune>
{
public:
    // ── Атака ─────────────────────────────────────────────────────────────
    enum class AttackMode { Alternate, Sting, Slap };
    EnumSettingT<AttackMode> mAttackMode = EnumSettingT<AttackMode>(
        "Attack Mode", "Slap = шлепок по дуге, Sting = укол", AttackMode::Alternate,
        "Alternate", "Sting", "Slap");

    // ── Кровь ─────────────────────────────────────────────────────────────
    enum class ParticleColor { DarkRed, BrightRed, Gradient };
    EnumSettingT<ParticleColor> mParticleColor = EnumSettingT<ParticleColor>(
        "Blood Color", "Blood particle color", ParticleColor::Gradient,
        "Dark Red", "Bright Red", "Gradient");

    // ── Палитра ───────────────────────────────────────────────────────────
    enum class Palette { Crimson, Blood, Ember, Custom };
    EnumSettingT<Palette> mPalette = EnumSettingT<Palette>(
        "Palette", "Color preset", Palette::Crimson,
        "Crimson", "Blood", "Ember", "Custom");

    // ── Параметры ─────────────────────────────────────────────────────────
    NumberSetting mLength    = NumberSetting("Length",       "Max tentacle length",         2.50f, 1.0f, 8.0f,  0.1f);
    NumberSetting mThickness = NumberSetting("Thickness",    "Tentacle size",               8.00f, 2.0f, 20.0f, 0.5f);
    NumberSetting mCount     = NumberSetting("Count",        "Number of tentacles",         4.00f, 1.0f, 8.0f,  1.0f);
    NumberSetting mSegments  = NumberSetting("Blocks",       "Blocks per tentacle",         12.0f, 6.0f, 24.0f, 1.0f);
    NumberSetting mAnimTime  = NumberSetting("Anim Time",    "Attack duration (sec)",       0.60f, 0.2f, 2.0f,  0.05f);
    NumberSetting mHideDelay = NumberSetting("Hide Delay",   "Auto-hide after (sec)",       12.0f, 3.0f, 60.0f, 1.0f);
    NumberSetting mSway      = NumberSetting("Sway",         "Idle motion amount",          1.00f, 0.0f, 2.0f,  0.1f);
    NumberSetting mShake     = NumberSetting("Camera Shake", "Shake on strike, degrees",   1.0f,  0.0f, 3.0f,  0.1f);
    NumberSetting mPulse     = NumberSetting("Pulse",        "Pulsing glow strength",       1.00f, 0.0f, 2.0f,  0.1f);
    NumberSetting mWidth     = NumberSetting("Blade Width",  "Block width multiplier",      1.00f, 0.4f, 2.0f,  0.1f);

    // ── Визуал ────────────────────────────────────────────────────────────
    BoolSetting mOutline = BoolSetting("Outline",  "Ink outline",                          true);
    BoolSetting mGlow    = BoolSetting("Glow",     "Blood particles",                      true);
    BoolSetting mShowFPV = BoolSetting("FPV Show", "Show in first person",                 true);
    BoolSetting mDetail  = BoolSetting("Detail",   "Glow bands + strike trail",            true);

    // ── Цвета (Palette = Custom) ──────────────────────────────────────────
    ColorSetting mColBlade = ColorSetting("Blade Color", "Flesh color",          0.52f, 0.03f, 0.06f, 1.0f);
    ColorSetting mColGlow  = ColorSetting("Glow Color",  "Glowing rings color",  1.00f, 0.22f, 0.12f, 1.0f);
    ColorSetting mColShade = ColorSetting("Shade Color", "Shadow / step color",  0.12f, 0.00f, 0.02f, 1.0f);

    // ── Новые настройки (в конце, чтобы не сдвигать старые) ───────────────
    BoolSetting   mSpikes   = BoolSetting("Spikes",        "Small spikes on the back of blocks", true);
    BoolSetting   mLegs     = BoolSetting("Spider Legs",   "Lower tentacles walk on the ground", true);
    BoolSetting   mExitAnim = BoolSetting("Exit Anim",     "Tentacles retract when module is disabled", true);
    NumberSetting mFpvSize  = NumberSetting("FPV Size",    "Tentacle size in first person", 0.65f, 0.3f, 1.2f, 0.05f);
    NumberSetting mYOffset  = NumberSetting("Height Offset", "Root height tweak (3rd person)", 0.0f, -1.0f, 1.0f, 0.05f);

    // ── Константы ─────────────────────────────────────────────────────────
    static constexpr int   kMax     = 8;
    static constexpr int   kNodes   = 14;     // узлов физики на щупальце
    static constexpr float kRootFrac = 0.53f; // высота корня от роста игрока
    static constexpr float kOffsetZ = 0.18f;  // корень на задней поверхности тела

    // ── Видимость ─────────────────────────────────────────────────────────
    uint64_t mEnableTime = 0;
    uint64_t mLastHit    = 0;
    bool     mWantShown  = false;
    float    mVis        = 0.f;
    bool     mListening  = false;
    bool     mClosing    = false;

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

    // ── Удары ─────────────────────────────────────────────────────────────
    struct HitRecord {
        uint64_t  hitTime = 0;
        glm::vec3 target  = {};
        glm::vec3 jit     = {};
        float     dur     = 0.6f;
        bool      isSlap  = false;
        uint8_t   var     = 0;
    };

    struct TrailPt { glm::vec3 p{}; float t = -100.f; };

    struct TentacleState {
        std::array<glm::vec3, kNodes> pos{};
        std::array<glm::vec3, kNodes> vel{};
        bool inited = false;

        HitRecord cur{};
        bool      hasActive  = false;
        bool      impactDone = false;
        uint32_t  hitCount   = 0;
        uint64_t  hardenTime = 0;

        // нога
        glm::vec3 foot{};
        glm::vec3 stepFrom{};
        bool      footInit = false;
        bool      stepping = false;
        float     stepT    = 0.f;

        std::array<TrailPt, 24> trail{};
        int trailHead = 0;
        void pushTrail(const glm::vec3& p, float t)
        {
            TrailPt tp; tp.p = p; tp.t = t;
            trail[trailHead] = tp;
            trailHead = (trailHead + 1) % 24;
        }
    };
    std::array<TentacleState, kMax> mTentacles{};

    // ── Планировщик ударов ────────────────────────────────────────────────
    struct PendingHit {
        bool      valid  = false;
        glm::vec3 target = {};
        uint64_t  time   = 0;
    };
    PendingHit mPending;
    uint64_t   mNextAttackAt   = 0;
    uint64_t   mLastPacketHit  = 0;
    float      mHitIntervalEma = 0.8f;
    int        mStriker        = 0;
    uint32_t   mHits           = 0;

    // ── Движение игрока ───────────────────────────────────────────────────
    bool      mWasOnGround     = true;
    bool      mJumped          = false;
    uint64_t  mAirStart        = 0;
    float     mAir             = 0.f;      // 0 на земле .. 1 в воздухе
    float     mPrevSpeed       = 0.f;
    float     mBrake           = 0.f;
    glm::vec3 mMoveDir         = {0.f, 0.f, 1.f};
    float     mSmoothedBodyYaw = -999.f;
    float     mSmY             = 0.f;
    bool      mSmYInit         = false;
    float     mGroundY         = 0.f;
    bool      mGroundInit      = false;
    int       mPosMode         = 0;        // 0 неизвестно, 1 getPos()=глаза, 2 getPos()=ноги

    // ── RNG ───────────────────────────────────────────────────────────────
    uint32_t mRng = 0x1234567u;
    float rndU() { mRng = mRng * 1664525u + 1013904223u; return float(mRng >> 8) / 16777216.f; }
    float rndS() { return rndU() * 2.f - 1.f; }

    // ── Тряска камеры ─────────────────────────────────────────────────────
    uint64_t  mShakeStart   = 0;
    float     mShakeAmp     = 0.f;
    glm::vec2 mShakeDir     = {1.f, 1.f};
    glm::vec2 mShakeApplied = {0.f, 0.f};

    Kagune() : ModuleBase("Kagune", "Tokyo Ghoul kagune cosmetic", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mAttackMode, &mParticleColor,
            &mLength, &mThickness, &mCount, &mSegments, &mAnimTime,
            &mHideDelay, &mSway,
            &mOutline, &mGlow, &mShowFPV,
            &mColBlade, &mColGlow, &mColShade,
            &mShake,
            &mPulse, &mWidth,
            &mPalette, &mDetail,
            &mSpikes, &mLegs, &mExitAnim, &mFpvSize, &mYOffset
        );

        VISIBILITY_CONDITION(mColBlade, mPalette.mValue == Palette::Custom);
        VISIBILITY_CONDITION(mColGlow,  mPalette.mValue == Palette::Custom);
        VISIBILITY_CONDITION(mColShade, mPalette.mValue == Palette::Custom);

        mNames = {
            {Lowercase,       "kagune"},
            {LowercaseSpaced, "kagune"},
            {Normal,          "Kagune"},
            {NormalSpaced,    "Kagune"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void unlisten();
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onLookInputEvent(LookInputEvent& event);
    void triggerShake(float power);
};
