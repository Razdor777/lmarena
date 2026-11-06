//
// PlayerHUD — красивая информационная панель как на скрине.
// По дефолту: сверху слева, все чипы в одну строку.
// Дизайн: тёмный фон с цветной верхней линией, тень у текста.
//

#include "PlayerHUD.hpp"
#include "HudEditor.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftSim.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/FontHelper.hpp>

// ─── HUD element ─────────────────────────────────────────────────────────────

static char sHUDId[] = "PlayerHUD";

struct PlayerHUDElement : public HudElement {
    PlayerHUDElement() : HudElement(sHUDId) {
        mVisible  = true;
        mAnchor   = Anchor::TopLeft;
        mPos      = {10.f, 10.f};  // сверху слева как на скрине
        mCentered = false;
    }
};
static PlayerHUDElement* gHUDElem = nullptr;

// ─── Enable / Disable ────────────────────────────────────────────────────────

void PlayerHUD::onEnable() {
    if (!gHUDElem) {
        gHUDElem = new PlayerHUDElement();
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gHUDElem);
    }
    gHUDElem->mVisible = true;

    gFeatureManager->mDispatcher->listen<RenderEvent,        &PlayerHUD::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,      &PlayerHUD::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PingUpdateEvent,    &PlayerHUD::onPingUpdateEvent,
        nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<SendImmediateEvent, &PlayerHUD::onSendImmediateEvent,
        nes::event_priority::VERY_LAST>(this);
}

void PlayerHUD::onDisable() {
    if (gHUDElem) gHUDElem->mVisible = false;
    gFeatureManager->mDispatcher->deafen<RenderEvent,        &PlayerHUD::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,      &PlayerHUD::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PingUpdateEvent,    &PlayerHUD::onPingUpdateEvent>(this);
    gFeatureManager->mDispatcher->deafen<SendImmediateEvent, &PlayerHUD::onSendImmediateEvent>(this);
}

// ─── Events ──────────────────────────────────────────────────────────────────

void PlayerHUD::onSendImmediateEvent(SendImmediateEvent& event) {
    if (event.send[0] == 0) {
        uint64_t ts = _byteswap_uint64(*reinterpret_cast<uint64_t*>(&event.send[1]));
        mDelay = (int64_t)(NOW - ts);
    }
}

void PlayerHUD::onPingUpdateEvent(PingUpdateEvent& event) {
    mPing = event.mPing - mDelay;
}

void PlayerHUD::onBaseTickEvent(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    static glm::vec3 prevPos = *player->getPos();
    glm::vec3 pos = *player->getPos();

    float sim = 1.f;
    auto* mc = ClientInstance::get()->getMinecraftSim();
    if (mc) sim = mc->getSimTimer() * mc->getSimSpeed();

    glm::vec2 xz  = {pos.x, pos.z};
    glm::vec2 pxz = {prevPos.x, prevPos.z};
    float bps = glm::distance(xz, pxz) * sim;

    mBpsHistory[NOW] = bps;
    for (auto it = mBpsHistory.begin(); it != mBpsHistory.end(); )
        it = (NOW - it->first > 1000) ? mBpsHistory.erase(it) : ++it;

    float total = 0.f; int cnt = 0;
    for (auto& [t, v] : mBpsHistory) { total += v; cnt++; }
    mBps = cnt > 0 ? total / cnt : 0.f;
    prevPos = pos;

    // Перлы и стрелы
    if (mShowPearls.mValue || mShowArrows.mValue) {
        auto* sup = player->getSupplies();
        auto* cont = sup ? sup->getContainer() : nullptr;
        if (cont) {
            mPearls = 0; mArrows = 0;
            for (int i = 0; i < 36; i++) {
                auto* st = cont->getItem(i);
                if (!st || !st->mItem) continue;
                
                // Получаем одинарный указатель на Item
                auto* item = st->getItem();
                if (!item) continue;
                
                if (item->mName == "ender_pearl") mPearls += st->mCount;
                if (item->mName == "arrow")       mArrows += st->mCount;
            }
        }
    }
}

// ─── Draw one chip ───────────────────────────────────────────────────────────

// Рисует один "чип" — скруглённый прямоугольник с текстом.
// Возвращает ширину чипа.
static float drawChip(ImDrawList* dl, float x, float y, float h,
                       const char* text, ImColor textCol, ImColor lineCol,
                       float opacity, float rounding)
{
    ImFont* font = ImGui::GetFont();
    float   fs   = ImGui::GetFontSize();
    float   padX = 8.f, padY = (h - fs) * 0.5f;
    float   tw   = font->CalcTextSizeA(fs, FLT_MAX, 0, text).x;
    float   w    = tw + padX * 2.f;

    // Фон
    dl->AddRectFilled({x, y}, {x+w, y+h},
        ImColor(10, 10, 18, (int)(230 * opacity)), rounding);

    // Цветная линия сверху
    dl->AddRectFilled({x + rounding * 0.4f, y},
                      {x + w - rounding * 0.4f, y + 1.5f},
        ImColor(lineCol.Value.x, lineCol.Value.y, lineCol.Value.z, 0.9f));

    // Тень + текст
    dl->AddText(font, fs, {x + padX + 1.f, y + padY + 1.f},
        ImColor(0, 0, 0, 100), text);
    dl->AddText(font, fs, {x + padX, y + padY},
        textCol, text);

    return w;
}

// ─── Render ──────────────────────────────────────────────────────────────────

void PlayerHUD::onRenderEvent(RenderEvent& event) {
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    if (!gHUDElem || !gHUDElem->mVisible) return;

    auto* dl  = ImGui::GetBackgroundDrawList();
    float now = (float)ImGui::GetTime();
    float sc  = mScale.mValue;
    float fs  = mFontSize.mValue * sc;
    float op  = mOpacity.mValue;
    float rd  = mRounding.mValue * sc;
    float gap = mSpacing.mValue * sc;

    FontHelper::pushPrefFont(false);
    ImGui::SetWindowFontScale(fs / ImGui::GetFontSize());

    ImVec2 anchor = gHUDElem->getPos();
    float  h = fs + 10.f * sc;  // высота чипа
    float  cx = anchor.x;
    float  cy = anchor.y;

    // Если One Row — рисуем всё одной строкой начиная от anchor
    // Если нет — каждый чип независим (тут пока все в ряд)

    // ── Имя ──────────────────────────────────────────────────────────────
    if (mShowName.mValue) {
        std::string raw = ColorUtils::removeColorCodes(player->getRawName());
        size_t sp = raw.rfind(' ');
        std::string name = (sp != std::string::npos) ? raw.substr(sp+1) : raw;
        if (name.empty()) name = raw;

        ImColor col;
        if (mRainbowName.mValue) {
            col = ColorUtils::getThemedColor(now * 25.f);
            col.Value.w = 0.95f;
        } else {
            col = ImColor(255, 255, 255, 240);
        }
        ImColor line = col;
        float w = drawChip(dl, cx, cy, h, name.c_str(), col, line, op, rd);
        gHUDElem->mSize = {w, h};
        cx += w + gap;
    }

    // ── FPS ──────────────────────────────────────────────────────────────
    if (mShowFPS.mValue) {
        int fps = (int)ImGui::GetIO().Framerate;
        char buf[24]; snprintf(buf, sizeof(buf), "%d FPS", fps);
        ImColor col = ImColor(130, 255, 160, 240);
        if      (fps < 30) col = ImColor(255, 70,  70,  240);
        else if (fps < 60) col = ImColor(255, 190, 50,  240);
        float w = drawChip(dl, cx, cy, h, buf, col, col, op, rd);
        cx += w + gap;
    }

    // ── Ping ─────────────────────────────────────────────────────────────
    if (mShowPing.mValue) {
        int ping = (int)mPing;
        char buf[24]; snprintf(buf, sizeof(buf), "%d ms", ping);
        ImColor col = ImColor(100, 220, 255, 240);
        if (mColorPing.mValue) {
            if      (ping > 150) col = ImColor(255, 70, 70,  240);
            else if (ping >  80) col = ImColor(255, 190, 50, 240);
        }
        float w = drawChip(dl, cx, cy, h, buf, col, col, op, rd);
        cx += w + gap;
    }

    // ── XYZ ──────────────────────────────────────────────────────────────
    if (mShowXYZ.mValue) {
        glm::ivec3 p = glm::ivec3(*player->getPos());
        char buf[48]; snprintf(buf, sizeof(buf), "%d / %d / %d", p.x, p.y, p.z);
        ImColor col  = ImColor(180, 180, 255, 240);
        ImColor line = ImColor(140, 140, 255, 240);
        float w = drawChip(dl, cx, cy, h, buf, col, line, op, rd);
        cx += w + gap;
    }

    // ── Скорость ─────────────────────────────────────────────────────────
    if (mShowSpeed.mValue) {
        char buf[24]; snprintf(buf, sizeof(buf), "%.1f Bps", mBps);
        ImColor col = ImColor(255, 200, 80, 240);
        float w = drawChip(dl, cx, cy, h, buf, col, col, op, rd);
        cx += w + gap;
    }

    // ── Перлы ────────────────────────────────────────────────────────────
    if (mShowPearls.mValue) {
        char buf[24]; snprintf(buf, sizeof(buf), "%d \xee\x80\x84", mPearls); // pearl icon
        snprintf(buf, sizeof(buf), "%d pearls", mPearls);
        ImColor col = ImColor(80, 200, 255, 240);
        float w = drawChip(dl, cx, cy, h, buf, col, col, op, rd);
        cx += w + gap;
    }

    // ── Стрелы ───────────────────────────────────────────────────────────
    if (mShowArrows.mValue) {
        char buf[24]; snprintf(buf, sizeof(buf), "%d arrows", mArrows);
        ImColor col = ImColor(220, 200, 130, 240);
        float w = drawChip(dl, cx, cy, h, buf, col, col, op, rd);
        cx += w + gap;
    }

    ImGui::SetWindowFontScale(1.f);
    ImGui::PopFont();
}
