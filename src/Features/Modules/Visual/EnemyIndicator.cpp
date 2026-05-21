#include "EnemyIndicator.hpp"
#include "HudEditor.hpp"
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <glm/glm.hpp>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

static char sEnemyIndId[] = "EnemyIndicator";
class EnemyIndElement : public HudElement {
public:
    EnemyIndElement() : HudElement(sEnemyIndId) {
        mVisible = true; mCentered = true; mAnchor = Anchor::Middle;
    }
};
static EnemyIndElement* gEnemyElem = nullptr;

void EnemyIndicator::onEnable() {
    gFeatureManager->mDispatcher->listen<RenderEvent, &EnemyIndicator::onRenderEvent>(this);
    if (!gEnemyElem) {
        gEnemyElem = new EnemyIndElement();
        gEnemyElem->mSize = {0.f, 0.f};
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gEnemyElem);
    }
    if (gEnemyElem) gEnemyElem->mVisible = true;
}

void EnemyIndicator::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &EnemyIndicator::onRenderEvent>(this);
    if (gEnemyElem) gEnemyElem->mVisible = false;
}

// ═══════════════════════════════════════════════════════════════════════════
// СТРЕЛКА — точная копия скриншота:
//   - Треугольный шеврон с вырезом посередине (как курсор мышки / стрелка навигатора)
//   - Цвет: розовый -> красный -> фиолетовый (анимированный)
//   - Острие смотрит НА противника (наружу от центра)
//   - Мягкое свечение вокруг
// ═══════════════════════════════════════════════════════════════════════════

// Одна точка в системе координат: cx,cy — центр, angle — направление (0=вверх), 
// lx,ly — локальные координаты (ось Y вверх)
static ImVec2 rotPt(float cx, float cy, float angle, float lx, float ly) {
    float c = cosf(angle), s = sinf(angle);
    return { cx + lx * c - ly * s, cy + lx * s + ly * c };
}

// Рисует красивую навигационную стрелку (как на скрине)
// angle: направление острия (0 = вверх экрана)
static void drawNavArrow(ImDrawList* dl, float cx, float cy, float angle,
                          float size, ImColor color, float glowStrength) {
    // Форма: острие смотрит В направлении angle
    // Шеврон с вырезом (как на скриншоте)
    float s = size;

    // Острие (tip) — в направлении angle
    ImVec2 tip    = rotPt(cx, cy, angle,  0.f,    -s);
    // Левый задний угол
    ImVec2 bLeft  = rotPt(cx, cy, angle, -s*0.65f, s*0.55f);
    // Правый задний угол
    ImVec2 bRight = rotPt(cx, cy, angle,  s*0.65f, s*0.55f);
    // Внутренний вырез (делает форму шеврона)
    ImVec2 inner  = rotPt(cx, cy, angle,  0.f,     s*0.15f);

    // Форма: tip -> bRight -> inner -> bLeft (квад = chevron)

    // === СВЕЧЕНИЕ (несколько слоёв полупрозрачных) ===
    if (glowStrength > 0.01f) {
        for (int gi = 3; gi >= 1; gi--) {
            float gs = s + gi * 3.5f * glowStrength;
            ImVec2 gTip    = rotPt(cx, cy, angle,  0.f,    -gs);
            ImVec2 gBLeft  = rotPt(cx, cy, angle, -gs*0.65f, gs*0.55f);
            ImVec2 gBRight = rotPt(cx, cy, angle,  gs*0.65f, gs*0.55f);
            ImVec2 gInner  = rotPt(cx, cy, angle,  0.f,     gs*0.15f);

            ImColor glow = color;
            glow.Value.w = color.Value.w * 0.12f * glowStrength * (4.f - gi) / 3.f;

            dl->AddQuadFilled(gTip, gBRight, gInner, gBLeft, glow);
        }
    }

    // === ОСНОВНАЯ ЗАЛИВКА ===
    dl->AddQuadFilled(tip, bRight, inner, bLeft, color);

    // === ОБВОДКА для чёткости ===
    ImColor outline = ImColor(1.f, 1.f, 1.f, color.Value.w * 0.45f);
    dl->AddQuad(tip, bRight, inner, bLeft, outline, 1.2f);

    // === НЕБОЛЬШОЙ БЛИК на острие ===
    ImVec2 hlEnd = rotPt(cx, cy, angle,  0.f, -s * 0.55f);
    ImColor hl = ImColor(1.f, 1.f, 1.f, color.Value.w * 0.35f);
    dl->AddLine(tip, hlEnd, hl, 1.5f);
}

// Анимированный цвет: розовый → красный → фиолетовый
// phase: 0..1, t: время
static ImColor getArrowColor(float t, float phase, float dist, float maxDist, bool colorByDist) {
    if (colorByDist) {
        // Близко = красный/розовый, далеко = фиолетовый
        float nd = std::clamp(dist / maxDist, 0.f, 1.f); // 0=близко, 1=далеко

        // Розовый (близко) → Красный (средне) → Фиолетовый (далеко)
        // HSV: 340° розовый, 0°/360° красный, 270° фиолетовый
        float hue;
        if (nd < 0.5f) {
            // Розовый → Красный: 340 → 360 (или 0)
            hue = (340.f + nd * 2.f * 20.f) / 360.f;
            if (hue > 1.f) hue -= 1.f;
        } else {
            // Красный → Фиолетовый: 0 → 270
            hue = (nd - 0.5f) * 2.f * 270.f / 360.f;
        }

        // Насыщенность и яркость
        float sat = 0.85f + 0.15f * (1.f - nd); // близко = ярче
        float val = 1.f;

        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(hue, sat, val, r, g, b);
        return ImColor(r, g, b, 1.f);
    } else {
        // Анимированный: плавный цикл розовый→красный→фиолетовый
        // phase сдвигает фазу для каждой стрелки (чтобы не все одинаковые)
        float hue = fmodf(t * 0.25f + phase, 1.f);
        // Ограничиваем диапазон: 270°..360°..340° (фиолетовый..красный..розовый)
        // Переводим hue в эту зону:
        // 0.75 = 270° фиолетовый, 1.0/0.0 = красный, 0.94 = розовый
        hue = 0.75f + hue * 0.25f;
        if (hue > 1.f) hue -= 1.f;

        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(hue, 0.9f, 1.f, r, g, b);
        return ImColor(r, g, b, 1.f);
    }
}

void EnemyIndicator::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto dl = ImGui::GetBackgroundDrawList();
    float now = (float)ImGui::GetTime();

    ImVec2 screenCenter;
    if (gEnemyElem && gEnemyElem->mVisible) {
        screenCenter = gEnemyElem->getPos();
        if (screenCenter.x == 0.f && screenCenter.y == 0.f) {
            auto disp = ImGui::GetIO().DisplaySize;
            screenCenter = {disp.x * 0.5f, disp.y * 0.5f};
        }
    } else {
        auto disp = ImGui::GetIO().DisplaySize;
        screenCenter = {disp.x * 0.5f, disp.y * 0.5f};
    }

    glm::vec3 myPos = *player->getPos();
    float myYaw = player->getActorRotationComponent()->mYaw;

    float maxRange = mRange.mValue;
    float ringR    = mRadius.mValue;
    float arrowSz  = mArrowSize.mValue;
    int   maxN     = (int)mMaxArrows.mValue;
    float opacity  = mOpacity.mValue;

    struct EnemyData { Actor* a; float dist; float screenAngle; };
    std::vector<EnemyData> enemies;

    for (auto* actor : ActorUtils::getActorList(true, true)) {
        if (actor == player) continue;
        glm::vec3 p = *actor->getPos();
        float d = glm::distance(myPos, p);
        if (d > maxRange) continue;

        float deltaX = p.x - myPos.x;
        float deltaZ = p.z - myPos.z;
        float worldYaw = atan2f(deltaZ, deltaX) * (180.f / (float)M_PI);
        float relativeYaw = worldYaw - myYaw - 90.f;
        float screenAngle = glm::radians(relativeYaw);

        enemies.push_back({actor, d, screenAngle});
    }

    std::sort(enemies.begin(), enemies.end(), [](const EnemyData& a, const EnemyData& b){
        return a.dist < b.dist;
    });
    if ((int)enemies.size() > maxN) enemies.resize(maxN);
    if (enemies.empty()) return;

    // Всего врагов — для счётчика
    int totalCount = (int)enemies.size();

    int i = 0;
    for (auto& en : enemies) {
        float angle = en.screenAngle;

        float ax = screenCenter.x + sinf(angle) * ringR;
        float ay = screenCenter.y - cosf(angle) * ringR;

        // Цвет стрелки
        ImColor arrowCol = getArrowColor(now, (float)i * 0.33f, en.dist, maxRange, mColorDist.mValue);

        // Пульсация при близком противнике
        float curSize = arrowSz;
        float finalAlpha = opacity;
        if (mPulseClose.mValue && en.dist < 10.f) {
            float pulse = 0.75f + 0.25f * sinf(now * 10.f + (float)i);
            finalAlpha  = std::min(finalAlpha * (0.8f + 0.4f * pulse), 1.f);
            curSize     = arrowSz * (0.9f + 0.15f * pulse);
        }
        arrowCol.Value.w = finalAlpha;

        // Рисуем стрелку
        float glowStr = mGlow.mValue ? 1.f : 0.f;
        drawNavArrow(dl, ax, ay, angle, curSize, arrowCol, glowStr);

        // Дистанция под стрелкой
        if (mShowDist.mValue) {
            char distBuf[32];
            snprintf(distBuf, sizeof(distBuf), "%.0fm", en.dist);
            ImVec2 ts = ImGui::CalcTextSize(distBuf);

            // Позиция текста: за стрелкой (снаружи кольца)
            float textR = ringR + curSize + 4.f;
            float tx = screenCenter.x + sinf(angle) * textR - ts.x * 0.5f;
            float ty = screenCenter.y - cosf(angle) * textR - ts.y * 0.5f;

            // Тень
            dl->AddText({tx + 1, ty + 1}, IM_COL32(0, 0, 0, (int)(180 * finalAlpha)), distBuf);
            ImColor tc = arrowCol;
            tc.Value.w = finalAlpha * 0.9f;
            dl->AddText({tx, ty}, tc, distBuf);
        }

        i++;
    }

    // Счётчик врагов в центре
    if (mShowCount.mValue && totalCount > 0) {
        char cntBuf[16];
        snprintf(cntBuf, sizeof(cntBuf), "%d", totalCount);
        ImVec2 ts = ImGui::CalcTextSize(cntBuf);
        float cx = screenCenter.x - ts.x * 0.5f;
        float cy = screenCenter.y - ts.y * 0.5f;

        // Анимированный цвет для счётчика
        ImColor cc = getArrowColor(now, 0.f, 0.f, maxRange, false);
        cc.Value.w = opacity;

        dl->AddText({cx + 1, cy + 1}, IM_COL32(0,0,0,160), cntBuf);
        dl->AddText({cx, cy}, cc, cntBuf);
    }
}
