#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <mutex>
#include <unordered_map>

class PearlStopper : public ModuleBase<PearlStopper>
{
public:
    NumberSetting mStepDistance = NumberSetting(
        "Step Distance", "Blocks per TP packet", 8.f, 1.f, 12.f, 0.5f
    );
    // Высота удара перлы (1.4 = уровень груди. Ноги будут на 1.4 блока ниже траектории перлы)
    NumberSetting mYOffset = NumberSetting(
        "Hitbox Target Y", "Where pearl hits u (1.4 = chest)", 1.4f, 0.5f, 1.8f, 0.1f
    );
    BoolSetting mTeleportBack = BoolSetting(
        "Teleport Back", "Return after block", true
    );
    BoolSetting mSilentMode = BoolSetting(
        "Silent Mode", "Suppress lagbacks", true
    );
    BoolSetting mDrawPrediction = BoolSetting(
        "Draw Prediction", "Show pearl trajectory", true
    );
    BoolSetting mNotifications = BoolSetting(
        "Notifications", "Show notifications", true
    );

    PearlStopper() : ModuleBase(
        "PearlStopper",
        "Intercept enemy ender pearls",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(
            &mStepDistance,
            &mYOffset,
            &mTeleportBack,
            &mSilentMode,
            &mDrawPrediction,
            &mNotifications
        );

        mNames = {
            {Lowercase,       "pearlstopper"},
            {LowercaseSpaced, "pearl stopper"},
            {Normal,          "PearlStopper"},
            {NormalSpaced,    "Pearl Stopper"}
        };
    }

    struct PearlData {
        Actor*    actor = nullptr;
        int64_t   runtimeId = -1;
        glm::vec3 position = {};
        glm::vec3 velocity = {};
        glm::vec3 throwerPos = {};
    };

    struct InterceptResult {
        bool      found     = false;
        glm::vec3 position  = {};
        int       tickIndex = -1;
        float     distance  = FLT_MAX;
    };

    // State
    bool      mIsActive          = false;
    bool      mIsFrozen          = false;
    uint64_t  mTargetPearlId     = 0;
    glm::vec3 mOriginalPosition  = {};
    glm::vec3 mInterceptPosition = {};
    glm::vec3 mCurrentRotation   = {};
    int       mFrozenTicks       = 0;
    int       mTotalStops        = 0;
    int       mMissedTicks       = 0;
    bool      mSavedCollision    = true;
    bool      mSavedGravity      = true;
    bool      mSavedPush         = true;

    // Velocity tracking (last tick)
    std::unordered_map<int64_t, glm::vec3> mPearlLastPos;
    std::unordered_map<int64_t, uint64_t>  mPearlLastTime;

    // Render
    std::vector<glm::vec3> mPredictedPath;
    std::vector<glm::vec3> mTeleportPath;
    uint64_t               mLastRenderTime = 0;
    std::mutex             mRenderMutex;

    // Hardcoded constants
    static constexpr float PEARL_GRAVITY    = 0.03f;
    static constexpr float PEARL_DRAG       = 0.99f;
    static constexpr float TICK_TIME        = 0.05f;
    static constexpr int   MAX_FREEZE_TICKS = 300;
    static constexpr int   PREDICT_TICKS    = 150;
    static constexpr int   REACTION_TICKS   = 2;
    static constexpr int   PING_TICKS       = 2;
    static constexpr float MIN_DIST         = 3.0f;
    static constexpr float MAX_DIST         = 120.f;
    static constexpr float MARGIN_TICKS     = 1.5f;
    static constexpr int   MISSED_TOLERANCE = 3;

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent  (class RenderEvent&   event);

    // Block collision
    bool isSolidBlock(glm::vec3 pos);
    glm::vec3 ensureNotInBlock(glm::vec3 pos);

    std::vector<glm::vec3> simulateTrajectory(glm::vec3 pos, glm::vec3 velPerTick, int ticks);
    InterceptResult findInterceptPoint(const std::vector<glm::vec3>& trajectory,
                                       glm::vec3 playerPos, glm::vec3 throwerPos, float stepDist);

    std::vector<PearlData> findEnemyPearls(Actor* localPlayer);
    bool      isEnderPearl(Actor* actor);
    bool      isOwnPearl(Actor* pearl, Actor* localPlayer);
    Actor*    getPearlOwner(Actor* pearl);
    glm::vec3 getReliableVelocity(const PearlData& pearl);

    // ClickTp-style teleport
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
    void teleportTo(glm::vec3 dest);

    void freezePlayer(Actor* player);
    void maintainFreeze(Actor* player);
    void unfreezePlayer(Actor* player);
    void resetState(Actor* player);
    bool isPearlAlive(uint64_t pearlId, Actor* localPlayer);
    float calculateTeleportTime(float distance, float stepDist);

    std::string getSettingDisplay() override {
        if (mIsFrozen) return "BLOCKING!";
        if (mTotalStops > 0) return std::to_string(mTotalStops) + " stopped";
        return "Scanning";
    }
};