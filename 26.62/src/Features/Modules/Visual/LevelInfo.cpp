//
// LevelInfo — одна цельная информационная полоса: ping, fps, ник, XYZ,
// скорость, стрелы, жемчуг, кики, HP и время сессии.
//
// Что изменилось по сравнению со старой версией (и почему):
//   • было 10 отдельных «пилюль» — каждая со своей подложкой, тенью, блюром и
//     собственной позицией в HudEditor. Десять размытий и десять теней на
//     кадр, десять объектов, которые нужно расставлять, и «лестница», если
//     значения меняют длину;
//   • стало: ОДНА полоса. Одна подложка, одна тень, один блюр, один объект в
//     HudEditor. Сегменты разделены тонкими линиями и выровнены по одной
//     базовой линии — как в дорогих клиентах;
//   • ширина полосы анимируется целиком: значение стало длиннее (ping 9 -> 10
//     ms) — полоса плавно растёт, а не прыгает;
//   • у каждого сегмента своя вспышка при изменении значения и свой
//     мини-график (ping/fps/hp);
//   • текст не пересчитывается зря: строки собираются один раз за кадр.
//

#include "LevelInfo.hpp"
#include "HudEditor.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/SendImmediateEvent.hpp>
#include <Features/Events/PingUpdateEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftSim.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/StringUtils.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Один объект в HudEditor — вся полоса
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
    // Метрики фиксированы один раз здесь: ровно один источник правды для
    // отступов, и никаких «поехавших» сегментов.
    constexpr float kPadH      = 11.f;   // слева/справа у сегмента
    constexpr float kPadV      = 5.f;    // сверху/снизу у полосы
    constexpr float kGap       = 6.f;    // между точкой/подписью/значением
    constexpr float kDotR      = 2.6f;
    constexpr float kHistSize  = 48;
    constexpr float kSampleHz  = 4.f;    // 4 Гц: график показывает секунды, а не шум

    struct SegmentStyle
    {
        float animValue  = NAN;    // плавное значение (цифры не дёргаются)
        float lastValue  = NAN;    // последнее показанное — для вспышки
        float flash      = 0.f;    // 1 -> 0 сразу после изменения
        float hist[48]{};          // история для мини-графика
        int   histCount  = 0;
        float sampleTimer = 0.f;
    };

    // Одна полоса. Наследуемся от HudElement, чтобы работал HudEditor: его
    // тянет пользователь, а размер (mSize) полоса обновляет сама.
    struct InfoBar : public HudElement
    {
        InfoBar(char* id) : HudElement(id)
        {
            mVisible = true;
            mAnchor  = Anchor::BottomLeft;
            mPos     = { 12.f, -36.f };
        }

        float        widthAnim = -1.f;   // анимированная ширина (-1 = первый кадр)
        SegmentStyle seg[12];
    };

    char sIdBar[] = "InfoBar";
    InfoBar* gBar = nullptr;

    void ensureBar()
    {
        if (gBar) return;
        gBar = new InfoBar(sIdBar);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gBar);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Enable / Disable
// ─────────────────────────────────────────────────────────────────────────────

void LevelInfo::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent,        &LevelInfo::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,      &LevelInfo::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,      &LevelInfo::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<SendImmediateEvent, &LevelInfo::onSendImmediateEvent,
        nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<PingUpdateEvent,    &LevelInfo::onPingUpdateEvent,
        nes::event_priority::VERY_LAST>(this);

    ensureBar();
    if (gBar)
    {
        gBar->mVisible = true;
        mHudElement = gBar;
    }

    mSessionStart = NOW;
}

void LevelInfo::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent,        &LevelInfo::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,      &LevelInfo::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,      &LevelInfo::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<SendImmediateEvent, &LevelInfo::onSendImmediateEvent>(this);
    gFeatureManager->mDispatcher->deafen<PingUpdateEvent,    &LevelInfo::onPingUpdateEvent>(this);

    if (gBar) gBar->mVisible = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Inventory helpers
// ─────────────────────────────────────────────────────────────────────────────

int LevelInfo::getArrowsAmount() {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return 0;
    int n = 0;
    for (int i = 0; i < 36; i++) {
        auto item = player->getSupplies()->getContainer()->getItem(i);
        if (!item->mItem) continue;
        if (item->getItem()->mName == "arrow") n += item->mCount;
    }
    return n;
}

int LevelInfo::getPearlsAmount() {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return 0;
    int n = 0;
    for (int i = 0; i < 36; i++) {
        auto item = player->getSupplies()->getContainer()->getItem(i);
        if (!item->mItem) continue;
        if (item->getItem()->mName == "ender_pearl") n += item->mCount;
    }
    return n;
}

int LevelInfo::getSpellsAmount(int idx) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return 0;
    int n = 0;
    static const char* names[] = {"", "Spell of Life", "Spell of Swiftness", "Spell of Fire Trail"};
    if (idx < 1 || idx > 3) return 0;
    for (int i = 0; i < 36; i++) {
        auto item = player->getSupplies()->getContainer()->getItem(i);
        if (!item->mItem) continue;
        if (StringUtils::containsIgnoreCase(item->getCustomName(), names[idx])) n++;
    }
    return n;
}

void LevelInfo::calculateValue(int idx) {
    switch(idx) {
        case 1: mHearts  = mHealthSpells    * 4;  break;
        case 2: mSeconds = mSpeedSpells     * 5;  break;
        case 3: mBlocks  = mFireTrailSpells * 10; break;
    }
}

void LevelInfo::spellsUpdate() {
    mHealthSpells    = getSpellsAmount(1); calculateValue(1);
    mSpeedSpells     = getSpellsAmount(2); calculateValue(2);
    mFireTrailSpells = getSpellsAmount(3); calculateValue(3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Events
// ─────────────────────────────────────────────────────────────────────────────

void LevelInfo::onSendImmediateEvent(SendImmediateEvent& event) {
    uint8_t pid = event.send[0];
    if (pid == 0) {
        uint64_t ts = _byteswap_uint64(*reinterpret_cast<uint64_t*>(&event.send[1]));
        mEventDelay = (int64_t)(NOW - ts);
    }
}

void LevelInfo::onPingUpdateEvent(PingUpdateEvent& event) {
    mPing = event.mPing - mEventDelay;
}

void LevelInfo::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    static glm::vec3 posPrev = *player->getPos();
    glm::vec3 pos = *player->getPos();
    glm::vec2 xz  = {pos.x, pos.z};
    glm::vec2 pxz = {posPrev.x, posPrev.z};

    float sim = ClientInstance::get()->getMinecraftSim()->getSimTimer()
              * ClientInstance::get()->getMinecraftSim()->getSimSpeed();
    float bps = glm::distance(xz, pxz) * sim;
    mBps = bps;
    mBpsHistory[NOW] = bps;

    for (auto it = mBpsHistory.begin(); it != mBpsHistory.end();)
        if (NOW - it->first > 1000) it = mBpsHistory.erase(it); else ++it;

    float total = 0.f; int cnt = 0;
    for (auto& [t, v] : mBpsHistory) { total += v; cnt++; }
    mAveragedBps = cnt > 0 ? total / cnt : 0.f;
    posPrev = pos;

    if (mShowArrows.mValue)      mArrows = getArrowsAmount();
    if (mShowEnderPearls.mValue) mPearls = getPearlsAmount();
    if (mShowSpells.mValue)      spellsUpdate();
}

void LevelInfo::onPacketInEvent(PacketInEvent& event) {
    if (!mShowKicksAmount.mValue) return;
    if (event.mPacket && event.mPacket->getId() == PacketID::Disconnect) mKicksAmount++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Render — одна полоса, сегменты внутри
// ─────────────────────────────────────────────────────────────────────────────

void LevelInfo::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    ensureBar();
    if (!gBar || !gBar->mVisible) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    FontHelper::pushPrefFont(false);
    ImFont* font = ImGui::GetFont();
    if (!font)
    {
        ImGui::PopFont();
        return;
    }

    const float dt    = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);
    const float fs    = mFontSize.mValue * mScale.mValue;
    const float labelFs = fs * 0.78f;
    const float opacity = std::clamp(mOpacity.mValue, 0.f, 1.f);
    const float rounding = mRounding.mValue;
    const float height = fs + kPadV * 2.f;

    // ── сборка сегментов ───────────────────────────────────────────────────
    struct Seg
    {
        const char* label = nullptr;
        std::string value;
        ImColor     accent = ImColor(255, 255, 255);
        bool        spark = false;
        int         slot = 0;
        float       width = 0.f;
    };

    std::vector<Seg> segs;
    segs.reserve(12);

    const float ping = (float)mPing;
    const int pingShown = (int)std::lround(std::isfinite(ping) ? ping : 0.f);
    ImColor pingColor(110, 255, 170, 255);
    if (mColorPing.mValue)
    {
        if      (pingShown > 150) pingColor = ImColor(255, 105, 105, 255);
        else if (pingShown >  80) pingColor = ImColor(255, 200, 95,  255);
    }

    const int fpsShown = (int)std::lround(ImGui::GetIO().Framerate);
    ImColor fpsColor(130, 240, 190, 255);
    if      (fpsShown < 30) fpsColor = ImColor(255, 105, 105, 255);
    else if (fpsShown < 60) fpsColor = ImColor(255, 200, 95,  255);

    auto add = [&](const char* label, std::string value, ImColor color, bool spark) {
        Seg s; s.label = label; s.value = std::move(value); s.accent = color;
        s.spark = spark; s.slot = (int)segs.size();
        segs.push_back(std::move(s));
    };

    char buf[80];

    if (mShowName.mValue)
    {
        std::string raw = ColorUtils::removeColorCodes(player->getRawName());
        const size_t sp = raw.rfind(' ');
        std::string name = (sp != std::string::npos) ? raw.substr(sp + 1) : raw;
        if (!name.empty() && name[0] == '[')
        {
            const size_t rb = name.find(']');
            if (rb != std::string::npos) name = name.substr(rb + 1);
        }
        if (name.empty()) name = raw;
        ImColor nc = ColorUtils::getThemedColor(ImGui::GetTime() * 20.f);
        nc.Value.w = 1.f;
        add("name", name, nc, false);
    }
    if (mShowPing.mValue)
    {
        snprintf(buf, sizeof(buf), "%d ms", pingShown);
        add("ping", buf, pingColor, true);
    }
    if (mShowFPS.mValue)
    {
        snprintf(buf, sizeof(buf), "%d", fpsShown);
        add("fps", buf, fpsColor, true);
    }
    if (mShowXYZ.mValue)
    {
        glm::ivec3 p = *player->getPos();
        snprintf(buf, sizeof(buf), "%d %d %d", p.x, p.y, p.z);
        add("xyz", buf, ImColor(125, 200, 255, 255), false);
    }
    if (mShowBPS.mValue)
    {
        snprintf(buf, sizeof(buf), "%.1f b/s", mBps);
        add("speed", buf, ImColor(255, 190, 95, 255), false);
    }
    if (mShowArrows.mValue)
    {
        snprintf(buf, sizeof(buf), "%d", mArrows);
        add("arrows", buf, ImColor(225, 210, 160, 255), false);
    }
    if (mShowEnderPearls.mValue)
    {
        snprintf(buf, sizeof(buf), "%d", mPearls);
        add("pearls", buf, ImColor(140, 175, 255, 255), false);
    }
    if (mShowKicksAmount.mValue)
    {
        snprintf(buf, sizeof(buf), "%d", mKicksAmount);
        add("kicks", buf, ImColor(255, 110, 110, 255), false);
    }
    if (mShowHealth.mValue)
    {
        float hp = 20.f, hpMax = 20.f;
        try
        {
            hp = player->getHealth();
            const float mx = player->getMaxHealth();
            if (mx > 0.f) hpMax = mx;
        }
        catch (...) {}
        const float ratio = hpMax > 0.f ? std::clamp(hp / hpMax, 0.f, 1.f) : 1.f;
        snprintf(buf, sizeof(buf), "%.1f", hp);
        add("hp", buf, ImColor::HSV(0.33f * ratio, 0.72f, 1.f), true);
    }
    if (mShowSession.mValue)
    {
        int64_t secs = (NOW - mSessionStart) / 1000;
        if (secs < 0) secs = 0;
        if (secs > 359999) secs = 359999;
        snprintf(buf, sizeof(buf), "%02lld:%02lld",
                 (long long)((secs / 60) % 60) + (long long)(secs / 3600) * 60,
                 (long long)(secs % 60));
        add("session", buf, ImColor(190, 165, 255, 255), false);
    }

    if (segs.empty())
    {
        ImGui::PopFont();
        return;
    }

    // ── измерение ──────────────────────────────────────────────────────────
    const float sparkW = fs * 2.4f;
    auto textW = [&](float px, const char* s) {
        return font->CalcTextSizeA(px, FLT_MAX, 0.f, s).x;
    };
    auto textWStr = [&](float px, const std::string& s) {
        return font->CalcTextSizeA(px, FLT_MAX, 0.f, s.c_str()).x;
    };

    for (auto& s : segs)
    {
        float w = kPadH * 2.f;
        if (mIcons.mValue) w += kDotR * 2.f + kGap;
        w += textW(labelFs, s.label) + kGap * 0.7f + textWStr(fs, s.value);
        if (mSparkline.mValue && s.spark) w += kGap + sparkW;
        s.width = w;
    }

    float totalW = 0.f;
    for (const auto& s : segs) totalW += s.width;

    // Ширина полосы анимируется целиком; фон никогда не уже контента.
    if (gBar->widthAnim < 0.f) gBar->widthAnim = totalW;
    gBar->widthAnim += (totalW - gBar->widthAnim) * std::clamp(dt * 14.f, 0.f, 1.f);
    const float drawW = std::max(gBar->widthAnim, totalW);

    ImVec2 pos = gBar->getPos();
    gBar->mSize = { drawW, height };

    const float x0 = pos.x;
    const float y0 = pos.y;
    const float round = std::min(rounding, height * 0.5f);

    // ── подложка (одна на всю полосу) ──────────────────────────────────────
    dl->AddShadowRect({ x0, y0 }, { x0 + drawW, y0 + height },
                      ImColor(0.f, 0.f, 0.f, 0.38f * opacity), 12.f, { 0.f, 2.f }, 0, round);

    if (mGlass.mValue)
        ImRenderUtils::addBlur(ImVec4(x0, y0, x0 + drawW, y0 + height), 3.f, round);

    const ImColor topCol(23, 24, 33, (int)(228 * opacity));
    const ImColor botCol(10, 11, 16, (int)(228 * opacity));
    dl->AddRectFilledMultiColor({ x0, y0 }, { x0 + drawW, y0 + height },
                                topCol, topCol, botCol, botCol, round, ImDrawFlags_RoundCornersAll);
    dl->AddRect({ x0, y0 }, { x0 + drawW, y0 + height },
                ImColor(255, 255, 255, (int)(18 * opacity)), round, 0, 1.f);

    // Светлая кромка сверху — «стекло» читается даже на тёмном фоне.
    dl->AddRectFilledMultiColor({ x0 + round, y0 + 1.f }, { x0 + drawW - round, y0 + 2.f },
                                ImColor(255, 255, 255, 0), ImColor(255, 255, 255, (int)(26 * opacity)),
                                ImColor(255, 255, 255, (int)(26 * opacity)), ImColor(255, 255, 255, 0));

    // ── сегменты ───────────────────────────────────────────────────────────
    float cx = x0;
    for (size_t i = 0; i < segs.size(); ++i)
    {
        Seg& s = segs[i];
        SegmentStyle& st = gBar->seg[std::clamp(s.slot, 0, 11)];

        const float midY = y0 + height * 0.5f;
        const float tx = cx + kPadH;

        // ── вспышка при изменении значения ─────────────────────────────────
        st.flash = std::max(0.f, st.flash - dt * 2.6f);

        float drawX = tx;

        if (mIcons.mValue)
        {
            ImColor halo = s.accent; halo.Value.w = 0.18f * opacity;
            ImColor core = s.accent; core.Value.w = 0.95f * opacity;
            dl->AddCircleFilled({ drawX + kDotR, midY }, kDotR * 2.0f, halo, 10);
            dl->AddCircleFilled({ drawX + kDotR, midY }, kDotR, core, 10);
            drawX += kDotR * 2.f + kGap;
        }

        if (st.flash > 0.001f)
        {
            ImColor wash = s.accent;
            wash.Value.w = 0.16f * st.flash * opacity;
            dl->AddRectFilled({ cx + 2.f, y0 + 3.f }, { cx + s.width - 2.f, y0 + height - 3.f },
                              wash, 4.f);
        }

        const float labelW = textW(labelFs, s.label);
        dl->AddText(font, labelFs, { drawX, midY - labelFs * 0.55f },
                    ImColor(146, 150, 166, (int)(235 * opacity)), s.label);
        drawX += labelW + kGap * 0.7f;

        ImColor valueCol = s.accent;
        valueCol.Value.w = std::clamp(valueCol.Value.w, 0.f, 1.f) * opacity;
        dl->AddText(font, fs, { drawX, midY - fs * 0.55f }, valueCol, s.value.c_str());
        drawX += textWStr(fs, s.value);

        // ── мини-график ────────────────────────────────────────────────────
        //
        // История обновляется ТОЛЬКО здесь, в кадре: так она идёт со скоростью
        // отрисовки, а не тика, и не зависит от того, сколько раз значение
        // успело измениться между кадрами.
        if (mSparkline.mValue && s.spark)
        {
            float numeric = 0.f;
            if (std::strcmp(s.label, "ping") == 0) numeric = (float)pingShown;
            else if (std::strcmp(s.label, "fps") == 0) numeric = (float)fpsShown;
            else if (std::strcmp(s.label, "hp") == 0)
            {
                try { numeric = player->getHealth(); } catch (...) { numeric = 0.f; }
            }

            st.sampleTimer += dt;
            if (st.sampleTimer >= 1.f / kSampleHz)
            {
                st.sampleTimer = 0.f;
                for (int k = 0; k < 47; ++k) st.hist[k] = st.hist[k + 1];
                st.hist[47] = numeric;
                if (st.histCount < 48) st.histCount++;
            }

            // Вспышка — по реальному изменению показанного числа.
            if (mHighlight.mValue)
            {
                if (std::isfinite(st.lastValue) && std::fabs(numeric - st.lastValue) >= 0.5f)
                    st.flash = 1.f;
            }
            st.lastValue = numeric;

            if (st.histCount > 1)
            {
                const float sx0 = drawX + kGap;
                const float sx1 = sx0 + sparkW;
                const float sy0 = y0 + kPadV + 1.f;
                const float sy1 = y0 + height - kPadV - 1.f;

                const int n = st.histCount;
                const int base = 48 - n;
                float mn = st.hist[base], mx = st.hist[base];
                for (int k = base + 1; k < 48; ++k)
                {
                    mn = std::min(mn, st.hist[k]);
                    mx = std::max(mx, st.hist[k]);
                }
                if (mx - mn < 1.f) { mn -= 1.f; mx += 1.f; }

                ImColor lineCol = s.accent;
                for (int k = 0; k < n - 1; ++k)
                {
                    const float t0 = (float)k / (float)(n - 1);
                    const float t1 = (float)(k + 1) / (float)(n - 1);
                    const float v0 = (st.hist[base + k]     - mn) / (mx - mn);
                    const float v1 = (st.hist[base + k + 1] - mn) / (mx - mn);
                    lineCol.Value.w = opacity * (0.20f + 0.65f * t1);
                    dl->AddLine({ sx0 + (sx1 - sx0) * t0, sy1 - (sy1 - sy0) * v0 },
                                { sx0 + (sx1 - sx0) * t1, sy1 - (sy1 - sy0) * v1 },
                                lineCol, 1.2f);
                }
            }
        }

        cx += s.width;

        // ── разделитель между сегментами ───────────────────────────────────
        if (mDividers.mValue && i + 1 < segs.size())
        {
            const float dh = height * 0.40f;
            dl->AddLine({ cx, midY - dh * 0.5f }, { cx, midY + dh * 0.5f },
                        ImColor(255, 255, 255, (int)(26 * opacity)), 1.f);
        }
    }

    ImGui::PopFont();
}
