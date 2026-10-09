//
// HitParticles — rewritten from scratch.
//
// Hit detection mirrors Kagune / HitEffects (the two that actually work):
//   PacketOutEvent -> InventoryTransaction -> ItemUseOnEntityTransaction
//   -> ActionType::Attack -> mActorId
// The contact point is found with the same eye-ray vs hitbox test Kagune
// uses, so the burst lands exactly where the hit registered.
//
// Rendering is fully self-contained: world -> screen via RenderUtils and a
// foreground-ish background draw list, no Level::addParticle involved.
//

#include "HitParticles.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace {
    constexpr float kPi = 3.14159265358979f;

    std::mt19937& rng()
    {
        static thread_local std::mt19937 gen{ std::random_device{}() };
        return gen;
    }

    float frand(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng());
    }

    float smoothFade(float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    // Per-style feel: speed / gravity / drag / lifetime / size multipliers
    // applied on top of the user's sliders. This is what makes Blood feel
    // like blood and Sparks feel like sparks without 30 extra settings.
    struct Tuning { float speed, gravity, drag, life, size; };

    Tuning tuningFor(int style)
    {
        switch (style) {
        case 0:  return { 1.00f,  1.00f, 1.00f, 1.00f, 1.00f }; // Blood
        case 1:  return { 1.65f,  0.55f, 0.55f, 0.55f, 0.70f }; // Sparks
        case 2:  return { 1.40f,  0.70f, 0.75f, 0.65f, 0.85f }; // Critical
        case 3:  return { 0.55f, -0.45f, 1.30f, 1.45f, 1.20f }; // Hearts
        case 4:  return { 0.85f, -0.30f, 1.20f, 1.30f, 0.95f }; // Fire
        case 5:  return { 0.70f,  0.35f, 2.20f, 1.65f, 0.80f }; // Frost
        case 6:  return { 0.60f,  0.05f, 1.80f, 1.80f, 1.05f }; // Soul
        default: return { 0.50f, -0.20f, 2.40f, 1.70f, 1.10f }; // Toxic
        }
    }

    struct Palette { ImColor a, b, c; };

    Palette paletteFor(int style)
    {
        switch (style) {
        case 0:  return { ImColor(0.40f, 0.02f, 0.03f, 1.f), ImColor(0.78f, 0.05f, 0.06f, 1.f), ImColor(1.00f, 0.24f, 0.18f, 1.f) };
        case 1:  return { ImColor(1.00f, 0.96f, 0.60f, 1.f), ImColor(1.00f, 0.72f, 0.20f, 1.f), ImColor(0.85f, 0.35f, 0.05f, 1.f) };
        case 2:  return { ImColor(1.00f, 1.00f, 1.00f, 1.f), ImColor(0.65f, 0.85f, 1.00f, 1.f), ImColor(0.30f, 0.55f, 1.00f, 1.f) };
        case 3:  return { ImColor(1.00f, 0.35f, 0.55f, 1.f), ImColor(1.00f, 0.62f, 0.75f, 1.f), ImColor(0.90f, 0.10f, 0.35f, 1.f) };
        case 4:  return { ImColor(1.00f, 0.85f, 0.25f, 1.f), ImColor(1.00f, 0.45f, 0.08f, 1.f), ImColor(0.55f, 0.15f, 0.03f, 1.f) };
        case 5:  return { ImColor(0.70f, 0.95f, 1.00f, 1.f), ImColor(0.35f, 0.75f, 1.00f, 1.f), ImColor(0.88f, 0.94f, 1.00f, 1.f) };
        case 6:  return { ImColor(0.35f, 0.95f, 0.90f, 1.f), ImColor(0.75f, 0.85f, 1.00f, 1.f), ImColor(0.25f, 0.65f, 0.80f, 1.f) };
        default: return { ImColor(0.55f, 0.95f, 0.25f, 1.f), ImColor(0.85f, 1.00f, 0.35f, 1.f), ImColor(0.20f, 0.55f, 0.12f, 1.f) };
        }
    }
    // Shape per style, so each style reads differently on screen.
    // 0 Blood / 2 Critical / 1 Sparks: square droplet · 3 Hearts: heart
    // 4 Fire / 6 Soul / 7 Toxic: round · 5 Frost: diamond
    void drawDroplet(ImDrawList* dl, ImVec2 c, float h, ImU32 col, int style)
    {
        switch (style) {
        case 3: { // Hearts
            const float r = h * 0.85f;
            dl->AddCircleFilled({ c.x - r, c.y - r * 0.35f }, r, col, 12);
            dl->AddCircleFilled({ c.x + r, c.y - r * 0.35f }, r, col, 12);
            dl->AddTriangleFilled({ c.x - r * 1.95f, c.y - r * 0.35f },
                                  { c.x + r * 1.95f, c.y - r * 0.35f },
                                  { c.x, c.y + r * 2.0f }, col);
            return;
        }
        case 4: case 6: // Fire, Soul
            dl->AddCircleFilled(c, h, col, 10);
            return;
        case 5: // Frost
            dl->AddQuadFilled({ c.x, c.y - h * 1.3f }, { c.x + h * 1.3f, c.y },
                              { c.x, c.y + h * 1.3f }, { c.x - h * 1.3f, c.y }, col);
            return;
        case 7: { // Toxic bubble
            dl->AddCircleFilled(c, h, col, 10);
            dl->AddCircle(c, h, ImColor(255, 255, 255, 60), 10, 1.f);
            return;
        }
        default: // Blood, Sparks, Critical
            dl->AddRectFilled({ c.x - h, c.y - h }, { c.x + h, c.y + h }, col, std::max(0.f, h * 0.35f));
            return;
        }
    }
}

void HitParticles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &HitParticles::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &HitParticles::onPacketOutEvent>(this);

    mParticles.clear();
    mPending.clear();
    mFlashes.clear();
}

void HitParticles::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &HitParticles::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &HitParticles::onPacketOutEvent>(this);

    mParticles.clear();
    mPending.clear();
    mFlashes.clear();
}

// ─── Hit detection ────────────────────────────────────────────────────────────

void HitParticles::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;

    auto pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt || !pkt->mTransaction) return;

    auto* cit = pkt->mTransaction.get();
    if (!cit || cit->type != ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) return;

    auto* iut = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(cit);
    if (!iut) return;
    if (iut->mActionType != ItemUseOnActorInventoryTransaction::ActionType::Attack) return;

    auto* ci = ClientInstance::get();
    if (!ci) return;
    auto* player = ci->getLocalPlayer();
    if (!player) return;

    // Resolve the victim. `mActorId` carries the actor's UNIQUE id — that's what
    // ActorUtils::createAttackTransaction writes into it and what the server
    // accepts — so look that up first. The runtime-id scan stays as a fallback
    // for builds that encode the field differently.
    Actor* target = ActorUtils::getActorFromUniqueId(static_cast<int64_t>(iut->mActorId));
    if (!target) {
        for (auto* a : ActorUtils::getActorList(false, true)) {
            if (!a) continue;
            if (a->getRuntimeID() == static_cast<int64_t>(iut->mActorId)) { target = a; break; }
        }
    }
    if (!target || target == player) return;

    // ── Eye ray ─────────────────────────────────────────────────────────────
    auto* rot  = player->getActorRotationComponent();
    if (!rot) return;
    auto* head = player->getActorHeadRotationComponent();

    float yaw   = (head ? head->mHeadRot : rot->mYaw) * (kPi / 180.f);
    float pitch = rot->mPitch * (kPi / 180.f);

    // Positions in this SDK already sit at EYE level (feet + PLAYER_HEIGHT) —
    // every renderer compensates by dropping PLAYER_HEIGHT before drawing
    // (see Actor::getAABB / Nametags / HitEffects). Adding it here, as the
    // first version did, pushed the whole burst ~1.6 blocks above the head.
    glm::vec3 eye = *player->getPos();

    glm::vec3 rayDir(
        -sinf(yaw) * cosf(pitch),
        -sinf(pitch),
         cosf(yaw) * cosf(pitch));

    // ── Target hitbox: the game's own AABB (exactly the box ESP draws) ──────
    auto* shape = target->getAABBShapeComponent();
    const float boxH = shape ? shape->mHeight : 1.8f;
    const float boxW = shape ? shape->mWidth  : 0.6f;

    AABB body = target->getAABB();
    const bool boxOk =
        std::isfinite(body.mMin.x) && std::isfinite(body.mMin.y) && std::isfinite(body.mMin.z) &&
        std::isfinite(body.mMax.x) && std::isfinite(body.mMax.y) && std::isfinite(body.mMax.z) &&
        (body.mMax.y - body.mMin.y) > 0.2f && (body.mMax.x - body.mMin.x) > 0.01f;
    if (!boxOk) {
        // Missing components — rebuild it the same way the client does:
        // the stored position is eye-level, so drop to the feet first.
        const glm::vec3 feet = *target->getPos() - glm::vec3(0.f, PLAYER_HEIGHT, 0.f);
        body.mMin = feet - glm::vec3(boxW * 0.5f, 0.f, boxW * 0.5f);
        body.mMax = feet + glm::vec3(boxW * 0.5f, boxH, boxW * 0.5f);
    }

    // ── Ray vs target AABB (slab test) ──────────────────────────────────────
    float tMin = 0.05f, tMax = 8.f;
    bool hitBox = true;
    for (int ax = 0; ax < 3; ++ax) {
        float d  = (&rayDir.x)[ax];
        float mn = (&body.mMin.x)[ax];
        float mx = (&body.mMax.x)[ax];
        float e  = (&eye.x)[ax];
        if (fabsf(d) < 1e-6f) {
            if (e < mn || e > mx) { hitBox = false; break; }
        } else {
            float t1 = (mn - e) / d;
            float t2 = (mx - e) / d;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) { hitBox = false; break; }
        }
    }

    glm::vec3 contact;
    if (hitBox) {
        contact = eye + rayDir * tMin;
        contact.x = std::clamp(contact.x, body.mMin.x + 0.05f, body.mMax.x - 0.05f);
        contact.y = std::clamp(contact.y, body.mMin.y + 0.05f, body.mMax.y - 0.05f);
        contact.z = std::clamp(contact.z, body.mMin.z + 0.05f, body.mMax.z - 0.05f);
    } else {
        // No clean intersection (server disagreed, fast move, desync) —
        // still splash on the body instead of dropping the hit.
        contact = (body.mMin + body.mMax) * 0.5f;
    }

    spawnBurst(contact, rayDir, 1.0f, 1.0f, 1.0f);

    if (mSecondBurst.mValue)
        mPending.push_back({ static_cast<uint64_t>(NOW) + 120u, contact, rayDir });

    if (mFlash.mValue)
        mFlashes.push_back({ static_cast<uint64_t>(NOW), 280u, contact, pickColor(1), 0.5f });
}

// ─── Spawning ─────────────────────────────────────────────────────────────────

void HitParticles::spawnBurst(const glm::vec3& center, const glm::vec3& dir,
                              float amountMul, float speedMul, float spreadMul)
{
    const Tuning tn = tuningFor(mStyle.as<int>());

    // Amount is capped at 15 by the setting; the clamp also keeps a stale config
    // value (from the older 2..80 range) from spawning a 80-particle burst.
    const int   count  = std::clamp(static_cast<int>(std::round(mAmount.mValue * amountMul)), 1, 15);
    const float spread = mSpread.mValue * spreadMul;
    const float speed  = mSpeed.mValue * speedMul * tn.speed;
    const float life   = mLifetime.mValue * tn.life;
    const float size   = mSize.mValue * tn.size;

    glm::vec3 bias = dir;
    if (glm::length(bias) < 1e-5f) bias = glm::vec3(0.f, 1.f, 0.f);
    else bias = glm::normalize(bias);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = center + glm::vec3(frand(-spread, spread), frand(-spread, spread), frand(-spread, spread));

        glm::vec3 spray(frand(-1.f, 1.f), frand(-1.f, 1.f), frand(-1.f, 1.f));
        if (glm::length(spray) < 1e-4f) spray = glm::vec3(0.f, 1.f, 0.f);
        spray = glm::normalize(spray);

        // Directional: the dome of droplets leans away from the eyes, which
        // is what makes Kagune's blood look like it really splashed off a hit.
        glm::vec3 vel = mDirectional.mValue ? (spray * 0.85f + bias * 1.15f) : spray;
        if (glm::length(vel) < 1e-5f) vel = spray;
        vel = glm::normalize(vel) * speed * frand(0.55f, 1.25f);
        // Slight upward kick so the spray arcs like real blood — kept small so
        // the cloud stays on the impact instead of drifting over the head.
        vel.y += frand(-0.05f, 0.95f);
        p.vel = vel;

        p.maxLife = p.life = std::max(0.05f, life * frand(0.65f, 1.35f));
        p.size = size * frand(0.60f, 1.40f);
        p.col = pickColor(i);

        mParticles.push_back(p);
    }

    if (mParticles.size() > 1400)
        mParticles.erase(mParticles.begin(), mParticles.begin() + static_cast<int>(mParticles.size() - 1400));
}

ImColor HitParticles::pickColor(int index)
{
    if (mColorMode.mValue == ColorMode::Custom) {
        return ImColor(mCustomColor.mValue[0], mCustomColor.mValue[1],
                       mCustomColor.mValue[2], mCustomColor.mValue[3]);
    }

    if (mColorMode.mValue == ColorMode::ThemeColor) {
        ImColor c = ColorUtils::getThemedColor(static_cast<float>((index * 23) % 360));
        c.Value.w = 1.f;
        return c;
    }

    const Palette pal = paletteFor(mStyle.as<int>());
    const int which = index % 3;
    ImColor c = (which == 0) ? pal.a : (which == 1 ? pal.b : pal.c);

    // Per-particle brightness jitter so the spray doesn't look flat.
    const float v = frand(-0.12f, 0.16f);
    c.Value.x = std::clamp(c.Value.x + v, 0.f, 1.f);
    c.Value.y = std::clamp(c.Value.y + v * 0.6f, 0.f, 1.f);
    c.Value.z = std::clamp(c.Value.z + v * 0.6f, 0.f, 1.f);
    return c;
}

// ─── Update + draw ────────────────────────────────────────────────────────────

void HitParticles::updateParticles(float dt)
{
    // Delayed follow-up splash
    for (auto it = mPending.begin(); it != mPending.end(); ) {
        if (NOW >= it->at) {
            spawnBurst(it->pos, it->dir, 0.45f, 0.55f, 2.1f);
            it = mPending.erase(it);
        } else {
            ++it;
        }
    }

    const Tuning tn = tuningFor(mStyle.as<int>());
    const float grav = 9.81f * mGravity.mValue * tn.gravity;
    const float drag = std::max(0.f, mDrag.mValue * tn.drag);

    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.f) { it = mParticles.erase(it); continue; }

        it->vel.y -= grav * dt;
        it->vel *= std::max(0.f, 1.f - drag * dt);
        it->pos += it->vel * dt;
        ++it;
    }

    for (auto it = mFlashes.begin(); it != mFlashes.end(); ) {
        const float t = static_cast<float>(NOW - it->start) / static_cast<float>(std::max<uint64_t>(it->dur, 1));
        if (t >= 1.f) it = mFlashes.erase(it);
        else ++it;
    }
}

void HitParticles::drawParticles()
{
    auto* dl = ImGui::GetBackgroundDrawList();
    const glm::vec3 cam = RenderUtils::transform.mOrigin;

    // Painter's order: far droplets first, so near ones sit on top.
    std::sort(mParticles.begin(), mParticles.end(),
        [&cam](const Particle& a, const Particle& b) {
            return glm::dot(a.pos - cam, a.pos - cam) > glm::dot(b.pos - cam, b.pos - cam);
        });

    const int style = mStyle.as<int>();

    for (const auto& p : mParticles) {
        ImVec2 sc;
        if (!RenderUtils::worldToScreen(p.pos, sc)) continue;

        const float lifeT = std::clamp(p.life / p.maxLife, 0.f, 1.f);
        const float fade  = smoothFade(lifeT);

        const float dist  = glm::length(p.pos - cam);
        const float persp = std::clamp(7.5f / std::max(dist, 0.65f), 0.35f, 2.4f);
        const float sz    = std::max(1.f, p.size * persp * (0.35f + 0.65f * lifeT));
        const float half  = sz * 0.5f;

        ImColor body(p.col.Value.x, p.col.Value.y, p.col.Value.z, fade);

        if (mTrails.mValue && sz > 1.6f) {
            glm::vec3 tail = p.pos - p.vel * 0.035f;
            ImVec2 tailSc;
            if (RenderUtils::worldToScreen(tail, tailSc)) {
                ImColor tc(p.col.Value.x, p.col.Value.y, p.col.Value.z, fade * 0.45f);
                dl->AddLine(tailSc, sc, tc, std::max(1.f, sz * 0.45f));
            }
        }

        if (mGlow.mValue) {
            ImColor halo(p.col.Value.x, p.col.Value.y, p.col.Value.z, fade * 0.16f);
            dl->AddCircleFilled(sc, sz * 2.1f, halo, 10);
        }

        drawDroplet(dl, sc, half, body, style);
    }

    for (const auto& f : mFlashes) {
        const float t = static_cast<float>(NOW - f.start) / static_cast<float>(std::max<uint64_t>(f.dur, 1));
        if (t >= 1.f) continue;

        const float ease   = 1.f - std::pow(1.f - t, 3.f);
        const float worldR = f.radius * ease;

        ImVec2 c0, c1;
        if (!RenderUtils::worldToScreen(f.pos, c0)) continue;
        if (!RenderUtils::worldToScreen(f.pos + glm::vec3(0.f, worldR, 0.f), c1)) continue;

        const float px = std::max(2.f, std::fabs(c1.y - c0.y));
        const float a  = (1.f - t) * (1.f - t);

        ImColor ring(f.col.Value.x, f.col.Value.y, f.col.Value.z, a * 0.55f);
        ImColor fill(f.col.Value.x, f.col.Value.y, f.col.Value.z, a * 0.10f);

        dl->AddCircleFilled(c0, px * 0.75f, fill, 22);
        dl->AddCircle(c0, px, ring, 28, std::max(1.f, px * 0.15f));
    }
}

void HitParticles::onRenderEvent(RenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    if (!ci->getLocalPlayer()) return;

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.05f);

    updateParticles(dt);

    if (mParticles.empty() && mFlashes.empty()) return;

    drawParticles();
}
