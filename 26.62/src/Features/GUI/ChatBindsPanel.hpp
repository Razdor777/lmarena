#pragma once
//
// ChatBindsPanel.hpp — меню чат-биндов в ClickGui (кнопка сверху + карточка).
//
// Самостоятельный кусок интерфейса: его встраивают оба стиля ClickGui
// (Modern — справа от поиска, Combatant — по центру сверху). Хост на каждом
// кадре:
//   layout(rect кнопки, масштаб, прозрачность, акцент, можно ли нажимать);
//   hovered()  — курсор над кнопкой/карточкой: панели под ней не подсвечивать;
//   input()    — до своих кликов; consumedClick() — клик забран;
//   draw(dl)   — после своих панелей (карточка поверх);
//   onKey()    — из своего onKey до всего остального (ввод текста, захват
//                клавиши, ESC закрывает карточку);
//   close()    — ClickGui закрыли.
//
// В карточке: строка на бинд — текст (клик — редактировать, Enter —
// сохранить, Esc — отмена), клавиша (клик — нажми любую, Esc — без клавиши,
// MB4/MB5 тоже можно) и удаление; внизу «+ Add bind». Всё сохраняется сразу
// (см. ChatBinds).
//

#include <imgui.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <Features/ChatBinds/ChatBinds.hpp>

class ChatBindsPanel
{
public:
    void layout(const ImVec4& button, float scale, float alpha, ImColor accent, bool interactive);
    bool hovered() const { return mHovered; }
    void input();
    bool consumedClick() const { return mClickConsumed; }
    void draw(ImDrawList* dl);
    bool onKey(int vk, bool pressed);
    void close();
    bool isOpen() const { return mOpen; }
    bool isCapturing() const { return mEditIdx >= 0 || mBindIdx >= 0; }

private:
    struct RowRects { ImVec4 row, text, key, del; };

    // кадр
    ImVec4 mButton{}, mCard{}, mList{}, mAdd{}, mClose{};
    std::vector<RowRects> mRowRects;
    std::vector<ChatBind> mRows;
    float  mS = 1.f, mAlpha = 1.f, mFontPx = 16.f, mDt = 0.f, mTime = 0.f;
    ImColor mAccent{};
    ImVec2 mMouse{};
    bool   mInteractive = false, mHovered = false, mClickConsumed = false;

    // состояние
    bool   mOpen = false;
    float  mOpenAnim = 0.f, mBtnHover = 0.f, mScroll = 0.f, mMaxScroll = 0.f, mBlink = 0.f;
    int    mEditIdx = -1, mBindIdx = -1;
    std::string mEditBuf;
    int    mFlashIdx = -1;
    float  mFlashUntil = 0.f;
    std::string mFlashText;
    std::unordered_map<int, float> mRowHover;

    float P(float v) const { return v * mS; }
    float textW(const std::string& s, float px) const;
    void  text(ImDrawList* dl, ImVec2 p, const std::string& s, ImU32 col, float px) const;

    // Размеры и помощники — члены класса, а не свободные функции в .cpp.
    // Unity-сборка (CMAKE_UNITY_BUILD) склеивает соседние .cpp в один, и
    // одноимённые in/rgba/kRowH... из ModernDropdown/CombatantGui давали
    // «уже имеет текст реализации» / «неоднозначный символ». Внутри методов
    // члены класса находятся раньше любых имён из других файлов.
    static constexpr float kRowH = 38.f, kHeaderH = 44.f, kAddH = 38.f, kFootH = 28.f, kPad = 10.f, kW = 420.f;
    static constexpr int   kVisibleRows = 8;

    static bool        in(const ImVec4& r, ImVec2 p);
    static ImVec2      mn(const ImVec4& r);
    static ImVec2      mx(const ImVec4& r);
    static ImU32       rgba(int r, int g, int b, float a);
    static ImU32       white(float a);
    static ImU32       withA(const ImColor& c, float a);
    static float       smoothTo(float cur, float tgt, float speed, float dt);
    static std::string toUtf8(const wchar_t* w, int n);
    static void        popUtf8(std::string& s);
    static std::string charFromKey(int vk);
    static std::string clipboardText();
    static int         clickGuiKey();

    void beginEdit(int idx);
    void commitEdit();
    void cancelEdit();
    void beginBind(int idx);
    void setOpen(bool open);
};
