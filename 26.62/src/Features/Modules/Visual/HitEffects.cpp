//
// HitEffects implementation with spawn/despawn animations
//

#include "HitEffects.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/RemoveActorPacket.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

// ==================== EASING FUNCTIONS ====================

float HitEffects::easeOutElastic(float t)
{
    if (t == 0 || t == 1) return t;
    float p = 0.3f;
    float s = p / 4.0f;
    return powf(2.0f, -10.0f * t) * sinf((t - s) * (2.0f * 3.14159f) / p) + 1.0f;
}

float HitEffects::easeOutBounce(float t)
{
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

float HitEffects::easeOutBack(float t)
{
    float c1 = 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

float HitEffects::easeOutExpo(float t)
{
    return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}

float HitEffects::easeInExpo(float t)
{
    return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * t - 10.0f);
}

float HitEffects::easeInOutCubic(float t)
{
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float HitEffects::easeInOutSine(float t)
{
    return -(cosf(3.14159f * t) - 1.0f) / 2.0f;
}

float HitEffects::easeOutCubic(float t)
{
    return 1.0f - powf(1.0f - t, 3.0f);
}

float HitEffects::easeInCubic(float t)
{
    return t * t * t;
}

// ==================== SPAWN ANIMATIONS ====================

float HitEffects::getSpawnScale(float progress)
{
    float t = mSpawnEasing.mValue ? progress : progress;
    
    switch (mSpawnStyle.mValue)
    {
        case SpawnStyle::Grow:
            return mSpawnEasing.mValue ? easeOutCubic(t) : t;
            
        case SpawnStyle::GrowRotate:
            return mSpawnEasing.mValue ? easeOutBack(t) : t;
            
        case SpawnStyle::Explode:
            // Быстро появляется и немного "отскакивает"
            return mSpawnEasing.mValue ? easeOutElastic(t) : t;
            
        case SpawnStyle::Fade:
            return 1.0f; // Размер не меняется
            
        case SpawnStyle::Bounce:
            return mSpawnEasing.mValue ? easeOutBounce(t) : t;
            
        case SpawnStyle::Spiral:
            return mSpawnEasing.mValue ? easeOutExpo(t) : t;
            
        default:
            return t;
    }
}

float HitEffects::getSpawnAlpha(float progress)
{
    float t = mSpawnEasing.mValue ? progress : progress;
    
    switch (mSpawnStyle.mValue)
    {
        case SpawnStyle::Fade:
            return mSpawnEasing.mValue ? easeOutCubic(t) : t;
            
        case SpawnStyle::Explode:
            return std::min(1.0f, t * 2.0f); // Быстро становится видимым
            
        default:
            return std::min(1.0f, t * 1.5f); // Чуть быстрее чем размер
    }
}

float HitEffects::getSpawnExtraRotation(float progress)
{
    float t = mSpawnEasing.mValue ? progress : progress;
    
    switch (mSpawnStyle.mValue)
    {
        case SpawnStyle::GrowRotate:
            // Быстро крутится в начале, замедляется к концу
            return (1.0f - easeOutExpo(t)) * 6.28318f * 2.0f; // 2 полных оборота
            
        case SpawnStyle::Spiral:
            // Много вращений при появлении
            return (1.0f - easeOutExpo(t)) * 6.28318f * 4.0f; // 4 оборота
            
        case SpawnStyle::Explode:
            return (1.0f - t) * 3.14159f; // Пол оборота
            
        default:
            return 0.0f;
    }
}

// ==================== DESPAWN ANIMATIONS ====================

float HitEffects::getDespawnScale(float progress)
{
    float t = mDespawnEasing.mValue ? progress : progress;
    
    switch (mDespawnStyle.mValue)
    {
        case DespawnStyle::Shrink:
            return 1.0f - (mDespawnEasing.mValue ? easeInCubic(t) : t);
            
        case DespawnStyle::ShrinkRotate:
            return 1.0f - (mDespawnEasing.mValue ? easeInExpo(t) : t);
            
        case DespawnStyle::Implode:
            // Резко схлопывается в конце
            return 1.0f - (mDespawnEasing.mValue ? easeInExpo(t) : t * t);
            
        case DespawnStyle::Fade:
            return 1.0f; // Размер не меняется
            
        case DespawnStyle::Spin:
            return 1.0f - (mDespawnEasing.mValue ? easeInCubic(t) : t) * 0.3f; // Немного уменьшается
            
        case DespawnStyle::Scatter:
            // Увеличивается при исчезновении ("разлетается")
            return 1.0f + (mDespawnEasing.mValue ? easeOutCubic(t) : t) * 0.5f;
            
        default:
            return 1.0f - t;
    }
}

float HitEffects::getDespawnAlpha(float progress)
{
    float t = mDespawnEasing.mValue ? progress : progress;
    
    switch (mDespawnStyle.mValue)
    {
        case DespawnStyle::Fade:
        case DespawnStyle::Scatter:
            return 1.0f - (mDespawnEasing.mValue ? easeInCubic(t) : t);
            
        case DespawnStyle::Implode:
            // Альфа падает в конце
            return 1.0f - (mDespawnEasing.mValue ? easeInExpo(t) : t * t);
            
        default:
            return 1.0f - t;
    }
}

float HitEffects::getDespawnExtraRotation(float progress)
{
    float t = mDespawnEasing.mValue ? progress : progress;
    
    switch (mDespawnStyle.mValue)
    {
        case DespawnStyle::ShrinkRotate:
            // Ускоряется при исчезновении
            return easeInExpo(t) * 6.28318f * 3.0f; // 3 оборота
            
        case DespawnStyle::Spin:
            // Очень быстро крутится
            return easeInExpo(t) * 6.28318f * 5.0f; // 5 оборотов
            
        case DespawnStyle::Implode:
            return t * 3.14159f; // Пол оборота
            
        default:
            return 0.0f;
    }
}

// ==================== LIFECYCLE ====================

void HitEffects::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &HitEffects::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &HitEffects::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &HitEffects::onPacketInEvent>(this);
    
    std::lock_guard<std::mutex> lock(mHitMutex);
    mActiveHit.reset();
    
    // Apply preset on enable
    if (mPreset.mValue != Preset::Custom)
        applyPreset(mPreset.mValue);
    mLastPreset = mPreset.mValue;
}

void HitEffects::applyPreset(Preset preset)
{
    switch (preset)
    {
        case Preset::AnimeStar:
            mStyle.mValue = Style::Both;
            mDuration.mValue = 2.5f;
            mThickness.mValue = 2.5f;
            mSpawnStyle.mValue = SpawnStyle::Bounce;
            mDespawnStyle.mValue = DespawnStyle::Scatter;
            mSpawnTime.mValue = 0.5f;
            mDespawnTime.mValue = 0.4f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mDiamondSize.mValue = 18.0f;
            mDiamondInnerSize.mValue = 0.4f;
            mCornersSize.mValue = 25.0f;
            mCornerLength.mValue = 0.35f;
            mCornerGap.mValue = 1.2f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Normal;
            mRotationSpeed.mValue = 4.0f;
            mRainbow.mValue = true;
            mRainbowSpeed.mValue = 3.0f;
            mColorA.mValue = 0.95f;
            mGlow.mValue = true;
            mGlowIntensity.mValue = 0.4f;
            break;

        case Preset::PVPDiamond:
            mStyle.mValue = Style::Diamond;
            mDuration.mValue = 1.5f;
            mThickness.mValue = 2.0f;
            mSpawnStyle.mValue = SpawnStyle::GrowRotate;
            mDespawnStyle.mValue = DespawnStyle::ShrinkRotate;
            mSpawnTime.mValue = 0.3f;
            mDespawnTime.mValue = 0.25f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mDiamondSize.mValue = 14.0f;
            mDiamondInnerSize.mValue = 0.45f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Normal;
            mRotationSpeed.mValue = 3.5f;
            mRainbow.mValue = false;
            mColorR.mValue = 1.0f;
            mColorG.mValue = 0.15f;
            mColorB.mValue = 0.15f;
            mColorA.mValue = 0.9f;
            mGlow.mValue = true;
            mGlowIntensity.mValue = 0.3f;
            break;

        case Preset::SakuraPetal:
            mStyle.mValue = Style::Corners;
            mDuration.mValue = 3.0f;
            mThickness.mValue = 1.5f;
            mSpawnStyle.mValue = SpawnStyle::Fade;
            mDespawnStyle.mValue = DespawnStyle::Fade;
            mSpawnTime.mValue = 0.6f;
            mDespawnTime.mValue = 0.5f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mCornersSize.mValue = 22.0f;
            mCornerLength.mValue = 0.4f;
            mCornerGap.mValue = 1.1f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Wave;
            mWaveAmplitude.mValue = 20.0f;
            mWaveFrequency.mValue = 1.5f;
            mRainbow.mValue = false;
            mColorR.mValue = 1.0f;
            mColorG.mValue = 0.6f;
            mColorB.mValue = 0.75f;
            mColorA.mValue = 0.85f;
            mGlow.mValue = true;
            mGlowIntensity.mValue = 0.25f;
            break;

        case Preset::FireStar:
            mStyle.mValue = Style::Diamond;
            mDuration.mValue = 2.0f;
            mThickness.mValue = 2.5f;
            mSpawnStyle.mValue = SpawnStyle::Explode;
            mDespawnStyle.mValue = DespawnStyle::Implode;
            mSpawnTime.mValue = 0.35f;
            mDespawnTime.mValue = 0.3f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mDiamondSize.mValue = 16.0f;
            mDiamondInnerSize.mValue = 0.5f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Pulse;
            mPulseMinSpeed.mValue = 1.5f;
            mPulseMaxSpeed.mValue = 7.0f;
            mPulseCycleTime.mValue = 0.8f;
            mRainbow.mValue = false;
            mColorR.mValue = 1.0f;
            mColorG.mValue = 0.55f;
            mColorB.mValue = 0.0f;
            mColorA.mValue = 0.95f;
            mGlow.mValue = true;
            mGlowIntensity.mValue = 0.5f;
            break;

        case Preset::NeonPulse:
            mStyle.mValue = Style::Both;
            mDuration.mValue = 2.0f;
            mThickness.mValue = 2.0f;
            mSpawnStyle.mValue = SpawnStyle::Spiral;
            mDespawnStyle.mValue = DespawnStyle::Spin;
            mSpawnTime.mValue = 0.4f;
            mDespawnTime.mValue = 0.35f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mDiamondSize.mValue = 15.0f;
            mDiamondInnerSize.mValue = 0.3f;
            mCornersSize.mValue = 20.0f;
            mCornerLength.mValue = 0.3f;
            mCornerGap.mValue = 1.15f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Custom;
            mCustomCycles.mValue = 2.0f;
            mCustomPause.mValue = 0.4f;
            mCustomSpeed.mValue = 5.0f;
            mCustomEasing.mValue = true;
            mRainbow.mValue = false;
            mColorR.mValue = 0.0f;
            mColorG.mValue = 0.9f;
            mColorB.mValue = 1.0f;
            mColorA.mValue = 0.9f;
            mGlow.mValue = true;
            mGlowIntensity.mValue = 0.45f;
            break;

        case Preset::MinimalDot:
            mStyle.mValue = Style::Diamond;
            mDuration.mValue = 1.0f;
            mThickness.mValue = 1.5f;
            mSpawnStyle.mValue = SpawnStyle::Grow;
            mDespawnStyle.mValue = DespawnStyle::Shrink;
            mSpawnTime.mValue = 0.2f;
            mDespawnTime.mValue = 0.2f;
            mSpawnEasing.mValue = true;
            mDespawnEasing.mValue = true;
            mDiamondSize.mValue = 8.0f;
            mDiamondInnerSize.mValue = 0.6f;
            mPositionY.mValue = 0.0f;
            mHeightPercent.mValue = 50.0f;
            mRotationMode.mValue = RotationMode::Normal;
            mRotationSpeed.mValue = 2.0f;
            mRainbow.mValue = false;
            mColorR.mValue = 1.0f;
            mColorG.mValue = 1.0f;
            mColorB.mValue = 1.0f;
            mColorA.mValue = 0.8f;
            mGlow.mValue = false;
            mGlowIntensity.mValue = 0.0f;
            break;

        case Preset::Custom:
        default:
            break; // Don't touch settings
    }
}

void HitEffects::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &HitEffects::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &HitEffects::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &HitEffects::onPacketInEvent>(this);
    
    std::lock_guard<std::mutex> lock(mHitMutex);
    mActiveHit.reset();
}

ImColor HitEffects::getColor(float timeOffset)
{
    if (mRainbow.mValue)
    {
        float time = ImGui::GetTime() * mRainbowSpeed.mValue + timeOffset;
        float r = (sinf(time) + 1.0f) / 2.0f;
        float g = (sinf(time + 2.094f) + 1.0f) / 2.0f;
        float b = (sinf(time + 4.189f) + 1.0f) / 2.0f;
        return ImColor(r, g, b, mColorA.mValue);
    }
    return ImColor(mColorR.mValue, mColorG.mValue, mColorB.mValue, mColorA.mValue);
}

// ==================== HIT PROCESSING ====================

void HitEffects::processHit(Actor* target)
{
    if (!target) return;
    
    std::lock_guard<std::mutex> lock(mHitMutex);
    
    int64_t targetRuntimeId = target->getRuntimeID();
    
    // Проверяем, это та же цель?
    if (mActiveHit.has_value() && mActiveHit->runtimeId == targetRuntimeId)
    {
        // Та же цель - просто продлеваем время!
        // НЕ сбрасываем анимацию
        mActiveHit->lastHitTime = NOW;
        
        // Если была в состоянии Despawning - возвращаем в Active
        if (mActiveHit->state == HitState::Despawning)
        {
            mActiveHit->state = HitState::Active;
            mActiveHit->animProgress = 1.0f;
        }
        
        return;
    }
    
    // Новая цель! Старая начинает исчезать (если есть)
    // Для простоты - просто заменяем на новую
    
    HitInfo newHit;
    newHit.target = target;
    newHit.runtimeId = targetRuntimeId;
    newHit.lastHitTime = NOW;
    newHit.stateStartTime = NOW;
    newHit.hitPosition = *target->getPos();
    newHit.rotation = 0.0f;
    newHit.rotationDirection = 1.0f;
    newHit.rotationCycles = 0;
    newHit.pauseTimer = 0.0f;
    newHit.isPaused = false;
    newHit.state = HitState::Spawning;
    newHit.animProgress = 0.0f;
    
    mActiveHit = newHit;
}

void HitEffects::updateState(HitInfo& hit)
{
    uint64_t now = NOW;
    
    switch (hit.state)
    {
        case HitState::Spawning:
        {
            float elapsed = static_cast<float>(now - hit.stateStartTime) / 1000.0f;
            hit.animProgress = std::min(1.0f, elapsed / mSpawnTime.mValue);
            
            if (hit.animProgress >= 1.0f)
            {
                hit.state = HitState::Active;
                hit.stateStartTime = now;
                hit.animProgress = 1.0f;
            }
            break;
        }
        
        case HitState::Active:
        {
            // Проверяем, не пора ли начать исчезновение
            uint64_t duration = static_cast<uint64_t>(mDuration.mValue * 1000);
            if (now - hit.lastHitTime >= duration)
            {
                hit.state = HitState::Despawning;
                hit.stateStartTime = now;
                hit.animProgress = 0.0f;
            }
            break;
        }
        
        case HitState::Despawning:
        {
            float elapsed = static_cast<float>(now - hit.stateStartTime) / 1000.0f;
            hit.animProgress = std::min(1.0f, elapsed / mDespawnTime.mValue);
            break;
        }
    }
}

void HitEffects::updateRotation(HitInfo& hit, float deltaTime)
{
    // Не обновляем вращение во время анимаций (оно добавляется отдельно)
    if (hit.state != HitState::Active) return;
    
    switch (mRotationMode.mValue)
    {
        case RotationMode::Normal:
        {
            hit.rotation += deltaTime * mRotationSpeed.mValue;
            break;
        }
        
        case RotationMode::PingPong:
        {
            float elapsed = static_cast<float>(NOW - hit.stateStartTime) / 1000.0f;
            float angle = mPingPongAngle.mValue * 3.14159f / 180.0f;
            hit.rotation = sinf(elapsed * mPingPongSpeed.mValue) * angle;
            break;
        }
        
        case RotationMode::Pulse:
        {
            float elapsed = static_cast<float>(NOW - hit.stateStartTime) / 1000.0f;
            float cycleProgress = fmodf(elapsed, mPulseCycleTime.mValue) / mPulseCycleTime.mValue;
            float speedMultiplier = easeInOutSine(cycleProgress);
            float currentSpeed = mPulseMinSpeed.mValue + (mPulseMaxSpeed.mValue - mPulseMinSpeed.mValue) * speedMultiplier;
            hit.rotation += deltaTime * currentSpeed;
            break;
        }
        
        case RotationMode::Wave:
        {
            float elapsed = static_cast<float>(NOW - hit.stateStartTime) / 1000.0f;
            float baseRotation = elapsed * 2.0f;
            float waveOffset = sinf(elapsed * mWaveFrequency.mValue * 6.28318f) * (mWaveAmplitude.mValue * 3.14159f / 180.0f);
            hit.rotation = baseRotation + waveOffset;
            break;
        }
        
        case RotationMode::Custom:
        {
            if (hit.isPaused)
            {
                hit.pauseTimer -= deltaTime;
                if (hit.pauseTimer <= 0.0f)
                {
                    hit.isPaused = false;
                    hit.rotationDirection *= -1.0f;
                    hit.rotationCycles = 0;
                }
            }
            else
            {
                float speed = mCustomSpeed.mValue;
                
                if (mCustomEasing.mValue)
                {
                    float cycleProgress = fmodf(fabsf(hit.rotation), 6.28318f) / 6.28318f;
                    
                    if (cycleProgress < 0.2f)
                    {
                        speed *= easeInOutCubic(cycleProgress / 0.2f);
                    }
                    else if (cycleProgress > 0.8f)
                    {
                        speed *= easeInOutCubic((1.0f - cycleProgress) / 0.2f);
                    }
                }
                
                hit.rotation += deltaTime * speed * hit.rotationDirection;
                
                float cyclesDone = fabsf(hit.rotation) / 6.28318f;
                if (cyclesDone >= mCustomCycles.mValue && hit.rotationCycles < static_cast<int>(mCustomCycles.mValue))
                {
                    hit.rotationCycles = static_cast<int>(cyclesDone);
                    
                    if (hit.rotationCycles >= static_cast<int>(mCustomCycles.mValue))
                    {
                        hit.isPaused = true;
                        hit.pauseTimer = mCustomPause.mValue;
                        hit.rotation = 0.0f;
                    }
                }
            }
            break;
        }
    }
}

// ==================== RENDERING ====================

void HitEffects::renderDiamond(const HitInfo& hit, const glm::vec2& screenPos, float scale, float alpha, ImColor color, float extraRotation)
{
    auto drawList = ImGui::GetBackgroundDrawList();
    
    float size = mDiamondSize.mValue * scale;
    float innerSize = size * mDiamondInnerSize.mValue;
    float thickness = mThickness.mValue * scale;
    
    ImColor mainColor = color;
    mainColor.Value.w *= alpha;
    
    ImColor innerColor = mainColor;
    innerColor.Value.w *= 0.7f;
    
    ImVec2 center(screenPos.x, screenPos.y);
    float totalRotation = hit.rotation + extraRotation;
    
    // Glow эффект
    if (mGlow.mValue && alpha > 0.1f)
    {
        ImColor glowColor = mainColor;
        glowColor.Value.w *= mGlowIntensity.mValue;
        
        for (int i = 0; i < 4; i++) {
            float angle = totalRotation + (i * 1.5708f);
            float nextAngle = totalRotation + ((i + 1) * 1.5708f);
            
            ImVec2 p1(center.x + cosf(angle) * (size + 3), center.y + sinf(angle) * (size + 3));
            ImVec2 p2(center.x + cosf(nextAngle) * (size + 3), center.y + sinf(nextAngle) * (size + 3));
            
            drawList->AddLine(p1, p2, glowColor, thickness + 4);
        }
    }
    
    // Основной ромб
    for (int i = 0; i < 4; i++) {
        float angle = totalRotation + (i * 1.5708f);
        float nextAngle = totalRotation + ((i + 1) * 1.5708f);
        
        ImVec2 p1(center.x + cosf(angle) * size, center.y + sinf(angle) * size);
        ImVec2 p2(center.x + cosf(nextAngle) * size, center.y + sinf(nextAngle) * size);
        
        drawList->AddLine(p1, p2, mainColor, thickness);
    }
    
    // Внутренний круг
    if (innerSize > 1.0f)
    {
        if (mGlow.mValue && alpha > 0.1f)
        {
            ImColor glowColor = innerColor;
            glowColor.Value.w *= mGlowIntensity.mValue;
            drawList->AddCircleFilled(center, innerSize + 2, glowColor);
        }
        
        drawList->AddCircleFilled(center, innerSize, innerColor);
        drawList->AddCircle(center, innerSize, mainColor, 0, thickness);
    }
}

void HitEffects::renderCorners(const HitInfo& hit, const glm::vec2& screenPos, float scale, float alpha, ImColor color, float extraRotation)
{
    auto drawList = ImGui::GetBackgroundDrawList();
    
    float size = mCornersSize.mValue * scale * mCornerGap.mValue;
    float cornerLen = size * mCornerLength.mValue;
    float thickness = mThickness.mValue * scale;
    
    ImColor mainColor = color;
    mainColor.Value.w *= alpha;
    
    ImVec2 center(screenPos.x, screenPos.y);
    float totalRotation = hit.rotation + extraRotation;
    
    // Вычисляем 4 угла
    float corners[4][2];
    
    for (int i = 0; i < 4; i++) {
        float baseAngle = (i * 1.5708f) + 0.7854f;
        float angle = totalRotation + baseAngle;
        
        corners[i][0] = cosf(angle) * size;
        corners[i][1] = sinf(angle) * size;
    }
    
    // Glow эффект
    if (mGlow.mValue && alpha > 0.1f)
    {
        ImColor glowColor = mainColor;
        glowColor.Value.w *= mGlowIntensity.mValue;
        
        for (int i = 0; i < 4; i++) {
            ImVec2 cornerPos(center.x + corners[i][0], center.y + corners[i][1]);
            
            int prevIdx = (i + 3) % 4;
            int nextIdx = (i + 1) % 4;
            
            float toPrevX = corners[prevIdx][0] - corners[i][0];
            float toPrevY = corners[prevIdx][1] - corners[i][1];
            float lenPrev = sqrtf(toPrevX * toPrevX + toPrevY * toPrevY);
            toPrevX = (toPrevX / lenPrev) * cornerLen;
            toPrevY = (toPrevY / lenPrev) * cornerLen;
            
            float toNextX = corners[nextIdx][0] - corners[i][0];
            float toNextY = corners[nextIdx][1] - corners[i][1];
            float lenNext = sqrtf(toNextX * toNextX + toNextY * toNextY);
            toNextX = (toNextX / lenNext) * cornerLen;
            toNextY = (toNextY / lenNext) * cornerLen;
            
            ImVec2 endPrev(cornerPos.x + toPrevX, cornerPos.y + toPrevY);
            ImVec2 endNext(cornerPos.x + toNextX, cornerPos.y + toNextY);
            
            drawList->AddLine(cornerPos, endPrev, glowColor, thickness + 3);
            drawList->AddLine(cornerPos, endNext, glowColor, thickness + 3);
        }
    }
    
    // Основные углы
    for (int i = 0; i < 4; i++) {
        ImVec2 cornerPos(center.x + corners[i][0], center.y + corners[i][1]);
        
        int prevIdx = (i + 3) % 4;
        int nextIdx = (i + 1) % 4;
        
        float toPrevX = corners[prevIdx][0] - corners[i][0];
        float toPrevY = corners[prevIdx][1] - corners[i][1];
        float lenPrev = sqrtf(toPrevX * toPrevX + toPrevY * toPrevY);
        toPrevX = (toPrevX / lenPrev) * cornerLen;
        toPrevY = (toPrevY / lenPrev) * cornerLen;
        
        float toNextX = corners[nextIdx][0] - corners[i][0];
        float toNextY = corners[nextIdx][1] - corners[i][1];
        float lenNext = sqrtf(toNextX * toNextX + toNextY * toNextY);
        toNextX = (toNextX / lenNext) * cornerLen;
        toNextY = (toNextY / lenNext) * cornerLen;
        
        ImVec2 endPrev(cornerPos.x + toPrevX, cornerPos.y + toPrevY);
        ImVec2 endNext(cornerPos.x + toNextX, cornerPos.y + toNextY);
        
        drawList->AddLine(cornerPos, endPrev, mainColor, thickness);
        drawList->AddLine(cornerPos, endNext, mainColor, thickness);
    }
}

void HitEffects::onRenderEvent(RenderEvent& event)
{
    // Auto-apply preset when changed
    if (mPreset.mValue != mLastPreset) {
        if (mPreset.mValue != Preset::Custom)
            applyPreset(mPreset.mValue);
        mLastPreset = mPreset.mValue;
    }

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) return;
    
    std::lock_guard<std::mutex> lock(mHitMutex);
    
    if (!mActiveHit.has_value()) return;
    
    HitInfo& hit = mActiveHit.value();
    float deltaTime = ImGui::GetIO().DeltaTime;
    
    // Обновляем состояние
    updateState(hit);
    
    // Проверяем, завершилось ли исчезновение
    if (hit.state == HitState::Despawning && hit.animProgress >= 1.0f)
    {
        mActiveHit.reset();
        return;
    }
    
    // Обновляем вращение
    updateRotation(hit, deltaTime);
    
    // Получаем позицию цели
    glm::vec3 targetPos;
    bool validTarget = false;
    float hitboxHeight = 1.8f;
    
    if (hit.target && hit.target->getRuntimeID() == hit.runtimeId)
    {
        try {
            targetPos = *hit.target->getPos();
            validTarget = true;
            
            auto shape = hit.target->getAABBShapeComponent();
            if (shape) {
                hitboxHeight = shape->mHeight;
            }
        } catch (...) {
            validTarget = false;
        }
    }
    
    if (!validTarget) {
        targetPos = hit.hitPosition;
    } else {
        hit.hitPosition = targetPos; // Обновляем сохранённую позицию
    }
    
    // Вычисляем позицию эффекта
    targetPos.y -= PLAYER_HEIGHT;
    targetPos.y += hitboxHeight * (mHeightPercent.mValue / 100.0f);
    targetPos.y += mPositionY.mValue;
    
    // Конвертируем в экранные координаты
    ImVec2 screenPosIm;
    if (!RenderUtils::worldToScreen(targetPos, screenPosIm)) {
        return;
    }
    glm::vec2 screenPos(screenPosIm.x, screenPosIm.y);
    
    // Вычисляем масштаб, альфу и дополнительное вращение в зависимости от состояния
    float scale = 1.0f;
    float alpha = 1.0f;
    float extraRotation = 0.0f;
    
    switch (hit.state)
    {
        case HitState::Spawning:
            scale = getSpawnScale(hit.animProgress);
            alpha = getSpawnAlpha(hit.animProgress);
            extraRotation = getSpawnExtraRotation(hit.animProgress);
            break;
            
        case HitState::Active:
            scale = 1.0f;
            alpha = 1.0f;
            extraRotation = 0.0f;
            break;
            
        case HitState::Despawning:
            scale = getDespawnScale(hit.animProgress);
            alpha = getDespawnAlpha(hit.animProgress);
            extraRotation = getDespawnExtraRotation(hit.animProgress);
            break;
    }
    
    // Масштаб по дистанции
    float distance = glm::distance(RenderUtils::transform.mOrigin, targetPos);
    if (distance > 0.1f) {
        float distanceScale = std::max(0.3f, 15.0f / distance);
        scale *= distanceScale;
    }
    
    // Получаем цвет
    ImColor color = getColor(0.0f);
    
    // Рендерим эффекты
    if (mStyle.mValue == Style::Diamond || mStyle.mValue == Style::Both) {
        renderDiamond(hit, screenPos, scale, alpha, color, extraRotation);
    }
    
    if (mStyle.mValue == Style::Corners || mStyle.mValue == Style::Both) {
        float cornerScale = (mStyle.mValue == Style::Both) ? scale * 1.3f : scale;
        renderCorners(hit, screenPos, cornerScale, alpha, color, extraRotation);
    }
}

// ==================== PACKET HANDLERS ====================

void HitEffects::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;
    
    auto pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt->mTransaction) return;
    
    auto cit = pkt->mTransaction.get();
    
    if (cit->type == ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction)
    {
        auto iut = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(cit);
        
        if (iut->mActionType == ItemUseOnActorInventoryTransaction::ActionType::Attack)
        {
            auto player = ClientInstance::get()->getLocalPlayer();
            if (!player) return;
            
            int64_t targetRuntimeId = iut->mActorId;
            
            auto actors = ActorUtils::getActorList(false, true);
            for (auto actor : actors) {
                if (actor && actor->getRuntimeID() == targetRuntimeId) {
                    processHit(actor);
                    break;
                }
            }
        }
    }
}

void HitEffects::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() == PacketID::RemoveActor)
    {
        auto packet = event.getPacket<RemoveActorPacket>();
        
        std::lock_guard<std::mutex> lock(mHitMutex);
        if (mActiveHit.has_value() && mActiveHit->runtimeId == packet->mRuntimeID)
        {
            // Цель удалена - начинаем исчезновение
            if (mActiveHit->state != HitState::Despawning)
            {
                mActiveHit->state = HitState::Despawning;
                mActiveHit->stateStartTime = NOW;
                mActiveHit->animProgress = 0.0f;
            }
            mActiveHit->target = nullptr;
        }
    }
}