//
// LevelInfo — Separate draggable HUD widgets: Ping, Name, XYZ, FPS, BPS
// Redesigned with soft, modern visuals — easy on the eyes.
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

// ─────────────────────────────────────────────────────────────────────────────
// One HudElement per widget
// ─────────────────────────────────────────────────────────────────────────────

static char sIdPing[]  = "InfoPing";
static char sIdName[]  = "InfoName";
static char sIdXYZ[]   = "InfoXYZ";
static char sIdFPS[]   = "InfoFPS";
static char sIdBPS[]   = "InfoBPS";
static char sIdArrow[] = "InfoArrows";
static char sIdPearl[] = "InfoPearls";
static char sIdKicks[] = "InfoKicks";

struct InfoWidget : public HudElement {
    InfoWidget(char* id, float defX, float defY)
        : HudElement(id) {
        mVisible = true;
        mAnchor  = Anchor::BottomLeft;
        mPos     = {defX, defY};
    }
};

static InfoWidget* gWPing  = nullptr;
static InfoWidget* gWName  = nullptr;
static InfoWidget* gWXYZ   = nullptr;
static InfoWidget* gWFPS   = nullptr;
static InfoWidget* gWBPS   = nullptr;
static InfoWidget* gWArrow = nullptr;
static InfoWidget* gWPearl = nullptr;
static InfoWidget* gWKicks = nullptr;

static void registerWidget(InfoWidget*& ptr, char* id, float dx, float dy) {
    if (!ptr) {
        ptr = new InfoWidget(id, dx, dy);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(ptr);
    }
    ptr->mVisible = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Soft pill helper — rounded, muted colors, left accent bar
// returns the chip width
// ─────────────────────────────────────────────────────────────────────────────
static float drawChip(ImDrawList* dl, ImFont* font, float fs,
    float x, float y, const char* icon, const char* text,
    ImColor accentColor, float opacity, float rounding, bool blur, float scale)
{
    float padH = 8.f * scale;
    float padV = 4.f * scale;
    float iconGap = (icon && icon[0]) ? 4.f * scale : 0.f;

    float iconW = 0.f;
    if (icon && icon[0]) {
        iconW = font->CalcTextSizeA(fs, FLT_MAX, 0, icon).x;
    }
    float tw = font->CalcTextSizeA(fs, FLT_MAX, 0, text).x;
    float w  = padH + iconW + iconGap + tw + padH;
    float h  = fs + padV * 2.f;

    // Blur background
    if (blur)
        ImRenderUtils::addBlur(ImVec4(x, y, x+w, y+h), 4.f, rounding);

    // Soft dark background
    dl->AddRectFilled({x, y}, {x+w, y+h},
        ImColor(15, 15, 20, (int)(180 * opacity)), rounding);

    // Subtle border
    dl->AddRect({x, y}, {x+w, y+h},
        ImColor(255, 255, 255, (int)(18 * opacity)), rounding, 0, 1.f);

    // Left accent bar (3px wide, rounded left corners)
    float barW = 3.f * scale;
    dl->AddRectFilled({x, y}, {x + barW, y + h},
        ImColor(accentColor.Value.x * 255, accentColor.Value.y * 255,
                accentColor.Value.z * 255, (int)(160 * opacity)),
        rounding, ImDrawFlags_RoundCornersLeft);

    // Icon (muted accent color)
    float curX = x + padH;
    if (icon && icon[0]) {
        ImColor iconColor = accentColor;
        iconColor.Value.w = 0.7f * opacity;
        dl->AddText(font, fs, {curX, y + padV}, iconColor, icon);
        curX += iconW + iconGap;
    }

    // Text (soft white)
    ImColor textColor(220, 225, 235, (int)(255 * opacity));
    dl->AddText(font, fs, {curX, y + padV}, textColor, text);

    return w;
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

    registerWidget(gWPing,  sIdPing,  10.f,  -14.f);
    registerWidget(gWName,  sIdName,  95.f,  -14.f);
    registerWidget(gWXYZ,   sIdXYZ,   210.f, -14.f);
    registerWidget(gWFPS,   sIdFPS,   380.f, -14.f);
    registerWidget(gWBPS,   sIdBPS,   450.f, -14.f);
    registerWidget(gWArrow, sIdArrow, 520.f, -14.f);
    registerWidget(gWPearl, sIdPearl, 600.f, -14.f);
    registerWidget(gWKicks, sIdKicks, 680.f, -14.f);
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
    if (event.mPacket->getId() == PacketID::Disconnect) mKicksAmount++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Render — one soft pill per enabled widget
// ─────────────────────────────────────────────────────────────────────────────

void LevelInfo::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto dl    = ImGui::GetBackgroundDrawList();
    float now  = (float)ImGui::GetTime();
    float fs   = mFontSize.mValue * mScale.mValue;
    float round= mRounding.mValue * mScale.mValue;
    float opacity = mOpacity.mValue;
    bool  blur  = mGlass.mValue;
    float scale = mScale.mValue;

    FontHelper::pushPrefFont(false);

    // ── Ping chip ─────────────────────────────────────────────────────────
    if (mShowPing.mValue && gWPing && gWPing->mVisible) {
        int ping = (int)mPing;
        ImColor pc = ImColor(120, 210, 170, 220); // soft teal
        if (mColorPing.mValue) {
            if      (ping > 150) pc = ImColor(210, 100, 100, 220); // soft red
            else if (ping >  80) pc = ImColor(210, 180, 90, 220);  // soft amber
        }
        char buf[32]; snprintf(buf, sizeof(buf), "%d ms", ping);
        ImVec2 pos = gWPing->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "⏱" : "", buf, pc, opacity, round, blur, scale);
        gWPing->mSize = {w, fs + 8.f * scale};
    }

    // ── Name chip ─────────────────────────────────────────────────────────
    if (mShowName.mValue && gWName && gWName->mVisible) {
        std::string raw = ColorUtils::removeColorCodes(player->getRawName());
        size_t sp = raw.rfind(' ');
        std::string name = (sp != std::string::npos) ? raw.substr(sp+1) : raw;
        if (!name.empty() && name[0] == '[') {
            size_t rb = name.find(']');
            if (rb != std::string::npos) name = name.substr(rb+1);
        }
        if (name.empty()) name = raw;

        ImColor nc = ColorUtils::getThemedColor(now * 15.f);
        nc.Value.w = 0.75f;
        ImVec2 pos = gWName->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "👤" : "", name.c_str(), nc, opacity, round, blur, scale);
        gWName->mSize = {w, fs + 8.f * scale};
    }

    // ── XYZ chip ──────────────────────────────────────────────────────────
    if (mShowXYZ.mValue && gWXYZ && gWXYZ->mVisible) {
        glm::ivec3 p = *player->getPos();
        char buf[64]; snprintf(buf, sizeof(buf), "%d / %d / %d", p.x, p.y, p.z);
        ImColor xc = ImColor(120, 180, 230, 220); // soft blue
        ImVec2 pos = gWXYZ->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "📍" : "", buf, xc, opacity, round, blur, scale);
        gWXYZ->mSize = {w, fs + 8.f * scale};
    }

    // ── FPS chip ──────────────────────────────────────────────────────────
    if (mShowFPS.mValue && gWFPS && gWFPS->mVisible) {
        int fps = (int)ImGui::GetIO().Framerate;
        ImColor fc = ImColor(120, 200, 160, 220); // soft green
        if (mColorPing.mValue) {
            if      (fps < 30) fc = ImColor(210, 100, 100, 220); // soft red
            else if (fps < 60) fc = ImColor(210, 180, 90, 220);  // soft amber
        }
        char buf[24]; snprintf(buf, sizeof(buf), "%d fps", fps);
        ImVec2 pos = gWFPS->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "⚙" : "", buf, fc, opacity, round, blur, scale);
        gWFPS->mSize = {w, fs + 8.f * scale};
    }

    // ── BPS chip ──────────────────────────────────────────────────────────
    if (mShowBPS.mValue && gWBPS && gWBPS->mVisible) {
        char buf[24]; snprintf(buf, sizeof(buf), "%.1f bps", mBps);
        ImColor bc = ImColor(200, 170, 110, 220); // soft warm
        ImVec2 pos = gWBPS->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "💨" : "", buf, bc, opacity, round, blur, scale);
        gWBPS->mSize = {w, fs + 8.f * scale};
    }

    // ── Arrows chip ───────────────────────────────────────────────────────
    if (mShowArrows.mValue && gWArrow && gWArrow->mVisible) {
        char buf[24]; snprintf(buf, sizeof(buf), "%d arrows", mArrows);
        ImColor ac = ImColor(190, 180, 140, 220); // soft gold
        ImVec2 pos = gWArrow->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "🏹" : "", buf, ac, opacity, round, blur, scale);
        gWArrow->mSize = {w, fs + 8.f * scale};
    }

    // ── Pearls chip ───────────────────────────────────────────────────────
    if (mShowEnderPearls.mValue && gWPearl && gWPearl->mVisible) {
        char buf[24]; snprintf(buf, sizeof(buf), "%d pearls", mPearls);
        ImColor pc2 = ImColor(130, 160, 220, 220); // soft indigo
        ImVec2 pos = gWPearl->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "🔮" : "", buf, pc2, opacity, round, blur, scale);
        gWPearl->mSize = {w, fs + 8.f * scale};
    }

    // ── Kicks chip ────────────────────────────────────────────────────────
    if (mShowKicksAmount.mValue && gWKicks && gWKicks->mVisible) {
        char buf[24]; snprintf(buf, sizeof(buf), "%d kicks", mKicksAmount);
        ImColor kc = ImColor(200, 120, 120, 220); // soft rose
        ImVec2 pos = gWKicks->getPos();
        float w = drawChip(dl, ImGui::GetFont(), fs, pos.x, pos.y,
            mIcons.mValue ? "⚡" : "", buf, kc, opacity, round, blur, scale);
        gWKicks->mSize = {w, fs + 8.f * scale};
    }

    ImGui::PopFont();
}
