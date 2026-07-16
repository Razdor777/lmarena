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

void CustomCrosshair::onRenderEvent(RenderEvent& event)
{
    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();

    // ====================================================================
    // Не рисовать в меню/инвентаре/ClickGui — только на игровом экране
    // ====================================================================
    if (!player || ci->getScreenName() != "hud_screen") return;

    // ====================================================================
    // Пресет CS:GO
    // ====================================================================
    if (mPresetCsGo.mValue) {
        mStyle.mValue = CrosshairStyle::CsGo;
        mSize.mValue = 4.f; mGap.mValue = 3.f;
        mThickness.mValue = 2.f; mExpandSize.mValue = 12.f;
        mOutline.mValue = true; mDot.mValue = true;
        mDynamic.mValue = DynamicMode::Full;
        mSneakShrink.mValue = true;
        mPresetCsGo.mValue = false;
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
        // Базовое расширение: 0→1 когда атакуем, 1→0 когда нет
        float targetExpand = isAttacking ? 1.f : 0.f;
        float expandSpeed = (targetExpand > mAttackExpand) ? 25.f : 14.f; // Быстрое сжатие!
        mAttackExpand = animFrame(mAttackExpand, targetExpand, expandSpeed, dt);

        // Осцилляция: начинается после 200мс непрерывной атаки
        if (isAttacking) {
            if (!mWasAttacking) mAttackStartTime = now;
            float attackDuration = (float)(now - mAttackStartTime) / 1000.f;

            // После 200мс — плавно запускаем пульсацию
            if (attackDuration > 0.2f && mAttackExpand > 0.7f) {
                mOscPhase += dt * 12.f; // ~2 герца (2 пульса в секунду)
                mOscAmplitude = animFrame(mOscAmplitude, 0.15f, 6.f, dt); // Плавное нарастание
            }
        } else {
            // Не атакуем — амплитуда плавно спадает
            mOscAmplitude = animFrame(mOscAmplitude, 0.f, 10.f, dt);
        }
        mWasAttacking = isAttacking;
    }
    else
    {
        mAttackExpand = animFrame(mAttackExpand, 0.f, 14.f, dt);
        mOscAmplitude = animFrame(mOscAmplitude, 0.f, 10.f, dt);
    }

    // Итоговый вклад атаки: базовое расширение × (1 + осцилляция)
    // Когда mAttackExpand = 1.0 и oscAmplitude = 0.15:
    //   sin=1  → 1.0 × 1.15 = 1.15 (+15%)
    //   sin=-1 → 1.0 × 0.85 = 0.85 (-15%)
    //   Итого размах = 30% — маленькая пульсация
    // Когда отпускаешь: mAttackExpand → 0, oscAmplitude → 0, всё → 0
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

    if (mDynamic.mValue == DynamicMode::Move || mDynamic.mValue == DynamicMode::Full)
    {
        auto moveInput = player->getMoveInputComponent();
        auto stateVector = player->getStateVectorComponent();

        if (stateVector) {
            float hSpeed = glm::length(glm::vec2(stateVector->mVelocity.x, stateVector->mVelocity.z));
            moveTarget = MathUtils::clamp(hSpeed / 0.15f, 0.f, 1.f);
        }

        if (!player->isOnGround()) airTarget = 1.f;

        if (mSneakShrink.mValue && moveInput && moveInput->mIsSneakDown) sneakTarget = 0.6f;
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
    float dotSize = thick * 1.5f;

    ImColor color = mColor.getAsImColor();
    if (mRainbow.mValue) color = ColorUtils::getThemedColor(0, 0);
    
    ImColor outlineColor = ImColor(0, 0, 0, 200);

    // ====================================================================
    // Рендер
    // ====================================================================
    if (mStyle.mValue == CrosshairStyle::Cross || mStyle.mValue == CrosshairStyle::CsGo)
    {
        if (mOutline.mValue) {
            ImRenderUtils::fillRectangle(ImVec4(center.x - thick / 2 - 1, center.y - currentGap - size - 1, center.x + thick / 2 + 1, center.y - currentGap + 1), outlineColor, 1.f);
            ImRenderUtils::fillRectangle(ImVec4(center.x - thick / 2 - 1, center.y + currentGap - 1, center.x + thick / 2 + 1, center.y + currentGap + size + 1), outlineColor, 1.f);
            ImRenderUtils::fillRectangle(ImVec4(center.x - currentGap - size - 1, center.y - thick / 2 - 1, center.x - currentGap + 1, center.y + thick / 2 + 1), outlineColor, 1.f);
            ImRenderUtils::fillRectangle(ImVec4(center.x + currentGap - 1, center.y - thick / 2 - 1, center.x + currentGap + size + 1, center.y + thick / 2 + 1), outlineColor, 1.f);
        }

        ImRenderUtils::fillRectangle(ImVec4(center.x - thick / 2, center.y - currentGap - size, center.x + thick / 2, center.y - currentGap), color, 1.f);
        ImRenderUtils::fillRectangle(ImVec4(center.x - thick / 2, center.y + currentGap, center.x + thick / 2, center.y + currentGap + size), color, 1.f);
        ImRenderUtils::fillRectangle(ImVec4(center.x - currentGap - size, center.y - thick / 2, center.x - currentGap, center.y + thick / 2), color, 1.f);
        ImRenderUtils::fillRectangle(ImVec4(center.x + currentGap, center.y - thick / 2, center.x + currentGap + size, center.y + thick / 2), color, 1.f);
    }

    if (mDot.mValue)
    {
        if (mOutline.mValue) {
            ImRenderUtils::fillCircle(center, dotSize + 1, outlineColor, 1.f, 12);
        }
        ImRenderUtils::fillCircle(center, dotSize, color, 1.f, 12);
    }

    if (mStyle.mValue == CrosshairStyle::Circle)
    {
        float radius = currentGap + size / 2;
        if (mOutline.mValue) {
            ImGui::GetBackgroundDrawList()->AddCircle(center, radius, outlineColor, 32, thick + 2.f);
        }
        ImGui::GetBackgroundDrawList()->AddCircle(center, radius, color, 32, thick);
    }
}