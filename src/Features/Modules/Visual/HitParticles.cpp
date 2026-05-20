//
// HitParticles — Particle effects on hit
//

#include "HitParticles.hpp"

#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace {
glm::vec3 findAttackParticlePos(Actor* player)
{
    auto fallback = player->getPos();
    glm::vec3 fallbackPos = fallback ? (*fallback + glm::vec3(0.f, 0.8f, 0.f)) : glm::vec3(0.f);

    auto level = player->getLevel();
    if (!level) return fallbackPos;

    // HitResult is safer than iterating runtime actors during packet dispatch.
    auto hr = level->getHitResult();
    if (!hr) return fallbackPos;
    if (hr->mType == HitType::ENTITY) {
        return hr->mPos;
    }
    return fallbackPos;
}
}

void HitParticles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &HitParticles::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &HitParticles::onPacketOutEvent>(this);
    
    std::lock_guard<std::mutex> lock(mParticleMutex);
    mParticles.clear();
}

void HitParticles::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &HitParticles::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &HitParticles::onPacketOutEvent>(this);
    
    std::lock_guard<std::mutex> lock(mParticleMutex);
    mParticles.clear();
}

// ==================== HELPERS ====================

float HitParticles::randFloat(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(mRng);
}

glm::vec3 HitParticles::randDir()
{
    float theta = randFloat(0.0f, 6.28318f);
    float phi = randFloat(-1.0f, 1.0f);
    float r = sqrtf(1.0f - phi * phi);
    return glm::vec3(r * cosf(theta), fabsf(phi) * 0.5f + 0.5f, r * sinf(theta));
}

ImColor HitParticles::randPresetColor(Preset preset, float variation)
{
    switch (preset) {
        case Preset::Sparks:
            return ImColor(
                1.0f,
                0.5f + randFloat(-0.2f, 0.3f),
                0.05f + randFloat(0.0f, 0.15f),
                1.0f);
        case Preset::Blood:
            return ImColor(
                0.7f + randFloat(0.0f, 0.3f),
                0.0f + randFloat(0.0f, 0.1f),
                0.0f + randFloat(0.0f, 0.05f),
                1.0f);
        case Preset::Stars: {
            float hue = randFloat(0.0f, 1.0f);
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.0f, r, g, b);
            return ImColor(r, g, b, 1.0f);
        }
        case Preset::Hearts:
            return ImColor(
                0.9f + randFloat(0.0f, 0.1f),
                0.2f + randFloat(0.0f, 0.3f),
                0.3f + randFloat(0.0f, 0.2f),
                1.0f);
        case Preset::Snow:
            return ImColor(
                0.85f + randFloat(0.0f, 0.15f),
                0.9f + randFloat(0.0f, 0.1f),
                1.0f,
                1.0f);
        case Preset::Fire: {
            float t = randFloat(0.0f, 1.0f);
            return ImColor(
                1.0f,
                0.2f + t * 0.6f,
                0.0f + t * 0.1f,
                1.0f);
        }
        case Preset::Shatter:
            return ImColor(
                0.7f + randFloat(0.0f, 0.3f),
                0.8f + randFloat(0.0f, 0.2f),
                1.0f,
                1.0f);
        case Preset::Custom:
        default:
            if (mRainbow.mValue) {
                float hue = fmodf(variation * 0.1f + randFloat(0.0f, 0.2f), 1.0f);
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.0f, r, g, b);
                return ImColor(r, g, b, 1.0f);
            }
            return ImColor(mColorR.mValue, mColorG.mValue, mColorB.mValue, 1.0f);
    }
}

// ==================== PARTICLE SPAWN ====================

void HitParticles::spawnPresetParticles(const glm::vec3& pos, Preset preset)
{
    int count;
    float size, speed, lifetime, gravity;
    
    switch (preset) {
        case Preset::Sparks:
            count = 15; size = 2.5f; speed = 6.0f; lifetime = 0.6f; gravity = 3.0f;
            break;
        case Preset::Blood:
            count = 10; size = 3.0f; speed = 3.5f; lifetime = 1.0f; gravity = 12.0f;
            break;
        case Preset::Stars:
            count = 8; size = 5.0f; speed = 3.0f; lifetime = 1.2f; gravity = 1.0f;
            break;
        case Preset::Hearts:
            count = 6; size = 6.0f; speed = 2.5f; lifetime = 1.5f; gravity = -1.5f; // Float up!
            break;
        case Preset::Snow:
            count = 20; size = 2.0f; speed = 2.0f; lifetime = 2.0f; gravity = 1.5f;
            break;
        case Preset::Fire:
            count = 18; size = 3.5f; speed = 4.0f; lifetime = 0.8f; gravity = -4.0f; // Rise up
            break;
        case Preset::Shatter:
            count = 12; size = 4.0f; speed = 5.0f; lifetime = 0.7f; gravity = 8.0f;
            break;
        case Preset::Custom:
        default:
            count = (int)mCount.mValue;
            size = mSize.mValue;
            speed = mSpeed.mValue;
            lifetime = mLifetime.mValue;
            gravity = mGravity.mValue;
            break;
    }
    
    for (int i = 0; i < count; i++) {
        HitParticle p;
        p.position = pos + glm::vec3(randFloat(-0.3f, 0.3f), randFloat(0.0f, 0.8f), randFloat(-0.3f, 0.3f));
        
        glm::vec3 dir = randDir();
        float spd = speed * randFloat(0.5f, 1.2f);
        p.velocity = dir * spd;
        
        // Snow: add lateral drift
        if (preset == Preset::Snow) {
            p.velocity.x += randFloat(-1.5f, 1.5f);
            p.velocity.z += randFloat(-1.5f, 1.5f);
        }
        // Hearts: mostly upward
        if (preset == Preset::Hearts) {
            p.velocity.y = fabsf(p.velocity.y) * 1.5f + 1.0f;
            p.velocity.x *= 0.5f;
            p.velocity.z *= 0.5f;
        }
        
        p.lifetime = 0.0f;
        p.maxLifetime = lifetime * randFloat(0.7f, 1.3f);
        p.size = size * randFloat(0.6f, 1.4f);
        p.rotation = randFloat(0.0f, 360.0f);
        p.rotationSpeed = randFloat(-360.0f, 360.0f);
        p.gravity = gravity;
        p.color = randPresetColor(preset, (float)i);
        
        mParticles.push_back(p);
    }
}

void HitParticles::spawnParticles(const glm::vec3& position)
{
    std::lock_guard<std::mutex> lock(mParticleMutex);
    
    // Prevent FPS drops from too many particles
    constexpr size_t MAX_PARTICLES = 250;
    if (mParticles.size() >= MAX_PARTICLES) {
        // Remove oldest particles to make room
        size_t toRemove = mParticles.size() - MAX_PARTICLES + 20;
        mParticles.erase(mParticles.begin(), mParticles.begin() + std::min(toRemove, mParticles.size()));
    }
    
    spawnPresetParticles(position, mPreset.mValue);
}

// ==================== PACKET DETECTION ====================

void HitParticles::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;
    
    auto pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt || !pkt->mTransaction) return;
    
    auto complex = pkt->mTransaction.get();
    if (complex->type != ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) return;
    
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;
    auto level = player->getLevel();
    if (!level) return;
    
    // Some versions deserialize this as base ComplexInventoryTransaction only.
    // In that case reading subclass fields (mActorId/mActionType) is unsafe.
    // Use current hit result to avoid bad casts and runtime actor races.
    glm::vec3 hitPos = findAttackParticlePos(player);
    spawnParticles(hitPos);
}

// ==================== SHAPE RENDERERS ====================

void HitParticles::renderDiamond(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha)
{
    color.Value.w = alpha;
    float rad = rotation * 0.0174533f;
    float c = cosf(rad), s = sinf(rad);
    
    ImVec2 points[4] = {
        {center.x + size * c, center.y + size * s},
        {center.x - size * s, center.y + size * c},
        {center.x - size * c, center.y - size * s},
        {center.x + size * s, center.y - size * c},
    };
    dl->AddConvexPolyFilled(points, 4, color);
}

void HitParticles::renderStar(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha)
{
    color.Value.w = alpha;
    float rad = rotation * 0.0174533f;
    
    ImVec2 points[10];
    for (int i = 0; i < 10; i++) {
        float angle = rad + (float)i * 0.6283185f; // 36 degrees
        float r = (i % 2 == 0) ? size : size * 0.4f;
        points[i] = ImVec2(center.x + r * cosf(angle), center.y + r * sinf(angle));
    }
    dl->AddConvexPolyFilled(points, 10, color);
}

void HitParticles::renderHeart(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha)
{
    color.Value.w = alpha;
    float s = size * 0.6f;
    
    // Simple heart using circles + triangle
    dl->AddCircleFilled(ImVec2(center.x - s * 0.3f, center.y - s * 0.15f), s * 0.4f, color, 12);
    dl->AddCircleFilled(ImVec2(center.x + s * 0.3f, center.y - s * 0.15f), s * 0.4f, color, 12);
    ImVec2 tri[3] = {
        {center.x - s * 0.65f, center.y},
        {center.x + s * 0.65f, center.y},
        {center.x, center.y + s * 0.7f},
    };
    dl->AddConvexPolyFilled(tri, 3, color);
}

void HitParticles::renderDot(ImDrawList* dl, ImVec2 center, float size, ImColor color, float alpha, bool glow)
{
    if (glow) {
        ImColor glowColor = color;
        glowColor.Value.w = alpha * 0.2f;
        dl->AddCircleFilled(center, size * 2.0f, glowColor, 12);
    }
    color.Value.w = alpha;
    dl->AddCircleFilled(center, size, color, 8);
}

// ==================== MAIN RENDER ====================

void HitParticles::onRenderEvent(RenderEvent& event)
{
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;
    auto playerPosPtr = player->getPos();
    if (!playerPosPtr) return;
    
    auto drawList = ImGui::GetBackgroundDrawList();
    float delta = ImGui::GetIO().DeltaTime;
    
    std::lock_guard<std::mutex> lock(mParticleMutex);
    
    // Update & render
    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        auto& p = *it;
        
        // Update physics
        p.lifetime += delta;
        p.velocity.y -= p.gravity * delta;
        p.position += p.velocity * delta;
        p.rotation += p.rotationSpeed * delta;
        
        // Friction
        p.velocity *= (1.0f - delta * 1.5f);
        
        if (p.isDead()) {
            it = mParticles.erase(it);
            continue;
        }
        
        // Project to screen
        ImVec2 screenPos;
        if (!RenderUtils::worldToScreen(p.position, screenPos)) {
            ++it;
            continue;
        }
        
        float progress = p.getProgress();
        
        // Fade out in last 30% of life
        float alpha = 1.0f;
        if (progress > 0.7f) {
            alpha = 1.0f - ((progress - 0.7f) / 0.3f);
        }
        // Fade in first 10%
        if (progress < 0.1f) {
            alpha = progress / 0.1f;
        }
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        
        // Scale down with distance
        float dist = glm::distance(*playerPosPtr, p.position);
        float distScale = std::clamp(8.0f / (dist + 1.0f), 0.3f, 2.0f);
        float renderSize = p.size * distScale;
        
        // Size pulse at start
        if (progress < 0.15f) {
            float t = progress / 0.15f;
            renderSize *= 0.5f + t * 0.5f + sinf(t * 3.14159f) * 0.3f;
        }
        // Shrink at end
        if (progress > 0.8f) {
            renderSize *= 1.0f - ((progress - 0.8f) / 0.2f) * 0.5f;
        }
        
        ImVec2 screenPt(screenPos.x, screenPos.y);
        
        // Render based on preset shape
        Preset preset = mPreset.mValue;
        bool useGlow = (preset == Preset::Custom) ? mGlow.mValue : true;
        
        switch (preset) {
            case Preset::Sparks:
                renderDot(drawList, screenPt, renderSize * 0.7f, p.color, alpha, true);
                break;
            case Preset::Blood:
                renderDot(drawList, screenPt, renderSize * 0.8f, p.color, alpha, false);
                break;
            case Preset::Stars:
                renderStar(drawList, screenPt, renderSize, p.rotation, p.color, alpha);
                break;
            case Preset::Hearts:
                renderHeart(drawList, screenPt, renderSize, p.rotation, p.color, alpha);
                break;
            case Preset::Snow:
                renderDiamond(drawList, screenPt, renderSize * 0.5f, p.rotation, p.color, alpha);
                break;
            case Preset::Fire:
                renderDot(drawList, screenPt, renderSize * 0.6f, p.color, alpha, true);
                break;
            case Preset::Shatter:
                renderDiamond(drawList, screenPt, renderSize * 0.8f, p.rotation, p.color, alpha);
                break;
            case Preset::Custom:
            default:
                renderDot(drawList, screenPt, renderSize * 0.7f, p.color, alpha, useGlow);
                break;
        }
        
        ++it;
    }
}
