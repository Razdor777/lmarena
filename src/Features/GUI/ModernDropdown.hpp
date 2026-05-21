#pragma once
#include <vector>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Setting.hpp>

class Module;

class ModernGui
{
public:
    struct CategoryPosition
    {
        float x = 0.f, y = 0.f;
        bool isDragging = false, isExtended = true, wasExtended = false;
        float yOffset = 0;
        float scrollEase = 0;
        glm::vec2 dragVelocity = glm::vec2(0, 0);
        glm::vec2 dragAcceleration = glm::vec2(0, 0);
    };

    const float catWidth = 200.f;
    const float catHeight = 30.f;
    const float modHeight = 30.f;
    const float catGap = 40.f;

    int lastDragged = -1;
    std::vector<CategoryPosition> catPositions;
    std::shared_ptr<Module> lastMod = nullptr;
    BoolSetting* lastBoolSetting = nullptr;
    ColorSetting* lastColorSetting = nullptr;
    Setting* lastDraggedSetting = nullptr;

    bool isBinding = false;
    bool isBoolSettingBinding = false;
    bool displayColorPicker = false;
    bool resetPosition = false;
    uint64_t lastReset = 0;

    // Search
    char mSearchBuffer[128] = { 0 };
    bool mSearching = false;

    // Cached per-frame data
    ImVec2 mMousePos = ImVec2(0, 0);
    ImVec2 mScreenSize = ImVec2(0, 0);
    float mDeltaTime = 0.f;
    float mAnimation = 0.f;
    float mInScale = 0.f;
    float mBlur = 0.f;
    float mMidclickRounding = 1.f;
    bool mIsEnabled = false;
    bool mIsPressingShift = false;
    float mTextSize = 1.f;
    float mTextHeight = 0.f;
    std::string mTooltip;
    bool mLowercase = false;

    // Colours
    ImColor textColor = ImColor(255, 255, 255);
    ImColor darkBlack = ImColor(24, 24, 24);
    ImColor mainColor = ImColor(22, 22, 22);
    ImColor grayColor = ImColor(40, 40, 40);
    ImColor enumBackGround = ImColor(30, 30, 30);

    // Utility
    ImVec4 scaleToPoint(const ImVec4& _this, const ImVec4& point, float amount);
    bool isMouseOver(const ImVec4& rect);
    ImVec4 getCenter(ImVec4& vec);

    // Main entry
    void render(float animation, float inScale, int& scrollDirection, float blur, float midclickRounding, bool isPressingShift);
    void onWindowResizeEvent(class WindowResizeEvent& event);

private:
    // Frame setup
    void cacheFrameData(float animation, float inScale, float blur, float midclickRounding, bool isPressingShift, bool isEnabled);
    void initCategoryPositions();
    std::vector<std::shared_ptr<Module>> getFilteredModules(size_t catIndex);
    bool isSearchActive() const;

    // Rendering
    void renderBackground();
    void renderSearchBar();
    void renderColorPickerWindow();
    void renderCategories(int& scrollDirection);
    void renderCategory(size_t index, int& scrollDirection);
    void renderCategoryHeader(size_t index, const ImVec4& catRect, const ImColor& rgb);
    void renderCategoryModules(size_t catIndex, const ImVec4& catRect);
    void renderModule(size_t catIndex, const ImVec4& catRect, const ImVec4& modRect,
                      const std::shared_ptr<Module>& mod, bool isLast, float& moduleY, bool& moduleToggled);
    void renderModuleSettings(size_t catIndex, const std::shared_ptr<Module>& mod, const ImVec4& modRect,
                              const ImVec4& catRect, float& moduleY, bool isLast);
    void renderBoolSetting(size_t catIndex, BoolSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                           float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim);
    void renderEnumSetting(size_t catIndex, EnumSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                           float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim);
    void renderNumberSetting(size_t catIndex, NumberSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                             float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim);
    void renderColorSetting(size_t catIndex, ColorSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                            float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim);
    void renderTooltip();

    // Input handling
    void handleCategoryDragging(size_t index, const ImVec4& catRect);
    void handleBinding();
    void handleBoolBinding();
};