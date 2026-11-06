#pragma once

#include <Features/Modules/Module.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

class Aimbot : public ModuleBase<Aimbot> {
public:
    NumberSetting mRange = NumberSetting("Range", "Maximum targeting range in blocks", 6.0f, 3.0f, 12.0f, 0.5f);
    NumberSetting mFOV = NumberSetting("FOV", "Field of view for targeting (degrees)", 60.0f, 10.0f, 180.0f, 5.0f);
    NumberSetting mHorizontalSpeed = NumberSetting("Horizontal Speed", "Horizontal aim speed (higher = faster)", 50.0f, 10.0f, 90.0f, 1.0f);
    NumberSetting mVerticalSpeed = NumberSetting("Vertical Speed", "Vertical aim speed (higher = faster)", 50.0f, 10.0f, 90.0f, 1.0f);
    BoolSetting mAimLock = BoolSetting("Aim Lock", "Instantly lock on target without smoothing", false);
    BoolSetting mVertical = BoolSetting("Vertical", "Also aim vertically (pitch)", true);
    NumberSetting mAimHeight = NumberSetting("Aim Height", "Where to aim on target (0=feet, 1=head)", 0.8f, 0.0f, 1.0f, 0.05f);
    BoolSetting mRequireClick = BoolSetting("Require Click", "Only aim while left clicking", false);

    BoolSetting mTargetPlayers = BoolSetting("Target Players", "Target players", true);
    BoolSetting mTargetMobs = BoolSetting("Target Mobs", "Target hostile mobs", false);
    BoolSetting mTargetAnimals = BoolSetting("Target Animals", "Target passive mobs", false);
    BoolSetting mIgnoreInvisible = BoolSetting("Ignore Invisible", "Don't target invisible entities", true);
    BoolSetting mIgnoreFriends = BoolSetting("Ignore Friends", "Don't target friends", true);
    BoolSetting mThroughWalls = BoolSetting("Through Walls", "Aim through walls", false);

    BoolSetting mVisuals = BoolSetting("Show FOV", "Show FOV circle on screen", true);
    NumberSetting mCircleOpacity = NumberSetting("Circle Opacity", "Opacity of the FOV circle", 100.0f, 10.0f, 255.0f, 5.0f);

    Aimbot() : ModuleBase("Aimbot", "Aims at enemies within FOV", ModuleCategory::Combat, 0, false) {
        addSettings(
            &mRange, &mFOV, &mHorizontalSpeed, &mVerticalSpeed,
            &mAimLock, &mVertical, &mAimHeight, &mRequireClick,
            &mTargetPlayers, &mTargetMobs, &mTargetAnimals,
            &mIgnoreInvisible, &mIgnoreFriends, &mThroughWalls,
            &mVisuals, &mCircleOpacity
        );

        VISIBILITY_CONDITION(mHorizontalSpeed, !mAimLock.mValue);
        VISIBILITY_CONDITION(mVerticalSpeed, !mAimLock.mValue);
        VISIBILITY_CONDITION(mCircleOpacity, mVisuals.mValue);

        mNames = {
            {Lowercase, "aimbot"},
            {LowercaseSpaced, "aim bot"},
            {Normal, "Aimbot"},
            {NormalSpaced, "Aim Bot"}
        };
    }

    static inline Actor* sCurrentTarget = nullptr;
    static inline bool sHasTarget = false;

    void onEnable() override;
    void onDisable() override;
    void onLookInputEvent(class LookInputEvent& event);
    void onRenderEvent(class RenderEvent& event);

    bool isEligibleTarget(Actor* actor, Actor* player);
    glm::vec3 getAimPoint(Actor* target);

    static glm::vec2 calcAngle(const glm::vec3& src, const glm::vec3& dst);
    static glm::vec2 normAngles(glm::vec2 angles);

    std::string getSettingDisplay() override {
        return sHasTarget ? "Locked" : "Idle";
    }
};