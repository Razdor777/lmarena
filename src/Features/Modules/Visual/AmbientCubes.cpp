//
// AmbientCubes.cpp — Screen-space ambient particles
//
// Renders soft circles directly on screen (2D overlay).
// No world-space dependency — always visible regardless of camera
// position, pearl distance, or flight speed.
//

#include "AmbientCubes.hpp"

#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void AmbientCubes::onEnable() {
    particles.clear();
    spawnTimer = 0;
}

void AmbientCubes::onDisable() {
    particles.clear();
}

ImColor AmbientCubes::getColor() {
    if (mUseThemeColor.mValue) {
        return ColorUtils::getThemedColor(0.f);
    }
    return mCustomColor.getAsImColor();
}

void AmbientCubes::spawnParticle(float screenW, float screenH) {
    AmbientParticle p;

    // Spawn position based on selected area
    switch (mSpawnArea.mValue) {
    case SpawnArea::Center: {
        float spread = 0.3f;
        p.position.x = screenW * (0.5f + ((rand() % 200 - 100) / 100.f) * spread);
        p.position.y = screenH * (0.5f + ((rand() % 200 - 100) / 100.f) * spread);
        break;
    }
    case SpawnArea::Edges: {
        int side = rand() % 4;
        float margin = 30.f;
        switch (side) {
        case 0: p.position = {margin + (float)(rand() % (int)(screenW - 2*margin)), margin}; break;
        case 1: p.position = {margin + (float)(rand() % (int)(screenW - 2*margin)), screenH - margin}; break;
        case 2: p.position = {margin, margin + (float)(rand() % (int)(screenH - 2*margin))}; break;
        case 3: p.position = {screenW - margin, margin + (float)(rand() % (int)(screenH - 2*margin))}; break;
        }
        break;
    }
    case SpawnArea::Random: {
        p.position.x = (float)(rand() % (int)screenW);
        p.position.y = (float)(rand() % (int)screenH);
        break;
    }
    case SpawnArea::Corners: {
        int corner = rand() % 4;
        float cornerSize = 0.2f;
        switch (corner) {
        case 0: p.position = {screenW * cornerSize * ((float)rand()/RAND_MAX), screenH * cornerSize * ((float)rand()/RAND_MAX)}; break;
        case 1: p.position = {screenW * (1.f - cornerSize * ((float)rand()/RAND_MAX)), screenH * cornerSize * ((float)rand()/RAND_MAX)}; break;
        case 2: p.position = {screenW * cornerSize * ((float)rand()/RAND_MAX), screenH * (1.f - cornerSize * ((float)rand()/RAND_MAX))}; break;
        case 3: p.position = {screenW * (1.f - cornerSize * ((float)rand()/RAND_MAX)), screenH * (1.f - cornerSize * ((float)rand()/RAND_MAX))}; break;
        }
        break;
    }
    }

    // Gentle drift velocity (pixels per second)
    float speedMul = mSpeed.mValue * 15.f;
    p.velocity.x = ((rand() % 100) - 50) / 50.f * speedMul;
    p.velocity.y = ((rand() % 100) - 50) / 50.f * speedMul;

    float baseSize = mSize.mValue;
    float variation = mSizeVariation.mValue;
    p.size = baseSize + ((rand() % 100) / 100.f - 0.5f) * variation * baseSize;
    p.size = std::max(p.size, 1.5f);

    p.alpha = mAlpha.mValue;
    p.maxLifetime = mLifetime.mValue + ((rand() % 40) / 10.f - 2.f);
    p.lifetime = p.maxLifetime;
    p.pulsePhase = (float)(rand() % 628) / 100.f; // 0..2PI

    particles.push_back(p);
}

void AmbientCubes::updateParticle(AmbientParticle& p, float dt, float screenW, float screenH) {
    p.position += p.velocity * dt;

    // Soft boundary bounce — keep particles on screen
    float margin = p.size * 2;
    if (p.position.x < margin)      { p.position.x = margin; p.velocity.x = abs(p.velocity.x); }
    if (p.position.x > screenW - margin) { p.position.x = screenW - margin; p.velocity.x = -abs(p.velocity.x); }
    if (p.position.y < margin)      { p.position.y = margin; p.velocity.y = abs(p.velocity.y); }
    if (p.position.y > screenH - margin) { p.position.y = screenH - margin; p.velocity.y = -abs(p.velocity.y); }

    p.lifetime -= dt;

    // Fade in/out
    if (mFadeInOut.mValue) {
        float fadeTime = 1.5f;
        if (p.lifetime > p.maxLifetime - fadeTime)
            p.alpha = mAlpha.mValue * (1.f - (p.lifetime - (p.maxLifetime - fadeTime)) / fadeTime);
        else if (p.lifetime < fadeTime)
            p.alpha = mAlpha.mValue * (p.lifetime / fadeTime);
        else
            p.alpha = mAlpha.mValue;
    }

    // Pulse phase
    if (mPulse.mValue)
        p.pulsePhase += dt * mPulseSpeed.mValue;
}

void AmbientCubes::renderParticle(const AmbientParticle& p, ImColor color) {
    auto drawList = ImGui::GetBackgroundDrawList();

    float currentSize = p.size;
    if (mPulse.mValue) {
        float pulse = 1.f + 0.15f * sinf(p.pulsePhase);
        currentSize *= pulse;
    }
    currentSize = std::max(currentSize, 1.f);

    float a = std::clamp(p.alpha, 0.f, 1.f);

    // Soft glow (larger, very transparent)
    if (mGlow.mValue) {
        ImColor glowColor = color;
        glowColor.Value.w = a * 0.08f;
        drawList->AddCircleFilled(
            ImVec2(p.position.x, p.position.y),
            currentSize * 2.5f,
            glowColor, 24);
    }

    // Main circle — soft, filled
    ImColor fillColor = color;
    fillColor.Value.w = a * 0.25f;
    drawList->AddCircleFilled(
        ImVec2(p.position.x, p.position.y),
        currentSize,
        fillColor, 20);

    // Core — brighter center
    ImColor coreColor = color;
    coreColor.Value.w = a * 0.5f;
    drawList->AddCircleFilled(
        ImVec2(p.position.x, p.position.y),
        currentSize * 0.4f,
        coreColor, 12);
}

void AmbientCubes::onRenderEvent(RenderEvent& event) {
    if (!mEnabled) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto guiData = ClientInstance::get()->getGuiData();
    if (!guiData) return;

    float screenW = guiData->mResolution.x;
    float screenH = guiData->mResolution.y;

    if (screenW < 1.f || screenH < 1.f) return;

    float deltaTime = ImGui::GetIO().DeltaTime;

    float spawnRate = (float)mCount.mValue / mLifetime.mValue;
    spawnTimer += deltaTime;

    while (spawnTimer > 1.f / spawnRate && particles.size() < (size_t)mCount.mValue) {
        spawnParticle(screenW, screenH);
        spawnTimer -= 1.f / spawnRate;
    }

    ImColor color = getColor();

    for (auto it = particles.begin(); it != particles.end();) {
        updateParticle(*it, deltaTime, screenW, screenH);

        if (it->lifetime <= 0) {
            it = particles.erase(it);
        } else {
            renderParticle(*it, color);
            ++it;
        }
    }
}
