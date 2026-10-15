//
// Ported from Combatant WorldParticles. Drawn in 2D on the ImGui background draw list.
//
// Dropped / simplified from Combatant:
//  - 3D depth test and textured sprites (glow is procedural)
//  - Air bubbles: 3D sphere shell becomes an outlined disc with a highlight
//  - Funnel: 3D distortion mesh and leak/ghost geometry become rings and spokes
//  - Funnel Tint mode and Light Rainbow / Sky / Fade / Analogous color modes
//    (same color set as JumpCircles)
//  - Partial-tick interpolation (Juzdex renders without tickDelta)
//

#include "WorldParticles.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace WorldParticlesImpl
{
    constexpr float SPRITE_SIZE_MULTIPLIER = 4.0f;
    constexpr float SPRITE_ALPHA_MULTIPLIER = 0.40f;
    constexpr float DIAGONAL_ALPHA_MULTIPLIER = 0.40f;
    constexpr float OUTLINE_ALPHA_MULTIPLIER = 205.0f / 255.0f;
    constexpr float BUBBLE_FILL_SIZE_MULTIPLIER = 3.15f;
    constexpr float BUBBLE_GLOW_SIZE_MULTIPLIER = 5.10f;
    constexpr float BUBBLE_HIGHLIGHT_SIZE_MULTIPLIER = 0.34f;
    constexpr float BUBBLE_FILL_ALPHA_MULTIPLIER = 0.34f;
    constexpr float BUBBLE_GLOW_ALPHA_MULTIPLIER = 0.13f;
    constexpr float BUBBLE_HIGHLIGHT_ALPHA_MULTIPLIER = 0.74f;
    constexpr float FUNNEL_EYE_SIZE_MULTIPLIER = 4.35f;
    constexpr float FUNNEL_DISTORTION_SIZE_MULTIPLIER = 5.25f;
    constexpr float FUNNEL_DISTORTION_OUTER_SIZE_MULTIPLIER = 6.85f;
    constexpr float FUNNEL_DISTORTION_INNER_SIZE_MULTIPLIER = 3.95f;
    constexpr float FUNNEL_EYE_ALPHA_MULTIPLIER = 0.72f;
    constexpr float FUNNEL_DISTORTION_OUTER_ALPHA_MULTIPLIER = 0.085f;
    constexpr float FUNNEL_DISTORTION_INNER_ALPHA_MULTIPLIER = 0.115f;
    constexpr float FUNNEL_RIM_ALPHA_MULTIPLIER = 0.22f;
    constexpr float FUNNEL_SPOKE_ALPHA_MULTIPLIER = 0.16f;
    constexpr float MOTION_SCALE = 0.04f;
    constexpr float DRAG = 0.98f;
    constexpr uint64_t ALPHA_ANIM_MS = 1000;

    // Cube corners: 0-3 bottom face, 4-7 top face
    inline const glm::vec3 CUBE_CORNERS[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1},
        {-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}
    };
    constexpr int CUBE_EDGES[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    constexpr int BODY_DIAGONALS[4][2] = {
        {0, 6}, {1, 7}, {2, 4}, {3, 5}
    };

    inline float clamp01(float v) {
        return std::clamp(v, 0.f, 1.f);
    }

    inline float smoothstep(float t) {
        t = clamp01(t);
        return t * t * (3.f - 2.f * t);
    }

    inline float randomRange(float min, float max) {
        return min + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (max - min);
    }

    // Color mode: 0 static, 1 rainbow, 2 double color, 3 theme
    inline ImColor colorFor(int mode, float speed, int index, ImColor c1, ImColor c2) {
        const float seconds = static_cast<float>(NOW) * speed / 18000.f;
        const int spatial = ((index % 360) + 360) % 360;
        switch (mode) {
            case 1: return ColorUtils::Rainbow(seconds, 1.f, 1.f, spatial);
            case 2: return ColorUtils::LerpColors(seconds, static_cast<float>(spatial), { c1, c2 });
            case 3: return ColorUtils::getThemedColor(static_cast<float>(spatial));
            default: return c1;
        }
    }

    inline ImU32 withAlpha(ImU32 color, float alphaMul) {
        const uint32_t a = std::clamp(static_cast<uint32_t>(((color >> 24) & 0xFF) * clamp01(alphaMul)), 0u, 255u);
        return (color & 0x00FFFFFFu) | (a << 24);
    }

    inline float screenRadius(float worldSize, float distance, float displayHeight) {
        const float d = std::max(0.5f, distance);
        return std::clamp(worldSize * displayHeight / (2.8f * d), 1.f, 120.f);
    }

    inline void drawGlow(ImDrawList* drawList, ImVec2 center, float radius, ImU32 color) {
        drawList->AddCircleFilled(center, radius * 1.8f, withAlpha(color, 0.25f), 12);
        drawList->AddCircleFilled(center, radius, color, 12);
    }

    // Rotation: X, then Y, then Z (radians)
    inline glm::vec3 rotateEuler(glm::vec3 p, glm::vec3 r) {
        const float cx = std::cos(r.x), sx = std::sin(r.x);
        p = glm::vec3(p.x, p.y * cx - p.z * sx, p.y * sx + p.z * cx);
        const float cy = std::cos(r.y), sy = std::sin(r.y);
        p = glm::vec3(p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy);
        const float cz = std::cos(r.z), sz = std::sin(r.z);
        p = glm::vec3(p.x * cz - p.y * sz, p.x * sz + p.y * cz, p.z);
        return p;
    }
}

using namespace WorldParticlesImpl;

// =========================================================
// Particle state
// =========================================================

namespace
{
    uint64_t particleAgeMs(uint64_t spawnMs) {
        return NOW - spawnMs;
    }

    float particleAlpha(const WorldParticles::Particle& p) {
        const uint64_t age = particleAgeMs(p.spawnMs);
        if (age == 0) return 0.f;
        if (age < ALPHA_ANIM_MS) return smoothstep(static_cast<float>(age) / ALPHA_ANIM_MS);
        if (age < p.lifeMs) return 1.f;
        const float fade = 1.f - static_cast<float>(age - p.lifeMs) / ALPHA_ANIM_MS;
        return smoothstep(clamp01(fade));
    }

    // Funnel pulse: opens in, closes out, breathes
    float funnelAlpha(const WorldParticles::Particle& p) {
        const uint64_t age = particleAgeMs(p.spawnMs);
        if (age == 0) return 0.f;
        const float t = std::min(1.f, static_cast<float>(age) / 720.f);
        const float appear = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
        float disappear = 1.f;
        if (age > p.lifeMs) {
            const float u = std::min(1.f, static_cast<float>(age - p.lifeMs) / ALPHA_ANIM_MS);
            disappear = 1.f - u * u * u;
        }
        const float breath = 0.88f + 0.12f * std::sin(static_cast<float>(age) * 0.0032f + p.visualPhase);
        return clamp01(appear * disappear * breath);
    }

    float funnelScale(const WorldParticles::Particle& p) {
        const uint64_t age = particleAgeMs(p.spawnMs);
        const float appear = std::min(1.f, static_cast<float>(age) / 900.f);
        float disappear = 1.f;
        if (age > p.lifeMs) {
            disappear = 1.f - smoothstep(static_cast<float>(age - p.lifeMs) / ALPHA_ANIM_MS);
        }
        const float open = 0.68f + (1.04f - 0.68f) * appear;
        const float close = 0.82f + (1.0f - 0.82f) * disappear;
        return open * close;
    }
}

// =========================================================
// Enable / disable
// =========================================================

void WorldParticles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &WorldParticles::onRenderEvent, nes::event_priority::VERY_FIRST>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &WorldParticles::onBaseTickEvent>(this);
}

void WorldParticles::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &WorldParticles::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &WorldParticles::onBaseTickEvent>(this);
    mParticles.clear();
}

// =========================================================
// Spawn / tick
// =========================================================

WorldParticles::Particle WorldParticles::createParticle(glm::vec3 feet)
{
    const bool bubbles = mMode.mValue == Mode::AirBubbles;
    const float range = std::max(0.1f, mRadius.mValue);
    const float heightRange = std::max(0.1f, mHeight.mValue);

    Particle p{};
    p.pos = glm::vec3(
        feet.x + randomRange(-range, range),
        feet.y + randomRange(0.f, heightRange),
        feet.z + randomRange(-range, range));
    p.prevPos = p.pos;

    if (bubbles) {
        p.motion = glm::vec3(randomRange(-0.42f, 0.42f), randomRange(1.10f, 2.55f), randomRange(-0.42f, 0.42f)) * MOTION_SCALE;
    } else {
        p.motion = glm::vec3(randomRange(-1.f, 1.f), randomRange(0.f, 2.f), randomRange(-1.f, 1.f)) * MOTION_SCALE;
    }
    p.rotationMotion = glm::vec3(randomRange(-1.f, 1.f), randomRange(-1.f, 1.f), randomRange(-1.f, 1.f)) * MOTION_SCALE;

    const float minLife = std::max(1.f, std::min(mLifeMinMs.mValue, mLifeMaxMs.mValue));
    const float maxLife = std::max(minLife, std::max(mLifeMinMs.mValue, mLifeMaxMs.mValue));
    p.lifeMs = static_cast<uint64_t>(randomRange(minLife, maxLife));

    const float minSize = std::max(0.02f, std::min(mSizeMin.mValue, mSizeMax.mValue));
    const float maxSize = std::max(minSize, std::max(mSizeMin.mValue, mSizeMax.mValue));
    p.size = randomRange(minSize, maxSize);

    p.spawnMs = NOW;
    p.visualPhase = randomRange(0.f, 2.f * 3.14159265f);
    p.eyeSpin = randomRange(0.38f, 0.72f);
    p.distortionSpin = randomRange(-0.28f, -0.12f);
    return p;
}

void WorldParticles::onBaseTickEvent(BaseTickEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    Actor* local = ci->getLocalPlayer();
    if (!ci->getLevelRenderer() || !local) return;

    const uint64_t now = NOW;
    std::erase_if(mParticles, [now](const Particle& p) {
        return now - p.spawnMs >= p.lifeMs + ALPHA_ANIM_MS;
    });

    for (Particle& p : mParticles) {
        p.prevPos = p.pos;
        p.prevRotation = p.rotation;
        p.pos += p.motion;
        p.rotation += p.rotationMotion;
        p.motion *= DRAG;
        p.rotationMotion *= DRAG;
    }

    const int target = std::max(1, static_cast<int>(mMaxParticles.mValue));
    const int available = target - static_cast<int>(mParticles.size());
    if (available <= 0) return;

    const glm::vec3 feet = *local->getPos() - PLAYER_HEIGHT_VEC;
    const int spawnCount = std::clamp(static_cast<int>(mSpawnPerTick.mValue), 0, available);
    for (int i = 0; i < spawnCount; i++) {
        mParticles.push_back(createParticle(feet));
    }
}

// =========================================================
// Render
// =========================================================

void WorldParticles::onRenderEvent(RenderEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    if (!ci->getLevelRenderer() || !ci->getLocalPlayer() || mParticles.empty()) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const float displayHeight = ImGui::GetIO().DisplaySize.y;
    const glm::vec3 camera = RenderUtils::transform.mOrigin;
    const float lineWidth = std::max(0.5f, mLineWidth.mValue);
    const int colorModeIndex = static_cast<int>(mColorMode.mValue);
    const float speed = mColorSpeed.mValue;
    const ImColor c1 = mColor.getAsImColor();
    const ImColor c2 = mColor2.getAsImColor();

    for (size_t i = 0; i < mParticles.size(); i++) {
        const Particle& p = mParticles[i];
        const float alpha = particleAlpha(p);
        if (alpha <= 0.003f) continue;

        ImVec2 screen;
        if (!RenderUtils::worldToScreen(p.pos, screen)) continue;

        const float dist = glm::distance(p.pos, camera);
        const int phase = static_cast<int>(mColorPhase.mValue)
                        + static_cast<int>(std::lround(static_cast<float>(i) * mColorSpread.mValue / 90.f))
                        + static_cast<int>(std::lround(p.visualPhase * 57.29578f));
        const ImU32 base = colorFor(colorModeIndex, speed, phase, c1, c2);

        switch (mMode.mValue) {
            case Mode::Quads: {
                // Glow sprite
                const float spriteRadius = screenRadius(p.size * SPRITE_SIZE_MULTIPLIER, dist, displayHeight);
                drawGlow(drawList, screen, spriteRadius, withAlpha(base, alpha * SPRITE_ALPHA_MULTIPLIER));

                // Wireframe cube with body diagonals
                const glm::vec3 rot = p.rotation;
                ImVec2 corners[8];
                bool ok[8];
                for (int c = 0; c < 8; c++) {
                    const glm::vec3 world = p.pos + rotateEuler(CUBE_CORNERS[c] * p.size, rot);
                    ok[c] = RenderUtils::worldToScreen(world, corners[c]);
                }
                const ImU32 outline = withAlpha(base, alpha * OUTLINE_ALPHA_MULTIPLIER);
                const ImU32 diagonal = withAlpha(base, alpha * DIAGONAL_ALPHA_MULTIPLIER);
                for (const auto& e : CUBE_EDGES) {
                    if (ok[e[0]] && ok[e[1]]) drawList->AddLine(corners[e[0]], corners[e[1]], outline, lineWidth);
                }
                for (const auto& d : BODY_DIAGONALS) {
                    if (ok[d[0]] && ok[d[1]]) drawList->AddLine(corners[d[0]], corners[d[1]], diagonal, lineWidth);
                }
                break;
            }
            case Mode::AirBubbles: {
                const float fillR = screenRadius(p.size * BUBBLE_FILL_SIZE_MULTIPLIER, dist, displayHeight);
                const float glowR = screenRadius(p.size * BUBBLE_GLOW_SIZE_MULTIPLIER, dist, displayHeight);
                const float highlightR = screenRadius(p.size * BUBBLE_HIGHLIGHT_SIZE_MULTIPLIER, dist, displayHeight);

                drawList->AddCircleFilled(screen, glowR, withAlpha(base, alpha * BUBBLE_GLOW_ALPHA_MULTIPLIER), 20);
                drawList->AddCircleFilled(screen, fillR, withAlpha(base, alpha * BUBBLE_FILL_ALPHA_MULTIPLIER), 20);
                drawList->AddCircle(screen, fillR, withAlpha(base, alpha * OUTLINE_ALPHA_MULTIPLIER), 20, lineWidth);

                // Highlight, offset toward the upper-left like light on a bubble
                const ImVec2 hl(screen.x - fillR * 0.35f, screen.y - fillR * 0.35f);
                drawList->AddCircleFilled(hl, std::max(1.f, highlightR), withAlpha(IM_COL32_WHITE, alpha * BUBBLE_HIGHLIGHT_ALPHA_MULTIPLIER), 8);
                break;
            }
            case Mode::Funnel: {
                const float s = funnelScale(p);
                const float fa = funnelAlpha(p);
                const float eyeR = screenRadius(p.size * FUNNEL_EYE_SIZE_MULTIPLIER * s, dist, displayHeight);
                const float outerR = screenRadius(p.size * FUNNEL_DISTORTION_OUTER_SIZE_MULTIPLIER * s, dist, displayHeight);
                const float innerR = screenRadius(p.size * FUNNEL_DISTORTION_INNER_SIZE_MULTIPLIER * s, dist, displayHeight);
                const float rimR = screenRadius(p.size * FUNNEL_DISTORTION_SIZE_MULTIPLIER * s, dist, displayHeight);

                drawGlow(drawList, screen, eyeR, withAlpha(base, fa * FUNNEL_EYE_ALPHA_MULTIPLIER));
                drawList->AddCircle(screen, outerR, withAlpha(base, fa * FUNNEL_DISTORTION_OUTER_ALPHA_MULTIPLIER), 28, lineWidth);
                drawList->AddCircle(screen, innerR, withAlpha(base, fa * FUNNEL_DISTORTION_INNER_ALPHA_MULTIPLIER), 24, lineWidth);
                drawList->AddCircle(screen, rimR, withAlpha(base, fa * FUNNEL_RIM_ALPHA_MULTIPLIER), 28, lineWidth);

                // Spokes spin with the funnel
                const float seconds = static_cast<float>(NOW) / 1000.f;
                const float spin = seconds * (p.eyeSpin + p.distortionSpin);
                const ImU32 spoke = withAlpha(base, fa * FUNNEL_SPOKE_ALPHA_MULTIPLIER);
                for (int k = 0; k < 3; k++) {
                    const float a = spin + k * 2.094395f;
                    const ImVec2 dir(std::cos(a), std::sin(a));
                    drawList->AddLine(ImVec2(screen.x + dir.x * innerR * 0.4f, screen.y + dir.y * innerR * 0.4f),
                                      ImVec2(screen.x + dir.x * outerR, screen.y + dir.y * outerR), spoke, lineWidth);
                }
                break;
            }
        }
    }
}
