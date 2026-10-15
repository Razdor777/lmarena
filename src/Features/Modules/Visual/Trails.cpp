//
// Ported from Combatant Trails. Drawn in 2D on the ImGui background draw list.
//
// Dropped from Combatant:
//  - 3D depth test (ImGui draws over the world)
//  - Textured sprites (FIRE_FLY / RANDOM_PARTICLES): tail dots and sparks are procedural circles
//  - Projectile sparks (Juzdex has no projectile entity query here)
//  - Light Rainbow / Sky / Fade / Analogous color modes (same set as JumpCircles)
//  - Partial-tick interpolation (Juzdex renders without tickDelta)
//

#include "Trails.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace TrailsImpl
{
    constexpr float PARTICLE_GRAVITY = 0.01f;
    constexpr float PARTICLE_DRAG = 0.96f;
    constexpr size_t PARTICLE_MAX = 600;
    constexpr int CONNECT_MAX_STEPS = 6;
    constexpr float RAINBOW_SPATIAL_SCALE = 1.f;

    inline float clamp01(float v) {
        return std::clamp(v, 0.f, 1.f);
    }

    inline float randomRange(float min, float max) {
        return min + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (max - min);
    }

    // Trail color mode: 0 static, 1 rainbow, 2 double color, 3 theme
    inline ImColor colorFor(int mode, float speed, int index, ImColor c1, ImColor c2) {
        const float seconds = static_cast<float>(NOW) * speed / 18000.f;
        const int spatial = static_cast<int>(index * RAINBOW_SPATIAL_SCALE);
        switch (mode) {
            case 1: return ColorUtils::Rainbow(seconds, 1.f, 1.f, ((spatial % 360) + 360) % 360);
            case 2: return ColorUtils::LerpColors(seconds, static_cast<float>(spatial), { c1, c2 });
            case 3: return ColorUtils::getThemedColor(static_cast<float>(spatial));
            default: return c1;
        }
    }

    inline ImU32 withAlpha(ImU32 color, float alphaMul) {
        const uint32_t a = std::clamp(static_cast<uint32_t>(((color >> 24) & 0xFF) * clamp01(alphaMul)), 0u, 255u);
        return (color & 0x00FFFFFFu) | (a << 24);
    }

    // Mix toward white (Combatant's "lift" on the head of the trail)
    inline ImU32 liftToWhite(ImU32 color, float t) {
        t = clamp01(t);
        const auto mixChannel = [t](uint32_t c) {
            return static_cast<uint32_t>(c + (255.f - static_cast<float>(c)) * t);
        };
        const uint32_t r = mixChannel((color >> 0) & 0xFF);
        const uint32_t g = mixChannel((color >> 8) & 0xFF);
        const uint32_t b = mixChannel((color >> 16) & 0xFF);
        return (color & 0xFF000000u) | (b << 16) | (g << 8) | r;
    }

    inline ImU32 averageColor(ImU32 a, ImU32 b) {
        const auto avg = [](uint32_t x, uint32_t y) { return (x + y) / 2; };
        return (avg((a >> 24) & 0xFF, (b >> 24) & 0xFF) << 24) |
               (avg((b >> 16) & 0xFF, (a >> 16) & 0xFF) << 16) |
               (avg((b >> 8) & 0xFF, (a >> 8) & 0xFF) << 8) |
               avg(a & 0xFF, b & 0xFF);
    }

    // Screen-space radius for a world-space size at distance (approximate, ~70 deg vertical FOV)
    inline float screenRadius(float worldSize, float distance, float displayHeight) {
        const float d = std::max(0.5f, distance);
        return std::clamp(worldSize * displayHeight / (2.8f * d), 1.f, 80.f);
    }

    // Soft dot: a bright core and a faint halo
    inline void drawGlow(ImDrawList* drawList, ImVec2 center, float radius, ImU32 color) {
        drawList->AddCircleFilled(center, radius * 1.8f, withAlpha(color, 0.25f), 12);
        drawList->AddCircleFilled(center, radius, color, 12);
    }
}

using namespace TrailsImpl;

// =========================================================
// Enable / disable
// =========================================================

void Trails::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Trails::onRenderEvent, nes::event_priority::VERY_FIRST>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Trails::onBaseTickEvent>(this);
}

void Trails::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Trails::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Trails::onBaseTickEvent>(this);
    mTrails.clear();
    mLastPos.clear();
    mGroundState.clear();
    mParticles.clear();
}

// =========================================================
// Spawn sparks
// =========================================================

void Trails::spawnParticle(glm::vec3 pos, float motionScale, int colorIndex)
{
    if (mParticles.size() >= PARTICLE_MAX) {
        mParticles.erase(mParticles.begin());
    }

    const float motion = 0.06f * motionScale;
    Particle p{};
    p.pos = pos;
    p.velocity = glm::vec3(randomRange(-motion, motion), randomRange(-0.015f, 0.015f), randomRange(-motion, motion));
    p.spawnedAtMs = NOW;
    p.lifeMs = std::max<uint64_t>(1, static_cast<uint64_t>(mParticleLifeMs.mValue));
    p.fadeInMs = std::min<uint64_t>(200, p.lifeMs / 4);
    p.fadeOutMs = std::min<uint64_t>(700, p.lifeMs / 2);
    p.size = mParticleSize.mValue;

    if (mParticleRandomColor.mValue) {
        ImVec4 rgb{};
        ImGui::ColorConvertHSVtoRGB(randomRange(0.f, 1.f), randomRange(0.7f, 1.f), randomRange(0.8f, 1.f),
                                    rgb.x, rgb.y, rgb.z);
        rgb.w = 1.f;
        p.color = ImColor(rgb);
    } else {
        p.color = colorFor(static_cast<int>(mColorMode.mValue), mColorSpeed.mValue, colorIndex,
                           mColor.getAsImColor(), mColor2.getAsImColor());
    }
    mParticles.push_back(p);
}

void Trails::spawnParticlesAround(glm::vec3 feet, int count, float radius, float height, float motionScale, int colorIndex)
{
    for (int i = 0; i < count; i++) {
        glm::vec3 pos(
            feet.x + randomRange(0.f, 1.f) * radius - radius * 0.5f,
            feet.y + randomRange(0.f, 1.f) * std::max(0.1f, height),
            feet.z + randomRange(0.f, 1.f) * radius - radius * 0.5f);
        spawnParticle(pos, motionScale, colorIndex);
    }
}

// =========================================================
// Tick: record movement, age points, spawn sparks
// =========================================================

void Trails::onBaseTickEvent(BaseTickEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    Actor* local = ci->getLocalPlayer();
    if (!ci->getLevelRenderer() || !local) return;

    const int colorIndex = static_cast<int>((NOW / 50) % 360);
    const bool particles = mParticlesEnabled.mValue;
    const bool firstPerson = ci->getOptions()->mThirdPerson->value == 0;

    std::vector<Actor*> actors = ActorUtils::getActorList(true, true);
    if (std::find(actors.begin(), actors.end(), local) == actors.end()) {
        actors.push_back(local);
    }

    std::unordered_map<int64_t, bool> active;
    for (Actor* actor : actors) {
        if (!actor) continue;
        if (mOnlySelf.mValue && actor != local) continue;

        const int64_t rid = actor->getRuntimeID();
        active[rid] = true;

        const glm::vec3 pos = *actor->getPos();
        const glm::vec3 last = mLastPos.contains(rid) ? mLastPos[rid] : pos;
        const bool moved = pos.x != last.x || pos.z != last.z;
        const bool isSelf = actor == local;
        const bool skipSelfFirstPerson = mHideFirstPerson.mValue && isSelf && firstPerson;

        auto& trail = mTrails[rid];
        if (!skipSelfFirstPerson && moved) {
            TrailPoint point{};
            point.from = last;
            point.to = pos;
            point.color = colorFor(static_cast<int>(mColorMode.mValue), mColorSpeed.mValue, colorIndex,
                                   mColor.getAsImColor(), mColor2.getAsImColor());
            point.maxTicks = std::max(1, static_cast<int>(mLifeTimeTicks.mValue));
            point.ticks = point.maxTicks;
            point.prevTicks = point.ticks;
            trail.push_back(point);
        }

        for (auto it = trail.begin(); it != trail.end();) {
            it->prevTicks = it->ticks;
            if (it->ticks-- <= 0) {
                it = trail.erase(it);
            } else {
                ++it;
            }
        }

        const bool skipParticlesSelf = mHideFirstPerson.mValue && isSelf && firstPerson;
        if (particles && !skipParticlesSelf) {
            if (mParticleWalk.mValue && moved) {
                spawnParticlesAround(pos, static_cast<int>(mParticleWalkCount.mValue), 0.5f, 0.4f, 0.1f, colorIndex);
            }
            const bool nowGround = actor->isOnGround();
            const bool prevGround = mGroundState.contains(rid) ? mGroundState[rid] : nowGround;
            if (mParticleJump.mValue && prevGround && !nowGround) {
                spawnParticlesAround(pos, static_cast<int>(mParticleJumpCount.mValue), 0.4f, 0.1f, 0.2f, colorIndex);
            }
            mGroundState[rid] = nowGround;
        }

        mLastPos[rid] = pos;
    }

    // Forget actors that are gone
    std::erase_if(mTrails, [&](const auto& e) { return !active.contains(e.first); });
    std::erase_if(mLastPos, [&](const auto& e) { return !active.contains(e.first); });
    std::erase_if(mGroundState, [&](const auto& e) { return !active.contains(e.first); });

    // Sparks: age and move
    const uint64_t now = NOW;
    std::erase_if(mParticles, [now](Particle& p) {
        return now - p.spawnedAtMs >= p.lifeMs;
    });
    for (Particle& p : mParticles) {
        p.pos += p.velocity;
        p.velocity = p.velocity * PARTICLE_DRAG + glm::vec3(0.f, -PARTICLE_GRAVITY, 0.f);
    }
}

// =========================================================
// Render
// =========================================================

void Trails::onRenderEvent(RenderEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    Actor* local = ci->getLocalPlayer();
    if (!ci->getLevelRenderer() || !local) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const glm::vec3 camera = RenderUtils::transform.mOrigin;
    const bool firstPerson = ci->getOptions()->mThirdPerson->value == 0;
    const uint64_t now = NOW;

    // Sparks
    if (mParticlesEnabled.mValue) {
        for (const Particle& p : mParticles) {
            const float age = static_cast<float>(now - p.spawnedAtMs);
            float alpha = 1.f;
            if (p.fadeInMs > 0) alpha = std::min(alpha, age / static_cast<float>(p.fadeInMs));
            if (p.fadeOutMs > 0 && now - p.spawnedAtMs >= p.lifeMs - p.fadeOutMs) {
                alpha = std::min(alpha, static_cast<float>(p.lifeMs - (now - p.spawnedAtMs)) / static_cast<float>(p.fadeOutMs));
            }
            alpha = clamp01(alpha);
            if (alpha <= 0.01f) continue;

            ImVec2 screen;
            if (!RenderUtils::worldToScreen(p.pos, screen)) continue;
            const float dist = glm::distance(p.pos, camera);
            const float radius = screenRadius(p.size, dist, display.y);
            drawGlow(drawList, screen, radius, withAlpha(static_cast<ImU32>(p.color), alpha));
        }
    }

    const int phaseBase = static_cast<int>(std::lround(static_cast<float>(now) / 50.f * 4.f)) + static_cast<int>(mColorPhase.mValue);
    const bool connect = mConnectedPath.mValue;
    const float step = std::max(0.05f, mConnectStep.mValue);

    // Helper: color of point i in a trail of given size
    auto pointColor = [&](const TrailPoint& pt, int index, int size) -> ImU32 {
        const float position = size <= 1 ? 0.f : index / static_cast<float>(size - 1);
        const float animation = pt.maxTicks > 0 ? static_cast<float>(pt.ticks) / static_cast<float>(pt.maxTicks) : 1.f;
        const float age = 1.f - animation;
        const int phase = phaseBase + static_cast<int>(std::lround(position * mColorLengthSpread.mValue))
                                    + static_cast<int>(std::lround(age * mColorAgeSpread.mValue));

        ImU32 animated = pt.color;
        if (mColorMode.mValue != ColorMode::Static) {
            animated = colorFor(static_cast<int>(mColorMode.mValue), mColorSpeed.mValue, phase,
                                mColor.getAsImColor(), mColor2.getAsImColor());
        }
        const float lift = std::pow(1.f - position, 2.f) * 0.45f;
        return liftToWhite(animated, lift);
    };

    for (Actor* actor : ActorUtils::getActorList(true, true)) {
        if (!actor) continue;
        if (mOnlySelf.mValue && actor != local) continue;
        if (mHideFirstPerson.mValue && actor == local && firstPerson) continue;
        auto it = mTrails.find(actor->getRuntimeID());
        if (it == mTrails.end()) continue;
        const auto& trail = it->second;
        const int size = static_cast<int>(trail.size());
        if (size == 0) continue;

        if (mTrailMode.mValue == TrailMode::Tail) {
            // Tail: glowing dots, optionally filled along the segment
            for (int i = 0; i < size; i++) {
                const TrailPoint& pt = trail[i];
                const float animation = pt.maxTicks > 0 ? static_cast<float>(pt.ticks) / static_cast<float>(pt.maxTicks) : 1.f;
                const ImU32 col = withAlpha(pointColor(pt, i, size), animation);

                const int steps = connect ? std::min(CONNECT_MAX_STEPS, static_cast<int>(std::ceil(glm::distance(pt.from, pt.to) / step))) : 0;
                const int count = steps <= 1 ? 1 : steps + 1;
                for (int s = 0; s < count; s++) {
                    const float t = count == 1 ? 1.f : s / static_cast<float>(count - 1);
                    const glm::vec3 p = glm::mix(pt.from, pt.to, t) + glm::vec3(0.f, 0.9f, 0.f);
                    ImVec2 screen;
                    if (!RenderUtils::worldToScreen(p, screen)) continue;
                    const float radius = screenRadius(mScale.mValue, glm::distance(p, camera), display.y);
                    drawGlow(drawList, screen, radius, col);
                }
            }
        } else {
            // Trail: ribbon between consecutive points
            for (int i = 1; i < size; i++) {
                const TrailPoint& prev = trail[i - 1];
                const TrailPoint& cur = trail[i];
                const ImU32 col0 = withAlpha(pointColor(prev, i - 1, size),
                    prev.maxTicks > 0 ? static_cast<float>(prev.ticks) / static_cast<float>(prev.maxTicks) : 1.f);
                const ImU32 col1 = withAlpha(pointColor(cur, i, size),
                    cur.maxTicks > 0 ? static_cast<float>(cur.ticks) / static_cast<float>(cur.maxTicks) : 1.f);

                const glm::vec3 p0 = prev.to;
                const glm::vec3 p1 = cur.to;
                const glm::vec3 down(0.f, mTrailDown.mValue, 0.f);
                const glm::vec3 up(0.f, mTrailDown.mValue + mTrailHeight.mValue, 0.f);

                ImVec2 b0, t0, b1, t1;
                if (!RenderUtils::worldToScreen(p0 + down, b0)) continue;
                if (!RenderUtils::worldToScreen(p0 + up, t0)) continue;
                if (!RenderUtils::worldToScreen(p1 + down, b1)) continue;
                if (!RenderUtils::worldToScreen(p1 + up, t1)) continue;

                drawList->AddQuadFilled(b0, b1, t1, t0, averageColor(col0, col1));

                if (mTrailStroke.mValue) {
                    const float alphaMul = mTrailStrokeAlpha.mValue;
                    const float width = std::max(0.5f, mTrailStrokeWidth.mValue);
                    drawList->AddLine(b0, b1, withAlpha(col0, alphaMul), width);
                    drawList->AddLine(t0, t1, withAlpha(col1, alphaMul), width);
                }
            }
        }
    }
}
