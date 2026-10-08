#pragma once
//
// Created by vastrakai on 7/23/2024.
//
// RobloxCamera — third person orbit camera with a smooth, rate-limited follow.
//
// The camera itself (mOrigin) is written by us inside LookInputEvent, so we own
// it for the frame. The rotation we follow is the one the game just computed
// into CameraDirectLookComponent::mRotRads — the same lever Aimbot / Scaffold /
// Freecam already use, so no reverse engineering is involved.
//
// Following is a critically damped spring (no rubber-band snap) plus a maximum
// swing speed, which is what keeps fast left/right flicks calm instead of
// throwing the camera across the screen.
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/EntityHurtEvent.hpp>
#include <glm/glm.hpp>

class RobloxCamera : public ModuleBase<RobloxCamera> {
public:
    // Smooth = slow cinematic wobble (barely a nudge), Punchy = short sharp
    // kick. Both are envelope-driven, so holding a fire tick cannot slam the
    // camera around any more.
    enum class ShakeMode { Off, Smooth, Punchy };

    // ---------------- base ----------------
    NumberSetting mRadius = NumberSetting("Radius", "How far the camera sits from you", 4.0f, 1.0f, 20.0f, 1.0f);
    BoolSetting mScroll = BoolSetting("Scroll Zoom", "Hold control and scroll to zoom", true);

    // ---------------- how it moves ----------------
    NumberSetting mSmoothness = NumberSetting("Smoothness", "How softly the camera follows the mouse\n0 = raw, 100 = very floaty", 60.0f, 0.0f, 100.0f, 1.0f);
    NumberSetting mTurnSpeed = NumberSetting("Turn Speed", "Maximum speed the orbit may swing, in degrees per second", 420.0f, 60.0f, 1440.0f, 10.0f);
    NumberSetting mTrail = NumberSetting("Body Trail", "How much the camera lags behind while you run and jump\n0 = rigidly glued to your eyes", 20.0f, 0.0f, 100.0f, 5.0f);

    // ---------------- extra looks ----------------
    NumberSetting mHeight = NumberSetting("Height", "Raise or lower the camera", 0.0f, -1.5f, 2.5f, 0.1f);
    NumberSetting mBob = NumberSetting("Walk Bob", "Camera bounce while you walk", 0.2f, 0.0f, 1.5f, 0.05f);

    // ---------------- damage shake ----------------
    EnumSettingT<ShakeMode> mShakeMode = EnumSettingT("Shake Mode",
        "Camera shake when you take damage\nSmooth = slow cinematic wobble, Punchy = short sharp kick",
        ShakeMode::Smooth, "Off", "Smooth", "Punchy");
    NumberSetting mShakeStrength = NumberSetting("Shake Strength", "How far the shake moves the camera", 28.f, 0.f, 100.f, 1.f);
    NumberSetting mShakeSpeed    = NumberSetting("Shake Speed",    "How quickly the shake wobbles", 4.5f, 1.f, 14.f, 0.5f);
    NumberSetting mShakeTime     = NumberSetting("Shake Time",     "Seconds the shake takes to fade out", 0.7f, 0.15f, 2.5f, 0.05f);
    NumberSetting mShakeTilt     = NumberSetting("Shake Tilt",     "How much the shake swings your view", 45.f, 0.f, 100.f, 1.f);

    RobloxCamera() : ModuleBase("RobloxCamera", "Change the camera to be like Roblox's camera", ModuleCategory::Visual, 0, false) {
        addSettings(&mRadius, &mScroll, &mSmoothness, &mTurnSpeed, &mTrail, &mHeight, &mBob,
                    &mShakeMode, &mShakeStrength, &mShakeSpeed, &mShakeTime, &mShakeTilt);

        VISIBILITY_CONDITION(mShakeStrength, mShakeMode.mValue != ShakeMode::Off);
        VISIBILITY_CONDITION(mShakeSpeed,    mShakeMode.mValue != ShakeMode::Off);
        VISIBILITY_CONDITION(mShakeTime,     mShakeMode.mValue != ShakeMode::Off);
        VISIBILITY_CONDITION(mShakeTilt,     mShakeMode.mValue != ShakeMode::Off);

        mNames = {
            {Lowercase, "robloxcamera"},
            {LowercaseSpaced, "roblox camera"},
            {Normal, "RobloxCamera"},
            {NormalSpaced, "Roblox Camera"}
        };

        gFeatureManager->mDispatcher->listen<BaseTickInitEvent, &RobloxCamera::onBaseTickInitEvent>(this);
        gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent, &RobloxCamera::onModuleStateChangeEvent>(this);
    }

    void onModuleStateChangeEvent(ModuleStateChangeEvent& event);
    void onBaseTickInitEvent(BaseTickInitEvent& event);

    bool mHasComponents = false;
    float mCurrentDistance = 4.f;

    void onEnable() override;
    void onDisable() override;
    void onActorRenderEvent(class ActorRenderEvent& event);
    void onMouseEvent(class MouseEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onLookInputEvent(class LookInputEvent& event);
    void onEntityHurtEvent(EntityHurtEvent& event);

    // ---- animation state ----
    glm::vec2 mOrbitRot{0.f, 0.f};   // smoothed orbit rotation we actually use (rad)
    glm::vec2 mOrbitVel{0.f, 0.f};   // spring velocity for the orbit rotation
    glm::vec3 mPivot{0.f};           // smoothed eye position (the orbit centre)
    glm::vec3 mPivotVel{0.f};        // spring velocity for the pivot
    glm::vec3 mLastEye{0.f};         // eye position of the previous frame
    float mDistVel = 0.f;            // spring velocity for the orbit distance
    float mClipVel = 0.f;            // spring velocity for the collision distance
    float mClippedDistance = 0.f;    // collision-governed orbit distance
    float mBobPhase = 0.f;
    float mBobAmp = 0.f;
    float mShakeLevel = 0.f;         // 0..1 shake envelope, decays over Shake Time
    bool mRotInit = false;
    bool mPivotInit = false;
    bool mCollisionInit = false;

    void resetAnimationState();
};
