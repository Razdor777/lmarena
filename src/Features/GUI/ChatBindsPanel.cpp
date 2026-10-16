//
// ChatBindsPanel.cpp — см. ChatBindsPanel.hpp
//
#include "ChatBindsPanel.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>

#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

// Помощники — static-члены ChatBindsPanel (см. .hpp: unity-сборка).

bool   ChatBindsPanel::in(const ImVec4& r, ImVec2 p) { return p.x >= r.x && p.y >= r.y && p.x < r.z && p.y < r.w; }
ImVec2 ChatBindsPanel::mn(const ImVec4& r) { return { r.x, r.y }; }
ImVec2 ChatBindsPanel::mx(const ImVec4& r) { return { r.z, r.w }; }
ImU32  ChatBindsPanel::rgba(int r, int g, int b, float a) { return IM_COL32(r, g, b, (int)(std::clamp(a, 0.f, 1.f) * 255.f)); }
ImU32  ChatBindsPanel::white(float a) { return rgba(255, 255, 255, a); }
ImU32  ChatBindsPanel::withA(const ImColor& c, float a) { return ImColor(c.Value.x, c.Value.y, c.Value.z, std::clamp(a, 0.f, 1.f)); }

float ChatBindsPanel::smoothTo(float cur, float tgt, float speed, float dt)
{
    float v = tgt + (cur - tgt) * std::exp(-speed * dt);
    return std::fabs(v - tgt) < 0.0005f ? tgt : v;
}

std::string ChatBindsPanel::toUtf8(const wchar_t* w, int n)
{
    if (n <= 0) return {};
    char buf[512];
    int len = WideCharToMultiByte(CP_UTF8, 0, w, n, buf, sizeof(buf), nullptr, nullptr);
    return len > 0 ? std::string(buf, len) : std::string();
}

void ChatBindsPanel::popUtf8(std::string& s)
{
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && (s[i] & 0xC0) == 0x80) --i;
    s.erase(i);
}

// Символ с учётом раскладки и Shift/Caps (как в поиске ModernGui).
std::string ChatBindsPanel::charFromKey(int vk)
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

std::string ChatBindsPanel::clipboardText()
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

int ChatBindsPanel::clickGuiKey()
{
    if (!gFeatureManager || !gFeatureManager->mModuleManager) return VK_TAB;
    auto cg = gFeatureManager->mModuleManager->getModule<ClickGui>();
    return cg ? cg->mKey : VK_TAB;
}

float ChatBindsPanel::textW(const std::string& s, float px) const
{
    return ImGui::GetFont()->CalcTextSizeA(px, FLT_MAX, 0, s.c_str()).x;
}

void ChatBindsPanel::text(ImDrawList* dl, ImVec2 p, const std::string& s, ImU32 col, float px) const
{
    if (s.empty()) return;
    const float a = ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    dl->AddText(ImGui::GetFont(), px, { std::floor(p.x) + 1.f, std::floor(p.y) + 1.f }, rgba(3, 4, 9, a * 0.55f), s.c_str());
    dl->AddText(ImGui::GetFont(), px, { std::floor(p.x), std::floor(p.y) }, col, s.c_str());
}

// ─────────────────────────────────────────────────────────────── state

void ChatBindsPanel::setOpen(bool open)
{
    if (!open) { commitEdit(); mBindIdx = -1; }
    mOpen = open;
}

void ChatBindsPanel::beginEdit(int idx)
{
    if (idx < 0 || idx >= (int)mRows.size()) return;
    mBindIdx = -1;
    mEditIdx = idx;
    mEditBuf = mRows[idx].text;
    mBlink = 0.f;
}

void ChatBindsPanel::commitEdit()
{
    if (mEditIdx < 0) return;
    ChatBinds::setText((size_t)mEditIdx, mEditBuf);
    mEditIdx = -1;
    mEditBuf.clear();
}

void ChatBindsPanel::cancelEdit()
{
    mEditIdx = -1;
    mEditBuf.clear();
}

void ChatBindsPanel::beginBind(int idx)
{
    commitEdit();
    mBindIdx = (mBindIdx == idx) ? -1 : idx; // второй клик — отмена
}

void ChatBindsPanel::close()
{
    commitEdit();
    mBindIdx = -1;
    mOpen = false;
    mOpenAnim = 0.f;
}

// ─────────────────────────────────────────────────────────────── layout

void ChatBindsPanel::layout(const ImVec4& button, float scale, float alpha, ImColor accent, bool interactive)
{
    ImGuiIO& io = ImGui::GetIO();
    mDt = std::min(io.DeltaTime, 0.1f);
    mTime += mDt;
    mBlink += mDt;
    mButton = button;
    mS = std::max(scale, 0.05f);
    mAlpha = std::clamp(alpha, 0.f, 1.f);
    mAccent = accent;
    mInteractive = interactive;
    mMouse = io.MousePos;
    mFontPx = 17.f * mS;
    mClickConsumed = false;
    mRows = ChatBinds::list();
    if (mEditIdx >= (int)mRows.size()) cancelEdit();
    if (mBindIdx >= (int)mRows.size()) mBindIdx = -1;

    mOpenAnim = smoothTo(mOpenAnim, mOpen ? 1.f : 0.f, 16.f, mDt);
    mBtnHover = smoothTo(mBtnHover, (interactive && in(mButton, mMouse)) ? 1.f : 0.f, 14.f, mDt);

    const ImVec2 screen = ImRenderUtils::getScreenSize();
    const float W = P(kW);
    const float cx = (mButton.x + mButton.z) * 0.5f;
    const float x = std::clamp(cx - W * 0.5f, P(8), std::max(P(8), screen.x - W - P(8)));
    const float y = mButton.w + P(8);
    const int n = (int)mRows.size();
    const float listH = n == 0 ? P(46) : P(kRowH) * (float)std::min(n, kVisibleRows);
    mList = { x + P(kPad), y + P(kHeaderH), x + W - P(kPad), y + P(kHeaderH) + listH };
    mAdd = { mList.x, mList.w + P(6), mList.z, mList.w + P(6) + P(kAddH) - P(6) };
    mCard = { x, y, x + W, mAdd.w + P(kFootH) + P(4) };
    mClose = { mCard.z - P(36), y + P(9), mCard.z - P(10), y + P(35) };

    mMaxScroll = std::max(0.f, P(kRowH) * (float)n - listH);
    mScroll = std::clamp(mScroll, 0.f, mMaxScroll);

    const float keyW = P(84), delW = P(30);
    mRowRects.resize(n);
    for (int i = 0; i < n; i++)
    {
        const float ry = mList.y + P(kRowH) * (float)i - mScroll;
        RowRects& r = mRowRects[i];
        r.row  = { mList.x, ry, mList.z, ry + P(kRowH) - P(5) };
        r.del  = { r.row.z - delW, r.row.y, r.row.z, r.row.w };
        r.key  = { r.del.x - P(6) - keyW, r.row.y + P(4), r.del.x - P(6), r.row.w - P(4) };
        r.text = { r.row.x, r.row.y, r.key.x - P(8), r.row.w };
    }

    mHovered = interactive && (in(mButton, mMouse) || (mOpen && in(mCard, mMouse)));
}

// ─────────────────────────────────────────────────────────────── input

void ChatBindsPanel::input()
{
    if (!mInteractive) return;

    // Боковые кнопки мыши тоже можно назначить.
    if (mBindIdx >= 0)
    {
        int vk = ImGui::IsMouseClicked(3) ? VK_XBUTTON1 : ImGui::IsMouseClicked(4) ? VK_XBUTTON2 : 0;
        if (vk)
        {
            ChatBinds::setKey((size_t)mBindIdx, vk);
            mBindIdx = -1;
            mClickConsumed = true;
            return;
        }
    }

    const float wheel = ImGui::GetIO().MouseWheel;
    if (mOpen && wheel != 0.f && in(mCard, mMouse) && mMaxScroll > 0.f)
        mScroll = std::clamp(mScroll - std::clamp(wheel, -1.f, 1.f) * P(kRowH), 0.f, mMaxScroll);

    if (!ImGui::IsMouseClicked(0) && !ImGui::IsMouseClicked(1)) return;
    const bool left = ImGui::IsMouseClicked(0);

    if (in(mButton, mMouse))
    {
        if (left) setOpen(!mOpen);
        mClickConsumed = true;
        return;
    }
    if (!mOpen) return;

    if (!in(mCard, mMouse))
    {
        // мимо карточки — закрываем (клик дальше достаётся ClickGui)
        setOpen(false);
        return;
    }
    mClickConsumed = true;
    if (!left) return;

    if (in(mClose, mMouse)) { setOpen(false); return; }

    if (in(mAdd, mMouse))
    {
        commitEdit();
        const int idx = ChatBinds::add();
        if (idx < 0)
        {
            mFlashIdx = -2;
            mFlashText = "max " + std::to_string(ChatBinds::kMaxBinds) + " binds";
            mFlashUntil = mTime + 1.6f;
            return;
        }
        mRows = ChatBinds::list();
        mEditIdx = idx;
        mEditBuf.clear();
        mBlink = 0.f;
        mScroll = 1e9f; // к новому, ниже — clamp в layout
        return;
    }

    if (in(mList, mMouse))
    {
        for (int i = 0; i < (int)mRowRects.size(); i++)
        {
            const RowRects& r = mRowRects[i];
            if (!in(r.row, mMouse)) continue;
            if (in(r.del, mMouse))
            {
                if (mEditIdx == i) cancelEdit();
                else commitEdit();
                if (mBindIdx == i) mBindIdx = -1;
                ChatBinds::remove((size_t)i);
                if (mEditIdx > i) mEditIdx--;
                if (mBindIdx > i) mBindIdx--;
                mRows = ChatBinds::list();
                return;
            }
            if (in(r.key, mMouse)) { beginBind(i); return; }
            if (in(r.text, mMouse))
            {
                if (mEditIdx != i) { commitEdit(); beginEdit(i); }
                return;
            }
            return;
        }
    }
    // пустое место в карточке
    commitEdit();
    mBindIdx = -1;
}

// ─────────────────────────────────────────────────────────────── keys

bool ChatBindsPanel::onKey(int vk, bool pressed)
{
    if (mBindIdx >= 0)
    {
        if (!pressed) return true;
        if (vk == VK_ESCAPE)
        {
            ChatBinds::setKey((size_t)mBindIdx, 0);
        }
        else if (vk == clickGuiKey())
        {
            // клавишу ClickGui забирать нельзя — ClickGui потом не открыть
            mFlashIdx = mBindIdx;
            mFlashText = "opens ClickGui";
            mFlashUntil = mTime + 1.4f;
            return true;
        }
        else
        {
            ChatBinds::setKey((size_t)mBindIdx, vk);
        }
        mBindIdx = -1;
        return true;
    }

    if (mEditIdx >= 0)
    {
        if (!pressed) return true;
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (vk)
        {
        case VK_ESCAPE: cancelEdit(); return true;
        case VK_RETURN: commitEdit(); return true;
        case VK_BACK:
            if (ctrl)
            {
                while (!mEditBuf.empty() && mEditBuf.back() == ' ') popUtf8(mEditBuf);
                while (!mEditBuf.empty() && mEditBuf.back() != ' ') popUtf8(mEditBuf);
            }
            else popUtf8(mEditBuf);
            mBlink = 0.f;
            return true;
        default: break;
        }
        std::string add;
        if (ctrl) { if (vk == 'V') add = clipboardText(); }
        else add = charFromKey(vk);
        if (!add.empty() && mEditBuf.size() + add.size() <= ChatBinds::kMaxText)
        {
            mEditBuf += add;
            mBlink = 0.f;
        }
        return true; // пока печатаем, ClickGui клавиш не видит (TAB не закроет его)
    }

    if (mOpen && pressed && vk == VK_ESCAPE)
    {
        setOpen(false);
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────── draw

void ChatBindsPanel::draw(ImDrawList* dl)
{
    if (!dl || mAlpha < 0.01f) return;
    const float A = mAlpha;
    const int n = (int)mRows.size();

    // ── кнопка ─────────────────────────────────────────────────────────
    {
        const ImVec4& b = mButton;
        const float rnd = P(10), cy = (b.y + b.w) * 0.5f;
        const float act = std::max(mBtnHover, mOpenAnim);
        dl->AddShadowRect(mn(b), mx(b), withA(mAccent, (0.06f + 0.14f * act) * A), P(20), { 0, P(3) }, 0, rnd);
        dl->AddShadowRect(mn(b), mx(b), rgba(0, 0, 0, 0.45f * A), P(16), { 0, P(4) }, 0, rnd);
        dl->AddRectFilled(mn(b), mx(b), rgba(13, 13, 20, 0.965f * A), rnd);
        dl->AddRect(mn(b), mx(b), act > 0.01f ? withA(mAccent, (0.25f + 0.5f * act) * A) : white(0.08f * A), rnd, 0, std::max(1.f, P(1.2f)));

        // значок: облачко с тремя точками
        const float ix = b.x + P(14), iw = P(18), ih = P(13), iy = cy - ih * 0.6f;
        const ImU32 ic = act > 0.01f ? withA(mAccent, (0.6f + 0.4f * act) * A) : white(0.55f * A);
        dl->AddRect({ ix, iy }, { ix + iw, iy + ih }, ic, P(4), 0, std::max(1.f, P(1.5f)));
        const ImVec2 t0 = { ix + P(4), iy + ih - P(0.5f) }, t1 = { ix + P(9), iy + ih - P(0.5f) }, t2 = { ix + P(3), iy + ih + P(4.5f) };
        dl->AddTriangleFilled(t0, t1, t2, ic);
        for (int k = 0; k < 3; k++)
            dl->AddCircleFilled({ ix + P(5) + P(4) * (float)k, iy + ih * 0.5f }, std::max(1.f, P(1.3f)), ic, 8);

        const float px = mFontPx * 0.92f;
        const std::string label = "Chat Binds";
        text(dl, { ix + iw + P(10), cy - px * 0.55f }, label, white((0.75f + 0.2f * act) * A), px);

        // счётчик
        const std::string cnt = std::to_string(n);
        const float cpx = mFontPx * 0.72f, cw = std::max(textW(cnt, cpx) + P(10), P(20));
        const ImVec4 cr = { b.z - P(10) - cw, cy - P(9), b.z - P(10), cy + P(9) };
        dl->AddRectFilled(mn(cr), mx(cr), n ? withA(mAccent, 0.22f * A) : white(0.06f * A), P(9));
        text(dl, { (cr.x + cr.z - textW(cnt, cpx)) * 0.5f, cy - cpx * 0.55f }, cnt, n ? withA(mAccent, A) : white(0.45f * A), cpx);
    }

    if (mOpenAnim < 0.01f) return;
    const float a = A * mOpenAnim;
    const ImVec4& c = mCard;
    const float rnd = P(12);

    // ── карточка ──────────────────────────────────────────────────────
    dl->AddShadowRect(mn(c), mx(c), rgba(0, 0, 0, 0.55f * a), P(26), { 0, P(6) }, 0, rnd);
    dl->AddShadowRect(mn(c), mx(c), withA(mAccent, 0.10f * a), P(24), { 0, 0 }, 0, rnd);
    dl->AddRectFilled(mn(c), mx(c), rgba(12, 12, 19, 0.975f * a), rnd);
    dl->AddRect(mn(c), mx(c), withA(mAccent, 0.32f * a), rnd, 0, std::max(1.f, P(1.2f)));
    dl->AddRectFilledMultiColor({ c.x + P(14), c.y + P(1) }, { c.z - P(14), c.y + P(2.5f) },
        withA(mAccent, 0.f), withA(mAccent, 0.8f * a), withA(mAccent, 0.8f * a), withA(mAccent, 0.f));

    // заголовок
    const float tpx = mFontPx * 1.02f;
    text(dl, { c.x + P(16), c.y + P(13) }, "Chat Binds", white(0.95f * a), tpx);
    const std::string sub = "key -> sends text";
    const float spx = mFontPx * 0.72f;
    text(dl, { c.x + P(16) + textW("Chat Binds", tpx) + P(12), c.y + P(13) + (tpx - spx) * 0.6f }, sub, white(0.38f * a), spx);
    {
        const bool hot = mInteractive && in(mClose, mMouse);
        const float ccx = (mClose.x + mClose.z) * 0.5f, ccy = (mClose.y + mClose.w) * 0.5f, s = P(5);
        if (hot) dl->AddRectFilled(mn(mClose), mx(mClose), white(0.07f * a), P(6));
        dl->AddLine({ ccx - s, ccy - s }, { ccx + s, ccy + s }, white((hot ? 0.95f : 0.45f) * a), std::max(1.f, P(1.6f)));
        dl->AddLine({ ccx - s, ccy + s }, { ccx + s, ccy - s }, white((hot ? 0.95f : 0.45f) * a), std::max(1.f, P(1.6f)));
    }
    dl->AddLine({ c.x + P(12), c.y + P(kHeaderH) - P(5) }, { c.z - P(12), c.y + P(kHeaderH) - P(5) }, white(0.06f * a));

    // строки
    const float rpx = mFontPx * 0.9f;
    dl->PushClipRect(mn(mList), mx(mList), true);
    if (n == 0)
    {
        const std::string e = "No binds yet - add one below";
        text(dl, { (mList.x + mList.z - textW(e, rpx)) * 0.5f, (mList.y + mList.w - rpx) * 0.5f }, e, white(0.4f * a), rpx);
    }
    for (int i = 0; i < n; i++)
    {
        const RowRects& r = mRowRects[i];
        if (r.row.w < mList.y || r.row.y > mList.w) continue;
        const bool rowHot = mInteractive && in(r.row, mMouse) && in(mList, mMouse);
        float& h = mRowHover[i];
        h = smoothTo(h, rowHot ? 1.f : 0.f, 14.f, mDt);
        const bool editing = mEditIdx == i, binding = mBindIdx == i;
        const float ry = (r.row.y + r.row.w) * 0.5f;

        dl->AddRectFilled(mn(r.row), mx(r.row), white((0.035f + 0.035f * h + (editing ? 0.03f : 0.f)) * a), P(8));
        if (editing) dl->AddRect(mn(r.row), mx(r.row), withA(mAccent, 0.45f * a), P(8));

        // текст
        {
            const ImVec4 tr = { r.text.x + P(12), r.text.y, r.text.z, r.text.w };
            dl->PushClipRect(mn(tr), mx(tr), true);
            const std::string& v = editing ? mEditBuf : mRows[i].text;
            const float w = textW(v, rpx), avail = tr.z - tr.x - P(4);
            const float tx = (editing && w > avail) ? tr.z - P(4) - w : tr.x; // правим — виден конец
            if (v.empty() && !editing)
                text(dl, { tx, ry - rpx * 0.55f }, "/spawn, .tp ~ 10 ~ or a message", white(0.28f * a), rpx);
            else
                text(dl, { tx, ry - rpx * 0.55f }, v, editing ? white(0.97f * a) : white(0.85f * a), rpx);
            if (editing && std::fmod(mBlink, 1.f) < 0.5f)
                dl->AddRectFilled({ tx + w + P(1), ry - rpx * 0.5f }, { tx + w + P(1) + std::max(1.f, P(1.5f)), ry + rpx * 0.5f }, white(0.9f * a));
            dl->PopClipRect();
        }

        // клавиша
        {
            const bool hot = mInteractive && in(r.key, mMouse);
            const bool flash = mFlashIdx == i && mTime < mFlashUntil;
            const int key = mRows[i].key;
            std::string kl = binding ? "press a key" : ChatBinds::keyName(key);
            if (flash) kl = mFlashText;
            const float pulse = binding ? 0.5f + 0.5f * std::sin(mTime * 7.f) : 0.f;
            ImU32 bg = binding ? withA(mAccent, (0.18f + 0.14f * pulse) * a)
                     : flash  ? rgba(255, 80, 80, 0.22f * a)
                     : key    ? withA(mAccent, (0.14f + 0.08f * (hot ? 1.f : 0.f)) * a)
                              : white((0.05f + 0.04f * (hot ? 1.f : 0.f)) * a);
            ImU32 bd = binding ? withA(mAccent, 0.85f * a) : flash ? rgba(255, 90, 90, 0.8f * a)
                     : key ? withA(mAccent, 0.45f * a) : white(0.12f * a);
            dl->AddRectFilled(mn(r.key), mx(r.key), bg, P(6));
            dl->AddRect(mn(r.key), mx(r.key), bd, P(6));
            const float kpx = mFontPx * ((binding || flash) ? 0.66f : 0.8f);
            const ImU32 kc = flash ? rgba(255, 140, 140, a) : (key || binding) ? withA(mAccent, a) : white(0.4f * a);
            text(dl, { (r.key.x + r.key.z - textW(kl, kpx)) * 0.5f, ry - kpx * 0.55f }, kl, kc, kpx);
        }

        // удалить
        {
            const bool hot = mInteractive && in(r.del, mMouse);
            const float dcx = (r.del.x + r.del.z) * 0.5f, s = P(4.5f);
            if (hot) dl->AddRectFilled(mn(r.del), mx(r.del), rgba(255, 70, 70, 0.18f * a), P(6));
            const ImU32 dc = hot ? rgba(255, 110, 110, a) : white(0.35f * a * (0.4f + 0.6f * h));
            dl->AddLine({ dcx - s, ry - s }, { dcx + s, ry + s }, dc, std::max(1.f, P(1.6f)));
            dl->AddLine({ dcx - s, ry + s }, { dcx + s, ry - s }, dc, std::max(1.f, P(1.6f)));
        }
    }
    dl->PopClipRect();

    // полоса прокрутки
    if (mMaxScroll > 0.f)
    {
        const float trackH = mList.w - mList.y, total = trackH + mMaxScroll;
        const float barH = std::max(P(18), trackH * trackH / total);
        const float by = mList.y + (trackH - barH) * (mScroll / mMaxScroll);
        dl->AddRectFilled({ c.z - P(6), by }, { c.z - P(3), by + barH }, withA(mAccent, 0.5f * a), P(2));
    }

    // добавить
    {
        const bool hot = mInteractive && in(mAdd, mMouse);
        const bool full = n >= (int)ChatBinds::kMaxBinds;
        dl->AddRectFilled(mn(mAdd), mx(mAdd), hot && !full ? withA(mAccent, 0.16f * a) : white(0.03f * a), P(8));
        dl->AddRect(mn(mAdd), mx(mAdd), withA(mAccent, (hot ? 0.6f : 0.28f) * a), P(8));
        const bool flash = mFlashIdx == -2 && mTime < mFlashUntil;
        const std::string s = flash ? mFlashText : "+  Add bind";
        const float apx = mFontPx * 0.9f;
        text(dl, { (mAdd.x + mAdd.z - textW(s, apx)) * 0.5f, (mAdd.y + mAdd.w) * 0.5f - apx * 0.55f }, s,
             flash ? rgba(255, 140, 140, a) : withA(mAccent, (hot ? 1.f : 0.85f) * a), apx);
    }

    // подсказка
    {
        const std::string hint = mBindIdx >= 0 ? "press any key or MB4/MB5   |   Esc = no key"
                               : mEditIdx >= 0 ? "Enter = save   |   Esc = cancel   |   Ctrl+V = paste"
                                               : "/cmd = server command   |   .cmd = client   |   else chat";
        const float hpx = mFontPx * 0.7f;
        text(dl, { (c.x + c.z - textW(hint, hpx)) * 0.5f, mAdd.w + P(8) }, hint, white(0.36f * a), hpx);
    }
}
