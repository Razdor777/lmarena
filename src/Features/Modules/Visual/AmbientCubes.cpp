//
// AmbientCubes.cpp — Soft floating ambient glow particles
//
// Behaviour:
//  - The swarm is anchored to what you actually see (camera origin).
//  - Teleports (pearls, lagbacks, dimension hops) refill the whole swarm
//    INSTANTLY around the new spot — no more waiting for particles.
//  - At any speed (pearls + fireworks), particles that fall outside the
//    radius are recycled back to you immediately, so the density around
//    you stays constant.
//
// Visuals:
//  - Soft bokeh-style glow shapes (layered transparency, no harsh outlines)
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

static inline float rand01() { return (float)rand() / (float)RAND_MAX; }

void AmbientCubes::onEnable() {
    cubes.clear();
    spawnTimer = 0;
    mLastAnchor = { FLT_MAX, FLT_MAX, FLT_MAX };
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

// Anchor = the point you are actually looking from.
// Camera/render origin first, player eye-pos as fallback.
glm::vec3 AmbientCubes::getAnchorPos() {
    auto player = ClientInstance::get()->getLocalPlayer();
    glm::vec3 fallback = player
        ? *player->getPos() + glm::vec3(0.f, PLAYER_HEIGHT, 0.f)
        : glm::vec3(0.f);

    glm::vec3 o = RenderUtils::transform.mOrigin;
    if (!std::isfinite(o.x) || !std::isfinite(o.y) || !std::isfinite(o.z)) return fallback;
    if (glm::length(o) < 0.001f) return fallback;
    return o;
}

// Build one particle around the anchor.
// popIn = appear at full alpha right away (no fade-in)
AmbientCube AmbientCubes::makeCube(const glm::vec3& anchor, bool popIn) {
    AmbientCube cube;
    float radius = mSpawnRadius.mValue;

    switch (mSpawnArea.mValue) {
    case SpawnArea::Everywhere:
        cube.position = anchor + glm::vec3(
            (rand01() * 2.f - 1.f) * radius,
            (rand01() * 1.6f - 0.8f) * radius,
            (rand01() * 2.f - 1.f) * radius);
        break;
    case SpawnArea::Above:
        cube.position = anchor + glm::vec3(
            (rand01() * 2.f - 1.f) * radius,
            (0.3f + rand01()) * radius * 0.7f,
            (rand01() * 2.f - 1.f) * radius);
        break;
    case SpawnArea::Front: {
        float angle = rand01() * 2.f * (float)M_PI;
        float d = 3.f + rand01() * radius;
        cube.position = anchor + glm::vec3(
            cosf(angle) * d,
            (rand01() * 2.f - 1.f) * radius * 0.5f,
            sinf(angle) * d);
        break;
    }
    case SpawnArea::Surround: {
        float angle = rand01() * 2.f * (float)M_PI;
        cube.position = anchor + glm::vec3(
            cosf(angle) * radius,
            (rand01() * 2.f - 1.f) * radius * 0.5f,
            sinf(angle) * radius);
        break;
    }
    }

    // Never spawn right on the camera — particles popping onto your
    // face are exactly what made this module annoying
    glm::vec3 fromAnchor = cube.position - anchor;
    float d = glm::length(fromAnchor);
    const float minDist = 2.0f;
    if (d < minDist) {
        if (d < 0.001f) fromAnchor = { 1.f, 0.f, 0.f };
        cube.position = anchor + glm::normalize(fromAnchor) * minDist;
    }

    // Gentle drift with a slight upward float
    cube.velocity = glm::vec3(
        (rand01() - 0.5f) * 0.6f,
        (rand01() - 0.35f) * 0.25f,
        (rand01() - 0.5f) * 0.6f);

    cube.rotation = glm::vec3(0.f);
    cube.rotation.z = rand01() * 2.f * (float)M_PI;
    cube.rotationSpeed = glm::vec3(
        0.f, 0.f,
        (rand01() - 0.5f) * 2.f * mRotationSpeed.mValue);

    float baseSize = mCubeSize.mValue;
    float variation = mSizeVariation.mValue;
    cube.size = baseSize + (rand01() - 0.5f) * variation * baseSize;
    cube.size = std::max(cube.size, 0.05f);

    cube.alpha = mAlpha.mValue;
    cube.maxLifetime = mLifetime.mValue + (rand01() * 4.f - 2.f);
    cube.maxLifetime = std::max(cube.maxLifetime, 1.f);
    cube.lifetime = cube.maxLifetime;

    switch (mStyle.mValue) {
    case ParticleStyle::Mixed:  cube.style = rand() % 4; break;
    case ParticleStyle::Orbs:   cube.style = 0; break;
    case ParticleStyle::Bokeh:  cube.style = 1; break;
    case ParticleStyle::Dust:   cube.style = 2; break;
    case ParticleStyle::Petals: cube.style = 3; break;
    }

    cube.popIn = popIn;
    cube.swayPhase = rand01() * 2.f * (float)M_PI;
    return cube;
}

void AmbientCubes::spawnCube(const glm::vec3& anchorPos, bool popIn) {
    cubes.push_back(makeCube(anchorPos, popIn));
}

// Refill the whole swarm around a new spot at once (enable / teleports).
// Lifetimes are staggered so the swarm doesn't die all at the same moment.
void AmbientCubes::reseedAll(const glm::vec3& anchor) {
    cubes.clear();
    int count = (int)mCubeCount.mValue;
    for (int i = 0; i < count; i++) {
        AmbientCube cube = makeCube(anchor, true);
        cube.lifetime = cube.maxLifetime * (0.35f + 0.65f * rand01());
        cubes.push_back(cube);
    }
    spawnTimer = 0.f;
}

void AmbientCubes::updateCube(AmbientCube& cube, float deltaTime, float timeSec) {
    // Gentle sinusoidal sway on top of the drift
    glm::vec3 sway = glm::vec3(
        sinf(timeSec * 0.9f + cube.swayPhase),
        sinf(timeSec * 0.6f + cube.swayPhase * 1.7f) * 0.4f,
        cosf(timeSec * 0.8f + cube.swayPhase)) * 0.15f;

    cube.position += (cube.velocity * mSpeed.mValue + sway) * deltaTime;

    if (mRotate.mValue)
        cube.rotation += cube.rotationSpeed * deltaTime;

    cube.lifetime -= deltaTime;

    float base = mAlpha.mValue;
    float a = base;
    if (mFadeInOut.mValue) {
        float fadeTime = 1.5f;
        float age = cube.maxLifetime - cube.lifetime;
        // popIn particles (teleport refill / recycled) skip the fade-in:
        // the whole point is that they are already there
        if (!cube.popIn && age < fadeTime)
            a = base * (age / fadeTime);
        if (cube.lifetime < fadeTime)
            a = std::min(a, base * (cube.lifetime / fadeTime));
    }
    cube.alpha = a;
}

// ============================================================
// SHAPE RENDERING — soft bokeh glow, layered transparency
// ============================================================
static inline ImColor withAlpha(ImColor c, float a) {
    a = MathUtils::clamp(a, 0.f, 1.f);
    c.Value.w = a;
    return c;
}

static inline ImColor mixColors(ImColor a, ImColor b, float t) {
    return ImColor(
        a.Value.x + (b.Value.x - a.Value.x) * t,
        a.Value.y + (b.Value.y - a.Value.y) * t,
        a.Value.z + (b.Value.z - a.Value.z) * t,
        a.Value.w + (b.Value.w - a.Value.w) * t);
}

static void glowLayer(ImDrawList* dl, ImVec2 c, float r, ImColor col, float a, int segments = 14) {
    if (r <= 0.05f || a <= 0.003f) return;
    dl->AddCircleFilled(c, r, withAlpha(col, a), segments);
}

void AmbientCubes::renderShape(const glm::vec2& screenPos, float pixelSize,
                                const glm::vec3& rotation, float alpha,
                                ImColor color, int style) {
    auto dl = ImGui::GetBackgroundDrawList();
    ImVec2 center(screenPos.x, screenPos.y);
    ImColor white(255, 255, 255);

    switch (style) {
    case 0: { // Orb — soft glowing sphere with a bright core
        float r = pixelSize;
        if (mGlow.mValue) {
            glowLayer(dl, center, r * 2.0f, color, alpha * 0.09f, 16);
            glowLayer(dl, center, r * 1.3f, color, alpha * 0.16f, 16);
        }
        glowLayer(dl, center, r * 0.72f, color, alpha * 0.55f, 14);
        ImColor core = mixColors(color, white, 0.65f);
        glowLayer(dl, center, r * 0.28f, core, alpha * 0.9f, 10);
        break;
    }
    case 1: { // Bokeh — big dreamy translucent circle, no core
        float r = pixelSize * 1.7f;
        glowLayer(dl, center, r * 1.9f, color, alpha * 0.045f, 18);
        glowLayer(dl, center, r * 1.35f, color, alpha * 0.07f, 18);
        glowLayer(dl, center, r * 1.0f, color, alpha * 0.10f, 16);
        // subtle edge sheen so the bokeh reads as a lens disc
        ImColor sheen = mixColors(color, white, 0.25f);
        glowLayer(dl, center, r * 0.62f, sheen, alpha * 0.10f, 16);
        break;
    }
    case 2: { // Dust — tiny bright speck with a faint glint cross
        float r = pixelSize * 0.45f;
        if (mGlow.mValue)
            glowLayer(dl, center, r * 2.2f, color, alpha * 0.16f, 10);
        ImColor core = mixColors(color, white, 0.75f);
        glowLayer(dl, center, r, core, alpha * 0.95f, 8);
        // tiny sparkle cross
        ImColor glint = withAlpha(core, alpha * 0.4f);
        float g = r * 2.6f;
        dl->AddLine({center.x - g, center.y}, {center.x + g, center.y}, glint, 1.0f);
        dl->AddLine({center.x, center.y - g}, {center.x, center.y + g}, glint, 1.0f);
        break;
    }
    case 3: { // Petal — soft rotated ellipse that slowly tumbles
        float r = pixelSize * 1.1f;
        float rx = r, ry = r * 0.55f;
        float ang = rotation.z;
        float ca = cosf(ang), sa = sinf(ang);

        auto ellipsePoints = [&](float scale, ImVec2 off, ImVec2* out, int n) {
            for (int i = 0; i < n; i++) {
                float t = (float)i / (float)n * 2.f * (float)M_PI;
                float ex = cosf(t) * rx * scale;
                float ey = sinf(t) * ry * scale;
                out[i] = ImVec2(center.x + ex * ca - ey * sa + off.x,
                                center.y + ex * sa + ey * ca + off.y);
            }
        };

        constexpr int n = 10;
        ImVec2 pts[n];
        ellipsePoints(1.0f, {0.f, 0.f}, pts, n);
        dl->AddConvexPolyFilled(pts, n, withAlpha(color, alpha * 0.30f));

        ImVec2 inner[n];
        ImVec2 lightOff(-r * 0.12f * ca, -r * 0.12f * sa);
        ImColor innerCol = mixColors(color, white, 0.35f);
        ellipsePoints(0.55f, lightOff, inner, n);
        dl->AddConvexPolyFilled(inner, n, withAlpha(innerCol, alpha * 0.45f));
        break;
    }
    }
}

void AmbientCubes::renderCube(const AmbientCube& cube) {
    auto ci = ClientInstance::get();
    if (!ci) return;
    if (!ci->getGuiData()) return;

    glm::vec2 screenPos;
    if (!RenderUtils::transform.mMatrix.OWorldToScreen(RenderUtils::transform.mOrigin, cube.position,
                                  screenPos, MathUtils::fov,
                                  ci->getGuiData()->mResolution)) {
        return;
    }

    // Perspective sizing: near = bigger, far = smaller (clamped)
    float dist = glm::distance(RenderUtils::transform.mOrigin, cube.position);
    float persp = MathUtils::clamp(3.2f / std::max(dist, 0.1f), 0.30f, 2.5f);
    float pixelSize = cube.size * 26.f * persp;
    if (pixelSize < 0.6f) return; // too small to matter

    // Gentle distance fade so the swarm dissolves instead of popping
    float maxDist = mSpawnRadius.mValue * 1.75f;
    float distFade = MathUtils::clamp(1.3f - dist / maxDist, 0.25f, 1.f);

    ImColor color = getColor();
    renderShape(screenPos, pixelSize, cube.rotation, cube.alpha * distFade, color, cube.style);
}

void AmbientCubes::onRenderEvent(RenderEvent& event) {
    if (!mEnabled) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    float deltaTime = ImGui::GetIO().DeltaTime;
    float timeSec = (float)ImGui::GetTime();

    glm::vec3 anchor = getAnchorPos();

    // Teleport / big jump (pearl, lagback, freecam drag): refill the whole
    // swarm around the new spot INSTANTLY instead of waiting for spawns
    if (mLastAnchor.x == FLT_MAX ||
        glm::distance(anchor, mLastAnchor) > mSpawnRadius.mValue * 0.75f) {
        reseedAll(anchor);
    }
    mLastAnchor = anchor;

    // Gentle trickle spawn for natural replenishment
    float spawnRate = (float)mCubeCount.mValue / mLifetime.mValue;
    spawnTimer += deltaTime;
    while (spawnTimer > 1.f / spawnRate && cubes.size() < (size_t)mCubeCount.mValue) {
        spawnCube(anchor);
        spawnTimer -= 1.f / spawnRate;
    }

    float recycleDist = mSpawnRadius.mValue * 1.75f;

    for (auto it = cubes.begin(); it != cubes.end();) {
        updateCube(*it, deltaTime, timeSec);

        if (it->lifetime <= 0) {
            it = cubes.erase(it);
            continue;
        }

        // Particle got left behind (fast flight, pearls): recycle it back
        // around the anchor right now at full alpha — the swarm keeps up
        // with you no matter the speed
        if (glm::distance(it->position, anchor) > recycleDist) {
            *it = makeCube(anchor, true);
        }

        renderCube(*it);
        ++it;
    }
}
