#include "CombatantGui.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Modules/ModuleCategory.hpp>
#include <Features/Modules/ModuleManager.hpp>
#include <Features/Modules/Setting.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>

#include <Windows.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <string>

// Unity-сборка: все вспомогательные имена живут в собственном namespace,
// чтобы не столкнуться с анонимными namespace других .cpp в одном чанке.
namespace CombatantGuiImpl
{
    // Геометрия (логические px, умножаются на UI scale) — ModulesMenuScreen.
    constexpr float kPanelW  = 128.f;
    constexpr float kHeaderH = 26.f;
    constexpr float kRowH    = 21.f;
    constexpr float kNumberH = 31.f;
    constexpr float kPad     = 4.f;
    constexpr float kMaxList = 300.f;
    constexpr float kRound   = 9.f;
    constexpr float kGap     = 12.f;
    constexpr float kTop     = 64.f;
    constexpr float kFontPx  = 14.f;

    // Цвета — combatant (ModulesMenuStyle / Themes default): ON/OFF и стекло.
    constexpr uint32_t kGlass    = 0xB30C0C0C;
    constexpr uint32_t kStroke   = 0x2EFFFFFF;
    constexpr uint32_t kHeadBg   = 0x14FFFFFF;
    constexpr uint32_t kHoverBg  = 0x12FFFFFF;
    constexpr uint32_t kText     = 0xFFFFFFFF;
    constexpr uint32_t kMuted    = 0xBFFFFFFF;
    constexpr uint32_t kFaint    = 0x80FFFFFF;
    constexpr uint32_t kOn       = 0xFF5DE878;
    constexpr uint32_t kOff      = 0xFFFF5E66;
    constexpr uint32_t kTrackOff = 0x40FFFFFF;

    ImU32 argb(uint32_t c, float a)
    {
        const float alpha = static_cast<float>((c >> 24) & 0xFF) * std::clamp(a, 0.f, 1.f);
        return IM_COL32(static_cast<int>((c >> 16) & 0xFF),
                        static_cast<int>((c >> 8) & 0xFF),
                        static_cast<int>(c & 0xFF),
                        static_cast<int>(alpha));
    }

    // Акцент берём из темы клиента (как InfiniteAura / ModernGui).
    ImU32 accent(float a)
    {
        ImVec4 c = ColorUtils::getStaticAccentColor(0.f).Value;
        c.w = std::clamp(a, 0.f, 1.f);
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    float approach(float cur, float tgt, float dt, float speed)
    {
        const float v = tgt + (cur - tgt) * std::exp(-speed * dt);
        return std::fabs(v - tgt) < 0.001f ? tgt : v;
    }

    bool inside(const ImVec4& r, ImVec2 p)
    {
        return p.x >= r.x && p.y >= r.y && p.x < r.z && p.y < r.w;
    }

    std::string keyName(int vk)
    {
        if (vk <= 0) return "None";
        switch (vk)
        {
        case VK_SHIFT:    return "Shift";
        case VK_CONTROL:  return "Ctrl";
        case VK_MENU:     return "Alt";
        case VK_SPACE:    return "Space";
        case VK_TAB:      return "Tab";
        case VK_CAPITAL:  return "Caps";
        case VK_XBUTTON1: return "Mouse4";
        case VK_XBUTTON2: return "Mouse5";
        default: break;
        }
        if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
        if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
            return std::string(1, static_cast<char>(vk));
        return "K" + std::to_string(vk);
    }

    std::string fmtValue(float v, float step)
    {
        const int d = step >= 1.f ? 0 : step >= 0.1f ? 1 : step >= 0.01f ? 2 : 3;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.*f", d, static_cast<double>(v));
        return buf;
    }

    float textWidth(const std::string& s, float px)
    {
        return ImGui::GetFont()->CalcTextSizeA(px, FLT_MAX, 0.f, s.c_str()).x;
    }

    void drawText(ImDrawList* dl, ImVec2 p, const std::string& s, ImU32 col, float px)
    {
        dl->AddText(ImGui::GetFont(), px, ImVec2(std::floor(p.x), std::floor(p.y)), col, s.c_str());
    }
}

using namespace CombatantGuiImpl;

// ---------------------------------------------------------------------------
// Преобразования координат
// ---------------------------------------------------------------------------

ImVec2 CombatantGui::T(ImVec2 p) const
{
    return { mCenter.x + (p.x - mCenter.x) * mInScale,
             mCenter.y + (p.y - mCenter.y) * mInScale };
}

ImVec4 CombatantGui::T(const ImVec4& r) const
{
    const ImVec2 a = T(ImVec2(r.x, r.y));
    const ImVec2 b = T(ImVec2(r.z, r.w));
    return { a.x, a.y, b.x, b.y };
}

ImVec2 CombatantGui::itf(ImVec2 p) const
{
    return { mCenter.x + (p.x - mCenter.x) / mInScale,
             mCenter.y + (p.y - mCenter.y) / mInScale };
}

ImVec4 CombatantGui::numberTrack(const ImVec4& rr) const
{
    return { rr.x + S(8.f), rr.y + S(20.f), rr.z - S(8.f), rr.y + S(24.f) };
}

// ---------------------------------------------------------------------------
// Состояние
// ---------------------------------------------------------------------------

void CombatantGui::onOpen()
{
    mWaitRelease = true;
    mBindModule = nullptr;
    mDragNumber = nullptr;
    for (auto& p : mPanels) p.dragging = false;
}

void CombatantGui::onClose()
{
    mBindModule = nullptr;
    mDragNumber = nullptr;
    for (auto& p : mPanels) p.dragging = false;
}

void CombatantGui::onWindowResizeEvent(WindowResizeEvent&)
{
    mResizePending = true;
}

bool CombatantGui::onKey(int vk, bool pressed)
{
    if (!mBindModule) return false;
    if (pressed)
    {
        mBindModule->mKey = (vk == VK_ESCAPE) ? 0 : vk;
        mBindModule = nullptr;
    }
    return true;
}

void CombatantGui::initPanels(size_t n)
{
    if (mPanels.size() == n) return;

    mPanels.assign(n, PanelState{});
    mDrawOrder.clear();

    const float totalW = static_cast<float>(n) * S(kPanelW) + static_cast<float>(n - 1) * S(kGap);
    const float startX = (mScreen.x - totalW) * 0.5f;
    for (size_t i = 0; i < n; i++)
    {
        mPanels[i].x = std::round(startX + static_cast<float>(i) * (S(kPanelW) + S(kGap)));
        mPanels[i].y = S(kTop);
        mDrawOrder.push_back(i);
    }
    mLayoutScreen = mScreen;
}

void CombatantGui::bringToFront(size_t i)
{
    auto it = std::find(mDrawOrder.begin(), mDrawOrder.end(), i);
    if (it != mDrawOrder.end()) mDrawOrder.erase(it);
    mDrawOrder.push_back(i);
}

// Строки панели: либо модули категории, либо (после правого клика) настройки модуля.
std::vector<CombatantGui::Row> CombatantGui::buildRows(size_t cat, const PanelState& p) const
{
    std::vector<Row> rows;
    if (p.selected)
    {
        rows.push_back({ RowKind::Back, p.selected, nullptr, S(kRowH) });
        rows.push_back({ RowKind::Bind, p.selected, nullptr, S(kRowH) });
        for (Setting* s : p.selected->mSettings)
        {
            if (!s) continue;
            if (s->mIsVisible && !s->mIsVisible()) continue;
            const float h = s->mType == SettingType::Number ? S(kNumberH) : S(kRowH);
            rows.push_back({ RowKind::Setting, p.selected, s, h });
        }
    }
    else
    {
        for (auto& m : gFeatureManager->mModuleManager->getModulesInCategory(static_cast<int>(cat)))
            rows.push_back({ RowKind::Module, m.get(), nullptr, S(kRowH) });
    }
    return rows;
}

// ---------------------------------------------------------------------------
// Ввод
// ---------------------------------------------------------------------------

void CombatantGui::handleRowClick(size_t i, const Row& row, const ImVec4& rr,
                                  bool left, bool right, bool middle)
{
    PanelState& p = mPanels[i];
    switch (row.kind)
    {
    case RowKind::Back:
        if (left || right)
        {
            p.selected = nullptr;
            p.scroll = 0.f;
            p.smoothScroll = 0.f;
        }
        break;

    case RowKind::Bind:
        if (left || middle) mBindModule = row.mod;
        break;

    case RowKind::Module:
        if (left)
        {
            row.mod->toggle();
        }
        else if (right)
        {
            p.selected = row.mod;
            p.scroll = 0.f;
            p.smoothScroll = 0.f;
        }
        else if (middle)
        {
            mBindModule = row.mod;
        }
        break;

    case RowKind::Setting:
    {
        Setting* s = row.set;
        switch (s->mType)
        {
        case SettingType::Bool:
            if (left || right)
            {
                auto* b = static_cast<BoolSetting*>(s);
                b->setValue(!b->mValue);
            }
            break;

        case SettingType::Number:
            if (left)
            {
                const ImVec4 tr = numberTrack(rr);
                const ImVec4 hit = { tr.x - S(6.f), tr.y - S(8.f), tr.z + S(6.f), tr.w + S(8.f) };
                if (inside(hit, mLayoutMouse))
                {
                    mDragNumber = static_cast<NumberSetting*>(s);
                    mDragTrack = tr;
                }
            }
            break;

        case SettingType::Enum:
            if (left || right)
            {
                auto* e = static_cast<EnumSetting*>(s);
                const int count = static_cast<int>(e->mValues.size());
                if (count > 0) e->setValue((e->mValue + (left ? 1 : count - 1)) % count);
            }
            break;

        default:
            // Color / String: только просмотр (редактора в этом стиле пока нет).
            break;
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Рендер
// ---------------------------------------------------------------------------

void CombatantGui::render(float animation, float inScale, float uiScale)
{
    const size_t n = ModuleCategoryNames.size();
    if (n == 0 || animation <= 0.f) return;

    FontHelper::pushPrefFont(true, false, false);

    ImGuiIO& io = ImGui::GetIO();
    mAnim = std::clamp(animation, 0.f, 1.f);
    mInScale = std::max(inScale, 0.001f);
    mScale = std::clamp(uiScale, 0.5f, 3.f);
    mDt = std::min(io.DeltaTime, 0.1f);
    mWheel = io.MouseWheel;
    mMouse = io.MousePos;
    mScreen = ImRenderUtils::getScreenSize();
    mCenter = { mScreen.x * 0.5f, mScreen.y * 0.5f };
    mLayoutMouse = itf(mMouse);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    initPanels(n);
    if (mResizePending && mLayoutScreen.x > 1.f && mLayoutScreen.y > 1.f)
    {
        for (auto& p : mPanels)
        {
            p.x = std::round(p.x * mScreen.x / mLayoutScreen.x);
            p.y = std::round(p.y * mScreen.y / mLayoutScreen.y);
        }
    }
    mResizePending = false;
    mLayoutScreen = mScreen;

    // Состояние мыши. Клики до отпускания открывающей кнопки игнорируем.
    if (mWaitRelease && !ImGui::IsMouseDown(0)) mWaitRelease = false;
    const bool mouseDown = ImGui::IsMouseDown(0);
    bool clickL = !mWaitRelease && ImGui::IsMouseClicked(0);
    bool clickR = !mWaitRelease && ImGui::IsMouseClicked(1);
    bool clickM = !mWaitRelease && ImGui::IsMouseClicked(2);

    // Захват бинда кнопками мыши 4/5; левый/правый клик отменяет захват.
    if (mBindModule)
    {
        if (ImGui::IsMouseClicked(3))
        {
            mBindModule->mKey = VK_XBUTTON1;
            mBindModule = nullptr;
        }
        else if (ImGui::IsMouseClicked(4))
        {
            mBindModule->mKey = VK_XBUTTON2;
            mBindModule = nullptr;
        }
        else if (clickL || clickR)
        {
            mBindModule = nullptr;
            clickL = false;
            clickR = false;
        }
    }

    // Перетаскивание слайдера.
    if (mDragNumber)
    {
        if (mouseDown)
        {
            const float w = std::max(1.f, mDragTrack.z - mDragTrack.x);
            const float t = std::clamp((mLayoutMouse.x - mDragTrack.x) / w, 0.f, 1.f);
            mDragNumber->setValue(mDragNumber->mMin + t * (mDragNumber->mMax - mDragNumber->mMin));
        }
        else
        {
            mDragNumber = nullptr;
        }
    }

    // Перетаскивание панелей за заголовок.
    for (auto& p : mPanels)
    {
        if (!p.dragging) continue;
        if (mouseDown)
        {
            p.x = mLayoutMouse.x - p.dragDX;
            p.y = mLayoutMouse.y - p.dragDY;
        }
        else
        {
            p.dragging = false;
        }
    }

    // Строки, размеры и прокрутка.
    std::vector<std::vector<Row>> rows(n);
    std::vector<ImVec4> rects(n);
    for (size_t i = 0; i < n; i++)
    {
        PanelState& p = mPanels[i];
        p.x = std::clamp(p.x, 0.f, std::max(0.f, mScreen.x - S(kPanelW)));
        p.y = std::clamp(p.y, 0.f, std::max(0.f, mScreen.y - S(kHeaderH)));

        rows[i] = buildRows(i, p);
        float total = S(kPad) * 2.f;
        for (const Row& r : rows[i]) total += r.h;

        const float listH = std::min(total, S(kMaxList));
        p.maxScroll = std::max(0.f, total - listH);
        p.scroll = std::clamp(p.scroll, 0.f, p.maxScroll);
        p.smoothScroll = approach(p.smoothScroll, p.scroll, mDt, 16.f);
        p.swap = approach(p.swap, p.selected ? 1.f : 0.f, mDt, 10.f);

        rects[i] = { p.x, p.y, p.x + S(kPanelW), p.y + S(kHeaderH) + listH };
    }

    // Верхняя панель под курсором.
    mHoverPanel = -1;
    for (auto it = mDrawOrder.rbegin(); it != mDrawOrder.rend(); ++it)
    {
        if (inside(rects[*it], mLayoutMouse))
        {
            mHoverPanel = static_cast<int>(*it);
            break;
        }
    }

    // Клики по панели.
    if (mHoverPanel >= 0 && mAnim > 0.5f)
    {
        const size_t i = static_cast<size_t>(mHoverPanel);
        PanelState& p = mPanels[i];
        const ImVec4 r = rects[i];
        const float hBottom = r.y + S(kHeaderH);
        const ImVec4 header = { r.x, r.y, r.z, hBottom };
        const ImVec4 list = { r.x, hBottom, r.z, r.w };

        if (clickL || clickR || clickM) bringToFront(i);

        if (clickL && inside(header, mLayoutMouse))
        {
            p.dragging = true;
            p.dragDX = mLayoutMouse.x - p.x;
            p.dragDY = mLayoutMouse.y - p.y;
        }

        if (mWheel != 0.f && inside(r, mLayoutMouse))
            p.scroll = std::clamp(p.scroll - mWheel * S(kRowH) * 2.f, 0.f, p.maxScroll);

        if ((clickL || clickR || clickM) && inside(list, mLayoutMouse))
        {
            float y = list.y + S(kPad) - p.smoothScroll;
            for (const Row& row : rows[i])
            {
                const ImVec4 rr = { r.x + S(4.f), y, r.z - S(4.f), y + row.h };
                if (inside(rr, mLayoutMouse))
                {
                    handleRowClick(i, row, rr, clickL, clickR, clickM);
                    break;
                }
                y += row.h;
            }
        }
    }

    // Отрисовка.
    dl->AddRectFilled({ 0.f, 0.f }, mScreen, IM_COL32(0, 0, 0, static_cast<int>(90.f * mAnim)));
    for (size_t i : mDrawOrder)
        drawPanel(dl, i, rects[i], rows[i]);

    const float hfs = S(kFontPx) * mInScale;
    const std::string hint = mBindModule
        ? "Press a key to bind " + mBindModule->getName() + "   (ESC = clear)"
        : std::string("Left: toggle  |  Right: settings  |  Middle: bind  |  Drag header: move");
    const float hw = textWidth(hint, hfs);
    drawText(dl, { (mScreen.x - hw) * 0.5f, mScreen.y - S(36.f) }, hint, argb(kFaint, mAnim), hfs);

    ImGui::PopFont();
}

void CombatantGui::drawPanel(ImDrawList* dl, size_t i, const ImVec4& r, const std::vector<Row>& rows)
{
    const PanelState& p = mPanels[i];
    const float a = mAnim;
    const ImVec4 R = T(r);
    const float rnd = S(kRound) * mInScale;
    const float hH = S(kHeaderH) * mInScale;
    const float fs = S(kFontPx) * mInScale;

    dl->AddRectFilled({ R.x, R.y }, { R.z, R.w }, argb(kGlass, a), rnd);
    dl->AddRect({ R.x, R.y }, { R.z, R.w }, argb(kStroke, a), rnd, 0, 1.f);
    dl->AddRectFilled({ R.x, R.y }, { R.z, R.y + hH }, argb(kHeadBg, a), rnd, ImDrawFlags_RoundCornersTop);
    dl->AddLine({ R.x + S(8.f) * mInScale, R.y + hH }, { R.z - S(8.f) * mInScale, R.y + hH },
                accent(0.55f * a), 1.f);

    // Заголовок: категория, либо имя модуля в режиме настроек.
    const std::string title = p.selected ? p.selected->getName() : ModuleCategoryNames[i];
    drawText(dl, { R.x + S(9.f) * mInScale, R.y + (hH - fs) * 0.5f }, title, argb(kText, a), fs);

    if (!p.selected)
    {
        const std::string count = std::to_string(rows.size());
        const float cw = textWidth(count, fs);
        drawText(dl, { R.z - S(9.f) * mInScale - cw, R.y + (hH - fs) * 0.5f }, count, argb(kMuted, a), fs);
    }

    // Список (с обрезкой по панели и прокруткой).
    const ImVec4 listL = { r.x, r.y + S(kHeaderH), r.z, r.w };
    const ImVec4 listR = T(listL);
    const bool canHover = mHoverPanel == static_cast<int>(i) && a > 0.5f && !p.dragging;
    dl->PushClipRect({ listR.x, listR.y }, { listR.z, listR.w }, true);

    float y = listL.y + S(kPad) - p.smoothScroll;
    for (const Row& row : rows)
    {
        const ImVec4 rr = { r.x + S(4.f), y, r.z - S(4.f), y + row.h };
        const bool selected = row.kind == RowKind::Module && p.selected == row.mod;
        drawRow(dl, row, rr, selected, canHover);
        y += row.h;
    }

    dl->PopClipRect();
}

void CombatantGui::drawRow(ImDrawList* dl, const Row& row, const ImVec4& rr, bool selected, bool canHover)
{
    const float a = mAnim;
    const float fs = S(kFontPx) * mInScale;
    const float rnd = S(6.f) * mInScale;

    // Плавная подсветка при наведении.
    const bool hovered = canHover && inside(rr, mLayoutMouse);
    const void* owner = row.set ? static_cast<const void*>(row.set) : static_cast<const void*>(row.mod);
    const uintptr_t key = (reinterpret_cast<uintptr_t>(owner) << 2) | static_cast<uintptr_t>(row.kind);
    float& hover = mHover[key];
    hover = approach(hover, hovered ? 1.f : 0.f, mDt, 16.f);

    const ImVec4 R = T(rr);
    if (hover > 0.01f)
        dl->AddRectFilled({ R.x, R.y }, { R.z, R.w }, argb(kHoverBg, hover * a), rnd);

    if (selected)
    {
        dl->AddRectFilled({ R.x, R.y }, { R.z, R.w }, accent(0.14f * a), rnd);
        dl->AddRectFilled({ R.x, R.y + S(4.f) * mInScale }, { R.x + S(2.f) * mInScale, R.w - S(4.f) * mInScale },
                          accent(a));
    }

    const float cy = (R.y + R.w) * 0.5f;
    const float tx = R.x + S(8.f) * mInScale;
    const float rx = R.z - S(8.f) * mInScale;
    const float ty = cy - fs * 0.5f;

    switch (row.kind)
    {
    case RowKind::Back:
        drawText(dl, { tx, ty }, "< Back", accent(a), fs);
        break;

    case RowKind::Bind:
    {
        drawText(dl, { tx, ty }, "Bind", argb(kMuted, a), fs);
        const std::string v = mBindModule == row.mod ? std::string("press key...") : keyName(row.mod->mKey);
        drawText(dl, { rx - textWidth(v, fs), ty }, v, accent(a), fs);
        break;
    }

    case RowKind::Module:
    {
        const bool on = row.mod->mEnabled;
        drawText(dl, { tx, ty }, row.mod->getName(), argb(on ? kOn : kMuted, a), fs);
        dl->AddCircleFilled({ rx - S(3.f) * mInScale, cy }, S(3.f) * mInScale, argb(on ? kOn : kOff, a), 12);
        break;
    }

    case RowKind::Setting:
    {
        Setting* s = row.set;
        const bool isNumber = s->mType == SettingType::Number;
        const float labelY = isNumber ? R.y + S(5.f) * mInScale : ty;
        drawText(dl, { tx, labelY }, s->mName, argb(kMuted, a), fs);

        switch (s->mType)
        {
        case SettingType::Bool:
        {
            auto* b = static_cast<BoolSetting*>(s);
            const float th = S(12.f) * mInScale;
            const float tw = S(24.f) * mInScale;
            const ImVec2 tmin = { rx - tw, cy - th * 0.5f };
            const ImVec2 tmax = { rx, cy + th * 0.5f };
            dl->AddRectFilled(tmin, tmax, b->mValue ? accent(a) : argb(kTrackOff, a), th * 0.5f);
            const float kx = b->mValue ? tmax.x - th * 0.5f : tmin.x + th * 0.5f;
            dl->AddCircleFilled({ kx, cy }, th * 0.42f, argb(kText, a), 12);
            break;
        }

        case SettingType::Number:
        {
            auto* ns = static_cast<NumberSetting*>(s);
            const float span = ns->mMax - ns->mMin;
            const float t = span > 0.f ? std::clamp((ns->mValue - ns->mMin) / span, 0.f, 1.f) : 0.f;
            const std::string v = fmtValue(ns->mValue, ns->mStep);
            drawText(dl, { rx - textWidth(v, fs), labelY }, v, accent(a), fs);

            const ImVec4 tr = T(numberTrack(rr));
            const float th = tr.w - tr.y;
            dl->AddRectFilled({ tr.x, tr.y }, { tr.z, tr.w }, argb(kTrackOff, a), th * 0.5f);
            const float fx = tr.x + (tr.z - tr.x) * t;
            dl->AddRectFilled({ tr.x, tr.y }, { fx, tr.w }, accent(a), th * 0.5f);
            dl->AddCircleFilled({ fx, (tr.y + tr.w) * 0.5f }, S(4.f) * mInScale, argb(kText, a), 12);
            break;
        }

        case SettingType::Enum:
        {
            auto* e = static_cast<EnumSetting*>(s);
            const bool valid = e->mValue >= 0 && e->mValue < static_cast<int>(e->mValues.size());
            const std::string v = valid ? e->mValues[e->mValue] : std::string("?");
            drawText(dl, { rx - textWidth(v, fs), ty }, v, accent(a), fs);
            break;
        }

        case SettingType::Color:
        {
            auto* c = static_cast<ColorSetting*>(s);
            ImVec4 cv = { c->mValue[0], c->mValue[1], c->mValue[2], c->mValue[3] * a };
            const float sw = S(10.f) * mInScale;
            dl->AddRectFilled({ rx - sw, cy - sw * 0.5f }, { rx, cy + sw * 0.5f },
                              ImGui::ColorConvertFloat4ToU32(cv), S(2.f) * mInScale);
            break;
        }

        case SettingType::String:
        {
            const std::string v = "(text)";
            drawText(dl, { rx - textWidth(v, fs), ty }, v, argb(kFaint, a), fs);
            break;
        }

        default:
            break;
        }
        break;
    }
    }
}
