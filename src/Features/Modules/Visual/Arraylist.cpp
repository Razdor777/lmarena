#include "Arraylist.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/EasingUtil.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <unordered_map>

// ═══════════════════════════════════════════════════════════════════════════
//  Arraylist — redesigned renderer
//  ▸ per entry plate: None / Dark / Gradient / Glass / Blur
//  ▸ accent element  : Bar / Outline / Split / Glow / None
//  ▸ smooth enable-disable animation, staggered elastic slide-in
//  ▸ hover highlight + toggle flash, click-to-toggle (optional)
// ═══════════════════════════════════════════════════════════════════════════

namespace
{
    // Состояние анимации/наведения каждого модуля держим ЛОКАЛЬНО здесь, а не в
    // Module.hpp: правка этого файла тогда не заставляет пересобирать весь проект.
    // (Модули живут всё время работы приложения, так что указатели стабильны.)
    struct ALState
    {
        float anim      = 0.f;
        float prevAnim  = 0.f;
        float hover     = 0.f;
        float burst     = 0.f;
        float y         = -99999.f;
        bool  wasEnabled = false;
    };

    std::unordered_map<Module*, ALState>& states()
    {
        static std::unordered_map<Module*, ALState> instance;
        return instance;
    }

    inline ALState& stateOf(Module* module) { return states()[module]; }

    struct ALItem
    {
        Module*     mod      = nullptr;
        std::string name;
        std::string suffix;
        std::string sortKey;      // lowercase name, used by the alphabetical sorting

        float nameW    = 0.f;
        float suffixW  = 0.f;
        float contentW = 0.f;

        float rawAnim = 0.f;      // 0..1 module animation state
        float alpha   = 0.f;
        float hover   = 0.f;
        float burst   = 0.f;
        bool  falling = false;    // entry is animating out

        ImColor color = ImColor(255, 255, 255, 255);

        float y       = -99999.f;
        float targetY = 0.f;

        // plate geometry (already animated)
        float cardMinX = 0.f;
        float cardMaxX = 0.f;
        float cardMinY = 0.f;
        float cardMaxY = 0.f;
    };

    inline float smoothStep(float t)
    {
        t = MathUtils::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    inline float clamp01(float v) { return MathUtils::clamp(v, 0.f, 1.f); }

    inline ImColor withAlpha(const ImColor& c, float mul)
    {
        return ImColor(c.Value.x, c.Value.y, c.Value.z, clamp01(c.Value.w * mul));
    }

    inline ImColor mixColor(const ImColor& a, const ImColor& b, float t)
    {
        return ImColor(
            a.Value.x + (b.Value.x - a.Value.x) * t,
            a.Value.y + (b.Value.y - a.Value.y) * t,
            a.Value.z + (b.Value.z - a.Value.z) * t,
            a.Value.w + (b.Value.w - a.Value.w) * t);
    }

    // Rounded rectangle filled with a vertical gradient
    inline void gradientRectV(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx,
                              const ImColor& top, const ImColor& bottom, float rounding)
    {
        dl->AddRectFilledMultiColor(mn, mx, top, top, bottom, bottom, rounding, ImDrawFlags_RoundCornersAll);
    }

    // Rounded rectangle filled with a horizontal gradient (left -> right)
    inline void gradientRectH(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx,
                              const ImColor& left, const ImColor& right, float rounding)
    {
        dl->AddRectFilledMultiColor(mn, mx, left, right, right, left, rounding, ImDrawFlags_RoundCornersAll);
    }

    inline ImColor categoryColor(ModuleCategory category)
    {
        switch (category)
        {
        case ModuleCategory::Combat:   return ImColor(255, 104, 104);
        case ModuleCategory::Movement: return ImColor(96, 200, 255);
        case ModuleCategory::Player:   return ImColor(120, 240, 172);
        case ModuleCategory::Visual:   return ImColor(190, 142, 255);
        case ModuleCategory::Misc:     return ImColor(255, 200, 110);
        default:                       return ImColor(225, 225, 235);
        }
    }

    inline std::string toLowerCopy(const std::string& value)
    {
        std::string out = value;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void Arraylist::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Arraylist::onRenderEvent>(this);
}

void Arraylist::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Arraylist::onRenderEvent>(this);

    states().clear();
}

// ─────────────────────────────────────────────────────────────────────────────

void Arraylist::onRenderEvent(RenderEvent& event)
{
    (void)event;

    auto* moduleManager = gFeatureManager->mModuleManager.get();
    if (!moduleManager) return;

    auto daInterface = moduleManager->getModule<Interface>();
    if (!daInterface) return;

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 screenSize = io.DisplaySize;
    if (screenSize.x <= 1.f || screenSize.y <= 1.f) return;

    const float delta = MathUtils::clamp(io.DeltaTime, 0.f, 0.1f);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    // ── Settings (values are clamped so old configs can never crash us) ─────
    const float scale    = mScale.mValue;
    const float fontSize = mFontSize.mValue * scale * 0.64f;
    if (fontSize <= 1.f) return;

    const float suffixSize = fontSize * 0.78f;

    const BackgroundStyle backgroundStyle = static_cast<BackgroundStyle>(MathUtils::clamp(mBackground.as<int>(), 0, (int)BackgroundStyle::Blur));
    const Display         accentStyle     = static_cast<Display>(MathUtils::clamp(mDisplay.as<int>(), 0, (int)Display::None));
    const ColorMode       colorMode       = static_cast<ColorMode>(MathUtils::clamp(mColorMode.as<int>(), 0, (int)ColorMode::Solid));
    const Side            side            = static_cast<Side>(MathUtils::clamp(mSide.as<int>(), 0, (int)Side::Left));
    const Sorting         sorting         = static_cast<Sorting>(MathUtils::clamp(mSorting.as<int>(), 0, (int)Sorting::Register));

    const bool rightSide = (side == Side::Right);
    const bool panelMode = (backgroundStyle == BackgroundStyle::Blur || backgroundStyle == BackgroundStyle::Glass);
    const bool hasPlate  = (backgroundStyle != BackgroundStyle::None);

    // Font atlas has a 20px and a 42px variant — picking the closer one keeps the text crisp
    FontHelper::pushPrefFont(fontSize > 24.f, mBoldText.mValue, false);
    ImFont* font = ImGui::GetFont();
    if (!font)
    {
        FontHelper::popPrefFont();
        return;
    }

    // ── Metrics ─────────────────────────────────────────────────────────────
    const float rounding     = mRounding.mValue * scale;
    const float padX         = hasPlate ? mPadding.mValue * scale : fontSize * 0.25f;
    const float padY         = hasPlate ? fontSize * 0.30f : fontSize * 0.10f;
    const float rowH         = fontSize + padY * 2.f;
    const float spacing      = panelMode ? 0.f : mSpacing.mValue * scale;
    const float step         = rowH + spacing;
    const float accentGap    = fontSize * 0.24f;
    const float accentW      = std::max(2.f, fontSize * 0.16f);
    const float suffixGap    = fontSize * 0.42f;
    const float edge         = mOffsetX.mValue * scale;
    const float minY         = mOffsetY.mValue * scale;
    const float maxContentW  = screenSize.x * 0.55f;

    // ── Animation state ─────────────────────────────────────────────────────
    auto& modules = moduleManager->getModules();

    for (auto& mod : modules)
    {
        if (!mod) continue;
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;

        ALState& st = stateOf(mod.get());

        // Any state change (keybind, ClickGui, command) plays the flash
        if (mod->mEnabled != st.wasEnabled)
        {
            st.burst = 1.f;
            st.wasEnabled = mod->mEnabled;
        }

        const float target = mod->mEnabled ? 1.f : 0.f;
        const float speed  = (target > st.anim) ? mEnableAnimSpeed.mValue : mDisableAnimSpeed.mValue;

        st.anim = MathUtils::lerp(st.anim, target, clamp01(delta * speed));
        if (std::abs(st.anim - target) < 0.002f) st.anim = target;
        st.anim = clamp01(st.anim);

        st.burst = std::max(0.f, st.burst - delta * 2.6f);
    }

    // ── Collect entries ─────────────────────────────────────────────────────
    std::vector<ALItem> items;
    items.reserve(modules.size());

    int colorIndex = 0;
    for (auto& mod : modules)
    {
        if (!mod) continue;
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;

        const ALState& st = stateOf(mod.get());
        if (st.anim < 0.004f) continue;

        ALItem item;
        item.mod      = mod.get();
        item.name     = mod->getName();
        item.suffix   = mRenderMode.mValue ? mod->getSettingDisplayText() : std::string();
        item.rawAnim  = st.anim;
        item.nameW    = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, item.name.c_str()).x;
        item.suffixW  = item.suffix.empty() ? 0.f : font->CalcTextSizeA(suffixSize, FLT_MAX, 0.f, item.suffix.c_str()).x;
        item.contentW = std::min(item.nameW + (item.suffix.empty() ? 0.f : suffixGap + item.suffixW), maxContentW);

        switch (colorMode)
        {
        case ColorMode::Rainbow:
            item.color = ColorUtils::Rainbow(daInterface->mColorSpeed.mValue, daInterface->mSaturation.mValue, 1.f, colorIndex * 30);
            break;
        case ColorMode::Category:
            item.color = categoryColor(mod->mCategory);
            break;
        case ColorMode::Solid:
            item.color = mSolidColor.getAsImColor();
            break;
        default:
            item.color = ColorUtils::getThemedColor(static_cast<float>(colorIndex) * 30.f);
            break;
        }

        if (sorting == Sorting::Alphabetical) item.sortKey = toLowerCopy(item.name);

        items.push_back(std::move(item));
        colorIndex++;
    }

    if (items.empty())
    {
        FontHelper::popPrefFont();
        return;
    }

    // ── Sorting ─────────────────────────────────────────────────────────────
    switch (sorting)
    {
    case Sorting::Width:
        std::stable_sort(items.begin(), items.end(),
            [](const ALItem& a, const ALItem& b) { return a.contentW > b.contentW; });
        break;
    case Sorting::Alphabetical:
        std::stable_sort(items.begin(), items.end(),
            [](const ALItem& a, const ALItem& b) { return a.sortKey < b.sortKey; });
        break;
    default:
        break;
    }

    // ── Vertical layout ─────────────────────────────────────────────────────
    float cursorY = minY;
    float widestContent = 0.f;

    for (auto& item : items)
    {
        ALState& st = stateOf(item.mod);

        // Direction of the animation (a module that is being disabled plays the outro)
        item.falling = item.rawAnim < st.prevAnim - 0.0005f;
        st.prevAnim  = item.rawAnim;

        item.alpha = clamp01(EasingUtil::easeOutQuad(item.rawAnim));
        item.burst = clamp01(st.burst);

        item.y = st.y;
        item.targetY = cursorY;
        if (item.y < -9000.f) item.y = item.targetY;
        item.y = MathUtils::lerp(item.y, item.targetY, clamp01(delta * 15.f));
        if (std::abs(item.y - item.targetY) < 0.15f) item.y = item.targetY;
        st.y = item.y;

        // Collapsing/expanding height keeps the list below perfectly synced
        cursorY += step * smoothStep(item.rawAnim);
        widestContent = std::max(widestContent, item.contentW);
    }

    // ── Horizontal geometry + hover ─────────────────────────────────────────
    float panelMinX = 0.f, panelMaxX = 0.f;
    if (panelMode)
    {
        // The accent bar of panel mode lives *inside* the panel, so the panel itself
        // can bleed all the way to the screen edge (minus the configured offset).
        const float panelW = widestContent + padX * 2.f + accentW + accentGap;
        if (rightSide)
        {
            panelMaxX = screenSize.x - edge;
            panelMinX = panelMaxX - panelW;
        }
        else
        {
            panelMinX = edge;
            panelMaxX = panelMinX + panelW;
        }
    }

    const bool guiOpen = [&]() {
        auto* clickGui = moduleManager->getModule<ClickGui>();
        return clickGui && clickGui->mGuiOpen;
    }();
    // getMouseGrabbed() reports whether the cursor is currently free (the wrapper is inverted),
    // so the list is only interactive while the player can actually point at it.
    auto* client = ClientInstance::get();
    const bool cursorVisible = client && client->getMouseGrabbed();
    const bool interactive = mHoverEffect.mValue && !guiOpen && cursorVisible;
    const ImVec2 mouse = io.MousePos;

    for (auto& item : items)
    {
        const float rowW = item.contentW + padX * 2.f;

        if (panelMode)
        {
            item.cardMinX = panelMinX;
            item.cardMaxX = panelMaxX;
        }
        else if (rightSide)
        {
            item.cardMaxX = screenSize.x - edge - accentW - accentGap;
            item.cardMinX = item.cardMaxX - rowW;
        }
        else
        {
            item.cardMinX = edge + accentW + accentGap;
            item.cardMaxX = item.cardMinX + rowW;
        }

        // Slide in from off-screen, slide out back
        const float hiddenOffset = rightSide ? (screenSize.x + 40.f) : (-rowW - 40.f);

        float slide;
        if (item.falling)
        {
            slide = EasingUtil::easeInBack(item.rawAnim);
        }
        else if (mElasticAnim.mValue && item.mod->mEnabled)
        {
            slide = EasingUtil::easeOutElastic(item.rawAnim);
        }
        else
        {
            slide = EasingUtil::easeOutExpo(item.rawAnim);
        }

        if (mStaggerAnim.mValue && !item.falling && !panelMode)
        {
            const float delay = static_cast<float>(&item - items.data()) * 0.03f;
            if (delay > 0.f)
            {
                const float raw = clamp01((item.rawAnim - delay) / std::max(0.05f, 1.f - delay));
                slide = mElasticAnim.mValue && item.mod->mEnabled
                    ? EasingUtil::easeOutElastic(raw)
                    : EasingUtil::easeOutExpo(raw);
            }
        }

        const float offset = MathUtils::lerp(hiddenOffset, 0.f, slide);
        item.cardMinX += offset;
        item.cardMaxX += offset;
        item.cardMinY = item.y;
        item.cardMaxY = item.y + rowH;

        // Hover — measured on the base plate so the highlight never fights itself
        float hoverMinX = item.cardMinX;
        float hoverMaxX = item.cardMaxX;
        if (rightSide) hoverMaxX += accentGap + accentW;
        else           hoverMinX -= accentGap + accentW;

        float hoverTarget = 0.f;
        if (interactive &&
            mouse.x >= hoverMinX - 2.f && mouse.x <= hoverMaxX + 2.f &&
            mouse.y >= item.cardMinY - 1.f && mouse.y <= item.cardMaxY + 1.f)
        {
            hoverTarget = 1.f;
        }

        ALState& st = stateOf(item.mod);
        st.hover = MathUtils::lerp(st.hover, hoverTarget, clamp01(delta * 14.f));
        item.hover = clamp01(st.hover);

        if (mClickToggle.mValue && hoverTarget > 0.f && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            st.burst = 1.f;
            item.mod->toggle();
        }

        item.mod->pos.x = item.cardMinX;
        item.mod->pos.y = item.y;
    }

    // ══════════════════════════════════════════════════════════════════════
    //  RENDER PASS
    // ══════════════════════════════════════════════════════════════════════

    float minRowY = FLT_MAX, maxRowY = -FLT_MAX, panelAlpha = 0.f;
    for (auto& item : items)
    {
        minRowY = std::min(minRowY, item.cardMinY);
        maxRowY = std::max(maxRowY, item.cardMaxY);
        panelAlpha = std::max(panelAlpha, item.alpha);
    }

    // ▸ Shared panel (Blur / Glass)
    if (panelMode)
    {
        const ImVec2 pMin(panelMinX, minRowY);
        const ImVec2 pMax(panelMaxX, maxRowY);

        if (backgroundStyle == BackgroundStyle::Blur)
        {
            ImRenderUtils::addBlur(ImVec4(pMin.x, pMin.y, pMax.x, pMax.y), mBlurStrength.mValue, rounding, dl, true);
            dl->AddRectFilled(pMin, pMax, ImColor(9, 9, 15, static_cast<int>(135 * panelAlpha)), rounding);
            dl->AddRect(pMin, pMax, ImColor(255, 255, 255, static_cast<int>(14 * panelAlpha)), rounding, 0, 1.f);
        }
        else // Glass
        {
            gradientRectV(dl, pMin, pMax,
                ImColor(255, 255, 255, static_cast<int>(26 * panelAlpha)),
                ImColor(255, 255, 255, static_cast<int>(4 * panelAlpha)), rounding);
            dl->AddRectFilled(pMin, pMax, ImColor(10, 10, 17, static_cast<int>(150 * panelAlpha)), rounding);
            dl->AddRect(pMin, pMax, ImColor(255, 255, 255, static_cast<int>(22 * panelAlpha)), rounding, 0, 1.f);
        }

        // Top inner highlight — sells the "glass" look
        dl->AddRectFilled(
            ImVec2(pMin.x + rounding, pMin.y + 1.f),
            ImVec2(pMax.x - rounding, pMin.y + 1.5f),
            ImColor(255, 255, 255, static_cast<int>(18 * panelAlpha)), 0.f);
    }

    // ▸ Plates (per entry backgrounds)
    for (auto& item : items)
    {
        if (item.alpha <= 0.01f) continue;
        if (panelMode)
        {
            // Hover / flash highlight inside the shared panel
            if (item.hover > 0.01f || item.burst > 0.01f)
            {
                const ImVec2 hMin(item.cardMinX + 3.f, item.cardMinY + 1.f);
                const ImVec2 hMax(item.cardMaxX - 3.f, item.cardMaxY - 1.f);
                dl->AddRectFilled(hMin, hMax,
                    ImColor(255, 255, 255, static_cast<int>((6.f * item.hover + 22.f * item.burst) * item.alpha)),
                    3.f, ImDrawFlags_RoundCornersAll);
            }
            continue;
        }

        const float grow = (item.hover * 7.f + item.burst * 5.f) * scale;
        ImVec2 mn(item.cardMinX, item.cardMinY);
        ImVec2 mx(item.cardMaxX, item.cardMaxY);
        if (rightSide) mn.x -= grow; else mx.x += grow;

        const ImColor accent = item.color;
        switch (backgroundStyle)
        {
        case BackgroundStyle::Dark:
        {
            dl->AddRectFilled(mn, mx, ImColor(9, 9, 15, static_cast<int>((200 + 25 * item.hover) * item.alpha)), rounding);
            dl->AddRect(mn, mx, ImColor(255, 255, 255, static_cast<int>((10 + 14 * item.hover) * item.alpha)), rounding, 0, 1.f);
            break;
        }
        case BackgroundStyle::Gradient:
        {
            dl->AddRectFilled(mn, mx, ImColor(9, 9, 15, static_cast<int>(205 * item.alpha)), rounding);
            gradientRectV(dl, mn, mx,
                withAlpha(accent, 0.42f * item.alpha),
                withAlpha(accent, 0.05f * item.alpha), rounding);
            dl->AddRect(mn, mx, withAlpha(accent, 0.35f * item.alpha), rounding, 0, 1.f);
            break;
        }
        case BackgroundStyle::Glass:
        {
            gradientRectV(dl, mn, mx,
                ImColor(255, 255, 255, static_cast<int>((30 + 30 * item.hover) * item.alpha)),
                ImColor(255, 255, 255, static_cast<int>(6 * item.alpha)), rounding);
            dl->AddRectFilled(mn, mx, ImColor(10, 10, 17, static_cast<int>((145 + 35 * item.hover) * item.alpha)), rounding);
            dl->AddRect(mn, mx, ImColor(255, 255, 255, static_cast<int>((22 + 20 * item.hover) * item.alpha)), rounding, 0, 1.f);
            break;
        }
        default:
            break;
        }

        if (item.burst > 0.01f)
        {
            dl->AddRectFilled(mn, mx, ImColor(255, 255, 255, static_cast<int>(26 * item.burst * item.alpha)), rounding);
        }
    }

    // ▸ Accent elements + text
    for (auto& item : items)
    {
        if (item.alpha <= 0.01f) continue;

        const ImColor accent = item.color;
        const float grow = (item.hover * 7.f + item.burst * 5.f) * scale;
        const bool accentOutside = !panelMode;

        // Accent bar geometry (outside the plate in every style except the panel ones)
        float barMinX = 0.f, barMaxX = 0.f;
        if (accentOutside)
        {
            const float cardEdge = rightSide ? item.cardMaxX : item.cardMinX;
            const float barW = accentW + (item.hover * 1.2f + item.burst * 1.5f) * scale;
            if (rightSide)
            {
                barMaxX = cardEdge + accentGap + accentW;
                barMinX = barMaxX - barW;
            }
            else
            {
                barMinX = cardEdge - accentGap - accentW;
                barMaxX = barMinX + barW;
            }
        }
        else
        {
            const float barW = accentW + (item.hover * 1.2f + item.burst * 1.5f) * scale;
            const float inset = 1.5f;
            if (rightSide)
            {
                barMaxX = item.cardMaxX - inset;
                barMinX = barMaxX - barW;
            }
            else
            {
                barMinX = item.cardMinX + inset;
                barMaxX = barMinX + barW;
            }
        }

        const ImVec2 barMin(barMinX, item.cardMinY + 2.5f);
        const ImVec2 barMax(barMaxX, item.cardMaxY - 2.5f);
        const float barRounding = std::min(barMax.x - barMin.x, barMax.y - barMin.y) * 0.5f;

        switch (accentStyle)
        {
        case Display::Bar:
        {
            dl->AddRectFilled(barMin, barMax, withAlpha(accent, item.alpha), barRounding);
            break;
        }
        case Display::Outline:
        {
            ImVec2 mn(item.cardMinX, item.cardMinY), mx(item.cardMaxX, item.cardMaxY);
            if (rightSide) mn.x -= grow; else mx.x += grow;
            dl->AddRectFilled(mn, mx, withAlpha(accent, 0.07f * item.alpha), rounding);
            dl->AddRect(mn, mx, withAlpha(accent, (0.75f + 0.25f * item.hover) * item.alpha), rounding, 0, 1.6f * scale);
            break;
        }
        case Display::Split:
        {
            ImVec2 mn(item.cardMinX, item.cardMinY), mx(item.cardMaxX, item.cardMaxY);
            if (rightSide) mn.x -= grow; else mx.x += grow;
            if (rightSide)
                gradientRectH(dl, mn, mx, withAlpha(accent, 0.f), withAlpha(accent, 0.38f * item.alpha), rounding);
            else
                gradientRectH(dl, mn, mx, withAlpha(accent, 0.38f * item.alpha), withAlpha(accent, 0.f), rounding);
            dl->AddRectFilled(barMin, barMax, withAlpha(accent, item.alpha), barRounding);
            break;
        }
        case Display::Glow:
        {
            ImVec2 mn(item.cardMinX, item.cardMinY), mx(item.cardMaxX, item.cardMaxY);
            if (rightSide) mn.x -= grow; else mx.x += grow;
            // Bottom underline + bloom around the plate
            dl->AddRectFilled(
                ImVec2(mn.x + rounding, mx.y - 1.6f * scale),
                ImVec2(mx.x - rounding, mx.y - 0.4f * scale),
                withAlpha(accent, 0.9f * item.alpha), 1.f);
            dl->AddShadowRect(mn, mx, withAlpha(accent, (0.35f + 0.25f * item.hover) * item.alpha),
                14.f * scale * item.alpha, ImVec2(0.f, 0.f),
                ImDrawFlags_ShadowCutOutShapeBackground, rounding);
            break;
        }
        default:
            break;
        }

        // Neon bloom behind the accent bar
        if (mGlow.mValue && (accentStyle == Display::Bar || accentStyle == Display::Split))
        {
            dl->AddShadowRect(barMin, barMax,
                withAlpha(accent, (0.45f + 0.30f * item.hover) * item.alpha),
                mGlowStrength.mValue * 22.f * item.alpha * scale, ImVec2(0.f, 0.f),
                ImDrawFlags_ShadowCutOutShapeBackground, barRounding);
        }

        // ── Text ────────────────────────────────────────────────────────────
        const float textShift = item.hover * 2.f * scale;
        const float textX = item.cardMinX + padX + (rightSide ? -textShift : textShift);
        const ImVec2 nameSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, item.name.c_str());
        const float textY = item.cardMinY + (rowH - nameSize.y) * 0.5f;

        if (mTextShadow.mValue)
        {
            const float so = std::max(1.f, fontSize * 0.07f);
            dl->AddText(font, fontSize, ImVec2(textX + so, textY + so),
                ImColor(0, 0, 0, static_cast<int>(150 * item.alpha)), item.name.c_str());
        }
        dl->AddText(font, fontSize, ImVec2(textX, textY),
            ImColor(255, 255, 255, static_cast<int>((238 + 17 * item.hover) * item.alpha)), item.name.c_str());

        if (!item.suffix.empty())
        {
            const ImVec2 suffixSize2 = font->CalcTextSizeA(suffixSize, FLT_MAX, 0.f, item.suffix.c_str());
            const float suffixX = textX + item.nameW + suffixGap;
            const float suffixY = item.cardMinY + (rowH - suffixSize2.y) * 0.5f;

            const ImColor suffixColor = mAccentSuffix.mValue
                ? withAlpha(accent, (0.80f + 0.20f * item.hover) * item.alpha)
                : ImColor(196, 200, 214, static_cast<int>((190 + 40 * item.hover) * item.alpha));

            if (mTextShadow.mValue)
            {
                const float so = std::max(1.f, suffixSize * 0.07f);
                dl->AddText(font, suffixSize, ImVec2(suffixX + so, suffixY + so),
                    ImColor(0, 0, 0, static_cast<int>(140 * item.alpha)), item.suffix.c_str());
            }
            dl->AddText(font, suffixSize, ImVec2(suffixX, suffixY), suffixColor, item.suffix.c_str());
        }
    }

    FontHelper::popPrefFont();
}
