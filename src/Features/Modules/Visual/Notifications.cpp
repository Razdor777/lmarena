//
// Notifications — Ultra Premium Edition
// Glassmorphism + particles + shimmer sweep + wave + geometric icons
//

#include "Notifications.hpp"
#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Events/NotifyEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/FontHelper.hpp>

// Strip Minecraft §X colour/format codes from a UTF-8 string.
// § = U+00A7 = 0xC2 0xA7 in UTF-8 (or rarely 0xA7 alone in latin-1 builds)
static std::string stripMCFormatting(const std::string& src)
{
    std::string out;
    out.reserve(src.size());
    for (size_t i = 0; i < src.size(); )
    {
        unsigned char c = (unsigned char)src[i];
        // UTF-8 § = 0xC2 0xA7 — skip § (2 bytes) + format char (1 byte) = 3 bytes
        if (c == 0xC2 && i + 2 < src.size() && (unsigned char)src[i+1] == 0xA7) {
            i += 3;
            continue;
        }
        // Raw § byte 0xA7 — skip § (1 byte) + format char (1 byte) = 2 bytes
        if (c == 0xA7 && i + 1 < src.size()) {
            i += 2;
            continue;
        }
        out += src[i++];
    }
    return out.empty() ? src : out;
}

void Notifications::onEnable()
{
    gFeatureManager->mDispatcher->listen<NotifyEvent,           &Notifications::onNotifyEvent>(this);
    gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent,&Notifications::onModuleStateChange>(this);
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent,&Notifications::onConnectionRequestEvent>(this);
}

void Notifications::onDisable()
{
    gFeatureManager->mDispatcher->deafen<NotifyEvent,           &Notifications::onNotifyEvent>(this);
    gFeatureManager->mDispatcher->deafen<ModuleStateChangeEvent,&Notifications::onModuleStateChange>(this);
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent,&Notifications::onConnectionRequestEvent>(this);
}

ImColor Notifications::getTypeColor(Notification::Type type, float offset)
{
    switch (type) {
        case Notification::Type::Info:    return ColorUtils::getThemedColor(offset);
        case Notification::Type::Warning: return ImColor(255, 195, 50);
        case Notification::Type::Error:   return ImColor(240, 70, 70);
        default: return ImColor(200, 200, 200);
    }
}

// ── Geometric icon ────────────────────────────────────────────────────────────
static void drawNotifIcon(ImDrawList* dl, ImVec2 center, float r, Notification::Type type, ImColor col, float alpha)
{
    col.Value.w = alpha;
    ImColor white(255, 255, 255, (int)(220 * alpha));

    switch (type) {
    case Notification::Type::Warning: {
        // Filled triangle + ! mark
        float h = r * 1.8f;
        ImVec2 p0(center.x,       center.y - h * 0.55f);
        ImVec2 p1(center.x - h * 0.55f, center.y + h * 0.45f);
        ImVec2 p2(center.x + h * 0.55f, center.y + h * 0.45f);
        dl->AddTriangleFilled(p0, p1, p2, col);
        // ! body
        dl->AddRectFilled(ImVec2(center.x - 1.f, center.y - 5.f),
                          ImVec2(center.x + 1.f, center.y + 1.f), white, 1.f);
        // ! dot
        dl->AddCircleFilled(ImVec2(center.x, center.y + 4.f), 1.2f, white, 6);
        break;
    }
    case Notification::Type::Error: {
        // Circle X
        dl->AddCircleFilled(center, r * 1.1f, col, 16);
        float c = r * 0.6f;
        dl->AddLine({center.x - c, center.y - c}, {center.x + c, center.y + c}, white, 2.f);
        dl->AddLine({center.x + c, center.y - c}, {center.x - c, center.y + c}, white, 2.f);
        break;
    }
    default: {
        // Circle with i
        dl->AddCircleFilled(center, r * 1.1f, col, 16);
        dl->AddCircleFilled({center.x, center.y - r * 0.4f}, 1.3f, white, 6);
        dl->AddRectFilled({center.x - 1.f, center.y - r * 0.05f},
                          {center.x + 1.f, center.y + r * 0.65f}, white, 1.f);
        break;
    }
    }
}

void Notifications::renderModern(ImDrawList* drawList, ImVec2 displaySize, float delta)
{
    float animSpeed = mAnimSpeed.mValue;
    float baseY     = displaySize.y - 16.f;
    const float padX    = 16.f;
    const float padY    = 10.f;
    const float rounding = 12.f;
    const float fontSize = 14.5f;
    const float accentW = 3.5f;
    const float iconAreaW = 28.f;

    int shown = 0;
    for (auto& n : mNotifications)
    {
        if (mLimitNotifications.mValue && shown >= mMaxNotifications.mValue) break;

        n.mTimeShown += delta;
        n.mIsTimeUp   = n.mTimeShown >= n.mDuration;

        float target = n.mIsTimeUp ? 0.f : 1.f;
        n.mCurrentDuration = MathUtils::lerp(n.mCurrentDuration, target, delta * animSpeed);

        float anim = n.mCurrentDuration;
        if (anim < 0.002f && n.mIsTimeUp) continue;

        // ── Spawn easing ─────────────────────────────────────────────────────
        float spawnT = MathUtils::clamp(n.mTimeShown / 0.3f, 0.f, 1.f);
        EasingUtil spawnEase; spawnEase.percentage = spawnT;
        float spawnScale = spawnEase.easeOutBack();

        // ── Dismiss easing ───────────────────────────────────────────────────
        float dismissT = n.mIsTimeUp ? MathUtils::clamp((n.mTimeShown - n.mDuration) / 0.4f, 0.f, 1.f) : 0.f;
        EasingUtil dismissEase; dismissEase.percentage = dismissT;
        float dismissSlide = dismissEase.easeInBack();

        float alpha = anim * spawnScale;

        const std::string stripped = stripMCFormatting(n.mMessage);

        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, stripped.c_str());
        float boxW = ts.x + padX * 2.f + iconAreaW + accentW;
        float boxH = ts.y + padY * 2.f;

        // ── Slide from right with elastic ────────────────────────────────────
        float slideTarget = displaySize.x - boxW - 14.f;
        float slideHidden = displaySize.x + boxW + 30.f;

        EasingUtil slideEase; slideEase.percentage = spawnT;
        float slideX = MathUtils::lerp(slideHidden, slideTarget, slideEase.easeOutElastic());
        slideX += dismissSlide * (boxW + 60.f);          // fly right on dismiss

        // ── Vertical stacking ────────────────────────────────────────────────
        baseY -= (boxH + 6.f) * anim;
        float y = baseY;

        ImVec2 min(slideX, y);
        ImVec2 max(slideX + boxW, y + boxH);

        // ── Hover ────────────────────────────────────────────────────────────
        ImVec2 mp = ImGui::GetIO().MousePos;
        bool hovered = mp.x >= min.x && mp.x <= max.x && mp.y >= min.y && mp.y <= max.y;
        if (hovered && ImGui::IsMouseClicked(0)) {
            n.mIsTimeUp = true;
            n.mTimeShown = n.mDuration;
        }
        n.hoverScale = MathUtils::lerp(n.hoverScale, hovered ? 1.f : 0.f, delta * 18.f);

        // ── Shimmer sweep ─────────────────────────────────────────────────────
        n.shimmerOffset = MathUtils::lerp(n.shimmerOffset, n.mIsTimeUp ? 0.f : 1.3f, delta * 0.9f);
        if (n.mTimeShown < 0.05f) n.shimmerOffset = -0.1f;

        ImColor accent = getTypeColor(n.mType, y * 2.8f);
        accent.Value.w = alpha;

        // ── Layer 1: outer glow ───────────────────────────────────────────────
        if (alpha > 0.05f) {
            ImColor glowCol = accent;
            glowCol.Value.w = alpha * 0.18f;
            drawList->AddShadowRect(
                {min.x - 4, min.y - 4}, {max.x + 4, max.y + 4},
                glowCol, 28.f, {0, 0}, 0, rounding + 3.f);
        }

        // ── Layer 2: blur (glassmorphism) ─────────────────────────────────────
        ImRenderUtils::addBlur(ImVec4(min.x, min.y, max.x, max.y), alpha * 4.5f, rounding);

        // ── Layer 3: translucent dark glass ──────────────────────────────────
        {
            ImColor bg(18, 18, 24, (int)(210 * alpha));
            bg.Value.x += 0.025f * n.hoverScale;
            bg.Value.y += 0.025f * n.hoverScale;
            bg.Value.z += 0.04f  * n.hoverScale;
            drawList->AddRectFilled(min, max, bg, rounding);
        }

        // ── Layer 4: top highlight edge (glass rim) ───────────────────────────
        {
            ImColor rim(255, 255, 255, (int)(18 * alpha));
            drawList->AddRectFilled(
                {min.x + rounding * 0.5f, min.y},
                {max.x - rounding * 0.5f, min.y + 1.2f},
                rim, 0.6f);
        }

        // ── Layer 5: drop shadow ──────────────────────────────────────────────
        drawList->AddShadowRect(min, max, ImColor(0, 0, 0, (int)(80 * alpha)),
            20.f, {0, 5}, 0, rounding);

        // ── Layer 6: shimmer sweep across the card ────────────────────────────
        if (!n.mIsTimeUp && n.shimmerOffset > 0.f && n.shimmerOffset < 1.2f) {
            float sx = min.x + (max.x - min.x) * n.shimmerOffset;
            float shimW = 40.f;
            // Clip to card bounds manually
            float clipL = std::max(min.x, sx - shimW);
            float clipR = std::min(max.x, sx + shimW * 0.5f);
            if (clipR > clipL) {
                ImColor shimmerCol(255, 255, 255, (int)(20 * alpha));
                drawList->AddRectFilled(
                    {clipL, min.y}, {clipR, max.y},
                    shimmerCol, rounding);
            }
        }

        // ── Accent side strip ─────────────────────────────────────────────────
        float stripH = boxH - rounding * 0.6f;
        float stripY = min.y + rounding * 0.3f;
        drawList->AddRectFilled(
            {min.x + 1.5f, stripY},
            {min.x + 1.5f + accentW, stripY + stripH},
            accent, accentW * 0.5f);

        // ── Accent glow ───────────────────────────────────────────────────────
        {
            ImColor glowA = accent; glowA.Value.w = alpha * 0.55f;
            drawList->AddShadowRect(
                {min.x, stripY}, {min.x + accentW + 3.f, stripY + stripH},
                glowA, 12.f, {0, 0}, 0, accentW * 0.5f);
        }

        // ── Geometric icon ────────────────────────────────────────────────────
        ImVec2 iconCenter(min.x + accentW + 4.f + iconAreaW * 0.5f, min.y + boxH * 0.5f);
        drawNotifIcon(drawList, iconCenter, 5.5f, n.mType, accent, alpha);

        // ── Text ──────────────────────────────────────────────────────────────
        float textX = min.x + accentW + 4.f + iconAreaW;
        float textY = min.y + padY - 0.5f;
        drawList->AddText(ImGui::GetFont(), fontSize,
            {textX, textY},
            ImColor(235, 235, 248, (int)(255 * alpha)),
            stripped.c_str());

        // ── Progress bar (thin, elegant) ──────────────────────────────────────
        float pct = std::clamp(n.mTimeShown / n.mDuration, 0.f, 1.f);
        float remaining = 1.f - pct;
        float progW = (max.x - textX) * remaining;
        if (progW > 0.5f && !n.mIsTimeUp) {
            float py = max.y - 2.f;
            // Track
            drawList->AddRectFilled({textX, py - 0.5f}, {max.x, py + 1.5f},
                ImColor(255, 255, 255, (int)(25 * alpha)), 1.f);
            // Fill
            ImColor prog = accent; prog.Value.w = 0.7f * alpha;
            drawList->AddRectFilled({textX, py - 0.5f}, {textX + progW, py + 1.5f}, prog, 1.f);
            // Glow dot at tip
            ImColor tipGlow = accent; tipGlow.Value.w = alpha * 0.9f;
            drawList->AddCircleFilled({textX + progW, py + 0.5f}, 2.5f, tipGlow, 8);
        }

        // ── Wave pulse ring on spawn ──────────────────────────────────────────
        if (n.mTimeShown < 0.5f && spawnT < 0.85f) {
            float waveAlpha = (1.f - spawnT) * alpha * 0.35f;
            float waveR = spawnT * 30.f;
            ImColor waveCol = accent; waveCol.Value.w = waveAlpha;
            drawList->AddCircle(iconCenter, waveR, waveCol, 20, 2.f);
        }

        if (!n.mIsTimeUp) shown++;
    }
}

void Notifications::onRenderEvent(RenderEvent& event)
{
    FontHelper::pushPrefFont(false);
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    auto drawList = ImGui::GetBackgroundDrawList();
    float delta = ImGui::GetIO().DeltaTime;

    std::erase_if(mNotifications, [](const Notification& n) {
        return n.mIsTimeUp && n.mTimeShown > n.mDuration + 3.f;
    });

    renderModern(drawList, ds, delta);
    ImGui::PopFont();
}

void Notifications::onModuleStateChange(ModuleStateChangeEvent& event)
{
    if (event.isCancelled() || !mShowOnToggle.mValue) return;
    mNotifications.push_back(Notification(
        event.mModule->getName() + " " + (event.mEnabled ? "enabled" : "disabled"),
        Notification::Type::Info, 3.0f));
}

void Notifications::onConnectionRequestEvent(ConnectionRequestEvent& event)
{
    if (!mShowOnJoin.mValue) return;
    mNotifications.push_back(Notification(
        "Connecting to " + *event.mServerAddress + "...",
        Notification::Type::Info, 6.0f));
}

void Notifications::onNotifyEvent(NotifyEvent& event)
{
    mNotifications.push_back(event.mNotification);
}