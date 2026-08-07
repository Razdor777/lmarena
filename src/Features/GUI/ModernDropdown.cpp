//
// ModernDropdown.cpp
// Rewritten to match HTML ClickGui design
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
#include <cmath>

// ============================================================
//  Utility
// ============================================================

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
    ImVec2 m = ImGui::GetIO().MousePos;
    return m.x >= rect.x && m.y >= rect.y && m.x < rect.z && m.y < rect.w;
}

ImVec4 ModernGui::getCenter(ImVec4& v)
{
    float cx = (v.x + v.z) * 0.5f;
    float cy = (v.y + v.w) * 0.5f;
    return { cx, cy, cx, cy };
}

// ============================================================
//  Frame data
// ============================================================

void ModernGui::cacheFrameData(float animation, float inScale, float blur,
                                float midclickRounding, bool isPressingShift, bool isEnabled)
{
    mAnimation        = animation;
    mInScale          = inScale;
    mBlur             = blur;
    mMidclickRounding = midclickRounding;
    mIsPressingShift  = isPressingShift;
    mIsEnabled        = isEnabled;
    mMousePos         = ImGui::GetIO().MousePos;
    mScreenSize       = ImRenderUtils::getScreenSize();
    mDeltaTime        = ImGui::GetIO().DeltaTime;
    mTextSize         = inScale;
    mTextHeight       = ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, "").y;
    mTooltip          = "";

    auto interfaceMod = gFeatureManager->mModuleManager->getModule<Interface>();
    mLowercase = interfaceMod &&
        (interfaceMod->mNamingStyle.mValue == NamingStyle::Lowercase ||
         interfaceMod->mNamingStyle.mValue == NamingStyle::LowercaseSpaced);
}

bool ModernGui::isSearchActive() const { return mSearchBuffer[0] != '\0'; }

std::vector<std::shared_ptr<Module>> ModernGui::getFilteredModules(size_t catIndex)
{
    auto all = gFeatureManager->mModuleManager->getModulesInCategory(catIndex);
    if (!isSearchActive()) return all;

    std::string q(mSearchBuffer);
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    std::vector<std::shared_ptr<Module>> out;
    for (auto& m : all) {
        // Match against EVERY name the module has:
        // the raw internal name + all naming-style aliases — so any
        // spelling ("nearbyplayers", "nearby players", "NearbyPlayers"...)
        // finds the module, regardless of the current naming style
        bool hit = false;

        std::string raw = m->mName;
        std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);
        if (raw.find(q) != std::string::npos) hit = true;

        if (!hit) {
            for (auto& [style, alias] : m->mNames) {
                std::string n = alias;
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (n.find(q) != std::string::npos) { hit = true; break; }
            }
        }

        if (hit) out.push_back(m);
    }
    return out;
}

// ============================================================
//  Category positions
// ============================================================

void ModernGui::initCategoryPositions()
{
    if (!resetPosition && !catPositions.empty()) return;
    if (resetPosition && NOW - lastReset <= 100) return;

    catPositions.clear();
    static std::vector<std::string> cats = ModuleCategoryNames;

    float totalW = cats.size() * catWidth + (cats.size() - 1) * catGap;
    float startX = (mScreenSize.x - totalW) * 0.5f;

    for (size_t i = 0; i < cats.size(); i++) {
        CategoryPosition p;
        p.x = std::round(startX + i * (catWidth + catGap));
        p.y = 60.f; // Опущено ниже, чтобы не перекрывать поиск
        catPositions.push_back(p);
    }

    resetPosition = false;
}

// ============================================================
//  Background (Убран круг, добавлены летающие сперматозоиды)
// ============================================================

void ModernGui::renderBackground()
{
    auto* dl = ImGui::GetBackgroundDrawList();

    dl->AddRectFilled({0,0}, {mScreenSize.x, mScreenSize.y},
        IM_COL32(0,0,0, (int)(255 * mAnimation * 0.55f)));

    ImRenderUtils::addBlur(ImVec4(0,0,mScreenSize.x,mScreenSize.y),
        mAnimation * mBlur, 0);

    // Анимированная акцентная полоса сверху экрана
    float time = (float)NOW * 0.001f;
    float lineY = 2.f;
    ImColor lineCol = ColorUtils::getThemedColor(time * 20);
    lineCol.Value.w = 0.6f * mAnimation;
    dl->AddRectFilled({0, lineY}, {mScreenSize.x, lineY + 2.f}, lineCol);

    // Рендер хаотичных частиц
    renderParticles();
}

void ModernGui::initParticles()
{
    if (mParticlesInited) return;
    mParticles.clear();
    srand(GetTickCount());
    
    // 50 хаотично летающих частиц
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.x = (float)(rand() % (int)mScreenSize.x);
        p.y = (float)(rand() % (int)mScreenSize.y);
        
        // Случайное направление полета
        float angle = (float)(rand() % 360) * 3.14159f / 180.f;
        p.speed = 20.f + (float)(rand() % 40);
        p.vx = cosf(angle);
        p.vy = sinf(angle);
        
        p.size = 1.f + (float)(rand() % 3);
        p.opacity = 0.03f + (float)(rand() % 5) * 0.01f;
        p.phase = (float)i * 0.7f;
        mParticles.push_back(p);
    }
    mParticlesInited = true;
}

void ModernGui::renderParticles()
{
    if (!mParticlesInited) initParticles();

    ImColor acc = ColorUtils::getThemedColor(0);
    auto*   dl  = ImGui::GetBackgroundDrawList();
    float   dt  = mDeltaTime;
    float   t   = (float)NOW * 0.001f;

    for (auto& p : mParticles) {
        // Движение вперед по вектору
        p.x += p.vx * p.speed * dt;
        p.y += p.vy * p.speed * dt;

        // Извилистое движение (эффект сперматозоида)
        float wobbleX = sinf(t * 3.0f + p.phase) * 1.5f;
        float wobbleY = cosf(t * 2.5f + p.phase) * 1.5f;

        float drawX = p.x + wobbleX;
        float drawY = p.y + wobbleY;

        // Если вылетели за экран — телепортируем на другую сторону
        if (p.x < -20.f) p.x = mScreenSize.x + 10.f;
        if (p.x > mScreenSize.x + 20.f) p.x = -10.f;
        if (p.y < -20.f) p.y = mScreenSize.y + 10.f;
        if (p.y > mScreenSize.y + 20.f) p.y = -10.f;

        float alpha = p.opacity * mAnimation;

        dl->AddCircleFilled({drawX, drawY}, p.size,
            ImColor(acc.Value.x, acc.Value.y, acc.Value.z, alpha), 8);
    }
}

// ============================================================
//  Search Bar (Моя версия)
// ============================================================

void ModernGui::renderSearchBar()
{
    if (!mIsEnabled) return;

    float barW = catWidth;
    float barH = 34.f;
    float barX = mScreenSize.x * 0.5f - barW * 0.5f;
    float barY = 8.f;

    ImVec4 rect = { barX, barY, barX + barW, barY + barH };

    // Glass background + Blur
    ImRenderUtils::addBlur(rect, mAnimation * mBlur, 8.f);
    ImRenderUtils::fillRectangle(rect, darkBlack, mAnimation * 0.9f, 8.f);
    ImRenderUtils::drawRoundRect(rect, ImDrawFlags_RoundCornersAll, 8.f, ImColor(255, 255, 255, 10), mAnimation, 1.f);

    // Animated bottom accent line
    static float searchLineAnim = 0.f;
    searchLineAnim = MathUtils::animate(mSearching ? 1.f : 0.f, searchLineAnim, ImRenderUtils::getDeltaTime() * 10);

    ImColor acc = ColorUtils::getThemedColor(0);
    float lineW = MathUtils::lerp(0, barW - 20.f, searchLineAnim);
    float lineX = rect.x + (barW - lineW) / 2.f;
    ImVec4 lineRect = ImVec4(lineX, rect.w - 1.5f, lineX + lineW, rect.w + 0.5f);
    ImRenderUtils::fillRectangle(lineRect, acc, mAnimation, 2.f);

    // Focus on click
    if (ImGui::IsMouseClicked(0)) mSearching = ImRenderUtils::isMouseOver(rect);

    // ИСПРАВЛЕНО: иконка лупы должна рисоваться иконочным шрифтом
    // tenacity_icons напрямую, а не форсированным Product Sans
    {
        ImFont* iconFont = FontHelper::Fonts.count("tenacity_icons")
            ? FontHelper::Fonts["tenacity_icons"] : ImGui::GetFont();
        ImGui::PushFont(iconFont);
        ImRenderUtils::drawText(ImVec2(rect.x + 10, rect.y + 8), "s", ImColor(255, 255, 255, 90), mTextSize * 0.8f, mAnimation, false);
        ImGui::PopFont();
    }

    // ИСПРАВЛЕНО: текст поиска теперь использует выбранный в Interface шрифт
    std::string display(mSearchBuffer);
    FontHelper::pushPrefFont(true, false, false);

    if (display.empty() && !mSearching) {
        ImRenderUtils::drawText(ImVec2(rect.x + 26, rect.y + 8), "Search...", ImColor(255, 255, 255, 60), mTextSize * 0.9f, mAnimation, false);
    } else {
        ImRenderUtils::drawText(ImVec2(rect.x + 26, rect.y + 8), display, ImColor(255, 255, 255, 230), mTextSize * 0.9f, mAnimation, false);

        if (mSearching) {
            static float blink = 0.f;
            blink += mDeltaTime;
            if (fmodf(blink, 1.0f) < 0.5f) {
                float tw = ImRenderUtils::getTextWidth(const_cast<std::string*>(&display), mTextSize * 0.9f);
                ImGui::GetBackgroundDrawList()->AddLine(
                    ImVec2(rect.x + 26 + tw + 2, rect.y + 8),
                    ImVec2(rect.x + 26 + tw + 2, rect.w - 8),
                    IM_COL32(255, 255, 255, (int)(200 * mAnimation)), 1.5f);
            }
        }
    }
    ImGui::PopFont();
}

// ============================================================
//  Color picker window
// ============================================================

void ModernGui::renderColorPickerWindow()
{
    if (!displayColorPicker || !mIsEnabled) return;

    // ИСПРАВЛЕНО: убрали mForcePSans=true
    FontHelper::pushPrefFont(false, false, false);
    ColorSetting* cs = lastColorSetting;

    ImGui::SetNextWindowPos({mScreenSize.x * 0.5f - 200.f, mScreenSize.y * 0.5f});
    ImGui::SetNextWindowSize({400.f, 400.f});
    ImGui::Begin("Color Picker", &displayColorPicker,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    {
        ImVec4 col = cs->getAsImColor().Value;
        ImGui::ColorPicker4("Color", cs->mValue,
            ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha);
        if (ImGui::Button("Close")) {
            cs->setFromImColor(ImColor(col));
            displayColorPicker = false;
        }
    }
    ImGui::End();
    ImGui::PopFont();

    if (ImGui::IsMouseClicked(0) && !ImRenderUtils::isMouseOver(
        {mScreenSize.x*0.5f-200, mScreenSize.y*0.5f,
         mScreenSize.x*0.5f+200, mScreenSize.y*0.5f+400}))
        displayColorPicker = false;
}

// ============================================================
//  Ripples
// ============================================================

void ModernGui::updateRipples(float dt)
{
    for (auto& r : mRipples) {
        r.radius += dt * 200.f;
        r.alpha  -= dt * 1.8f;
    }
    mRipples.erase(
        std::remove_if(mRipples.begin(), mRipples.end(),
            [](const GuiRipple& r){ return r.alpha <= 0.f; }),
        mRipples.end());
}

// ============================================================
//  Toggle switch
// ============================================================

void ModernGui::drawToggleSwitch(ImVec2 center, float boolScale,
                                  const ImColor& accent, float alpha)
{
    float tw = 28.f, th = 15.f;
    ImVec2 tMin = { center.x - tw*0.5f, center.y - th*0.5f };
    ImVec2 tMax = { center.x + tw*0.5f, center.y + th*0.5f };

    float r = MathUtils::lerp(0.15f, accent.Value.x, boolScale);
    float g = MathUtils::lerp(0.15f, accent.Value.y, boolScale);
    float b = MathUtils::lerp(0.15f, accent.Value.z, boolScale);

    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(tMin, tMax, ImColor(r, g, b, alpha * 0.9f), th * 0.5f);

    if (boolScale > 0.01f) {
        dl->AddShadowRect(tMin, tMax,
            ImColor(accent.Value.x, accent.Value.y, accent.Value.z, boolScale * 0.3f * alpha),
            12.f, {0,0}, 0, th * 0.5f);
    }

    float thumbX = MathUtils::lerp(tMin.x + 2.f + 6.5f, tMax.x - 2.f - 6.5f, boolScale);
    dl->AddCircleFilled({thumbX, center.y}, 6.5f, ImColor(1.f, 1.f, 1.f, alpha), 16);

    if (boolScale > 0.01f)
        dl->AddShadowCircle({thumbX, center.y}, 6.5f,
            ImColor(accent.Value.x, accent.Value.y, accent.Value.z, boolScale * 0.4f * alpha),
            10.f, {0,0}, 0, 6.5f);
}

// ============================================================
//  Main render
// ============================================================

void ModernGui::render(float animation, float inScale, int& scrollDirection,
                        float blur, float midclickRounding, bool isPressingShift)
{
    auto  clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    bool  isEnabled = clickGui->mEnabled;

    cacheFrameData(animation, inScale, blur, midclickRounding, isPressingShift, isEnabled);

    // ИСПРАВЛЕНО: убрали mForcePSans=true — теперь ClickGui использует
    // тот шрифт, который выбран в Interface -> Font (с поддержкой кириллицы)
    FontHelper::pushPrefFont(true, false, false);

    initCategoryPositions();
    updateRipples(mDeltaTime);

    renderBackground();
    renderParticles();
    renderSearchBar();
    renderColorPickerWindow();

    if (!mIsEnabled) displayColorPicker = false;
    if (!catPositions.empty()) renderCategories(scrollDirection);
    renderTooltip();

    if (isEnabled) scrollDirection = 0;

    ImGui::PopFont();
}

// ============================================================
//  Categories
// ============================================================

void ModernGui::renderCategories(int& scrollDirection)
{
    static std::vector<std::string> cats = ModuleCategoryNames;
    for (size_t i = 0; i < cats.size(); i++)
        renderCategory(i, scrollDirection);

    handleBinding();
    handleBoolBinding();
}

void ModernGui::renderCategory(size_t index, int& scrollDirection)
{
    auto mods = getFilteredModules(index);
    if (isSearchActive() && mods.empty()) return;

    auto& pos = catPositions[index];

    float targetExpand = pos.isExtended ? 1.f : 0.f;
    pos.expandAnim = MathUtils::animate(targetExpand, pos.expandAnim, mDeltaTime * 12.f);
    pos.expandAnim = MathUtils::clamp(pos.expandAnim, 0.f, 1.f);

    ImVec4 catRect = { pos.x, pos.y, pos.x + catWidth, pos.y + catHeight };
    catRect = scaleToPoint(catRect,
        {mScreenSize.x*0.5f, mScreenSize.y*0.5f, mScreenSize.x*0.5f, mScreenSize.y*0.5f},
        mInScale);

    float contentH = 0.f;
    for (auto& mod : mods) {
        contentH += modHeight;
        if (mod->cAnim > 0.001f) {
            for (auto* s : mod->mSettings) {
                if (!s->mIsVisible()) continue;
                switch (s->mType) {
                    case SettingType::Bool:
                    case SettingType::Number:
                    case SettingType::Color:
                        contentH += modHeight * mod->cAnim; break;
                    case SettingType::Enum: {
                        contentH += modHeight * mod->cAnim;
                        auto* es = reinterpret_cast<EnumSetting*>(s);
                        contentH += modHeight * es->mValues.size() * s->enumSlide * mod->cAnim;
                        break;
                    }
                    default: break;
                }
            }
        }
    }

    // Убрана граница 400.f. Теперь панель тянется до низа экрана (с отступом 10px)
    float maxVisibleH = mScreenSize.y - (pos.y + catHeight) - 10.f;
    float maxScrollH = std::max(0.f, contentH - maxVisibleH);

    float visH = std::min(contentH, maxVisibleH) * pos.expandAnim;
    ImVec4 catWindow = { pos.x, pos.y, pos.x + catWidth, pos.y + catHeight + visH };
    catWindow = scaleToPoint(catWindow,
        {mScreenSize.x*0.5f, mScreenSize.y*0.5f, mScreenSize.x*0.5f, mScreenSize.y*0.5f},
        mInScale);

    if (ImRenderUtils::isMouseOver(catWindow) && pos.isExtended) {
        if (scrollDirection > 0)
            pos.scrollEase = std::min(pos.scrollEase + catHeight, maxScrollH);
        else if (scrollDirection < 0)
            pos.scrollEase = std::max(pos.scrollEase - catHeight, 0.f);
        scrollDirection = 0;
    }

    pos.yOffset = MathUtils::animate(pos.scrollEase, pos.yOffset, mDeltaTime * 10.5f);

    ImVec4 clipRect = { catRect.x, catRect.w, catRect.z, catRect.w + visH };
    ImGui::GetBackgroundDrawList()->PushClipRect(
        {clipRect.x, clipRect.y}, {clipRect.z, clipRect.w}, true);

    renderCategoryModules(index, catRect);

    ImGui::GetBackgroundDrawList()->PopClipRect();

    renderCategoryHeader(index, catRect);
    handleCategoryDragging(index, catRect);
}

// ============================================================
//  Category header  (Исправлены отступы иконок)
// ============================================================

void ModernGui::renderCategoryHeader(size_t index, const ImVec4& catRect)
{
    static std::vector<std::string> cats = ModuleCategoryNames;
    auto& pos = catPositions[index];

    std::string name = mLowercase ? StringUtils::toLower(cats[index]) : cats[index];

    if (ImRenderUtils::isMouseOver(catRect) && ImGui::IsMouseClicked(1))
        pos.isExtended = !pos.isExtended;

    auto* dl = ImGui::GetBackgroundDrawList();

    float rounding = 8.f;
    ImDrawFlags flags = pos.expandAnim > 0.01f ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll;

    dl->AddRectFilled({catRect.x, catRect.y}, {catRect.z, catRect.w},
        IM_COL32(14,14,22, (int)(230 * mAnimation)), rounding, flags);
    dl->AddRect({catRect.x, catRect.y}, {catRect.z, catRect.w},
        IM_COL32(255,255,255, (int)(10 * mAnimation)), rounding, flags, 1.f);

    ImColor acc = ColorUtils::getThemedColor(index * 20);
    float   breath = 0.35f + 0.15f * sinf((float)NOW * 0.001f * 3.5f);
    acc.Value.w = breath * mAnimation;
    dl->AddRectFilled({catRect.x + 12.f, catRect.y + 0.5f}, {catRect.z - 12.f, catRect.y + 2.f}, ImColor(acc), 2.f);

    FontHelper::pushPrefFont(true, true, true);
    
    std::string iconStr = "B";
    if (StringUtils::equalsIgnoreCase(name, "Combat")) iconStr = "c";
    else if (StringUtils::equalsIgnoreCase(name, "Movement")) iconStr = "f";
    else if (StringUtils::equalsIgnoreCase(name, "Visual")) iconStr = "d";
    else if (StringUtils::equalsIgnoreCase(name, "Player")) iconStr = "e";
    else if (StringUtils::equalsIgnoreCase(name, "Misc")) iconStr = "a";

    float nameY = catRect.y + (catRect.w - catRect.y - mTextHeight) * 0.5f;

    ImGui::PushFont(FontHelper::Fonts["tenacity_icons_large"]);
    float iconX = catRect.x + 10.f;
    ImRenderUtils::drawText({iconX, nameY}, iconStr,
        ImColor(1.f, 1.f, 1.f, 0.85f * mAnimation), mTextSize * 1.15f, mAnimation, false);
    ImGui::PopFont();

    float iconW = ImRenderUtils::getTextWidth(&iconStr, mTextSize * 1.15f);
    float nameX = iconX + iconW + 12.f; // УВЕЛИЧЕН ОТСТУП (было 5, потом 10, теперь 12)

    ImRenderUtils::drawText({nameX, nameY}, name,
        ImColor(1.f, 1.f, 1.f, 0.85f * mAnimation), mTextSize * 1.05f, mAnimation, false);
    
    ImGui::PopFont();

    // ЦИФРЫ УДАЛЕНЫ (блок countStr полностью убран)

    std::string arrow = pos.isExtended ? "v" : ">";
    float arrW = ImRenderUtils::getTextWidth(&arrow, mTextSize * 0.8f);
    ImRenderUtils::drawText({catRect.z - arrW - 8.f, nameY},
        arrow, ImColor(1.f,1.f,1.f, 0.3f * mAnimation), mTextSize * 0.8f, mAnimation, false);

    pos.x = std::clamp(pos.x, 0.f, mScreenSize.x - catWidth);
    pos.y = std::clamp(pos.y, 0.f, mScreenSize.y - catHeight);
}

// ============================================================
//  Module list & Single module row (Без изменений)
// ============================================================

void ModernGui::renderCategoryModules(size_t catIndex, const ImVec4& catRect)
{
    auto mods    = getFilteredModules(catIndex);
    float moduleY = -catPositions[catIndex].yOffset;
    bool  toggled = false;
    int   idx     = 0;

    for (auto& mod : mods) {
        bool isLast = (idx == (int)mods.size() - 1);

        ImVec4 modRect = {
            catPositions[catIndex].x,
            catPositions[catIndex].y + catHeight + moduleY,
            catPositions[catIndex].x + catWidth,
            catPositions[catIndex].y + catHeight + moduleY + modHeight
        };
        modRect = scaleToPoint(modRect,
            {mScreenSize.x*0.5f, mScreenSize.y*0.5f,
             mScreenSize.x*0.5f, mScreenSize.y*0.5f}, mInScale);
        modRect.y = std::floor(modRect.y);
        modRect.x = std::floor(modRect.x);

        float tgt = mod->showSettings ? 1.f : 0.f;
        mod->cAnim = MathUtils::animate(tgt, mod->cAnim, mDeltaTime * 12.5f);
        mod->cAnim = MathUtils::clamp(mod->cAnim, 0.f, 1.f);

        if (mod->cAnim > 0.001f)
            renderModuleSettings(catIndex, mod, modRect, catRect, moduleY, isLast);

        renderModule(catIndex, catRect, modRect, mod, isLast, moduleY, toggled);

        moduleY += modHeight;
        idx++;
    }
}

void ModernGui::renderModule(size_t catIndex, const ImVec4& catRect,
                              const ImVec4& modRect,
                              const std::shared_ptr<Module>& mod,
                              bool isLast, float& moduleY, bool& moduleToggled)
{
    if (modRect.y <= catRect.y + 0.5f) return;

    auto*  dl      = ImGui::GetBackgroundDrawList();
    float  modW    = modRect.z - modRect.x;
    float  modH    = modRect.w - modRect.y;
    ImVec2 center  = { modRect.x + modW*0.5f, modRect.y + modH*0.5f };

    float rounding = 0.f;
    if (isLast) rounding = 8.f * (1.f - mod->cAnim);
    ImDrawFlags botFlags = ImDrawFlags_RoundCornersBottom;

    dl->AddRectFilled({modRect.x, modRect.y}, {modRect.z, modRect.w},
        IM_COL32(14,14,22, (int)(230 * mAnimation)), rounding, botFlags);
    dl->AddLine({modRect.x, modRect.w - 0.5f}, {modRect.z, modRect.w - 0.5f},
        IM_COL32(255,255,255, (int)(6 * mAnimation)));

    if (ImRenderUtils::isMouseOver(modRect))
        mod->hoverAnim = MathUtils::animate(1.f, mod->hoverAnim, mDeltaTime * 10.f);
    else
        mod->hoverAnim = MathUtils::animate(0.f, mod->hoverAnim, mDeltaTime * 10.f);

    if (mod->hoverAnim > 0.001f)
        dl->AddRectFilled({modRect.x, modRect.y}, {modRect.z, modRect.w},
            IM_COL32(255,255,255, (int)(9 * mod->hoverAnim * mAnimation)),
            rounding, botFlags);

    mod->cScale = MathUtils::animate(mod->mEnabled ? 1.f : 0.f,
        mod->cScale, mDeltaTime * 10.f);

    ImColor modColor = ColorUtils::getThemedColor((int)moduleY * 2); // Получаем цвет модуля

    if (mod->cScale > 0.001f) {
        ImColor acc1 = modColor;
        ImColor acc2 = ColorUtils::getThemedColor((int)(moduleY * 2) + 60);

        float fillW = modW * mod->cScale;
        ImVec4 fillRect = { modRect.x, modRect.y, modRect.x + fillW, modRect.w };
        ImRenderUtils::fillRoundedGradientRectangle(fillRect,
            ImColor(acc1.Value.x, acc1.Value.y, acc1.Value.z, 0.12f * mAnimation),
            ImColor(acc2.Value.x, acc2.Value.y, acc2.Value.z, 0.04f * mAnimation),
            0.f, mAnimation * mod->cScale, mAnimation * mod->cScale);
    }

    if (mod->togglePulse > 0.f) {
        mod->togglePulse = MathUtils::animate(0.f, mod->togglePulse, mDeltaTime * 3.f);
        ImColor acc = modColor;
        acc.Value.w = mod->togglePulse * 0.15f * mAnimation;
        dl->AddRectFilled({modRect.x, modRect.y}, {modRect.z, modRect.w},
            ImColor(acc), rounding, botFlags);
    }

    {
        ImColor acc = modColor;
        float   barH = (modRect.w - modRect.y - 12.f) * mod->cScale;
        float   barY0 = modRect.y + 6.f + (modRect.w - modRect.y - 12.f - barH) * 0.5f;

        if (mod->cScale > 0.001f) {
            dl->AddRectFilled(
                { modRect.x, barY0 }, { modRect.x + 2.5f, barY0 + barH },
                ImColor(acc.Value.x, acc.Value.y, acc.Value.z, mAnimation), 2.f);
            dl->AddShadowRect(
                { modRect.x, barY0 }, { modRect.x + 2.5f, barY0 + barH },
                ImColor(acc.Value.x, acc.Value.y, acc.Value.z, 0.5f * mAnimation),
                6.f, {0,0}, 0, 2.f);
        }
    }

    dl->PushClipRect({modRect.x, modRect.y}, {modRect.z, modRect.w}, true);
    for (auto& rp : mRipples) {
        if (rp.alpha > 0.f) {
            ImColor acc = modColor;
            dl->AddCircleFilled({rp.x, rp.y}, rp.radius,
                ImColor(acc.Value.x, acc.Value.y, acc.Value.z,
                    rp.alpha * 0.3f * mAnimation), 32);
        }
    }
    dl->PopClipRect();

    // ИСПРАВЛЕНО: убрали mForcePSans=true — название модуля теперь
    // тоже рисуется выбранным в Interface шрифтом (с кириллицей)
    FontHelper::pushPrefFont(true, false, false);
    std::string modName = mod->getName();
    float nameX = modRect.x + 12.f;
    float nameY = center.y - mTextHeight * 0.5f;

    // ====================================================================
    // ФИКС БЕЛОГО ТЕКСТА: Рассчитываем контрастность цвета темы
    // ====================================================================
    ImColor nameCol;
    if (mod->mEnabled) {
        // Считаем яркость цвета (Luminance)
        float lum = modColor.Value.x * 0.299f + modColor.Value.y * 0.587f + modColor.Value.z * 0.114f;
        // Если цвет светлый (тема Ocean и т.д.) — делаем текст черным, иначе белым
        if (lum > 0.5f) {
            nameCol = ImColor(0.f, 0.f, 0.f, mAnimation);
        } else {
            nameCol = ImColor(1.f, 1.f, 1.f, mAnimation);
        }
    } else {
        nameCol = ImColor(1.f, 1.f, 1.f, 0.55f * mAnimation);
    }

    ImRenderUtils::drawText({nameX, nameY}, modName, nameCol, mTextSize, mAnimation, false);
    ImGui::PopFont();

    if (!mod->mSettings.empty()) {
        std::string arr = "v";
        float arrW = ImRenderUtils::getTextWidth(&arr, mTextSize * 0.75f);
        ImColor arrCol = mod->showSettings ? modColor : ImColor(1.f,1.f,1.f, 0.2f);
        arrCol.Value.w *= mAnimation;

        ImRenderUtils::drawText(
            {modRect.z - arrW - 8.f, nameY + 1.f},
            arr, arrCol, mTextSize * 0.75f, mAnimation, false);
    }

    if (ImRenderUtils::isMouseOver(modRect) && catPositions[catIndex].isExtended && mIsEnabled) {
        mTooltip = mod->mDescription;
        if (ImGui::IsMouseClicked(0) && !displayColorPicker && !moduleToggled) {
            mod->toggle();
            mod->togglePulse = 1.f;
            GuiRipple rp;
            rp.x = mMousePos.x; rp.y = mMousePos.y; rp.radius = 4.f; rp.alpha = 1.f; rp.maxRadius = 120.f;
            mRipples.push_back(rp);
            ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
            moduleToggled = true;
        }
        else if (ImGui::IsMouseClicked(1) && !displayColorPicker) {
            if (!mod->mSettings.empty()) mod->showSettings = !mod->showSettings;
        }
        else if (ImGui::IsMouseClicked(2) && !displayColorPicker) {
            lastMod = mod; isBinding = true;
            ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
        }
    }
}

// ============================================================
//  Settings dispatcher (Без изменений)
// ============================================================

void ModernGui::renderModuleSettings(size_t catIndex, const std::shared_ptr<Module>& mod,
                                      const ImVec4& modRect, const ImVec4& catRect, float& moduleY, bool isLast)
{
    int sIdx = 0; int visible = 0;
    for (auto* s : mod->mSettings) if (s->mIsVisible()) visible++;

    for (auto* s : mod->mSettings) {
        if (!s->mIsVisible()) { s->sliderEase = 0; s->enumSlide = 0; continue; }
        bool isLastSetting = (sIdx == visible - 1);
        float radius  = (isLast && isLastSetting) ? 8.f : 0.f;
        float padding = isLastSetting ? -2.f * mAnimation : 0.f;
        ImColor acc = ColorUtils::getThemedColor((int)moduleY * 2); acc.Value.w = mAnimation;

        switch (s->mType) {
            case SettingType::Bool: renderBoolSetting(catIndex, reinterpret_cast<BoolSetting*>(s), modRect, catRect, moduleY, radius, acc, padding, mod->cAnim); break;
            case SettingType::Enum: renderEnumSetting(catIndex, reinterpret_cast<EnumSetting*>(s), modRect, catRect, moduleY, radius, acc, padding, mod->cAnim); break;
            case SettingType::Number: renderNumberSetting(catIndex, reinterpret_cast<NumberSetting*>(s), modRect, catRect, moduleY, radius, acc, padding, mod->cAnim); break;
            case SettingType::Color: renderColorSetting(catIndex, reinterpret_cast<ColorSetting*>(s), modRect, catRect, moduleY, radius, acc, padding, mod->cAnim); break;
            default: break;
        }
        sIdx++;
    }
}

// ============================================================
//  Bool & Enum & Color settings (Оставлены как были, они норм)
// ============================================================

void ModernGui::renderBoolSetting(size_t catIndex, BoolSetting* s, const ImVec4& modRect, const ImVec4& catRect, float& moduleY, float radius, const ImColor& accent, float padding, float cAnim) {
    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);
    ImVec4 rect = { modRect.x, catRect.w + moduleY + padding, modRect.z, catRect.w + moduleY + modHeight };
    rect = scaleToPoint(rect, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
    rect.y = std::floor(rect.y); if (rect.y < modRect.y) rect.y = modRect.y; if (rect.y <= catRect.y + 0.5f) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, IM_COL32(8,8,14, (int)(200 * mAnimation)), radius, ImDrawFlags_RoundCornersBottom);
    dl->AddLine({rect.x, rect.y}, {rect.z, rect.y}, IM_COL32(255,255,255, (int)(6 * mAnimation)));

    std::string name = mLowercase ? StringUtils::toLower(s->mName) : s->mName;
    float nameY = rect.y + ((rect.w - rect.y) - mTextHeight) * 0.5f;
    ImRenderUtils::drawText({rect.x + 12.f, nameY}, name, ImColor(1.f,1.f,1.f, 0.7f * mAnimation), mTextSize, mAnimation, false);

    s->hoverAnim = MathUtils::animate(ImRenderUtils::isMouseOver(rect) ? 1.f : 0.f, s->hoverAnim, mDeltaTime * 10.f);
    if (s->hoverAnim > 0.001f) dl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, IM_COL32(255,255,255, (int)(6 * s->hoverAnim * mAnimation)), radius, ImDrawFlags_RoundCornersBottom);

    s->boolScale = MathUtils::animate(s->mValue ? 1.f : 0.f, s->boolScale, mDeltaTime * 10.f);
    drawToggleSwitch({rect.z - 20.f, rect.y + (rect.w - rect.y) * 0.5f}, s->boolScale, accent, mAnimation);

    if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended) {
        mTooltip = s->mDescription;
        if (ImGui::IsMouseClicked(0) && !displayColorPicker) s->mValue = !s->mValue;
        if (ImGui::IsMouseClicked(2) && !displayColorPicker) { lastBoolSetting = s; isBoolSettingBinding = true; ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f); }
    }
}

void ModernGui::renderEnumSetting(size_t catIndex, EnumSetting* s, const ImVec4& modRect, const ImVec4& catRect, float& moduleY, float radius, const ImColor& accent, float padding, float cAnim) {
    std::string name = mLowercase ? StringUtils::toLower(s->mName) : s->mName;
    std::vector<std::string> vals = s->mValues;
    if (mLowercase) for (auto& v : vals) v = StringUtils::toLower(v);
    int numVals = (int)vals.size();
    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);
    ImVec4 rect = { modRect.x, catRect.w + moduleY + padding, modRect.z, catRect.w + moduleY + modHeight };
    rect = scaleToPoint(rect, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
    rect.y = std::floor(rect.y); if (rect.y < modRect.y) rect.y = modRect.y;

    float tgt = s->enumExtended ? 1.f : 0.f;
    s->enumSlide = MathUtils::animate(tgt, s->enumSlide, mDeltaTime * 10.f);
    s->enumSlide = MathUtils::clamp(s->enumSlide, 0.f, 1.f);

    auto* dl = ImGui::GetBackgroundDrawList();

    if (s->enumSlide > 0.001f) {
        for (int j = 0; j < numVals; j++) {
            moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, s->enumSlide);
            ImVec4 r2 = { modRect.x, catRect.w + moduleY + padding, modRect.z, catRect.w + moduleY + modHeight };
            r2 = scaleToPoint(r2, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
            r2.y = std::floor(r2.y); if (r2.y < modRect.y) r2.y = modRect.y; if (r2.y <= catRect.y + 0.5f) continue;

            dl->AddRectFilled({r2.x, r2.y}, {r2.z, r2.w}, IM_COL32(5,5,10, (int)(230 * mAnimation * s->enumSlide)), 0.f, ImDrawFlags_RoundCornersBottom);
            if (s->mValue == j) {
                dl->AddRectFilled({r2.x, r2.y}, {r2.x + 2.f, r2.w}, ImColor(accent.Value.x, accent.Value.y, accent.Value.z, mAnimation * s->enumSlide));
                dl->AddRectFilled({r2.x, r2.y}, {r2.z, r2.w}, ImColor(accent.Value.x, accent.Value.y, accent.Value.z, 0.1f * mAnimation * s->enumSlide), 0.f, ImDrawFlags_RoundCornersBottom);
            }
            float cY = r2.y + ((r2.w - r2.y) - mTextHeight) * 0.5f;
            ImColor textCol = (s->mValue == j) ? ImColor(1.f,1.f,1.f, mAnimation * s->enumSlide) : ImColor(1.f,1.f,1.f, 0.5f * mAnimation * s->enumSlide);
            ImRenderUtils::drawText({r2.x + 12.f, cY}, vals[j], textCol, mTextSize, mAnimation * s->enumSlide, false);
            if (ImRenderUtils::isMouseOver(r2) && ImGui::IsMouseClicked(0) && mIsEnabled && !displayColorPicker) s->mValue = j;
        }
    }

    if (rect.y <= catRect.y + 0.5f) return;
    dl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, IM_COL32(8,8,14, (int)(200 * mAnimation)), radius, ImDrawFlags_RoundCornersBottom);
    dl->AddLine({rect.x, rect.y}, {rect.z, rect.y}, IM_COL32(255,255,255, (int)(6 * mAnimation)));

    float cY = rect.y + ((rect.w - rect.y) - mTextHeight) * 0.5f;
    ImRenderUtils::drawText({rect.x + 12.f, cY}, name, ImColor(1.f,1.f,1.f, 0.7f * mAnimation), mTextSize, mAnimation, false);
    std::string curVal = vals[s->mValue]; float valW = ImRenderUtils::getTextWidth(&curVal, mTextSize);
    ImColor valCol = ColorUtils::getThemedColor((int)moduleY * 2); valCol.Value.w = 0.85f * mAnimation;
    ImRenderUtils::drawText({rect.z - valW - 8.f, cY}, curVal, valCol, mTextSize, mAnimation, false);

    if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended) {
        mTooltip = s->mDescription;
        if (ImGui::IsMouseClicked(0) && !displayColorPicker) s->mValue = (s->mValue + 1) % numVals;
        else if (ImGui::IsMouseClicked(1) && !displayColorPicker) s->enumExtended = !s->enumExtended;
    }
    if (s->enumSlide > 0.001f && rect.y > catRect.y - modHeight) ImRenderUtils::fillGradientOpaqueRectangle({rect.x, rect.w, rect.z, rect.w + 8.f * s->enumSlide * mAnimation}, ImColor(0,0,0), ImColor(0,0,0), 0.f, 0.35f * mAnimation);
}

void ModernGui::renderColorSetting(size_t catIndex, ColorSetting* s, const ImVec4& modRect, const ImVec4& catRect, float& moduleY, float radius, const ImColor& accent, float padding, float cAnim) {
    std::string name = mLowercase ? StringUtils::toLower(s->mName) : s->mName;
    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);
    ImVec4 rect = { modRect.x, catRect.w + moduleY + padding, modRect.z, catRect.w + moduleY + modHeight };
    rect = scaleToPoint(rect, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
    rect.y = std::floor(rect.y); if (rect.y < modRect.y) rect.y = modRect.y; if (rect.y <= catRect.y + 0.5f) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled({rect.x, rect.y}, {rect.z, rect.w}, IM_COL32(8,8,14, (int)(200 * mAnimation)), radius, ImDrawFlags_RoundCornersBottom);
    dl->AddLine({rect.x, rect.y}, {rect.z, rect.y}, IM_COL32(255,255,255, (int)(6 * mAnimation)));

    float cY = rect.y + ((rect.w - rect.y) - mTextHeight) * 0.5f;
    ImRenderUtils::drawText({rect.x + 12.f, cY}, name, ImColor(1.f,1.f,1.f, 0.7f * mAnimation), mTextSize, mAnimation, false);

    ImVec4 swatchR = { rect.z - 22.f, rect.y + 5.f, rect.z - 4.f, rect.w - 5.f };
    ImColor swCol = s->getAsImColor();
    dl->AddRectFilled({swatchR.x, swatchR.y}, {swatchR.z, swatchR.w}, swCol, 3.f);
    dl->AddRect({swatchR.x, swatchR.y}, {swatchR.z, swatchR.w}, IM_COL32(255,255,255, (int)(26 * mAnimation)), 3.f, 0, 1.f);
    dl->AddShadowRect({swatchR.x, swatchR.y}, {swatchR.z, swatchR.w}, ImColor(swCol.Value.x, swCol.Value.y, swCol.Value.z, 0.4f * mAnimation), 8.f, {0,0}, 0, 3.f);

    if (ImRenderUtils::isMouseOver(rect) && mIsEnabled && catPositions[catIndex].isExtended) {
        mTooltip = s->mDescription;
        if (ImGui::IsMouseClicked(0) && !displayColorPicker) { displayColorPicker = !displayColorPicker; lastColorSetting = s; }
    }
}

// ============================================================
//  Number setting (Slider с тематическим цветом и свечением)
// ============================================================

void ModernGui::renderNumberSetting(size_t catIndex, NumberSetting* s,
                                     const ImVec4& modRect, const ImVec4& catRect,
                                     float& moduleY, float radius,
                                     const ImColor& accent, float padding, float cAnim)
{
    const float val = s->mValue;
    const float mn  = s->mMin;
    const float mx  = s->mMax;
    float pct = (mx - mn) > 0.f ? (val - mn) / (mx - mn) : 0.f;

    char buf[12]; sprintf_s(buf,12,"%.2f",val);
    std::string valStr = buf;
    std::string name   = mLowercase ? StringUtils::toLower(s->mName) : s->mName;

    moduleY = MathUtils::lerp(moduleY, moduleY + modHeight, cAnim);

    ImVec4 bgRect = { modRect.x, catRect.w + moduleY, modRect.z, catRect.w + moduleY + modHeight };
    bgRect = scaleToPoint(bgRect, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
    bgRect.y = std::floor(bgRect.y); if (bgRect.y < modRect.y) bgRect.y = modRect.y;
    if (bgRect.y <= catRect.y + 0.5f) return;

    ImVec4 trackRect = { bgRect.x + 10.f, bgRect.y + padding, bgRect.z - 10.f, bgRect.y + modHeight };
    trackRect = scaleToPoint(trackRect, {modRect.x, mScreenSize.y*0.5f, modRect.z, mScreenSize.y*0.5f}, mInScale);
    trackRect.y = std::floor(trackRect.y); if (trackRect.y < modRect.y) trackRect.y = modRect.y;

    auto* dl = ImGui::GetBackgroundDrawList();

    dl->AddRectFilled({bgRect.x, bgRect.y}, {bgRect.z, bgRect.w}, IM_COL32(8,8,14, (int)(200 * mAnimation)), radius, ImDrawFlags_RoundCornersBottom);
    dl->AddLine({bgRect.x, bgRect.y}, {bgRect.z, bgRect.y}, IM_COL32(255,255,255, (int)(6 * mAnimation)));

    float labelY = bgRect.y + 3.f;
    ImRenderUtils::drawText({bgRect.x + 10.f, labelY}, name, ImColor(1.f,1.f,1.f, 0.7f * mAnimation), mTextSize, mAnimation, false);
    float valW = ImRenderUtils::getTextWidth(&valStr, mTextSize);
    ImRenderUtils::drawText({bgRect.z - valW - 6.f, labelY}, valStr, ImColor(1.f,1.f,1.f, 0.4f * mAnimation), mTextSize, mAnimation, false);

    float targetEase = pct * (trackRect.z - trackRect.x);
    s->sliderEase = MathUtils::animate(targetEase, s->sliderEase, mDeltaTime * 10.f);
    s->sliderEase = std::clamp(s->sliderEase, 0.f, trackRect.z - trackRect.x);

    float trackY = trackRect.w - 6.5f * mInScale;

    // Трек (задний фон полоски)
    dl->AddRectFilled({trackRect.x, trackY - 2.f}, {trackRect.z, trackY + 2.f}, IM_COL32(255,255,255, (int)(18 * mAnimation)), 4.f);

    // Тематическая закрашенная часть ползунка (с акцентным цветом)
    float filledEndX = trackRect.x + s->sliderEase * mInScale;
    dl->AddRectFilled({trackRect.x, trackY - 2.f}, {filledEndX, trackY + 2.f}, ImColor(accent.Value.x, accent.Value.y, accent.Value.z, mAnimation), 4.f);
    
    // Свечение (Glow) закрашенной части
    dl->AddShadowRect({trackRect.x, trackY - 2.f}, {filledEndX, trackY + 2.f}, 
        ImColor(accent.Value.x, accent.Value.y, accent.Value.z, 0.35f * mAnimation), 10.f, {0,0}, 0, 4.f);

    // Анимация кружка при клике
    bool hovering = ImRenderUtils::isMouseOver(trackRect);
    s->clickScale = MathUtils::animate((hovering && ImGui::IsMouseDown(0)) ? 0.75f : 1.f, s->clickScale, mDeltaTime * 10.f);

    // Кружок (Ползунок) с тематическим свечением
    float knobX = trackRect.x + s->sliderEase * mInScale;
    float knobR = 5.5f * s->clickScale * mAnimation;
    dl->AddCircleFilled({knobX, trackY}, knobR, ImColor(accent.Value.x, accent.Value.y, accent.Value.z, mAnimation), 16);
    dl->AddShadowCircle({knobX, trackY}, knobR, ImColor(accent.Value.x, accent.Value.y, accent.Value.z, 0.5f * mAnimation), 10.f, {0,0}, 0, knobR);
    
    // Внутренний белый кружок для красоты
    dl->AddCircleFilled({knobX, trackY}, knobR * 0.45f, IM_COL32(255, 255, 255, (int)(220 * mAnimation)), 12);

    // Логика перетаскивания (оставлена оригинальная от другого ИИ)
    if (ImRenderUtils::isMouseOver(trackRect) && mIsEnabled && catPositions[catIndex].isExtended) {
        mTooltip = s->mDescription;
        if (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(2)) { s->isDragging = true; lastDraggedSetting = s; }
    }

    if (ImGui::IsMouseDown(0) && s->isDragging && mIsEnabled) {
        if (lastDraggedSetting != s) { s->isDragging = false; }
        else {
            float frac = std::clamp((mMousePos.x - trackRect.x) / (trackRect.z - trackRect.x), 0.f, 1.f);
            s->setValue(mn + frac * (mx - mn));
        }
    } else if (ImGui::IsMouseDown(2) && s->isDragging && mIsEnabled) {
        if (lastDraggedSetting != s) { s->isDragging = false; }
        else {
            float frac = std::clamp((mMousePos.x - trackRect.x) / (trackRect.z - trackRect.x), 0.f, 1.f);
            float raw = mn + frac * (mx - mn);
            raw = std::round(raw / mMidclickRounding) * mMidclickRounding;
            s->mValue = std::clamp(raw, mn, mx);
        }
    } else {
        s->isDragging = false;
    }
}

// ============================================================
//  Tooltip, Dragging, Binding, Resize (Без изменений)
// ============================================================

void ModernGui::renderTooltip() {
    if (mTooltip.empty()) return;
    float textW = ImRenderUtils::getTextWidth(&mTooltip, mTextSize * 0.8f);
    float textH = ImGui::GetFont()->CalcTextSizeA(mTextSize * 14.4f, FLT_MAX, 0, mTooltip.c_str()).y;
    float pad = 6.f, offX = 10.f;
    ImVec4 tr = { mMousePos.x + offX, mMousePos.y - textH * 0.5f - pad, mMousePos.x + offX + textW + pad * 2.f, mMousePos.y + textH * 0.5f + pad };
    tr = scaleToPoint(tr, {mScreenSize.x*0.5f, mScreenSize.y*0.5f, mScreenSize.x*0.5f, mScreenSize.y*0.5f}, mInScale);
    static float tooltipAlpha = 1.f;
    tooltipAlpha = MathUtils::animate((ImGui::IsMouseDown(0) || ImGui::IsMouseDown(2)) ? 0.f : 1.f, tooltipAlpha, mDeltaTime * 10.f);
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled({tr.x, tr.y}, {tr.z, tr.w}, IM_COL32(10,10,16, (int)(242 * mAnimation * tooltipAlpha)), 6.f);
    dl->AddRect({tr.x, tr.y}, {tr.z, tr.w}, IM_COL32(255,255,255, (int)(13 * mAnimation * tooltipAlpha)), 6.f, 0, 1.f);
    ImColor acc = ColorUtils::getThemedColor(0); acc.Value.w = 0.05f * mAnimation * tooltipAlpha;
    dl->AddRectFilled({tr.x, tr.y}, {tr.z, tr.w}, ImColor(acc), 6.f);
    ImRenderUtils::drawText({tr.x + pad, tr.y + pad}, mTooltip, ImColor(1.f,1.f,1.f, 0.75f * mAnimation * tooltipAlpha), mTextSize * 0.8f, mAnimation * tooltipAlpha, false, 0, dl);
}

void ModernGui::handleCategoryDragging(size_t index, const ImVec4& catRect) {
    static bool dragging = false; static ImVec2 dragOff = {};
    if (catPositions[index].isDragging) {
        if (ImGui::IsMouseDown(0)) { if (!dragging) { dragOff = { mMousePos.x - catRect.x, mMousePos.y - catRect.y }; dragging = true; } catPositions[index].x = std::clamp(mMousePos.x - dragOff.x, 0.f, mScreenSize.x - catWidth); catPositions[index].y = std::clamp(mMousePos.y - dragOff.y, 0.f, mScreenSize.y - catHeight); catPositions[index].x = std::round(catPositions[index].x); catPositions[index].y = std::round(catPositions[index].y); }
        else { catPositions[index].isDragging = false; dragging = false; }
    } else if (ImRenderUtils::isMouseOver(catRect) && ImGui::IsMouseClicked(0) && mIsEnabled) { catPositions[index].isDragging = true; dragOff = { mMousePos.x - catRect.x, mMousePos.y - catRect.y }; }
}

void ModernGui::handleBinding() {
    if (!isBinding) return; mTooltip = "Binding " + lastMod->getName() + "... ESC to unbind";
    for (auto& [key, pressed] : Keyboard::mPressedKeys) { if (!pressed) continue; lastMod->mKey = (key == VK_ESCAPE) ? 0 : key; isBinding = false; ClientInstance::get()->playUi(key == VK_ESCAPE ? "random.break" : "random.orb", 0.75f, 1.f); }
}

void ModernGui::handleBoolBinding() {
    if (!isBoolSettingBinding) return; mTooltip = "Binding " + lastBoolSetting->mName + "... ESC to unbind";
    for (auto& [key, pressed] : Keyboard::mPressedKeys) { if (!pressed) continue; lastBoolSetting->mKey = (key == VK_ESCAPE) ? 0 : key; isBoolSettingBinding = false; ClientInstance::get()->playUi(key == VK_ESCAPE ? "random.break" : "random.orb", 0.75f, 1.f); }
}

void ModernGui::onWindowResizeEvent(WindowResizeEvent& event) {
    resetPosition = true; lastReset = NOW; mParticlesInited = false;
}