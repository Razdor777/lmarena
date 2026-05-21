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
        "Step Distance", "Blocks per TP step (same as ClickTP)", 8.f, 1.f, 12.f, 0.5f
    );

    NumberSetting mYOffset = NumberSetting(
        "Y Offset", "Extra Y offset at intercept point", 0.f, -2.f, 2.f, 0.1f
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
        "Teleport into the path of enemy ender pearls to stop them",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(
            &mStepDistance,
            &mYOffset,
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
        Actor*    actor     = nullptr;
        int64_t   runtimeId = -1;
        glm::vec3 position  = {};
        glm::vec3 velocity  = {};
    };

    struct InterceptResult {
        bool      found     = false;
        glm::vec3 feetPos   = {};
        int       tickIndex = -1;
    };

    bool      mIsActive         = false;
    bool      mIsFrozen         = false;
    int64_t   mTargetPearlId    = 0;
    glm::vec3 mOriginalPos      = {};
    glm::vec3 mInterceptFeetPos = {};
    glm::vec3 mRots             = {};
    int       mFrozenTicks      = 0;
    int       mMissedTicks      = 0;
    int       mTotalStops       = 0;

    bool      mSavedCollision   = true;
    bool      mSavedGravity     = true;
    bool      mSavedPush        = true;

    std::unordered_map<int64_t, glm::vec3>   mPearlPrevPos;
    std::unordered_map<int64_t, uint64_t>    mPearlPrevTime;
    // Кеш перл по runtimeId — не теряем перлы между тиками при лагах
    std::unordered_map<int64_t, PearlData>   mKnownPearls;

    std::vector<glm::vec3> mPredictedPath;
    std::vector<glm::vec3> mPacketPositions;
    uint64_t               mLastPathTime = 0;
    std::mutex             mMutex;

    // Bedrock EnderPearl physics: vel *= drag -> vel.y -= gravity -> pos += vel
    static constexpr float PEARL_DRAG       = 0.99f;
    static constexpr float PEARL_GRAVITY    = 0.03f;
    static constexpr int   MAX_SIM_TICKS    = 200;
    static constexpr int   MAX_FROZEN_TICKS = 300;
    static constexpr int   MISSED_TOLERANCE = 5;

    void onEnable()  override;
    void onDisable() override;

    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    bool   isEnderPearl(Actor* a);
    bool   isOwnPearl(Actor* pearl, Actor* local);
    Actor* getPearlOwner(Actor* pearl);
    bool   isPearlStillAlive(int64_t runtimeId);
    std::vector<PearlData> findEnemyPearls(Actor* local);

    glm::vec3 estimateVelocity(PearlData& pearl, uint64_t nowMs);

    std::vector<glm::vec3> simulateTrajectory(glm::vec3 startPos, glm::vec3 startVel, int maxTicks);

    InterceptResult findIntercept(const std::vector<glm::vec3>& traj, glm::vec3 ourFeetPos);

    bool isSolidAt(glm::vec3 pos);
    bool isSpaceClear(glm::vec3 feetPos);

    // Teleport logic — 1:1 copy of ClickTP
    std::shared_ptr<MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool save);
    void teleportTo(glm::vec3 dest);

    void freezeAt(Actor* player, glm::vec3 feetPos);
    void maintainFreeze(Actor* player);
    void unfreeze(Actor* player);
    void resetState(Actor* player);

    std::string getSettingDisplay() override {
        if (mIsFrozen) return "BLOCKING";
        if (mTotalStops > 0) return std::to_string(mTotalStops) + " stopped";
        return "Watching";
    }
};
