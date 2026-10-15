//
// Ported from Combatant TazikHat. Drawn in 2D: every bowl segment is projected
// to screen space and filled on the ImGui background draw list.
//
// Dropped from Combatant:
//  - 3D depth test (ImGui draws over the world; hats are not occluded by blocks)
//  - Use Relations (Juzdex has no category colors)
//  - Hit wobble on damage (no hurt-time data on Juzdex actors)
//  - Light Rainbow / Sky / Fade / Analogous color modes (same set as JumpCircles)
//

#include "TazikHat.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/ActorHeadRotationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorRotationComponent.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/BlockUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace TazikHatImpl
{
    constexpr int SEGMENTS = 56;
    constexpr uint64_t DEATH_LIFE_MS = 3600;
    constexpr uint64_t DEATH_HANDLED_RESET_MS = 8000;
    constexpr float RAINBOW_SPATIAL_SCALE = 12.f;
    constexpr float PI = 3.14159265358979f;

    constexpr float BOWL_RIM_RADIUS = 0.42f;
    constexpr float BOWL_TOP_RADIUS = 0.245f;
    constexpr float BOWL_HEIGHT = 0.18f;
    constexpr float BOWL_RIM_WIDTH = 0.052f;

    // Head basis (or world basis for flying hats)
    struct TazikBasis {
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 forward;
    };

    struct TazikAnchor {
        glm::vec3 origin;
        TazikBasis basis;
    };

    struct TazikPoint {
        ImVec2 screen;
        bool ok;
    };

    inline glm::vec3 safeNormalize(glm::vec3 v, glm::vec3 fallback) {
        float len2 = glm::dot(v, v);
        if (len2 < 1.0e-6f) return fallback;
        return v / std::sqrt(len2);
    }

    inline float randomRange(float min, float max) {
        return min + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (max - min);
    }

    // Local rotation: yaw around Y, pitch around X, roll around Z (degrees)
    inline glm::vec3 rotateLocal(glm::vec3 p, float yawDeg, float pitchDeg, float rollDeg) {
        float yaw = yawDeg * PI / 180.f;
        float pitch = pitchDeg * PI / 180.f;
        float roll = rollDeg * PI / 180.f;

        float cy = std::cos(yaw), sy = std::sin(yaw);
        p = glm::vec3(p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy);

        float cp = std::cos(pitch), sp = std::sin(pitch);
        p = glm::vec3(p.x, p.y * cp - p.z * sp, p.y * sp + p.z * cp);

        float cr = std::cos(roll), sr = std::sin(roll);
        p = glm::vec3(p.x * cr - p.y * sr, p.x * sr + p.y * cr, p.z);
        return p;
    }

    inline glm::vec3 toWorld(const TazikBasis& b, glm::vec3 local) {
        return b.right * local.x + b.up * local.y + b.forward * local.z;
    }

    inline TazikPoint project(glm::vec3 world) {
        TazikPoint out{};
        out.ok = RenderUtils::worldToScreen(world, out.screen);
        return out;
    }

    inline float clamp01(float v) {
        return std::clamp(v, 0.f, 1.f);
    }

    inline float smoothstep(float t) {
        t = clamp01(t);
        return t * t * (3.f - 2.f * t);
    }

    inline ImColor scaleColor(ImColor c, float brightness, float alphaMul) {
        return ImColor(
            clamp01(c.Value.x * brightness),
            clamp01(c.Value.y * brightness),
            clamp01(c.Value.z * brightness),
            clamp01(c.Value.w * alphaMul));
    }

    // Same mapping as JumpCircles: animated colors use a spatial index around the bowl
    inline ImColor colorFor(int mode, float speed, int index, ImColor primary, ImColor secondary) {
        const float seconds = static_cast<float>(NOW) * speed / 18000.f;
        const int spatial = ((static_cast<int>(std::lround(index * RAINBOW_SPATIAL_SCALE)) % 3600) + 3600) % 3600;
        switch (mode) {
            case 1: return ColorUtils::Rainbow(seconds, 1.f, 1.f, spatial);
            case 2: return ColorUtils::LerpColors(seconds, static_cast<float>(spatial), { primary, secondary });
            case 3: return ColorUtils::getThemedColor(static_cast<float>(spatial));
            default: return primary;
        }
    }

    inline TazikAnchor headAnchor(Actor* actor, float topOffset, float forwardOffset, float rightOffset) {
        glm::vec3 eye = *actor->getPos();

        float yawDeg = 0.f;
        float pitchDeg = 0.f;
        if (auto* head = actor->getActorHeadRotationComponent()) yawDeg = head->mHeadRot;
        if (auto* rot = actor->getActorRotationComponent()) pitchDeg = rot->mPitch;

        const float yaw = yawDeg * PI / 180.f;
        const float pitch = pitchDeg * PI / 180.f;
        const glm::vec3 look(-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch));

        TazikBasis b{};
        b.forward = safeNormalize(look, glm::vec3(0.f, 0.f, 1.f));
        b.right = safeNormalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), b.forward), glm::vec3(1.f, 0.f, 0.f));
        b.up = safeNormalize(glm::cross(b.forward, b.right), glm::vec3(0.f, 1.f, 0.f));

        TazikAnchor anchor{};
        anchor.origin = eye + b.up * topOffset + b.forward * forwardOffset + b.right * rightOffset;
        anchor.basis = b;
        return anchor;
    }

    inline TazikAnchor worldAnchor(glm::vec3 origin) {
        TazikAnchor anchor{};
        anchor.origin = origin;
        anchor.basis = { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f) };
        return anchor;
    }

    inline bool hasHelmet(Actor* actor) {
        auto* container = actor->getArmorContainer();
        if (!container) return false;
        // Armor container slot 0 is the head slot
        auto* item = container->getItem(0);
        return item && item->mItem;
    }

    // Draws one bowl: walls, inner top, brim band, top cap, and outlines
    inline void drawBowl(ImDrawList* drawList,
                         const TazikAnchor& anchor,
                         float size,
                         float yawDeg,
                         float pitchDeg,
                         float rollDeg,
                         float alphaMul,
                         int seed,
                         int mode,
                         float speed,
                         ImColor primary,
                         ImColor secondary,
                         ImColor rim) {
        const float rimRadius = BOWL_RIM_RADIUS * size;
        const float topRadius = BOWL_TOP_RADIUS * size;
        const float height = BOWL_HEIGHT * size;
        const float rimWidth = BOWL_RIM_WIDTH * size;
        const float rimOuterY = 0.055f * size;
        const float rimInnerY = 0.073f * size;
        const float topOuterY = height * 0.82f;
        const float topInnerY = height;

        auto place = [&](glm::vec3 local) {
            return project(anchor.origin + toWorld(anchor.basis, rotateLocal(local, yawDeg, pitchDeg, rollDeg)));
        };

        std::array<TazikPoint, SEGMENTS> rimOuter{}, rimInner{}, topOuter{}, topInner{};
        for (int i = 0; i < SEGMENTS; i++) {
            const float angle = static_cast<float>((PI * 2.0 * i) / SEGMENTS);
            const float sx = std::sin(angle);
            const float sz = std::cos(angle);
            const float rimWave = 1.0f + 0.010f * std::sin(angle * 4.0f + seed * 0.013f);
            const float topWave = 1.0f + 0.006f * std::cos(angle * 3.0f + seed * 0.011f);
            const float outerRim = rimRadius * rimWave;
            const float innerRim = std::max(0.02f, outerRim - rimWidth);
            const float outerTop = topRadius * topWave;
            const float innerTop = std::max(0.02f, outerTop - rimWidth * 0.35f);

            rimOuter[i] = place(glm::vec3(sx * outerRim, rimOuterY, sz * outerRim));
            rimInner[i] = place(glm::vec3(sx * innerRim, rimInnerY, sz * innerRim));
            topOuter[i] = place(glm::vec3(sx * outerTop, topOuterY, sz * outerTop));
            topInner[i] = place(glm::vec3(sx * innerTop, topInnerY, sz * innerTop));
        }
        const TazikPoint topCenter = place(glm::vec3(0.f, topInnerY + 0.002f * size, 0.f));

        for (int i = 0; i < SEGMENTS; i++) {
            const int next = (i + 1) % SEGMENTS;

            const ImColor wall = scaleColor(colorFor(mode, speed, i, primary, secondary), 0.92f, alphaMul);
            const ImColor inner = scaleColor(colorFor(mode, speed, i, primary, secondary), 0.58f, alphaMul * 0.86f);
            const ImColor brim = scaleColor(colorFor(mode, speed, i + 120, rim, secondary), 1.0f, alphaMul);
            const ImColor cap = scaleColor(colorFor(mode, speed, i, primary, secondary), 1.10f, alphaMul * 0.95f);

            const TazikPoint* outerA = &rimOuter[i];
            const TazikPoint* outerB = &rimOuter[next];
            if (outerA->ok && outerB->ok && topOuter[next].ok && topOuter[i].ok) {
                drawList->AddQuadFilled(outerA->screen, outerB->screen, topOuter[next].screen, topOuter[i].screen, wall);
            }
            if (topInner[i].ok && topInner[next].ok && rimInner[next].ok && rimInner[i].ok) {
                drawList->AddQuadFilled(topInner[i].screen, topInner[next].screen, rimInner[next].screen, rimInner[i].screen, inner);
            }
            if (rimInner[i].ok && rimInner[next].ok && rimOuter[next].ok && rimOuter[i].ok) {
                drawList->AddQuadFilled(rimInner[i].screen, rimInner[next].screen, rimOuter[next].screen, rimOuter[i].screen, brim);
            }
            if (topCenter.ok && topInner[i].ok && topInner[next].ok) {
                drawList->AddTriangleFilled(topCenter.screen, topInner[i].screen, topInner[next].screen, cap);
            }
            if (outerA->ok && outerB->ok) {
                drawList->AddLine(outerA->screen, outerB->screen, brim, 1.0f);
            }
            if (topOuter[i].ok && topOuter[next].ok) {
                drawList->AddLine(topOuter[i].screen, topOuter[next].screen, wall, 1.0f);
            }
        }
    }
}

using namespace TazikHatImpl;

// =========================================================
// Mode helpers
// =========================================================

static int colorModeIndex(TazikHat::ColorMode mode) {
    switch (mode) {
        case TazikHat::ColorMode::Rainbow: return 1;
        case TazikHat::ColorMode::DoubleColor: return 2;
        case TazikHat::ColorMode::Theme: return 3;
        case TazikHat::ColorMode::Static: break;
    }
    return 0;
}

// =========================================================
// Enable / disable
// =========================================================

void TazikHat::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &TazikHat::onRenderEvent, nes::event_priority::VERY_FIRST>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &TazikHat::onBaseTickEvent>(this);
}

void TazikHat::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &TazikHat::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &TazikHat::onBaseTickEvent>(this);
    mHandledDeaths.clear();
    mFlying.clear();
}

// =========================================================
// Death detection + flying hat physics (per tick)
// =========================================================

void TazikHat::onBaseTickEvent(BaseTickEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    if (!ci->getLevelRenderer() || !ci->getLocalPlayer()) return;

    const uint64_t now = NOW;
    for (Actor* actor : ActorUtils::getActorList(true, true)) {
        if (!actor) continue;
        const int64_t rid = actor->getRuntimeID();

        if (actor->getHealth() > 0.f) {
            mHandledDeaths.erase(rid);
            continue;
        }
        if (!mDeathFlight.mValue) continue;
        if (mHandledDeaths.contains(rid)) continue;

        mHandledDeaths[rid] = now;
        spawnFlying(actor);
    }

    std::erase_if(mHandledDeaths, [now](const auto& entry) {
        return now - entry.second > DEATH_HANDLED_RESET_MS;
    });

    for (auto it = mFlying.begin(); it != mFlying.end();) {
        if (updateFlying(it->second)) {
            it = mFlying.erase(it);
        } else {
            ++it;
        }
    }
}

void TazikHat::spawnFlying(Actor* actor)
{
    const bool helmet = hasHelmet(actor);
    const float size = mScale.mValue * (helmet ? mHelmetScale.mValue : 1.0f);
    const TazikAnchor anchor = headAnchor(actor, mHeadTopOffset.mValue, mHeadForwardOffset.mValue, mHeadRightOffset.mValue);
    const glm::vec3 base = anchor.origin;

    glm::vec3 away(0.f);
    Actor* local = ClientInstance::get()->getLocalPlayer();
    if (local) {
        away = base - *local->getPos();
    }
    if (glm::dot(away, away) < 1.0e-5f) {
        away = glm::vec3(randomRange(-1.f, 1.f), 0.f, randomRange(-1.f, 1.f));
    }
    away = safeNormalize(away, glm::vec3(1.f, 0.f, 0.f));

    const float power = mDeathFlightPower.mValue;
    FlyingTazik tazik{};
    tazik.scale = size;
    tazik.seed = std::rand();
    tazik.createdAtMs = NOW;
    tazik.pos = base;
    tazik.velocity = glm::vec3(
        away.x * randomRange(0.18f, 0.34f) * power + randomRange(-0.08f, 0.08f),
        randomRange(0.34f, 0.58f) * power,
        away.z * randomRange(0.18f, 0.34f) * power + randomRange(-0.08f, 0.08f));
    tazik.yaw = randomRange(0.f, 360.f);
    tazik.pitch = randomRange(-20.f, 20.f);
    tazik.roll = randomRange(-20.f, 20.f);
    tazik.yawVelocity = randomRange(-18.f, 18.f) * power;
    tazik.pitchVelocity = randomRange(-22.f, 22.f) * power;
    tazik.rollVelocity = randomRange(-24.f, 24.f) * power;

    mFlying[actor->getRuntimeID()] = tazik;
}

bool TazikHat::updateFlying(FlyingTazik& tazik)
{
    const uint64_t age = NOW - tazik.createdAtMs;
    if (age > DEATH_LIFE_MS) return true;

    tazik.velocity += glm::vec3(0.f, -0.034f, 0.f);
    tazik.velocity = glm::vec3(tazik.velocity.x * 0.985f, tazik.velocity.y * 0.965f, tazik.velocity.z * 0.985f);
    tazik.pos += tazik.velocity;

    ClientInstance* ci = ClientInstance::get();
    if (ci->getBlockSource()) {
        const glm::ivec3 ground = glm::ivec3(glm::floor(glm::vec3(tazik.pos.x, tazik.pos.y - 0.035f, tazik.pos.z)));
        if (!BlockUtils::isAirBlock(ground) && tazik.velocity.y < 0.f) {
            tazik.pos.y = ground.y + 1.035f;
            if (std::abs(tazik.velocity.y) > 0.045f) {
                tazik.velocity.x *= 0.72f;
                tazik.velocity.y = -tazik.velocity.y * 0.34f;
                tazik.velocity.z *= 0.72f;
                tazik.pitchVelocity *= 0.72f;
                tazik.rollVelocity *= 0.72f;
            } else {
                tazik.velocity.x *= 0.62f;
                tazik.velocity.y = 0.f;
                tazik.velocity.z *= 0.62f;
                tazik.pitchVelocity *= 0.58f;
                tazik.rollVelocity *= 0.58f;
            }
        }
    }

    tazik.yaw += tazik.yawVelocity;
    tazik.pitch += tazik.pitchVelocity;
    tazik.roll += tazik.rollVelocity;
    tazik.yawVelocity *= 0.985f;
    tazik.pitchVelocity *= 0.975f;
    tazik.rollVelocity *= 0.975f;
    return false;
}

// =========================================================
// Rendering
// =========================================================

void TazikHat::renderAttached(ImDrawList* drawList, Actor* actor)
{
    const bool helmet = hasHelmet(actor);
    const float size = mScale.mValue * (helmet ? mHelmetScale.mValue : 1.0f);
    const TazikAnchor anchor = headAnchor(actor, mHeadTopOffset.mValue, mHeadForwardOffset.mValue, mHeadRightOffset.mValue);

    // Idle sway only (hit wobble needs hurt-time data Juzdex does not expose)
    const int64_t rid = actor->getRuntimeID();
    const float time = static_cast<float>(NOW) / 50.f * 0.35f + static_cast<float>(rid) * 0.17f;
    const float idle = 1.3f * mWobbleStrength.mValue;
    const float pitch = std::sin(time * 1.2f) * idle;
    const float roll = std::cos(time * 0.9f) * idle;

    drawBowl(drawList, anchor, size, 0.f, pitch, roll, 1.0f, static_cast<int>(rid),
             colorModeIndex(mColorMode.mValue), mColorSpeed.mValue,
             mPrimaryColor.getAsImColor(), mSecondaryColor.getAsImColor(), mRimColor.getAsImColor());
}

void TazikHat::renderFlying(ImDrawList* drawList)
{
    const uint64_t now = NOW;
    for (auto& [rid, tazik] : mFlying) {
        const uint64_t age = now - tazik.createdAtMs;
        const float life = static_cast<float>(age) / static_cast<float>(DEATH_LIFE_MS);
        const float fade = 1.0f - smoothstep((std::max(0.f, life - 0.65f)) / 0.35f);
        drawBowl(drawList, worldAnchor(tazik.pos), tazik.scale, tazik.yaw, tazik.pitch, tazik.roll, fade, tazik.seed,
                 colorModeIndex(mColorMode.mValue), mColorSpeed.mValue,
                 mPrimaryColor.getAsImColor(), mSecondaryColor.getAsImColor(), mRimColor.getAsImColor());
    }
}

void TazikHat::onRenderEvent(RenderEvent& event)
{
    ClientInstance* ci = ClientInstance::get();
    if (!ci->getLevelRenderer()) return;

    Actor* local = ci->getLocalPlayer();
    if (!local) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const RenderTarget target = mRenderTarget.mValue;
    const bool showSelf = target == RenderTarget::Self || target == RenderTarget::Both;
    const bool showPlayers = target == RenderTarget::Players || target == RenderTarget::Both;

    // Own hat only in third person (same as Combatant)
    if (showSelf && ci->getOptions()->mThirdPerson->value != 0) {
        renderAttached(drawList, local);
    }

    if (showPlayers) {
        for (Actor* actor : ActorUtils::getActorList(true, true)) {
            if (!actor || actor == local) continue;
            if (actor->getHealth() <= 0.f) continue;
            renderAttached(drawList, actor);
        }
    }

    renderFlying(drawList);
}
