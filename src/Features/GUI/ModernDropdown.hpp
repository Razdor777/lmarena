#pragma once
//
// ModernDropdown.hpp — Modern ClickGui
// layout -> input -> draw, single transform, self-contained UI state
//
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>

class Module;
class WindowResizeEvent;

class ModernGui
{
public:
    void render(float animation, float inScale, float blur, float midclickRounding,
                float uiScale, bool ambient, bool hints, float effectIntensity,
                bool panelGlow, bool cursorGlow, bool parallax, bool backdropGrid);

    bool onKey(int vk, bool pressed);          // true => consumed
    void onOpen();
    void onClose();
    void onWindowResizeEvent(WindowResizeEvent& event);
    void resetLayout() { mResetPending = true; }

    bool isBinding() const { return mBind.active; }
    bool isTyping()  const { return mText.active; }

    nlohmann::json saveLayout() const;
    void loadLayout(const nlohmann::json& j);

private:
    static constexpr float kCatW = 220.f, kHeaderH = 38.f, kRowH = 34.f, kNumRowH = 46.f,
                           kGap = 14.f, kRound = 10.f, kFontPx = 18.f;

    struct Row {
        enum Kind { Mod, Bool, Number, Enum, EnumVal, Color, String } kind = Mod;
        float y = 0, h = 0, alpha = 1;
        std::shared_ptr<Module> mod;
        Setting* set = nullptr;
        int idx = 0, modIdx = 0;
        bool last = false;
    };
    struct Panel {
        float x = 0, y = 0;
        bool  extended = true;
        float expand = 0, scroll = 0, scrollAnim = 0, scrollbarAlpha = 0, hover = 0, focus = 0;
        float contentH = 0, visH = 0, maxScroll = 0;
        bool  roundBottom = true;
        ImVec4 header{}, content{}, window{};
        std::vector<Row> rows;
        std::vector<std::shared_ptr<Module>> mods;
        bool pressed = false, dragging = false;
        ImVec2 pressPos{}, dragOff{};
    };
    struct ModuleUi  { float open = 0, enable = 0, hover = 0, pulse = 0; bool expanded = false; int searchHit = 0; };
    struct SettingUi { float hover = 0, slide = 0, knob = 1, enumOpen = 0, toggle = 0; bool enumExpanded = false; };
    struct Ripple    { const void* owner; float ox, oy, r, a; };
    struct Bind      { bool active = false; std::shared_ptr<Module> mod; BoolSetting* set = nullptr; };
    struct TextEdit  { enum Kind { Search, String, Number, Hex } kind = Search; bool active = false; Setting* set = nullptr; std::string buf; float blink = 0; };
    struct Picker    { bool open = false; ColorSetting* set = nullptr; float h = 0, s = 0, v = 0, a = 1, anim = 0; int drag = 0;
                       ImVec4 anchor{}, rect{}, sv{}, hue{}, alp{}, hex{}; float presetY = 0; };
    struct Drag      { NumberSetting* set = nullptr; bool fine = false, mid = false; float anchorX = 0, anchorVal = 0; ImVec4 track{}; };
    struct Tooltip   { std::string text, pending; float timer = 0, alpha = 0; };
    struct Orb       { float x, y, vx, vy, r, phase; int hue; };
    struct CursorTrail { ImVec2 pos{}; float life = 0, radius = 0, hue = 0; };

    // frame
    float  mAnim = 0, mInScale = 1, mBlur = 0, mMidRound = 1, mScale = 1;
    float  mTextPx = 18, mTextH = 0, mDt = 0, mTime = 0, mWheel = 0, mOpenTime = -100;
    float  mFxIntensity = 1.f;
    bool   mIsEnabled = false, mLowercase = false, mHints = true;
    bool   mPanelGlow = true, mCursorGlow = true, mParallax = true, mBackdropGrid = true;
    bool   mClickConsumed = false, mWheelConsumed = false, mTextClicked = false;
    bool   mFilterDirty = true, mResetPending = false, mResizePending = false;
    ImVec2 mScreen{}, mCenter{}, mMouse{}, mLayoutScreen{};
    ImDrawList* mDl = nullptr;

    std::vector<Panel>  mPanels;
    std::vector<size_t> mDrawOrder;
    int mHoveredCat = -1, mDragPanel = -1;

    std::unordered_map<const void*, ModuleUi>  mModUi;
    std::unordered_map<const void*, SettingUi> mSetUi;

    std::string mFilterKey, mSearch;
    int    mResultCount = 0;
    ImVec4 mSearchRect{}, mSearchClear{};
    float  mSearchFocus = 0;

    TextEdit mText;
    Bind     mBind;
    Picker   mPicker;
    Drag     mDrag;
    Tooltip  mTooltip;
    std::vector<Ripple> mRipples;
    std::vector<Orb>    mOrbs;
    std::vector<CursorTrail> mCursorTrail;
    ImVec2 mLastMouse{};
    nlohmann::json      mPendingLayout;

    // helpers
    float  S(float v) const { return v * mScale; }              // layout px (pre-transform)
    float  P(float v) const { return v * mScale * mInScale; }   // screen px (post-transform)
    ImVec2 tf(ImVec2 p) const;
    ImVec4 tf(const ImVec4& r) const;
    ImVec2 itf(ImVec2 p) const;
    float  tw(const std::string& s, float scale = 1.f) const;
    void   text(ImDrawList* dl, ImVec2 p, const std::string& s, ImColor c, float scale = 1.f, float alpha = 1.f);
    void   chevron(ImDrawList* dl, ImVec2 c, float s, float t, ImU32 col, float thick = 1.8f);
    void   toggleSwitch(ImVec2 c, float t, const ImColor& acc, float a);
    void   caret(ImDrawList* dl, float x, float y, float h, float a);
    float  keyBadge(float rightX, float cy, const std::string& label, const ImColor& acc, float a, bool hot);
    void   checker(ImDrawList* dl, const ImVec4& r, float cell, float a, float rnd);
    ImVec4 rowRect(const Panel& p, const Row& r) const;
    ImVec4 trackOf(const ImVec4& rs) const;
    ImColor accent(size_t cat, int modIdx) const;
    bool   canInteract(size_t cat, const ImVec4& rs) const;
    bool   hovering(size_t cat, const ImVec4& rs) const;
    bool   panelVisible(size_t i) const { return !(isSearchActive() && mPanels[i].mods.empty()); }
    bool   isSearchActive() const { return !mSearch.empty(); }
    bool   panelExtended(const Panel& p) const { return p.extended || isSearchActive(); }
    ModuleUi&  modUi(const std::shared_ptr<Module>& m) { return mModUi[m.get()]; }
    SettingUi& setUi(Setting* s) { return mSetUi[s]; }
    void   bringToFront(size_t i);
    void   play(const char* s, float pitch = 1.f);
    void   setBindKey(int vk);
    std::string settingName(Setting* s) const;

    // passes
    void beginFrame(float animation, float inScale, float blur, float midclick, float uiScale,
                    float effectIntensity, bool panelGlow, bool cursorGlow,
                    bool parallax, bool backdropGrid);
    void initPanels();
    void applyPendingLayout();
    void refreshFilter();
    void layoutPanel(size_t i);
    void resolveHover();

    void inputPanel(size_t i);
    void inputRow(size_t i, const Row& r, const ImVec4& rs, bool hov);
    void processDrag();
    void inputSearch();

    void drawBackground(bool ambient);
    void drawOrbs();
    void drawCursorFx();
    void drawPanel(size_t i);
    void drawHeader(size_t i);
    void drawScrollbar(Panel& p);
    void drawRow(size_t i, const Row& r, const ImVec4& rs, bool hov);
    void settingBg(const Row& r, const ImVec4& rs, float hover);
    void drawModRow   (size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawBoolRow  (size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawNumberRow(size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawEnumRow  (size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawEnumValRow(size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawColorRow (size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawStringRow(size_t i, const Row& r, const ImVec4& rs, bool hov);
    void drawSearch();

    void openPicker(ColorSetting* cs, const ImVec4& anchor);
    void applyPicker();
    void layoutPicker();
    void inputPicker();
    void drawPicker();

    void beginText(TextEdit::Kind k, Setting* s, const std::string& initial);
    void finishText(bool commit);
    void insertText(const std::string& utf8);

    void updateRipples();
    void drawHints();
    void drawTooltip();
    void endFrame();
};
