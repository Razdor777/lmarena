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
#include <Utils/MiscUtils/ColorUtils.hpp>

// ─── HUD element ─────────────────────────────────────────────────────────────
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
    gEnemyElem->mVisible = true;
}

void EnemyIndicator::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &EnemyIndicator::onRenderEvent>(this);
    if (gEnemyElem) gEnemyElem->mVisible = false;
}

// ─── rotate point ────────────────────────────────────────────────────────────
static ImVec2 rotPt(float cx, float cy, float angle, float lx, float ly) {
    float c = cosf(angle), s = sinf(angle);
    return { cx + lx*c - ly*s, cy + lx*s + ly*c };
}

// ─── Draw: chevron (Mode Arrows) ─────────────────────────────────────────────
static void drawChevron(ImDrawList* dl, float cx, float cy, float angle,
                         float size, ImColor col, bool glow)
{
    float s = size;
    ImVec2 tip   = rotPt(cx, cy, angle,  0.f,      -s);
    ImVec2 bL    = rotPt(cx, cy, angle, -s*0.65f,   s*0.55f);
    ImVec2 bR    = rotPt(cx, cy, angle,  s*0.65f,   s*0.55f);
    ImVec2 inner = rotPt(cx, cy, angle,  0.f,        s*0.15f);

    if (glow) {
        for (int gi = 3; gi >= 1; gi--) {
            float gs  = s + gi * 3.f;
            ImVec2 gT = rotPt(cx, cy, angle, 0.f,      -gs);
            ImVec2 gL = rotPt(cx, cy, angle,-gs*0.65f,  gs*0.55f);
            ImVec2 gR = rotPt(cx, cy, angle, gs*0.65f,  gs*0.55f);
            ImVec2 gI = rotPt(cx, cy, angle, 0.f,       gs*0.15f);
            ImColor g = col; g.Value.w = col.Value.w * 0.1f * (4.f-gi)/3.f;
            dl->AddQuadFilled(gT, gR, gI, gL, g);
        }
    }
    dl->AddQuadFilled(tip, bR, inner, bL, col);
    dl->AddQuad(tip, bR, inner, bL, ImColor(1.f,1.f,1.f,col.Value.w*0.4f), 1.1f);
}

// ─── Draw: triangle on circle edge (Mode Radar) ───────────────────────────────
static void drawTriPointer(ImDrawList* dl, float cx, float cy, float angle,
                            float circleR, float ptrSize, ImColor col, bool glow)
{
    float px = cx + sinf(angle) * circleR;
    float py = cy - cosf(angle) * circleR;
    ImVec2 tip = rotPt(px, py, angle,  0.f,            -ptrSize * 0.85f);
    ImVec2 bL  = rotPt(px, py, angle, -ptrSize * 0.55f,  ptrSize * 0.5f);
    ImVec2 bR  = rotPt(px, py, angle,  ptrSize * 0.55f,  ptrSize * 0.5f);

    if (glow) {
        for (int gi = 2; gi >= 1; gi--) {
            float gs = ptrSize * (1.f + gi * 0.5f);
            ImVec2 gT = rotPt(px, py, angle,  0.f,          -gs * 0.85f);
            ImVec2 gL = rotPt(px, py, angle, -gs * 0.55f,    gs * 0.5f);
            ImVec2 gR = rotPt(px, py, angle,  gs * 0.55f,    gs * 0.5f);
            ImColor g = col; g.Value.w = col.Value.w * 0.15f;
            dl->AddTriangleFilled(gT, gL, gR, g);
        }
    }
    dl->AddTriangleFilled(tip, bL, bR, col);
    dl->AddTriangle(tip, bL, bR, ImColor(1.f,1.f,1.f, col.Value.w*0.45f), 1.f);
}

// ─── Draw: compass arrow (Mode Compass) ──────────────────────────────────────
// Рисует одну стрелку — точно как на скрине: маленькая белая стрелка выше центра
// angle = направление к врагу (уже повёрнута)
static void drawCompassArrow(ImDrawList* dl, float cx, float cy, float angle,
                              float size, ImColor col, bool glow)
{
    // Стрелка: острое тело + раздвоенный хвост (как на скрине)
    float s  = size;
    float s2 = s * 0.5f;

    // Острие
    ImVec2 tip  = rotPt(cx, cy, angle,  0.f,      -s);
    // Тело-бока
    ImVec2 mL   = rotPt(cx, cy, angle, -s2*0.45f,  s*0.1f);
    ImVec2 mR   = rotPt(cx, cy, angle,  s2*0.45f,  s*0.1f);
    // Хвост — раздвоен (V-форма снизу)
    ImVec2 tL   = rotPt(cx, cy, angle, -s2*0.7f,   s*0.85f);
    ImVec2 tR   = rotPt(cx, cy, angle,  s2*0.7f,   s*0.85f);
    ImVec2 tCen = rotPt(cx, cy, angle,  0.f,        s*0.45f); // выемка в центре хвоста

    if (glow) {
        // Мягкое свечение вокруг острия
        for (int gi = 3; gi >= 1; gi--) {
            float r = s * 0.35f * gi;
            ImColor g = col;
            g.Value.w = col.Value.w * 0.08f * (4.f - gi) / 3.f;
            dl->AddCircleFilled({tip.x, tip.y}, r, g, 12);
        }
    }

    // Основная форма: треугольник-тело + раздвоенный хвост
    // Тело (от острия до середины)
    dl->AddTriangleFilled(tip, mL, mR, col);
    // Хвост левый
    dl->AddTriangleFilled(mL, tL, tCen, col);
    // Хвост правый
    dl->AddTriangleFilled(mR, tCen, tR, col);

    // Белая обводка
    ImColor outline = ImColor(1.f, 1.f, 1.f, col.Value.w * 0.5f);
    // Контур тела
    dl->AddTriangle(tip, mL, mR, outline, 0.8f);
    // Контур хвоста
    dl->AddLine(mL, tL,   outline, 0.8f);
    dl->AddLine(mR, tR,   outline, 0.8f);
    dl->AddLine(tL, tCen, outline, 0.8f);
    dl->AddLine(tR, tCen, outline, 0.8f);
}

// ─── Main render ──────────────────────────────────────────────────────────────
void EnemyIndicator::onRenderEvent(RenderEvent& event) {
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto* dl  = ImGui::GetBackgroundDrawList();
    float now = (float)ImGui::GetTime();

    ImVec2 sc;
    if (gEnemyElem && gEnemyElem->mVisible) {
        sc = gEnemyElem->getPos();
        if (sc.x == 0.f && sc.y == 0.f) {
            auto d = ImGui::GetIO().DisplaySize;
            sc = {d.x * 0.5f, d.y * 0.5f};
        }
    } else {
        auto d = ImGui::GetIO().DisplaySize;
        sc = {d.x * 0.5f, d.y * 0.5f};
    }

    glm::vec3 myPos = *player->getPos();
    float myYaw = player->getActorRotationComponent()->mYaw;

    float maxRange = mRange.mValue;
    float op       = mOpacity.mValue;
    float R        = mRadius.mValue;
    float sz       = mSize.mValue;
    bool  showDist = mShowDist.mValue;
    bool  showCnt  = mShowCount.mValue;
    bool  glow     = mGlow.mValue;
    auto  mode     = mMode.mValue;

    // Собираем врагов
    struct Enemy { float dist; float angle; };
    std::vector<Enemy> enemies;

    for (auto* a : ActorUtils::getActorList(true, true)) {
        if (a == player) continue;
        glm::vec3 p = *a->getPos();
        float d = glm::distance(myPos, p);
        if (d > maxRange) continue;

        float dx = p.x - myPos.x, dz = p.z - myPos.z;
        float worldYaw = atan2f(dz, dx) * (180.f / (float)M_PI);
        float rel      = worldYaw - myYaw - 90.f;
        enemies.push_back({d, glm::radians(rel)});
    }

    std::sort(enemies.begin(), enemies.end(), [](const Enemy& a, const Enemy& b){
        return a.dist < b.dist;
    });

    if (mode != IndicatorMode::Compass) {
        int maxN = (int)mMaxCount.mValue;
        if ((int)enemies.size() > maxN) enemies.resize(maxN);
    } else {
        // Compass — только ближайший
        if (enemies.size() > 1) enemies.resize(1);
    }

    if (enemies.empty()) return;
    int total = (int)enemies.size();

    // ─────────────────────────────────────────────────────────────────────────
    // MODE 3: COMPASS — одна стрелка над перекрестием как на скрине
    // ─────────────────────────────────────────────────────────────────────────
    if (mode == IndicatorMode::Compass) {
        float offsetY = mOffset.mValue; // пикселей выше центра
        float dist    = enemies[0].dist;
        float angle   = enemies[0].angle;

        // Позиция стрелки — фиксированная над центром экрана
        float ax = sc.x;
        float ay = sc.y - offsetY;

        // Пульсация
        float pulse = 0.88f + 0.12f * sinf(now * 3.5f);
        float curSz = sz * pulse;
        float curOp = op * pulse;

        // Тематический цвет
        ImColor col = ColorUtils::getThemedColor(now * 30.f);
        col.Value.w = curOp;

        // Стрелка повёрнута на врага (angle = направление от игрока к врагу)
        drawCompassArrow(dl, ax, ay, angle, curSz, col, glow);

        // Дистанция над стрелкой (как на скрине — "46m" сверху)
        if (showDist) {
            char buf[24]; snprintf(buf, sizeof(buf), "%.0fm", dist);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float tx  = ax - ts.x * 0.5f;
            float ty  = ay - curSz - ts.y - 4.f;  // выше острия стрелки
            dl->AddText({tx+1, ty+1}, IM_COL32(0,0,0,(int)(180*curOp)), buf);
            dl->AddText({tx,   ty  }, ImColor(1.f,1.f,1.f, curOp*0.95f), buf);
        }
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MODE 2: RADAR — большой круг + треугольники на краю
    // ─────────────────────────────────────────────────────────────────────────
    if (mode == IndicatorMode::Radar) {
        dl->AddCircle({sc.x, sc.y}, R, ImColor(1.f,1.f,1.f, op*0.8f), 128, 1.5f);

        for (int i = 0; i < total; i++) {
            float angle = enemies[i].angle;
            float pulse = 0.85f + 0.15f * sinf(now * 4.f + (float)i * 1.1f);
            float curSz = sz * pulse;
            float curOp = op * (0.85f + 0.15f * pulse);

            ImColor col = ColorUtils::getThemedColor(now * 30.f + (float)i * 25.f);
            col.Value.w = curOp;

            drawTriPointer(dl, sc.x, sc.y, angle, R, curSz, col, glow);

            if (showDist) {
                char buf[24]; snprintf(buf, sizeof(buf), "%.0fm", enemies[i].dist);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                float textR = R + curSz * 1.6f + 4.f;
                float tx = sc.x + sinf(angle) * textR - ts.x * 0.5f;
                float ty = sc.y - cosf(angle) * textR - ts.y * 0.5f;
                dl->AddText({tx+1,ty+1}, IM_COL32(0,0,0,(int)(180*curOp)), buf);
                dl->AddText({tx,  ty  }, ImColor(1.f,1.f,1.f, curOp*0.9f), buf);
            }
        }

        if (showCnt && total > 0) {
            char buf[8]; snprintf(buf, sizeof(buf), "%d", total);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float tx = sc.x - ts.x * 0.5f, ty = sc.y - ts.y * 0.5f;
            dl->AddText({tx+1,ty+1}, IM_COL32(0,0,0,160), buf);
            dl->AddText({tx,  ty  }, ImColor(1.f,1.f,1.f, op*0.75f), buf);
        }
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MODE 1: ARROWS — шевроны на кольце
    // ─────────────────────────────────────────────────────────────────────────
    for (int i = 0; i < total; i++) {
        float angle = enemies[i].angle;
        float ax    = sc.x + sinf(angle) * R;
        float ay    = sc.y - cosf(angle) * R;

        float pulse = 0.85f + 0.15f * sinf(now * 4.f + (float)i * 1.1f);
        float curSz = sz * pulse;
        float curOp = op * (0.85f + 0.15f * pulse);

        ImColor col = ColorUtils::getThemedColor(now * 30.f + (float)i * 25.f);
        col.Value.w = curOp;

        drawChevron(dl, ax, ay, angle, curSz, col, glow);

        if (showDist) {
            char buf[24]; snprintf(buf, sizeof(buf), "%.0fm", enemies[i].dist);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float textR = R + curSz + 4.f;
            float tx = sc.x + sinf(angle) * textR - ts.x * 0.5f;
            float ty = sc.y - cosf(angle) * textR - ts.y * 0.5f;
            dl->AddText({tx+1,ty+1}, IM_COL32(0,0,0,(int)(180*curOp)), buf);
            ImColor tc = col; tc.Value.w = curOp * 0.9f;
            dl->AddText({tx, ty}, tc, buf);
        }
    }

    if (showCnt && total > 0) {
        char buf[8]; snprintf(buf, sizeof(buf), "%d", total);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        float tx = sc.x - ts.x * 0.5f, ty = sc.y - ts.y * 0.5f;
        ImColor cc = ColorUtils::getThemedColor(now * 30.f);
        cc.Value.w = op;
        dl->AddText({tx+1,ty+1}, IM_COL32(0,0,0,160), buf);
        dl->AddText({tx,  ty  }, cc, buf);
    }
}
