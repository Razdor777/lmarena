//
// LevelInfo — separate draggable HUD widgets: Ping, Name, XYZ, FPS, BPS
// Each widget is independently positioned via HudEditor.
//
// Visual style: clean "info pill" chips — soft gradient body, 1px hairline,
// accent dot + dim label + bright value. Easy on the eyes.
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

// ─────────────────────────────────────────────────────────────────────────────
// One HudElement per widget
// ─────────────────────────────────────────────────────────────────────────────

// IDs must be unique static strings
static char sIdPing[]  = "InfoPing";
static char sIdName[]  = "InfoName";
static char sIdXYZ[]   = "InfoXYZ";
static char sIdFPS[]   = "InfoFPS";
static char sIdBPS[]   = "InfoBPS";
static char sIdArrow[] = "InfoArrows";
static char sIdPearl[] = "InfoPearls";
static char sIdKicks[] = "InfoKicks";
static char sIdHealth[] = "InfoHealth";
static char sIdSession[] = "InfoSession";

struct InfoWidget : public HudElement {
    InfoWidget(char* id, float defX, float defY)
        : HudElement(id) {
        mVisible = true;
        mAnchor  = Anchor::BottomLeft;
        mPos     = {defX, defY};
    }

    // ---- animation state (per widget, so nothing leaks between pills) ----
    float animValue  = NAN;   // eased display value (numbers stop twitching)
    float lastValue  = NAN;   // last committed value, for the change flash
    float flash      = 0.f;   // 1 -> 0 highlight right after a change
    float widthAnim  = -1.f;  // animated chip width (-1 = snap on first draw)
    float hist[48]{};         // sparkline samples, oldest .. newest
    int   histCount  = 0;
    float sampleTimer = 0.f;
};

static InfoWidget* gWPing  = nullptr;
static InfoWidget* gWName  = nullptr;
static InfoWidget* gWXYZ   = nullptr;
static InfoWidget* gWFPS   = nullptr;
static InfoWidget* gWBPS   = nullptr;
static InfoWidget* gWArrow = nullptr;
static InfoWidget* gWPearl = nullptr;
static InfoWidget* gWKicks = nullptr;
static InfoWidget* gWHealth = nullptr;
static InfoWidget* gWSession = nullptr;

static void registerWidget(InfoWidget*& ptr, char* id, float dx, float dy) {
    if (!ptr) {
        ptr = new InfoWidget(id, dx, dy);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(ptr);
    }
    ptr->mVisible = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: draw a clean "info pill" chip at (x, y)
//  [ ● LABEL  value ]
// returns the chip width
// ─────────────────────────────────────────────────────────────────────────────
static float drawChip(ImDrawList* dl, ImFont* font, float fs,
    float x, float y, const char* label, const char* value, ImColor accent,
    float opacity, float rounding, bool blur, bool showDot, bool showDivider,
    InfoWidget* widget, bool sparkline, float dt)
{
    // Clamp rounding so pills keep a sane shape
    float padH  = 9.f;
    float padV  = 4.f;
    float dotR  = 3.f;
    float gap   = 6.f;

    float labelFs = fs * 0.82f;
    float valueFs = fs;

    std::string labelUpper = label ? label : "";
    for (auto& c : labelUpper) c = (char)toupper((unsigned char)c);

    float labelW = labelUpper.empty()
        ? 0.f
        : font->CalcTextSizeA(labelFs, FLT_MAX, 0, labelUpper.c_str()).x;
    float valueW = font->CalcTextSizeA(valueFs, FLT_MAX, 0, value).x;

    float dotSpace  = showDot ? (dotR * 2.f + gap) : 0.f;
    float divSpace  = (!labelUpper.empty() && showDivider) ? gap : 0.f;
    float labelSpace = labelUpper.empty() ? 0.f : (labelW + gap * 0.75f);
    if (labelUpper.empty()) divSpace = 0.f;

    // Sparkline slot: a small graph on the right of the chip.
    const bool  hasSpark = sparkline && widget && widget->histCount > 1;
    const float sparkW   = hasSpark ? fs * 2.6f : 0.f;

    float w = padH * 2.f + dotSpace + labelSpace + divSpace + valueW;
    if (hasSpark) w += gap + sparkW;

    float h = valueFs + padV * 2.f;

    // Animated width: the pill grows/shrinks into its new size instead of
    // teleporting when a value gets longer. The background is never allowed to
    // be narrower than the content, or the text would spill out of it.
    float drawW = w;
    if (widget) {
        if (widget->widthAnim < 0.f) widget->widthAnim = w;
        widget->widthAnim += (w - widget->widthAnim) * std::clamp(dt * 12.f, 0.f, 1.f);
        drawW = std::max(widget->widthAnim, w);
    }

    float round = std::min(rounding, h * 0.5f);

    // Soft shadow
    dl->AddShadowRect({x, y}, {x + drawW, y + h},
        ImColor(0.f, 0.f, 0.f, 0.35f * opacity), 10.f, {0.f, 1.5f}, 0, round);

    // Glass blur
    if (blur)
        ImRenderUtils::addBlur(ImVec4(x, y, x + drawW, y + h), 3.f, round);

    // Gradient body (top slightly lighter than bottom)
    ImColor topCol(22, 23, 31, (int)(225 * opacity));
    ImColor botCol(11, 11, 17, (int)(225 * opacity));
    dl->AddRectFilledMultiColor({x, y}, {x + drawW, y + h},
        topCol, topCol, botCol, botCol, round, ImDrawFlags_RoundCornersAll);

    // Hairline border
    dl->AddRect({x, y}, {x + drawW, y + h},
        ImColor(255, 255, 255, (int)(16 * opacity)), round, 0, 1.f);

    // Change highlight: a short accent wash + border right after the value
    // moved, so a fresh arrow/pearl/kick is impossible to miss.
    if (widget) {
        widget->flash = std::max(0.f, widget->flash - dt * 2.6f);
        if (widget->flash > 0.001f) {
            ImColor wash = accent; wash.Value.w = 0.26f * widget->flash * opacity;
            ImColor edge = accent; edge.Value.w = 0.85f * widget->flash * opacity;
            dl->AddRectFilled({x, y}, {x + drawW, y + h}, wash, round);
            dl->AddRect({x, y}, {x + drawW, y + h}, edge, round, 0, 1.f);
        }
    }

    float cx = x + padH;
    float textY = y + padV;

    // Accent dot, vertically centered
    if (showDot) {
        float cy = y + h * 0.5f;
        // faint halo + core
        ImColor halo = accent; halo.Value.w = 0.18f * opacity;
        ImColor core = accent; core.Value.w = 0.95f * opacity;
        dl->AddCircleFilled({cx + dotR, cy}, dotR * 2.1f, halo, 12);
        dl->AddCircleFilled({cx + dotR, cy}, dotR, core, 12);
        cx += dotR * 2.f + gap;
    }

    // Dim label
    if (!labelUpper.empty()) {
        ImColor labelCol(148, 152, 168, (int)(235 * opacity));
        // align baseline-ish with value text
        float labelY = textY + (valueFs - labelFs) * 0.45f;
        dl->AddText(font, labelFs, {cx, labelY}, labelCol, labelUpper.c_str());
        cx += labelW;

        if (showDivider) {
            cx += gap * 0.75f;
            ImColor divCol(255, 255, 255, (int)(28 * opacity));
            float divH = h * 0.42f;
            dl->AddLine({cx, y + h * 0.5f - divH * 0.5f}, {cx, y + h * 0.5f + divH * 0.5f}, divCol, 1.f);
            cx += gap * 0.6f;
        } else {
            cx += gap * 0.75f;
        }
    }

    // Bright value
    ImColor valueCol = accent;
    if (valueCol.Value.w > 0.98f) valueCol.Value.w = 0.98f;
    valueCol.Value.w *= opacity;
    dl->AddText(font, valueFs, {cx, textY}, valueCol, value);

    // ── sparkline ─────────────────────────────────────────────────────────
    // Oldest sample on the left, newest on the right, brighter toward the
    // head — a tiny history strip that costs almost nothing.
    if (hasSpark) {
        const float sx0 = cx + valueW + gap;
        const float sx1 = sx0 + sparkW;
        const float sy0 = y + padV + 1.f;
        const float sy1 = y + h - padV - 1.f;

        const int n = widget->histCount;
        const int base = 48 - n;

        float mn = widget->hist[base], mx = widget->hist[base];
        for (int i = base + 1; i < 48; ++i) {
            mn = std::min(mn, widget->hist[i]);
            mx = std::max(mx, widget->hist[i]);
        }
        if (mx - mn < 1.f) { mn -= 1.f; mx += 1.f; }

        ImColor lineCol = accent;

        for (int i = 0; i < n - 1; ++i) {
            const float t0 = (float)i / (float)(n - 1);
            const float t1 = (float)(i + 1) / (float)(n - 1);
            const float v0 = (widget->hist[base + i]     - mn) / (mx - mn);
            const float v1 = (widget->hist[base + i + 1] - mn) / (mx - mn);

            lineCol.Value.w = opacity * (0.25f + 0.6f * t1);

            dl->AddLine({
                sx0 + (sx1 - sx0) * t0, sy1 - (sy1 - sy0) * v0
            }, {
                sx0 + (sx1 - sx0) * t1, sy1 - (sy1 - sy0) * v1
            }, lineCol, 1.2f);
        }

        // Faint floor line for the graph to sit on.
        ImColor baseCol(255, 255, 255, (int)(22 * opacity));
        dl->AddLine({sx0, sy1}, {sx1, sy1}, baseCol, 1.f);
    }

    return drawW;
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

    // Register all widgets — bottom-left area, staggered horizontally
    registerWidget(gWPing,  sIdPing,  10.f,  -32.f);
    registerWidget(gWName,  sIdName,  90.f,  -32.f);
    registerWidget(gWXYZ,   sIdXYZ,   190.f, -32.f);
    registerWidget(gWFPS,   sIdFPS,   330.f, -32.f);
    registerWidget(gWBPS,   sIdBPS,   400.f, -32.f);
    registerWidget(gWArrow, sIdArrow, 470.f, -32.f);
    registerWidget(gWPearl, sIdPearl, 540.f, -32.f);
    registerWidget(gWKicks, sIdKicks, 610.f, -32.f);
    registerWidget(gWHealth, sIdHealth, 700.f, -32.f);
    registerWidget(gWSession, sIdSession, 790.f, -32.f);

    mSessionStart = NOW;
}

void LevelInfo::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent,        &LevelInfo::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,      &LevelInfo::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,      &LevelInfo::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<SendImmediateEvent, &LevelInfo::onSendImmediateEvent>(this);
    gFeatureManager->mDispatcher->deafen<PingUpdateEvent,    &LevelInfo::onPingUpdateEvent>(this);

    auto hide = [](InfoWidget* w){ if (w) w->mVisible = false; };
    hide(gWPing); hide(gWName); hide(gWXYZ); hide(gWFPS);
    hide(gWBPS);  hide(gWArrow); hide(gWPearl); hide(gWKicks);
    hide(gWHealth); hide(gWSession);
}

// ─────────────────────────────────────────────────────────────────────────────
// Inventory helpers
// ─────────────────────────────────────────────────────────────────────────────

int LevelInfo::getArrowsAmount() {
    auto player = ClientInstance::get()->getLocalPlayer();
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
    if (event.mPacket->getId() == PacketID::Disconnect) mKicksAmount++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Render — one pill per enabled widget
// ─────────────────────────────────────────────────────────────────────────────

void LevelInfo::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto dl    = ImGui::GetBackgroundDrawList();
    float now  = (float)ImGui::GetTime();
    float fs   = mFontSize.mValue * mScale.mValue;
    float round= mRounding.mValue;
    float opacity = mOpacity.mValue;
    bool  blur  = mGlass.mValue;
    bool  dots  = mIcons.mValue;
    bool  divs  = mDividers.mValue;

    FontHelper::pushPrefFont(false);
    ImFont* font = ImGui::GetFont();

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);

    // Numbers ease toward their real value instead of snapping, so a jittery
    // ping/fps readout stops twitching and counters roll up like an odometer.
    auto animTo = [&](InfoWidget* w, float target, float speed) -> float {
        if (!std::isfinite(w->animValue)) w->animValue = target;
        w->animValue += (target - w->animValue) * std::clamp(dt * speed, 0.f, 1.f);
        return w->animValue;
    };

    // Flash the pill when a counter actually changed.
    auto markChange = [&](InfoWidget* w, float v) {
        if (!mHighlight.mValue) return;
        if (std::isfinite(w->lastValue) && std::fabs(v - w->lastValue) >= 0.5f) w->flash = 1.f;
        w->lastValue = v;
    };

    // Feed the sparkline; sampled at 4 Hz so the strip shows seconds, not noise.
    auto sparkSample = [&](InfoWidget* w, float v) {
        if (!mSparkline.mValue) return;
        w->sampleTimer += dt;
        if (w->sampleTimer < 0.25f) return;
        w->sampleTimer = 0.f;
        for (int i = 0; i < 47; ++i) w->hist[i] = w->hist[i + 1];
        w->hist[47] = v;
        if (w->histCount < 48) w->histCount++;
    };

    auto draw = [&](InfoWidget* w, const char* label, const char* value, ImColor accent, bool spark = false) {
        ImVec2 pos = w->getPos();
        float width = drawChip(dl, font, fs, pos.x, pos.y, label, value,
                               accent, opacity, round, blur, dots, divs,
                               w, spark, dt);
        w->mSize = {width, fs + 8.f};
    };

    // ── Ping pill ─────────────────────────────────────────────────────────
    if (mShowPing.mValue && gWPing && gWPing->mVisible) {
        const float pingRaw = (float)mPing;
        sparkSample(gWPing, pingRaw);
        const int ping = (int)std::lround(animTo(gWPing, pingRaw, 7.f));
        ImColor pc(110, 255, 170, 255);
        if (mColorPing.mValue) {
            if      (ping > 150) pc = ImColor(255, 105, 105, 255);
            else if (ping >  80) pc = ImColor(255, 200, 95,  255);
        }
        char buf[32]; snprintf(buf, sizeof(buf), "%d ms", ping);
        draw(gWPing, "ping", buf, pc, true);
    }

    // ── Name pill ─────────────────────────────────────────────────────────
    if (mShowName.mValue && gWName && gWName->mVisible) {
        // Strip color codes from own name
        std::string raw = ColorUtils::removeColorCodes(player->getRawName());
        // Also remove any prefix like [Player] or [Sr$7...]
        // Take only the last word after the last space (actual nick)
        size_t sp = raw.rfind(' ');
        std::string name = (sp != std::string::npos) ? raw.substr(sp+1) : raw;
        // Additional strip if still has prefix bracket
        if (!name.empty() && name[0] == '[') {
            size_t rb = name.find(']');
            if (rb != std::string::npos) name = name.substr(rb+1);
        }
        if (name.empty()) name = raw; // fallback

        ImColor nc = ColorUtils::getThemedColor(now * 20.f);
        nc.Value.w = 1.f;
        draw(gWName, "name", name.c_str(), nc);
    }

    // ── XYZ pill ──────────────────────────────────────────────────────────
    if (mShowXYZ.mValue && gWXYZ && gWXYZ->mVisible) {
        glm::ivec3 p = *player->getPos();
        char buf[64]; snprintf(buf, sizeof(buf), "%d  %d  %d", p.x, p.y, p.z);
        draw(gWXYZ, "xyz", buf, ImColor(125, 200, 255, 255));
    }

    // ── FPS pill ──────────────────────────────────────────────────────────
    if (mShowFPS.mValue && gWFPS && gWFPS->mVisible) {
        const float fpsRaw = ImGui::GetIO().Framerate;
        sparkSample(gWFPS, fpsRaw);
        const int fps = (int)std::lround(animTo(gWFPS, fpsRaw, 7.f));
        ImColor fc(130, 240, 190, 255);
        if      (fps < 30) fc = ImColor(255, 105, 105, 255);
        else if (fps < 60) fc = ImColor(255, 200, 95,  255);
        char buf[24]; snprintf(buf, sizeof(buf), "%d", fps);
        draw(gWFPS, "fps", buf, fc, true);
    }

    // ── BPS pill ──────────────────────────────────────────────────────────
    if (mShowBPS.mValue && gWBPS && gWBPS->mVisible) {
        char buf[24]; snprintf(buf, sizeof(buf), "%.1f b/s", mBps);
        draw(gWBPS, "speed", buf, ImColor(255, 190, 95, 255));
    }

    // ── Arrows pill ───────────────────────────────────────────────────────
    if (mShowArrows.mValue && gWArrow && gWArrow->mVisible) {
        markChange(gWArrow, (float)mArrows);
        const int shown = (int)std::lround(animTo(gWArrow, (float)mArrows, 10.f));
        char buf[24]; snprintf(buf, sizeof(buf), "%d", shown);
        draw(gWArrow, "arrows", buf, ImColor(225, 210, 160, 255));
    }

    // ── Pearls pill ───────────────────────────────────────────────────────
    if (mShowEnderPearls.mValue && gWPearl && gWPearl->mVisible) {
        markChange(gWPearl, (float)mPearls);
        const int shown = (int)std::lround(animTo(gWPearl, (float)mPearls, 10.f));
        char buf[24]; snprintf(buf, sizeof(buf), "%d", shown);
        draw(gWPearl, "pearls", buf, ImColor(140, 175, 255, 255));
    }

    // ── Kicks pill ────────────────────────────────────────────────────────
    if (mShowKicksAmount.mValue && gWKicks && gWKicks->mVisible) {
        markChange(gWKicks, (float)mKicksAmount);
        const int shown = (int)std::lround(animTo(gWKicks, (float)mKicksAmount, 10.f));
        char buf[24]; snprintf(buf, sizeof(buf), "%d", shown);
        draw(gWKicks, "kicks", buf, ImColor(255, 110, 110, 255));
    }

    // Health pill: colour ramps green -> red with the actual HP ratio.
    if (mShowHealth.mValue && gWHealth && gWHealth->mVisible) {
        float hp = 0.f, hpMax = 20.f;

        try {
            hp = player->getHealth();
            const float mx = player->getMaxHealth();
            if (mx > 0.f) hpMax = mx;
        } catch (...) {}

        const float ratio = hpMax > 0.f ? std::clamp(hp / hpMax, 0.f, 1.f) : 1.f;

        sparkSample(gWHealth, hp);
        markChange(gWHealth, std::floor(hp * 2.f) * 0.5f);

        const float shown = animTo(gWHealth, hp, 8.f);
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f", shown);
        draw(gWHealth, "hp", buf, ImColor::HSV(0.33f * ratio, 0.72f, 1.f), true);
    }

    // Session pill: mm:ss since the module was enabled.
    if (mShowSession.mValue && gWSession && gWSession->mVisible) {
        int64_t secs = (NOW - mSessionStart) / 1000;
        if (secs < 0)      secs = 0;
        if (secs > 359999) secs = 359999;   // keep the pill from growing forever

        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d",
                 (int)((secs / 60) % 60) + (int)(secs / 3600) * 60, (int)(secs % 60));
        draw(gWSession, "session", buf, ImColor(190, 165, 255, 255));
    }

    ImGui::PopFont();
}
