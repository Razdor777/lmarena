#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>
#include <Features/Modules/Module.hpp>
#include <Utils/MiscUtils/EasingUtil.hpp>

class Module;

class ModernGui
{
public:
    struct CategoryPosition
    {
        float x = 0.f, y = 0.f;
        bool isDragging = false;
        bool isExtended = true;
        bool wasExtended = false;
        float yOffset = 0.f;
        float scrollEase = 0.f;
        glm::vec2 dragVelocity = glm::vec2(0.f, 0.f);
        glm::vec2 dragAcceleration = glm::vec2(0.f, 0.f);
        float extendAnim = 1.f; // smooth spring expand/collapse animation multiplier
        bool isDraggingScrollbar = false;
    };

    // Layout constants
    static constexpr float catWidth = 200.f;
    static constexpr float catHeight = 32.f;
    static constexpr float catGap = 30.f;
    static constexpr float modHeight = 34.f;

    int lastDragged = -1;
    std::vector<CategoryPosition> catPositions;
    std::shared_ptr<Module> lastMod = nullptr;
    bool isBinding = false;
    bool isBoolSettingBinding = false;
    BoolSetting* lastBoolSetting = nullptr;
    ColorSetting* lastColorSetting = nullptr;
    Setting* mLastDraggedSetting = nullptr;
    bool displayColorPicker = false;
    bool resetPosition = false;
    uint64_t lastReset = 0;

    // Search fields to guarantee 100% compilation compatibility
    bool mSearchFocused = false;
    char searchBuf[64] = {0};
    std::string mSearchQuery;

    // Animation / state trackers
    float mOpenProgress = 0.f;
    float mGlobalTime = 0.f;
    float mColorPickerAnim = 0.f;
    float mTooltipAlpha = 0.f;
    std::string mTooltipText;
    ImVec2 mTooltipPos;

    // Premium Colors
    ImColor textColor = ImColor(255, 255, 255);
    ImColor darkBlack = ImColor(16, 16, 20, 240);
    ImColor mainColor = ImColor(22, 22, 28, 220);
    ImColor grayColor = ImColor(40, 40, 48, 120);

    // Helpers
    ImVec4 scaleToPoint(const ImVec4& _this, const ImVec4& point, float amount);
    bool isMouseOver(const ImVec4& rect);
    ImVec4 getCenter(ImVec4& vec);
    bool isMouseOverGuiElement();

    // Interactive Particle system
    struct Particle {
        glm::vec2 pos;
        glm::vec2 vel;
        float size;
        float alpha;
        float fade;
    };
    std::vector<Particle> mParticles;

    // Core methods called by ClickGui
    void render(float animation, float inScale, int& scrollDirection,
                float blur, float midclickRounding, bool isPressingShift);
    void onWindowResizeEvent(class WindowResizeEvent& event);

private:
    void renderColorPicker(ImVec2 screen, float animation, float deltaTime);
    void renderTooltip(float animation, float inScale, float deltaTime);
    void renderBindings(float animation, float deltaTime);
    void updateAndDrawParticles(ImVec2 screen, float animation, int themeIdx, float deltaTime);
    void renderDashboard(ImVec2 screen, float animation, float inScale, int themeIdx, float deltaTime);
    void renderSearchBar(ImVec2 screen, float animation, int themeIdx, float deltaTime);
    ImColor getGlassColor(float alpha, bool isHeader, bool isBody, bool isCard);
    ImColor getAccentColor(float offset, float alpha, int themeIdx);
    std::string getCategoryIcon(int idx);
};