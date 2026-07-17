//
// AmbientCubes.cpp — Soft floating ambient particles
//
// Small, subtle aesthetic shapes that float around the player.
// Designed to be easy on the eyes: thin lines, soft glow, gentle movement.
// Instantly adapts to high-speed movement and pearl teleports.
//

#include "AmbientCubes.hpp"

#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void AmbientCubes::onEnable() {
    cubes.clear();
    spawnTimer = 0;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (player) {
        mLastPlayerPos = *player->getPos();
    } else {
        mLastPlayerPos = glm::vec3(0, 0, 0);
    }
}

void AmbientCubes::onDisable() {
    cubes.clear();
}

ImColor AmbientCubes::getColor() {
    if (mUseThemeColor.mValue) {
        return ColorUtils::getThemedColor(0.f);
    }
    return mCustomColor.getAsImColor();
}

void AmbientCubes::spawnCube(const glm::vec3& playerPos) {
    AmbientCube cube;
    spawnCubeNear(cube, playerPos);

    float baseSize = mCubeSize.mValue;
    float variation = mSizeVariation.mValue;
    cube.size = baseSize + ((rand() % 100) / 100.f - 0.5f) * variation * baseSize;
    cube.size = std::max(cube.size, 0.05f);

    switch (mStyle.mValue) {
    case ParticleStyle::Mixed:    cube.style = rand() % 4; break;
    case ParticleStyle::Diamonds: cube.style = 0; break;
    case ParticleStyle::Rings:    cube.style = 1; break;
    case ParticleStyle::Dots:     cube.style = 2; break;
    case ParticleStyle::Stars:    cube.style = 3; break;
    case ParticleStyle::Triangles:cube.style = 3; break;
    }

    cubes.push_back(cube);
}

void AmbientCubes::spawnCubeNear(AmbientCube& cube, const glm::vec3& playerPos) {
    float radius = mSpawnRadius.mValue;
    float angle = (rand() % 360) * (float)M_PI / 180.f;
    float dist = (rand() % (int)(radius * 100)) / 100.f;

    switch (mSpawnArea.mValue) {
    case SpawnArea::Everywhere:
        cube.position.x = playerPos.x + ((rand() % (int)(radius * 2)) - radius);
        cube.position.y = playerPos.y + ((rand() % (int)(radius * 1.5f)) - radius * 0.5f);
        cube.position.z = playerPos.z + ((rand() % (int)(radius * 2)) - radius);
        break;
    case SpawnArea::Above:
        cube.position.x = playerPos.x + ((rand() % (int)(radius * 2)) - radius);
        cube.position.y = playerPos.y + radius * 0.5f + (rand() % (int)(radius));
        cube.position.z = playerPos.z + ((rand() % (int)(radius * 2)) - radius);
        break;
    case SpawnArea::Front:
        cube.position.x = playerPos.x + cosf(angle) * dist;
        cube.position.y = playerPos.y + ((rand() % (int)(radius)) - radius * 0.3f);
        cube.position.z = playerPos.z + sinf(angle) * dist + radius * 0.3f;
        break;
    case SpawnArea::Surround:
        cube.position.x = playerPos.x + cosf(angle) * radius;
        cube.position.y = playerPos.y + ((rand() % (int)(radius * 0.8f)) - radius * 0.2f);
        cube.position.z = playerPos.z + sinf(angle) * radius;
        break;
    }

    // Gentle drift velocity
    cube.velocity.x = ((rand() % 100) - 50) / 100.f * 0.25f;
    cube.velocity.y = ((rand() % 100) - 20) / 100.f * 0.15f;
    cube.velocity.z = ((rand() % 100) - 50) / 100.f * 0.25f;

    cube.rotation = glm::vec3(
        (rand() % 360) * (float)M_PI / 180.f,
        (rand() % 360) * (float)M_PI / 180.f,
        (rand() % 360) * (float)M_PI / 180.f
    );

    cube.rotationSpeed = glm::vec3(
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue * 0.6f,
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue * 0.6f,
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue * 0.6f
    );

    cube.alpha = mAlpha.mValue;
    cube.maxLifetime = mLifetime.mValue + ((rand() % 40) / 10.f - 2.f);
    cube.lifetime = cube.maxLifetime;

    float baseSize = mCubeSize.mValue;
    float variation = mSizeVariation.mValue;
    cube.size = baseSize + ((rand() % 100) / 100.f - 0.5f) * variation * baseSize;
    cube.size = std::max(cube.size, 0.05f);

    switch (mStyle.mValue) {
    case ParticleStyle::Mixed:    cube.style = rand() % 4; break;
    case ParticleStyle::Diamonds: cube.style = 0; break;
    case ParticleStyle::Rings:    cube.style = 1; break;
    case ParticleStyle::Dots:     cube.style = 2; break;
    case ParticleStyle::Stars:    cube.style = 3; break;
    case ParticleStyle::Triangles:cube.style = 3; break;
    }
}

void AmbientCubes::updateCube(AmbientCube& cube, float deltaTime) {
    cube.position += cube.velocity * deltaTime * mSpeed.mValue;

    if (mRotate.mValue)
        cube.rotation += cube.rotationSpeed * deltaTime;

    cube.lifetime -= deltaTime;

    if (mFadeInOut.mValue) {
        float fadeTime = 1.5f;
        if (cube.lifetime > cube.maxLifetime - fadeTime)
            cube.alpha = mAlpha.mValue * (1.f - (cube.lifetime - (cube.maxLifetime - fadeTime)) / fadeTime);
        else if (cube.lifetime < fadeTime)
            cube.alpha = mAlpha.mValue * (cube.lifetime / fadeTime);
        else
            cube.alpha = mAlpha.mValue;
    }
}

// ============================================================
// SHAPE RENDERING — clean, minimal, aesthetic ambient particles
// ============================================================
void AmbientCubes::renderShape(const glm::vec2& screenPos, float size,
                                const glm::vec3& rotation, float alpha,
                                ImColor color, int style) {
    auto drawList = ImGui::GetBackgroundDrawList();
    color.Value.w = alpha;

    float pixelSize = size * 14.f;

    switch (style) {
    case 0: { // Soft Glowing Orb / Sparkle
        ImColor glowColor = color;
        glowColor.Value.w = alpha * 0.35f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), pixelSize * 0.9f, glowColor, 12);

        ImColor coreColor = ImColor(255, 255, 255, (int)(255 * alpha * 0.8f));
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), pixelSize * 0.35f, coreColor, 8);
        break;
    }
    case 1: { // Thin Minimal Ring
        float s = pixelSize * 0.85f;
        ImColor ringColor = color;
        ringColor.Value.w = alpha * 0.6f;
        drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), s, ringColor, 16, 1.0f);

        ImColor dotColor = color;
        dotColor.Value.w = alpha * 0.25f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), s * 0.25f, dotColor, 8);
        break;
    }
    case 2: { // Soft Micro Dot
        float s = pixelSize * 0.45f;
        ImColor dotColor = color;
        dotColor.Value.w = alpha * 0.7f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), s, dotColor, 12);
        break;
    }
    case 3: { // Delicate 4-Pointed Sparkle Star
        float s = pixelSize * 1.1f;
        float inner = s * 0.25f;

        ImColor starColor = color;
        starColor.Value.w = alpha * 0.65f;

        float baseAngle = rotation.z;
        for (int i = 0; i < 4; i++) {
            float a = baseAngle + i * (float)M_PI * 0.5f;
            ImVec2 tip(screenPos.x + cosf(a) * s, screenPos.y + sinf(a) * s);
            float a1 = a + (float)M_PI * 0.25f;
            float a2 = a - (float)M_PI * 0.25f;
            ImVec2 side1(screenPos.x + cosf(a1) * inner, screenPos.y + sinf(a1) * inner);
            ImVec2 side2(screenPos.x + cosf(a2) * inner, screenPos.y + sinf(a2) * inner);

            drawList->AddTriangleFilled(tip, side1, side2, starColor);
        }
        break;
    }
    }

    // Soft background aura glow
    if (mGlow.mValue) {
        ImColor auraColor = color;
        auraColor.Value.w = alpha * 0.1f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), pixelSize * 1.6f, auraColor, 12);
    }
}

void AmbientCubes::renderCube(const AmbientCube& cube) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto ci = ClientInstance::get();
    if (!ci->getGuiData()) return;

    auto corrected = RenderUtils::transform.mMatrix;
    glm::vec2 screenPos;
    if (!corrected.OWorldToScreen(RenderUtils::transform.mOrigin, cube.position,
                                  screenPos, MathUtils::fov,
                                  ci->getGuiData()->mResolution)) {
        return;
    }

    ImColor color = getColor();
    renderShape(screenPos, cube.size, cube.rotation, cube.alpha, color, cube.style);
}

void AmbientCubes::onRenderEvent(RenderEvent& event) {
    if (!mEnabled) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    float deltaTime = ImGui::GetIO().DeltaTime;
    glm::vec3 playerPos = *player->getPos();

    // Instantly redistribute/recycle particles if player moved very fast (pearl throw / elytra flight)
    float moveDist = glm::distance(playerPos, mLastPlayerPos);
    if (moveDist > mSpawnRadius.mValue || cubes.empty()) {
        for (auto& cube : cubes) {
            spawnCubeNear(cube, playerPos);
        }
    }
    mLastPlayerPos = playerPos;

    float spawnRate = (float)mCubeCount.mValue / mLifetime.mValue;
    spawnTimer += deltaTime;

    while (spawnTimer > 1.f / spawnRate && cubes.size() < (size_t)mCubeCount.mValue) {
        AmbientCube cube;
        spawnCubeNear(cube, playerPos);
        cubes.push_back(cube);
        spawnTimer -= 1.f / spawnRate;
    }

    for (auto it = cubes.begin(); it != cubes.end();) {
        updateCube(*it, deltaTime);

        // If particle drifted too far from player, recycle near player immediately
        if (glm::distance(it->position, playerPos) > mSpawnRadius.mValue * 2.2f || it->lifetime <= 0) {
            spawnCubeNear(*it, playerPos);
        }

        renderCube(*it);
        ++it;
    }
}
