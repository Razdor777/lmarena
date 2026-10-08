#include "CustomCrosshair.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MobHurtTimeComponent.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/ModuleManager.hpp>
#include <Utils/MemUtils.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>

void CustomCrosshair::onEnable()
{
    if (SigManager::HudCursorRenderer_render != 0) {
        unsigned char retByte = 0xC3;
        MemUtils::writeBytes(SigManager::HudCursorRenderer_render, &retByte, 1);
    }
    mWasSwinging = false;
    mPrevSwingProgress = 0;
    mWasHurt = false;
    mPrevHurtTime = 0;
    mAttackExpand = 0.f;
    mOscPhase = 0.f;
    mOscAmplitude = 0.f;
    mWasAttacking = false;
    mHitAnim = 0.f;
    mHurtAnim = 0.f;
    mMoveAnim = 0.f;
    mAirAnim = 0.f;
    mSneakAnim = 1.f;
    mSwayOffset = ImVec2(0.f, 0.f);
    mSwayPhase = 0.f;
    mRotOffsetX = 0.f;
    mRotOffsetY = 0.f;
    mRotVelX = 0.f;
    mRotVelY = 0.f;
    mHasPrevCamRot = false;
}

void CustomCrosshair::onDisable()
{
    if (SigManager::HudCursorRenderer_render != 0) {
        unsigned char originalByte = 0x48;
        MemUtils::writeBytes(SigManager::HudCursorRenderer_render, &originalByte, 1);
    }
}

static int64_t getNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

static float animFrame(float current, float target, float speed, float dt)
{
    float factor = 1.f - std::exp(-speed * dt);
    return current + (target - current) * factor;
}

static ImU32 col32(const ImColor& c, float alpha)
{
    return IM_COL32(
        (int)(MathUtils::clamp(c.Value.x, 0.f, 1.f) * 255.f),
        (int)(MathUtils::clamp(c.Value.y, 0.f, 1.f) * 255.f),
        (int)(MathUtils::clamp(c.Value.z, 0.f, 1.f) * 255.f),
        (int)(MathUtils::clamp(alpha, 0.f, 1.f) * 255.f));
}

// A ready-made look: writes the sliders once, then the caller resets the
// Preset setting back to Custom so it never fights your own tweaking.
void CustomCrosshair::applyPreset(Preset preset)
{
    switch (preset)
    {
    case Preset::CsGo:
        mStyle.setValue(CrosshairStyle::CsGo);
        mScale.setValue(1.f);       mSize.setValue(5.f);
        mGap.setValue(2.f);         mThickness.setValue(2.f);
        mExpandSize.setValue(8.f);
        mDot.setValue(false);       mOutline.setValue(false);
        mSneakShrink.setValue(true); mDynamic.setValue(DynamicMode::Full);
        break;

    case Preset::Valorant:
        mStyle.setValue(CrosshairStyle::Cross);
        mScale.setValue(1.f);       mSize.setValue(6.5f);
        mGap.setValue(4.f);         mThickness.setValue(2.f);
        mExpandSize.setValue(6.f);
        mDot.setValue(true);        mOutline.setValue(true);
        mSneakShrink.setValue(false); mDynamic.setValue(DynamicMode::Move);
        break;

    case Preset::Dot:
        mStyle.setValue(CrosshairStyle::Cross);
        mScale.setValue(1.f);       mSize.setValue(1.f);
        mGap.setValue(0.f);         mThickness.setValue(2.f);
        mExpandSize.setValue(4.f);
        mDot.setValue(true);        mOutline.setValue(true);
        mSneakShrink.setValue(false); mDynamic.setValue(DynamicMode::Off);
        break;

    case Preset::Classic:
        mStyle.setValue(CrosshairStyle::Cross);
        mScale.setValue(1.f);       mSize.setValue(6.f);
        mGap.setValue(2.f);         mThickness.setValue(2.f);
        mExpandSize.setValue(7.f);
        mDot.setValue(false);       mOutline.setValue(true);
        mSneakShrink.setValue(true); mDynamic.setValue(DynamicMode::Attack);
        break;

    case Preset::Custom:
    default:
        break;
    }
}

void CustomCrosshair::onRenderEvent(RenderEvent& event)
{
    // The handler is registered in the constructor and therefore runs even
    // while the module is off — without this the crosshair would keep being
    // drawn on top of the game's own one.
    if (!mEnabled) return;

    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();

    // ====================================================================
    // Не рисовать в меню/инвентаре/ClickGui — только на игровом экране
    // ====================================================================
    if (!player || ci->getScreenName() != "hud_screen") return;

    // ====================================================================
    // Пресеты
    // ====================================================================
    if (mPreset.as<Preset>() != Preset::Custom) {
        applyPreset(mPreset.as<Preset>());
        mPreset.setValue(Preset::Custom);
    }

    ImVec2 screenSize = ImRenderUtils::getScreenSize();
    ImVec2 center = ImVec2(screenSize.x / 2.f, screenSize.y / 2.f);

    float dt = ImRenderUtils::getDeltaTime();
    int64_t now = getNowMs();

    // ====================================================================
    // ОТСЛЕЖИВАНИЕ СОБЫТИЙ
    // ====================================================================

    // --- АТАКА: isSwinging() работает и с AutoClicker ---
    bool isAttacking = player->isSwinging();
    int swingProgress = player->getSwingProgress();

    // Детект нового свинга (для попадания по сущности)
    bool newSwing = false;
    if (isAttacking && !mWasSwinging) newSwing = true;
    if (isAttacking && swingProgress < mPrevSwingProgress && mPrevSwingProgress > 1) newSwing = true;

    if (newSwing) {
        auto level = player->getLevel();
        if (level) {
            auto hitResult = level->getHitResult();
            if (hitResult && hitResult->mType == HitType::ENTITY) {
                mLastHitEntityTime = now;
            }
        }
    }

    mWasSwinging = isAttacking;
    mPrevSwingProgress = swingProgress;

    // --- ПОЛУЧЕНИЕ УРОНА ---
    auto hurtComp = player->getMobHurtTimeComponent();
    bool isHurt = hurtComp && hurtComp->mHurtTime > 0;

    if (isHurt && !mWasHurt) mLastHurtTime = now;
    if (hurtComp && hurtComp->mHurtTime > mPrevHurtTime && hurtComp->mHurtTime > 0) mLastHurtTime = now;

    mWasHurt = isHurt;
    mPrevHurtTime = hurtComp ? hurtComp->mHurtTime : 0;

    // ====================================================================
    // АНИМАЦИЯ АТАКИ
    //
    // Фаза 1: Зажал → расширение на 100% (быстро)
    // Фаза 2: Держишь → осцилляция ><>< ±15% (30% размах)
    // Фаза 3: Отпустил → сжатие обратно (быстро)
    // ====================================================================

    if (mDynamic.mValue == DynamicMode::Attack || mDynamic.mValue == DynamicMode::Full)
    {
        float targetExpand = isAttacking ? 1.f : 0.f;
        float expandSpeed = (targetExpand > mAttackExpand) ? 25.f : 14.f;
        mAttackExpand = animFrame(mAttackExpand, targetExpand, expandSpeed, dt);

        if (isAttacking) {
            if (!mWasAttacking) mAttackStartTime = now;
            float attackDuration = (float)(now - mAttackStartTime) / 1000.f;

            if (attackDuration > 0.2f && mAttackExpand > 0.7f) {
                mOscPhase += dt * 12.f;
                mOscAmplitude = animFrame(mOscAmplitude, 0.15f, 6.f, dt);
            }
        } else {
            mOscAmplitude = animFrame(mOscAmplitude, 0.f, 10.f, dt);
        }
        mWasAttacking = isAttacking;
    }
    else
    {
        mAttackExpand = animFrame(mAttackExpand, 0.f, 14.f, dt);
        mOscAmplitude = animFrame(mOscAmplitude, 0.f, 10.f, dt);
    }

    float oscValue = sinf(mOscPhase) * mOscAmplitude;
    float attackContribution = mAttackExpand * (1.f + oscValue);

    // --- Попадание по сущности ---
    float hitTarget = 0.f;
    if ((mDynamic.mValue == DynamicMode::Attack || mDynamic.mValue == DynamicMode::Full)
        && now - mLastHitEntityTime < 300) {
        hitTarget = 1.f;
    }
    float hitSpeed = (hitTarget > mHitAnim) ? 35.f : 10.f;
    mHitAnim = animFrame(mHitAnim, hitTarget, hitSpeed, dt);

    // --- Получение урона ---
    float hurtTarget = 0.f;
    if (mDynamic.mValue == DynamicMode::Full && now - mLastHurtTime < 350) {
        hurtTarget = 1.f;
    }
    float hurtSpeed = (hurtTarget > mHurtAnim) ? 40.f : 8.f;
    mHurtAnim = animFrame(mHurtAnim, hurtTarget, hurtSpeed, dt);

    // --- Движение + Воздух + Присед ---
    float moveTarget = 0.f;
    float airTarget = 0.f;
    float sneakTarget = 1.f;

    auto moveInput = player->getMoveInputComponent();
    auto stateVector = player->getStateVectorComponent();

    // Sway target: how far the reticle should be pushed away from the exact
    // screen center this frame. Movement keys give the direction (they are
    // already camera relative), horizontal speed gives how "alive" it looks.
    ImVec2 swayTarget(0.f, 0.f);

    if (mDynamic.mValue == DynamicMode::Move || mDynamic.mValue == DynamicMode::Full)
    {
        if (stateVector) {
            float hSpeed = glm::length(glm::vec2(stateVector->mVelocity.x, stateVector->mVelocity.z));
            moveTarget = MathUtils::clamp(hSpeed / 0.15f, 0.f, 1.f);
        }

        if (!player->isOnGround()) airTarget = 1.f;

        if (mSneakShrink.mValue && moveInput && moveInput->mIsSneakDown) sneakTarget = 0.6f;
    }

    if (mSway.mValue)
    {
        float fwd = 0.f, side = 0.f;
        if (moveInput) {
            fwd = (moveInput->mForward ? 1.f : 0.f) - (moveInput->mBackward ? 1.f : 0.f);
            side = (moveInput->mRight ? 1.f : 0.f) - (moveInput->mLeft ? 1.f : 0.f);
        }

        float speed01 = 0.f;
        if (stateVector)
            speed01 = MathUtils::clamp(glm::length(glm::vec2(stateVector->mVelocity.x, stateVector->mVelocity.z)) / 0.16f, 0.f, 1.f);

        glm::vec2 dir(side, fwd);
        float len = glm::length(dir);
        const bool moving = len > 0.0001f;
        if (moving) dir /= len;

        const float amp = mSwayAmount.mValue * (0.35f + 0.65f * speed01);

        // The reticle "walks" with you: forward lifts it, strafing pushes it
        // sideways. A small sine on top makes it sway back and forth instead
        // of sliding linearly, and only while you are actually moving.
        if (moving) mSwayPhase += dt * (5.f + 5.f * speed01);
        const float wobble = moving ? sinf(mSwayPhase) * amp * 0.28f : 0.f;

        swayTarget = ImVec2(dir.x * (amp + wobble), -dir.y * (amp + wobble));
    }

    mSwayOffset.x = animFrame(mSwayOffset.x, swayTarget.x, 9.f, dt);
    mSwayOffset.y = animFrame(mSwayOffset.y, swayTarget.y, 9.f, dt);

    // ====================================================================
    // ROTATION SWAY (Roblox-style camera lag)
    //
    // While you flick the camera the reticle is dragged behind it, then a
    // spring pulls it back to the exact center. The reticle is never "pinned":
    // turn left/right and it swings, stop - and it settles.
    // ====================================================================
    if (mRotSway.mValue)
    {
        auto rot = player->getActorRotationComponent();
        if (rot)
        {
            const float yaw = rot->mYaw;
            const float pitch = rot->mPitch;

            if (!mHasPrevCamRot)
            {
                mPrevCamYaw = yaw;
                mPrevCamPitch = pitch;
                mHasPrevCamRot = true;
            }

            // wrap() keeps a 359 -> 0 crossing from looking like a full spin
            float dYaw = MathUtils::wrap(yaw - mPrevCamYaw, -180.f, 180.f);
            float dPitch = pitch - mPrevCamPitch;
            mPrevCamYaw = yaw;
            mPrevCamPitch = pitch;

            // Respawn / teleport jumps are not camera flicks.
            if (std::fabs(dYaw) > 30.f) dYaw = 0.f;
            if (std::fabs(dPitch) > 30.f) dPitch = 0.f;

            const float safeDt = MathUtils::clamp(dt, 0.0001f, 0.1f);
            float vYaw = MathUtils::clamp(dYaw / safeDt, -1440.f, 1440.f);
            float vPitch = MathUtils::clamp(dPitch / safeDt, -1440.f, 1440.f);

            // Trailing behind = opposite to the rotation. Screen X grows to the
            // right while yaw grows to the right as well, and pitch grows when
            // looking down, so both targets are negated - the reticle stays
            // where the view used to point and is dragged back by the spring.
            const float pxPerDegreePerSecond = 0.055f * mRotSwayAmount.mValue;
            const float targetX = MathUtils::clamp(-vYaw * pxPerDegreePerSecond, -60.f, 60.f);
            const float targetY = MathUtils::clamp(-vPitch * pxPerDegreePerSecond, -60.f, 60.f);

            // Underdamped spring: slight overshoot is what makes it feel alive
            // instead of like a plain follow-with-delay filter.
            const float omega = 6.f / (std::max)(0.05f, mRotSwayLag.mValue);
            const float zeta = 0.55f;

            const float accelX = omega * omega * (targetX - mRotOffsetX) - 2.f * zeta * omega * mRotVelX;
            const float accelY = omega * omega * (targetY - mRotOffsetY) - 2.f * zeta * omega * mRotVelY;

            mRotVelX += accelX * safeDt;
            mRotVelY += accelY * safeDt;
            mRotOffsetX += mRotVelX * safeDt;
            mRotOffsetY += mRotVelY * safeDt;
        }
    }
    else if (mRotOffsetX != 0.f || mRotOffsetY != 0.f)
    {
        // Turning the setting off should not snap the reticle instantly.
        const float decay = std::exp(-14.f * MathUtils::clamp(dt, 0.0001f, 0.1f));
        mRotOffsetX *= decay;
        mRotOffsetY *= decay;
        mRotVelX = 0.f;
        mRotVelY = 0.f;
        if (std::fabs(mRotOffsetX) < 0.01f) mRotOffsetX = 0.f;
        if (std::fabs(mRotOffsetY) < 0.01f) mRotOffsetY = 0.f;
    }

    mMoveAnim = animFrame(mMoveAnim, moveTarget, 12.f, dt);
    float airSpeed = (airTarget > mAirAnim) ? 20.f : 7.f;
    mAirAnim = animFrame(mAirAnim, airTarget, airSpeed, dt);
    mSneakAnim = animFrame(mSneakAnim, sneakTarget, 10.f, dt);

    // ====================================================================
    // ИТОГОВЫЙ РАСЧЁТ
    // R = R_base × sneak + Expand × (attack + hit×0.6 + move×0.5 + air×0.8 + hurt×0.7)
    // ====================================================================
    float totalExpand = attackContribution
                      + mHitAnim * 0.6f
                      + mMoveAnim * 0.5f
                      + mAirAnim * 0.8f
                      + mHurtAnim * 0.7f;

    totalExpand = MathUtils::clamp(totalExpand, 0.f, 3.5f);

    float baseGap = mGap.mValue * mSneakAnim * mScale.mValue;
    float expandGap = mExpandSize.mValue * totalExpand * mScale.mValue;
    float currentGap = MathUtils::clamp(baseGap + expandGap, 0.f, 100.f);

    float size = mSize.mValue * mScale.mValue;
    float thick = mThickness.mValue * mScale.mValue;

    // Thickness 0 means "no crosshair": the arms would be degenerate, but the
    // outline pass would still paint a thin ghost cross, so bail out here.
    if (thick <= 0.001f) return;

    float dotSize = thick * 0.85f;

    ImColor color = mColor.getAsImColor();
    if (mRainbow.mValue) color = ColorUtils::getThemedColor(0, 0);
    const float colorAlpha = MathUtils::clamp(color.Value.w, 0.15f, 1.f);
    const ImU32 body = col32(color, colorAlpha);
    const ImU32 outline = IM_COL32(0, 0, 0, (int)(MathUtils::clamp(colorAlpha * 0.75f, 0.f, 1.f) * 255.f));
    // One slider for both: the outline keeps a visible floor so hairlines stay
    // readable, but stays proportional so thick lines get a thick outline.
    const float outlinePad = mOutline.mValue ? (std::max)(0.25f, thick * 0.30f) : 0.f;
    const float round = (std::max)(0.f, thick * 0.35f);

    // Reticle position = screen center + movement sway + rotation lag.
    const float cx = center.x + mSwayOffset.x + mRotOffsetX;
    const float cy = center.y + mSwayOffset.y + mRotOffsetY;
    const float halfT = thick * 0.5f;

    auto* dl = ImGui::GetBackgroundDrawList();

    auto arm = [&](float x0, float y0, float x1, float y1, ImU32 c) {
        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), c, round);
    };

    // ====================================================================
    // Рендер
    // ====================================================================
    if (mStyle.mValue == CrosshairStyle::Cross || mStyle.mValue == CrosshairStyle::CsGo)
    {
        auto drawArms = [&](float o, ImU32 c) {
            // top
            arm(cx - halfT - o, cy - currentGap - size - o, cx + halfT + o, cy - currentGap + o, c);
            // bottom
            arm(cx - halfT - o, cy + currentGap - o, cx + halfT + o, cy + currentGap + size + o, c);
            // left
            arm(cx - currentGap - size - o, cy - halfT - o, cx - currentGap + o, cy + halfT + o, c);
            // right
            arm(cx + currentGap - o, cy - halfT - o, cx + currentGap + size + o, cy + halfT + o, c);
        };

        if (outlinePad > 0.f) drawArms(outlinePad, outline);
        drawArms(0.f, body);
    }

    if (mDot.mValue)
    {
        if (outlinePad > 0.f)
            dl->AddCircleFilled(ImVec2(cx, cy), dotSize + outlinePad, outline, 16);
        dl->AddCircleFilled(ImVec2(cx, cy), dotSize, body, 16);
    }

    if (mStyle.mValue == CrosshairStyle::Circle)
    {
        float radius = currentGap + size * 0.5f;
        if (outlinePad > 0.f)
            dl->AddCircle(ImVec2(cx, cy), radius, outline, 48, thick + outlinePad * 2.f);
        dl->AddCircle(ImVec2(cx, cy), radius, body, 48, thick);
    }
}

std::string CustomCrosshair::getSettingDisplay()
{
    return mStyle.mValues[mStyle.as<int>()];
}
