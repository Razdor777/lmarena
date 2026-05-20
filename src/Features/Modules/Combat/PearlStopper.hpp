#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

class PearlStopper : public ModuleBase<PearlStopper> {
public:
    NumberSetting mStepDistance = NumberSetting(
        "Step Distance", "Blocks per TP packet (smaller = smoother but more packets)", 0.8f, 0.3f, 2.0f, 0.1f
    );

    NumberSetting mYOffset = NumberSetting(
        "Hitbox Target Y", "Where pearl should hit you (1.4 = chest level)", 1.4f, 0.8f, 1.9f, 0.1f
    );

    NumberSetting mReactionTicks = NumberSetting(
        "Reaction Ticks", "Extra ticks for reaction/ping", 3, 1, 8, 1
    );

    NumberSetting mPingTicks = NumberSetting(
        "Ping Ticks", "Extra ticks for ping compensation", 2, 0, 6, 1
    );

    BoolSetting mTeleportBack = BoolSetting(
        "Teleport Back", "Return to original position after block", true
    );

    BoolSetting mDrawPrediction = BoolSetting(
        "Draw Prediction", "Show pearl trajectory and TP path", true
    );

    BoolSetting mNotifications = BoolSetting(
        "Notifications", "Show block notifications", true
    );

    PearlStopper() : ModuleBase(
        "PearlStopper",
        "Reliably intercept enemy ender pearls by teleporting into their path",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(
            &mStepDistance,
            &mYOffset,
            &mReactionTicks,
            &mPingTicks,
            &mTeleportBack,
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
        Actor*    actor      = nullptr;
        int64_t   runtimeId  = -1;
        glm::vec3 position   = glm::vec3(0.f);
        glm::vec3 velocity   = glm::vec3(0.f);
        glm::vec3 throwerPos = glm::vec3(0.f);
        uint64_t  lastSeen   = 0;
    };

    struct InterceptResult {
        bool      found      = false;
        glm::vec3 position   = glm::vec3(0.f);
        int       tickIndex  = -1;
        float     distance   = 9999.f;
        float     timeNeeded = 0.f;
    };

    // State
    bool      mIsActive          = false;
    bool      mIsFrozen          = false;
    uint64_t  mTargetPearlId     = 0;
    glm::vec3 mOriginalPosition  = glm::vec3(0.f);
    glm::vec3 mInterceptPosition = glm::vec3(0.f);
    glm::vec3 mCurrentRotation   = glm::vec3(0.f);
    int       mFrozenTicks       = 0;
    int       mMissedTicks       = 0;
    int       mTotalStops        = 0;

    bool      mSavedCollision    = true;
    bool      mSavedGravity      = true;
    bool      mSavedPush         = true;

    // Velocity tracking (per pearl)
    std::unordered_map<int64_t, glm::vec3>   mPearlLastPos;
    std::unordered_map<int64_t, uint64_t>    mPearlLastTime;

    // Render paths
    std::vector<glm::vec3> mPredictedPath;
    std::vector<glm::vec3> mTeleportPath;
    uint64_t               mLastRenderTime = 0;
    std::mutex             mRenderMutex;

    // Constants (tuned for 1.21.44 Bedrock)
    static constexpr float PEARL_GRAVITY         = 0.03f;
    static constexpr float PEARL_DRAG            = 0.99f;
    static constexpr float TICK_TIME             = 0.05f;
    static constexpr int   MAX_FREEZE_TICKS      = 280;
    static constexpr int   PREDICT_TICKS         = 160;
    static constexpr int   REACTION_TICKS        = 3;
    static constexpr int   PING_TICKS            = 2;
    static constexpr float MIN_INTERCEPT_DIST    = 2.5f;
    static constexpr float MAX_INTERCEPT_DIST    = 90.f;
    static constexpr int   MISSED_TOLERANCE      = 4;

    void onEnable()  override;
    void onDisable() override;

    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    // Helpers
    bool isEnderPearl(Actor* actor);
    bool isOwnPearl(Actor* pearl, Actor* localPlayer);
    Actor* getPearlOwner(Actor* pearl);
    glm::vec3 getReliableVelocity(const PearlData& pearl, uint64_t now);

    std::vector<PearlData> findEnemyPearls(Actor* localPlayer);
    std::vector<glm::vec3> simulateTrajectory(glm::vec3 start, glm::vec3 vel, int maxTicks, bool checkCollision = true);

    InterceptResult findBestIntercept(const std::vector<glm::vec3>& trajectory,
                                      glm::vec3 playerPos, glm::vec3 throwerPos, float step);

    std::shared_ptr<MovePlayerPacket> createTeleportPacket(glm::vec3 pos, glm::vec3 rot);
    void straightLineTeleport(glm::vec3 from, glm::vec3 to, bool savePath = true);
    void teleportTo(glm::vec3 dest, bool savePath = true);

    void freezePlayer(Actor* player);
    void maintainFreeze(Actor* player);
    void unfreezePlayer(Actor* player);
    void resetState(Actor* player);

    std::string getSettingDisplay() override {
        if (mIsFrozen) return "BLOCKING PEARL";
        if (mTotalStops > 0) return std::to_string(mTotalStops) + " blocked";
        return "Scanning...";
    }

private:
    bool      mIsFrozen          = false;
    uint64_t  mTargetPearlId     = 0;
    glm::vec3 mOriginalPosition  = glm::vec3(0.f);
    glm::vec3 mInterceptPosition = glm::vec3(0.f);
    glm::vec3 mCurrentRotation   = glm::vec3(0.f);
    int       mFrozenTicks       = 0;
    int       mMissedTicks       = 0;
    int       mTotalStops        = 0;

    bool      mSavedCollision    = true;
    bool      mSavedGravity      = true;
    bool      mSavedPush         = true;

    std::unordered_map<int64_t, glm::vec3>   mPearlLastPos;
    std::unordered_map<int64_t, uint64_t>    mPearlLastTime;

    std::vector<glm::vec3> mPredictedPath;
    std::vector<glm::vec3> mTeleportPath;
    uint64_t               mLastRenderTime = 0;
    std::mutex             mRenderMutex;
};
