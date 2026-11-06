#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

class PearlStopper : public ModuleBase<PearlStopper> {
public:

    NumberSetting mStepDistance = NumberSetting(
        "Step Distance", "Blocks per TP step (same as ClickTP)", 8.f, 1.f, 12.f, 0.5f
    );

    NumberSetting mYOffset = NumberSetting(
        "Y Offset", "Extra Y offset at intercept point", 1.6f, -2.f, 2.f, 0.1f
    );

    NumberSetting mInterceptTicks = NumberSetting(
        "Intercept Ticks", "How far ahead to intercept (higher = further, more reliable)", 8.f, 3.f, 25.f, 1.f
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
            &mInterceptTicks,
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

    // ── Tracked pearl data ───────────────────────────────────────────────────
    struct TrackedPearl {
        int64_t   runtimeId = -1;
        glm::vec3 pos       = {};
        glm::vec3 vel       = {};      // from StateVectorComponent (blocks/tick)
        glm::vec3 prevPos   = {};      // position on previous tick
        bool      hasPrev   = false;   // do we have a valid previous position?
        int       age       = 0;       // ticks since first detected
    };

    // ── Intercept result ─────────────────────────────────────────────────────
    struct InterceptResult {
        bool      valid    = false;
        glm::vec3 feetPos  = {};
        int       tick     = -1;
        float     distance = 0.f;
    };

    // ── Module state ─────────────────────────────────────────────────────────
    bool      mIsFrozen      = false;
    int64_t   mLockedPearlId = 0;
    glm::vec3 mOriginalPos   = {};
    glm::vec3 mInterceptPos  = {};
    glm::vec3 mRots          = {};
    int       mFrozenTicks   = 0;
    int       mGoneTicks     = 0;       // ticks since locked pearl disappeared
    int       mTotalStops    = 0;

    // Saved player physics flags
    bool      mSavedCollision = true;
    bool      mSavedGravity   = true;
    bool      mSavedPush      = true;

    // Pearl tracking cache
    std::unordered_map<int64_t, TrackedPearl> mTrackedPearls;

    // Render data (protected by mRenderMutex)
    std::vector<glm::vec3> mPredictedPath;
    std::vector<glm::vec3> mTpPath;
    uint64_t               mLastRenderTime = 0;
    std::mutex             mRenderMutex;

    // ── Bedrock EnderPearl physics constants ─────────────────────────────────
    // Each tick: vel *= DRAG → vel.y -= GRAVITY → pos += vel
    static constexpr float DRAG              = 0.99f;
    static constexpr float GRAVITY           = 0.03f;
    static constexpr int   MAX_SIM_TICKS     = 300;
    static constexpr int   MAX_FROZEN_TICKS  = 400;
    static constexpr int   GONE_GRACE_TICKS  = 5;
    // MIN_INTERCEPT_TICK is now mInterceptTicks setting (default 8)
    static constexpr float PEARL_PLAYER_H   = 1.8f;

    // ── Event handlers ───────────────────────────────────────────────────────
    void onEnable()  override;
    void onDisable() override;

    void onBaseTick(BaseTickEvent& event);
    void onPacketIn(PacketInEvent& event);
    void onRender(RenderEvent& event);

    // ── Pearl detection ──────────────────────────────────────────────────────
    bool isEnderPearl(Actor* a);
    bool isOwnPearl(Actor* pearl, Actor* local);
    void updateTrackedPearls(Actor* local);

    // ── Velocity ─────────────────────────────────────────────────────────────
    glm::vec3 getReliableVelocity(TrackedPearl& pearl);

    // ── Trajectory simulation ────────────────────────────────────────────────
    std::vector<glm::vec3> simulateTrajectory(glm::vec3 pos, glm::vec3 vel);
    bool isSolidAt(glm::vec3 pos);

    // ── Intercept ────────────────────────────────────────────────────────────
    InterceptResult findBestIntercept(const std::vector<glm::vec3>& traj, glm::vec3 myPos);
    bool isSpaceClear(glm::vec3 feetPos);

    // ── Teleport (ClickTP-style) ─────────────────────────────────────────────
    std::shared_ptr<MovePlayerPacket> makePacket(glm::vec3 pos);
    void doStepTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
    void teleportTo(glm::vec3 dest);

    // ── Freeze management ────────────────────────────────────────────────────
    void freezeAt(Actor* player, glm::vec3 feetPos);
    void holdFreeze(Actor* player);
    void unfreeze(Actor* player);
    void fullReset(Actor* player);

    std::string getSettingDisplay() override {
        if (mIsFrozen) return "BLOCKING";
        if (mTotalStops > 0) return std::to_string(mTotalStops) + " stopped";
        return "Watching";
    }
};