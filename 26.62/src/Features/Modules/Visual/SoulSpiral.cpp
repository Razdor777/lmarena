//
// SoulSpiral - Spirit effect implementation
//

#include "SoulSpiral.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/RemoveActorPacket.hpp>

// ==================== EASING ====================

float SoulSpiral::easeOutCubic(float t) {
    return 1.0f - powf(1.0f - t, 3.0f);
}

float SoulSpiral::easeInCubic(float t) {
    return t * t * t;
}

float SoulSpiral::easeInOutSine(float t) {
    return -(cosf(3.14159f * t) - 1.0f) / 2.0f;
}

float SoulSpiral::easeOutElastic(float t) {
    if (t == 0 || t == 1) return t;
    float p = 0.3f;
    float s = p / 4.0f;
    return powf(2.0f, -10.0f * t) * sinf((t - s) * (2.0f * 3.14159f) / p) + 1.0f;
}

// ==================== COLOR ====================

ImColor SoulSpiral::lerpColor(ImColor a, ImColor b, float t) {
    return ImColor(
        a.Value.x + (b.Value.x - a.Value.x) * t,
        a.Value.y + (b.Value.y - a.Value.y) * t,
        a.Value.z + (b.Value.z - a.Value.z) * t,
        a.Value.w + (b.Value.w - a.Value.w) * t
    );
}

ImColor SoulSpiral::getColor(float progress, float timeOffset) {
    if (mRainbow.mValue) {
        float time = ImGui::GetTime() * mRainbowSpeed.mValue + timeOffset;
        float r = (sinf(time) + 1.0f) / 2.0f;
        float g = (sinf(time + 2.094f) + 1.0f) / 2.0f;
        float b = (sinf(time + 4.189f) + 1.0f) / 2.0f;
        return ImColor(r, g, b, mColorA.mValue);
    }
    
    ImColor color1(mColorR.mValue, mColorG.mValue, mColorB.mValue, mColorA.mValue);
    
    if (mUseGradient.mValue) {
        ImColor color2(mColor2R.mValue, mColor2G.mValue, mColor2B.mValue, mColorA.mValue);
        return lerpColor(color1, color2, progress);
    }
    
    return color1;
}

// ==================== LIFECYCLE ====================

void SoulSpiral::onEnable() {
    gFeatureManager->mDispatcher->listen<RenderEvent, &SoulSpiral::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &SoulSpiral::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &SoulSpiral::onPacketInEvent>(this);
    
    std::lock_guard<std::mutex> lock(mSoulMutex);
    mActiveSoul.reset();
}

void SoulSpiral::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &SoulSpiral::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &SoulSpiral::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &SoulSpiral::onPacketInEvent>(this);
    
    std::lock_guard<std::mutex> lock(mSoulMutex);
    mActiveSoul.reset();
}

// ==================== HIT PROCESSING ====================

void SoulSpiral::processHit(Actor* target) {
    if (!target) return;
    
    std::lock_guard<std::mutex> lock(mSoulMutex);
    
    int64_t targetRuntimeId = target->getRuntimeID();
    
    // Та же цель - продлеваем
    if (mActiveSoul.has_value() && mActiveSoul->runtimeId == targetRuntimeId) {
        mActiveSoul->lastHitTime = NOW;
        
        if (mActiveSoul->isDespawning) {
            mActiveSoul->isDespawning = false;
            mActiveSoul->isActive = true;
        }
        return;
    }
    
    // Новая цель
    SoulInfo newSoul;
    newSoul.target = target;
    newSoul.runtimeId = targetRuntimeId;
    newSoul.startTime = NOW;
    newSoul.lastHitTime = NOW;
    newSoul.lastTargetPos = *target->getPos();
    newSoul.progress = 0.0f;
    newSoul.currentAngle = 0.0f;
    newSoul.trail.clear();
    newSoul.isActive = true;
    newSoul.spawnScale = 0.0f;
    newSoul.despawnAlpha = 1.0f;
    newSoul.isDespawning = false;
    
    mActiveSoul = newSoul;
}

// ==================== UPDATE ====================

void SoulSpiral::updateSoul(SoulInfo& soul, float deltaTime) {
    uint64_t now = NOW;
    float elapsed = static_cast<float>(now - soul.startTime) / 1000.0f;
    float timeSinceHit = static_cast<float>(now - soul.lastHitTime) / 1000.0f;
    
    // Анимация появления
    if (soul.spawnScale < 1.0f) {
        float spawnProgress = std::min(1.0f, elapsed / mSpawnTime.mValue);
        soul.spawnScale = easeOutElastic(spawnProgress);
    }
    
    // Проверка на despawn
    if (timeSinceHit >= mDuration.mValue && !soul.isDespawning) {
        soul.isDespawning = true;
        soul.despawnAlpha = 1.0f;
    }
    
    // Анимация исчезновения
    if (soul.isDespawning) {
        soul.despawnAlpha -= deltaTime / mDespawnTime.mValue;
        if (soul.despawnAlpha <= 0.0f) {
            soul.isActive = false;
            return;
        }
    }
    
    // Обновление угла спирали
    soul.currentAngle += deltaTime * mSpiralSpeed.mValue;
    
    // Обновление прогресса (позиция по высоте)
    float totalDuration = (mSpiralLoops.mValue * 2.0f * 3.14159f) / mSpiralSpeed.mValue;
    
    switch (mMovementStyle.mValue) {
        case MovementStyle::SpiralUp:
            soul.progress = fmodf(elapsed / totalDuration, 1.0f);
            break;
            
        case MovementStyle::SpiralDown:
            soul.progress = 1.0f - fmodf(elapsed / totalDuration, 1.0f);
            break;
            
        case MovementStyle::SpiralBoth: {
            float cycle = fmodf(elapsed / totalDuration, 2.0f);
            soul.progress = (cycle < 1.0f) ? cycle : (2.0f - cycle);
            break;
        }
            
        case MovementStyle::Orbit:
            soul.progress = 0.5f; // Фиксированная высота посередине
            break;
            
        case MovementStyle::Chaotic: {
            float chaos1 = sinf(elapsed * 1.7f) * 0.3f;
            float chaos2 = cosf(elapsed * 2.3f) * 0.2f;
            soul.progress = 0.5f + chaos1 + chaos2;
            soul.progress = std::clamp(soul.progress, 0.0f, 1.0f);
            break;
        }
    }
    
    // Looping
    if (mLooping.mValue && timeSinceHit < mDuration.mValue) {
        // Продолжаем анимацию пока бьём
    }
}

glm::vec3 SoulSpiral::calculateSoulPosition(const SoulInfo& soul, const glm::vec3& targetPos, float hitboxHeight) {
    float startH = hitboxHeight * (mStartHeight.mValue / 100.0f);
    float endH = hitboxHeight * (mEndHeight.mValue / 100.0f);
    float currentHeight = startH + (endH - startH) * soul.progress;
    
    float radius = mSpiralRadius.mValue;
    
    // Добавляем вариации радиуса для некоторых стилей
    if (mMovementStyle.mValue == MovementStyle::Chaotic) {
        radius *= (1.0f + sinf(soul.currentAngle * 1.5f) * 0.3f);
    }
    
    float x = targetPos.x + cosf(soul.currentAngle) * radius;
    float y = targetPos.y - 1.62f + currentHeight;
    float z = targetPos.z + sinf(soul.currentAngle) * radius;
    
    return glm::vec3(x, y, z);
}

// ==================== RENDERING ====================

void SoulSpiral::renderSoul(const SoulInfo& soul, const glm::vec2& screenPos, float scale, float alpha, ImColor color) {
    auto drawList = ImGui::GetBackgroundDrawList();
    
    // Пульсация
    float pulse = 1.0f;
    if (mSoulPulse.mValue > 0.01f) {
        pulse = 1.0f + sinf(ImGui::GetTime() * mSoulPulseSpeed.mValue) * mSoulPulse.mValue;
    }
    
    float size = mSoulSize.mValue * scale * pulse * soul.spawnScale;
    ImVec2 center(screenPos.x, screenPos.y);
    
    ImColor mainColor = color;
    mainColor.Value.w *= alpha * soul.despawnAlpha;
    
    // Glow эффект
    if (mSoulGlow.mValue > 0.01f) {
        ImColor glowColor = mainColor;
        glowColor.Value.w *= mSoulGlow.mValue * 0.5f;
        
        for (int i = 3; i >= 1; i--) {
            float glowSize = size + (i * 4);
            drawList->AddCircleFilled(center, glowSize, glowColor, 32);
            glowColor.Value.w *= 0.5f;
        }
    }
    
    switch (mSoulStyle.mValue) {
        case SoulStyle::Orb:
            // Основной круг
            drawList->AddCircleFilled(center, size, mainColor, 32);
            // Блик
            {
                ImColor highlightColor = mainColor;
                highlightColor.Value.x = std::min(1.0f, highlightColor.Value.x + 0.3f);
                highlightColor.Value.y = std::min(1.0f, highlightColor.Value.y + 0.3f);
                highlightColor.Value.z = std::min(1.0f, highlightColor.Value.z + 0.3f);
                drawList->AddCircleFilled(ImVec2(center.x - size * 0.3f, center.y - size * 0.3f), size * 0.3f, highlightColor, 16);
            }
            break;
            
        case SoulStyle::Flame: {
            // Огонёк - треугольник с колебаниями
            float time = ImGui::GetTime() * 5.0f;
            float flicker = sinf(time) * 0.2f + sinf(time * 1.7f) * 0.1f;
            
            ImVec2 p1(center.x, center.y - size * (1.5f + flicker));
            ImVec2 p2(center.x - size * 0.7f, center.y + size * 0.5f);
            ImVec2 p3(center.x + size * 0.7f, center.y + size * 0.5f);
            
            // Внешнее пламя
            drawList->AddTriangleFilled(p1, p2, p3, mainColor);
            
            // Внутреннее пламя (ярче)
            ImColor innerColor = mainColor;
            innerColor.Value.x = std::min(1.0f, innerColor.Value.x + 0.4f);
            innerColor.Value.y = std::min(1.0f, innerColor.Value.y + 0.3f);
            innerColor.Value.w *= 0.8f;
            
            ImVec2 ip1(center.x, center.y - size * (0.8f + flicker * 0.5f));
            ImVec2 ip2(center.x - size * 0.35f, center.y + size * 0.3f);
            ImVec2 ip3(center.x + size * 0.35f, center.y + size * 0.3f);
            drawList->AddTriangleFilled(ip1, ip2, ip3, innerColor);
            break;
        }
            
        case SoulStyle::Star: {
            // Исправленная звёздочка - 5 лучей
            float rotation = ImGui::GetTime() * 2.0f;
            const int numPoints = 5;
            const float outerRadius = size;
            const float innerRadius = size * 0.4f;
            
            std::vector<ImVec2> starPoints;
            starPoints.reserve(numPoints * 2);
            
            for (int i = 0; i < numPoints * 2; i++) {
                float angle = rotation + (i * 3.14159f / numPoints) - (3.14159f / 2.0f);
                float r = (i % 2 == 0) ? outerRadius : innerRadius;
                starPoints.push_back(ImVec2(
                    center.x + cosf(angle) * r,
                    center.y + sinf(angle) * r
                ));
            }
            
            // Рисуем звезду треугольниками от центра
            for (int i = 0; i < numPoints * 2; i++) {
                int next = (i + 1) % (numPoints * 2);
                drawList->AddTriangleFilled(center, starPoints[i], starPoints[next], mainColor);
            }
            break;
        }
            
        case SoulStyle::Diamond: {
            // Ромбик
            float rotation = ImGui::GetTime() * 1.5f;
            ImVec2 points[4];
            for (int i = 0; i < 4; i++) {
                float angle = rotation + (i * 1.5708f);
                points[i] = ImVec2(center.x + cosf(angle) * size, center.y + sinf(angle) * size);
            }
            drawList->AddQuadFilled(points[0], points[1], points[2], points[3], mainColor);
            break;
        }
    }
}

void SoulSpiral::renderTrail(const SoulInfo& soul, float alpha) {
    if (mTrailStyle.mValue == TrailStyle::None) return;
    if (soul.trail.size() < 2) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    auto ci = ClientInstance::get();
    auto corrected = RenderUtils::transform.mMatrix;
    
    float baseAlpha = alpha * soul.despawnAlpha;
    
    for (size_t i = 1; i < soul.trail.size(); i++) {
        const TrailPoint& prev = soul.trail[i - 1];
        const TrailPoint& curr = soul.trail[i];
        
        // Конвертируем в экранные координаты
        glm::vec2 prevScreen, currScreen;
        if (!corrected.OWorldToScreen(RenderUtils::transform.mOrigin, prev.worldPos, prevScreen, MathUtils::fov, ci->getGuiData()->mResolution)) continue;
        if (!corrected.OWorldToScreen(RenderUtils::transform.mOrigin, curr.worldPos, currScreen, MathUtils::fov, ci->getGuiData()->mResolution)) continue;
        
        float progress = static_cast<float>(i) / soul.trail.size();
        ImColor color = getColor(progress, progress * 0.5f);
        
        float pointAlpha = curr.alpha * baseAlpha;
        
        switch (mTrailStyle.mValue) {
            case TrailStyle::Solid:
                color.Value.w = pointAlpha * 0.8f;
                drawList->AddLine(ImVec2(prevScreen.x, prevScreen.y), ImVec2(currScreen.x, currScreen.y), color, mTrailWidth.mValue * curr.size);
                break;
                
            case TrailStyle::Dots:
                color.Value.w = pointAlpha;
                drawList->AddCircleFilled(ImVec2(currScreen.x, currScreen.y), mTrailWidth.mValue * curr.size * 0.5f, color);
                break;
                
            case TrailStyle::Fading:
                color.Value.w = pointAlpha * (1.0f - progress);
                drawList->AddLine(ImVec2(prevScreen.x, prevScreen.y), ImVec2(currScreen.x, currScreen.y), color, mTrailWidth.mValue * curr.size * (1.0f - progress * 0.5f));
                break;
                
            case TrailStyle::Smoke: {
                // Дымка - много маленьких кругов
                color.Value.w = pointAlpha * 0.3f * (1.0f - progress);
                float smokeSize = mTrailWidth.mValue * curr.size * (1.0f + progress);
                drawList->AddCircleFilled(ImVec2(currScreen.x, currScreen.y), smokeSize, color, 16);
                
                // Дополнительные частицы дыма
                float offset = sinf(i * 1.5f) * 3.0f;
                drawList->AddCircleFilled(ImVec2(currScreen.x + offset, currScreen.y - offset), smokeSize * 0.7f, color, 12);
                break;
            }
                
            default:
                break;
        }
        
        // Glow для trail
        if (mTrailGlow.mValue && mTrailStyle.mValue != TrailStyle::Smoke) {
            ImColor glowColor = color;
            glowColor.Value.w *= 0.3f;
            drawList->AddCircleFilled(ImVec2(currScreen.x, currScreen.y), mTrailWidth.mValue * curr.size * 2.0f, glowColor);
        }
    }
}

void SoulSpiral::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    auto ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) return;
    
    std::lock_guard<std::mutex> lock(mSoulMutex);
    
    if (!mActiveSoul.has_value()) return;
    
    SoulInfo& soul = mActiveSoul.value();
    float deltaTime = ImGui::GetIO().DeltaTime;
    
    // Обновляем состояние
    updateSoul(soul, deltaTime);
    
    // Проверяем активность
    if (!soul.isActive) {
        mActiveSoul.reset();
        return;
    }
    
    // Получаем позицию цели
    glm::vec3 targetPos;
    float hitboxHeight = 1.8f;
    bool validTarget = false;
    
    if (soul.target && soul.target->getRuntimeID() == soul.runtimeId) {
        try {
            targetPos = *soul.target->getPos();
            validTarget = true;
            
            auto shape = soul.target->getAABBShapeComponent();
            if (shape) {
                hitboxHeight = shape->mHeight;
            }
        } catch (...) {
            validTarget = false;
        }
    }
    
    if (!validTarget) {
        targetPos = soul.lastTargetPos;
    } else {
        soul.lastTargetPos = targetPos;
    }
    
    // Вычисляем позицию души
    glm::vec3 soulWorldPos = calculateSoulPosition(soul, targetPos, hitboxHeight);
    
    // Добавляем точку в trail
    TrailPoint newPoint;
    newPoint.worldPos = soulWorldPos;
    newPoint.alpha = 1.0f;
    newPoint.size = 1.0f;
    newPoint.creationTime = NOW;
    
    soul.trail.push_front(newPoint);
    
    // Ограничиваем длину trail и обновляем fade
    while (soul.trail.size() > static_cast<size_t>(mTrailLength.mValue)) {
        soul.trail.pop_back();
    }
    
    // Обновляем alpha для каждой точки trail
    for (size_t i = 0; i < soul.trail.size(); i++) {
        float age = static_cast<float>(i) / soul.trail.size();
        soul.trail[i].alpha = powf(1.0f - age, mTrailFade.mValue);
        soul.trail[i].size = 1.0f - age * 0.5f;
    }
    
    // Рендерим trail
    renderTrail(soul, 1.0f);
    
    // Конвертируем позицию души в экранные координаты
    auto corrected = RenderUtils::transform.mMatrix;
    glm::vec2 screenPos;
    if (!corrected.OWorldToScreen(
            RenderUtils::transform.mOrigin,
            soulWorldPos, screenPos, MathUtils::fov,
            ci->getGuiData()->mResolution)) {
        return;
    }
    
    // Проверяем границы
    if (screenPos.x < -100 || screenPos.y < -100 || 
        screenPos.x > ci->getGuiData()->mResolution.x + 100 || 
        screenPos.y > ci->getGuiData()->mResolution.y + 100) {
        return;
    }
    
    // Масштаб по дистанции
    float distance = glm::distance(RenderUtils::transform.mOrigin, soulWorldPos);
    float scale = std::max(0.3f, 10.0f / distance);
    
    // Получаем цвет
    ImColor color = getColor(soul.progress, 0.0f);
    
    // Рендерим душу
    renderSoul(soul, screenPos, scale, 1.0f, color);
}

// ==================== PACKET HANDLERS ====================

void SoulSpiral::onPacketOutEvent(PacketOutEvent& event) {
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;
    
    auto pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt->mTransaction) return;
    
    auto cit = pkt->mTransaction.get();
    
    if (cit->type == ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) {
        auto iut = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(cit);
        
        if (iut->mActionType == ItemUseOnActorInventoryTransaction::ActionType::Attack) {
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

void SoulSpiral::onPacketInEvent(PacketInEvent& event) {
    if (event.mPacket->getId() == PacketID::RemoveActor) {
        auto packet = event.getPacket<RemoveActorPacket>();
        
        std::lock_guard<std::mutex> lock(mSoulMutex);
        if (mActiveSoul.has_value() && mActiveSoul->runtimeId == packet->mRuntimeID) {
            if (!mActiveSoul->isDespawning) {
                mActiveSoul->isDespawning = true;
            }
            mActiveSoul->target = nullptr;
        }
    }
}