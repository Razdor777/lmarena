#pragma once
#include <vector>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>

class Module;

// Ripple structure for click effect
struct GuiRipple {
    float x, y;       // position relative to module rect
    float radius;     // current radius
    float alpha;      // current alpha
    float maxRadius;  // target radius
};

class ModernGui
{
public:
    struct CategoryPosition {
        float x = 0.f, y = 0.f;
        bool isDragging    = false;
        bool isExtended    = true;
        bool wasExtended   = false;
        float yOffset      = 0.f;
        float scrollEase   = 0.f;
        float expandAnim   = 0.f;  // 0→1 panel open animation
    };

    // Layout constants
    const float catWidth  = 210.f;
    const float catHeight = 38.f;
    const float modHeight = 34.f;
    const float catGap    = 14.f;

    int lastDragged = -1;
    std::vector<CategoryPosition> catPositions;
    std::shared_ptr<Module> lastMod       = nullptr;
    BoolSetting*   lastBoolSetting        = nullptr;
    ColorSetting*  lastColorSetting       = nullptr;
    Setting*       lastDraggedSetting     = nullptr;

    bool isBinding           = false;
    bool isBoolSettingBinding = false;
    bool displayColorPicker  = false;
    bool resetPosition       = false;
    uint64_t lastReset       = 0;

    // Search
    char  mSearchBuffer[128] = { 0 };
    bool  mSearching         = false;

    // Cached per-frame data
    ImVec2 mMousePos    = ImVec2(0, 0);
    ImVec2 mScreenSize  = ImVec2(0, 0);
    float  mDeltaTime   = 0.f;
    float  mAnimation   = 0.f;
    float  mInScale     = 0.f;
    float  mBlur        = 0.f;
    float  mMidclickRounding = 1.f;
    bool   mIsEnabled   = false;
    bool   mIsPressingShift = false;
    float  mTextSize    = 1.f;
    float  mTextHeight  = 0.f;
    std::string mTooltip;
    bool   mLowercase   = false;

    // Colours matching HTML glass-card style
    ImColor darkBlack   = ImColor(14,  14,  22);   
    ImColor darkBlack2  = ImColor(8,   8,   14);   
    ImColor modBg       = ImColor(30,  30,  40);   
    ImColor grayColor   = ImColor(255, 255, 255, 9); 

    // Particles
    struct Particle {
        float x, y;
        float speed;
        float vx, vy; // Направление полета
        float size;
        float opacity;
        float phase;   
    };
    std::vector<Particle> mParticles;
    bool mParticlesInited = false;

    // Utility
    ImVec4 scaleToPoint(const ImVec4& _this, const ImVec4& point, float amount);
    bool   isMouseOver(const ImVec4& rect);
    ImVec4 getCenter(ImVec4& vec);

    // Main entry
    void render(float animation, float inScale, int& scrollDirection,
                float blur, float midclickRounding, bool isPressingShift);
    void onWindowResizeEvent(class WindowResizeEvent& event);

private:
    // Frame setup
    void cacheFrameData(float animation, float inScale, float blur,
                        float midclickRounding, bool isPressingShift, bool isEnabled);
    void initCategoryPositions();
    std::vector<std::shared_ptr<Module>> getFilteredModules(size_t catIndex);
    bool isSearchActive() const;

    // Background & effects
    void renderBackground();
    void renderParticles();
    void initParticles();

    // Search bar
    void renderSearchBar();

    // Color picker
    void renderColorPickerWindow();

    // Categories
    void renderCategories(int& scrollDirection);
    void renderCategory(size_t index, int& scrollDirection);
    void renderCategoryHeader(size_t index, const ImVec4& catRect);
    void renderCategoryModules(size_t catIndex, const ImVec4& catRect);

    // Module row
    void renderModule(size_t catIndex, const ImVec4& catRect, const ImVec4& modRect,
                      const std::shared_ptr<Module>& mod,
                      bool isLast, float& moduleY, bool& moduleToggled);

    // Ripple helpers
    void updateRipples(float dt);

    // Settings dispatcher
    void renderModuleSettings(size_t catIndex, const std::shared_ptr<Module>& mod,
                              const ImVec4& modRect, const ImVec4& catRect,
                              float& moduleY, bool isLast);

    // Individual setting renderers
    void renderBoolSetting  (size_t catIndex, BoolSetting*   s, const ImVec4& modRect,
                             const ImVec4& catRect, float& moduleY,
                             float radius, const ImColor& accent, float setPadding, float cAnim);
    void renderEnumSetting  (size_t catIndex, EnumSetting*   s, const ImVec4& modRect,
                             const ImVec4& catRect, float& moduleY,
                             float radius, const ImColor& accent, float setPadding, float cAnim);
    void renderNumberSetting(size_t catIndex, NumberSetting* s, const ImVec4& modRect,
                             const ImVec4& catRect, float& moduleY,
                             float radius, const ImColor& accent, float setPadding, float cAnim);
    void renderColorSetting (size_t catIndex, ColorSetting*  s, const ImVec4& modRect,
                             const ImVec4& catRect, float& moduleY,
                             float radius, const ImColor& accent, float setPadding, float cAnim);

    // Toggle switch helper
    void drawToggleSwitch(ImVec2 center, float boolScale,
                          const ImColor& accent, float alpha);

    // Tooltip
    void renderTooltip();

    // Input
    void handleCategoryDragging(size_t index, const ImVec4& catRect);
    void handleBinding();
    void handleBoolBinding();

    // Global ripple list
    std::vector<GuiRipple> mRipples;
};