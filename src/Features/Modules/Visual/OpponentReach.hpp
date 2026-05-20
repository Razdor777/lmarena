#pragma once

#include "Features/Modules/Module.hpp"
#include "HudEditor.hpp"
#include <chrono>

class OpponentReach : public ModuleBase<OpponentReach>
{
public:
    NumberSetting mResetTime = NumberSetting("Reset Time", "Time in seconds before resetting", 15.0f, 1.0f, 30.0f, 1.0f);
    BoolSetting mExcludeTeam = BoolSetting("Exclude Team", "Try to exclude teammates", true);
    BoolSetting mDebug = BoolSetting("Debug", "Show debug info", false);

    OpponentReach() : ModuleBase<OpponentReach>("OpponentReach", "Shows opponent's reach when they hit you!", ModuleCategory::Visual, 0, false) {
        addSettings(&mResetTime, &mExcludeTeam, &mDebug);

        mNames = {
            {Lowercase, "opponentreach"},
            {LowercaseSpaced, "opponent reach"},
            {Normal, "OpponentReach"},
            {NormalSpaced, "Opponent Reach"}
        };

        mElement = std::make_unique<HudElement>();
        mElement->mPos = { 0, 110 };
        mElement->mAnchor = HudElement::Anchor::TopMiddle;
        mElement->mCentered = true;
        const char* typeId = ModuleBase<OpponentReach>::getTypeID();
        mElement->mParentTypeIdentifier = const_cast<char*>(typeId);
        mElement->mSize = glm::vec2(200, 24);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(mElement.get());
    }

    void onEnable() override;
    void onDisable() override;
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);

    std::unique_ptr<HudElement> mElement = nullptr;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastHit;
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastAttackSignal;
    float mOpponentReach = 0.f;
    int mHitCount = 0;
    std::string mAttackerName;
    int64_t mLastAttackerRuntimeId = -1;

    // Correct reach calculation: attacker eye → closest point on target AABB
    float calculateReachToAABB(Actor* attacker, Actor* target);
    float getLookAngleDeg(Actor* attacker, Actor* target);
    bool isOnSameTeam(Actor* player, Actor* other);
    bool isLookingAt(Actor* attacker, Actor* target, float maxAngleDeg = 60.f);
};