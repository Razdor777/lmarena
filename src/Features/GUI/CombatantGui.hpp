#pragma once
//
// CombatantGui.hpp — второй стиль ClickGui, по мотивам combatant-client.
//
// Что перенесено по виду (ModulesMenuScreen / ModulesMenuStyle в combatant):
//   - одна стеклянная панель на категорию, заголовок с количеством модулей;
//   - строки модулей с цветами ON/OFF (зелёный / красный из combatant);
//   - правый клик по модулю открывает его настройки внутри той же панели;
//   - панели перетаскиваются за заголовок, колесо прокручивает список.
//
// Ввод и биндинг — той же моделью, что у ModernGui: состояние мыши из ImGui,
// клавиши приходят через KeyEvent (см. ClickGui::onKeyEvent).
//

#include <imgui.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Module;
class Setting;
class NumberSetting;
class WindowResizeEvent;

class CombatantGui
{
public:
    void render(float animation, float inScale, float uiScale);
    bool onKey(int vk, bool pressed); // true => клавиша съедена (захват бинда)
    void onOpen();
    void onClose();
    void onWindowResizeEvent(WindowResizeEvent& event);

private:
    enum class RowKind { Back, Bind, Module, Setting };

    struct Row
    {
        RowKind kind;
        Module* mod;
        class Setting* set;
        float h;
    };

    struct PanelState
    {
        float x = 0.f, y = 0.f;            // позиция в layout-координатах
        float scroll = 0.f, smoothScroll = 0.f, maxScroll = 0.f;
        float swap = 0.f;                  // 0 — список модулей, 1 — настройки
        Module* selected = nullptr;        // модуль, чьи настройки открыты
        bool dragging = false;
        float dragDX = 0.f, dragDY = 0.f;
    };

    std::vector<PanelState> mPanels;
    std::vector<size_t>     mDrawOrder;
    ImVec2 mLayoutScreen{};
    bool   mResizePending = false;
    bool   mWaitRelease   = true;          // не кликать, пока открывающий клик не отпущен

    float  mAnim = 0.f, mInScale = 1.f, mScale = 1.f, mDt = 0.f, mWheel = 0.f;
    ImVec2 mScreen{}, mCenter{}, mMouse{}, mLayoutMouse{};
    int    mHoverPanel = -1;

    Module*        mBindModule = nullptr;  // модуль, для которого ждём клавишу
    NumberSetting* mDragNumber = nullptr;  // слайдер, который сейчас тянем
    ImVec4         mDragTrack{};

    std::unordered_map<uintptr_t, float> mHover;

    float  S(float v) const { return v * mScale; }
    ImVec2 T(ImVec2 p) const;              // layout -> экран (масштаб открытия)
    ImVec4 T(const ImVec4& r) const;
    ImVec2 itf(ImVec2 p) const;            // экран -> layout
    ImVec4 numberTrack(const ImVec4& rr) const;

    std::vector<Row> buildRows(size_t cat, const PanelState& p) const;
    void initPanels(size_t n);
    void bringToFront(size_t i);
    void handleRowClick(size_t i, const Row& row, const ImVec4& rr,
                        bool left, bool right, bool middle);
    void drawPanel(ImDrawList* dl, size_t i, const ImVec4& r, const std::vector<Row>& rows);
    void drawRow(ImDrawList* dl, const Row& row, const ImVec4& rr, bool selected, bool canHover);
};
