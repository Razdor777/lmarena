#include "Kagune.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

#include <vector>
#include <algorithm>
#include <random>
#include <cmath>

static constexpr float KPI  = 3.14159265358979f;
static constexpr float K2PI = 6.28318530717959f;

static inline glm::vec3 kgVSafe(const glm::vec3& v, const glm::vec3& fb = {0,1,0})
{
    float l = glm::length(v);
    return l > 1e-7f ? v / l : fb;
}
static inline float kgSS(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
static inline float kgSS3(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}
static inline glm::vec3 kgMix(const glm::vec3& a, const glm::vec3& b, float t)
{
    return a + (b - a) * t;
}
static inline glm::vec3 kgCR(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3, float t)
{
    float t2 = t*t, t3 = t2*t;
    return 0.5f * (2.f*p1 + (-p0+p2)*t
        + (2.f*p0 - 5.f*p1 + 4.f*p2 - p3)*t2
        + (-p0 + 3.f*p1 - 3.f*p2 + p3)*t3);
}

struct KTri { glm::vec3 p[3]; ImColor col; float depth; };
static const glm::vec3 kKL = kgVSafe({0.4f, -1.f, 0.3f});

static ImColor kgLight(ImColor base, const glm::vec3& fn)
{
    float f = std::min(0.28f + std::max(glm::dot(fn, -kKL), 0.f) * 0.72f
                      + std::max(fn.y, 0.f) * 0.15f, 1.f);
    return ImColor(
        std::min(base.Value.x * f, 1.f),
        std::min(base.Value.y * f, 1.f),
        std::min(base.Value.z * f, 1.f),
        base.Value.w);
}

static void kgTri(std::vector<KTri>& out,
    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
    ImColor base, const glm::vec3& cam, bool ds = false)
{
    glm::vec3 fn = glm::cross(b - a, c - a);
    float fl = glm::length(fn);
    if (fl < 1e-10f) return;
    fn /= fl;
    glm::vec3 ctr = (a + b + c) * (1.f / 3.f);
    float dep = glm::dot(ctr - cam, ctr - cam);
    KTri t;
    if (glm::dot(fn, cam - ctr) > 0.f) {
        t.p[0] = a; t.p[1] = b; t.p[2] = c;
        t.col = kgLight(base, fn); t.depth = dep;
        out.push_back(t);
    } else if (ds) {
        ImColor dk(base.Value.x*.5f, base.Value.y*.5f, base.Value.z*.5f, base.Value.w);
        t.p[0] = a; t.p[1] = c; t.p[2] = b;
        t.col = kgLight(dk, -fn); t.depth = dep;
        out.push_back(t);
    }
}

struct KRing { glm::vec3 center, n, b; std::vector<glm::vec3> v; };

static void kgRings(const std::vector<glm::vec3>& sp,
    float R, int res, float iT, std::vector<KRing>& out)
{
    int N = (int)sp.size();
    if (N < 2) return;
    out.resize(N);
    glm::vec3 t0  = kgVSafe(sp[1] - sp[0]);
    glm::vec3 ref = (fabsf(t0.x) < 0.9f) ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
    glm::vec3 n0  = kgVSafe(glm::cross(t0, ref));
    glm::vec3 b0  = kgVSafe(glm::cross(t0, n0));
    for (int i = 0; i < N; i++) {
        glm::vec3 tan;
        if (i == 0)        tan = kgVSafe(sp[1] - sp[0]);
        else if (i == N-1) tan = kgVSafe(sp[N-1] - sp[N-2]);
        else               tan = kgVSafe(sp[i+1] - sp[i-1]);
        if (i > 0) {
            glm::vec3 pr = (i == 1) ? kgVSafe(sp[1]-sp[0]) : kgVSafe(sp[i]-sp[i-1]);
            glm::vec3 ax = glm::cross(pr, tan);
            float sA = glm::length(ax), cA = glm::dot(pr, tan);
            if (sA > 1e-6f) {
                ax /= sA;
                auto rod = [&](const glm::vec3& v) {
                    return v*cA + glm::cross(ax,v)*sA + ax*glm::dot(ax,v)*(1.f-cA);
                };
                n0 = kgVSafe(rod(n0));
                b0 = kgVSafe(rod(b0));
            }
        }
        float spT = float(i) / float(N-1);
        float r   = std::max(R*(1.f-spT*0.95f)*(1.f+sinf(iT*7.f-spT*12.f)*0.08f), R*0.02f);
        out[i].center = sp[i]; out[i].n = n0; out[i].b = b0;
        out[i].v.resize(res);
        for (int k = 0; k < res; k++) {
            float a = K2PI * float(k) / float(res);
            out[i].v[k] = sp[i] + n0*(cosf(a)*r) + b0*(sinf(a)*r);
        }
    }
}

static void kgStrip(std::vector<KTri>& out,
    const KRing& r0, const KRing& r1, int res,
    float t, const glm::vec3& cam, ImColor cA, ImColor cB)
{
    ImColor c(
        cA.Value.x + (cB.Value.x-cA.Value.x)*t,
        cA.Value.y + (cB.Value.y-cA.Value.y)*t,
        cA.Value.z + (cB.Value.z-cA.Value.z)*t, 1.f);
    for (int k = 0; k < res; k++) {
        int nk = (k+1)%res;
        kgTri(out, r0.v[k], r1.v[k],  r0.v[nk], c, cam);
        kgTri(out, r0.v[nk],r1.v[k],  r1.v[nk], c, cam);
    }
}

static void kgCrystalSpike(std::vector<KTri>& out,
    const glm::vec3& root, const glm::vec3& dir, const glm::vec3& nrm,
    float len, float width, const glm::vec3& cam, ImColor col)
{
    glm::vec3 tip  = root + dir * len;
    glm::vec3 side = kgVSafe(glm::cross(dir, nrm)) * width;
    glm::vec3 perp = kgVSafe(glm::cross(dir, side)) * width * 0.35f;
    glm::vec3 b0=root+side, b1=root+perp, b2=root-side, b3=root-perp;
    glm::vec3 mid = root + dir*(len*0.35f);
    ImColor bright(std::min(col.Value.x*1.3f,1.f),std::min(col.Value.y*1.3f,1.f),std::min(col.Value.z*1.3f,1.f),1.f);
    ImColor dark  (col.Value.x*.35f,col.Value.y*.35f,col.Value.z*.35f,1.f);
    ImColor mid2  (col.Value.x*.7f, col.Value.y*.7f, col.Value.z*.7f, 1.f);
    kgTri(out,b0,mid,b1,bright,cam,true); kgTri(out,b1,mid,b2,mid2,cam,true);
    kgTri(out,b2,mid,b3,dark,  cam,true); kgTri(out,b3,mid,b0,mid2,cam,true);
    kgTri(out,b0,tip,b1,bright,cam,true); kgTri(out,b1,tip,b2,mid2,cam,true);
    kgTri(out,b2,tip,b3,dark,  cam,true); kgTri(out,b3,tip,b0,mid2,cam,true);
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────
void Kagune::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent,    &Kagune::onRenderEvent   >(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);

    mEnableTime      = NOW;
    mSpawnState      = SpawnState::Spawning;
    mSpawnStart      = NOW;
    mLastHit         = NOW;
    mPendingHit      = false;
    mParticles.clear();
    for (auto& t : mTentacles) t = TentacleState{};
    mStriker         = 0;
    mLastStriker     = -1;
    mHits            = 0;
    mJumpVel         = 0.f;
    mJumpPush        = 0.f;
    mWasOnGround     = true;
    mDespawnProgress = 0.f;
}

void Kagune::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &Kagune::onRenderEvent   >(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);
    mParticles.clear();
}

// ─── Packet ───────────────────────────────────────────────────────────────────
void Kagune::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;
    auto pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt || !pkt->mTransaction) return;
    auto* cit = pkt->mTransaction.get();
    if (cit->type != ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) return;
    auto* iut = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(cit);
    if (iut->mActionType != ItemUseOnActorInventoryTransaction::ActionType::Attack) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    int cnt = std::clamp((int)mCount.mValue, 1, kMax);

    glm::vec3 tPos{};
    float     targetDist = mLength.mValue;

    auto* rot   = player->getActorRotationComponent();
    auto* headR = player->getActorHeadRotationComponent();

    float headYaw = (headR ? headR->mHeadRot : rot->mYaw) * (KPI / 180.f);
    float pitch   = rot->mPitch * (KPI / 180.f);

    glm::vec3 rayDir(
        -sinf(headYaw) * cosf(pitch),
        -sinf(pitch),
         cosf(headYaw) * cosf(pitch));

    glm::vec3 eyePos = *player->getPos();
    eyePos.y += PLAYER_HEIGHT;

    bool  found      = false;
    float bestDist   = FLT_MAX;

    for (auto* a : ActorUtils::getActorList(false, true)) {
        if (!a || a->getRuntimeID() != iut->mActorId) continue;

        auto* sh2 = a->getAABBShapeComponent();
        float h2  = sh2 ? sh2->mHeight : 1.8f;
        float w2  = sh2 ? sh2->mWidth  : 0.6f;
        glm::vec3 aPos = *a->getPos();

        // AABB цели
        glm::vec3 aabbMin(aPos.x - w2*0.5f, aPos.y,      aPos.z - w2*0.5f);
        glm::vec3 aabbMax(aPos.x + w2*0.5f, aPos.y + h2, aPos.z + w2*0.5f);

        // Ray-AABB slab intersection
        float tMin = 0.001f, tMax = mLength.mValue * 3.f;
        bool  hit  = true;
        for (int ax = 0; ax < 3; ax++) {
            float d  = (&rayDir.x)[ax];
            float mn = (&aabbMin.x)[ax];
            float mx = (&aabbMax.x)[ax];
            float ey = (&eyePos.x)[ax];
            if (fabsf(d) < 1e-6f) {
                if (ey < mn || ey > mx) { hit = false; break; }
            } else {
                float t1 = (mn - ey) / d;
                float t2 = (mx - ey) / d;
                if (t1 > t2) std::swap(t1, t2);
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) { hit = false; break; }
            }
        }

        if (hit && tMin < bestDist) {
            bestDist = tMin;
            tPos = eyePos + rayDir * tMin;
            // Клампим строго внутрь хитбокса
            tPos.y = std::clamp(tPos.y, aPos.y + 0.05f, aPos.y + h2 - 0.05f);
            targetDist = bestDist;
            found = true;
        }
        break;
    }

    if (!found) {
        tPos = eyePos + rayDir * mLength.mValue;
        targetDist = mLength.mValue;
    }

    // Блокируем при spawn/despawn
    if (mSpawnState == SpawnState::Spawning) {
        mPendingHit = true; mPendingTarget = tPos; return;
    }
    if (mSpawnState == SpawnState::Despawning) {
        mPendingHit  = true; mPendingTarget = tPos;
        mSpawnState  = SpawnState::Spawning;
        mSpawnStart  = NOW - (uint64_t)(mDespawnProgress * 2000.f);
        return;
    }
    if (mSpawnState == SpawnState::Hidden) {
        mSpawnState = SpawnState::Spawning; mSpawnStart = NOW;
        mPendingHit = true; mPendingTarget = tPos;
        return;
    }

    mLastHit = NOW;
    float maxReach    = 3.0f;
    float reachFactor = std::clamp(targetDist / maxReach, 0.15f, 1.0f);

    auto& st = mTentacles[mStriker];
    uint32_t seed = (st.hitCount * 2654435761u) ^ (uint32_t(mStriker) * 2246822519u)
                   ^ (uint32_t(NOW / 100));

    auto mode   = mAttackMode.mValue;
    bool isSlap = (mode == AttackMode::Sting) ? false
                : (mode == AttackMode::Slap)  ? true
                : ((mHits & 1u) != 0u);

    HitRecord rec;
    rec.hitTime = NOW;
    rec.target  = tPos;
    rec.animId  = uint8_t(seed & 0x7u);
    rec.isSlap  = isSlap;
    rec.stretch = (0.7f + reachFactor*0.3f) * (0.85f + float(seed>>8 & 0xFF)/255.f*0.15f);

    st.hitCount++;
    st.hardenTime = NOW;
    st.hitQueue.push_back(rec);
    if (st.hitQueue.size() > 4)
        st.hitQueue.erase(st.hitQueue.begin());

    mHits++;
    mLastStriker = mStriker;
    mStriker = (mStriker + 1) % cnt;
}

// ─── Render ───────────────────────────────────────────────────────────────────
void Kagune::onRenderEvent(RenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    auto* opts = ci->getOptions();
    if (!opts) return;

    int  camMode = opts->mThirdPerson->value;
    bool isFPV   = (camMode == 0);
    bool isBack  = (camMode == 1);
    bool isFront = (camMode == 2);
    if (!mShowFPV.mValue && isFPV) return;

    auto* player = ci->getLocalPlayer();
    if (!player) return;

    float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.05f);
    float iT = float(ImGui::GetTime());

    auto* sh    = player->getAABBShapeComponent();
    auto* rot   = player->getActorRotationComponent();
    auto* headR = player->getActorHeadRotationComponent();
    auto* bodyR = player->getMobBodyRotationComponent();
    auto* svc   = player->getStateVectorComponent();
    if (!rot) return;

    // ── Позиция тела (визуальная, сглаженная) ─────────────────────────────
    // RenderPositionComponent = eye level. Вычитаем PLAYER_HEIGHT → feet level.
    // Эта точка синхронна с визуальной моделью игрока (прыжок, бег, bobbing).
    glm::vec3 pPos = *player->getPos();
    {
        auto* rpc = player->getRenderPositionComponent();
        if (rpc) {
            pPos = rpc->mPosition;
            pPos.y -= PLAYER_HEIGHT; // eye → feet
        }
    }

    glm::vec3 vel    = svc ? svc->mVelocity : glm::vec3(0.f);
    float hSpd   = sqrtf(vel.x*vel.x + vel.z*vel.z);
    bool  moving = hSpd > 0.003f;

    // ── Углы: тело, не голова/камера ──────────────────────────────────────
    // fwdBody ориентирует tentacle по направлению тела (MobBodyRotationComponent).
    // fwdAim нужен только для микро-смещения root в 1-м лице.
    float bodyYawDeg = bodyR ? bodyR->yBodyRot : (headR ? headR->mHeadRot : rot->mYaw);
    float headYawDeg = headR ? headR->mHeadRot : rot->mYaw;
    float pitchDeg   = rot->mPitch;

    float bodyYaw = glm::radians(bodyYawDeg);
    float headYaw = glm::radians(headYawDeg);
    float pitch   = glm::radians(pitchDeg);

    glm::vec3 fwdBody = kgVSafe({-sinf(bodyYaw), 0.f, cosf(bodyYaw)});
    glm::vec3 fwdAim  = kgVSafe({
        -sinf(headYaw) * cosf(pitch),
        -sinf(pitch),
         cosf(headYaw) * cosf(pitch)
    });
    glm::vec3 worldUp(0, 1, 0);
    glm::vec3 right = kgVSafe(glm::cross(fwdBody, worldUp));

    float pH = sh ? sh->mHeight : 1.8f;

    glm::vec3 root = pPos;
    root.y += pH * kHeightFac;
    root   += worldUp * kOffsetY;
    root   -= fwdBody * kOffsetZ;
    if (isFPV) { root -= fwdAim * 0.4f; root -= worldUp * 0.08f; }

    glm::vec3 camPos = RenderUtils::transform.mOrigin;
    auto&     mat    = RenderUtils::transform.mMatrix;
    auto*     dl     = ImGui::GetBackgroundDrawList();
    auto      guiRes = ci->getGuiData()->mResolution;

    int   cnt     = std::clamp((int)mCount.mValue,    1, kMax);
    float maxLen  = mLength.mValue;
    float baseRad = mThickness.mValue * 0.022f;
    int   segs    = std::clamp((int)mSegments.mValue, 4, 20);
    int   rRes    = std::clamp((int)mRingRes.mValue,  3, 10);
    float animDur = mAnimTime.mValue;

    ImColor colOuter = mColOuter.getAsImColor();
    ImColor colCore  = mColCore .getAsImColor();
    ImColor colSpike = mColSpike.getAsImColor();

    // ── Spawn/despawn ──────────────────────────────────────────────────────
    float globalScale = 1.f;
    glm::vec3 spawnOrigin = root - fwdBody * 0.05f;

    if (mSpawnState == SpawnState::Spawning) {
        float elapsed = float(NOW - mSpawnStart) / 1000.f;
        if (elapsed >= 2.0f) {
            mSpawnState = SpawnState::Visible;
            globalScale = 1.f;
            if (mPendingHit) {
                mPendingHit = false;
                mLastHit    = NOW;
                float targetDist  = glm::length(mPendingTarget - pPos);
                float reachFactor = std::clamp(targetDist / 3.f, 0.15f, 1.f);
                auto& st = mTentacles[mStriker];
                uint32_t seed = (st.hitCount * 2654435761u) ^ (uint32_t(mStriker) * 2246822519u);
                HitRecord rec;
                rec.hitTime = NOW;
                rec.target  = mPendingTarget;
                rec.animId  = uint8_t(seed & 0x7u);
                rec.isSlap  = (mHits & 1u) != 0u;
                rec.stretch = 0.7f + reachFactor * 0.3f;
                st.hitCount++;
                st.hitQueue.push_back(rec);
                mHits++;
                mLastStriker = mStriker;
                mStriker = (mStriker + 1) % cnt;
            }
        } else {
            globalScale = elapsed / 2.0f;
        }
    } else if (mSpawnState == SpawnState::Visible) {
        globalScale = 1.f;
        float timeSinceHit = float(NOW - mLastHit) / 1000.f;
        if (timeSinceHit > mHideDelay.mValue) {
            mSpawnState      = SpawnState::Despawning;
            mSpawnStart      = NOW;
            mDespawnProgress = 0.f;
        }
    } else if (mSpawnState == SpawnState::Despawning) {
        float elapsed = float(NOW - mSpawnStart) / 1000.f;
        mDespawnProgress = std::clamp(elapsed / 3.f, 0.f, 1.f);
        if (elapsed >= 3.f) {
            mSpawnState = SpawnState::Hidden;
            globalScale = 0.f;
        } else {
            globalScale = 1.f - mDespawnProgress;
        }
    } else {
        return;
    }

    if (globalScale < 0.01f) return;

    float scaledLen = maxLen * globalScale;
    float scaledRad = baseRad * globalScale;

    glm::vec3 p2cam = camPos - pPos;
    float camDot = glm::dot(kgVSafe(p2cam), fwdBody);
    bool  hide   = isFront || (camDot > 0.5f && !isFPV && !isBack);

    std::mt19937 rng((uint32_t)NOW / 16u);
    std::uniform_real_distribution<float> dr(-1.f, 1.f);
    std::uniform_real_distribution<float> dp( 0.f, 1.f);

    std::vector<KTri> tris;
    tris.reserve(cnt * segs * rRes * 4);

    // ── Главный цикл щупалец ──────────────────────────────────────────────
    for (int ti = 0; ti < cnt; ti++) {
        auto& st = mTentacles[ti];

        // ── Очередь ударов ─────────────────────────────────────────────────
        float sp   = 0.f;
        bool  atk  = false;

        if (st.hasActiveHit) {
            float e2 = float(NOW - st.currentHit.hitTime) * 0.001f;
            if (e2 >= animDur) st.hasActiveHit = false;
        }
        if (!st.hasActiveHit && !st.hitQueue.empty()) {
            st.currentHit   = st.hitQueue.front();
            st.hitQueue.erase(st.hitQueue.begin());
            st.hasActiveHit = true;
            float e2 = float(NOW - st.currentHit.hitTime) * 0.001f;
            if (e2 > animDur * 1.5f) st.hasActiveHit = false; // протух
        }
        if (st.hasActiveHit) {
            float e2 = float(NOW - st.currentHit.hitTime) * 0.001f;
            if (e2 < animDur) {
                atk = true;
                float np = e2 / animDur;
                if      (np < 0.18f) sp = sinf((np / 0.18f) * 1.5707f);
                else if (np < 0.58f) sp = 1.f;
                else                 sp = cosf(((np - 0.58f) / 0.42f) * 1.5707f);
            }
        }

        // Spawn T
        float tentSpawnT = 1.f;
        if (mSpawnState == SpawnState::Spawning || globalScale < 1.f) {
            float elapsed = float(NOW - mSpawnStart) / 1000.f;
            float delay   = float(ti) * 0.4f;
            tentSpawnT    = kgSS3(std::clamp((elapsed - delay) / 0.6f, 0.f, 1.f));
            if (mSpawnState == SpawnState::Despawning) {
                float dd = float(cnt - 1 - ti) * 0.4f;
                float de = float(NOW - mSpawnStart) / 1000.f - dd;
                tentSpawnT = 1.f - kgSS3(std::clamp(de / 0.6f, 0.f, 1.f));
            }
        }
        if (tentSpawnT <= 0.f) continue;

        float hardenFactor  = 0.f;
        float hardenElapsed = float(NOW - st.hardenTime) * 0.001f;
        if (hardenElapsed < 0.35f) hardenFactor = 1.f - hardenElapsed / 0.35f;

        float xSide = (ti % 2 == 0) ? -1.f : 1.f;
        int   pair  = ti / 2;
        bool  isUp  = (pair % 2 == 0);
        float xOff  = xSide * (kSpreadX + float(pair) * 0.04f);
        float yOff  = isUp ? (kUpperY - float(pair)*0.04f) : (kLowerY + float(pair)*0.04f);

        glm::vec3 shoulder = root + right*xOff + worldUp*yOff;
        if (mSpawnState == SpawnState::Spawning || tentSpawnT < 1.f)
            shoulder = kgMix(spawnOrigin, shoulder, tentSpawnT);

        float wPhase = iT * 1.3f + float(ti) * 1.1f;
        float wAmp   = moving ? 0.10f : 0.04f;

        float fanT = std::clamp(
            (cnt > 1) ? float(pair) / float((cnt/2) > 0 ? (cnt/2) : 1) : 0.f, 0.f, 1.f);
        float fanA = KPI * (0.12f + fanT*0.28f + (isUp ? 0.f : 0.08f));

        glm::vec3 idleDir = kgVSafe(
            fwdBody * cosf(fanA) + right*(xSide*sinf(fanA)) - worldUp*0.35f);

        // idleTip следует за плавной позицией root — никаких искусственных
        // jump-push смещений, которые рассинхронизируют tentacle с телом.
        glm::vec3 idleTip = shoulder
            + idleDir * (scaledLen * tentSpawnT)
            + right   * (sinf(wPhase)      * wAmp * scaledLen)
            + worldUp * (cosf(wPhase*1.2f) * wAmp * scaledLen * 0.45f);

        if (!st.restInited) { st.restTip = idleTip; st.restInited = true; }
        st.restTip = MathUtils::lerp(st.restTip, idleTip,
            std::clamp(dt * (atk ? 20.f : 5.5f), 0.f, 1.f));

        glm::vec3 curTarget = atk ? st.currentHit.target : (st.hasActiveHit ? st.currentHit.target : st.restTip);
        bool      curIsSlap = atk ? st.currentHit.isSlap : false;
        uint8_t   curAnimId = atk ? st.currentHit.animId : 0;
        float     curStretch = atk ? st.currentHit.stretch : 1.f;

        glm::vec3 toTgt   = curTarget - shoulder;
        float     tgtDist = glm::length(toTgt);
        if (tgtDist > scaledLen * 2.5f) toTgt = kgVSafe(toTgt) * scaledLen * 2.5f;

        glm::vec3 atkTip   = shoulder + toTgt * curStretch;
        glm::vec3 finalTip = kgMix(st.restTip, atkTip, sp);

        float     bendV = isUp ? 0.45f : 0.20f;
        float     bendH = 0.28f;
        glm::vec3 dir01 = finalTip - shoulder;
        glm::vec3 mid1  = shoulder + dir01*0.28f
            + worldUp*(scaledLen*bendV*(1.f-sp*0.55f))
            + right  *(xSide*scaledLen*bendH*(1.f-sp*0.5f));
        glm::vec3 mid2  = shoulder + dir01*0.62f
            + worldUp*(scaledLen*bendV*0.35f*(1.f-sp*0.6f))
            + right  *(xSide*scaledLen*bendH*0.22f);

        if (!curIsSlap) {
            if (atk && sp > 0.01f) {
                uint8_t aId  = curAnimId;
                float   hOff = float(int(aId & 3) - 1) * 0.7f * xSide;
                float   vOff = float(int((aId >> 1) & 3) - 1) * 0.5f;
                glm::vec3 approachDir = kgVSafe(
                    toTgt + right*(hOff*scaledLen*0.5f) + worldUp*(vOff*scaledLen*0.3f));
                glm::vec3 launchOff =
                      right*(xSide*(0.8f+fabsf(hOff)*0.3f)*scaledLen)
                    + worldUp*((isUp ? 0.6f : 0.2f+vOff*0.3f)*scaledLen)
                    - fwdBody*(0.3f*scaledLen);
                float ss = kgSS(sp);
                mid1 = kgMix(mid1, shoulder + launchOff, ss);
                mid2 = kgMix(mid2, shoulder + approachDir*(tgtDist*0.6f), ss);
            }
        } else {
            if (atk && sp > 0.01f) {
                uint8_t   aId     = curAnimId;
                float     slapPow = kgSS(sp);
                float     sPh     = std::clamp((sp-0.5f)*2.f, 0.f, 1.f);
                glm::vec3 strikePos = shoulder + toTgt * curStretch;
                glm::vec3 swingPos;
                switch (aId & 7) {
                case 0: swingPos=shoulder+right*(xSide*scaledLen*2.2f)+worldUp*(scaledLen*0.15f); break;
                case 1: swingPos=shoulder+right*(xSide*scaledLen*2.0f)+worldUp*(scaledLen*0.45f); break;
                case 2: swingPos=shoulder+worldUp*(scaledLen*1.8f)+right*(xSide*scaledLen*0.8f); break;
                case 3: swingPos=shoulder+worldUp*(scaledLen*1.5f)+right*(xSide*scaledLen*1.5f)-fwdBody*(scaledLen*0.5f); break;
                case 4: swingPos=shoulder-worldUp*(scaledLen*0.8f)+right*(xSide*scaledLen*1.2f); break;
                case 5: swingPos=shoulder-worldUp*(scaledLen*0.6f)+right*(xSide*scaledLen*1.8f)+fwdBody*(scaledLen*0.4f); break;
                case 6: swingPos=shoulder+right*(xSide*scaledLen*2.5f)+fwdBody*(xSide*scaledLen*0.8f); break;
                default:swingPos=shoulder-fwdBody*(scaledLen*1.2f)+worldUp*(scaledLen*1.2f)+right*(xSide*scaledLen*1.0f); break;
                }
                glm::vec3 slapTip = kgMix(swingPos, strikePos, sPh);
                mid1     = kgMix(mid1, shoulder+(swingPos-shoulder)*0.45f, slapPow);
                mid2     = kgMix(mid2, shoulder+(slapTip -shoulder)*0.7f,  slapPow);
                finalTip = kgMix(st.restTip, slapTip, sp);
            }
        }

        glm::vec3 cps[6] = {
            shoulder - (mid1-shoulder)*0.5f,
            shoulder, mid1, mid2, finalTip,
            finalTip + (finalTip-mid2)*0.3f
        };

        std::vector<glm::vec3> spine;
        spine.reserve(segs + 1);
        for (int j = 0; j <= segs; j++) {
            float gT = float(j) / float(segs);
            float mp = gT * 3.f;
            int   sg = std::min((int)mp, 2);
            float lt = mp - float(sg);
            spine.push_back(kgCR(cps[sg], cps[sg+1], cps[sg+2], cps[sg+3], lt));
        }

        int startIdx = 0;
        if (isFPV) {
            for (int j = 0; j < (int)spine.size(); j++) {
                if (glm::length(spine[j] - camPos) > 0.55f) {
                    startIdx = std::max(0, j-1); break;
                }
                if (j == (int)spine.size()-1) startIdx = j;
            }
            if (startIdx >= (int)spine.size()-1) continue;
        }

        std::vector<glm::vec3> usedSpine(spine.begin()+startIdx, spine.end());
        if ((int)usedSpine.size() < 2) continue;

        std::vector<KRing> rings;
        kgRings(usedSpine, scaledRad, rRes, iT, rings);
        if ((int)rings.size() < 2) continue;

        int  nR       = (int)rings.size();
        bool isHitter = (ti == mLastStriker) && atk && sp > 0.25f;

        for (int j = 0; j < nR-1; j++) {
            float spT = float(j+startIdx) / float(segs);

            if (hide) {
                glm::vec3 midPt = (rings[j].center + rings[j+1].center) * 0.5f;
                if (glm::dot(midPt - pPos, fwdBody) < -0.15f) continue;
            }

            ImColor outerH = colOuter, coreH = colCore;
            if (hardenFactor > 0.f) {
                float hf = 1.f - hardenFactor*0.5f;
                outerH = ImColor(outerH.Value.x*hf, outerH.Value.y*hf, outerH.Value.z*hf, 1.f);
                coreH  = ImColor(coreH.Value.x*hf,  coreH.Value.y*hf,  coreH.Value.z*hf,  1.f);
            }
            kgStrip(tris, rings[j], rings[j+1], rRes, spT, camPos, outerH, coreH);

            if (mSpikes.mValue && j%3==1 && spT<0.75f) {
                glm::vec3 sDir  = kgVSafe(rings[j].n + worldUp*(isUp ? 0.6f : -0.6f));
                float     lR    = glm::length(rings[j].v[0]-rings[j].center);
                glm::vec3 sRoot = rings[j].center + sDir*lR;
                float     sLen  = lR*(3.8f-spT*2.f);
                float     sWid  = lR*0.28f;
                kgCrystalSpike(tris, sRoot, sDir, rings[j].b, sLen, sWid, camPos, colSpike);
                glm::vec3 sDir2  = kgVSafe(rings[j].b*xSide + worldUp*0.2f);
                glm::vec3 sRoot2 = rings[j].center + sDir2*lR;
                ImColor   sc2(colSpike.Value.x*.6f,colSpike.Value.y*.6f,colSpike.Value.z*.6f,1.f);
                kgCrystalSpike(tris, sRoot2, sDir2, rings[j].n, sLen*0.45f, sWid*0.6f, camPos, sc2);
            }

            // Sanguis — только ударившее щупальце, только кончик
            if (mGlow.mValue && isHitter) {
                float tipT = float(j) / float(nR-1);
                if (tipT < 0.65f) continue;
                int pCount = 2 + (int)(dp(rng)*4.f);
                for (int pi = 0; pi < pCount; pi++) {
                    if (dp(rng) > 0.55f) continue;
                    Particle p;
                    glm::vec3 sOff = glm::vec3(dr(rng),dr(rng),dr(rng)) * scaledRad*3.f;
                    p.pos = rings[j].center + sOff;
                    p.vel = kgVSafe(sOff)*(1.5f+dp(rng)*3.5f)
                          + glm::vec3(dr(rng),fabsf(dr(rng))*0.4f,dr(rng))*1.2f;
                    p.maxLife = p.life = 0.4f+dp(rng)*0.7f;
                    p.size    = 1.5f+dp(rng)*3.f;
                    float br = dp(rng);
                    switch (mParticleColor.mValue) {
                    case ParticleColor::DarkRed:
                        p.col = ImColor(0.45f+br*0.15f, 0.02f, 0.02f, 1.f); break;
                    case ParticleColor::BrightRed:
                        p.col = ImColor(0.9f, 0.1f+br*0.15f, 0.05f, 1.f);   break;
                    default:
                        p.col = ImColor(0.5f+br*0.5f, 0.02f+br*0.12f, 0.02f, 1.f); break;
                    }
                    mParticles.push_back(p);
                }
            }
        }

        // Торцы
        if (!rings.empty()) {
            const KRing& tipR = rings.back();
            for (int k = 0; k < rRes; k++) {
                int nk = (k+1)%rRes;
                kgTri(tris, tipR.center, tipR.v[k], tipR.v[nk], colCore, camPos, true);
            }
            const KRing& bR = rings.front();
            for (int k = 0; k < rRes; k++) {
                int nk = (k+1)%rRes;
                kgTri(tris, bR.center, bR.v[nk], bR.v[k], colOuter, camPos, true);
            }
        }

        // Spawn частицы (брызги крови при вылезании)
        if (mSpawnState == SpawnState::Spawning) {
            float spawnEl   = float(NOW - mSpawnStart) / 1000.f;
            float tentDelay = float(ti) * 0.4f;
            float tentEl    = spawnEl - tentDelay;
            if (tentEl > 0.f && tentEl < 0.3f && dp(rng) > 0.7f) {
                for (int pi = 0; pi < 6; pi++) {
                    Particle p;
                    p.pos = shoulder + glm::vec3(dr(rng),dr(rng),dr(rng)) * scaledRad;
                    p.vel = glm::vec3(dr(rng)*3.f, fabsf(dr(rng))*2.f+1.f, dr(rng)*3.f);
                    p.maxLife = p.life = 0.3f+dp(rng)*0.5f;
                    p.size    = 2.f+dp(rng)*3.f;
                    p.col     = ImColor(0.55f, 0.02f, 0.02f, 1.f);
                    mParticles.push_back(p);
                }
            }
        }
    }

    // ── Частицы ───────────────────────────────────────────────────────────
    if (mParticles.size() > 400)
        mParticles.erase(mParticles.begin(),
            mParticles.begin() + (int)(mParticles.size()-400));

    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.f) { it = mParticles.erase(it); continue; }
        it->pos += it->vel * dt;
        it->vel.y -= dt * 9.8f;
        it->vel   *= 0.97f;
        float a = it->life / it->maxLife;
        glm::vec2 sc2;
        if (mat.OWorldToScreen(camPos, it->pos, sc2, MathUtils::fov, guiRes)) {
            float sz   = it->size * a;
            float half = sz * 0.5f;
            dl->AddRectFilled({sc2.x-half,sc2.y-half},{sc2.x+half,sc2.y+half},
                ImColor(it->col.Value.x,it->col.Value.y,it->col.Value.z,a));
            dl->AddCircleFilled({sc2.x,sc2.y}, sz*1.8f,
                ImColor(1.f,0.15f,0.08f,a*0.12f), 8);
            glm::vec3 prevP = it->pos - it->vel*dt*2.5f;
            glm::vec2 prevSc;
            if (mat.OWorldToScreen(camPos, prevP, prevSc, MathUtils::fov, guiRes))
                dl->AddLine({prevSc.x,prevSc.y},{sc2.x,sc2.y},
                    ImColor(it->col.Value.x,it->col.Value.y,it->col.Value.z,a*0.45f), sz*0.4f);
        }
        ++it;
    }

    // ── Рендер ────────────────────────────────────────────────────────────
    std::sort(tris.begin(), tris.end(),
        [](const KTri& a, const KTri& b){ return a.depth > b.depth; });

    dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    dl->Flags |=  ImDrawListFlags_AntiAliasedLines;

    for (auto& tr : tris) {
        glm::vec2 sc[3]; bool ok = true;
        for (int i = 0; i < 3; i++)
            if (!mat.OWorldToScreen(camPos, tr.p[i], sc[i], MathUtils::fov, guiRes))
            { ok = false; break; }
        if (!ok) continue;
        dl->AddTriangleFilled(
            {sc[0].x,sc[0].y},{sc[1].x,sc[1].y},{sc[2].x,sc[2].y}, tr.col);
    }
    for (auto& tr : tris) {
        glm::vec2 sc[3]; bool ok = true;
        for (int i = 0; i < 3; i++)
            if (!mat.OWorldToScreen(camPos, tr.p[i], sc[i], MathUtils::fov, guiRes))
            { ok = false; break; }
        if (!ok) continue;
        dl->AddTriangle(
            {sc[0].x,sc[0].y},{sc[1].x,sc[1].y},{sc[2].x,sc[2].y},
            ImColor(5,0,1,120), 0.38f);
    }
    dl->Flags |= ImDrawListFlags_AntiAliasedFill;
}