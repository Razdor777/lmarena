//
// AmbientCubes.cpp — ambient particles that live in the world.
//
// The swarm is filled instantly and kept at full population: particles that
// expire or that you outrun are reborn around you right away, which is what
// makes the air feel full instead of thinning out as you move. The population
// scales with Radius, so widening the field also adds more particles.
//

#include "AmbientCubes.hpp"

#include <algorithm>
#include <cmath>

#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace
{
    constexpr float kPi = 3.14159265f;

    inline float rand01() { return (float)rand() / (float)RAND_MAX; }

    inline ImU32 col32(const ImColor& c, float alpha)
    {
        return IM_COL32(
            (int)(MathUtils::clamp(c.Value.x, 0.f, 1.f) * 255.f),
            (int)(MathUtils::clamp(c.Value.y, 0.f, 1.f) * 255.f),
            (int)(MathUtils::clamp(c.Value.z, 0.f, 1.f) * 255.f),
            (int)(MathUtils::clamp(alpha, 0.f, 1.f) * 255.f));
    }
}

void AmbientCubes::onEnable()
{
    cubes.clear();
}

void AmbientCubes::onDisable()
{
    cubes.clear();
}

// Colour of a particle. colorIndex (0..1) spreads the shades across the theme
// palette so the swarm shimmers instead of being one flat colour.
ImColor AmbientCubes::getColor(float index)
{
    switch (mColorMode.mValue)
    {
    case ColorMode::Custom:
        return mCustomColor.getAsImColor();

    case ColorMode::ThemeStatic:
        return ColorUtils::getThemedColor(0.f);

    case ColorMode::Rainbow: {
        float hue = std::fmod((float)ImGui::GetTime() * 0.08f + index, 1.f);
        if (hue < 0.f) hue += 1.f;
        return ImColor::HSV(hue, 0.72f, 1.f);
    }

    case ColorMode::ThemeFlow:
    default:
        return ColorUtils::getThemedColor(index * 620.f);
    }
}

glm::vec3 AmbientCubes::getAnchorPos()
{
    auto* ci = ClientInstance::get();

    auto* player = ci ? ci->getLocalPlayer() : nullptr;
    glm::vec3 fallback = player
        ? *player->getPos() + glm::vec3(0.f, PLAYER_HEIGHT, 0.f)
        : glm::vec3(0.f);

    glm::vec3 o = RenderUtils::transform.mOrigin;
    if (!std::isfinite(o.x) || !std::isfinite(o.y) || !std::isfinite(o.z)) return fallback;
    if (glm::length(o) < 0.001f) return fallback;
    return o;
}

// Count is a density: doubling Radius roughly quadruples the volume, so scale
// the population to keep the air just as thick when the field grows.
int AmbientCubes::wantedCount() const
{
    const float base = 26.f;
    const float scale = (mRadius.mValue / base) * (mRadius.mValue / base);
    int want = (int)std::lround(mCount.mValue * scale);
    return (int)MathUtils::clamp((float)want, 20.f, 900.f);
}

// Put one particle back into the air around the anchor. Everything random about
// a particle is decided here, so recycling costs nothing extra.
void AmbientCubes::respawn(AmbientCube& c, const glm::vec3& anchor, float maxDist)
{
    const float minDist = (std::min)(3.5f, maxDist * 0.35f);

    // Uniform-ish point in an ellipsoid: wider than it is tall so the swarm
    // hugs the world around you instead of forming a thin shell.
    const float theta = rand01() * 2.f * kPi;
    const float z     = rand01() * 2.f - 1.f;
    const float xy    = std::sqrt((std::max)(0.f, 1.f - z * z));

    glm::vec3 dir{ xy * std::cos(theta), z * 0.62f + 0.22f, xy * std::sin(theta) };
    const float len = glm::length(dir);
    dir = (len > 0.0001f) ? dir / len : glm::vec3(0.f, 1.f, 0.f);

    c.position = anchor + dir * (minDist + rand01() * (maxDist - minDist));

    // Slow, genuinely world-space drift (this is why they float "in the world").
    c.velocity = glm::vec3(
        (rand01() - 0.5f) * 0.45f,
        (rand01() - 0.35f) * 0.22f,
        (rand01() - 0.5f) * 0.45f);

    c.prevPosition = c.position;
    c.phase        = 0.f;
    c.colorIndex   = rand01();
    c.swayPhase    = rand01() * 2.f * kPi;

    // 55% .. 100% of Size — the old "Size Variation" slider, now automatic.
    c.size = (std::max)(0.05f, mSize.mValue * (1.f - 0.45f * rand01()));

    c.maxLife = 9.f + rand01() * 9.f;
    c.life    = c.maxLife;
    c.alpha   = 0.f;      // fades in
}

AmbientCube AmbientCubes::makeCube(const glm::vec3& anchor)
{
    AmbientCube c;
    respawn(c, anchor, mRadius.mValue);

    // Stagger the lifetimes and the fade-in so the whole swarm does not breathe
    // in unison.
    c.life  = c.maxLife * (0.20f + 0.80f * rand01());
    c.alpha = rand01() * 0.6f;
    return c;
}

void AmbientCubes::updateCube(AmbientCube& c, float dt, float t)
{
    c.prevPosition = c.position;

    // Per particle sway — a gentle circulation rather than straight-line motion.
    const glm::vec3 sway{
        std::sin(t * 0.70f + c.swayPhase) * 0.10f + std::sin(t * 0.23f + c.swayPhase * 0.7f) * 0.05f,
        std::sin(t * 0.50f + c.swayPhase * 1.7f) * 0.06f,
        std::cos(t * 0.60f + c.swayPhase) * 0.10f + std::cos(t * 0.19f + c.swayPhase * 0.4f) * 0.05f
    };

    // One breeze that slowly turns, so the swarm drifts together.
    const float windAngle = t * 0.06f + c.swayPhase * 0.15f;
    const glm::vec3 wind{ std::cos(windAngle), 0.06f, std::sin(windAngle) };

    // Vertical bias: positive Rise lifts the swarm, negative lets it settle.
    const glm::vec3 rise{ 0.f, mRise.mValue * 0.35f, 0.f };

    c.position += (c.velocity * mSpeed.mValue + sway + wind * (0.35f * mSpeed.mValue) + rise) * dt;

    c.life -= dt;

    // Smooth alpha: fade in on birth, hold, then fade out before recycling.
    const float p = (c.maxLife > 0.01f) ? MathUtils::clamp(1.f - c.life / c.maxLife, 0.f, 1.f) : 1.f;
    float target = 1.f;
    if (p > 0.82f) target = MathUtils::clamp((1.f - p) / 0.18f, 0.f, 1.f);

    c.alpha += (target - c.alpha) * (std::min)(1.f, dt * 4.f);
    c.phase = p;
}

// ============================================================
// DRAW — one of four shapes, tuned by Size / Alpha / Glow / Trails
// ============================================================

void AmbientCubes::renderCube(const AmbientCube& cube)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    // worldToScreen already bails out safely while GuiData is missing (window
    // resize, world load), so it is the only guard needed here.
    ImVec2 screenPos;
    if (!RenderUtils::worldToScreen(cube.position, screenPos)) return;

    const glm::vec3& camPos = RenderUtils::transform.mOrigin;

    const float dist    = glm::distance(camPos, cube.position);
    const float maxDist = mRadius.mValue * 1.6f;
    if (dist > maxDist) return;

    // Perspective sizing: near = bigger, far = smaller (clamped).
    const float persp = MathUtils::clamp(3.2f / (std::max)(dist, 0.1f), 0.30f, 2.5f);

    // Birth pop: particles grow into their size instead of appearing at full.
    const float pop = (cube.phase < 0.12f) ? (0.60f + 0.40f * (cube.phase / 0.12f)) : 1.f;

    const float sz = cube.size * 30.f * persp * pop;
    if (sz < 1.f) return;

    // Fade the edges of the swarm so its boundary is never visible, and let
    // every particle breathe on its own phase.
    const float edgeFade = MathUtils::clamp(1.25f - dist / maxDist, 0.f, 1.f);
    const float twinkle  = mTwinkle.mValue
        ? (0.80f + 0.20f * std::sin((float)ImGui::GetTime() * 2.1f + cube.swayPhase * 3.1f))
        : 1.f;

    const float alpha = cube.alpha * edgeFade * mAlpha.mValue * twinkle;
    if (alpha <= 0.004f) return;

    const ImColor color = getColor(cube.colorIndex);
    auto* dl = ImGui::GetBackgroundDrawList();
    const ImU32 body = col32(color, alpha);

    // Motion streak.
    if (mTrails.mValue && sz > 1.6f)
    {
        const float streak = (mShape.mValue == Shape::Spark) ? 3.4f : (0.30f * (0.4f + mSpeed.mValue));
        const glm::vec3 tail = cube.position - cube.velocity * streak;
        ImVec2 tailSc;
        if (RenderUtils::worldToScreen(tail, tailSc))
        {
            const float len = glm::distance(glm::vec2(tailSc.x, tailSc.y), glm::vec2(screenPos.x, screenPos.y));
            if (len > 0.75f && len < 420.f)
                dl->AddLine(tailSc, screenPos, col32(color, alpha * 0.5f), (std::max)(1.f, sz * 0.45f));
        }
    }

    switch (mShape.mValue)
    {
    case Shape::Square: {
        if (mGlow.mValue)
            dl->AddCircleFilled(screenPos, sz * 2.1f, col32(color, alpha * 0.16f), 10);

        const float half = sz * 0.5f;
        dl->AddRectFilled({ screenPos.x - half, screenPos.y - half },
                          { screenPos.x + half, screenPos.y + half },
                          body,
                          (std::max)(0.f, half * 0.35f));
        break;
    }

    case Shape::Spark: {
        dl->AddCircleFilled(screenPos, sz * 0.85f, body, 12);
        dl->AddCircleFilled(screenPos, sz * 2.2f, col32(color, alpha * 0.18f), 10);
        break;
    }

    case Shape::Snow: {
        if (mGlow.mValue)
            dl->AddCircleFilled(screenPos, sz * 1.8f, col32(color, alpha * 0.16f), 10);
        dl->AddCircleFilled(screenPos, sz * 0.75f, body, 12);

        const float arm = sz * 1.35f;
        dl->AddLine({ screenPos.x - arm, screenPos.y }, { screenPos.x + arm, screenPos.y }, col32(color, alpha * 0.7f), 1.f);
        dl->AddLine({ screenPos.x, screenPos.y - arm }, { screenPos.x, screenPos.y + arm }, col32(color, alpha * 0.7f), 1.f);
        break;
    }

    case Shape::Orb:
    default: {
        if (mGlow.mValue)
            dl->AddCircleFilled(screenPos, sz * 2.4f, col32(color, alpha * 0.14f), 12);
        dl->AddCircleFilled(screenPos, sz * 1.15f, col32(color, alpha * 0.45f), 14);
        dl->AddCircleFilled(screenPos, sz * 0.65f, body, 14);
        break;
    }
    }
}

void AmbientCubes::onRenderEvent(RenderEvent& event)
{
    if (!mEnabled) return;

    auto* ci = ClientInstance::get();
    if (!ci) return;

    auto* player = ci->getLocalPlayer();
    if (!player || !ci->getGuiData()) return;

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);
    const float t  = (float)ImGui::GetTime();

    const glm::vec3 anchor = getAnchorPos();
    const float radius = mRadius.mValue;
    const int   want   = wantedCount();

    // ── population ─────────────────────────────────────────────────────────
    // Fill instantly (a chunk per frame, so a 600 particle field does not
    // stall the frame), then keep it there.
    int spawnBudget = 200;
    while ((int)cubes.size() < want && spawnBudget-- > 0)
        cubes.push_back(makeCube(anchor));
    while ((int)cubes.size() > want && !cubes.empty())
        cubes.pop_back();

    // ── update / recycle ───────────────────────────────────────────────────
    const float killDist = radius * 1.35f;

    for (auto& c : cubes)
    {
        updateCube(c, dt, t);

        // Expired, or you ran/flew past it: recycle it into the air around you
        // instead of letting it fade away, so the population never thins out.
        if (c.life <= 0.f || !std::isfinite(c.position.x) ||
            glm::distance(c.position, anchor) > killDist)
        {
            respawn(c, anchor, radius);
        }
    }

    // Depth sort: the far ones are drawn first so the near ones land on top.
    std::sort(cubes.begin(), cubes.end(), [&](const AmbientCube& a, const AmbientCube& b) {
        return glm::distance(a.position, anchor) > glm::distance(b.position, anchor);
    });

    for (const auto& cube : cubes)
        renderCube(cube);
}
