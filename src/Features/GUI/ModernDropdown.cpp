//
// ModernDropdown.cpp — Modern ClickGui
//
#include "ModernDropdown.hpp"

#include <Features/Modules/ModuleCategory.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/StringUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <random>

namespace
{
    constexpr float kPi = 3.14159265f;

    float smoothTo(float cur, float tgt, float speed, float dt)
    {
        float v = tgt + (cur - tgt) * std::exp(-speed * dt);
        return std::fabs(v - tgt) < 0.0005f ? tgt : v;
    }
    ImColor withA(const ImColor& c, float a) { return { c.Value.x, c.Value.y, c.Value.z, std::clamp(a, 0.f, 1.f) }; }
    ImU32   white(float a) { return IM_COL32(255, 255, 255, (int)(std::clamp(a, 0.f, 1.f) * 255.f)); }
    ImU32   black(float a) { return IM_COL32(0, 0, 0, (int)(std::clamp(a, 0.f, 1.f) * 255.f)); }
    ImU32   rgba(int r, int g, int b, float a) { return IM_COL32(r, g, b, (int)(std::clamp(a, 0.f, 1.f) * 255.f)); }
    bool    in(const ImVec4& r, ImVec2 p) { return p.x >= r.x && p.y >= r.y && p.x < r.z && p.y < r.w; }
    ImVec2  mn(const ImVec4& r) { return { r.x, r.y }; }
    ImVec2  mx(const ImVec4& r) { return { r.z, r.w }; }
    float   dist(ImVec2 a, ImVec2 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

    const std::vector<std::string>& cats() { static std::vector<std::string> c = ModuleCategoryNames; return c; }

    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    }

    // "Ambient Cubes" -> "ambientcubes": separators and case are noise when
    // comparing a typed query against a module name.
    std::string squashName(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s)
            if (std::isalnum(c))
                out.push_back((char)std::tolower(c));
        return out;
    }

    // Standard edit distance. Only ever runs for the handful of names that
    // missed the plain substring search, so the cost does not matter.
    int editDistance(const std::string& a, const std::string& b)
    {
        const int n = (int)a.size(), m = (int)b.size();
        std::vector<int> prev(m + 1), cur(m + 1);

        for (int j = 0; j <= m; ++j) prev[j] = j;

        for (int i = 1; i <= n; ++i) {
            cur[0] = i;
            for (int j = 1; j <= m; ++j) {
                const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                cur[j] = (std::min)({ prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost });
            }
            prev.swap(cur);
        }

        return prev[m];
    }

    // Near-miss match: tolerates typos and alternate spellings, both across the
    // whole name and across just the beginning of it. This is what lets
    // "ambience" or "ambience cubes" find AmbientCubes — no substring search
    // can ever do that, and guessing the exact spelling should not be required.
    bool fuzzyNameMatch(const std::string& name, const std::string& query)
    {
        if (query.size() < 4) return false;

        const std::string ss = squashName(name);
        if (ss.empty()) return false;

        if (ss.find(query) != std::string::npos) return true;

        const int maxDist = (query.size() >= 10) ? 3 : (query.size() >= 6 ? 2 : 1);

        // Typo in the first word only ("ambience" against "ambientcubes").
        const std::string head = ss.size() > query.size() ? ss.substr(0, query.size()) : ss;
        if (editDistance(head, query) <= maxDist) return true;

        // Typo in the whole thing ("ambience cubes" against "ambientcubes").
        if (std::abs((int)ss.size() - (int)query.size()) <= maxDist &&
            editDistance(ss, query) <= maxDist)
            return true;

        return false;
    }
    std::string toUtf8(const wchar_t* w, int n)
    {
        if (n <= 0) return {};
        char buf[256];
        int len = WideCharToMultiByte(CP_UTF8, 0, w, n, buf, sizeof(buf), nullptr, nullptr);
        return len > 0 ? std::string(buf, len) : std::string();
    }
    std::wstring toWide(const std::string& s)
    {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
        return w;
    }
    void popUtf8(std::string& s)
    {
        if (s.empty()) return;
        size_t i = s.size() - 1;
        while (i > 0 && (s[i] & 0xC0) == 0x80) --i;
        s.erase(i);
    }
    HKL usLayout() { static HKL us = LoadKeyboardLayoutA("00000409", KLF_NOTELLSHELL); return us; }

    std::string usEquivalent(const std::string& s)
    {
        HKL us = usLayout(), cur = GetKeyboardLayout(0);
        if (!us || us == cur) return {};
        std::string out;
        for (wchar_t wc : toWide(s)) {
            SHORT r = VkKeyScanExW(wc, cur);
            if (r == -1) { out += toUtf8(&wc, 1); continue; }
            UINT vk  = r & 0xFF;
            UINT sc  = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, cur);
            UINT uvk = MapVirtualKeyExW(sc, MAPVK_VSC_TO_VK, us) & 0xFF;
            if (uvk >= 'A' && uvk <= 'Z')      out += (char)(uvk + 32);
            else if (uvk >= '0' && uvk <= '9') out += (char)uvk;
            else if (vk == VK_SPACE)           out += ' ';
            else                               out += toUtf8(&wc, 1);
        }
        return out;
    }

    std::string charFromKey(int vk)
    {
        BYTE ks[256] = {};
        BYTE sh = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0x80 : 0;
        ks[VK_SHIFT] = ks[VK_LSHIFT] = ks[VK_RSHIFT] = sh;
        ks[VK_CAPITAL] = (GetKeyState(VK_CAPITAL) & 1) ? 1 : 0;
        HKL hkl = GetKeyboardLayout(0);
        UINT sc = MapVirtualKeyExW((UINT)vk, MAPVK_VK_TO_VSC, hkl);
        WCHAR out[8]{};
        int n = ToUnicodeEx((UINT)vk, sc, ks, out, 8, 0x4, hkl);
        if (n <= 0 || out[0] < 32) return {};
        return toUtf8(out, n);
    }

    std::string clipboardText()
    {
        std::string out;
        if (!OpenClipboard(nullptr)) return out;
        if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
            if (auto* w = (const wchar_t*)GlobalLock(h)) {
                out = toUtf8(w, (int)wcslen(w));
                GlobalUnlock(h);
            }
        }
        CloseClipboard();
        out.erase(std::remove_if(out.begin(), out.end(), [](char c) { return c == '\r' || c == '\n' || c == '\t'; }), out.end());
        return out;
    }

    std::string keyName(int vk)
    {
        switch (vk) {
        case VK_TAB: return "TAB";     case VK_SPACE: return "SPACE";  case VK_RETURN: return "ENTER";
        case VK_BACK: return "BKSP";   case VK_CAPITAL: return "CAPS"; case VK_ESCAPE: return "ESC";
        case VK_DELETE: return "DEL";  case VK_INSERT: return "INS";   case VK_HOME: return "HOME";
        case VK_END: return "END";     case VK_PRIOR: return "PGUP";   case VK_NEXT: return "PGDN";
        case VK_UP: return "UP";       case VK_DOWN: return "DOWN";    case VK_LEFT: return "LEFT";  case VK_RIGHT: return "RIGHT";
        case VK_SHIFT: case VK_LSHIFT: return "SHIFT";  case VK_RSHIFT: return "RSHIFT";
        case VK_CONTROL: case VK_LCONTROL: return "CTRL"; case VK_RCONTROL: return "RCTRL";
        case VK_MENU: case VK_LMENU: return "ALT";      case VK_RMENU: return "RALT";
        case VK_XBUTTON1: return "MB4"; case VK_XBUTTON2: return "MB5";
        case VK_OEM_MINUS: return "-"; case VK_OEM_PLUS: return "="; case VK_OEM_COMMA: return ",";
        case VK_OEM_PERIOD: return "."; case VK_OEM_1: return ";"; case VK_OEM_2: return "/";
        case VK_OEM_3: return "`"; case VK_OEM_4: return "["; case VK_OEM_6: return "]"; case VK_OEM_5: return "\\";
        case VK_OEM_7: return "'";
        }
        if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
        if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return "NUM" + std::to_string(vk - VK_NUMPAD0);
        if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, (char)vk);
        return "K" + std::to_string(vk);
    }

    std::string fmtValue(float v, float step)
    {
        int d = step >= 1.f ? 0 : step >= 0.1f ? 1 : step >= 0.01f ? 2 : 3;
        char b[32]; std::snprintf(b, sizeof(b), "%.*f", d, v);
        return b;
    }
    std::string hexOf(float r, float g, float b)
    {
        char buf[16]; std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", (int)std::lround(r * 255), (int)std::lround(g * 255), (int)std::lround(b * 255));
        return buf;
    }
}

ImVec2 ModernGui::tf(ImVec2 p) const { return { mCenter.x + (p.x - mCenter.x) * mInScale, mCenter.y + (p.y - mCenter.y) * mInScale }; }
ImVec4 ModernGui::tf(const ImVec4& r) const { ImVec2 a = tf({ r.x, r.y }), b = tf({ r.z, r.w }); return { a.x, a.y, b.x, b.y }; }
ImVec2 ModernGui::itf(ImVec2 p) const { return { mCenter.x + (p.x - mCenter.x) / mInScale, mCenter.y + (p.y - mCenter.y) / mInScale }; }
float  ModernGui::tw(const std::string& s, float scale) const { return ImGui::GetFont()->CalcTextSizeA(mTextPx * scale, FLT_MAX, 0, s.c_str()).x; }

void ModernGui::text(ImDrawList* dl, ImVec2 p, const std::string& s, ImColor c, float scale, float alpha)
{
    c.Value.w *= alpha * mAnim;
    if (c.Value.w <= 0.004f || s.empty()) return;
    // Crisp dark backing so panel text stays readable over light world backgrounds
    ImColor shadow(3, 4, 9, (int)(c.Value.w * 255.f * 0.55f));
    dl->AddText(ImGui::GetFont(), mTextPx * scale, { std::floor(p.x) + 1.f, std::floor(p.y) + 1.f }, shadow, s.c_str());
    dl->AddText(ImGui::GetFont(), mTextPx * scale, { std::floor(p.x), std::floor(p.y) }, c, s.c_str());
}

void ModernGui::chevron(ImDrawList* dl, ImVec2 c, float s, float t, ImU32 col, float thick)
{
    float a = t * kPi * 0.5f, ca = std::cos(a), sa = std::sin(a);
    auto R = [&](float x, float y) { return ImVec2(c.x + x * ca - y * sa, c.y + x * sa + y * ca); };
    ImVec2 p[3] = { R(-s * 0.5f, -s), R(s * 0.5f, 0), R(-s * 0.5f, s) };
    dl->AddPolyline(p, 3, col, 0, thick);
}

void ModernGui::toggleSwitch(ImVec2 c, float t, const ImColor& acc, float a)
{
    float w = P(30), h = P(16), r = h * 0.5f;
    ImVec2 a0 = { c.x - w * 0.5f, c.y - h * 0.5f }, a1 = { c.x + w * 0.5f, c.y + h * 0.5f };
    ImColor bg = { 0.18f + (acc.Value.x - 0.18f) * t, 0.18f + (acc.Value.y - 0.18f) * t, 0.2f + (acc.Value.z - 0.2f) * t, a * mAnim };
    mDl->AddRectFilled(a0, a1, bg, r);
    if (t > 0.05f) mDl->AddShadowRect(a0, a1, withA(acc, 0.35f * t * a * mAnim), P(10), { 0, 0 }, 0, r);
    float kx = a0.x + r + (w - h) * t, kr = r - P(2.5f);
    mDl->AddCircleFilled({ kx, c.y }, kr, white(a * mAnim), 16);
}

void ModernGui::caret(ImDrawList* dl, float x, float y, float h, float a)
{
    if (std::fmod(mText.blink, 1.f) > 0.5f) return;
    dl->AddRectFilled({ x, y }, { x + std::max(1.f, P(1.5f)), y + h }, white(0.9f * a * mAnim));
}

float ModernGui::keyBadge(float rightX, float cy, const std::string& label, const ImColor& acc, float a, bool hot)
{
    float w = tw(label, 0.7f) + P(10), h = P(17);
    ImVec4 r = { rightX - w, cy - h * 0.5f, rightX, cy + h * 0.5f };
    mDl->AddRectFilled(mn(r), mx(r), hot ? withA(acc, 0.25f * a * mAnim) : ImColor(1.f, 1.f, 1.f, 0.06f * a * mAnim), P(4));
    mDl->AddRect(mn(r), mx(r), hot ? withA(acc, 0.6f * a * mAnim) : ImColor(1.f, 1.f, 1.f, 0.1f * a * mAnim), P(4));
    text(mDl, { r.x + P(5), cy - mTextH * 0.35f }, label, hot ? acc : ImColor(1.f, 1.f, 1.f, 0.6f), 0.7f, a);
    return w;
}

void ModernGui::checker(ImDrawList* dl, const ImVec4& r, float cell, float a, float rnd)
{
    dl->AddRectFilled(mn(r), mx(r), rgba(200, 200, 200, a), rnd);
    dl->PushClipRect(mn(r), mx(r), true);
    int cols = (int)std::ceil((r.z - r.x) / cell), rows = (int)std::ceil((r.w - r.y) / cell);
    for (int y = 0; y < rows; y++)
        for (int x = (y & 1); x < cols; x += 2)
            dl->AddRectFilled({ r.x + x * cell, r.y + y * cell }, { r.x + (x + 1) * cell, r.y + (y + 1) * cell }, rgba(120, 120, 120, a));
    dl->PopClipRect();
}

ImVec4 ModernGui::rowRect(const Panel& p, const Row& r) const
{
    float top = p.y + S(kHeaderH) + r.y - p.scrollAnim;
    return tf({ p.x, top, p.x + S(kCatW), top + r.h });
}
ImVec4 ModernGui::trackOf(const ImVec4& rs) const { return { rs.x + P(14), rs.w - P(20), rs.z - P(14), rs.w - P(6) }; }
ImColor ModernGui::accent(size_t cat, int modIdx) const { return ColorUtils::getThemedColor((int)(cat * 30 + modIdx * 5)); }

bool ModernGui::hovering(size_t cat, const ImVec4& rs) const
{
    return mIsEnabled && !mPicker.open && mDragPanel < 0 && (int)cat == mHoveredCat
        && in(rs, mMouse) && in(mPanels[cat].content, mMouse);
}
bool ModernGui::canInteract(size_t cat, const ImVec4& rs) const
{
    return hovering(cat, rs) && !mClickConsumed && !mBind.active && panelExtended(mPanels[cat]);
}

void ModernGui::bringToFront(size_t i)
{
    auto it = std::find(mDrawOrder.begin(), mDrawOrder.end(), i);
    if (it != mDrawOrder.end()) mDrawOrder.erase(it);
    mDrawOrder.push_back(i);
}
void ModernGui::play(const char* s, float pitch) { if (auto* ci = ClientInstance::get()) ci->playUi(s, 0.7f, pitch); }

std::string ModernGui::settingName(Setting* s) const { return mLowercase ? StringUtils::toLower(s->mName) : s->mName; }

void ModernGui::setBindKey(int vk)
{
    int k = vk == VK_ESCAPE ? 0 : vk;
    if (mBind.mod) mBind.mod->mKey = k; else if (mBind.set) mBind.set->mKey = k;
    mBind = {};
    play(k ? "random.orb" : "random.break");
}

void ModernGui::onOpen()
{
    mOpenTime = mTime; mFilterDirty = true;
    mClickConsumed = true;
}
void ModernGui::onClose()
{
    if (mText.active) finishText(false);
    mBind = {}; mPicker.open = false; mDrag.set = nullptr; mDragPanel = -1;
    mSearch.clear(); mFilterDirty = true; mTooltip = {};
    mCursorTrail.clear();
    for (auto& p : mPanels) { p.pressed = p.dragging = false; }
}
void ModernGui::onWindowResizeEvent(WindowResizeEvent&) { mResizePending = true; }

nlohmann::json ModernGui::saveLayout() const
{
    nlohmann::json j;
    j["screen"] = { mLayoutScreen.x, mLayoutScreen.y };
    for (auto& p : mPanels) j["panels"].push_back({ {"x", p.x}, {"y", p.y}, {"ext", p.extended} });
    for (size_t i : mDrawOrder) j["order"].push_back(i);
    for (auto& p : mPanels) for (auto& m : p.mods) {
        auto it = mModUi.find(m.get());
        if (it != mModUi.end() && it->second.expanded) j["expanded"].push_back(m->mName);
    }
    return j;
}
void ModernGui::loadLayout(const nlohmann::json& j) { mPendingLayout = j; }

void ModernGui::applyPendingLayout()
{
    if (mPendingLayout.is_null() || mPanels.empty()) return;
    auto j = mPendingLayout; mPendingLayout = nullptr;
    try {
        float sx = 1, sy = 1;
        if (j.contains("screen") && j["screen"][0].get<float>() > 1) { sx = mScreen.x / j["screen"][0].get<float>(); sy = mScreen.y / j["screen"][1].get<float>(); }
        if (j.contains("panels") && j["panels"].size() == mPanels.size())
            for (size_t i = 0; i < mPanels.size(); i++) {
                mPanels[i].x = std::clamp(j["panels"][i]["x"].get<float>() * sx, 0.f, mScreen.x - S(kCatW));
                mPanels[i].y = std::clamp(j["panels"][i]["y"].get<float>() * sy, 0.f, mScreen.y - S(kHeaderH));
                mPanels[i].extended = j["panels"][i]["ext"].get<bool>();
            }
        if (j.contains("order") && j["order"].size() == mPanels.size()) {
            std::vector<size_t> o; for (auto& v : j["order"]) o.push_back(v.get<size_t>());
            std::vector<size_t> sorted = o; std::sort(sorted.begin(), sorted.end());
            bool ok = true; for (size_t i = 0; i < sorted.size(); i++) ok &= sorted[i] == i;
            if (ok) mDrawOrder = o;
        }
        if (j.contains("expanded")) {
            for (auto& p : mPanels) for (auto& m : p.mods)
                for (auto& n : j["expanded"]) if (n.get<std::string>() == m->mName) modUi(m).expanded = true;
        }
    } catch (...) {}
}

void ModernGui::beginFrame(float animation, float inScale, float blur, float midclick, float uiScale,
                           float effectIntensity, bool panelGlow, bool cursorGlow,
                           bool parallax, bool backdropGrid)
{
    auto& io = ImGui::GetIO();
    mAnim = animation; mInScale = std::max(inScale, 0.001f); mBlur = blur; mMidRound = std::max(midclick, 0.001f);
    mScale = std::clamp(uiScale, 0.5f, 3.f);
    mFxIntensity = std::clamp(effectIntensity, 0.1f, 2.5f);
    mPanelGlow = panelGlow; mCursorGlow = cursorGlow;
    mParallax = parallax; mBackdropGrid = backdropGrid;
    mDt = std::min(io.DeltaTime, 0.1f); mTime += mDt;
    mMouse = io.MousePos; mWheel = io.MouseWheel;
    mScreen = ImRenderUtils::getScreenSize(); mCenter = { mScreen.x * 0.5f, mScreen.y * 0.5f };
    mDl = ImGui::GetBackgroundDrawList();

    auto cg = gFeatureManager->mModuleManager->getModule<ClickGui>();
    mIsEnabled = cg && cg->mEnabled;
    auto im = gFeatureManager->mModuleManager->getModule<Interface>();
    bool lc = im && (im->mNamingStyle.mValue == NamingStyle::Lowercase || im->mNamingStyle.mValue == NamingStyle::LowercaseSpaced);
    if (lc != mLowercase) mFilterDirty = true;
    mLowercase = lc;

    FontHelper::pushPrefFont(true, false, false);
    mTextPx = kFontPx * mScale * mInScale;
    mTextH = ImGui::GetFont()->CalcTextSizeA(mTextPx, FLT_MAX, 0, "Ag").y;

    mWheelConsumed = false; mTextClicked = false;
    mTooltip.pending.clear();
    if (mText.active) mText.blink += mDt;

    if (mLastMouse.x == 0.f && mLastMouse.y == 0.f) mLastMouse = mMouse;
    float mouseTravel = dist(mLastMouse, mMouse);
    if (mCursorGlow && mIsEnabled && mouseTravel > P(2.f)) {
        int samples = std::clamp((int)(mouseTravel / std::max(P(8.f), 1.f)), 1, 3);
        for (int n = 1; n <= samples; ++n) {
            float t = (float)n / (float)samples;
            ImVec2 p = {mLastMouse.x + (mMouse.x - mLastMouse.x) * t,
                        mLastMouse.y + (mMouse.y - mLastMouse.y) * t};
            mCursorTrail.push_back({p, 1.f, P(7.f + std::min(mouseTravel, P(40.f)) * 0.08f),
                                    mTime * 34.f + n * 11.f});
        }
    }
    mLastMouse = mMouse;
    for (auto& p : mCursorTrail) p.life -= mDt * 2.6f;
    mCursorTrail.erase(std::remove_if(mCursorTrail.begin(), mCursorTrail.end(),
        [](const CursorTrail& p) { return p.life <= 0.f; }), mCursorTrail.end());
    if (mCursorTrail.size() > 28)
        mCursorTrail.erase(mCursorTrail.begin(), mCursorTrail.begin() + (mCursorTrail.size() - 28));

    if (!mIsEnabled) { mPicker.open = false; mBind = {}; mDrag.set = nullptr; mDragPanel = -1; if (mText.active) finishText(false); }
}

void ModernGui::initPanels()
{
    size_t n = cats().size();
    bool fresh = mPanels.size() != n || mResetPending;
    if (fresh) {
        mPanels.assign(n, Panel{});
        mDrawOrder.clear();
        float totalW = n * S(kCatW) + (n - 1) * S(kGap);
        float startX = (mScreen.x - totalW) * 0.5f;
        for (size_t i = 0; i < n; i++) {
            mPanels[i].x = std::round(std::max(0.f, startX + i * (S(kCatW) + S(kGap))));
            mPanels[i].y = S(64);
            mDrawOrder.push_back(i);
        }
        mLayoutScreen = mScreen; mResetPending = false; mResizePending = false; mFilterDirty = true;
        applyPendingLayout();
        return;
    }
    if (mResizePending && mScreen.x > 1 && mScreen.y > 1) {
        if (mLayoutScreen.x > 1 && mLayoutScreen.y > 1)
            for (auto& p : mPanels) { p.x = std::round(p.x * mScreen.x / mLayoutScreen.x); p.y = std::round(p.y * mScreen.y / mLayoutScreen.y); }
        mLayoutScreen = mScreen; mResizePending = false; mOrbs.clear();
    }
    for (auto& p : mPanels) {
        p.x = std::clamp(p.x, 0.f, std::max(0.f, mScreen.x - S(kCatW)));
        p.y = std::clamp(p.y, 0.f, std::max(0.f, mScreen.y - S(kHeaderH)));
    }
}

void ModernGui::refreshFilter()
{
    std::string key = lower(mSearch);
    if (!mFilterDirty && key == mFilterKey) return;
    mFilterKey = key; mFilterDirty = false; mResultCount = 0;

    std::string q = key, q2 = lower(usEquivalent(mSearch));
    if (q2 == q) q2.clear();
    auto has = [&](const std::string& s) {
        std::string l = lower(s);
        return l.find(q) != std::string::npos || (!q2.empty() && l.find(q2) != std::string::npos);
    };

    // Second chance for names the substring test missed (typos, "ambience" vs
    // "ambient"). Never widens the setting search — only whole module names.
    const std::string qs  = squashName(mSearch);
    const std::string qs2 = squashName(q2);
    auto fuzzy = [&](const std::string& s) {
        if (qs.empty()) return false;
        if (fuzzyNameMatch(s, qs)) return true;
        return !qs2.empty() && fuzzyNameMatch(s, qs2);
    };

    for (size_t i = 0; i < mPanels.size(); i++) {
        auto all = gFeatureManager->mModuleManager->getModulesInCategory(i);
        mPanels[i].mods.clear();
        for (auto& m : all) {
            auto& ui = modUi(m);
            if (q.empty()) { ui.searchHit = 0; mPanels[i].mods.push_back(m); continue; }
            int hit = 0;
            if (has(m->mName) || has(m->getName())) hit = 1;
            if (!hit) for (auto& [style, alias] : m->mNames) if (has(alias)) { hit = 1; break; }
            if (!hit) {
                if (fuzzy(m->mName) || fuzzy(m->getName())) hit = 1;
                else for (auto& [style, alias] : m->mNames) if (fuzzy(alias)) { hit = 1; break; }
            }
            if (!hit) for (auto* s : m->mSettings) if (has(s->mName)) { hit = 2; break; }
            ui.searchHit = hit;
            if (hit) { mPanels[i].mods.push_back(m); mResultCount++; }
        }
    }
}

void ModernGui::layoutPanel(size_t i)
{
    Panel& p = mPanels[i];
    p.rows.clear();
    const float W = S(kCatW), H = S(kHeaderH), RH = S(kRowH), NH = S(kNumRowH);
    p.expand = smoothTo(p.expand, panelExtended(p) ? 1.f : 0.f, 14.f, mDt);

    float y = 0; int mi = 0;
    for (auto& m : p.mods) {
        auto& ui = modUi(m);
        bool exp = (ui.expanded || ui.searchHit == 2) && !m->mSettings.empty();
        ui.open = smoothTo(ui.open, exp ? 1.f : 0.f, 14.f, mDt);

        Row r; r.kind = Row::Mod; r.mod = m; r.y = y; r.h = RH; r.modIdx = mi;
        p.rows.push_back(r); y += RH;

        if (ui.open > 0.002f) {
            for (auto* s : m->mSettings) {
                if (!s->mIsVisible()) continue;
                auto& su = setUi(s);
                Row sr; sr.mod = m; sr.set = s; sr.modIdx = mi; sr.alpha = ui.open; sr.y = y;
                switch (s->mType) {
                case SettingType::Bool:   sr.kind = Row::Bool;   sr.h = RH * ui.open; break;
                case SettingType::Number: sr.kind = Row::Number; sr.h = NH * ui.open; break;
                case SettingType::Enum:   sr.kind = Row::Enum;   sr.h = RH * ui.open; break;
                case SettingType::Color:  sr.kind = Row::Color;  sr.h = RH * ui.open; break;
                case SettingType::String: sr.kind = Row::String; sr.h = RH * ui.open; break;
                default: continue;
                }
                p.rows.push_back(sr); y += sr.h;
                if (s->mType == SettingType::Enum) {
                    auto* es = static_cast<EnumSetting*>(s);
                    su.enumOpen = smoothTo(su.enumOpen, su.enumExpanded ? 1.f : 0.f, 14.f, mDt);
                    if (su.enumOpen > 0.002f)
                        for (int j = 0; j < (int)es->mValues.size(); j++) {
                            Row vr = sr; vr.kind = Row::EnumVal; vr.idx = j; vr.y = y;
                            vr.h = RH * ui.open * su.enumOpen; vr.alpha = ui.open * su.enumOpen;
                            p.rows.push_back(vr); y += vr.h;
                        }
                }
            }
        }
        mi++;
    }
    if (!p.rows.empty()) p.rows.back().last = true;

    p.contentH = y;
    float maxVis = std::max(0.f, mScreen.y - (p.y + H) - S(12));
    p.maxScroll = std::max(0.f, y - maxVis);
    p.scroll = std::clamp(p.scroll, 0.f, p.maxScroll);
    p.scrollAnim = smoothTo(p.scrollAnim, p.scroll, 18.f, mDt);
    p.visH = std::min(y, maxVis) * p.expand;
    p.roundBottom = p.contentH <= maxVis + 0.5f;

    p.header  = tf({ p.x, p.y, p.x + W, p.y + H });
    p.content = tf({ p.x, p.y + H, p.x + W, p.y + H + p.visH });
    p.window  = { p.header.x, p.header.y, p.header.z, p.content.w };
}

void ModernGui::resolveHover()
{
    mHoveredCat = -1;
    if (mDragPanel >= 0) { mHoveredCat = mDragPanel; return; }
    if (mPicker.open && in(mPicker.rect, mMouse)) return;
    if (in(mSearchRect, mMouse)) return;
    for (auto it = mDrawOrder.rbegin(); it != mDrawOrder.rend(); ++it)
        if (panelVisible(*it) && in(mPanels[*it].window, mMouse)) { mHoveredCat = (int)*it; return; }
}

void ModernGui::inputPanel(size_t i)
{
    Panel& p = mPanels[i];
    if (!panelVisible(i)) { p.pressed = p.dragging = false; return; }

    if (p.visH > 0.5f)
        for (auto& r : p.rows) {
            ImVec4 rs = rowRect(p, r);
            if (rs.w < p.content.y || rs.y > p.content.w) continue;
            inputRow(i, r, rs, hovering(i, rs));
        }

    bool overHeader = mIsEnabled && !mPicker.open && !mBind.active && mHoveredCat == (int)i && in(p.header, mMouse);
    if (overHeader && !mClickConsumed && ImGui::IsMouseClicked(0)) {
        p.pressed = true; p.dragging = false; p.pressPos = mMouse;
        ImVec2 m = itf(mMouse); p.dragOff = { m.x - p.x, m.y - p.y };
        bringToFront(i); mClickConsumed = true;
    }
    if (overHeader && !mClickConsumed && ImGui::IsMouseClicked(1)) {
        p.extended = !p.extended; mClickConsumed = true; play("random.click", p.extended ? 1.1f : 0.9f);
    }
    if (p.pressed) {
        if (ImGui::IsMouseDown(0) && mIsEnabled) {
            if (!p.dragging && dist(mMouse, p.pressPos) > 3.f) { p.dragging = true; mDragPanel = (int)i; }
            if (p.dragging) {
                ImVec2 m = itf(mMouse);
                p.x = std::round(std::clamp(m.x - p.dragOff.x, 0.f, std::max(0.f, mScreen.x - S(kCatW))));
                p.y = std::round(std::clamp(m.y - p.dragOff.y, 0.f, std::max(0.f, mScreen.y - S(kHeaderH))));
            }
        } else {
            if (!p.dragging && mIsEnabled) { p.extended = !p.extended; play("random.click", p.extended ? 1.1f : 0.9f); }
            p.pressed = p.dragging = false;
            if (mDragPanel == (int)i) mDragPanel = -1;
        }
    }
    if (!mWheelConsumed && mWheel != 0.f && mIsEnabled && !mPicker.open && mHoveredCat == (int)i && panelExtended(p) && p.maxScroll > 0.f) {
        // Cap the per-frame wheel delta: high-res wheels and low FPS batch several
        // notches into one frame, which made the list fly several rows per flick.
        // One notch now scrolls exactly one row.
        float wheel = std::clamp(mWheel, -1.f, 1.f);
        p.scroll = std::clamp(p.scroll - wheel * S(kRowH + 6.f), 0.f, p.maxScroll);
        p.scrollbarAlpha = 1.f; mWheelConsumed = true;
    }
}

void ModernGui::inputRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    if (r.kind == Row::Number && mDrag.set == r.set) mDrag.track = trackOf(rs);
    if (!hov) return;
    bool ok = canInteract(i, rs);
    bool c0 = ok && ImGui::IsMouseClicked(0), c1 = ok && ImGui::IsMouseClicked(1), c2 = ok && ImGui::IsMouseClicked(2);
    bool ctrlShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    switch (r.kind) {
    case Row::Mod: {
        auto& m = r.mod; auto& ui = modUi(m);
        std::string tip = m->mDescription;
        if (m->mKey > 0) tip += "  [" + keyName(m->mKey) + "]";
        mTooltip.pending = tip;
        if (c0) {
            m->toggle(); ui.pulse = 1.f;
            mRipples.push_back({ m.get(), mMouse.x - rs.x, mMouse.y - rs.y, P(4), 1.f });
            play("random.pop", m->mEnabled ? 1.1f : 0.85f); mClickConsumed = true;
        } else if (c1 && !m->mSettings.empty()) { ui.expanded = !ui.expanded; mClickConsumed = true; play("random.click"); }
        else if (c2) { mBind = { true, m, nullptr }; mClickConsumed = true; play("random.pop", 1.3f); }
        break;
    }
    case Row::Bool: {
        auto* s = static_cast<BoolSetting*>(r.set);
        mTooltip.pending = s->mDescription;
        if (c0) { s->mValue = !s->mValue; mClickConsumed = true; play("random.click", s->mValue ? 1.15f : 0.9f); }
        else if (c2) { mBind = { true, nullptr, s }; mClickConsumed = true; play("random.pop", 1.3f); }
        break;
    }
    case Row::Number: {
        auto* s = static_cast<NumberSetting*>(r.set);
        mTooltip.pending = s->mDescription + "  (Shift = fine, dbl-click = type)";
        if (ok && ImGui::IsMouseDoubleClicked(0)) { beginText(TextEdit::Number, s, fmtValue(s->mValue, s->mStep)); mClickConsumed = true; mTextClicked = true; }
        else if (c0 || c2) {
            mDrag = {}; mDrag.set = s; mDrag.mid = c2; mDrag.fine = ctrlShift && !c2;
            mDrag.anchorX = mMouse.x; mDrag.anchorVal = s->mValue; mDrag.track = trackOf(rs);
            mClickConsumed = true;
        }
        // No wheel handling here on purpose: the wheel always scrolls the list,
        // even when the cursor sits on a slider. Values are changed by dragging
        // (LMB/RMB, Shift = fine) or by double-clicking to type them.
        break;
    }
    case Row::Enum: {
        auto* s = static_cast<EnumSetting*>(r.set); auto& su = setUi(s);
        int n = (int)s->mValues.size();
        mTooltip.pending = s->mDescription + "  (RMB = list)";
        if (n > 0) {
            if (c0) { s->mValue = (s->mValue + 1) % n; mClickConsumed = true; play("random.click"); }
            else if (c1) { su.enumExpanded = !su.enumExpanded; mClickConsumed = true; }
            // Wheel never cycles enum values either — it scrolls the panel.
        }
        break;
    }
    case Row::EnumVal: {
        auto* s = static_cast<EnumSetting*>(r.set);
        if (c0) { s->mValue = r.idx; mClickConsumed = true; play("random.click"); }
        break;
    }
    case Row::Color: {
        auto* s = static_cast<ColorSetting*>(r.set);
        mTooltip.pending = s->mDescription;
        if (c0) { openPicker(s, rs); mClickConsumed = true; }
        break;
    }
    case Row::String: {
        auto* s = static_cast<StringSetting*>(r.set);
        mTooltip.pending = s->mDescription + "  (click to edit)";
        if (c0) {
            if (!(mText.active && mText.set == s)) beginText(TextEdit::String, s, s->mValue);
            mClickConsumed = true; mTextClicked = true;
        }
        break;
    }
    }
}

void ModernGui::processDrag()
{
    if (!mDrag.set) return;
    bool down = mDrag.mid ? ImGui::IsMouseDown(2) : ImGui::IsMouseDown(0);
    if (!down || !mIsEnabled) { mDrag.set = nullptr; return; }
    auto* s = mDrag.set;
    float w = std::max(1.f, mDrag.track.z - mDrag.track.x), range = s->mMax - s->mMin, v;
    if (mDrag.fine) v = mDrag.anchorVal + (mMouse.x - mDrag.anchorX) / w * range * 0.1f;
    else            v = s->mMin + std::clamp((mMouse.x - mDrag.track.x) / w, 0.f, 1.f) * range;
    float q = mDrag.mid ? mMidRound : std::max(s->mStep, 0.0001f);
    v = std::round(v / q) * q;
    s->mValue = std::clamp(v, s->mMin, s->mMax);
}

void ModernGui::inputSearch()
{
    float W = S(kCatW) * 1.25f;
    mSearchRect  = tf({ mCenter.x - W * 0.5f, S(12), mCenter.x + W * 0.5f, S(12) + S(36) });
    mSearchClear = { mSearchRect.z - P(30), mSearchRect.y, mSearchRect.z, mSearchRect.w };
    if (!mIsEnabled || mClickConsumed || mPicker.open || mBind.active) return;
    if (ImGui::IsMouseClicked(0) && in(mSearchRect, mMouse)) {
        mClickConsumed = true; mTextClicked = true;
        if (!mSearch.empty() && in(mSearchClear, mMouse)) { mSearch.clear(); if (mText.active && mText.kind == TextEdit::Search) mText.buf.clear(); return; }
        if (!(mText.active && mText.kind == TextEdit::Search)) beginText(TextEdit::Search, nullptr, mSearch);
    }
}

bool ModernGui::onKey(int vk, bool pressed)
{
    if (!mIsEnabled) return false;
    if (mBind.active) { if (pressed) setBindKey(vk); return true; }
    if (!pressed) return false;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    if (mText.active) {
        if (vk == VK_TAB) return mText.active;
        switch (vk) {
        case VK_ESCAPE: finishText(false); return true;
        case VK_RETURN: {
            bool search = mText.kind == TextEdit::Search;
            finishText(true);
            if (search && isSearchActive())
                for (auto& p : mPanels) for (auto& m : p.mods) { m->toggle(); modUi(m).pulse = 1.f; play("random.pop"); return true; }
            return true;
        }
        case VK_BACK:
            if (ctrl) { while (!mText.buf.empty() && mText.buf.back() == ' ') popUtf8(mText.buf); while (!mText.buf.empty() && mText.buf.back() != ' ') popUtf8(mText.buf); }
            else popUtf8(mText.buf);
            if (mText.kind == TextEdit::Search) mSearch = mText.buf;
            mText.blink = 0; return true;
        case VK_DELETE: return true;
        default: break;
        }
        if (ctrl) { if (vk == 'V') insertText(clipboardText()); return true; }
        std::string c = charFromKey(vk);
        if (!c.empty()) insertText(c);
        return true;
    }

    if (vk == VK_ESCAPE) {
        if (mPicker.open) { mPicker.open = false; return true; }
        if (isSearchActive()) { mSearch.clear(); return true; }
        return false;
    }
    if (mPicker.open || ctrl) return false;

    if (vk != VK_SPACE && vk != VK_TAB && vk != VK_RETURN) {
        std::string c = charFromKey(vk);
        if (!c.empty()) { beginText(TextEdit::Search, nullptr, mSearch); insertText(c); return true; }
    }
    return false;
}

void ModernGui::beginText(TextEdit::Kind k, Setting* s, const std::string& initial)
{
    if (mText.active) finishText(mText.kind != TextEdit::Search);
    mText = {}; mText.kind = k; mText.set = s; mText.buf = initial; mText.active = true; mText.blink = 0;
}
void ModernGui::finishText(bool commit)
{
    if (!mText.active) return;
    if (commit) switch (mText.kind) {
        case TextEdit::String: if (auto* s = static_cast<StringSetting*>(mText.set)) s->setValue(mText.buf); break;
        case TextEdit::Number: if (auto* s = static_cast<NumberSetting*>(mText.set)) {
            std::string b = mText.buf; std::replace(b.begin(), b.end(), ',', '.');
            char* e = nullptr; float v = std::strtof(b.c_str(), &e);
            if (e != b.c_str()) { v = std::round(v / std::max(s->mStep, 0.0001f)) * s->mStep; s->mValue = std::clamp(v, s->mMin, s->mMax); }
        } break;
        case TextEdit::Hex: if (mPicker.open && mPicker.set) {
            std::string b = mText.buf; if (!b.empty() && b[0] == '#') b.erase(0, 1);
            if (b.size() == 6) {
                unsigned v = std::strtoul(b.c_str(), nullptr, 16);
                float r = ((v >> 16) & 0xFF) / 255.f, g = ((v >> 8) & 0xFF) / 255.f, bl = (v & 0xFF) / 255.f;
                ImGui::ColorConvertRGBtoHSV(r, g, bl, mPicker.h, mPicker.s, mPicker.v); applyPicker();
            }
        } break;
        default: break;
    }
    if (mText.kind == TextEdit::Search) mSearch = mText.buf;
    mText = {};
}
void ModernGui::insertText(const std::string& utf8)
{
    if (utf8.empty()) return;
    size_t cap = mText.kind == TextEdit::Search ? 64 : mText.kind == TextEdit::Hex ? 7 : 120;
    if (mText.kind == TextEdit::Number) { for (char c : utf8) if (!(std::isdigit((unsigned char)c) || c == '.' || c == ',' || c == '-')) return; }
    if (mText.kind == TextEdit::Hex)    { for (char c : utf8) if (!std::isxdigit((unsigned char)c) && c != '#') return; }
    if (mText.buf.size() + utf8.size() <= cap) mText.buf += utf8;
    if (mText.kind == TextEdit::Search) mSearch = mText.buf;
    mText.blink = 0;
}

void ModernGui::openPicker(ColorSetting* cs, const ImVec4& anchor)
{
    if (mPicker.open && mPicker.set == cs) { mPicker.open = false; return; }
    mPicker = {}; mPicker.open = true; mPicker.set = cs; mPicker.anchor = anchor; mPicker.a = cs->mValue[3];
    ImGui::ColorConvertRGBtoHSV(cs->mValue[0], cs->mValue[1], cs->mValue[2], mPicker.h, mPicker.s, mPicker.v);
}
void ModernGui::applyPicker()
{
    if (!mPicker.set) return;
    float r, g, b; ImGui::ColorConvertHSVtoRGB(mPicker.h, mPicker.s, mPicker.v, r, g, b);
    mPicker.set->setValue(r, g, b, mPicker.a);
}
void ModernGui::layoutPicker()
{
    if (!mPicker.open) return;
    float pad = P(12), w = P(236), svH = P(150), bar = P(12), gap = P(10), hexH = P(24), preH = P(20);
    float h = pad + svH + gap + bar + gap + bar + gap + hexH + gap + preH + pad;
    float x = mPicker.anchor.z + P(10);
    if (x + w > mScreen.x - 4) x = mPicker.anchor.x - w - P(10);
    x = std::clamp(x, 4.f, std::max(4.f, mScreen.x - w - 4));
    float y = std::clamp(mPicker.anchor.y - P(8), 4.f, std::max(4.f, mScreen.y - h - 4));
    mPicker.rect = { x, y, x + w, y + h };
    float cy = y + pad;
    mPicker.sv  = { x + pad, cy, x + w - pad, cy + svH }; cy += svH + gap;
    mPicker.hue = { x + pad, cy, x + w - pad, cy + bar }; cy += bar + gap;
    mPicker.alp = { x + pad, cy, x + w - pad, cy + bar }; cy += bar + gap;
    mPicker.hex = { x + pad, cy, x + pad + P(110), cy + hexH }; cy += hexH + gap;
    mPicker.presetY = cy;
}
void ModernGui::inputPicker()
{
    if (!mPicker.open || !mIsEnabled) return;
    static const float presets[10][3] = { {1,1,1},{0,0,0},{1,.25f,.25f},{1,.6f,.15f},{1,.9f,.2f},{.3f,.9f,.4f},{.2f,.85f,.95f},{.3f,.5f,1},{.65f,.4f,1},{1,.45f,.8f} };
    if (ImGui::IsMouseClicked(0) && !mClickConsumed) {
        mClickConsumed = true;
        if (!in(mPicker.rect, mMouse)) { mPicker.open = false; return; }
        if (in(mPicker.sv, mMouse)) mPicker.drag = 1;
        else if (in(mPicker.hue, mMouse)) mPicker.drag = 2;
        else if (in(mPicker.alp, mMouse)) mPicker.drag = 3;
        else if (in(mPicker.hex, mMouse)) { float r, g, b; ImGui::ColorConvertHSVtoRGB(mPicker.h, mPicker.s, mPicker.v, r, g, b); beginText(TextEdit::Hex, nullptr, hexOf(r, g, b)); mTextClicked = true; }
        else {
            float sw = P(18), gap = P(4), x0 = mPicker.rect.x + P(12);
            for (int i = 0; i < 10; i++) {
                ImVec4 r = { x0 + i * (sw + gap), mPicker.presetY, x0 + i * (sw + gap) + sw, mPicker.presetY + sw };
                if (in(r, mMouse)) { ImGui::ColorConvertRGBtoHSV(presets[i][0], presets[i][1], presets[i][2], mPicker.h, mPicker.s, mPicker.v); applyPicker(); play("random.click"); }
            }
        }
    }
    if (mPicker.drag && ImGui::IsMouseDown(0)) {
        auto frac = [&](const ImVec4& r, bool vert) { return vert ? std::clamp((mMouse.y - r.y) / std::max(1.f, r.w - r.y), 0.f, 1.f) : std::clamp((mMouse.x - r.x) / std::max(1.f, r.z - r.x), 0.f, 1.f); };
        if (mPicker.drag == 1) { mPicker.s = frac(mPicker.sv, false); mPicker.v = 1.f - frac(mPicker.sv, true); }
        else if (mPicker.drag == 2) mPicker.h = std::min(frac(mPicker.hue, false), 0.9999f);
        else mPicker.a = frac(mPicker.alp, false);
        applyPicker();
    } else mPicker.drag = 0;
}
void ModernGui::drawPicker()
{
    mPicker.anim = smoothTo(mPicker.anim, mPicker.open ? 1.f : 0.f, 18.f, mDt);
    if (mPicker.anim < 0.01f || !mPicker.set) return;
    auto* dl = ImGui::GetForegroundDrawList();
    float a = mPicker.anim * mAnim, rnd = P(10);
    const ImVec4& R = mPicker.rect;
    float r, g, b; ImGui::ColorConvertHSVtoRGB(mPicker.h, mPicker.s, mPicker.v, r, g, b);
    float hr, hg, hb; ImGui::ColorConvertHSVtoRGB(mPicker.h, 1, 1, hr, hg, hb);
    ImColor cur = { r, g, b, 1.f }, hueCol = { hr, hg, hb, 1.f };

    dl->AddShadowRect(mn(R), mx(R), black(0.6f * a), P(24), { 0, P(6) }, 0, rnd);
    dl->AddRectFilled(mn(R), mx(R), rgba(16, 16, 24, 0.98f * a), rnd);
    dl->AddRect(mn(R), mx(R), white(0.1f * a), rnd);

    const ImVec4& sv = mPicker.sv;
    dl->AddRectFilledMultiColor(mn(sv), mx(sv), white(a), withA(hueCol, a), withA(hueCol, a), white(a));
    dl->AddRectFilledMultiColor(mn(sv), mx(sv), black(0), black(0), black(a), black(a));
    dl->AddRect(mn(sv), mx(sv), white(0.12f * a));
    ImVec2 svc = { sv.x + mPicker.s * (sv.z - sv.x), sv.y + (1 - mPicker.v) * (sv.w - sv.y) };
    dl->AddCircle(svc, P(6), black(0.6f * a), 16, 2.f); dl->AddCircle(svc, P(5), white(a), 16, 1.5f);

    const ImVec4& hu = mPicker.hue;
    for (int i = 0; i < 6; i++) {
        float x0 = hu.x + (hu.z - hu.x) * i / 6.f, x1 = hu.x + (hu.z - hu.x) * (i + 1) / 6.f;
        float r0, g0, b0, r1, g1, b1;
        ImGui::ColorConvertHSVtoRGB(i / 6.f, 1, 1, r0, g0, b0); ImGui::ColorConvertHSVtoRGB((i + 1) / 6.f - 0.0001f, 1, 1, r1, g1, b1);
        dl->AddRectFilledMultiColor({ x0, hu.y }, { x1, hu.w }, ImColor(r0, g0, b0, a), ImColor(r1, g1, b1, a), ImColor(r1, g1, b1, a), ImColor(r0, g0, b0, a));
    }
    dl->AddRect(mn(hu), mx(hu), white(0.12f * a), P(3));
    float hx = hu.x + mPicker.h * (hu.z - hu.x);
    dl->AddRectFilled({ hx - P(2), hu.y - P(2) }, { hx + P(2), hu.w + P(2) }, white(a), P(2));
    dl->AddRect({ hx - P(2), hu.y - P(2) }, { hx + P(2), hu.w + P(2) }, black(0.6f * a), P(2));

    const ImVec4& al = mPicker.alp;
    checker(dl, al, P(6), a, P(3));
    dl->AddRectFilledMultiColor(mn(al), mx(al), withA(cur, 0), withA(cur, a), withA(cur, a), withA(cur, 0));
    dl->AddRect(mn(al), mx(al), white(0.12f * a), P(3));
    float ax = al.x + mPicker.a * (al.z - al.x);
    dl->AddRectFilled({ ax - P(2), al.y - P(2) }, { ax + P(2), al.w + P(2) }, white(a), P(2));
    dl->AddRect({ ax - P(2), al.y - P(2) }, { ax + P(2), al.w + P(2) }, black(0.6f * a), P(2));

    const ImVec4& hx4 = mPicker.hex;
    bool editing = mText.active && mText.kind == TextEdit::Hex;
    dl->AddRectFilled(mn(hx4), mx(hx4), white(0.06f * a), P(5));
    dl->AddRect(mn(hx4), mx(hx4), editing ? withA(cur, a) : ImColor(1.f, 1.f, 1.f, 0.1f * a), P(5));
    std::string hs = editing ? mText.buf : hexOf(r, g, b);
    float ty = (hx4.y + hx4.w) * 0.5f - mTextH * 0.45f;
    text(dl, { hx4.x + P(8), ty }, hs, ImColor(1.f, 1.f, 1.f, 0.85f), 0.9f, a);
    if (editing) caret(dl, hx4.x + P(8) + tw(hs, 0.9f) + P(1), ty, mTextH * 0.9f, a);
    ImVec4 pv = { R.z - P(12) - P(56), hx4.y, R.z - P(12), hx4.w };
    checker(dl, pv, P(6), a, P(5));
    dl->AddRectFilled(mn(pv), mx(pv), withA(cur, mPicker.a * a), P(5));
    dl->AddRect(mn(pv), mx(pv), white(0.12f * a), P(5));

    static const float presets[10][3] = { {1,1,1},{0,0,0},{1,.25f,.25f},{1,.6f,.15f},{1,.9f,.2f},{.3f,.9f,.4f},{.2f,.85f,.95f},{.3f,.5f,1},{.65f,.4f,1},{1,.45f,.8f} };
    float sw = P(18), gap = P(4), x0 = R.x + P(12);
    for (int i = 0; i < 10; i++) {
        ImVec4 pr = { x0 + i * (sw + gap), mPicker.presetY, x0 + i * (sw + gap) + sw, mPicker.presetY + sw };
        dl->AddRectFilled(mn(pr), mx(pr), ImColor(presets[i][0], presets[i][1], presets[i][2], a), P(4));
        dl->AddRect(mn(pr), mx(pr), white((in(pr, mMouse) ? 0.6f : 0.12f) * a), P(4));
    }
}

void ModernGui::drawBackground(bool ambient)
{
    if (mBlur > 0.01f) ImRenderUtils::addBlur(ImVec4(0, 0, mScreen.x, mScreen.y), mAnim * mBlur, 0);
    mDl->AddRectFilled({ 0, 0 }, mScreen, black((0.48f + 0.04f * mFxIntensity) * mAnim));

    ImColor ca = ColorUtils::getStaticAccentColor(mTime * 18.f);
    ImColor cb = ColorUtils::getStaticAccentColor(mTime * 18.f + 110.f);
    float wash = 0.035f * mFxIntensity * mAnim;
    mDl->AddRectFilledMultiColor({0, 0}, mScreen,
        withA(ca, wash * 1.25f), withA(cb, wash * 0.65f),
        withA(ca, wash * 0.28f), withA(cb, wash * 0.85f));

    if (mBackdropGrid) {
        float step = std::max(P(44.f), 22.f);
        float ox = std::fmod(mTime * P(5.f) * mFxIntensity, step);
        float oy = std::fmod(mTime * P(3.f) * mFxIntensity, step);
        ImU32 grid = white(0.018f * mFxIntensity * mAnim);
        ImU32 dot = withA(ca, 0.075f * mFxIntensity * mAnim);
        for (float x = ox; x < mScreen.x; x += step)
            mDl->AddLine({x, 0}, {x, mScreen.y}, grid, 1.f);
        for (float y = oy; y < mScreen.y; y += step)
            mDl->AddLine({0, y}, {mScreen.x, y}, grid, 1.f);
        for (float x = ox; x < mScreen.x; x += step * 2.f)
            for (float y = oy; y < mScreen.y; y += step * 2.f)
                mDl->AddCircleFilled({x, y}, std::max(1.f, P(1.f)), dot, 8);
    }
    if (ambient) drawOrbs();
    ImColor lineA = ColorUtils::getThemedColor((int)(mTime * 25));
    ImColor lineB = ColorUtils::getThemedColor((int)(mTime * 25 + 120));
    mDl->AddRectFilledMultiColor({0, 0}, {mScreen.x, P(2)},
        withA(lineA, 0.62f * mAnim), withA(lineB, 0.62f * mAnim),
        withA(lineB, 0.62f * mAnim), withA(lineA, 0.62f * mAnim));
    float head = std::fmod(mTime * 0.18f * mFxIntensity, 1.f) * mScreen.x;
    float tail = P(110.f);
    mDl->AddRectFilledMultiColor({head-tail, 0}, {head, P(2.8f)},
        withA(lineA, 0.f), withA(lineB, 0.95f * mAnim),
        withA(lineB, 0.95f * mAnim), withA(lineA, 0.f));
}

void ModernGui::drawOrbs()
{
    if (mScreen.x < 2 || mScreen.y < 2) return;
    if (mOrbs.empty()) {
        std::mt19937 rng((unsigned)GetTickCount64());
        std::uniform_real_distribution<float> ux(0, mScreen.x), uy(0, mScreen.y), ua(0, 2 * kPi), us(8, 22), ur(50, 130);
        for (int i = 0; i < 10; i++) { float an = ua(rng), sp = us(rng); mOrbs.push_back({ ux(rng), uy(rng), std::cos(an) * sp, std::sin(an) * sp, S(ur(rng)), ua(rng), i * 36 }); }
    }
    for (auto& o : mOrbs) {
        o.x += o.vx * mDt; o.y += o.vy * mDt;
        if (o.x < -o.r) o.x = mScreen.x + o.r; if (o.x > mScreen.x + o.r) o.x = -o.r;
        if (o.y < -o.r) o.y = mScreen.y + o.r; if (o.y > mScreen.y + o.r) o.y = -o.r;
        float br = 0.75f + 0.25f * std::sin(mTime * 0.8f + o.phase);
        ImColor c = ColorUtils::getThemedColor(o.hue);
        float px = 0.f, py = 0.f;
        if (mParallax) {
            px = (mMouse.x - mCenter.x) / std::max(mScreen.x, 1.f) * o.r * 0.18f;
            py = (mMouse.y - mCenter.y) / std::max(mScreen.y, 1.f) * o.r * 0.18f;
        }
        ImVec2 p = {o.x + std::sin(mTime * 0.5f + o.phase) * 12.f + px,
                    o.y + std::cos(mTime * 0.4f + o.phase) * 12.f + py};
        float intensity = mFxIntensity * mAnim;
        mDl->AddCircleFilled(p, o.r, withA(c, 0.026f * br * intensity), 32);
        mDl->AddCircleFilled(p, o.r * 0.6f, withA(c, 0.038f * br * intensity), 24);
        mDl->AddCircleFilled(p, o.r * 0.3f, withA(c, 0.055f * br * intensity), 16);
    }
}

void ModernGui::drawCursorFx()
{
    if (!mCursorGlow || mAnim < 0.01f) return;
    // Foreground list: the trail stays above every panel/tooltip/hint of the ClickGui.
    auto* dl = ImGui::GetForegroundDrawList();
    for (const auto& p : mCursorTrail) {
        float life = std::clamp(p.life, 0.f, 1.f);
        ImColor c = ColorUtils::getStaticAccentColor(p.hue);
        c.Value.w = std::min(1.f, life * life * 0.16f * mFxIntensity * mAnim);
        dl->AddCircleFilled(p.pos, p.radius * (0.65f + 0.35f * life), c, 18);
        ImColor rim = c; rim.Value.w = std::min(1.f, rim.Value.w * 1.5f);
        dl->AddCircle(p.pos, p.radius * (0.72f + 0.28f * life), rim, 18, 1.f);
    }
    ImColor c = ColorUtils::getStaticAccentColor(mTime * 30.f);
    float pulse = 0.5f + 0.5f * std::sin(mTime * 3.f);
    dl->AddCircleFilled(mMouse, P(22.f + pulse * 3.f), withA(c, 0.03f * mFxIntensity * mAnim), 28);
    dl->AddCircle(mMouse, P(8.f + pulse * 1.5f), withA(c, 0.25f * mFxIntensity * mAnim), 24, std::max(1.f, P(1.f)));
    dl->AddCircleFilled(mMouse, std::max(1.2f, P(1.4f)), white(0.75f * mAnim), 10);
}

void ModernGui::drawPanel(size_t i)
{
    Panel& p = mPanels[i];
    if (!panelVisible(i)) return;
    p.hover = smoothTo(p.hover, mHoveredCat == (int)i ? 1.f : 0.f, 14.f, mDt);
    p.focus = smoothTo(p.focus, (!mDrawOrder.empty() && mDrawOrder.back() == i) ? 1.f : 0.f, 10.f, mDt);
    float rnd = P(kRound);
    bool hasContent = p.visH > 0.5f;
    ImDrawFlags winFlags = ImDrawFlags_RoundCornersTop | ((!hasContent || p.roundBottom) ? ImDrawFlags_RoundCornersBottom : 0);

    ImColor panelAcc = ColorUtils::getStaticAccentColor((float)i * 34.f + mTime * 7.f);
    if (mPanelGlow) {
        float glowA = (0.075f + 0.055f * p.hover + 0.065f * p.focus) * mFxIntensity * mAnim;
        mDl->AddShadowRect(mn(p.window), mx(p.window), withA(panelAcc, glowA),
            P(18.f + 6.f * p.focus) * mFxIntensity, {0, P(3)}, 0, rnd + P(1));
    }
    mDl->AddShadowRect(mn(p.window), mx(p.window), black((0.45f + 0.2f * p.focus) * mAnim), P(22), { 0, P(6) }, 0, rnd);

    if (hasContent) {
        mDl->PushClipRect(mn(p.content), mx(p.content), true);
        // Theme-tinted panel body instead of the flat dark gray
        ImColor body = ColorUtils::getStaticAccentColor((float)(i * 8));
        mDl->AddRectFilled(mn(p.content), mx(p.content), rgba(
            (int)(12 + body.Value.x * 14.f),
            (int)(12 + body.Value.y * 14.f),
            (int)(18 + body.Value.z * 22.f),
            0.97f * mAnim), rnd, p.roundBottom ? ImDrawFlags_RoundCornersBottom : 0);
        for (auto& r : p.rows) {
            ImVec4 rs = rowRect(p, r);
            if (rs.w < p.content.y || rs.y > p.content.w || r.h < 0.5f) continue;
            drawRow(i, r, rs, hovering(i, rs));
        }
        if (!p.roundBottom)
            mDl->AddRectFilledMultiColor({ p.content.x, p.content.w - P(14) }, mx(p.content), black(0), black(0), black(0.5f * mAnim), black(0.5f * mAnim));
        mDl->PopClipRect();
        drawScrollbar(p);
    }
    drawHeader(i);
    mDl->AddRect(mn(p.window), mx(p.window), white((0.07f + 0.09f * p.focus) * mAnim), rnd, winFlags, 1.f);

    float travel = std::fmod(mTime * 0.20f * mFxIntensity + (float)i * 0.17f, 1.f);
    float gx = p.window.x + (p.window.z - p.window.x) * travel;
    float gw = P(34.f);
    mDl->PushClipRect(mn(p.window), mx(p.window), true);
    mDl->AddRectFilledMultiColor({gx-gw, p.window.y}, {gx+gw, p.window.y+P(1.5f)},
        withA(panelAcc, 0.f), withA(panelAcc, (0.55f+0.25f*p.focus)*mAnim),
        withA(panelAcc, (0.55f+0.25f*p.focus)*mAnim), withA(panelAcc, 0.f));
    mDl->PopClipRect();
}

void ModernGui::drawScrollbar(Panel& p)
{
    p.scrollbarAlpha = smoothTo(p.scrollbarAlpha, (p.maxScroll > 0 && (p.hover > 0.5f || std::fabs(p.scroll - p.scrollAnim) > 0.5f)) ? 1.f : 0.f, 6.f, mDt);
    if (p.scrollbarAlpha < 0.01f || p.maxScroll <= 0) return;
    float trackH = p.content.w - p.content.y - P(8), visFrac = (p.visH / std::max(1.f, p.contentH));
    float thumbH = std::max(P(20), trackH * visFrac), thumbY = p.content.y + P(4) + (trackH - thumbH) * (p.scrollAnim / p.maxScroll);
    float x = p.content.z - P(5);
    mDl->AddRectFilled({ x - P(2), thumbY }, { x, thumbY + thumbH }, white(0.28f * p.scrollbarAlpha * mAnim), P(2));
}

void ModernGui::drawHeader(size_t i)
{
    Panel& p = mPanels[i];
    const ImVec4& h = p.header;
    float rnd = P(kRound);
    bool hasContent = p.visH > 0.5f;
    ImDrawFlags fl = hasContent ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll;
    ImColor acc = accent(i, 0);

    // Theme-tinted header matching the category accent
    ImColor headTint = ColorUtils::getStaticAccentColor((float)(i * 8));
    mDl->AddRectFilled(mn(h), mx(h), rgba(
        (int)(18 + headTint.Value.x * 22.f),
        (int)(18 + headTint.Value.y * 22.f),
        (int)(26 + headTint.Value.z * 34.f),
        0.985f * mAnim), rnd, fl);
    if (p.hover > 0.01f) mDl->AddRectFilled(mn(h), mx(h), white(0.04f * p.hover * mAnim), rnd, fl);
    float breath = 0.55f + 0.2f * std::sin(mTime * 2.5f + i);
    mDl->AddRectFilled({ h.x + P(14), h.y }, { h.z - P(14), h.y + P(2) }, withA(acc, (breath * 0.6f + 0.4f * p.focus) * mAnim), P(1));
    if (hasContent) mDl->AddLine({ h.x, h.w - 0.5f }, { h.z, h.w - 0.5f }, withA(headTint, 0.22f * mAnim));

    std::string name = cats()[i];
    if (mLowercase) name = StringUtils::toLower(name);
    std::string icon = "B";
    if (StringUtils::equalsIgnoreCase(name, "Combat")) icon = "c";
    else if (StringUtils::equalsIgnoreCase(name, "Movement")) icon = "f";
    else if (StringUtils::equalsIgnoreCase(name, "Visual")) icon = "d";
    else if (StringUtils::equalsIgnoreCase(name, "Player")) icon = "e";
    else if (StringUtils::equalsIgnoreCase(name, "Misc")) icon = "a";

    float cy = (h.y + h.w) * 0.5f, x = h.x + P(12);
    auto it = FontHelper::Fonts.find("tenacity_icons_large");
    if (it != FontHelper::Fonts.end() && it->second) {
        ImGui::PushFont(it->second);
        float sz = mTextPx * 1.1f;
        ImVec2 isz = it->second->CalcTextSizeA(sz, FLT_MAX, 0, icon.c_str());
        mDl->AddText(it->second, sz, { std::floor(x), std::floor(cy - isz.y * 0.5f) }, withA(acc, 0.95f * mAnim), icon.c_str());
        ImGui::PopFont();
        x += isz.x + P(10);
    }
    text(mDl, { x, cy - mTextH * 0.55f }, name, ImColor(1.f, 1.f, 1.f, 0.92f), 1.05f);

    std::string c = std::to_string(p.mods.size());
    float cw = tw(c, 0.68f), badgeW = cw + P(10), badgeH = P(17);
    ImVec4 badge = {h.z-P(34)-badgeW, cy-badgeH*0.5f, h.z-P(34), cy+badgeH*0.5f};
    mDl->AddRectFilled(mn(badge), mx(badge), withA(acc, (isSearchActive()?0.20f:0.10f)*mAnim), P(5));
    mDl->AddRect(mn(badge), mx(badge), withA(acc, (isSearchActive()?0.38f:0.18f)*mAnim), P(5));
    text(mDl, {badge.x+P(5), cy-mTextH*0.34f}, c,
        ImColor(1.f,1.f,1.f,isSearchActive()?0.72f:0.42f), 0.68f);
    chevron(mDl, { h.z - P(18), cy }, P(4), p.expand, white(0.35f * mAnim), std::max(1.f, P(1.6f)));
}

void ModernGui::drawRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    switch (r.kind) {
    case Row::Mod:     drawModRow(i, r, rs, hov); break;
    case Row::Bool:    drawBoolRow(i, r, rs, hov); break;
    case Row::Number:  drawNumberRow(i, r, rs, hov); break;
    case Row::Enum:    drawEnumRow(i, r, rs, hov); break;
    case Row::EnumVal: drawEnumValRow(i, r, rs, hov); break;
    case Row::Color:   drawColorRow(i, r, rs, hov); break;
    case Row::String:  drawStringRow(i, r, rs, hov); break;
    }
}

void ModernGui::settingBg(const Row& r, const ImVec4& rs, float hover)
{
    float a = r.alpha * mAnim;
    ImColor tint = ColorUtils::getStaticAccentColor((float)(r.modIdx * 4));
    mDl->AddRectFilled(mn(rs), mx(rs), rgba(
        (int)(8 + tint.Value.x * 8.f),
        (int)(8 + tint.Value.y * 8.f),
        (int)(13 + tint.Value.z * 12.f),
        0.94f * a));
    mDl->AddLine({ rs.x, rs.y }, { rs.z, rs.y }, white(0.045f * a));
    mDl->AddRectFilled({ rs.x + P(7), rs.y }, { rs.x + P(8), rs.w }, withA(tint, 0.22f * a));
    if (hover > 0.01f) mDl->AddRectFilled(mn(rs), mx(rs), white(0.045f * hover * a));
}

void ModernGui::drawModRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto& m = r.mod; auto& ui = modUi(m);
    auto* daInterface = gFeatureManager->mModuleManager->getModule<Interface>();
    ImColor acc = accent(i, r.modIdx);
    ui.enable = smoothTo(ui.enable, m->mEnabled ? 1.f : 0.f, 14.f, mDt);
    ui.hover  = smoothTo(ui.hover, hov ? 1.f : 0.f, 14.f, mDt);
    ui.pulse  = smoothTo(ui.pulse, 0.f, 4.f, mDt);
    float a = mAnim, cy = (rs.y + rs.w) * 0.5f;

    ImColor tint = ColorUtils::getStaticAccentColor((float)(r.modIdx * 4));
    mDl->AddRectFilled(mn(rs), mx(rs), rgba(
        (int)(14 + tint.Value.x * 12.f),
        (int)(14 + tint.Value.y * 12.f),
        (int)(20 + tint.Value.z * 18.f),
        0.97f * a));
    if (ui.enable > 0.005f) {
        float fw = (rs.z - rs.x) * (0.35f + 0.65f * ui.enable);
        mDl->AddRectFilledMultiColor(mn(rs), { rs.x + fw, rs.w }, withA(acc, 0.18f * ui.enable * a), withA(acc, 0.02f * ui.enable * a), withA(acc, 0.02f * ui.enable * a), withA(acc, 0.18f * ui.enable * a));
        float bh = (rs.w - rs.y - P(12)) * ui.enable, by = cy - bh * 0.5f;
        mDl->AddRectFilled({ rs.x, by }, { rs.x + P(3), by + bh }, withA(acc, a), P(1.5f));
        mDl->AddShadowRect({ rs.x, by }, { rs.x + P(3), by + bh }, withA(acc, 0.5f * ui.enable * a * (daInterface ? daInterface->mGlowBoost.mValue : 1.f)), P(8), { 0, 0 }, 0, P(1.5f));
    }
    if (ui.hover > 0.01f) {
        mDl->AddRectFilled(mn(rs), mx(rs), white(0.05f * ui.hover * a));
        float phase = std::fmod(mTime * 0.42f + r.modIdx * 0.09f, 1.f);
        float sx = rs.x + (rs.z - rs.x) * phase, sw = P(26.f);
        mDl->PushClipRect(mn(rs), mx(rs), true);
        mDl->AddRectFilledMultiColor({sx-sw,rs.y},{sx+sw,rs.w},
            white(0), white(0.055f*ui.hover*a), white(0.055f*ui.hover*a), white(0));
        mDl->PopClipRect();
    }
    if (ui.pulse > 0.01f) mDl->AddRectFilled(mn(rs), mx(rs), withA(acc, 0.2f * ui.pulse * a));
    mDl->AddLine({ rs.x, rs.w - 0.5f }, { rs.z, rs.w - 0.5f }, white(0.05f * a));

    for (auto& rp : mRipples) if (rp.owner == m.get()) {
        mDl->PushClipRect(mn(rs), mx(rs), true);
        mDl->AddCircleFilled({ rs.x + rp.ox, rs.y + rp.oy }, rp.r, withA(acc, rp.a * 0.3f * a), 32);
        mDl->AddCircle({ rs.x + rp.ox, rs.y + rp.oy }, rp.r, withA(acc, rp.a * 0.42f * a), 32, P(1));
        mDl->PopClipRect();
    }

    std::string name = m->getName();
    float dotR = P(2.2f + ui.enable * 0.8f);
    ImVec2 stateDot = {rs.x + P(10), cy};
    ImColor dotCol = ui.enable > 0.01f ? withA(acc, (0.35f + 0.65f * ui.enable) * a)
                                       : ImColor(1.f, 1.f, 1.f, 0.13f * a);
    mDl->AddCircleFilled(stateDot, dotR, dotCol, 12);
    if (ui.enable > 0.05f)
        mDl->AddShadowRect({stateDot.x-dotR,stateDot.y-dotR},{stateDot.x+dotR,stateDot.y+dotR},
            withA(acc,0.42f*ui.enable*a),P(7),{0,0},0,dotR);

    float nx = rs.x + P(18), ny = cy - mTextH * 0.5f;
    if (isSearchActive() && ui.searchHit == 1) {
        std::string ln = lower(name), q = lower(mSearch), q2 = lower(usEquivalent(mSearch));
        size_t pos = ln.find(q); size_t len = q.size();
        if (pos == std::string::npos && !q2.empty()) { pos = ln.find(q2); len = q2.size(); }
        if (pos != std::string::npos) {
            float x0 = nx + tw(name.substr(0, pos)), x1 = x0 + tw(name.substr(pos, len));
            mDl->AddRectFilled({ x0 - P(2), ny - P(1) }, { x1 + P(2), ny + mTextH + P(1) }, withA(acc, 0.28f * a), P(3));
        }
    }
    text(mDl, { nx, ny }, name, ImColor(1.f, 1.f, 1.f, 0.6f + 0.4f * ui.enable));

    float right = rs.z - P(12);
    if (!m->mSettings.empty()) {
        ImColor cc = ui.open > 0.5f ? acc : ImColor(1.f, 1.f, 1.f, 0.3f);
        chevron(mDl, { right - P(4), cy }, P(3.5f), ui.open, withA(cc, cc.Value.w * a), std::max(1.f, P(1.5f)));
        right -= P(20);
    }
    bool binding = mBind.active && mBind.mod == m;
    if (m->mKey > 0 || binding) {
        std::string lbl = binding ? (std::fmod(mTime, 0.8f) < 0.4f ? "..." : "   ") : keyName(m->mKey);
        keyBadge(right, cy, lbl, acc, 1.f, binding || ui.enable > 0.5f);
    }
}

void ModernGui::drawBoolRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<BoolSetting*>(r.set); auto& su = setUi(s);
    ImColor acc = accent(i, r.modIdx);
    su.hover = smoothTo(su.hover, hov ? 1.f : 0.f, 14.f, mDt);
    su.toggle = smoothTo(su.toggle, s->mValue ? 1.f : 0.f, 16.f, mDt);
    settingBg(r, rs, su.hover);
    float cy = (rs.y + rs.w) * 0.5f, a = r.alpha;
    text(mDl, { rs.x + P(18), cy - mTextH * 0.5f }, settingName(s), ImColor(1.f, 1.f, 1.f, 0.55f + 0.35f * su.toggle), 0.95f, a);
    toggleSwitch({ rs.z - P(26), cy }, su.toggle, acc, a);
    bool binding = mBind.active && mBind.set == s;
    if (s->mKey > 0 || binding) {
        std::string lbl = binding ? (std::fmod(mTime, 0.8f) < 0.4f ? "..." : "   ") : keyName(s->mKey);
        keyBadge(rs.z - P(48), cy, lbl, acc, a, binding || s->mValue);
    }
}

void ModernGui::drawNumberRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<NumberSetting*>(r.set); auto& su = setUi(s);
    ImColor acc = accent(i, r.modIdx);
    su.hover = smoothTo(su.hover, hov ? 1.f : 0.f, 14.f, mDt);
    settingBg(r, rs, su.hover);
    float a = r.alpha;
    bool drag = mDrag.set == s, editing = mText.active && mText.set == s;

    text(mDl, { rs.x + P(18), rs.y + P(6) }, settingName(s), ImColor(1.f, 1.f, 1.f, 0.7f), 0.92f, a);
    std::string val = editing ? mText.buf : fmtValue(s->mValue, s->mStep);
    float vw = tw(val, 0.92f), vx = rs.z - P(14) - vw;
    text(mDl, { vx, rs.y + P(6) }, val, (drag || editing) ? acc : ImColor(1.f, 1.f, 1.f, 0.45f), 0.92f, a);
    if (editing) {
        caret(mDl, vx + vw + P(1), rs.y + P(6), mTextH * 0.92f, a);
        mDl->AddRectFilled({ vx - P(2), rs.y + P(6) + mTextH * 0.92f }, { rs.z - P(12), rs.y + P(6) + mTextH * 0.92f + P(1.5f) }, withA(acc, a * mAnim));
    }

    ImVec4 tr = trackOf(rs);
    float range = s->mMax - s->mMin, pct = range > 0 ? std::clamp((s->mValue - s->mMin) / range, 0.f, 1.f) : 0.f;
    su.slide = smoothTo(su.slide, pct, 20.f, mDt);
    float ty = (tr.y + tr.w) * 0.5f, tw_ = std::max(1.f, tr.z - tr.x), fx = tr.x + tw_ * su.slide;
    mDl->AddRectFilled({ tr.x, ty - P(2) }, { tr.z, ty + P(2) }, white(0.1f * a * mAnim), P(2));
    mDl->AddRectFilled({ tr.x, ty - P(2) }, { fx, ty + P(2) }, withA(acc, a * mAnim), P(2));
    if (a > 0.5f) mDl->AddShadowRect({ tr.x, ty - P(2) }, { fx, ty + P(2) }, withA(acc, 0.35f * a * mAnim), P(8), { 0, 0 }, 0, P(2));

    su.knob = smoothTo(su.knob, drag ? 1.3f : hov ? 1.12f : 1.f, 16.f, mDt);
    float kr = P(5.5f) * su.knob;
    mDl->AddCircleFilled({ fx, ty }, kr, withA(acc, a * mAnim), 16);
    mDl->AddCircleFilled({ fx, ty }, kr * 0.42f, white(0.92f * a * mAnim), 12);

    if (drag) {
        auto* fg = ImGui::GetForegroundDrawList();
        std::string b = fmtValue(s->mValue, s->mStep) + (mDrag.fine ? " (fine)" : "");
        float bw = tw(b, 0.8f) + P(12), bh = mTextH * 0.8f + P(8);
        ImVec4 br = { fx - bw * 0.5f, ty - P(16) - bh, fx + bw * 0.5f, ty - P(16) };
        fg->AddShadowRect(mn(br), mx(br), black(0.5f * mAnim), P(10), { 0, P(2) }, 0, P(5));
        fg->AddRectFilled(mn(br), mx(br), rgba(20, 20, 28, 0.98f * mAnim), P(5));
        fg->AddRect(mn(br), mx(br), withA(acc, 0.6f * mAnim), P(5));
        text(fg, { br.x + P(6), br.y + P(4) }, b, ImColor(1.f, 1.f, 1.f, 0.95f), 0.8f);
    }
}

void ModernGui::drawEnumRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<EnumSetting*>(r.set); auto& su = setUi(s);
    ImColor acc = accent(i, r.modIdx);
    su.hover = smoothTo(su.hover, hov ? 1.f : 0.f, 14.f, mDt);
    settingBg(r, rs, su.hover);
    float cy = (rs.y + rs.w) * 0.5f, a = r.alpha;
    text(mDl, { rs.x + P(18), cy - mTextH * 0.5f }, settingName(s), ImColor(1.f, 1.f, 1.f, 0.7f), 0.95f, a);
    if (s->mValues.empty()) return;
    std::string cur = s->mValues[std::clamp(s->mValue, 0, (int)s->mValues.size() - 1)];
    if (mLowercase) cur = StringUtils::toLower(cur);
    float cw = tw(cur, 0.95f);
    ImVec4 valueBadge = {rs.z-P(34)-cw-P(8), cy-P(9), rs.z-P(26), cy+P(9)};
    mDl->AddRectFilled(mn(valueBadge), mx(valueBadge), withA(acc, 0.10f*a*mAnim), P(5));
    mDl->AddRect(mn(valueBadge), mx(valueBadge), withA(acc, 0.22f*a*mAnim), P(5));
    text(mDl, { valueBadge.x + P(4), cy - mTextH * 0.5f }, cur, withA(acc, 0.94f), 0.95f, a);
    chevron(mDl, { rs.z - P(16), cy }, P(3.5f), su.enumOpen, white(0.35f * a * mAnim), std::max(1.f, P(1.5f)));
}

void ModernGui::drawEnumValRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<EnumSetting*>(r.set);
    ImColor acc = accent(i, r.modIdx);
    float a = r.alpha * mAnim, cy = (rs.y + rs.w) * 0.5f;
    bool sel = s->mValue == r.idx;
    mDl->AddRectFilled(mn(rs), mx(rs), rgba(5, 5, 9, 0.96f * a));
    if (hov) mDl->AddRectFilled(mn(rs), mx(rs), white(0.04f * a));
    if (sel) {
        mDl->AddRectFilled(mn(rs), mx(rs), withA(acc, 0.1f * a));
        mDl->AddRectFilled({ rs.x + P(7), rs.y + P(6) }, { rs.x + P(9), rs.w - P(6) }, withA(acc, a), P(1));
    }
    std::string v = s->mValues[r.idx]; if (mLowercase) v = StringUtils::toLower(v);
    text(mDl, { rs.x + P(30), cy - mTextH * 0.5f }, v, ImColor(1.f, 1.f, 1.f, sel ? 0.95f : 0.45f), 0.92f, r.alpha);
}

void ModernGui::drawColorRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<ColorSetting*>(r.set); auto& su = setUi(s);
    ImColor acc = accent(i, r.modIdx);
    su.hover = smoothTo(su.hover, hov ? 1.f : 0.f, 14.f, mDt);
    settingBg(r, rs, su.hover);
    float cy = (rs.y + rs.w) * 0.5f, a = r.alpha;
    text(mDl, { rs.x + P(18), cy - mTextH * 0.5f }, settingName(s), ImColor(1.f, 1.f, 1.f, 0.7f), 0.95f, a);
    ImColor c = s->getAsImColor();
    ImVec4 sw = { rs.z - P(40), cy - P(8), rs.z - P(12), cy + P(8) };
    checker(mDl, sw, P(5), a * mAnim, P(4));
    mDl->AddRectFilled(mn(sw), mx(sw), withA(c, c.Value.w * a * mAnim), P(4));
    bool open = mPicker.open && mPicker.set == s;
    mDl->AddRect(mn(sw), mx(sw), open ? withA(acc, a * mAnim) : ImColor(1.f, 1.f, 1.f, 0.15f * a * mAnim), P(4), 0, open ? 1.5f : 1.f);
    if (a > 0.5f) mDl->AddShadowRect(mn(sw), mx(sw), withA(c, 0.35f * a * mAnim), P(8), { 0, 0 }, 0, P(4));
}

void ModernGui::drawStringRow(size_t i, const Row& r, const ImVec4& rs, bool hov)
{
    auto* s = static_cast<StringSetting*>(r.set); auto& su = setUi(s);
    ImColor acc = accent(i, r.modIdx);
    su.hover = smoothTo(su.hover, hov ? 1.f : 0.f, 14.f, mDt);
    settingBg(r, rs, su.hover);
    float cy = (rs.y + rs.w) * 0.5f, a = r.alpha;
    std::string name = settingName(s);
    text(mDl, { rs.x + P(18), cy - mTextH * 0.5f }, name, ImColor(1.f, 1.f, 1.f, 0.7f), 0.95f, a);
    bool editing = mText.active && mText.set == s;
    std::string raw = editing ? mText.buf : s->mValue;
    std::string shown = s->mIsPassword ? std::string(toWide(raw).size(), '*') : raw;
    if (shown.empty() && !editing) shown = "...";
    float maxW = (rs.z - rs.x) - P(40) - tw(name, 0.95f);
    while (!shown.empty() && tw(shown, 0.92f) > maxW) shown.erase(0, 1);
    float vw = tw(shown, 0.92f), vx = rs.z - P(14) - vw, ty = cy - mTextH * 0.46f;
    text(mDl, { vx, ty }, shown, editing ? acc : ImColor(1.f, 1.f, 1.f, 0.5f), 0.92f, a);
    if (editing) {
        caret(mDl, vx + vw + P(1), ty, mTextH * 0.92f, a);
        mDl->AddRectFilled({ std::min(vx, rs.z - P(80)) - P(2), rs.w - P(4) }, { rs.z - P(12), rs.w - P(2.5f) }, withA(acc, a * mAnim));
    }
}

void ModernGui::drawSearch()
{
    if (mAnim < 0.01f) return;
    const ImVec4& r = mSearchRect;
    bool focused = mText.active && mText.kind == TextEdit::Search;
    mSearchFocus = smoothTo(mSearchFocus, focused ? 1.f : 0.f, 12.f, mDt);
    float rnd = P(10), cy = (r.y + r.w) * 0.5f;
    ImColor acc = ColorUtils::getThemedColor(0);

    mDl->AddShadowRect(mn(r), mx(r), withA(acc, (0.07f+0.08f*mSearchFocus)*mFxIntensity*mAnim),
        P(22)*mFxIntensity, {0,P(3)}, 0, rnd);
    mDl->AddShadowRect(mn(r), mx(r), black(0.45f * mAnim), P(18), { 0, P(4) }, 0, rnd);
    ImColor searchTint = ColorUtils::getStaticAccentColor(mTime * 8.f + 40.f);
    mDl->AddRectFilled(mn(r), mx(r), rgba(
        (int)(13+searchTint.Value.x*8.f), (int)(13+searchTint.Value.y*8.f),
        (int)(20+searchTint.Value.z*12.f), 0.965f*mAnim), rnd);
    mDl->AddRect(mn(r), mx(r), white((0.08f + 0.1f * mSearchFocus) * mAnim), rnd);
    float lw = (r.z - r.x - P(24)) * mSearchFocus;
    if (lw > 1) mDl->AddRectFilled({ (r.x + r.z) * 0.5f - lw * 0.5f, r.w - P(2) }, { (r.x + r.z) * 0.5f + lw * 0.5f, r.w - P(0.5f) }, withA(acc, mAnim), P(1));
    float scan = std::fmod(mTime * 0.24f, 1.f);
    float scanX = r.x + (r.z-r.x) * scan, scanW = P(28.f);
    mDl->PushClipRect(mn(r), mx(r), true);
    mDl->AddRectFilledMultiColor({scanX-scanW,r.y},{scanX+scanW,r.y+P(1.2f)},
        withA(acc,0),withA(acc,0.65f*mAnim),withA(acc,0.65f*mAnim),withA(acc,0));
    mDl->PopClipRect();

    float x = r.x + P(12);
    auto it = FontHelper::Fonts.find("tenacity_icons");
    if (it != FontHelper::Fonts.end() && it->second) {
        float sz = mTextPx * 0.85f; ImVec2 s = it->second->CalcTextSizeA(sz, FLT_MAX, 0, "s");
        mDl->AddText(it->second, sz, { x, cy - s.y * 0.5f }, white((0.35f + 0.4f * mSearchFocus) * mAnim), "s");
        x += s.x + P(10);
    }
    float ty = cy - mTextH * 0.47f;
    if (mSearch.empty() && !focused) text(mDl, { x, ty }, "Search modules...  (just start typing)", ImColor(1.f, 1.f, 1.f, 0.3f), 0.92f);
    else {
        text(mDl, { x, ty }, mSearch, ImColor(1.f, 1.f, 1.f, 0.92f), 0.95f);
        if (focused) caret(mDl, x + tw(mSearch, 0.95f) + P(2), ty, mTextH * 0.95f, 1.f);
    }
    if (isSearchActive()) {
        bool hot = in(mSearchClear, mMouse);
        float cx = (mSearchClear.x + mSearchClear.z) * 0.5f, s = P(4);
        mDl->AddLine({ cx - s, cy - s }, { cx + s, cy + s }, white((hot ? 0.9f : 0.4f) * mAnim), std::max(1.f, P(1.5f)));
        mDl->AddLine({ cx - s, cy + s }, { cx + s, cy - s }, white((hot ? 0.9f : 0.4f) * mAnim), std::max(1.f, P(1.5f)));
        std::string cnt = mResultCount == 0 ? "no matches" : std::to_string(mResultCount) + (mResultCount == 1 ? " match" : " matches");
        text(mDl, { mSearchClear.x - P(8) - tw(cnt, 0.72f), cy - mTextH * 0.36f }, cnt, mResultCount ? withA(acc, 0.8f) : ImColor(1.f, 0.4f, 0.4f, 0.8f), 0.72f);
    }
}

void ModernGui::updateRipples()
{
    for (auto& rp : mRipples) { rp.r += mDt * P(280); rp.a -= mDt * 2.2f; }
    mRipples.erase(std::remove_if(mRipples.begin(), mRipples.end(), [](const Ripple& r) { return r.a <= 0.f; }), mRipples.end());
}

void ModernGui::drawHints()
{
    if (!mHints || !mIsEnabled) return;
    float t = mTime - mOpenTime;
    float a = std::clamp(t * 2.f, 0.f, 1.f) * std::clamp((9.f - t) * 1.5f, 0.f, 1.f) * mAnim;
    if (a < 0.01f) return;
    std::string s = "LMB toggle   ·   RMB settings   ·   MMB bind   ·   click header to fold, drag to move   ·   ESC close";
    float w = tw(s, 0.75f), x = mCenter.x - w * 0.5f, y = mScreen.y - S(28);
    auto* fg = ImGui::GetForegroundDrawList();
    fg->AddRectFilled({ x - P(12), y - P(6) }, { x + w + P(12), y + mTextH * 0.75f + P(6) }, rgba(10, 10, 16, 0.75f * a), P(8));
    text(fg, { x, y }, s, ImColor(1.f, 1.f, 1.f, 0.55f), 0.75f, a);
}

void ModernGui::drawTooltip()
{
    if (mBind.active) mTooltip.pending = "Binding " + (mBind.mod ? mBind.mod->getName() : mBind.set ? mBind.set->mName : "") + " — press any key or MB4/MB5, ESC to unbind";
    if (mDragPanel >= 0 || mDrag.set || mPicker.open) mTooltip.pending.clear();
    if (mTooltip.pending != mTooltip.text) { mTooltip.text = mTooltip.pending; mTooltip.timer = 0; }
    mTooltip.timer += mDt;
    bool show = !mTooltip.text.empty() && (mTooltip.timer > 0.35f || mBind.active) && !ImGui::IsMouseDown(0);
    mTooltip.alpha = smoothTo(mTooltip.alpha, show ? 1.f : 0.f, 14.f, mDt);
    if (mTooltip.alpha < 0.01f || mTooltip.text.empty()) return;
    auto* fg = ImGui::GetForegroundDrawList();
    float wrap = S(280), pad = P(8), fs = mTextPx * 0.82f;
    ImVec2 sz = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, wrap, mTooltip.text.c_str());
    float x = mMouse.x + P(14), y = mMouse.y + P(18);
    if (x + sz.x + pad * 2 > mScreen.x) x = mMouse.x - sz.x - pad * 2 - P(6);
    if (y + sz.y + pad * 2 > mScreen.y) y = mMouse.y - sz.y - pad * 2 - P(6);
    ImVec4 r = { x, y, x + sz.x + pad * 2, y + sz.y + pad * 2 };
    float a = mTooltip.alpha * mAnim;
    ImColor acc = ColorUtils::getThemedColor(0);
    fg->AddShadowRect(mn(r), mx(r), black(0.5f * a), P(14), { 0, P(3) }, 0, P(6));
    fg->AddRectFilled(mn(r), mx(r), rgba(12, 12, 18, 0.97f * a), P(6));
    fg->AddRect(mn(r), mx(r), withA(acc, 0.35f * a), P(6));
    ImColor tc = { 1.f, 1.f, 1.f, 0.85f * a };
    fg->AddText(ImGui::GetFont(), fs, { r.x + pad, r.y + pad }, tc, mTooltip.text.c_str(), nullptr, wrap);
}

void ModernGui::endFrame()
{
    if (mText.active && mText.kind != TextEdit::Search && ImGui::IsMouseClicked(0) && !mTextClicked) finishText(true);
    if (mText.active && mText.kind == TextEdit::Search && ImGui::IsMouseClicked(0) && !mTextClicked) finishText(true);
    if (mBind.active && mIsEnabled) {
        if (ImGui::IsMouseClicked(3)) setBindKey(VK_XBUTTON1);
        else if (ImGui::IsMouseClicked(4)) setBindKey(VK_XBUTTON2);
    }
    updateRipples();
    mClickConsumed = false;
    ImGui::PopFont();
}

void ModernGui::render(float animation, float inScale, float blur, float midclickRounding,
                       float uiScale, bool ambient, bool hints, float effectIntensity,
                       bool panelGlow, bool cursorGlow, bool parallax, bool backdropGrid)
{
    beginFrame(animation, inScale, blur, midclickRounding, uiScale,
        effectIntensity, panelGlow, cursorGlow, parallax, backdropGrid);
    mHints = hints;
    if (animation >= 0.98f) mInScale = 1.f;

    initPanels();
    refreshFilter();

    for (size_t i = 0; i < mPanels.size(); i++)
        layoutPanel(i);

    {
        float W = S(kCatW) * 1.25f;
        mSearchRect  = tf({ mCenter.x - W * 0.5f, S(12), mCenter.x + W * 0.5f, S(12) + S(36) });
        mSearchClear = { mSearchRect.z - P(30), mSearchRect.y, mSearchRect.z, mSearchRect.w };
    }
    layoutPicker();
    resolveHover();

    if (mIsEnabled && mAnim > 0.001f) {
        inputPicker();
        inputSearch();
        for (auto it = mDrawOrder.rbegin(); it != mDrawOrder.rend(); ++it)
            inputPanel(*it);
        processDrag();
        layoutPicker();
    }

    drawBackground(ambient);
    drawSearch();
    for (size_t i : mDrawOrder)
        drawPanel(i);
    drawPicker();
    if (hints) drawHints();
    drawTooltip();
    drawCursorFx(); // always last + foreground draw list => above panels, tooltip and hints
    endFrame();
}
