#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/LookInputEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftSim.hpp>
#include <mutex>

class GhostMode : public ModuleBase<GhostMode>
{
public:
    NumberSetting mSpeed       = NumberSetting("Speed",         "Ghost fly speed",     5.5f, 0.1f, 10.f, 0.1f);
    NumberSetting mAttackRange = NumberSetting("Attack Range",  "Hit range from ghost",6.f,  2.f,  12.f, 0.5f);
    NumberSetting mStepDist    = NumberSetting("Step Distance", "TP step per packet",  8.f,  1.f,  12.f, 0.5f);
    BoolSetting   mCriticals   = BoolSetting("Criticals",      "Force crits",         true);
    BoolSetting   mIgnoreFriends = BoolSetting("Ignore Friends","Skip friends",       true);
    BoolSetting   mSilentAccept  = BoolSetting("Silent Accept", "Echo lagbacks",      true);
    BoolSetting   mDrawBody    = BoolSetting("Draw Body",      "Body at A",           true);
    BoolSetting   mDrawGhost   = BoolSetting("Draw Ghost",     "Ghost at hit pos",    true);
    BoolSetting   mDrawPath    = BoolSetting("Draw Path",      "Chain TP lines",      true);

    GhostMode() : ModuleBase("GhostMode",
        "Fly as ghost, click to hit from A via chain TP",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(&mSpeed, &mAttackRange, &mStepDist,
                    &mCriticals, &mIgnoreFriends, &mSilentAccept,
                    &mDrawBody, &mDrawGhost, &mDrawPath);
        mNames = {
            {Lowercase,       "ghostmode"},
            {LowercaseSpaced, "ghost mode"},
            {Normal,          "GhostMode"},
            {NormalSpaced,    "Ghost Mode"}
        };
    }

    // Point A — body (set ONCE on enable, never updated)
    glm::vec3 mBodyPos{};
    glm::vec3 mSvPosOld{};

    // Ghost absolute position (like Freecam Detached origin)
    glm::vec3 mOrigin{};
    glm::vec3 mOldOrigin{};

    glm::vec3 getGhostPos() { return mOrigin; }

    glm::vec3 getLerpedOrigin() {
        return MathUtils::lerp(mOldOrigin, mOrigin,
            ClientInstance::get()->getMinecraftSim()->getGameSim()->mDeltaTime);
    }

    // Rotation for chain TP packets (read from outgoing MovePlayer)
    glm::vec3 mRots{};

    // Camera rads (from LookInputEvent, for WASD direction)
    glm::vec2 mRotRads{};

    bool mWantsAttack = false;

    // Rendering
    std::vector<glm::vec3> mAttackPath;
    uint64_t               mLastPathTime = 0;
    glm::vec3              mAttackGhostPos{};
    bool                   mAttackGhostVisible = false;
    std::mutex             mMutex;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onLookInputEvent(LookInputEvent& event);
    void onMouseEvent(MouseEvent& event);
    void onRenderEvent(RenderEvent& event);

    std::string getSettingDisplay() override {
        return std::to_string((int)glm::distance(mOrigin, mBodyPos)) + "m";
    }
};
