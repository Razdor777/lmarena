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

// Длина кубической Безье (ломаной по 24 точкам).
static float kgBezLen(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
{
    float L = 0.f; glm::vec3 prev = a;
    for (int i = 1; i <= 24; i++) {
        glm::vec3 p = kgBez(a, b, c, d, float(i) / 24.f);
        L += glm::length(p - prev); prev = p;
    }
    return L;
}

// Щупальце не сжимается само по себе: кривая короче, чем нужно, — третья
// опорная точка уходит вдоль bend, пока длина не станет нужной (выгиб, а не
// гармошка). Длиннее — оставляем как есть (это вытягивание на выпаде).
static void kgFitBend(const glm::vec3& a, const glm::vec3& b, glm::vec3& c, const glm::vec3& d,
                      const glm::vec3& bend, float wantLen)
{
    if (kgBezLen(a, b, c, d) >= wantLen) return;
    const glm::vec3 c0 = c;
    float lo = 0.f, hi = wantLen * 1.5f;
    for (int it = 0; it < 8; it++) {
        const float mid = 0.5f * (lo + hi);
        if (kgBezLen(a, b, c0 + bend * mid, d) < wantLen) lo = mid; else hi = mid;
    }
    c = c0 + bend * (0.5f * (lo + hi));
}

// n узлов на равном расстоянии ВДОЛЬ кривой: «кости» одинаковой длины, без
// сгустков у изгибов и растянутых участков.
static void kgResample(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
                       glm::vec3* out, int n)
{
    constexpr int S = 48;
    glm::vec3 pts[S + 1];
    float cum[S + 1];
    pts[0] = a; cum[0] = 0.f;
    for (int i = 1; i <= S; i++) {
        pts[i] = kgBez(a, b, c, d, float(i) / float(S));
        cum[i] = cum[i - 1] + glm::length(pts[i] - pts[i - 1]);
    }
    const float total = std::max(cum[S], 1e-5f);
    int seg = 0;
    for (int k = 0; k < n; k++) {
        const float want = total * float(k) / float(n - 1);
        while (seg < S - 1 && cum[seg + 1] < want) seg++;
        const float l0 = cum[seg], l1 = cum[seg + 1];
        const float t = l1 > l0 ? (want - l0) / (l1 - l0) : 0.f;
        out[k] = kgMix(pts[seg], pts[seg + 1], std::clamp(t, 0.f, 1.f));
    }
}

// Плавная интерполяция направлений (единичных векторов).
static glm::vec3 kgSlerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    float d = std::clamp(glm::dot(a, b), -1.f, 1.f);
    if (d > 0.9995f) return kgVSafe(kgMix(a, b, t), a);
    float th = acosf(d) * t;
    glm::vec3 o = kgVSafe(b - a * d, glm::vec3(0.f, 1.f, 0.f));
    return a * cosf(th) + o * sinf(th);
}

// Выход с небольшим перелётом (пружинистое возвращение в стойку).
static inline float kgBackOut(float t)
{
    t = std::clamp(t, 0.f, 1.f) - 1.f;
    constexpr float c1 = 1.4f, c3 = c1 + 1.f;
    return 1.f + c3 * t * t * t + c1 * t * t;
}
static inline ImU32 kgCol(const glm::vec3& c, float a)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, a));
}

// ═════════════════════════════════════════════════════════════════════════════
//  Палитры
// ═════════════════════════════════════════════════════════════════════════════
struct KPal { glm::vec3 blade, shade, glow; };
static const KPal kPals[3] = {
    { {0.52f, 0.03f, 0.05f}, {0.08f, 0.00f, 0.01f}, {1.00f, 0.26f, 0.16f} },   // Crimson (тёмная плоть, яркая кромка)
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
    float bands = 0.f;   // >0: число ступеней освещения (Minecraft)
};

static ImVec4 kgShade(const KMat& m, const glm::vec3& n, const glm::vec3& V, float u, float ang)
{
    const glm::vec3 Ld = -kKL;
    float d = glm::dot(n, Ld) * 0.5f + 0.5f;
    if (m.bands > 0.f) d = std::floor(d * m.bands) / m.bands;   // плоские полосы света

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

// Фазы атаки (доли длительности), как в аниме-ударе:
//   замах (взвод над плечом, дрожь напряжения) -> выпад (взрывной, почти по
//   прямой, насквозь) -> стоп-кадр в цели (вспышка, кровь, тряска) -> рывок
//   обратно и пружинистый возврат в стойку.
static constexpr float kPA = 0.34f;   // конец замаха = начало выпада
static constexpr float kPS = 0.43f;   // удар (кончик в цели)
static constexpr float kPH = 0.55f;   // конец стоп-кадра
static constexpr float kGhostLife = 0.09f; // сек, «послеобраз» выпада

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
    mClosing = false;   // включили обратно во время втягивания — просто отменяем его
    if (!mListening) {
        gFeatureManager->mDispatcher->listen<RenderEvent,    &Kagune::onRenderEvent   >(this);
        gFeatureManager->mDispatcher->listen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);
        gFeatureManager->mDispatcher->listen<LookInputEvent, &Kagune::onLookInputEvent>(this);
        mListening = true;
    }

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
    // Exit Anim: не отписываемся сразу — рендер доигрывает втягивание и сам отпишется
    if (mExitAnim.mValue && mListening && mVis > 0.f) {
        mClosing   = true;
        mWantShown = false;
        return;
    }
    unlisten();
}

void Kagune::unlisten()
{
    if (!mListening) return;
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &Kagune::onRenderEvent   >(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Kagune::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &Kagune::onLookInputEvent>(this);
    mListening = false;
    mClosing   = false;
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
    if (mClosing || !event.mPacket) return;
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
    // втягивание закончено (или игрока нет) — отписываемся от рендера
    if (mClosing && (mVis <= 0.f || !ci->getLocalPlayer())) { unlisten(); return; }
    auto* opts = ci->getOptions();
    if (!opts) return;

    const int  camMode = opts->mThirdPerson->value;
    const bool isFPV   = (camMode == 0);
    if (!mShowFPV.mValue && isFPV) { if (mClosing) mVis = 0.f; return; }

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
    // 3-е лицо: корень на доле роста + ручная поправка «Height Offset»
    const float kRootH = pH * Kagune::kRootFrac + mYOffset.mValue;

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
    const bool  blocky  = mStyle.mValue == Style::Minecraft;
    const float fpvK    = isFPV ? mFpvSize.mValue : 1.f;   // FPV Size: масштаб от первого лица
    const int   cnt     = std::clamp((int)mCount.mValue,    1, kMax);
    const float maxLen  = mLength.mValue * fpvK;
    const float baseRad = mThickness.mValue * 0.02f * fpvK;   // толщина: тело, а не верёвка
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
        return np > kPH + 0.1f;                 // уже возвращается — можно перебить
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

        // ── Атака: фаза ───────────────────────────────────────────────────
        if (st.hasActive && kgMs(st.cur.hitTime) * 0.001f >= st.cur.dur) st.hasActive = false;

        float np = 1.f, amt = 0.f, kin = 0.f;
        if (st.hasActive) {
            np = std::max(kgMs(st.cur.hitTime) * 0.001f / st.cur.dur, 0.f);
            // вес позы атаки: быстро набирается в начале замаха, спадает на возврате
            if (np < kPH) amt = kgSS(np / (kPA * 0.6f));
            else          amt = 1.f - kgSS3((np - kPH) / (1.f - kPH));
            // выпад и стоп-кадр — узлы прямо в позу (пружины отстали бы и
            // размазали удар); замах — наполовину; на возврате снова пружины
            if (np >= kPA && np < kPH) kin = 1.f;
            else if (np >= kPH)        kin = 1.f - kgSS((np - kPH) / ((1.f - kPH) * 0.35f));
            else                       kin = 0.55f * kgSS(np / kPA);
        }
        st.smear = 0.f;

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
        // Скелет щупальца в ударе — одна гладкая кривая от корня до кончика:
        // корень выходит из спины туда же, куда в стойке, «локоть» идёт над
        // плечом, кончик — по траектории удара. Длина кривой подгоняется под
        // длину щупальца (выгиб вместо гармошки), узлы ставятся на равном
        // расстоянии вдоль неё — кости одинаковые, без изломов и провалов
        // сквозь тело.
        glm::vec3 goal[N];
        float thin = 1.f;
        if (amt > 0.001f) {
            const HitRecord& h = st.cur;
            const glm::vec3 toT  = h.target - shoulder;
            const float     dist = glm::length(toT);
            const glm::vec3 dirT = dist > 1e-4f ? toT / dist : fwdAim;
            const float maxReach = Leff * kMaxStretch;
            const glm::vec3 idleTip = idle[N - 1];
            const glm::vec3 emerge  = kgVSafe(loc(sty.c1), up);
            // дрожь напряжения в конце замаха и вибрация в цели
            const glm::vec3 shakeV = right * sinf(iT * 61.f + float(ti) * 1.7f) + up * cosf(iT * 73.f + float(ti));

            glm::vec3 tip, elbow;
            float wantLen = Leff;
            bool  hitNow  = false;

            if (!h.isSlap) {
                // ── Укол: взвод над плечом -> выстрел насквозь ──
                // Взвод — «хвост скорпиона»: верхние щупальца встают высоко
                // над плечом, нижние разводятся в стороны; кончик смотрит
                // вперёд, локоть уходит назад — тело изгибается буквой S.
                const float hiK = std::max(sty.lift, 0.f), loK = std::max(-sty.lift, 0.f);
                const glm::vec3 cocked = shoulder + (up * (0.55f + 0.95f * hiK)
                                                   + right * (side * (0.30f + 0.9f * loK))
                                                   + fwdBody * 0.10f + h.jit * 0.25f) * Leff;
                const float pierce = std::min(0.35f * Leff, 0.55f);
                glm::vec3 hitP = h.target + dirT * pierce;
                if (glm::length(hitP - shoulder) > maxReach)
                    hitP = shoulder + kgVSafe(hitP - shoulder, dirT) * maxReach;
                const float reachLen = glm::length(hitP - shoulder);
                const glm::vec3 elbowCocked = shoulder + (up * (0.45f + 0.5f * hiK) - fwdBody * 0.42f
                                                        + right * (side * (0.18f + 0.4f * loK))) * Leff;
                const glm::vec3 elbowStrike = shoulder + up * (0.18f * Leff) + dirT * (0.45f * reachLen);
                float e = 0.f; // 0 — взведено, 1 — в цели
                if (np < kPA) {
                    const float a = kgSS(np / kPA);
                    tip = kgMix(idleTip, cocked, 1.f - (1.f - a) * (1.f - a))
                        + shakeV * (0.022f * Leff * kgSm(0.55f, 1.f, a));
                } else if (np < kPS) {
                    const float k = (np - kPA) / (kPS - kPA);
                    e = k * k;                     // взрывной разгон: в цель — на полной скорости
                    tip = kgMix(cocked, hitP, e);
                    st.smear = 1.f;
                    st.strikeDir = kgVSafe(hitP - cocked, dirT);
                } else if (np < kPH) {
                    e = 1.f;
                    const float hold = (np - kPS) / (kPH - kPS);
                    tip = hitP + shakeV * (0.016f * Leff * (1.f - hold));
                } else {
                    e = 1.f;
                    const float r = (np - kPH) / (1.f - kPH);
                    tip = hitP - dirT * (0.32f * Leff * kgSS(r / 0.22f));   // рывок назад из цели
                }
                elbow   = kgMix(elbowCocked, elbowStrike, e);
                wantLen = kgLerp(Leff, std::max(Leff, reachLen * 1.06f), e);
                hitNow  = np >= kPS;
            } else {
                // ── Хлёст: широкий замах вбок -> дуга сквозь цель и дальше ──
                const glm::vec3 start = shoulder + loc(sty.slap + glm::vec3(0.25f, 0.12f, -0.25f) + h.jit * 0.5f) * Leff;
                const glm::vec3 d0 = kgVSafe(start - shoulder, right * side);
                const glm::vec3 d1 = kgVSafe(dirT + (dirT - d0) * 0.65f, dirT);   // проводка за цель
                const float r0 = glm::length(start - shoulder);
                const float r1 = std::clamp(dist + 0.15f, 0.4f, maxReach);
                constexpr float kMid = 0.62f;                                     // где кончик проходит цель
                auto sweep = [&](float k) {
                    return k < kMid ? kgSlerp(d0, dirT, k / kMid) : kgSlerp(dirT, d1, (k - kMid) / (1.f - kMid));
                };
                const float kEnd = kPS + 0.07f;
                float rr = r0;
                if (np < kPA) {
                    const float a = kgSS(np / kPA);
                    tip = kgMix(idleTip, start, 1.f - (1.f - a) * (1.f - a))
                        + shakeV * (0.02f * Leff * kgSm(0.55f, 1.f, a));
                    elbow = shoulder + kgSlerp(emerge, d0, 0.6f) * (0.45f * Leff) + up * (0.25f * Leff);
                } else {
                    const float k = np < kEnd ? kgSS((np - kPA) / (kEnd - kPA)) : 1.f;
                    if (np < kEnd) {
                        st.smear = 1.f;
                        st.strikeDir = kgVSafe(sweep(std::min(k + 0.05f, 1.f)) - sweep(k), dirT);
                    }
                    rr = kgLerp(r0, r1, kgSS(std::min(k / kMid, 1.f)));
                    tip = shoulder + sweep(k) * rr;
                    // середина отстаёт от кончика — по щупальцу бежит волна хлыста
                    elbow = shoulder + sweep(k * 0.5f) * (0.5f * rr) + up * (0.2f * Leff);
                    if (np >= kEnd) {
                        const float r = (np - kEnd) / std::max(1.f - kEnd, 0.01f);
                        tip -= dirT * (0.25f * Leff * kgSS(r / 0.3f));
                    }
                    hitNow = k >= kMid;
                }
                wantLen = std::max(Leff, rr * 1.05f);
            }

            glm::vec3 P1 = shoulder + emerge * (0.25f * wantLen);
            glm::vec3 P2 = elbow;
            const glm::vec3 chord = kgVSafe(tip - shoulder, dirT);
            glm::vec3 bend = up * 0.8f + right * (side * 0.45f);
            bend = kgVSafe(bend - chord * glm::dot(bend, chord), up);
            kgFitBend(shoulder, P1, P2, tip, bend, wantLen);

            glm::vec3 atk[N];
            kgResample(shoulder, P1, P2, tip, atk, N);
            for (int i = 0; i < N; i++) goal[i] = kgMix(idle[i], atk[i], amt);

            // вытянулось -> тоньше (объём сохраняется)
            const float curLen = kgBezLen(shoulder, P1, P2, tip);
            thin = kgLerp(1.f, 1.f / sqrtf(std::max(1.f, curLen / std::max(Leff, 0.1f))), amt);

            // удар: вспышка, кровь насквозь и обратно, тряска камеры
            if (hitNow && !st.impactDone) {
                st.impactDone = true;
                st.hardenTime = NOW;
                st.impactPos  = h.target;
                st.impactT    = iT;
                const int nB = std::max(3, int((8.f + rndU() * 8.f) * shakeScale));
                spawnBurst(h.target, dirT, nB);
                spawnBurst(h.target, kgVSafe(-dirT * 0.6f + up * 0.5f), std::max(2, nB / 2));
                triggerShake((h.isSlap ? 0.8f : 1.0f) * shakeScale);
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

        glm::vec3 prevP[N];
        for (int i = 0; i < N; i++) prevP[i] = st.pos[i];

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
        // Выпад/стоп-кадр: узлы — прямо в позу, скорость — по факту движения,
        // чтобы после отпускания пружины подхватили его без рывка.
        if (kin > 0.001f) {
            for (int i = 1; i < N; i++) {
                const glm::vec3 p = kgMix(st.pos[i], goal[i], kin);
                glm::vec3 v = (p - prevP[i]) / dt;
                const float vl = glm::length(v);
                if (vl > 60.f) v *= 60.f / vl;
                st.vel[i] = kgMix(st.vel[i], v, kin);
                st.pos[i] = p;
            }
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
        // послеобраз выпада: прошлые позы для смаза
        if (st.smear > 0.f && iT - st.lastGhostT > 0.012f) {
            st.ghost[st.ghostHead] = st.pos;
            st.ghostT[st.ghostHead] = iT;
            st.ghostHead = (st.ghostHead + 1) % (int)st.ghost.size();
            st.lastGhostT = iT;
        }

        float he = kgMs(st.hardenTime) * 0.001f;
        float flash = (he >= 0.f && he < 0.35f) ? 1.f - he / 0.35f : 0.f;

        glm::vec3 dmid = st.pos[N / 2] - camPos;
        acts.push_back({ti, glm::dot(dmid, dmid), Reff * thin, flash});
    }

    // ═════════════════════════════════════════════════════════════════════
    //  Проход 2: отрисовка
    // ═════════════════════════════════════════════════════════════════════
    // сечение — суперэллипс (острые кромки, выпуклые грани)
    const float kP = blocky ? 10.f : 1.5f;   // Minecraft: почти квадратное сечение
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
        const KMat km{cBlade, cShade, cGlow, iT, ac.flash, pulseK, blocky ? 3.f : 0.f};

        auto proj = [&](const glm::vec3& w, ImVec2& out) -> bool {
            glm::vec2 sp;
            if (!mat.OWorldToScreen(camPos, w, sp, MathUtils::fov, guiRes)) return false;
            out = ImVec2(sp.x, sp.y);
            return true;
        };

        // 0a. послеобраз выпада (смаз, как в аниме): прошлые позы —
        // полупрозрачные ленты по всему телу, самые свежие ярче
        for (int g = 0; g < (int)st.ghost.size(); g++) {
            const float age = iT - st.ghostT[g];
            if (age < 0.f || age > kGhostLife) continue;
            const float f = 1.f - age / kGhostLife;
            ImVec2 pts[kNodes];
            int n = 0;
            for (int i = 0; i < N; i++) {
                if (!proj(st.ghost[g][i], pts[n])) { n = 0; break; }
                n++;
            }
            if (n < 2) continue;
            ImVec2 a, b;
            float px = 6.f;
            if (proj(st.ghost[g][N / 2], a) && proj(st.ghost[g][N / 2] + right * ac.R, b))
                px = std::clamp(std::hypot(b.x - a.x, b.y - a.y), 1.5f, 40.f);
            dl->AddPolyline(pts, n, kgCol(kgMix(cBlade, cGlow, 0.35f), 0.30f * f * f), 0, px * 1.7f * (0.6f + 0.4f * f));
            dl->AddPolyline(pts, n, kgCol(cGlow, 0.40f * f * f), 0, std::max(1.f, px * 0.45f));
        }

        // 0b. линии скорости вдоль выпада — по тому, как кончик реально
        // движется по экрану (от самого старого послеобраза), а не по 3D
        // направлению: удар «в экран» иначе давал бы линии вбок.
        if (st.smear > 0.f && detail) {
            ImVec2 tipS, backS;
            int oldest = -1;
            float oldestAge = -1.f;
            for (int g = 0; g < (int)st.ghost.size(); g++) {
                const float age = iT - st.ghostT[g];
                if (age >= 0.f && age <= kGhostLife && age > oldestAge) { oldestAge = age; oldest = g; }
            }
            const glm::vec3 tipW = st.pos[N - 1];
            if (oldest >= 0 && proj(tipW, tipS) && proj(st.ghost[oldest][N - 1], backS)) {
                float dx = tipS.x - backS.x, dy = tipS.y - backS.y;
                const float l = std::hypot(dx, dy);
                if (l > 6.f) {
                    dx /= l; dy /= l;
                    const float qx = -dy, qy = dx;
                    for (int k = 0; k < 8; k++) {
                        const float o   = float(((k * 37 + ac.ti * 11) % 13) - 6) / 6.f;  // -1..1, не в линию
                        const float len = 45.f + float((k * 53 + ac.ti * 7) % 80);
                        const float off = 10.f + float(k) * 4.f;
                        const ImVec2 a0(tipS.x + qx * o * 26.f - dx * off, tipS.y + qy * o * 26.f - dy * off);
                        const ImVec2 a1(a0.x - dx * len, a0.y - dy * len);
                        const glm::vec3 lc = (k & 1) ? glm::vec3(1.f, 0.88f, 0.82f) : cGlow;
                        dl->AddLine(a0, a1, kgCol(lc, (k & 1) ? 0.55f : 0.40f), (k % 3 == 0) ? 2.0f : 1.2f);
                    }
                }
            }
        }

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
        // почти круглое сечение (было 1.7 / 0.42 — плоская лента, «бумага»)
        const float rW = ac.R * 1.30f * widthK;
        const float rT = ac.R * 1.05f;
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
        if (blocky) {
            // радиусы кратны шагу «блока» — силуэт собирается из ступеней
            const float step  = std::max(ac.R * 0.55f, 0.012f);
            const float stepT = step * 0.6f;
            for (int j = 0; j < M; j++) {
                sR[j].rw = std::max(step,  std::round(sR[j].rw / step)  * step);
                sR[j].rt = std::max(stepT, std::round(sR[j].rt / stepT) * stepT);
            }
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

        // 6b. костяные шипы (манга): треугольники от поверхности, сметены к кончику
        if (mSpikes.mValue) {
            const int spStep = std::max(2, M / 8);
            for (int j = 1; j < M - 2; j += spStep) {
                const KRing& R  = sR[j];
                const float  u  = float(j) / float(M - 1);
                const float  phi = float(j) * 2.39996f + float(ac.ti) * 1.3f;   // золотой угол: не в линию
                const glm::vec3 radial = R.N * cosf(phi) + R.B * sinf(phi);
                const glm::vec3 dirS   = kgVSafe(radial * 0.75f + R.T * (0.55f + 0.25f * (1.f - u)));
                const glm::vec3 spBase = R.c + radial * (R.rw * 0.5f);
                const glm::vec3 toCam  = kgVSafe(camPos - spBase);
                if (glm::dot(radial, toCam) < 0.15f) continue;         // обратная сторона
                if (kgOccluded(body, camPos, spBase)) continue;        // спрятан за телом игрока

                const float     len    = R.rw * (2.2f - 1.2f * u) * (blocky ? 1.25f : 1.f);
                const glm::vec3 spTip  = spBase + dirS * len;
                const glm::vec3 spSide = kgVSafe(glm::cross(dirS, radial), R.T) * (R.rw * (blocky ? 0.55f : 0.42f));

                glm::vec2 p0, p1, pt;
                if (!mat.OWorldToScreen(camPos, spBase + spSide, p0, MathUtils::fov, guiRes)) continue;
                if (!mat.OWorldToScreen(camPos, spBase - spSide, p1, MathUtils::fov, guiRes)) continue;
                if (!mat.OWorldToScreen(camPos, spTip,           pt, MathUtils::fov, guiRes)) continue;

                const ImVec2 a(p0.x, p0.y), b(p1.x, p1.y), c(pt.x, pt.y);
                const ImVec4 sc = kgShade(km, radial, toCam, u, phi);
                dl->AddTriangleFilled(a, b, c, ImGui::ColorConvertFloat4ToU32(sc));
                dl->AddTriangle(a, b, c, kgCol(cShade * 0.3f, 0.9f), 1.0f);
                // красная жила вдоль шипа: светится сильнее при ударе
                const ImVec2 mid((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                dl->AddLine(mid, c, kgCol(cGlow, 0.35f + 0.35f * ac.flash), 1.2f);
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
    //  Вспышка удара (аниме): звезда лучей + белое ядро + кольцо
    // ═════════════════════════════════════════════════════════════════════
    for (int ti = 0; ti < cnt; ti++) {
        const auto& st = mTentacles[ti];
        const float age = iT - st.impactT;
        if (age < 0.f || age > 0.16f) continue;
        glm::vec2 c2;
        if (!mat.OWorldToScreen(camPos, st.impactPos, c2, MathUtils::fov, guiRes)) continue;
        const ImVec2 c(c2.x, c2.y);
        const float f = age / 0.16f, grow = kgSS(std::min(f * 3.f, 1.f)), fade = 1.f - kgSS(f);
        const float R = (22.f + 40.f * grow) * (isFPV ? 1.2f : 1.f);
        const float rot = st.impactT * 7.f;
        for (int r = 0; r < 12; r++) {
            const float a = rot + float(r) * (K2PI / 12.f);
            const float len = R * ((r & 1) ? 0.55f : 1.f);
            const ImVec2 tipP(c.x + cosf(a) * len, c.y + sinf(a) * len);
            const ImVec2 b1(c.x + cosf(a + 0.16f) * len * 0.16f, c.y + sinf(a + 0.16f) * len * 0.16f);
            const ImVec2 b2(c.x + cosf(a - 0.16f) * len * 0.16f, c.y + sinf(a - 0.16f) * len * 0.16f);
            dl->AddTriangleFilled(b1, tipP, b2, kgCol((r & 1) ? glm::vec3(1.f, 0.92f, 0.85f) : cGlow, 0.85f * fade));
        }
        dl->AddCircleFilled(c, R * 0.2f * (1.f + grow), IM_COL32(255, 246, 236, (int)(235.f * fade)), 20);
        dl->AddCircle(c, R * (0.55f + 0.75f * grow), kgCol(cGlow, 0.55f * fade), 28, 2.2f);
    }

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
