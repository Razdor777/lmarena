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
#include <cmath>
#include <cfloat>

static constexpr float KPI  = 3.14159265358979f;
static constexpr float K2PI = 6.28318530717959f;

// true  -> getPos() у игрока = уровень глаз (feet = pos - 1.62)
// false -> getPos() = ноги.
// Если щупальца растут не из спины (на 1.6 блока выше/ниже), переключи.
static constexpr bool kPosIsEye = true;

// ═════════════════════════════════════════════════════════════════════════════
//  Math helpers
// ═════════════════════════════════════════════════════════════════════════════
static inline glm::vec3 kgVSafe(const glm::vec3& v, const glm::vec3& fb = {0, 1, 0})
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
static inline float kgSm(float a, float b, float x)
{
    float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}
static inline float kgLerp(float a, float b, float t) { return a + (b - a) * t; }
static inline glm::vec3 kgMix(const glm::vec3& a, const glm::vec3& b, float t) { return a + (b - a) * t; }

static inline glm::vec3 kgCR(const glm::vec3& p0, const glm::vec3& p1,
                             const glm::vec3& p2, const glm::vec3& p3, float t)
{
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * (2.f * p1 + (-p0 + p2) * t
        + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2
        + (-p0 + 3.f * p1 - 3.f * p2 + p3) * t3);
}
static inline glm::vec3 kgBez(const glm::vec3& a, const glm::vec3& b,
                              const glm::vec3& c, const glm::vec3& d, float t)
{
    float u = 1.f - t;
    return a * (u * u * u) + b * (3.f * u * u * t) + c * (3.f * u * t * t) + d * (t * t * t);
}
static inline float kgMs(uint64_t since) { return float(int64_t(NOW - since)); }
static inline ImU32 kgCol(const glm::vec3& c, float a)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, a));
}

// ═════════════════════════════════════════════════════════════════════════════
//  Палитры
// ═════════════════════════════════════════════════════════════════════════════
struct KPal { glm::vec3 blade, shade, glow; };
static const KPal kPals[3] = {
    { {0.80f, 0.03f, 0.06f}, {0.22f, 0.00f, 0.03f}, {1.00f, 0.30f, 0.16f} },   // Crimson
    { {0.58f, 0.02f, 0.05f}, {0.15f, 0.00f, 0.02f}, {0.95f, 0.16f, 0.12f} },   // Blood
    { {0.95f, 0.10f, 0.04f}, {0.30f, 0.02f, 0.02f}, {1.00f, 0.55f, 0.20f} },   // Ember
};

// ═════════════════════════════════════════════════════════════════════════════
//  Shading: тоновые полосы + глянец + тёмная кромка + бегущий пульс
// ═════════════════════════════════════════════════════════════════════════════
static const glm::vec3 kKL = kgVSafe({0.4f, -1.f, 0.3f});

struct KMat {
    glm::vec3 blade, shade, glow;
    float t, flash, pulse;
};

static ImVec4 kgShade(const KMat& m, const glm::vec3& n, const glm::vec3& V, float u, float ang)
{
    const glm::vec3 Ld = -kKL;
    float d = glm::dot(n, Ld) * 0.5f + 0.5f;

    // анимешные полосы света с мягкими границами
    glm::vec3 hot = kgMix(m.blade, m.glow, 0.55f);
    glm::vec3 c = kgMix(m.shade, m.blade, kgSm(0.22f, 0.50f, d));
    c = kgMix(c, hot, kgSm(0.66f, 0.92f, d) * 0.75f);

    // мраморность плоти
    float mb = 0.5f + 0.5f * sinf(u * 21.f + 2.f * sinf(ang * 2.f + u * 6.f));
    c *= 0.84f + 0.16f * mb;

    // темнее у корня и у кончика
    c = kgMix(c, m.shade * 0.8f, (1.f - kgSm(0.f, 0.14f, u)) * 0.6f);
    c = kgMix(c, m.shade * 0.9f, kgSm(0.72f, 1.f, u) * 0.55f);

    // тёмная кромка
    float ndv  = std::max(glm::dot(n, V), 0.f);
    float edge = powf(1.f - ndv, 2.2f);
    c = kgMix(c, m.shade * 0.30f, std::clamp(edge * 0.85f, 0.f, 1.f));

    // глянцевый блик
    glm::vec3 H = kgVSafe(Ld + V);
    float sp = powf(std::max(glm::dot(n, H), 0.f), 46.f);
    c += glm::vec3(1.f, 0.62f, 0.50f) * (kgSm(0.45f, 0.80f, sp) * 0.40f * (1.f - edge));

    // пульс: «сердцебиение» + волна от корня к кончику
    float beat = powf(0.5f + 0.5f * sinf(m.t * 2.6f), 3.f);
    float wave = powf(0.5f + 0.5f * sinf(u * 8.f - m.t * 3.4f), 4.f);
    c *= 1.f + m.pulse * 0.10f * beat;
    c += m.glow * (m.pulse * 0.30f * wave * (1.f - edge * 0.7f));
    c += m.glow * (m.flash * 0.45f);
    return ImVec4(c.x, c.y, c.z, 1.f);
}

// профили лезвия
static float kgWidthProf(float u)
{
    float root = 0.50f + 0.50f * kgSS(u / 0.10f);
    float body = powf(std::max(1.f - powf(u, 1.4f), 0.f), 1.2f);
    return root * body;
}
static float kgThickProf(float u)
{
    float root = 0.60f + 0.40f * kgSS(u / 0.10f);
    return root * powf(std::max(1.f - u, 0.f), 0.75f);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Geometry containers
// ═════════════════════════════════════════════════════════════════════════════
struct KRing   { glm::vec3 c, T, N, B; float rw, rt; };   // N = широкая ось, B = тонкая
struct KVert   { glm::vec3 w{}, n{}; ImVec2 s{}; ImU32 col = 0; bool ok = false, occ = false, face = false; };
struct KTriRef { int a, b, c; float depth; };

// Коробка тела игрока (окклюзия + коллизия)
struct KBody {
    bool      enabled = false;        // только для окклюзии
    glm::vec3 base{};
    glm::vec3 right{}, fwd{};
    float     hx = 0.30f, zBack = 0.12f, zFront = 0.15f, h = 1.8f;
};

static bool kgOccluded(const KBody& b, const glm::vec3& cam, const glm::vec3& p)
{
    if (!b.enabled) return false;
    glm::vec3 a = cam - b.base;
    glm::vec3 d = p - cam;
    float o[3]  = { glm::dot(a, b.right), a.y, glm::dot(a, b.fwd) };
    float dd[3] = { glm::dot(d, b.right), d.y, glm::dot(d, b.fwd) };
    float mn[3] = { -b.hx, 0.f, -b.zBack };
    float mx[3] = {  b.hx, b.h,  b.zFront };
    float t0 = 0.f, t1 = 1.f;
    for (int i = 0; i < 3; i++) {
        if (fabsf(dd[i]) < 1e-8f) {
            if (o[i] < mn[i] || o[i] > mx[i]) return false;
        } else {
            float ta = (mn[i] - o[i]) / dd[i];
            float tb = (mx[i] - o[i]) / dd[i];
            if (ta > tb) std::swap(ta, tb);
            t0 = std::max(t0, ta);
            t1 = std::min(t1, tb);
            if (t0 > t1) return false;
        }
    }
    return true;
}

// Коллизия: выталкиваем точку из тела игрока к ближайшей грани.
// Грани спереди/сзади менее выгодны — щупальце уходит вбок или вверх, а не «обнимает» торс.
static void kgPushOut(const KBody& b, glm::vec3& p, float margin)
{
    glm::vec3 a = p - b.base;
    float lx = glm::dot(a, b.right), ly = a.y, lz = glm::dot(a, b.fwd);
    float hx = b.hx + margin, zb = b.zBack + margin, zf = b.zFront + margin;
    float top = b.h + 0.10f + margin * 0.5f;
    if (ly < -0.05f || ly > top) return;
    if (lx <= -hx || lx >= hx || lz <= -zb || lz >= zf) return;
    float d[5] = { hx - lx, lx + hx, (zf - lz) * 1.6f, (lz + zb) * 1.6f, top - ly };
    int bi = 0;
    for (int i = 1; i < 5; i++) if (d[i] < d[bi]) bi = i;
    switch (bi) {
    case 0: p += b.right * (hx - lx);  break;
    case 1: p -= b.right * (lx + hx);  break;
    case 2: p += b.fwd   * (zf - lz);  break;
    case 3: p -= b.fwd   * (lz + zb);  break;
    default: p.y += top - ly;          break;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Стили щупалец: у каждого своя стойка и своя атака.
//  Локальные оси: x = наружу (зеркалится по side), y = вверх, z = вперёд.
//  Вектора wind / slap заданы в долях длины щупальца.
//  Порядок бьющих: 0 -> 1 -> 2 -> 3 (лево-верх, право-низ, право-верх, лево-низ)
// ═════════════════════════════════════════════════════════════════════════════
struct KStyle {
    float     side;            // -1 лево, +1 право
    glm::vec3 sh;              // плечо (3-е лицо): наружу, вверх, вперёд от корня
    glm::vec2 fpv;             // плечо (1-е лицо): наружу, вверх от камеры
    glm::vec3 c1, c2, tip;     // стойка: веер за спиной
    glm::vec3 wind;            // Sting: кончик в замахе (свёрнут над плечом)
    glm::vec3 slap;            // Slap: откуда начинается хлёст
    glm::vec3 bend;            // в какую сторону выгибается дуга
    float     lift;            // дуга кончика при выпаде (+ сверху, - снизу), в долях длины
    float     spd;             // личная скорость
};
static const KStyle kStyles[8] = {
    // 0: лево-верх — пикирует сверху
    { -1.f, {0.40f, 0.30f, 0.f},   {0.55f, 0.30f},
      {0.25f, 0.55f,-0.25f}, {0.70f, 1.05f,-0.80f}, {1.00f, 1.05f,-1.50f},
      {0.30f, 0.65f,-0.25f}, {0.85f, 0.30f,-0.45f}, {0.5f, 1.0f,-0.3f},  0.30f, 1.00f },
    // 1: право-низ — низкий боковой выпад
    {  1.f, {0.42f,-0.02f, 0.f},   {0.62f,-0.10f},
      {0.45f, 0.00f,-0.35f}, {1.00f,-0.15f,-0.95f}, {1.35f,-0.10f,-1.70f},
      {0.70f, 0.15f,-0.45f}, {0.95f,-0.15f,-0.30f}, {1.0f,-0.2f,-0.2f}, -0.05f, 0.92f },
    // 2: право-верх — прямой рубящий
    {  1.f, {0.34f, 0.36f, 0.f},   {0.52f, 0.42f},
      {0.20f, 0.50f,-0.30f}, {0.55f, 0.95f,-0.95f}, {0.75f, 0.75f,-1.70f},
      {0.20f, 0.75f,-0.10f}, {0.80f, 0.45f,-0.55f}, {0.3f, 1.0f,-0.2f},  0.18f, 1.08f },
    // 3: лево-низ — снизу вверх
    { -1.f, {0.44f,-0.10f, 0.f},   {0.62f,-0.22f},
      {0.40f, 0.05f,-0.40f}, {0.85f,-0.30f,-1.00f}, {1.05f,-0.55f,-1.70f},
      {0.65f, 0.05f,-0.50f}, {0.90f,-0.05f,-0.35f}, {1.0f,-0.4f,-0.1f}, -0.25f, 0.96f },
    // 4: лево-середина
    { -1.f, {0.36f, 0.12f,-0.03f}, {0.50f, 0.05f},
      {0.30f, 0.25f,-0.40f}, {0.85f, 0.35f,-1.00f}, {1.30f, 0.20f,-1.50f},
      {0.60f, 0.40f,-0.50f}, {0.85f, 0.15f,-0.60f}, {1.0f, 0.3f,-0.3f},  0.12f, 1.04f },
    // 5: право-середина
    {  1.f, {0.36f, 0.12f,-0.03f}, {0.50f, 0.05f},
      {0.25f, 0.35f,-0.40f}, {0.65f, 0.55f,-1.00f}, {0.95f, 0.60f,-1.60f},
      {0.55f, 0.45f,-0.45f}, {0.85f, 0.10f,-0.60f}, {0.8f, 0.6f,-0.3f},  0.10f, 0.98f },
    // 6: лево-верх-центр — вертикальная стойка
    { -1.f, {0.20f, 0.45f, 0.f},   {0.40f, 0.60f},
      {0.05f, 0.70f,-0.20f}, {0.15f, 1.25f,-0.50f}, {0.45f, 1.65f,-0.60f},
      {0.10f, 0.80f,-0.15f}, {0.60f, 0.60f,-0.40f}, {0.2f, 1.0f,-0.3f},  0.34f, 1.10f },
    // 7: право-верх-центр
    {  1.f, {0.20f, 0.45f, 0.f},   {0.40f, 0.60f},
      {0.05f, 0.70f,-0.20f}, {0.25f, 1.20f,-0.55f}, {0.60f, 1.55f,-0.85f},
      {0.15f, 0.80f,-0.15f}, {0.60f, 0.55f,-0.40f}, {0.5f, 1.0f,-0.2f},  0.26f, 1.02f },
};

// Фазы атаки (доли длительности): замах -> выпад -> удержание -> возврат
static constexpr float kTW = 0.30f;   // конец замаха
static constexpr float kTI = 0.42f;   // момент удара
static constexpr float kTH = 0.64f;   // конец удержания

static constexpr float kMaxStretch = 1.9f;   // максимум вытягивания (в длинах щупальца)
static constexpr float kTrailLife  = 0.20f;  // сек, след кончика

// Физика (пружина-демпфер на узел)
static constexpr float kIdleRoot = 160.f, kIdleTip = 35.f;
static constexpr float kAtkRoot  = 500.f, kAtkTip  = 1100.f;
static constexpr float kZetaIdle = 0.45f, kZetaAtk = 0.85f;

static constexpr float kSpawnTime   = 1.4f;
static constexpr float kDespawnTime = 1.1f;

// Тряска камеры
static constexpr float kShakeDur   = 0.55f;
static constexpr float kShakeDecay = 9.0f;
// Порядок осей в CameraDirectLookComponent::mRotRads.
// Если вертикаль и горизонталь перепутаны, поменяй местами.
static constexpr int   kPitchIdx = 0;
static constexpr int   kYawIdx   = 1;

// scratch-буферы
static std::vector<KVert>     sV, sC;
static std::vector<KRing>     sR;
static std::vector<glm::vec3> sSp;
static std::vector<KTriRef>   sT;
static std::vector<ImVec2>    sOL, sOR;

// ═════════════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═════════════════════════════════════════════════════════════════════════════
void Kagune::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent,    &Kagune::onRenderEvent   >(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<LookInputEvent, &Kagune::onLookInputEvent>(this);

    mEnableTime = NOW;
    mLastHit    = NOW;
    mWantShown  = true;
    mVis        = 0.f;
    mParticles.clear();
    for (auto& t : mTentacles) t = TentacleState{};
    mStriker         = 0;
    mHits            = 0;
    mWasOnGround     = true;
    mSmoothedBodyYaw = -999.f;
    mSmYInit         = false;

    mPending         = PendingHit{};
    mNextAttackAt    = 0;
    mLastPacketHit   = 0;
    mHitIntervalEma  = 0.8f;

    mShakeStart   = 0;
    mShakeAmp     = 0.f;
    mShakeApplied = glm::vec2(0.f);
}

void Kagune::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &Kagune::onRenderEvent   >(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &Kagune::onLookInputEvent>(this);
    mParticles.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Camera shake
// ═════════════════════════════════════════════════════════════════════════════
void Kagune::triggerShake(float power)
{
    if (mShake.mValue <= 0.f) return;

    float t   = kgMs(mShakeStart) * 0.001f;
    float cur = (t >= 0.f && t < kShakeDur) ? mShakeAmp * expf(-t * kShakeDecay) : 0.f;
    float cap = 1.6f * mShake.mValue;
    mShakeAmp   = std::min(cur + power * mShake.mValue, cap);
    mShakeStart = NOW;
    mShakeDir   = glm::vec2(rndS() < 0.f ? -1.f : 1.f, rndS() < 0.f ? -1.f : 1.f)
                * (0.7f + 0.3f * rndU());
}

// mRotRads хранит накопленный поворот, поэтому прибавляем только дельту между
// нужным сейчас смещением и уже применённым. Когда тряска затухает, суммарное
// смещение возвращается в ноль и прицел не уезжает.
void Kagune::onLookInputEvent(LookInputEvent& event)
{
    auto* dlk = event.mCameraDirectLookComponent;
    if (!dlk) return;

    glm::vec2 target(0.f);
    if (mShake.mValue > 0.f && mShakeAmp > 0.f) {
        float t = kgMs(mShakeStart) * 0.001f;
        if (t >= 0.f && t < kShakeDur) {
            float env  = expf(-t * kShakeDecay);
            float pDeg = mShakeAmp * env * (0.35f * sinf(t * 24.f) + 0.65f * sinf(t * 51.f));
            float yDeg = mShakeAmp * env * 0.7f * (0.5f * sinf(t * 19.f) + 0.5f * sinf(t * 43.f));
            target[kPitchIdx] = pDeg * mShakeDir.x * (KPI / 180.f);
            target[kYawIdx]   = yDeg * mShakeDir.y * (KPI / 180.f);
        }
    }

    glm::vec2 d = target - mShakeApplied;
    if (d.x == 0.f && d.y == 0.f) return;

    dlk->mRotRads.x += d.x;
    dlk->mRotRads.y += d.y;
    mShakeApplied = target;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Packet: регистрируем удар (только запоминаем, анимацией занимается планировщик)
// ═════════════════════════════════════════════════════════════════════════════
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
    auto* rot = player->getActorRotationComponent();
    if (!rot) return;
    auto* headR = player->getActorHeadRotationComponent();

    const float headYaw = (headR ? headR->mHeadRot : rot->mYaw) * (KPI / 180.f);
    const float pitch   = rot->mPitch * (KPI / 180.f);
    const glm::vec3 rayDir(-sinf(headYaw) * cosf(pitch), -sinf(pitch), cosf(headYaw) * cosf(pitch));

    glm::vec3 eyePos = *player->getPos();
    if (!kPosIsEye) eyePos.y += PLAYER_HEIGHT;

    glm::vec3 tPos{};
    bool found = false;

    for (auto* a : ActorUtils::getActorList(false, true)) {
        if (!a || a->getRuntimeID() != iut->mActorId) continue;

        auto* sh2 = a->getAABBShapeComponent();
        float h2 = sh2 ? sh2->mHeight : 1.8f;
        float w2 = sh2 ? sh2->mWidth  : 0.6f;
        glm::vec3 aPos = *a->getPos();
        float feetY = (kPosIsEye && a->isPlayer()) ? aPos.y - PLAYER_HEIGHT : aPos.y;

        glm::vec3 bMin(aPos.x - w2 * 0.5f, feetY,      aPos.z - w2 * 0.5f);
        glm::vec3 bMax(aPos.x + w2 * 0.5f, feetY + h2, aPos.z + w2 * 0.5f);
        glm::vec3 chest(aPos.x, feetY + h2 * 0.6f, aPos.z);

        float tMin = 0.001f, tMax = 16.f;
        bool hit = true;
        for (int ax = 0; ax < 3; ax++) {
            float d = rayDir[ax], mn = bMin[ax], mx = bMax[ax], o = eyePos[ax];
            if (fabsf(d) < 1e-6f) {
                if (o < mn || o > mx) { hit = false; break; }
            } else {
                float t1 = (mn - o) / d, t2 = (mx - o) / d;
                if (t1 > t2) std::swap(t1, t2);
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) { hit = false; break; }
            }
        }
        tPos  = hit ? (eyePos + rayDir * ((tMin + tMax) * 0.5f)) : chest;
        found = true;
        break;
    }
    if (!found) tPos = eyePos + rayDir * mLength.mValue;

    tPos += glm::vec3(rndS(), rndS() * 0.6f, rndS()) * 0.08f;

    // статистика CPS (сглаженный интервал между ударами)
    uint64_t now = NOW;
    float interval = (mLastPacketHit == 0) ? 0.8f : float(int64_t(now - mLastPacketHit)) * 0.001f;
    mLastPacketHit = now;
    mHitIntervalEma = (interval > 0.8f) ? 0.8f
                    : kgLerp(mHitIntervalEma, std::max(interval, 0.005f), 0.4f);

    // самый свежий удар перезаписывает несъеденный — на высоком CPS лишнее отбрасываем
    mPending.valid  = true;
    mPending.target = tPos;
    mPending.time   = now;

    mLastHit   = now;
    mWantShown = true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Render
// ═════════════════════════════════════════════════════════════════════════════
void Kagune::onRenderEvent(RenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    auto* opts = ci->getOptions();
    if (!opts) return;

    const int  camMode = opts->mThirdPerson->value;
    const bool isFPV   = (camMode == 0);
    if (!mShowFPV.mValue && isFPV) return;

    auto* player = ci->getLocalPlayer();
    if (!player) return;
    auto* rot = player->getActorRotationComponent();
    if (!rot) return;
    auto* headR = player->getActorHeadRotationComponent();
    auto* sh    = player->getAABBShapeComponent();
    auto* svc   = player->getStateVectorComponent();

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.05f);
    const float iT = float(ImGui::GetTime());

    // ── Позиция игрока ────────────────────────────────────────────────────
    glm::vec3 pPos = *player->getPos();
    if (auto* rpc = player->getRenderPositionComponent()) {
        pPos.x = rpc->mPosition.x;
        pPos.z = rpc->mPosition.z;
    }
    if (!mSmYInit || fabsf(pPos.y - mSmY) > 3.f) { mSmY = pPos.y; mSmYInit = true; }
    mSmY += (pPos.y - mSmY) * (1.f - expf(-dt * 30.f));
    pPos.y = mSmY;

    const glm::vec3 vel = svc ? svc->mVelocity : glm::vec3(0.f);
    const float hSpd    = sqrtf(vel.x * vel.x + vel.z * vel.z);
    const bool  moving  = hSpd > 0.003f;
    const bool  onGround = player->isOnGround();

    float kick = 0.f;
    if (mWasOnGround && !onGround && vel.y > 0.05f) kick = -2.5f;
    else if (!mWasOnGround && onGround)             kick = -1.5f;
    mWasOnGround = onGround;

    // ── Сглаженный yaw тела ───────────────────────────────────────────────
    const float rawHeadYaw = (headR ? headR->mHeadRot : rot->mYaw) * (KPI / 180.f);
    const float pitch      = rot->mPitch * (KPI / 180.f);
    if (mSmoothedBodyYaw < -900.f) mSmoothedBodyYaw = rawHeadYaw;

    float dyaw = rawHeadYaw - mSmoothedBodyYaw;
    while (dyaw >  KPI) dyaw -= K2PI;
    while (dyaw < -KPI) dyaw += K2PI;
    mSmoothedBodyYaw += dyaw * std::clamp(dt * (moving ? 8.f : 4.f), 0.f, 1.f);
    while (mSmoothedBodyYaw >  KPI) mSmoothedBodyYaw -= K2PI;
    while (mSmoothedBodyYaw < -KPI) mSmoothedBodyYaw += K2PI;
    const float bodyYaw = mSmoothedBodyYaw;

    const glm::vec3 up(0.f, 1.f, 0.f);
    const glm::vec3 fwdBody = kgVSafe({-sinf(bodyYaw), 0.f, cosf(bodyYaw)});
    const glm::vec3 fwdAim  = kgVSafe({
        -sinf(rawHeadYaw) * cosf(pitch),
        -sinf(pitch),
         cosf(rawHeadYaw) * cosf(pitch)});
    const glm::vec3 right = kgVSafe(glm::cross(fwdBody, up), {1.f, 0.f, 0.f});

    const float pH    = sh ? sh->mHeight : 1.8f;
    const float feetY = kPosIsEye ? pPos.y - PLAYER_HEIGHT : pPos.y;

    // ── Камера ────────────────────────────────────────────────────────────
    const glm::vec3 camPos = RenderUtils::transform.mOrigin;
    auto&           mat    = RenderUtils::transform.mMatrix;
    auto*           dl     = ImGui::GetBackgroundDrawList();
    auto            guiRes = ci->getGuiData()->mResolution;
    if (!dl) return;

    // Корень: от первого лица — у камеры (чуть сзади), от третьего — на спине
    glm::vec3 root;
    if (isFPV) root = camPos - fwdBody * 0.30f - up * 0.22f;
    else       root = glm::vec3(pPos.x, feetY + kRootH, pPos.z) - fwdBody * kOffsetZ;

    // ── Видимость (спавн / деспавн) ───────────────────────────────────────
    if (mWantShown && kgMs(mLastHit) * 0.001f > mHideDelay.mValue) mWantShown = false;
    if (mWantShown) mVis = std::min(mVis + dt / kSpawnTime,   1.f);
    else            mVis = std::max(mVis - dt / kDespawnTime, 0.f);

    if (mVis <= 0.f && !mWantShown) {
        for (auto& t : mTentacles) { t.inited = false; t.hasActive = false; }
        mParticles.clear();
        mPending.valid = false;
        return;
    }

    // ── Настройки ─────────────────────────────────────────────────────────
    const int   cnt     = std::clamp((int)mCount.mValue,    1, kMax);
    const float maxLen  = mLength.mValue;
    const float baseRad = mThickness.mValue * 0.014f;
    const int   M       = std::clamp((int)mSegments.mValue, 8, 48);
    const int   res     = std::clamp(((int)mRingRes.mValue / 4) * 4, 8, 16);   // кратно 4
    const float animDur = std::max(mAnimTime.mValue, 0.2f);
    const float swayMul = mSway.mValue;
    const float pulseK  = std::clamp(mPulse.mValue, 0.f, 2.f);
    const float widthK  = std::clamp(mWidth.mValue, 0.4f, 2.f);
    const bool  detail  = mDetail.mValue;

    glm::vec3 cBlade, cShade, cGlow;
    {
        const Palette pal = mPalette.mValue;
        if (pal == Palette::Custom) {
            cBlade = glm::vec3(mColBlade.mValue[0], mColBlade.mValue[1], mColBlade.mValue[2]);
            cGlow  = glm::vec3(mColGlow .mValue[0], mColGlow .mValue[1], mColGlow .mValue[2]);
            cShade = glm::vec3(mColShade.mValue[0], mColShade.mValue[1], mColShade.mValue[2]);
        } else {
            const KPal& p = kPals[std::clamp((int)pal, 0, 2)];
            cBlade = p.blade; cShade = p.shade; cGlow = p.glow;
        }
    }

    const float alert      = 1.f - kgSS(kgMs(mLastHit) * 0.001f / 4.f);
    const float alertFwd   = isFPV ? 0.90f : 0.25f;
    const float shakeScale = std::clamp(mHitIntervalEma / 0.25f, 0.25f, 1.f);
    const float beatNow    = powf(0.5f + 0.5f * sinf(iT * 2.6f), 3.f);

    KBody body;
    body.enabled = !isFPV;
    body.base    = glm::vec3(pPos.x, feetY, pPos.z);
    body.right   = right;
    body.fwd     = fwdBody;
    body.h       = pH;

    // ── Кровь ─────────────────────────────────────────────────────────────
    const ParticleColor pcMode = mParticleColor.mValue;
    auto bloodCol = [&](float br) -> ImColor {
        switch (pcMode) {
        case ParticleColor::DarkRed:   return ImColor(0.45f + br * 0.15f, 0.02f, 0.02f, 1.f);
        case ParticleColor::BrightRed: return ImColor(0.90f, 0.10f + br * 0.15f, 0.05f, 1.f);
        default:                       return ImColor(0.50f + br * 0.50f, 0.02f + br * 0.12f, 0.02f, 1.f);
        }
    };
    auto spawnBurst = [&](const glm::vec3& at, const glm::vec3& outDir, int n) {
        if (!mGlow.mValue) return;
        for (int i = 0; i < n; i++) {
            Particle p;
            p.pos = at + glm::vec3(rndS(), rndS(), rndS()) * 0.05f;
            p.vel = outDir * (1.5f + rndU() * 3.5f)
                  + glm::vec3(rndS(), std::fabs(rndS()) * 0.8f, rndS()) * 1.6f;
            p.maxLife = p.life = 0.4f + rndU() * 0.7f;
            p.size    = 1.5f + rndU() * 3.f;
            p.col     = bloodCol(rndU());
            mParticles.push_back(p);
        }
    };

    // ═════════════════════════════════════════════════════════════════════
    //  Планировщик ударов: по очереди, с лимитом частоты
    // ═════════════════════════════════════════════════════════════════════
    auto startAttack = [&](int ti, const glm::vec3& target, float dur) {
        auto& st = mTentacles[ti];
        const KStyle& sty = kStyles[ti];
        const AttackMode mode = mAttackMode.mValue;
        const bool slap = (mode == AttackMode::Slap) ||
                          (mode == AttackMode::Alternate && (((st.hitCount + (uint32_t)ti) & 1u) != 0u));
        HitRecord r;
        r.hitTime = NOW;
        r.target  = target;
        r.isSlap  = slap;
        r.dur     = std::max(dur * sty.spd, 0.18f);
        r.jit     = glm::vec3(rndS(), rndS(), rndS()) * 0.22f;
        st.cur = r;
        st.hasActive  = true;
        st.impactDone = false;
        st.hitCount++;
        mHits++;
    };
    auto freeTent = [&](int idx) -> bool {
        const auto& t = mTentacles[idx];
        if (!t.hasActive) return true;
        float np = kgMs(t.cur.hitTime) * 0.001f / t.cur.dur;
        return np > kTH;                        // уже возвращается — можно перебить
    };

    if (mPending.valid) {
        if (mVis < 0.95f)                         mPending.time = NOW;    // ждём, пока вылезут
        else if (kgMs(mPending.time) > 300.f)     mPending.valid = false; // протух
    }
    if (mPending.valid && mVis >= 0.95f && NOW >= mNextAttackAt) {
        int pick = -1;
        for (int o = 0; o < cnt; o++) {
            int idx = (mStriker + o) % cnt;
            if (freeTent(idx)) { pick = idx; break; }
        }
        if (pick >= 0) {
            float effInt = std::max(mHitIntervalEma, 0.075f);
            float dur    = std::clamp(effInt * float(cnt) * 0.92f, std::min(0.22f, animDur), animDur);
            startAttack(pick, mPending.target, dur);

            // очень редко — парный удар с противоположной стороны (только на низком CPS)
            if (cnt >= 2 && mHitIntervalEma > 0.22f && rndU() < 0.06f) {
                for (int o = 1; o < cnt; o++) {
                    int idx = (pick + o) % cnt;
                    if (kStyles[idx].side != kStyles[pick].side && freeTent(idx)) {
                        startAttack(idx, mPending.target + glm::vec3(rndS(), rndS() * 0.5f, rndS()) * 0.15f, dur);
                        break;
                    }
                }
            }
            mStriker = (pick + 1) % cnt;
            float gap = std::max(0.07f, dur / float(cnt) * 0.9f);
            mNextAttackAt = NOW + (uint64_t)(gap * 1000.f);
            mPending.valid = false;
        }
    }

    // ═════════════════════════════════════════════════════════════════════
    //  Проход 1: цели -> физика
    // ═════════════════════════════════════════════════════════════════════
    struct Act { int ti; float depth, R, flash; };
    std::vector<Act> acts;
    acts.reserve(cnt);

    constexpr int N = kNodes;
    const float kStag = 0.6f;

    for (int ti = 0; ti < cnt; ti++) {
        auto& st = mTentacles[ti];
        const KStyle& sty = kStyles[ti];
        const float side  = sty.side;

        // рост (со стаггером)
        float delay = (cnt > 1) ? kStag * float(ti) / float(cnt - 1) : 0.f;
        float grow  = kgSS3(std::clamp(mVis * (1.f + kStag) - delay, 0.f, 1.f));
        if (grow < 0.02f) { st.inited = false; st.hasActive = false; continue; }

        const float Leff = maxLen * grow;
        const float Reff = baseRad * (0.4f + 0.6f * grow);
        const float colMargin = 0.04f + Reff * 0.7f;

        auto loc = [&](const glm::vec3& v) -> glm::vec3 {
            return right * (side * v.x) + up * v.y + fwdBody * v.z;
        };

        const glm::vec3 shoulder = isFPV
            ? root + right * (side * sty.fpv.x) + up * sty.fpv.y
            : root + right * (side * sty.sh.x) + up * sty.sh.y + fwdBody * sty.sh.z;

        // ── Огибающие атаки ───────────────────────────────────────────────
        if (st.hasActive && kgMs(st.cur.hitTime) * 0.001f >= st.cur.dur) st.hasActive = false;

        float amt = 0.f, ss = 0.f, ft = 0.f, coil = 0.f;
        if (st.hasActive) {
            float np = std::max(kgMs(st.cur.hitTime) * 0.001f / st.cur.dur, 0.f);
            if      (np < 0.14f) amt = kgSS(np / 0.14f);
            else if (np < kTH)   amt = 1.f;
            else                 amt = 1.f - kgSS((np - kTH) / (1.f - kTH));

            float q = std::clamp((np - kTW) / (kTI - kTW), 0.f, 1.f);
            ss = st.cur.isSlap ? powf(q, 1.5f) : q * q;
            ft = kgSS(std::clamp((np - kTI) / (kTH - kTI), 0.f, 1.f));
            if (np < kTW) coil = kgSS(np / kTW);
        }

        // ── Стойка: веер за спиной, бегущая волна, дрейф от скорости ──────
        glm::vec3 idle[N];
        {
            const glm::vec3 lc[4] = {{0, 0, 0}, sty.c1, sty.c2, sty.tip};
            float arc = 0.f; glm::vec3 prev(0.f);
            for (int m = 1; m <= 20; m++) {
                glm::vec3 p = kgBez(lc[0], lc[1], lc[2], lc[3], float(m) / 20.f);
                arc += glm::length(p - prev); prev = p;
            }
            const float sc = Leff / std::max(arc, 1e-3f);

            const float ph  = iT * 1.10f + float(ti) * 1.9f;
            const float amp = Leff * 0.14f * swayMul * (moving ? 1.5f : 1.f) * (1.f + (1.f - grow) * 1.5f);

            glm::vec3 drift = -glm::vec3(vel.x, 0.f, vel.z) * (isFPV ? 1.5f : 3.5f);
            float dl3 = glm::length(drift);
            if (dl3 > Leff * 0.4f) drift *= (Leff * 0.4f / dl3);

            for (int i = 0; i < N; i++) {
                float u = float(i) / float(N - 1);
                glm::vec3 lp = kgBez(lc[0], lc[1], lc[2], lc[3], u) * sc;
                if (isFPV) {
                    // в первом лице держим щупальца по краям экрана, чуть впереди
                    lp = glm::vec3(lp.x * 0.9f, lp.y * 0.55f,
                                   1.2f * fabsf(lp.x) + 0.25f + 0.15f * lp.z);
                }
                glm::vec3 off = loc(lp);

                float w = powf(u, 1.2f);
                off += up      * (sinf(ph - u * 4.0f)              * amp * w)
                     + right   * (sinf(ph * 0.77f + 1.2f - u * 3.1f) * amp * 0.6f * w)
                     + fwdBody * (sinf(ph * 0.53f + 0.7f - u * 2.2f) * amp * 0.3f * w);
                off += drift * (u * u);
                off += (fwdAim * alertFwd + up * 0.15f) * (Leff * alert * u * u);

                idle[i] = shoulder + off;
            }
        }

        // ── Поза атаки ────────────────────────────────────────────────────
        glm::vec3 goal[N];
        float thin = 1.f;
        if (amt > 0.001f) {
            const HitRecord& h = st.cur;
            glm::vec3 toT  = h.target - shoulder;
            float     dist = glm::length(toT);
            glm::vec3 dirT = dist > 1e-4f ? toT / dist : fwdAim;

            // далеко — вытягиваемся; рядом — остаёмся компактными (дуга вместо растяжения)
            float reach = std::clamp(dist + 0.05f, 0.4f, Leff * kMaxStretch);
            glm::vec3 strikeTip = shoulder + dirT * reach;

            // начальная точка: Sting — свёрнут над плечом, Slap — широко сбоку
            glm::vec3 startTip = h.isSlap
                ? shoulder + loc(sty.slap + h.jit * 0.6f) * Leff
                : shoulder + loc(sty.wind + h.jit * 0.5f) * Leff;

            // прицеливание: мелкая дрожь в конце замаха
            startTip += (right * sinf(iT * 63.f + float(ti)) + up * cosf(iT * 71.f + float(ti) * 2.f))
                      * (0.012f * Leff * coil * coil * (1.f - ss));

            const glm::vec3 dS   = kgVSafe(startTip - shoulder, right);
            const float     lenS = glm::length(startTip - shoulder);

            auto tipAt = [&](float k) -> glm::vec3 {
                float kc = std::clamp(k, 0.f, 1.f);
                if (!h.isSlap) {
                    // выпад по дуге: сверху (пикирование) или снизу (подъём)
                    glm::vec3 p = kgMix(startTip, strikeTip, kc);
                    p += up * (sty.lift * Leff * sinf(KPI * kc));
                    return p;
                }
                // хлёст: дуга вокруг плеча, k > 1 = проводка дальше цели
                glm::vec3 d = kgVSafe(kgMix(dS, dirT, k), dirT);
                float len = kgLerp(lenS, reach, kc);
                return shoulder + d * len;
            };

            const glm::vec3 bendW = loc(sty.bend);
            auto arcPose = [&](const glm::vec3& tip, float bulgeMul, glm::vec3& c1, glm::vec3& c2) {
                glm::vec3 d = tip - shoulder;
                float c = glm::length(d);
                glm::vec3 dn = c > 1e-4f ? d / c : fwdAim;
                float extra = std::max(Leff * 0.98f - c, 0.f) * bulgeMul;
                float hh = sqrtf(std::max(3.f * std::max(c, 0.05f) * extra / 8.f, 0.f));
                hh = std::min(std::max(hh, 0.06f * c + 0.03f), Leff * 0.7f);
                glm::vec3 bd = bendW - dn * glm::dot(bendW, dn);
                bd = kgVSafe(bd, right);
                c1 = shoulder + d * 0.28f + bd * (hh * 1.45f);
                c2 = shoulder + d * 0.72f + bd * (hh * 0.95f);
            };

            const float slapExtra = h.isSlap ? 0.28f * ft : 0.f;
            const float ssLag     = h.isSlap ? powf(ss, 2.2f) : powf(ss, 1.5f);
            const float bulge     = kgLerp(1.f, h.isSlap ? 0.35f : 0.25f, ss);
            glm::vec3 tipA = tipAt(ss + slapExtra);
            glm::vec3 tipB = tipAt(ssLag + slapExtra);

            // дрожь жала в цели
            if (ss > 0.97f)
                tipA += (right * sinf(iT * 90.f + float(ti)) + up * cosf(iT * 97.f)) * (0.012f * Leff * (1.f - ft));

            glm::vec3 a1, a2, b1, b2;
            arcPose(tipA, bulge, a1, a2);
            arcPose(tipB, bulge, b1, b2);
            glm::vec3 c1 = kgMix(a1, b1, 0.4f);
            glm::vec3 c2 = b2;                     // середина отстаёт -> хлёст

            for (int i = 0; i < N; i++) {
                float u = float(i) / float(N - 1);
                glm::vec3 ag = kgBez(shoulder, c1, c2, tipA, u);
                goal[i] = kgMix(idle[i], ag, amt);
            }

            // вытянулось -> тоньше (сохраняем объём)
            float ratio = std::max(1.f, reach / std::max(Leff, 0.1f));
            thin = kgLerp(1.f, 1.f / sqrtf(ratio), ss * amt);

            // удар достиг цели
            if (ss >= 0.97f && !st.impactDone) {
                st.impactDone = true;
                st.hardenTime = NOW;
                spawnBurst(h.target, -dirT, std::max(3, int((8.f + rndU() * 8.f) * shakeScale)));
                triggerShake((h.isSlap ? 0.7f : 1.0f) * shakeScale);
            }
        } else {
            for (int i = 0; i < N; i++) goal[i] = idle[i];
        }

        // не проходим сквозь тело игрока: цель тоже выталкиваем наружу
        for (int i = 2; i < N; i++) kgPushOut(body, goal[i], colMargin);

        // ── Инициализация / телепорт ──────────────────────────────────────
        if (!st.inited || glm::length(st.pos[0] - goal[0]) > 3.f) {
            for (int i = 0; i < N; i++) { st.pos[i] = goal[i]; st.vel[i] = glm::vec3(0.f); }
            st.inited = true;
        }

        if (kick != 0.f)
            for (int i = 1; i < N; i++) st.vel[i].y += kick * (float(i) / float(N - 1));

        // ── Физика: пружина на узел ───────────────────────────────────────
        float kN[N], cN[N];
        for (int i = 0; i < N; i++) {
            float u = float(i) / float(N - 1);
            float k = kgLerp(kgLerp(kIdleRoot, kIdleTip, u), kgLerp(kAtkRoot, kAtkTip, u), amt);
            float z = kgLerp(kZetaIdle, kZetaAtk, amt);
            kN[i] = k;
            cN[i] = 2.f * z * sqrtf(k);
        }
        const int   sub = std::clamp((int)ceilf(dt / 0.008f), 1, 6);
        const float hs  = dt / float(sub);
        for (int s = 0; s < sub; s++) {
            st.pos[0] = goal[0];
            st.vel[0] = glm::vec3(0.f);
            for (int i = 1; i < N; i++) {
                glm::vec3 acc = (goal[i] - st.pos[i]) * kN[i] - st.vel[i] * cN[i];
                st.vel[i] += acc * hs;
                st.pos[i] += st.vel[i] * hs;
            }
        }
        const float maxDev = Leff * 0.55f;
        for (int i = 1; i < N; i++) {
            glm::vec3 dv = st.pos[i] - goal[i];
            float dl2 = glm::length(dv);
            if (dl2 > maxDev) st.pos[i] = goal[i] + dv * (maxDev / dl2);

            glm::vec3 gseg = goal[i] - goal[i - 1];
            float tl = glm::length(gseg);
            glm::vec3 d = st.pos[i] - st.pos[i - 1];
            float l = glm::length(d);
            if (l < 1e-5f) { d = gseg; l = std::max(tl, 1e-5f); }
            float lo = tl * 0.7f, hi = tl * 1.3f + 0.01f;
            if      (l > hi) st.pos[i] = st.pos[i - 1] + d * (hi / l);
            else if (l < lo) st.pos[i] = st.pos[i - 1] + d * (lo / l);
        }
        // финальная коллизия с телом (после пружин, которые могли занести узел внутрь)
        for (int i = 2; i < N; i++) kgPushOut(body, st.pos[i], colMargin);

        for (int i = 0; i < N; i++) {
            if (!std::isfinite(st.pos[i].x + st.pos[i].y + st.pos[i].z)) {
                for (int j = 0; j < N; j++) { st.pos[j] = goal[j]; st.vel[j] = glm::vec3(0.f); }
                break;
            }
        }

        if (detail && amt > 0.05f) st.pushTrail(st.pos[N - 1], iT);

        float he = kgMs(st.hardenTime) * 0.001f;
        float flash = (he >= 0.f && he < 0.35f) ? 1.f - he / 0.35f : 0.f;

        glm::vec3 dmid = st.pos[N / 2] - camPos;
        acts.push_back({ti, glm::dot(dmid, dmid), Reff * thin, flash});
    }

    // ═════════════════════════════════════════════════════════════════════
    //  Проход 2: отрисовка
    // ═════════════════════════════════════════════════════════════════════
    // сечение — суперэллипс (острые кромки, выпуклые грани)
    constexpr float kP = 1.5f;
    float cxk[16], syk[16], nxk[16], nyk[16], cs[16], sn[16];
    for (int k = 0; k < res; k++) {
        float a = K2PI * float(k) / float(res);
        float c = cosf(a), s = sinf(a);
        cs[k] = c; sn[k] = s;
        float sgc = c < 0.f ? -1.f : 1.f, sgs = s < 0.f ? -1.f : 1.f;
        cxk[k] = sgc * powf(fabsf(c), 2.f / kP);
        syk[k] = sgs * powf(fabsf(s), 2.f / kP);
        nxk[k] = (cxk[k] < 0.f ? -1.f : 1.f) * powf(fabsf(cxk[k]), kP - 1.f);
        nyk[k] = (syk[k] < 0.f ? -1.f : 1.f) * powf(fabsf(syk[k]), kP - 1.f);
    }

    auto drawTentacle = [&](const Act& ac)
    {
        const auto& st = mTentacles[ac.ti];
        const KMat km{cBlade, cShade, cGlow, iT, ac.flash, pulseK};

        // 0. след кончика (под телом щупальца)
        if (detail) {
            bool havePrev = false; glm::vec2 prevS(0.f);
            for (int i = 0; i < 24; i++) {
                const auto& tp = st.trail[(st.trailHead - 1 - i + 48) % 24];
                float age = iT - tp.t;
                if (age < 0.f || age > kTrailLife) break;
                glm::vec2 s;
                bool ok = mat.OWorldToScreen(camPos, tp.p, s, MathUtils::fov, guiRes);
                if (ok && havePrev) {
                    float f = 1.f - age / kTrailLife;
                    dl->AddLine(ImVec2(prevS.x, prevS.y), ImVec2(s.x, s.y),
                                kgCol(cGlow, f * f * 0.55f), 1.f + 4.5f * f);
                    dl->AddLine(ImVec2(prevS.x, prevS.y), ImVec2(s.x, s.y),
                                kgCol(glm::vec3(1.f, 0.78f, 0.62f), f * f * 0.50f), 0.8f);
                }
                havePrev = ok; prevS = s;
            }
        }

        // 1. спайн (Catmull-Rom по узлам физики)
        sSp.resize(M);
        for (int j = 0; j < M; j++) {
            float mp = float(j) / float(M - 1) * float(N - 1);
            int   i  = std::min((int)mp, N - 2);
            float lt = mp - float(i);
            glm::vec3 p0 = (i > 0)     ? st.pos[i - 1] : st.pos[0] * 2.f - st.pos[1];
            glm::vec3 p3 = (i + 2 < N) ? st.pos[i + 2] : st.pos[N - 1] * 2.f - st.pos[N - 2];
            sSp[j] = kgCR(p0, st.pos[i], st.pos[i + 1], p3, lt);
        }

        // 2. рамки колец: N — широкая ось лезвия, B — тонкая
        const float rW = ac.R * 1.7f * widthK;
        const float rT = ac.R * 0.42f;
        sR.resize(M);
        for (int j = 0; j < M; j++) {
            glm::vec3 d = (j == 0) ? sSp[1] - sSp[0]
                        : (j == M - 1) ? sSp[M - 1] - sSp[M - 2]
                        : sSp[j + 1] - sSp[j - 1];
            float u  = float(j) / float(M - 1);
            float br = 1.f + 0.03f * sinf(iT * 2.6f - u * 5.f);     // «дыхание»
            sR[j].T  = kgVSafe(d, fwdBody);
            sR[j].c  = sSp[j];
            sR[j].rw = rW * kgWidthProf(u) * br;
            sR[j].rt = rT * kgThickProf(u) * br;
        }
        {
            const glm::vec3& T0 = sR[0].T;
            glm::vec3 Nn = up - T0 * glm::dot(up, T0);
            if (glm::length(Nn) < 0.2f) Nn = right - T0 * glm::dot(right, T0);
            Nn = kgVSafe(Nn, right);
            for (int j = 0; j < M; j++) {
                const glm::vec3& T = sR[j].T;
                Nn = Nn - T * glm::dot(Nn, T);
                Nn = kgVSafe(Nn, kgVSafe(glm::cross(T, right), up));
                sR[j].N = Nn;
                sR[j].B = kgVSafe(glm::cross(T, Nn), up);
            }
        }

        // 3. вершины
        auto fin = [&](KVert& v, float u, float ang) {
            glm::vec3 V = kgVSafe(camPos - v.w);
            v.face = glm::dot(v.n, V) > 0.f;
            v.col  = ImGui::ColorConvertFloat4ToU32(kgShade(km, v.n, V, u, ang));
            glm::vec2 s;
            v.ok = mat.OWorldToScreen(camPos, v.w, s, MathUtils::fov, guiRes);
            if (v.ok) {
                v.s = ImVec2(s.x, s.y);
                glm::vec3 dv = v.w - camPos;
                if (glm::dot(dv, dv) < 0.05f) v.ok = false;          // слишком близко к камере
            }
            v.occ = v.ok && kgOccluded(body, camPos, v.w);
        };

        const int capBase = M * res;
        sV.assign(capBase + res + 1, KVert{});
        sC.assign(M, KVert{});

        for (int j = 0; j < M; j++) {
            const KRing& R = sR[j];
            float u  = float(j) / float(M - 1);
            int   jp = std::max(j - 1, 0), jn = std::min(j + 1, M - 1);
            float ds = glm::length(sSp[jn] - sSp[jp]);
            float slopeW = ds > 1e-6f ? (sR[jn].rw - sR[jp].rw) / ds : 0.f;
            float slopeT = ds > 1e-6f ? (sR[jn].rt - sR[jp].rt) / ds : 0.f;
            float rwS = std::max(R.rw, 1e-4f), rtS = std::max(R.rt, 1e-4f);

            for (int k = 0; k < res; k++) {
                float a2 = nxk[k] / rwS, b2 = nyk[k] / rtS;
                float l2 = sqrtf(a2 * a2 + b2 * b2);
                if (l2 < 1e-9f) { a2 = 1.f; b2 = 0.f; l2 = 1.f; }
                a2 /= l2; b2 /= l2;
                float tcomp = a2 * slopeW * cxk[k] + b2 * slopeT * syk[k];
                glm::vec3 nrm = R.N * a2 + R.B * b2;

                KVert& v = sV[j * res + k];
                v.w = R.c + R.N * (cxk[k] * R.rw) + R.B * (syk[k] * R.rt);
                v.n = kgVSafe(nrm - R.T * tcomp, nrm);
                fin(v, u, K2PI * float(k) / float(res));
            }
            sC[j].w = R.c;
            sC[j].n = R.N;
            fin(sC[j], u, 0.f);
        }
        // торец у корня
        {
            const KRing& R0 = sR[0];
            glm::vec3 capN = -R0.T;
            for (int k = 0; k < res; k++) {
                KVert& v = sV[capBase + k];
                v.w = R0.c + R0.N * (cxk[k] * R0.rw) + R0.B * (syk[k] * R0.rt);
                v.n = capN;
                fin(v, 0.f, 0.f);
            }
            KVert& cv = sV[capBase + res];
            cv.w = R0.c; cv.n = capN;
            fin(cv, 0.f, 0.f);
        }

        // 4. треугольники
        sT.clear();
        auto addTri = [&](int a, int b, int c) {
            const KVert &A = sV[a], &B = sV[b], &C = sV[c];
            if (!(A.ok && B.ok && C.ok)) return;
            if ((int)A.occ + (int)B.occ + (int)C.occ >= 2) return;
            glm::vec3 cen = (A.w + B.w + C.w) * (1.f / 3.f);
            glm::vec3 nn  = A.n + B.n + C.n;
            if (glm::dot(nn, camPos - cen) <= 0.f) return;           // back-face
            glm::vec3 dv = cen - camPos;
            sT.push_back({a, b, c, glm::dot(dv, dv)});
        };
        for (int j = 0; j < M - 1; j++) {
            for (int k = 0; k < res; k++) {
                int nk  = (k + 1) % res;
                int i00 = j * res + k,       i01 = j * res + nk;
                int i10 = (j + 1) * res + k, i11 = (j + 1) * res + nk;
                addTri(i00, i10, i01);
                addTri(i01, i10, i11);
            }
        }
        for (int k = 0; k < res; k++)
            addTri(capBase + res, capBase + k, capBase + (k + 1) % res);

        // 5. свечение + тушевой контур по силуэту (под мешем)
        if (mOutline.mValue) {
            const ImU32 olCol = IM_COL32(6, 0, 2, 235);
            const float ga    = 0.06f + 0.09f * beatNow * pulseK + 0.25f * ac.flash;
            const ImU32 glWide = kgCol(cGlow, ga * 0.6f);
            const ImU32 glMid  = kgCol(cGlow, ga);
            sOL.clear(); sOR.clear();
            auto flush = [&]() {
                if (sOL.size() >= 2) {
                    dl->AddPolyline(sOL.data(), (int)sOL.size(), glWide, 0, 12.f);
                    dl->AddPolyline(sOR.data(), (int)sOR.size(), glWide, 0, 12.f);
                    dl->AddPolyline(sOL.data(), (int)sOL.size(), glMid,  0, 6.f);
                    dl->AddPolyline(sOR.data(), (int)sOR.size(), glMid,  0, 6.f);
                    dl->AddPolyline(sOL.data(), (int)sOL.size(), olCol,  0, 2.8f);
                    dl->AddPolyline(sOR.data(), (int)sOR.size(), olCol,  0, 2.8f);
                }
                sOL.clear(); sOR.clear();
            };
            for (int j = 0; j < M; j++) {
                const KVert& C = sC[j];
                bool good = C.ok && !C.occ;
                if (good)
                    for (int k = 0; k < res; k++)
                        if (!sV[j * res + k].ok) { good = false; break; }
                if (!good) { flush(); continue; }

                const KVert& Cn = sC[std::min(j + 1, M - 1)].ok ? sC[std::min(j + 1, M - 1)] : C;
                const KVert& Cp = sC[std::max(j - 1, 0)].ok     ? sC[std::max(j - 1, 0)]     : C;
                float dx = Cn.s.x - Cp.s.x, dy = Cn.s.y - Cp.s.y;
                float l  = sqrtf(dx * dx + dy * dy);
                if (l < 1e-3f) { dx = 0.f; dy = 1.f; l = 1.f; }
                float px = -dy / l, py = dx / l;

                float mn = FLT_MAX, mx = -FLT_MAX;
                ImVec2 pmn = C.s, pmx = C.s;
                for (int k = 0; k < res; k++) {
                    const ImVec2& s = sV[j * res + k].s;
                    float pr = (s.x - C.s.x) * px + (s.y - C.s.y) * py;
                    if (pr < mn) { mn = pr; pmn = s; }
                    if (pr > mx) { mx = pr; pmx = s; }
                }
                sOL.push_back(pmn);
                sOR.push_back(pmx);
            }
            flush();
        }

        // 6. сортировка и вывод меша с per-vertex цветом
        if (!sT.empty()) {
            std::sort(sT.begin(), sT.end(),
                [](const KTriRef& a, const KTriRef& b) { return a.depth > b.depth; });

            const ImVec2 uv = dl->_Data->TexUvWhitePixel;
            const int nT = (int)sT.size();
            dl->PrimReserve(nT * 3, nT * 3);
            for (const auto& t : sT) {
                const KVert& A = sV[t.a];
                const KVert& B = sV[t.b];
                const KVert& C = sV[t.c];
                dl->PrimVtx(A.s, uv, A.col);
                dl->PrimVtx(B.s, uv, B.col);
                dl->PrimVtx(C.s, uv, C.col);
            }
        }

        // 7. детали поверх меша: чешуя, пластины, жилы
        if (!detail) return;

        auto vis = [&](const KVert& v) { return v.ok && !v.occ && v.face; };

        // ломаная по видимой части кольца j
        auto ringLine = [&](int j, ImU32 col, float th) {
            ImVec2 pts[17]; int n = 0;
            int start = 0;
            for (int k = 0; k < res; k++)
                if (!vis(sV[j * res + k])) { start = k; break; }
            for (int q = 0; q <= res; q++) {
                int k = (start + q) % res;
                const KVert& v = sV[j * res + k];
                if (vis(v)) { if (n < 17) pts[n++] = v.s; }
                else {
                    if (n >= 2) dl->AddPolyline(pts, n, col, 0, th);
                    n = 0;
                }
            }
            if (n >= 2) dl->AddPolyline(pts, n, col, 0, th);
        };

        // 7a. решётка чешуи у корня (две спирали навстречу друг другу -> ромбы)
        {
            const int jMaxL = std::max(3, int(float(M) * 0.5f));
            for (int dir = -1; dir <= 1; dir += 2) {
                for (int p = 0; p < res; p += 2) {
                    for (int j = 1; j < jMaxL; j++) {
                        float u  = float(j) / float(M - 1);
                        float al = 0.36f * (1.f - kgSm(0.12f, 0.50f, u));
                        if (al < 0.02f) break;
                        int k0 = (((p + dir * (j - 1)) % res) + res) % res;
                        int k1 = (((p + dir * j) % res) + res) % res;
                        const KVert& A = sV[(j - 1) * res + k0];
                        const KVert& B = sV[j * res + k1];
                        if (!vis(A) || !vis(B)) continue;
                        dl->AddLine(A.s, B.s, kgCol(cShade * 0.35f, al), 1.0f);
                    }
                }
            }
        }

        // 7b. рёбра-пластины с «фаской»
        {
            const int ribStep = std::max(2, M / 12);
            for (int j = 2; j < M - 2; j += ribStep) {
                float u  = float(j) / float(M - 1);
                float al = 0.55f * (1.f - kgSm(0.60f, 0.95f, u));
                if (al < 0.03f) continue;
                ringLine(j,     kgCol(cShade * 0.25f, al), 1.6f - 0.8f * u);
                ringLine(j + 1, kgCol(kgMix(cBlade, cGlow, 0.5f), al * 0.40f), 1.0f);
            }
        }

        // 7c. светящиеся жилы с бегущей энергией
        {
            const int kTop = res / 4, kBot = (3 * res) / 4;
            auto faceK = [&](int j) -> int {
                const KVert& a = sV[j * res + kTop];
                const KVert& b = sV[j * res + kBot];
                float da = glm::dot(a.n, kgVSafe(camPos - a.w));
                float db = glm::dot(b.n, kgVSafe(camPos - b.w));
                return da >= db ? kTop : kBot;
            };
            static const int offs[3] = {0, 1, -1};
            for (int vi = 0; vi < 3; vi++) {
                for (int j = 0; j < M - 1; j++) {
                    int kA = faceK(j), kB = faceK(j + 1);
                    if (kA != kB) continue;
                    int kk = (kA + offs[vi] + res) % res;
                    const KVert& A = sV[j * res + kk];
                    const KVert& B = sV[(j + 1) * res + kk];
                    if (!vis(A) || !vis(B)) continue;

                    float u    = (float(j) + 0.5f) / float(M - 1);
                    float e    = powf(0.5f + 0.5f * sinf(u * 9.f - iT * 3.6f + float(vi) * 1.9f), 3.f);
                    float fade = kgSm(0.02f, 0.12f, u) * (1.f - kgSm(0.78f, 1.f, u));
                    float wgt  = (vi == 0) ? 1.f : 0.55f;
                    float al   = (0.10f + 0.55f * e * pulseK) * fade * wgt;
                    if (al < 0.01f) continue;
                    dl->AddLine(A.s, B.s, kgCol(cGlow, al),
                                ((vi == 0) ? 1.4f : 0.9f) + 1.4f * e * pulseK);
                    if (vi == 0)
                        dl->AddLine(A.s, B.s, kgCol(glm::vec3(1.f, 0.82f, 0.65f), e * pulseK * 0.35f * fade), 0.8f);
                }
            }
        }
    };

    std::sort(acts.begin(), acts.end(),
        [](const Act& a, const Act& b) { return a.depth > b.depth; });
    for (const auto& ac : acts) drawTentacle(ac);

    // ═════════════════════════════════════════════════════════════════════
    //  Частицы крови
    // ═════════════════════════════════════════════════════════════════════
    if (mParticles.size() > 300)
        mParticles.erase(mParticles.begin(), mParticles.begin() + (int)(mParticles.size() - 300));

    for (auto it = mParticles.begin(); it != mParticles.end(); ) {
        it->life -= dt;
        if (it->life <= 0.f) { it = mParticles.erase(it); continue; }
        it->pos   += it->vel * dt;
        it->vel.y -= dt * 9.8f;
        it->vel   *= 0.97f;
        float a = it->life / it->maxLife;

        glm::vec2 sc2;
        if (mat.OWorldToScreen(camPos, it->pos, sc2, MathUtils::fov, guiRes)) {
            float sz = it->size * a, half = sz * 0.5f;
            dl->AddRectFilled({sc2.x - half, sc2.y - half}, {sc2.x + half, sc2.y + half},
                ImColor(it->col.Value.x, it->col.Value.y, it->col.Value.z, a));
            dl->AddCircleFilled({sc2.x, sc2.y}, sz * 1.8f, ImColor(1.f, 0.15f, 0.08f, a * 0.12f), 8);

            glm::vec3 prevP = it->pos - it->vel * dt * 2.5f;
            glm::vec2 prevSc;
            if (mat.OWorldToScreen(camPos, prevP, prevSc, MathUtils::fov, guiRes))
                dl->AddLine({prevSc.x, prevSc.y}, {sc2.x, sc2.y},
                    ImColor(it->col.Value.x, it->col.Value.y, it->col.Value.z, a * 0.45f), sz * 0.4f);
        }
        ++it;
    }
}
