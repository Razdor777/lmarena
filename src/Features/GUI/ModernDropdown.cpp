//
// Created by Tozic on 7/15/2024.
// Refactored for readability and maintainability
//

#include "ModernDropdown.hpp"
#include <Features/Modules/ModuleCategory.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Features/Modules/Setting.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/Keyboard.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>


// ============================================================================
// Utility methods
// ============================================================================

ImVec4 ModernGui::scaleToPoint(const ImVec4& _this, const ImVec4& point, float amount)
{
    return {
        point.x + (_this.x - point.x) * amount,
        point.y + (_this.y - point.y) * amount,
        point.z + (_this.z - point.z) * amount,
        point.w + (_this.w - point.w) * amount
    };
}

bool ModernGui::isMouseOver(const ImVec4& rect)
{
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    return mousePos.x >= rect.x && mousePos.y >= rect.y && mousePos.x < rect.z && mousePos.y < rect.w;
}

ImVec4 ModernGui::getCenter(ImVec4& vec)
{
    float cx = (vec.x + vec.z) / 2.0f;
    float cy = (vec.y + vec.w) / 2.0f;
    return { cx, cy, cx, cy };
}

// ============================================================================
// Frame data caching
// ============================================================================

void ModernGui::cacheFrameData(float animation, float inScale, float blur, float midclickRounding,
                                bool isPressingShift, bool isEnabled)
{
    mAnimation = animation;
    mInScale = inScale;
    mBlur = blur;
    mMidclickRounding = midclickRounding;
    mIsPressingShift = isPressingShift;
    mIsEnabled = isEnabled;
    mMousePos = ImGui::GetIO().MousePos;
    mScreenSize = ImRenderUtils::getScreenSize();
    mDeltaTime = ImGui::GetIO().DeltaTime;
    mTextSize = inScale;
    mTextHeight = ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, "").y;
    mTooltip = "";

    auto interfaceMod = gFeatureManager->mModuleManager->getModule<Interface>();
    mLowercase = interfaceMod && (interfaceMod->mNamingStyle.mValue == NamingStyle::Lowercase ||
                                   interfaceMod->mNamingStyle.mValue == NamingStyle::LowercaseSpaced);
}

bool ModernGui::isSearchActive() const
{
    return mSearchBuffer[0] != '\0';
}

std::vector<std::shared_ptr<Module>> ModernGui::getFilteredModules(size_t catIndex)
{
    auto allModules = gFeatureManager->mModuleManager->getModulesInCategory(catIndex);

    if (!isSearchActive()) return allModules;

    std::string searchStr(mSearchBuffer);
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    std::vector<std::shared_ptr<Module>> filtered;
    for (const auto& mod : allModules)
    {
        std::string modName = mod->getName();
        std::transform(modName.begin(), modName.end(), modName.begin(), ::tolower);
        if (modName.find(searchStr) != std::string::npos)
            filtered.push_back(mod);
    }
    return filtered;
}

// ============================================================================
// Category position initialization
// ============================================================================

void ModernGui::initCategoryPositions()
{
    if (!resetPosition && !catPositions.empty()) return;
    if (resetPosition && NOW - lastReset <= 100) return;

    catPositions.clear();
    static std::vector<std::string> categories = ModuleCategoryNames;

    float centerX = mScreenSize.x / 2.f;
    float xPos = centerX - (categories.size() * (catWidth + catGap) / 2);

    for (size_t i = 0; i < categories.size(); i++)
    {
        CategoryPosition pos;
        pos.x = std::round(xPos / 2) * 2;
        pos.y = std::round((catGap * 2) / 2) * 2;
        xPos += catWidth + catGap;
        catPositions.push_back(pos);
    }

    resetPosition = false;
}

// ============================================================================
// Background rendering
// ============================================================================

void ModernGui::renderBackground()
{
    auto drawList = ImGui::GetBackgroundDrawList();

    // Dark overlay
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(mScreenSize.x, mScreenSize.y),
        IM_COL32(0, 0, 0, (int)(255 * mAnimation * 0.38f)));

    // Blur
    ImRenderUtils::addBlur(ImVec4(0.f, 0.f, mScreenSize.x, mScreenSize.y), mAnimation * mBlur, 0);

    // Bottom gradient glow
    ImColor shadowColor = ColorUtils::getThemedColor(0);
    shadowColor.Value.w = 0.5f * mAnimation;

    float glowStart = MathUtils::lerp(mScreenSize.y, mScreenSize.y - mScreenSize.y / 3, mInScale);
    ImRenderUtils::fillGradientOpaqueRectangle(
        ImVec4(0, glowStart, mScreenSize.x, mScreenSize.y),
        shadowColor, shadowColor, 0.4f * mInScale, 0.0f);
}

// ============================================================================
// Search bar
// ============================================================================

void ModernGui::renderSearchBar()
{
    if (!mIsEnabled) return;

    float barWidth = catWidth;
    float barHeight = catHeight;
    float barX = mScreenSize.x / 2.f - barWidth / 2.f;
    float barY = 8.f;

    ImVec4 barRect = ImVec4(barX, barY, barX + barWidth, barY + barHeight);
    barRect = scaleToPoint(barRect,
        ImVec4(mScreenSize.x / 2, mScreenSize.y / 2, mScreenSize.x / 2, mScreenSize.y / 2), mInScale);

    // Background — same style as category headers
    ImRenderUtils::fillRectangle(barRect, darkBlack, mAnimation, 15);

    // Accent line at bottom
    ImColor accentColor = ColorUtils::getThemedColor(0);
    accentColor.Value.w = mAnimation * (mSearching ? 0.8f : 0.3f);
    ImVec4 lineRect = ImVec4(barRect.x + 4, barRect.w - 1.5f, barRect.z - 4, barRect.w + 0.5f);
    ImRenderUtils::fillRectangle(lineRect, accentColor, mAnimation * (mSearching ? 1.f : 0.5f), 2);

    // Handle focus on click
    if (ImGui::IsMouseClicked(0))
    {
        mSearching = ImRenderUtils::isMouseOver(barRect);
    }

    // Text rendering
    std::string displayText(mSearchBuffer);
    float fontSize = mTextSize;
    float textPad = 10.f;
    float textY = barRect.y + ((barRect.w - barRect.y) - mTextHeight) / 2;

    if (displayText.empty() && !mSearching)
    {
        // Hint text
        ImRenderUtils::drawText(ImVec2(barRect.x + textPad, textY),
            "Search...", ImColor(100, 100, 100), fontSize, mAnimation, true);
    }
    else
    {
        // User text
        ImRenderUtils::drawText(ImVec2(barRect.x + textPad, textY),
            displayText, ImColor(255, 255, 255), fontSize, mAnimation, true);

        // Blinking cursor when focused
        if (mSearching)
        {
            static float cursorBlink = 0.f;
            cursorBlink += mDeltaTime;
            if (fmod(cursorBlink, 1.0f) < 0.5f)
            {
                float textW = ImRenderUtils::getTextWidth(&displayText, fontSize);
                float cursorX = barRect.x + textPad + textW + 2;
                float cursorY1 = barRect.y + 5 * mInScale;
                float cursorY2 = barRect.w - 5 * mInScale;
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(cursorX, cursorY1), ImVec2(cursorX, cursorY2),
                    IM_COL32(255, 255, 255, (int)(255 * mAnimation)), 1.5f);
            }
        }
    }

    // Search icon (⌕) on the right side
    std::string icon = "s"; // tenacity_icons — or just use a simple text char
    float iconW = ImRenderUtils::getTextWidth(&icon, fontSize * 0.85f);
    float iconY = barRect.y + ((barRect.w - barRect.y) - mTextHeight * 0.85f) / 2;
    ImRenderUtils::drawText(ImVec2(barRect.z - iconW - 8, iconY),
        icon, ImColor(150, 150, 150), fontSize * 0.85f, mAnimation * 0.7f, true);
}

// ============================================================================
// Color picker window
// ============================================================================

void ModernGui::renderColorPickerWindow()
{
    if (!displayColorPicker || !mIsEnabled) return;

    FontHelper::pushPrefFont(false, false, true);
    ColorSetting* colorSetting = lastColorSetting;

    ImGui::SetNextWindowPos(ImVec2(mScreenSize.x / 2 - 200, mScreenSize.y / 2));
    ImGui::SetNextWindowSize(ImVec2(400, 400));

    ImGui::Begin("Color Picker", &displayColorPicker,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        ImVec4 color = colorSetting->getAsImColor().Value;
        ImGui::ColorPicker4("Color", colorSetting->mValue,
            ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha);
        ImGui::Button("Close");
        if (ImGui::IsItemClicked())
        {
            colorSetting->setFromImColor(ImColor(color));
            displayColorPicker = false;
        }
    }
    ImGui::End();
    ImGui::PopFont();

    if (ImGui::IsMouseClicked(0) && !ImRenderUtils::isMouseOver(
        ImVec4(mScreenSize.x / 2 - 200, mScreenSize.y / 2,
               mScreenSize.x / 2 + 200, mScreenSize.y / 2 + 400)))
    {
        displayColorPicker = false;
    }
}

// ============================================================================
// Main render entry point
// ============================================================================

void ModernGui::render(float animation, float inScale, int& scrollDirection, float blur,
                        float midclickRounding, bool isPressingShift)
{
    auto clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    bool isEnabled = clickGui->mEnabled;

    cacheFrameData(animation, inScale, blur, midclickRounding, isPressingShift, isEnabled);

    FontHelper::pushPrefFont(true, false, true);

    initCategoryPositions();
    renderBackground();
    renderSearchBar();
    renderColorPickerWindow();

    if (!mIsEnabled) displayColorPicker = false;

    if (!catPositions.empty())
        renderCategories(scrollDirection);

    renderTooltip();

    if (isEnabled) scrollDirection = 0;

    ImGui::PopFont();
}

// ============================================================================
// Categories rendering
// ============================================================================

void ModernGui::renderCategories(int& scrollDirection)
{
    static std::vector<std::string> categories = ModuleCategoryNames;

    for (size_t i = 0; i < categories.size(); i++)
    {
        renderCategory(i, scrollDirection);
    }

    handleBinding();
    handleBoolBinding();
}

void ModernGui::renderCategory(size_t index, int& scrollDirection)
{
    static std::vector<std::string> categories = ModuleCategoryNames;

    auto modsInCategory = getFilteredModules(index);

    // Skip categories with no results when searching
    if (isSearchActive() && modsInCategory.empty()) return;

    float moduleY = -catPositions[index].yOffset;

    // Category rect (scaled)
    ImVec4 catRect = ImVec4(catPositions[index].x, catPositions[index].y,
                            catPositions[index].x + catWidth, catPositions[index].y + catHeight)
        .scaleToPoint(ImVec4(mScreenSize.x / 2, mScreenSize.y / 2,
                              mScreenSize.x / 2, mScreenSize.y / 2), mInScale);

    // Calculate total window height including animated settings
    float settingsHeight = 0;
    for (const auto& mod : modsInCategory)
    {
        for (const auto& setting : mod->mSettings)
        {
            switch (setting->mType)
            {
            case SettingType::Bool:
            case SettingType::Number:
            case SettingType::Color:
                settingsHeight = MathUtils::lerp(settingsHeight, settingsHeight + modHeight, mod->cAnim);
                break;
            case SettingType::Enum:
            {
                auto* es = reinterpret_cast<EnumSetting*>(setting);
                settingsHeight = MathUtils::lerp(settingsHeight, settingsHeight + modHeight, mod->cAnim);
                if (setting->enumSlide > 0.01)
                {
                    for (int j = 0; j < static_cast<int>(es->mValues.size()); j++)
                        settingsHeight = MathUtils::lerp(settingsHeight, settingsHeight + modHeight, setting->enumSlide);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    float catWindowHeight = catHeight + modHeight * modsInCategory.size() + settingsHeight;
    ImVec4 catWindow = ImVec4(catPositions[index].x, catPositions[index].y,
                               catPositions[index].x + catWidth,
                               catPositions[index].y + moduleY + catWindowHeight)
        .scaleToPoint(ImVec4(mScreenSize.x / 2, mScreenSize.y / 2,
                              mScreenSize.x / 2, mScreenSize.y / 2), mInScale);

    ImColor rgb = ColorUtils::getThemedColor(index * 20);

    // Scrolling
    if (ImRenderUtils::isMouseOver(catWindow) && catPositions[index].isExtended)
    {
        if (scrollDirection > 0)
        {
            catPositions[index].scrollEase += scrollDirection * catHeight;
            catPositions[index].scrollEase = std::min(catPositions[index].scrollEase, catWindowHeight - modHeight * 2);
        }
        else if (scrollDirection < 0)
        {
            catPositions[index].scrollEase += scrollDirection * catHeight;
            catPositions[index].scrollEase = std::max(catPositions[index].scrollEase, 0.f);
        }
        scrollDirection = 0;
    }

    // Handle extend/collapse
    if (!catPositions[index].isExtended)
    {
        catPositions[index].scrollEase = catWindowHeight - catHeight;
        catPositions[index].wasExtended = false;
    }
    else if (!catPositions[index].wasExtended)
    {
        catPositions[index].scrollEase = 0;
        catPositions[index].wasExtended = true;
    }

    // Smooth scroll lerp
    catPositions[index].yOffset = MathUtils::animate(
        catPositions[index].scrollEase, catPositions[index].yOffset,
        ImRenderUtils::getDeltaTime() * 10.5);

    // Clip rect
    ImVec4 clipRect = ImVec4(catRect.x, catRect.w, catRect.z, mScreenSize.y);
    ImGui::GetBackgroundDrawList()->PushClipRect(
        ImVec2(clipRect.x, clipRect.y), ImVec2(clipRect.z, clipRect.w), true);

    renderCategoryModules(index, catRect);

    ImGui::GetBackgroundDrawList()->PopClipRect();

    // Header & dragging
    renderCategoryHeader(index, catRect, rgb);
    handleCategoryDragging(index, catRect);
}

// ============================================================================
// Category header
// ============================================================================

void ModernGui::renderCategoryHeader(size_t index, const ImVec4& catRect, const ImColor& rgb)
{
    static std::vector<std::string> categories = ModuleCategoryNames;
    std::string catName = mLowercase ? StringUtils::toLower(categories[index]) : categories[index];

    // Right-click to toggle extend
    if (ImRenderUtils::isMouseOver(catRect) && ImGui::IsMouseClicked(1))
        catPositions[index].isExtended = !catPositions[index].isExtended;

    ImVec4 headerRect = catRect;
    headerRect.w += 1.5f;
    ImRenderUtils::fillRectangle(headerRect, darkBlack, mAnimation, 15,
        ImGui::GetBackgroundDrawList(), ImDrawFlags_RoundCornersTop);

    // Icon mapping
    std::string iconStr = "B";
    if (StringUtils::equalsIgnoreCase(catName, "Combat")) iconStr = "c";
    else if (StringUtils::equalsIgnoreCase(catName, "Movement")) iconStr = "f";
    else if (StringUtils::equalsIgnoreCase(catName, "Visual")) iconStr = "d";
    else if (StringUtils::equalsIgnoreCase(catName, "Player")) iconStr = "e";
    else if (StringUtils::equalsIgnoreCase(catName, "Misc")) iconStr = "a";

    FontHelper::pushPrefFont(true, true, true);
    float textH = ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, catName.c_str()).y;
    float cX = catRect.x + ((catRect.z - catRect.x) - ImRenderUtils::getTextWidth(&catName, mTextSize * 1.15)) / 2;
    float cY = catRect.y + ((catRect.w - catRect.y) - textH) / 2;

    ImGui::PushFont(FontHelper::Fonts["tenacity_icons_large"]);
    ImRenderUtils::drawText(ImVec2(catRect.x + 10, cY), iconStr,
        ImColor(255, 255, 255), mTextSize * 1.15, mAnimation, true);
    ImGui::PopFont();

    ImRenderUtils::drawText(ImVec2(cX, cY), catName,
        ImColor(255, 255, 255), mTextSize * 1.15, mAnimation, true);
    ImGui::PopFont();

    // Clamp positions to screen
    catPositions[index].x = std::clamp(catPositions[index].x, 0.f, mScreenSize.x - catWidth);
    catPositions[index].y = std::clamp(catPositions[index].y, 0.f, mScreenSize.y - catHeight);
}

// ============================================================================
// Category modules
// ============================================================================

void ModernGui::renderCategoryModules(size_t catIndex, const ImVec4& catRect)
{
    auto modsInCategory = getFilteredModules(catIndex);
    float moduleY = -catPositions[catIndex].yOffset;
    bool moduleToggled = false;
    int modIndex = 0;

    for (const auto& mod : modsInCategory)
    {
        bool isLast = modIndex == modsInCategory.size() - 1;

        ImVec4 modRect = ImVec4(catPositions[catIndex].x,
                                catPositions[catIndex].y + catHeight + moduleY,
                                catPositions[catIndex].x + catWidth,
                                catPositions[catIndex].y + catHeight + moduleY + modHeight)
            .scaleToPoint(ImVec4(mScreenSize.x / 2, mScreenSize.y / 2,
                                  mScreenSize.x / 2, mScreenSize.y / 2), mInScale);

        modRect.y = std::floor(modRect.y);
        modRect.x = std::floor(modRect.x);

        // Animate settings expansion
        float targetAnim = mod->showSettings ? 1.f : 0.f;
        mod->cAnim = MathUtils::animate(targetAnim, mod->cAnim, ImRenderUtils::getDeltaTime() * 12.5);
        mod->cAnim = MathUtils::clamp(mod->cAnim, 0.f, 1.f);

        // Render settings first (they appear below the module)
        if (mod->cAnim > 0.001)
            renderModuleSettings(catIndex, mod, modRect, catRect, moduleY, isLast);

        // Render module row
        renderModule(catIndex, catRect, modRect, mod, isLast, moduleY, moduleToggled);

        moduleY += modHeight;
        modIndex++;
    }
}

// ============================================================================
// Single module row
// ============================================================================

void ModernGui::renderModule(size_t catIndex, const ImVec4& catRect, const ImVec4& modRect,
                              const std::shared_ptr<Module>& mod, bool isLast, float& moduleY, bool& moduleToggled)
{
    float radius = isLast ? 15.f * (1.f - mod->cAnim) : 0.f;
    ImDrawFlags flags = ImDrawFlags_RoundCornersBottom;
    ImColor modRgb = ColorUtils::getThemedColor(moduleY * 2);

    if (modRect.y > catRect.y + 0.5f)
    {
        // Background layers
        if (mod->cScale <= 1)
        {
            ImColor bgColor = mod->mEnabled ? modRgb : ImColor(30, 30, 30);
            ImRenderUtils::fillRectangle(modRect, bgColor, mAnimation, radius,
                ImGui::GetBackgroundDrawList(), ImDrawCornerFlags_BotRight | ImDrawCornerFlags_BotLeft);
            ImRenderUtils::fillRectangle(modRect, grayColor, mAnimation, radius,
                ImGui::GetBackgroundDrawList(), ImDrawCornerFlags_BotRight | ImDrawCornerFlags_BotLeft);
        }

        std::string modName = mod->getName();
        float modW = modRect.z - modRect.x;
        float modH = modRect.w - modRect.y;
        ImVec2 center = ImVec2(modRect.x + modW / 2.f,
                                modRect.y + modH / 2.f);

        mod->cScale = MathUtils::animate(mod->mEnabled ? 1 : 0, mod->cScale,
            ImRenderUtils::getDeltaTime() * 10);

        // Gradient fill for enabled modules
        if (mod->cScale > 0)
        {
            float sw = modW;
            float sh = modH;
            ImVec4 scaledRect = ImVec4(center.x - sw / 2.f, center.y - sh / 2.f,
                                       center.x + sw / 2.f, center.y + sh / 2.f);

            ImColor rgb1 = modRgb;
            ImColor rgb2 = ColorUtils::getThemedColor(scaledRect.y + (scaledRect.z - scaledRect.x));
            ImRenderUtils::fillRoundedGradientRectangle(scaledRect, rgb1, rgb2, radius,
                mAnimation * mod->cScale, mAnimation * mod->cScale, flags);
        }

        // Module name
        float cX = modRect.x + ((modRect.z - modRect.x) - ImRenderUtils::getTextWidth(&modName, mTextSize)) / 2;
        float cY = modRect.y + ((modRect.w - modRect.y) - mTextHeight) / 2;

        ImColor nameColor = ImColor(mod->mEnabled ? ImColor(255, 255, 255) : ImColor(180, 180, 180))
            .Lerp(mod->mEnabled ? ImColor(255, 255, 255) : ImColor(180, 180, 180), mod->cAnim);

        ImRenderUtils::drawText(ImVec2(cX, cY), modName, nameColor, mTextSize, mAnimation, true);

        // Mouse interaction
        if (ImRenderUtils::isMouseOver(modRect) && catPositions[catIndex].isExtended && mIsEnabled)
        {
            mTooltip = mod->mDescription;

            if (ImGui::IsMouseClicked(0) && !displayColorPicker && catPositions[catIndex].isExtended)
            {
                if (!moduleToggled) mod->toggle();
                ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
                moduleToggled = true;
            }
            else if (ImGui::IsMouseClicked(1) && !displayColorPicker && catPositions[catIndex].isExtended)
            {
                if (!mod->mSettings.empty()) mod->showSettings = !mod->showSettings;
            }
            else if (ImGui::IsMouseClicked(2) && !displayColorPicker && catPositions[catIndex].isExtended)
            {
                lastMod = mod;
                isBinding = true;
                ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
            }
        }
    }

    // Glow below module
    if (modRect.y > catRect.y - modHeight)
    {
        ImRenderUtils::fillGradientOpaqueRectangle(
            ImVec4(modRect.x, modRect.w, modRect.z, modRect.w + 10.f * mod->cAnim * mAnimation),
            ImColor(0, 0, 0), ImColor(0, 0, 0), 0.f * mAnimation, 0.55f * mAnimation);
    }
}

// ============================================================================
// Module settings dispatcher
// ============================================================================

void ModernGui::renderModuleSettings(size_t catIndex, const std::shared_ptr<Module>& mod, const ImVec4& modRect,
                                      const ImVec4& catRect, float& moduleY, bool isLast)
{
    int sIndex = 0;
    for (const auto& setting : mod->mSettings)
    {
        if (!setting->mIsVisible())
        {
            setting->sliderEase = 0;
            setting->enumSlide = 0;
            continue;
        }

        float radius = 0.f;
        if (isLast && sIndex == static_cast<int>(mod->mSettings.size()) - 1)
            radius = 15.f;
        else if (isLast)
            radius = 15.f * (1.f - mod->cAnim);

        float setPadding = (sIndex == static_cast<int>(mod->mSettings.size()) - 1)
            ? (-2.f * mAnimation) : 0.f;

        ImColor rgb = ColorUtils::getThemedColor(moduleY * 2);
        rgb.Value.w = mAnimation;

        switch (setting->mType)
        {
        case SettingType::Bool:
            renderBoolSetting(catIndex, reinterpret_cast<BoolSetting*>(setting), modRect, catRect,
                              moduleY, radius, rgb, setPadding, mod->cAnim);
            break;
        case SettingType::Enum:
            renderEnumSetting(catIndex, reinterpret_cast<EnumSetting*>(setting), modRect, catRect,
                              moduleY, radius, rgb, setPadding, mod->cAnim);
            break;
        case SettingType::Number:
            renderNumberSetting(catIndex, reinterpret_cast<NumberSetting*>(setting), modRect, catRect,
                                moduleY, radius, rgb, setPadding, mod->cAnim);
            break;
        case SettingType::Color:
            renderColorSetting(catIndex, reinterpret_cast<ColorSetting*>(setting), modRect, catRect,
                               moduleY, radius, rgb, setPadding, mod->cAnim);
            break;
        default:
            break;
        }

        sIndex++;
    }
}

// ============================================================================
// Bool setting
// ============================================================================

void ModernGui::renderBoolSetting(size_t catIndex, BoolSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                                   float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim)
{
    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);

    ImVec4 rect = ImVec4(modRect.x, catRect.w + moduleY + setPadding,
                          modRect.z, catRect.w + moduleY + modHeight)
        .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

    rect.y = std::floor(rect.y);
    if (rect.y < modRect.y) rect.y = modRect.y;

    if (rect.y > catRect.y + 0.5f)
    {
        std::string setName = mLowercase ? StringUtils::toLower(setting->mName) : setting->mName;

        ImRenderUtils::fillRectangle(rect, ImColor(30, 30, 30), mAnimation, radius,
            ImGui::GetBackgroundDrawList(), ImDrawFlags_RoundCornersBottom);

        if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended)
        {
            mTooltip = setting->mDescription;
            if (ImGui::IsMouseClicked(0) && !displayColorPicker)
                setting->mValue = !setting->mValue;
            if (ImGui::IsMouseClicked(2) && !displayColorPicker)
            {
                lastBoolSetting = setting;
                isBoolSettingBinding = true;
                ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
            }
        }

        setting->boolScale = MathUtils::animate(setting->mValue ? 1 : 0, setting->boolScale,
            ImRenderUtils::getDeltaTime() * 10);

        float cSetRectCentreX = rect.x + ((rect.z - rect.x) - ImRenderUtils::getTextWidth(&setName, mTextSize)) / 2;
        float cSetRectCentreY = rect.y + ((rect.w - rect.y) - mTextHeight) / 2;

        // Toggle switch
        ImVec4 smoothScaledRect = ImVec4(rect.z - 19, rect.y + 5, rect.z - 5, rect.w - 5);
        ImVec2 circleRect = ImVec2(smoothScaledRect.getCenter().x, smoothScaledRect.getCenter().y);

        ImColor targetShadow = ImColor(15, 15, 15);
        ImColor rgbCopy = rgb;
        ImColor shadowCol = MathUtils::lerpImColor(targetShadow, rgbCopy, setting->boolScale);
        ImRenderUtils::fillShadowCircle(circleRect, 5, shadowCol, mAnimation, 40, 0);

        // Check mark
        ImVec4 booleanRect = ImVec4(rect.z - 23.5f, cSetRectCentreY - 2.5f, rect.z - 5, cSetRectCentreY + 17.5f);
        booleanRect = booleanRect.scaleToPoint(ImVec4(rect.z, rect.y, rect.z, rect.w), mAnimation);

        if (setting->boolScale > 0.01)
        {
            float rectXDiff = booleanRect.z - booleanRect.x;
            ImGui::GetForegroundDrawList()->PushClipRect(
                ImVec2(booleanRect.x, booleanRect.y),
                ImVec2(booleanRect.x + rectXDiff * setting->boolScale, booleanRect.w), true);

            ImRenderUtils::drawCheckMark(
                ImVec2(booleanRect.getCenter().x - (4 * mAnimation),
                       booleanRect.getCenter().y - (1 * mAnimation)),
                1.3f * mAnimation, rgb, mAnimation);

            ImGui::GetForegroundDrawList()->PopClipRect();
        }

        ImRenderUtils::drawText(ImVec2(rect.x + 5.f, cSetRectCentreY), setName,
            ImColor(255, 255, 255), mTextSize, mAnimation, true);
    }
}

// ============================================================================
// Enum setting
// ============================================================================

void ModernGui::renderEnumSetting(size_t catIndex, EnumSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                                   float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim)
{
    std::string setName = mLowercase ? StringUtils::toLower(setting->mName) : setting->mName;
    std::vector<std::string> enumValues = setting->mValues;
    if (mLowercase)
    {
        for (auto& val : enumValues)
            val = StringUtils::toLower(val);
    }

    int* iterator = &setting->mValue;
    int numValues = static_cast<int>(enumValues.size());

    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);

    ImVec4 rect = ImVec4(modRect.x, catRect.w + moduleY + setPadding,
                          modRect.z, catRect.w + moduleY + modHeight)
        .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

    rect.y = std::floor(rect.y);
    if (rect.y < modRect.y) rect.y = modRect.y;

    // Animate enum expansion
    float targetAnim = setting->enumExtended ? 1.f : 0.f;
    setting->enumSlide = MathUtils::animate(targetAnim, setting->enumSlide, ImRenderUtils::getDeltaTime() * 10);
    setting->enumSlide = MathUtils::clamp(setting->enumSlide, 0.f, 1.f);

    // Expanded values
    if (setting->enumSlide > 0.001)
    {
        for (int j = 0; j < numValues; j++)
        {
            moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, setting->enumSlide);

            ImVec4 rect2 = ImVec4(modRect.x, catRect.w + moduleY + setPadding,
                                   modRect.z, catRect.w + moduleY + modHeight)
                .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

            if (rect2.y > catRect.y + 0.5f)
            {
                float cY = rect2.y + ((rect2.w - rect2.y) - mTextHeight) / 2;

                ImRenderUtils::fillRectangle(rect2, ImColor(20, 20, 20), mAnimation, radius,
                    ImGui::GetBackgroundDrawList(), ImDrawFlags_RoundCornersBottom);

                if (*iterator == j)
                    ImRenderUtils::fillRectangle(ImVec4(rect2.x, rect2.y, rect2.x + 1.5f, rect2.w),
                        rgb, mAnimation);

                if (ImRenderUtils::isMouseOver(rect2) && ImGui::IsMouseClicked(0) &&
                    mIsEnabled && !displayColorPicker)
                {
                    *iterator = j;
                }

                ImRenderUtils::drawText(ImVec2(rect2.x + 5.f, cY), enumValues[j],
                    ImColor(255, 255, 255), mTextSize, mAnimation, true);
            }
        }
    }

    // Main enum row
    if (rect.y > catRect.y + 0.5f)
    {
        ImRenderUtils::fillRectangle(rect, ImColor(30, 30, 30), mAnimation, radius,
            ImGui::GetBackgroundDrawList(), ImDrawFlags_RoundCornersBottom);

        if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended)
        {
            mTooltip = setting->mDescription;
            if (ImGui::IsMouseClicked(0) && !displayColorPicker)
                *iterator = (*iterator + 1) % enumValues.size();
            else if (ImGui::IsMouseClicked(1) && !displayColorPicker)
                setting->enumExtended = !setting->enumExtended;
        }

        float cY = rect.y + ((rect.w - rect.y) - mTextHeight) / 2;

        std::string enumValue = enumValues[*iterator];
        auto valueLen = ImRenderUtils::getTextWidth(&enumValue, mTextSize);

        ImRenderUtils::drawText(ImVec2(rect.x + 5.f, cY), setName,
            ImColor(255, 255, 255), mTextSize, mAnimation, true);
        ImRenderUtils::drawText(ImVec2((rect.z - 5.f) - valueLen, cY), enumValue,
            ImColor(170, 170, 170), mTextSize, mAnimation, true);
    }

    // Shadow below expanded enum
    if (rect.y > catRect.y - modHeight)
    {
        ImRenderUtils::fillGradientOpaqueRectangle(
            ImVec4(rect.x, rect.w, rect.z, rect.w + 10.f * setting->enumSlide * mAnimation),
            ImColor(0, 0, 0), ImColor(0, 0, 0), 0.f * mAnimation, 0.55f * mAnimation);
    }
}

// ============================================================================
// Number setting
// ============================================================================

void ModernGui::renderNumberSetting(size_t catIndex, NumberSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                                     float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim)
{
    const float value = setting->mValue;
    const float min = setting->mMin;
    const float max = setting->mMax;

    char str[10];
    sprintf_s(str, 10, "%.2f", value);
    std::string rVal = str;

    std::string setName = mLowercase ? StringUtils::toLower(setting->mName) : setting->mName;

    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);

    ImVec4 backGroundRect = ImVec4(modRect.x, catRect.w + moduleY,
                                    modRect.z, catRect.w + moduleY + modHeight)
        .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

    backGroundRect.y = std::floor(backGroundRect.y);
    if (backGroundRect.y < modRect.y) backGroundRect.y = modRect.y;

    ImVec4 rect = ImVec4(modRect.x + 7, catRect.w + moduleY + setPadding,
                          modRect.z - 7, catRect.w + moduleY + modHeight)
        .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

    rect.y = std::floor(rect.y);
    if (rect.y < modRect.y) rect.y = modRect.y;

    // Click animation
    static float clickAnimation = 1.f;
    if (ImGui::IsMouseDown(0) && ImRenderUtils::isMouseOver(rect))
        clickAnimation = MathUtils::animate(0.60f, clickAnimation, ImRenderUtils::getDeltaTime() * 10);
    else
        clickAnimation = MathUtils::animate(1.f, clickAnimation, ImRenderUtils::getDeltaTime() * 10);

    if (backGroundRect.y > catRect.y + 0.5f)
    {
        ImRenderUtils::fillRectangle(backGroundRect, ImColor(30, 30, 30), mAnimation, radius,
            ImGui::GetBackgroundDrawList(), ImDrawFlags_RoundCornersBottom);

        const float sliderPos = (value - min) / (max - min) * (rect.z - rect.x);
        setting->sliderEase = MathUtils::animate(sliderPos, setting->sliderEase, ImRenderUtils::getDeltaTime() * 10);
        setting->sliderEase = std::clamp(setting->sliderEase, 0.f, rect.getWidth());

        // Slider dragging
        if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended)
        {
            mTooltip = setting->mDescription;
            if (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(2))
            {
                setting->isDragging = true;
                lastDraggedSetting = setting;
            }
        }

        if (ImGui::IsMouseDown(0) && setting->isDragging && mIsEnabled)
        {
            if (lastDraggedSetting != setting)
                setting->isDragging = false;
            else
            {
                float newValue = std::clamp(
                    (ImRenderUtils::getMousePos().x - rect.x) / (rect.z - rect.x) * (max - min) + min,
                    min, max);
                setting->setValue(newValue);
            }
        }
        else if (ImGui::IsMouseDown(2) && setting->isDragging && mIsEnabled)
        {
            if (lastDraggedSetting != setting)
                setting->isDragging = false;
            else
            {
                float newValue = std::clamp(
                    (ImRenderUtils::getMousePos().x - rect.x) / (rect.z - rect.x) * (max - min) + min,
                    min, max);
                newValue = std::round(newValue / mMidclickRounding) * mMidclickRounding;
                setting->mValue = newValue;
            }
        }
        else
        {
            setting->isDragging = false;
        }

        // Slider bar
        float ySize = rect.w - rect.y;
        ImVec2 sliderBarMin = ImVec2(rect.x, rect.w - ySize / 8);
        ImVec2 sliderBarMax = ImVec2(rect.x + (setting->sliderEase * mInScale), rect.w);
        sliderBarMin.y = sliderBarMax.y - 4 * mInScale;

        ImVec4 sliderRect = ImVec4(sliderBarMin.x, sliderBarMin.y - 4.5f,
                                    sliderBarMax.x, sliderBarMax.y - 6.5f);

        ImRenderUtils::fillRectangle(sliderRect, rgb, mAnimation, 15);

        // Circle
        ImVec2 circlePos = ImVec2(sliderRect.z - 2.25f, sliderRect.getCenter().y);
        if (value <= min + 0.83f)
            circlePos.x = sliderRect.z + 2.25f;

        ImRenderUtils::fillCircle(circlePos, 5.5f * clickAnimation * mAnimation, rgb, mAnimation, 12);

        // Shadow
        ImGui::GetBackgroundDrawList()->PushClipRect(
            ImVec2(sliderRect.x, sliderRect.y), ImVec2(sliderRect.z, sliderRect.w), true);
        ImRenderUtils::fillShadowRectangle(sliderRect, rgb, mAnimation * 0.75f, 15.f, 0);
        ImGui::GetBackgroundDrawList()->PopClipRect();

        // Labels
        auto valueLen = ImRenderUtils::getTextWidth(&rVal, mTextSize);
        ImRenderUtils::drawText(ImVec2((backGroundRect.z - 5.f) - valueLen, backGroundRect.y + 2.5f),
            rVal, ImColor(170, 170, 170), mTextSize, mAnimation, true);
        ImRenderUtils::drawText(ImVec2(backGroundRect.x + 5.f, backGroundRect.y + 2.5f),
            setName, ImColor(255, 255, 255), mTextSize, mAnimation, true);
    }
}

// ============================================================================
// Color setting
// ============================================================================

void ModernGui::renderColorSetting(size_t catIndex, ColorSetting* setting, const ImVec4& modRect, const ImVec4& catRect,
                                    float& moduleY, float radius, const ImColor& rgb, float setPadding, float cAnim)
{
    std::string setName = mLowercase ? StringUtils::toLower(setting->mName) : setting->mName;

    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);

    ImVec4 rect = ImVec4(modRect.x, catRect.w + moduleY + setPadding,
                          modRect.z, catRect.w + moduleY + modHeight)
        .scaleToPoint(ImVec4(modRect.x, mScreenSize.y / 2, modRect.z, mScreenSize.y / 2), mInScale);

    rect.y = std::floor(rect.y);
    if (rect.y < modRect.y) rect.y = modRect.y;

    if (rect.y > catRect.y + 0.5f)
    {
        ImRenderUtils::fillRectangle(rect, ImColor(30, 30, 30), mAnimation);

        if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended)
        {
            mTooltip = setting->mDescription;
            if (ImGui::IsMouseClicked(0) && !displayColorPicker)
            {
                displayColorPicker = !displayColorPicker;
                lastColorSetting = setting;
            }
        }

        float cY = rect.y + ((rect.w - rect.y) - mTextHeight) / 2;
        ImRenderUtils::drawText(ImVec2(rect.x + 5.f, cY), setName,
            ImColor(255, 255, 255), mTextSize, mAnimation, true);

        ImRenderUtils::fillRectangle(ImVec4(rect.z - 20, rect.y + 5, rect.z - 5, rect.w - 5),
            setting->getAsImColor(), mAnimation);
    }
}

// ============================================================================
// Tooltip
// ============================================================================

void ModernGui::renderTooltip()
{
    if (mTooltip.empty()) return;

    ImVec2 toolTipSize = ImGui::GetFont()->CalcTextSizeA(mTextSize * 14.4f, FLT_MAX, 0, mTooltip.c_str());
    float textWidth = ImRenderUtils::getTextWidth(&mTooltip, mTextSize * 0.8f);
    float textHeight = toolTipSize.y;
    float padding = 2.5f;
    float offset = 8.f;

    ImVec4 tooltipRect = ImVec4(
        mMousePos.x + offset - padding,
        mMousePos.y + textHeight / 2 - textHeight - padding,
        mMousePos.x + offset + textWidth + padding * 2,
        mMousePos.y + textHeight / 2 + padding
    ).scaleToPoint(ImVec4(mScreenSize.x / 2, mScreenSize.y / 2, mScreenSize.x / 2, mScreenSize.y / 2), mInScale);

    static float alpha = 1.f;
    if (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(2))
        alpha = MathUtils::animate(0.0f, alpha, ImRenderUtils::getDeltaTime() * 10);
    else
        alpha = MathUtils::animate(1.f, alpha, ImRenderUtils::getDeltaTime() * 10);

    tooltipRect = tooltipRect.scaleToCenter(alpha);

    ImRenderUtils::fillRectangle(tooltipRect, ImColor(20, 20, 20), mAnimation * alpha, 0.f, ImGui::GetForegroundDrawList());
    ImRenderUtils::drawText(ImVec2(tooltipRect.x + padding, tooltipRect.y + padding), mTooltip,
        ImColor(255, 255, 255), (mTextSize * 0.8f) * alpha, mAnimation * alpha, true, 0, ImGui::GetForegroundDrawList());
}

// ============================================================================
// Input: Category dragging
// ============================================================================

void ModernGui::handleCategoryDragging(size_t index, const ImVec4& catRect)
{
    static bool dragging = false;
    static ImVec2 dragOffset;

    if (catPositions[index].isDragging)
    {
        if (ImGui::IsMouseDown(0))
        {
            if (!dragging)
            {
                dragOffset = ImVec2(ImRenderUtils::getMousePos().x - catRect.x,
                                     ImRenderUtils::getMousePos().y - catRect.y);
                dragging = true;
            }
            ImVec2 newPos = ImVec2(ImRenderUtils::getMousePos().x - dragOffset.x,
                                    ImRenderUtils::getMousePos().y - dragOffset.y);
            newPos.x = std::clamp(newPos.x, 0.f, mScreenSize.x - catWidth);
            newPos.y = std::clamp(newPos.y, 0.f, mScreenSize.y - catHeight);
            newPos.x = std::round(newPos.x / 2) * 2;
            newPos.y = std::round(newPos.y / 2) * 2;
            catPositions[index].x = newPos.x;
            catPositions[index].y = newPos.y;
        }
        else
        {
            catPositions[index].isDragging = false;
            dragging = false;
        }
    }
    else if (ImRenderUtils::isMouseOver(catRect) && ImGui::IsMouseClicked(0) && mIsEnabled)
    {
        catPositions[index].isDragging = true;
        dragOffset = ImVec2(ImRenderUtils::getMousePos().x - catRect.x,
                             ImRenderUtils::getMousePos().y - catRect.y);
    }
}

// ============================================================================
// Input: Key binding
// ============================================================================

void ModernGui::handleBinding()
{
    if (!isBinding) return;

    mTooltip = "Currently binding " + lastMod->getName() + "... Press ESC to unbind.";

    for (const auto& key : Keyboard::mPressedKeys)
    {
        if (key.second && lastMod)
        {
            lastMod->mKey = key.first == VK_ESCAPE ? 0 : key.first;
            isBinding = false;

            if (key.first == VK_ESCAPE)
                ClientInstance::get()->playUi("random.break", 0.75f, 1.0f);
            else
                ClientInstance::get()->playUi("random.orb", 0.75f, 1.0f);
        }
    }
}

void ModernGui::handleBoolBinding()
{
    if (!isBoolSettingBinding) return;

    mTooltip = "Currently binding " + lastBoolSetting->mName + "... Press ESC to unbind.";

    for (const auto& key : Keyboard::mPressedKeys)
    {
        if (key.second && lastBoolSetting)
        {
            lastBoolSetting->mKey = (key.first == VK_ESCAPE) ? 0 : key.first;
            isBoolSettingBinding = false;

            if (key.first == VK_ESCAPE)
                ClientInstance::get()->playUi("random.break", 0.75f, 1.0f);
            else
                ClientInstance::get()->playUi("random.orb", 0.75f, 1.0f);
        }
    }
}

// ============================================================================
// Window resize
// ============================================================================

void ModernGui::onWindowResizeEvent(WindowResizeEvent& event)
{
    resetPosition = true;
    lastReset = NOW;
}