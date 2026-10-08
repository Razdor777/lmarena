//
// Created by vastrakai on 7/23/2024.
//

#include "RobloxCamera.hpp"

#include <algorithm>
#include <cmath>

#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/LookInputEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/CameraComponent.hpp>
#include <SDK/Minecraft/Actor/Components/FlagComponent.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>

namespace
{
    constexpr float kPi = 3.14159265f;

    float wrapAngle(float a)
    {
        while (a >  kPi) a -= 2.f * kPi;
        while (a < -kPi) a += 2.f * kPi;
        return a;
    }

    // Exponential smoothing, frame-rate independent. Used for the small stuff
    // (bob amplitude, shake blend) where a spring would be overkill.
    float toward(float cur, float target, float speed, float dt)
    {
        const float k = 1.f - std::exp(-(std::max)(0.01f, speed) * dt);
        return cur + (target - cur) * k;
    }

    // Critically damped spring (Unity's SmoothDamp). Unlike a plain lerp it also
    // controls velocity, so it eases *in* as well as out — no rubber-band snap
    // at the start of a flick, no hard stop at the end.
    float smoothDamp(float cur, float target, float& vel, float smoothTime, float dt)
    {
        smoothTime = (std::max)(smoothTime, 0.0001f);
        const float omega   = 2.f / smoothTime;
        const float x       = omega * dt;
        const float expTerm = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);

        const float change = cur - target;
        const float temp   = (vel + omega * change) * dt;
        vel = (vel - omega * temp) * expTerm;
        return target + (change + temp) * expTerm;
    }

    // Same spring, but shortest way around the circle.
    float smoothDampAngle(float cur, float target, float& vel, float smoothTime, float dt)
    {
        return smoothDamp(cur, cur + wrapAngle(target - cur), vel, smoothTime, dt);
    }

    glm::vec3 smoothDampVec(const glm::vec3& cur, const glm::vec3& target, glm::vec3& vel, float smoothTime, float dt)
    {
        return { smoothDamp(cur.x, target.x, vel.x, smoothTime, dt),
                 smoothDamp(cur.y, target.y, vel.y, smoothTime, dt),
                 smoothDamp(cur.z, target.z, vel.z, smoothTime, dt) };
    }

    bool finite3(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    // Unit vector pointing from the player towards the camera (the orbit arm).
    glm::vec3 orbitDirection(float yawRad, float pitchRad)
    {
        const float pitch = -pitchRad;   // convention this module always used
        return {
            std::cos(pitch) * std::sin(yawRad),
            std::sin(pitch),
            std::cos(pitch) * std::cos(yawRad)
        };
    }
}

void RobloxCamera::resetAnimationState()
{
    mOrbitRot = glm::vec2(0.f);
    mOrbitVel = glm::vec2(0.f);
    mPivot = glm::vec3(0.f);
    mPivotVel = glm::vec3(0.f);
    mLastEye = glm::vec3(0.f);
    mDistVel = 0.f;
    mClipVel = 0.f;
    mClippedDistance = 0.f;
    mBobPhase = 0.f;
    mBobAmp = 0.f;
    mShakeLevel = 0.f;
    mRotInit = false;
    mPivotInit = false;
    mCollisionInit = false;
}

void RobloxCamera::onModuleStateChangeEvent(ModuleStateChangeEvent& event)
{
    if (event.mModule == this && !mHasComponents)
    {
        NotifyUtils::notify("RobloxCamera: Failed to initialize required components :(", 5.f, Notification::Type::Error);
        event.cancel();
    }
}

void RobloxCamera::onBaseTickInitEvent(BaseTickInitEvent& event)
{
    if (!TRY_CALL([&](BaseTickInitEvent& event) {
        auto actor = event.mActor;
        TRY_CALL(actor->getFlag<RenderCameraComponent>);
        TRY_CALL(actor->getFlag<OnGroundFlagComponent>);
        TRY_CALL(actor->getFlag<WasOnGroundFlagComponent>);
        TRY_CALL(actor->getFlag<RenderCameraComponent>);
        TRY_CALL(actor->getFlag<GameCameraComponent>);
        TRY_CALL(actor->getFlag<OnFireComponent>);
        TRY_CALL(actor->getFlag<MoveRequestComponent>);
        actor->getFlag<CameraRenderPlayerModelComponent>();
        spdlog::info("[RobloxCamera] Initialized required components :3");
        mHasComponents = true;
    }, event))
    {
        spdlog::error("[RobloxCamera] Failed to initialize required components :(");
        NotifyUtils::notify("RobloxCamera: Failed to initialize required components :(", 15.f, Notification::Type::Error);
        mHasComponents = false;
    }
}

void RobloxCamera::onEnable()
{
    if (!mHasComponents)
    {
        NotifyUtils::notify("RobloxCamera: Failed to initialize required components :(", 15.f, Notification::Type::Error);
        this->disable();
        return;
    }

    gFeatureManager->mDispatcher->listen<LookInputEvent, &RobloxCamera::onLookInputEvent>(this);
    gFeatureManager->mDispatcher->listen<MouseEvent, &RobloxCamera::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<ActorRenderEvent, &RobloxCamera::onActorRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &RobloxCamera::onBaseTickEvent, nes::event_priority::LAST>(this);
    gFeatureManager->mDispatcher->listen<EntityHurtEvent, &RobloxCamera::onEntityHurtEvent>(this);

    resetAnimationState();

    auto* ci = ClientInstance::get();
    auto* player = ci ? ci->getLocalPlayer() : nullptr;
    if (player && player->isValid())
    {
        player->setFlag<RenderCameraComponent>(true);
        player->setFlag<CameraRenderPlayerModelComponent>(true);
    }
}

void RobloxCamera::onDisable()
{
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &RobloxCamera::onLookInputEvent>(this);
    gFeatureManager->mDispatcher->deafen<MouseEvent, &RobloxCamera::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<ActorRenderEvent, &RobloxCamera::onActorRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &RobloxCamera::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<EntityHurtEvent, &RobloxCamera::onEntityHurtEvent>(this);

    auto* ci = ClientInstance::get();
    auto* player = ci ? ci->getLocalPlayer() : nullptr;
    if (player && player->isValid())
    {
        player->setFlag<RenderCameraComponent>(false);
        player->setFlag<CameraRenderPlayerModelComponent>(false);
    }

    mCurrentDistance = 0.f;   // next enable glides back out from the player
    resetAnimationState();
}

void RobloxCamera::onActorRenderEvent(ActorRenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    auto* player = ci->getLocalPlayer();
    if (!player || !player->isValid()) return;

    if (mRadius.mValue != 0.f)
    {
        player->setFlag<RenderCameraComponent>(true);
        player->setFlag<CameraRenderPlayerModelComponent>(true);

        if (event.mEntity != player) return;
        if (*event.mPos == glm::vec3(0.f, 0.f, 0.f) && *event.mRot == glm::vec2(0.f, 0.f))
            event.cancel();
    }
}

void RobloxCamera::onMouseEvent(MouseEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    if (ci->getMouseGrabbed()) return;
    if (!mScroll.mValue) return;
    if (!Keyboard::mPressedKeys[VK_CONTROL]) return;

    if (event.mActionButtonId == 4)
    {
        if (event.mButtonData == 0x78 || event.mButtonData == 0x7F)
        {
            mRadius.mValue -= 1.f;
            event.cancel();
        }
        else if (event.mButtonData == 0x88 || event.mButtonData == 0x80 || event.mButtonData == -0x78)
        {
            mRadius.mValue += 1.f;
            event.cancel();
        }
    }

    mRadius.mValue = std::clamp(mRadius.mValue, 1.f, 20.f);
}

void RobloxCamera::onBaseTickEvent(BaseTickEvent& event)
{
    if (mRadius.mValue == 0.f) return;

    auto* player = event.mActor;
    if (!player || !player->isValid()) return;

    // The held item is hidden while the orbit camera is up. getSupplies() is only
    // meaningful on a player, so it is checked before the write.
    auto* supplies = player->getSupplies();
    if (!supplies) return;

    supplies->mInHandSlot = -1;
}

void RobloxCamera::onEntityHurtEvent(EntityHurtEvent& event)
{
    if (!event.mEntity) return;
    if (mShakeMode.mValue == ShakeMode::Off) return;
    if (mShakeStrength.mValue <= 0.f) return;

    auto* ci = ClientInstance::get();
    auto* player = ci ? ci->getLocalPlayer() : nullptr;
    if (!player || event.mEntity != player) return;

    // Additive, not a reset. Standing in fire fires a hurt event every tick,
    // and resetting the envelope to full each time is exactly what made the
    // camera thrash while burning. Adding a fraction keeps it at a steady,
    // readable wobble instead.
    mShakeLevel = (std::min)(1.f, mShakeLevel + 0.55f);
}

void RobloxCamera::onLookInputEvent(LookInputEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    // Third person is forced off — this module draws the orbit itself.
    if (auto* options = ci->getOptions())
        if (options->mThirdPerson) options->mThirdPerson->value = 0;

    auto* player = ci->getLocalPlayer();
    if (!player || !player->isValid()) return;

    if (mRadius.mValue != 0.f)
    {
        player->setFlag<RenderCameraComponent>(true);
        player->setFlag<CameraRenderPlayerModelComponent>(true);
    }
    else
    {
        player->setFlag<RenderCameraComponent>(false);
        player->setFlag<CameraRenderPlayerModelComponent>(false);
    }

    auto* camera = event.mFirstPersonCamera;
    auto* direct = event.mCameraDirectLookComponent;
    if (!camera) return;

    // No ImGui frame yet (world load, very first tick) — skip instead of
    // dereferencing a null context.
    if (!ImGui::GetCurrentContext()) return;

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);

    // ─── rotation the game computed for this frame ──────────────────────────
    glm::vec2 raw = direct ? direct->mRotRads : glm::vec2(0.f, 0.f);
    if (!std::isfinite(raw.x) || !std::isfinite(raw.y)) raw = glm::vec2(0.f, 0.f);
    raw.x = wrapAngle(raw.x);
    raw.y = std::clamp(raw.y, -1.55f, 1.55f);   // never touch the pole

    if (!mRotInit)
    {
        mOrbitRot = raw;
        mOrbitVel = glm::vec2(0.f);
        mRotInit = true;
    }

    // Smoothness -> spring time. This is the whole "feel" knob. A damped spring
    // eases in *and* out, so a quick left/right flick is absorbed and released
    // instead of yanking the camera around; a plain lerp snaps at the start.
    const float s = std::clamp(mSmoothness.mValue, 0.f, 100.f) / 100.f;
    const float smoothTime = 0.010f + 0.32f * std::pow(s, 1.45f);

    // Turn Speed caps how far the orbit may actually swing this frame. Together
    // with the spring this is what stops "flick the mouse, camera flies" — the
    // camera still ends up exactly behind you, it just gets there calmly.
    const float maxStep = glm::radians(mTurnSpeed.mValue) * dt;

    // When the cap kicks in, the spring's own velocity estimate is no longer
    // valid (it assumed the camera really moved that far), so it is scaled down
    // by the same factor — otherwise it would keep pushing and overshoot.
    auto limitStep = [maxStep](float step, float& vel) {
        const float magnitude = std::fabs(step);
        if (magnitude <= maxStep) return step;
        vel *= maxStep / magnitude;
        return step < 0.f ? -maxStep : maxStep;
    };

    const float prevYaw = mOrbitRot.x;
    const float springYaw = smoothDampAngle(mOrbitRot.x, raw.x, mOrbitVel.x, smoothTime, dt);
    mOrbitRot.x = wrapAngle(prevYaw + limitStep(wrapAngle(springYaw - prevYaw), mOrbitVel.x));

    const float prevPitch = mOrbitRot.y;
    const float springPitch = smoothDamp(mOrbitRot.y, raw.y, mOrbitVel.y, smoothTime, dt);
    mOrbitRot.y = std::clamp(prevPitch + limitStep(springPitch - prevPitch, mOrbitVel.y), -1.55f, 1.55f);

    // A bad frame must never poison the camera.
    if (!std::isfinite(mOrbitRot.x) || !std::isfinite(mOrbitRot.y))
    {
        mOrbitRot = raw;
        mOrbitVel = glm::vec2(0.f);
    }

    const glm::vec3 aim = orbitDirection(mOrbitRot.x, mOrbitRot.y);
    const glm::vec3 right{ std::cos(mOrbitRot.x), 0.f, -std::sin(mOrbitRot.x) };

    // ─── orbit distance (scroll zoom, springed) ─────────────────────────────
    mCurrentDistance = smoothDamp(mCurrentDistance, mRadius.mValue, mDistVel, 0.09f, dt);

    // ─── pivot: the game refreshes mOrigin every call, so this is the true eye ──
    const glm::vec3 eye = camera->mOrigin;
    if (!finite3(eye)) return;

    if (!mPivotInit)
    {
        mPivot = eye;
        mLastEye = eye;
        mPivotVel = glm::vec3(0.f);
        mPivotInit = true;
    }

    // Body Trail: the orbit centre lags the eyes, which is what makes fast
    // movement look alive instead of glued to the skull. 0 = perfectly rigid.
    const float trail = std::clamp(mTrail.mValue, 0.f, 100.f) / 100.f;
    if (trail <= 0.001f)
    {
        mPivot = eye;
        mPivotVel = glm::vec3(0.f);
    }
    else
    {
        mPivot = smoothDampVec(mPivot, eye, mPivotVel, 0.02f + 0.28f * trail, dt);
    }

    // ─── wall handling (always on: the camera never sits inside a block) ─────
    float wantDistance = mCurrentDistance;
    bool blocked = false;

    if (auto* blockSource = ci->getBlockSource())
    {
        HitResult result = blockSource->checkRayTrace(mPivot, mPivot + aim * mCurrentDistance, player);

        if (result.mType == HitType::BLOCK)
        {
            blocked = true;
            wantDistance = (std::max)(0.35f, glm::length(result.mPos - mPivot) - 0.12f);
        }
    }

    if (!mCollisionInit)
    {
        mClippedDistance = wantDistance;
        mCollisionInit = true;
    }

    if (blocked)
    {
        // Never end up inside the wall: clamp first, then ease the rest of the
        // way in. Pulling back out is slower, otherwise every corner pops.
        mClippedDistance = (std::min)(mClippedDistance, wantDistance);
        mClippedDistance = smoothDamp(mClippedDistance, wantDistance, mClipVel, 0.05f, dt);
    }
    else
    {
        mClippedDistance = smoothDamp(mClippedDistance, wantDistance, mClipVel, 0.18f, dt);
    }

    // ─── damage shake (envelope fed by EntityHurtEvent) ─────────────────────
    if (mShakeLevel > 0.f)
    {
        const float decay = 1.f / (std::max)(0.05f, mShakeTime.mValue);
        mShakeLevel = (std::max)(0.f, mShakeLevel - decay * dt);
    }

    // Squaring the envelope leaves a long, quiet tail instead of the hard stop
    // a linear ramp has. This is the "super smooth" part.
    float shakeAmp = 0.f;
    if (mShakeMode.mValue != ShakeMode::Off && mShakeStrength.mValue > 0.f)
        shakeAmp = mShakeLevel * mShakeLevel * std::clamp(mShakeStrength.mValue / 100.f, 0.f, 1.f);

    // Punchy runs the same noise faster; it stays two low sines per axis, so it
    // reads as a kick rather than a vibration.
    const float freq = mShakeSpeed.mValue * (mShakeMode.mValue == ShakeMode::Punchy ? 2.4f : 1.f);

    glm::vec3 aimShaken = aim;
    glm::vec3 lateral{ 0.f };

    if (shakeAmp > 0.0001f)
    {
        const float t = static_cast<float>(ImGui::GetTime());

        // Two unrelated sines per axis, so nothing beats against the frame rate
        // and the image never buzzes the way the old 57 Hz shake did.
        const float n1 = std::sin(t * freq)          * 0.62f + std::sin(t * freq * 2.3f + 1.1f) * 0.38f;
        const float n2 = std::sin(t * freq * 1.37f + 2.4f) * 0.62f + std::sin(t * freq * 2.9f + 0.7f) * 0.38f;
        const float n3 = std::sin(t * freq * 0.83f + 4.0f) * 0.62f + std::sin(t * freq * 1.9f + 3.3f) * 0.38f;

        // Swinging the view around the pivot instead of writing the game's
        // orientation (mQuat) means the shake can never corrupt the camera.
        const float tilt = glm::radians(mShakeTilt.mValue) * 0.03f * shakeAmp;
        aimShaken = orbitDirection(mOrbitRot.x + n1 * tilt,
                                   std::clamp(mOrbitRot.y + n2 * tilt, -1.55f, 1.55f));

        // A little positional jitter on top, perpendicular to the view, so the
        // shake is still visible when Shake Tilt is 0.
        lateral = right * (n3 * 0.14f * shakeAmp) + glm::vec3(0.f, n2 * 0.10f * shakeAmp, 0.f);
    }

    glm::vec3 offset = aimShaken * mClippedDistance + lateral;
    offset.y += mHeight.mValue;

    // ─── walk bob (driven by how far the eyes moved this frame) ─────────────
    if (mBob.mValue > 0.f)
    {
        const glm::vec3 moved = eye - mLastEye;
        const float walkSpeed = std::sqrt(moved.x * moved.x + moved.z * moved.z) / dt;

        mBobPhase += walkSpeed * dt * 2.2f;
        if (mBobPhase > 1.0e4f) mBobPhase = std::fmod(mBobPhase, 2.f * kPi);
        mBobAmp = toward(mBobAmp, (walkSpeed > 0.9f) ? 1.f : 0.f, 7.f, dt);

        if (mBobAmp > 0.001f)
        {
            // Vertical is the readable part; the sideways wobble is deliberately
            // small, it used to make strafing look like the camera was sliding.
            offset.y += std::sin(mBobPhase) * mBob.mValue * 0.075f * mBobAmp;
            offset += right * (std::cos(mBobPhase * 0.5f) * mBob.mValue * 0.015f * mBobAmp);
        }
    }
    mLastEye = eye;

    const glm::vec3 camPos = mPivot + offset;

    // Never let a bad frame poison the camera — fall back to the eyes.
    camera->mOrigin = finite3(camPos) ? camPos : eye;
}
