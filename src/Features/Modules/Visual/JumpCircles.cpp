//
// Created by alteik on 26/10/2024.
// Rewritten: movement visuals — a clean expanding ground ring on jump/land,
// soft dust, an optional world-anchored trail and ground shadows.
//
// The rings used to be a stack of thin 1px circles that read as a jagged
// mess. Now each ring is stroked in three passes (wide soft glow, mid, crisp
// core) so it looks like a real shockwave on the ground, and the `Speed` /
// `Glow` sliders actually drive it.
//

#include "JumpCircles.hpp"

#include <algorithm>
#include <cmath>

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace
{
    inline bool project(const glm::vec3& world, ImVec2& out)
    {
        return RenderUtils::worldToScreen(world, out);
    }

    inline ImU32 col32(const ImVec4& c, float alpha)
    {
        return IM_COL32(
            (int)(std::clamp(c.x, 0.f, 1.f) * 255.f),
            (int)(std::clamp(c.y, 0.f, 1.f) * 255.f),
            (int)(std::clamp(c.z, 0.f, 1.f) * 255.f),
            (int)(std::clamp(alpha, 0.f, 1.f) * 255.f));
    }

    constexpr float kPi            = 3.14159265f;
    constexpr float kTrailSampleMs = 20.f;   // 50 Hz sampling of the trail
    constexpr float kTrailMoveEps   = 0.02f; // ignore samples from standing still
    constexpr int   kRingSegments   = 64;
}

glm::vec3 JumpCircles::feetPos(Actor* actor) const
{
    if (!actor) return glm::vec3(0.f);

    auto* pos = actor->getPos();
    if (!pos) return glm::vec3(0.f);

    // getPos() is at eye height in this SDK (the AABB getters subtract
    // PLAYER_HEIGHT), so the feet are one player height below it.
    return *pos - glm::vec3(0.f, PLAYER_HEIGHT, 0.f);
}

void JumpCircles::addCircle(const glm::vec3& pos) {
    uint64_t currentTime = NOW;
    if (currentTime - lastAddTime < 100) return;

    ImVec4 color = ColorUtils::getThemedColor(0);
    color.w = mOpacity.mValue;

    Circle newCircle = { pos, 0.1f, mMaxRadius.mValue, color, mGlowAmount.mValue, mOpacity.mValue, currentTime };
    circles.push_back(newCircle);
    lastAddTime = currentTime;
}

// Dust thrown out of the ground: used on takeoff (and, lightly, on landing).
void JumpCircles::addBurst(const glm::vec3& pos, int count, float power)
{
    const uint64_t now = NOW;
    if (now - lastBurst < 60) return;
    lastBurst = now;

    for (int i = 0; i < count; i++) {
        const float ang = (float)i / (float)count * 2.f * kPi + MathUtils::randomFloat(-0.25f, 0.25f);
        const float spd = power * MathUtils::randomFloat(0.6f, 1.2f);

        Puff p;
        p.pos     = pos + glm::vec3(std::cos(ang) * 0.22f, 0.05f, std::sin(ang) * 0.22f);
        p.vel     = glm::vec3(std::cos(ang) * spd, MathUtils::randomFloat(0.7f, 1.6f) * power, std::sin(ang) * spd);
        p.size    = MathUtils::randomFloat(0.035f, 0.075f);
        p.maxLife = MathUtils::randomFloat(0.35f, 0.7f);
        p.life    = p.maxLife;
        puffs.push_back(p);
    }
}

void JumpCircles::updatePuffs(float dt)
{
    for (auto it = puffs.begin(); it != puffs.end(); ) {
        it->life -= dt;
        if (it->life <= 0.f) { it = puffs.erase(it); continue; }

        it->vel.y -= 9.5f * dt;                         // gravity
        it->vel   *= (1.f - std::min(0.9f, 3.5f * dt)); // air drag
        it->pos   += it->vel * dt;
        ++it;
    }
}

void JumpCircles::drawPuffs()
{
    if (puffs.empty()) return;

    auto* dl = ImGui::GetBackgroundDrawList();

    for (const auto& p : puffs) {
        ImVec2 screen;
        if (!project(p.pos, screen)) continue;

        const float t = std::clamp(p.life / p.maxLife, 0.f, 1.f);
        const ImVec4 c = ColorUtils::getThemedColor((float)p.size * 900.f);
        const float alpha = t * t * 0.8f;

        // Cheap size falloff by distance so the dust reads as 3D.
        const float dist  = glm::distance(RenderUtils::transform.mOrigin, p.pos);
        const float persp = MathUtils::clamp(3.2f / (std::max)(dist, 0.1f), 0.3f, 2.5f);
        const float r     = (std::max)(0.6f, p.size * 26.f * persp * t);

        dl->AddCircleFilled(screen, r * 2.0f, col32(c, alpha * 0.18f), 10);
        dl->AddCircleFilled(screen, r, col32(c, alpha * 0.85f), 10);
    }
}

// A soft ellipse on the ground. Drawn as a set of flat, very thin quads so it
// inherits the perspective of the world instead of being a 2D circle pasted on
// screen.
void JumpCircles::drawShadow(const glm::vec3& feet, float groundY, float alphaScale)
{
    auto* dl = ImGui::GetBackgroundDrawList();

    const float height = std::clamp(feet.y - groundY, 0.f, 6.f);
    const float spread = 1.f + height * 0.12f;
    const float fade   = std::clamp(1.f - height / 4.5f, 0.12f, 1.f);
    const float base   = mShadowAlpha.mValue * fade * alphaScale;
    const float r      = mShadowSize.mValue * spread;

    for (int layer = 3; layer >= 1; --layer) {
        const float k = (float)layer / 3.f;
        const float lr = r * (0.72f + 0.28f * k);

        const glm::vec3 centre{ feet.x, groundY + 0.02f, feet.z };
        const AABB box(centre - glm::vec3(lr, 0.004f, lr), centre + glm::vec3(lr, 0.01f, lr), true);

        auto pts = MathUtils::getImBoxPoints(box);
        if (pts.size() < 3) continue;

        dl->AddConvexPolyFilled(pts.data(), (int)pts.size(),
                                IM_COL32(0, 0, 0, (int)(base * 255.f * (0.35f + 0.25f * k))));
    }
}

// Project the ground circle into screen space. `valid[i]` is false when the
// point is behind the camera, so callers can skip the bad segments instead of
// drawing a line across the whole screen.
std::vector<ImVec2> JumpCircles::projectRing(const Circle& circle, float radius, std::vector<bool>& valid) const
{
    std::vector<ImVec2> pts;
    pts.reserve(kRingSegments + 1);
    valid.assign(kRingSegments + 1, false);

    for (int j = 0; j <= kRingSegments; j++) {
        const float angle = (float)j / (float)kRingSegments * 2.f * kPi;
        const glm::vec3 worldPos = {
            circle.position.x + radius * std::cos(angle),
            circle.position.y,
            circle.position.z + radius * std::sin(angle)
        };

        ImVec2 screenPos;
        valid[j] = project(worldPos, screenPos);
        pts.push_back(screenPos);
    }

    return pts;
}

void JumpCircles::drawRing(const Circle& circle, float radius, float thickness, float alpha, const ImVec4& baseColor)
{
    if (radius <= 0.01f || alpha <= 0.005f) return;

    auto* dl = ImGui::GetBackgroundDrawList();

    std::vector<bool> valid;
    auto pts = projectRing(circle, radius, valid);
    const int n = (int)pts.size();
    if (n < 3) return;

    // Translucent disc inside the ring — only when the whole ellipse is in
    // front of the camera, otherwise the polygon would wrap around the screen.
    bool allValid = true;
    for (int i = 0; i < n; i++) { if (!valid[i]) { allValid = false; break; } }

    if (mFill.mValue && allValid)
        dl->AddConvexPolyFilled(pts.data(), n, col32(baseColor, alpha * 0.10f));

    auto stroke = [&](float w, float a) {
        if (a <= 0.004f) return;
        for (int i = 1; i < n; i++) {
            if (!valid[i - 1] || !valid[i]) continue;
            dl->AddLine(pts[i - 1], pts[i], col32(baseColor, a), w);
        }
    };

    // Three passes: wide soft glow, mid halo, crisp core.
    const float glow = std::clamp((float)mGlowAmount.mValue / 100.f, 0.f, 1.f);
    if (glow > 0.01f) {
        stroke(thickness * (2.6f + 2.2f * glow), alpha * 0.14f * glow);
        stroke(thickness * 1.5f, alpha * 0.30f);
    }
    stroke(thickness, alpha);
}

void JumpCircles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &JumpCircles::onRenderEvent>(this);

    circles.clear();
    trail.clear();
    puffs.clear();
    mOtherGroundY.clear();
    mGroundKnown = false;
    lastTrailSample = 0;
}

void JumpCircles::onDisable()
{
    circles.clear();
    trail.clear();
    puffs.clear();
    mOtherGroundY.clear();
    gFeatureManager->mDispatcher->deafen<RenderEvent, &JumpCircles::onRenderEvent>(this);
}

void JumpCircles::onRenderEvent(RenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    auto* player = ci->getLocalPlayer();
    if (!player || !player->getLevel() || !ci->getGuiData()) return;

    const uint64_t currentTime = NOW;
    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);

    auto* drawList = ImGui::GetBackgroundDrawList();

    // ── takeoff / landing ─────────────────────────────────────────────────
    const bool onGround = player->isOnGround();
    const glm::vec3 feet = feetPos(player);

    if (!mGroundKnown) { mGroundY = feet.y; mGroundKnown = true; }

    if (!player->wasOnGround() && onGround) {
        glm::vec3 ringPos = feet;
        ringPos.y -= 0.05f;
        addCircle(ringPos);
        if (mJumpBurst.mValue) addBurst(feet, 8, 0.9f);
    }
    else if (player->wasOnGround() && !onGround) {
        if (mJumpBurst.mValue) addBurst(feet, 12, 1.25f);
    }

    if (onGround) mGroundY = feet.y;

    updatePuffs(dt);

    // ── trail sampling ────────────────────────────────────────────────────
    if (mTrail.mValue) {
        if (currentTime - lastTrailSample >= (uint64_t)kTrailSampleMs) {
            lastTrailSample = currentTime;

            const glm::vec3 sample = feet + glm::vec3(0.f, 0.12f, 0.f);

            if (trail.empty() || glm::distance(sample, trail.back().pos) >= kTrailMoveEps)
                trail.push_back({ sample, currentTime });
        }

        const uint64_t maxAge = (uint64_t)(mTrailLength.mValue * 1000.f);
        while (!trail.empty() && currentTime - trail.front().time > maxAge)
            trail.erase(trail.begin());

        if (trail.size() > 400) trail.erase(trail.begin(), trail.end() - 400);
    } else if (!trail.empty()) {
        trail.clear();
    }

    // ── shadows ───────────────────────────────────────────────────────────
    if (mShadow.mValue) {
        drawShadow(feet, mGroundY, 1.f);

        if (mOthers.mValue) {
            for (auto* actor : ActorUtils::getActorList(true, true)) {
                if (!actor || actor == player) continue;
                if (!actor->isPlayer()) continue;
                if (!actor->getPos()) continue;

                const int64_t id = actor->getRuntimeID();
                const glm::vec3 otherFeet = feetPos(actor);

                if (actor->isOnGround() || !mOtherGroundY.count(id))
                    mOtherGroundY[id] = otherFeet.y;

                drawShadow(otherFeet, mOtherGroundY[id], 0.85f);
            }
            // Forget players that left.
            for (auto it = mOtherGroundY.begin(); it != mOtherGroundY.end(); )
                it = ActorUtils::getActorFromRuntimeID(it->first) ? ++it : mOtherGroundY.erase(it);
        } else if (!mOtherGroundY.empty()) {
            mOtherGroundY.clear();
        }
    }

    // ── trail drawing ─────────────────────────────────────────────────────
    if (mTrail.mValue && trail.size() >= 2) {
        const uint64_t maxAge = (uint64_t)(mTrailLength.mValue * 1000.f);
        const float width = mTrailWidth.mValue;

        if (mTrailStyle.mValue == TrailStyle::Ribbon) {
            for (int pass = 1; pass >= 0; --pass) {
                for (size_t i = 1; i < trail.size(); i++) {
                    const float age = 1.f - (float)(currentTime - trail[i].time) / (float)maxAge;
                    if (age <= 0.f) continue;

                    ImVec2 a, b;
                    if (!project(trail[i - 1].pos, a)) continue;
                    if (!project(trail[i].pos, b)) continue;

                    const ImVec4 c = ColorUtils::getThemedColor((float)i * 0.35f);
                    const float alpha = (pass == 0 ? 0.85f : 0.16f) * std::clamp(age, 0.f, 1.f) * mOpacity.mValue;
                    const float w = (pass == 0 ? width : width * 3.2f) * (0.35f + 0.65f * std::clamp(age, 0.f, 1.f));

                    drawList->AddLine(a, b, col32(c, alpha), (std::max)(0.7f, w));
                }
            }
        } else {
            for (size_t i = 0; i < trail.size(); i++) {
                const float age = 1.f - (float)(currentTime - trail[i].time) / (float)maxAge;
                if (age <= 0.f) continue;

                ImVec2 p;
                if (!project(trail[i].pos, p)) continue;

                const ImVec4 c = ColorUtils::getThemedColor((float)i * 0.35f);
                const float alpha = std::clamp(age, 0.f, 1.f) * mOpacity.mValue;

                if (i > 0) {
                    ImVec2 prev;
                    if (project(trail[i - 1].pos, prev))
                        drawList->AddLine(prev, p, col32(c, alpha * 0.25f), 1.f);
                }

                drawList->AddCircleFilled(p, width * 2.1f, col32(c, alpha * 0.16f), 12);
                drawList->AddCircleFilled(p, width * 0.85f, col32(c, alpha * 0.9f), 12);
            }
        }
    }

    drawPuffs();

    // ── rings ─────────────────────────────────────────────────────────────
    const int ringCount = (int)mRings.mValue;
    const float thickness = mThickness.mValue;
    const float lifeTime = mLifeTime.mValue;

    if (lifeTime <= 1.f) return;

    for (auto it = circles.begin(); it != circles.end(); ) {
        const uint64_t elapsed = currentTime - it->startTime;
        if (elapsed > (uint64_t)lifeTime) { it = circles.erase(it); continue; }

        const float lifeProgress = (float)elapsed / lifeTime; // 0 -> 1

        // Expansion curve. `Speed` bends it: higher = the ring punches out
        // fast and then settles, lower = a slow, even swell.
        const float speedN = std::clamp(mSpeed.mValue / 0.20f, 0.f, 1.f);
        const float exponent = 1.4f + speedN * 2.6f;
        const float ease = 1.f - std::pow(1.f - lifeProgress, exponent);

        const float radius = mMaxRadius.mValue * ease;

        // Hold full opacity for the first 25% of the life, then fade out.
        float fade = 1.f;
        if (lifeProgress > 0.25f) fade = 1.f - (lifeProgress - 0.25f) / 0.75f;
        fade = std::clamp(fade, 0.f, 1.f);
        const float alpha = mOpacity.mValue * fade;
        if (alpha <= 0.005f) { ++it; continue; }

        const ImVec4 color = ColorUtils::getThemedColor((float)elapsed * 0.08f);

        if (mStyle.mValue == Style::Shockwave) {
            drawRing(*it, radius, thickness * 0.7f, alpha, color);
        }
        else if (mStyle.mValue == Style::Disc) {
            drawRing(*it, radius, thickness, alpha * 0.9f, color);
        }
        else if (mStyle.mValue == Style::Ring) {
            drawRing(*it, radius, thickness, alpha, color);
        }
        else // Ripple
        {
            const int count = (std::max)(1, ringCount);
            const float spacing = radius / (float)(count + 1);
            for (int r = 0; r < count; r++) {
                const float rr = radius - r * spacing;
                const float rAlpha = alpha * (1.f - (float)r / (float)count);
                const float rThick = thickness * (1.f - (float)r / (float)count * 0.4f);
                drawRing(*it, rr, rThick, rAlpha, color);
            }
        }

        ++it;
    }
}
