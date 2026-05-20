//
// AmbientCubes.cpp — Soft floating ambient particles
//
// Small, subtle shapes that float around the player.
// Designed to be easy on the eyes: thin lines, soft glow, gentle movement.
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

    float radius = mSpawnRadius.mValue;
    float angle = (rand() % 360) * (float)M_PI / 180.f;

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
        cube.position.x = playerPos.x + cosf(angle) * (rand() % (int)radius);
        cube.position.y = playerPos.y + ((rand() % (int)(radius)) - radius * 0.3f);
        cube.position.z = playerPos.z + (rand() % (int)radius);
        break;
    case SpawnArea::Surround:
        cube.position.x = playerPos.x + cosf(angle) * radius;
        cube.position.y = playerPos.y + ((rand() % (int)(radius * 0.8f)) - radius * 0.2f);
        cube.position.z = playerPos.z + sinf(angle) * radius;
        break;
    }

    // Gentle drift velocity
    cube.velocity.x = ((rand() % 100) - 50) / 100.f * 0.3f;
    cube.velocity.y = ((rand() % 100) - 20) / 100.f * 0.2f;
    cube.velocity.z = ((rand() % 100) - 50) / 100.f * 0.3f;

    cube.rotation = glm::vec3(
        (rand() % 360) * (float)M_PI / 180.f,
        (rand() % 360) * (float)M_PI / 180.f,
        (rand() % 360) * (float)M_PI / 180.f
    );

    cube.rotationSpeed = glm::vec3(
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue,
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue,
        ((rand() % 100) - 50) / 50.f * mRotationSpeed.mValue
    );

    float baseSize = mCubeSize.mValue;
    float variation = mSizeVariation.mValue;
    cube.size = baseSize + ((rand() % 100) / 100.f - 0.5f) * variation * baseSize;
    cube.size = std::max(cube.size, 0.05f);

    cube.alpha = mAlpha.mValue;
    cube.maxLifetime = mLifetime.mValue + ((rand() % 40) / 10.f - 2.f);
    cube.lifetime = cube.maxLifetime;

    switch (mStyle.mValue) {
    case ParticleStyle::Mixed:    cube.style = rand() % 5; break;
    case ParticleStyle::Diamonds: cube.style = 0; break;
    case ParticleStyle::Rings:    cube.style = 1; break;
    case ParticleStyle::Dots:     cube.style = 2; break;
    case ParticleStyle::Stars:    cube.style = 3; break;
    case ParticleStyle::Triangles:cube.style = 4; break;
    }

    cubes.push_back(cube);
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
// SHAPE RENDERING — soft, thin, gentle shapes
// ============================================================
void AmbientCubes::renderShape(const glm::vec2& screenPos, float size,
                                const glm::vec3& rotation, float alpha,
                                ImColor color, int style) {
    auto drawList = ImGui::GetBackgroundDrawList();
    color.Value.w = alpha;

    // Small pixel size (12px per unit instead of 20)
    float pixelSize = size * 12.f;
    float rx = rotation.x;
    float ry = rotation.y;

    switch (style) {
    case 0: { // Diamond — small rotating rhombus
        float s = pixelSize;
        float stretch = 1.0f + 0.2f * sinf(rx);

        ImVec2 top(screenPos.x, screenPos.y - s * stretch);
        ImVec2 right(screenPos.x + s * 0.5f, screenPos.y);
        ImVec2 bottom(screenPos.x, screenPos.y + s * stretch * 0.5f);
        ImVec2 left(screenPos.x - s * 0.5f, screenPos.y);

        ImColor fillColor = color;
        fillColor.Value.w = alpha * 0.25f;
        drawList->AddQuadFilled(top, right, bottom, left, fillColor);

        ImColor edgeColor = color;
        edgeColor.Value.w = alpha * 0.65f;
        drawList->AddQuad(top, right, bottom, left, edgeColor, 1.0f);
        break;
    }
    case 1: { // Ring — thin circle outline
        float s = pixelSize * 0.8f;
        ImColor ringColor = color;
        ringColor.Value.w = alpha * 0.55f;
        drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), s, ringColor, 16, 1.0f);

        // Inner dot
        ImColor dotColor = color;
        dotColor.Value.w = alpha * 0.3f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), s * 0.2f, dotColor, 8);
        break;
    }
    case 2: { // Dot — soft filled circle
        float s = pixelSize * 0.5f;
        ImColor dotColor = color;
        dotColor.Value.w = alpha * 0.5f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), s, dotColor, 12);
        break;
    }
    case 3: { // Star — 4-pointed star
        float s = pixelSize;
        float inner = s * 0.3f;

        ImColor starColor = color;
        starColor.Value.w = alpha * 0.6f;

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
    case 4: { // Triangle — small rotating triangle
        float s = pixelSize * 0.7f;
        ImVec2 points[3];
        for (int i = 0; i < 3; i++) {
            float a = rotation.z + i * (float)M_PI * 2.f / 3.f;
            points[i] = ImVec2(screenPos.x + cosf(a) * s, screenPos.y + sinf(a) * s);
        }

        ImColor fillColor = color;
        fillColor.Value.w = alpha * 0.2f;
        drawList->AddTriangleFilled(points[0], points[1], points[2], fillColor);

        ImColor edgeColor = color;
        edgeColor.Value.w = alpha * 0.55f;
        drawList->AddTriangle(points[0], points[1], points[2], edgeColor, 1.0f);
        break;
    }
    }

    // Soft glow
    if (mGlow.mValue) {
        ImColor glowColor = color;
        glowColor.Value.w = alpha * 0.12f;
        drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), pixelSize * 1.2f, glowColor, 12);
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

    float spawnRate = (float)mCubeCount.mValue / mLifetime.mValue;
    spawnTimer += deltaTime;

    while (spawnTimer > 1.f / spawnRate && cubes.size() < (size_t)mCubeCount.mValue) {
        spawnCube(playerPos);
        spawnTimer -= 1.f / spawnRate;
    }

    for (auto it = cubes.begin(); it != cubes.end();) {
        updateCube(*it, deltaTime);

        if (it->lifetime <= 0) {
            it = cubes.erase(it);
        } else {
            renderCube(*it);
            ++it;
        }
    }
}
