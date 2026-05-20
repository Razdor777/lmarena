#pragma execution_character_set("utf-8")
#include "ModernDropdown.hpp"
#include <sstream>
#include <ctime>
#include <Features/Modules/ModuleCategory.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <Features/Modules/Visual/NameProtect.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/Keyboard.hpp>
#include <Utils/StringUtils.hpp>

// Helper functions for layouts
ImVec4 ModernGui::scaleToPoint(const ImVec4& _this, const ImVec4& point, float amount)
{
    return { point.x + (_this.x - point.x) * amount, point.y + (_this.y - point.y) * amount,
             point.z + (_this.z - point.z) * amount, point.w + (_this.w - point.w) * amount };
}

bool ModernGui::isMouseOver(const ImVec4& rect)
{
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    return mousePos.x >= rect.x && mousePos.y >= rect.y && mousePos.x < rect.z && mousePos.y < rect.w;
}

ImVec4 ModernGui::getCenter(ImVec4& vec)
{
    float centerX = (vec.x + vec.z) / 2.0f;
    float centerY = (vec.y + vec.w) / 2.0f;
    return { centerX, centerY, centerX, centerY };
}

bool ModernGui::isMouseOverGuiElement()
{
    for (size_t i = 0; i < catPositions.size(); i++)
    {
        ImVec4 rect = ImVec4(catPositions[i].x, catPositions[i].y, catPositions[i].x + catWidth, catPositions[i].y + catHeight);
        if (isMouseOver(rect)) return true;
    }
    return false;
}

static ImColor lerpColors(const ImColor& a, const ImColor& b, float t) {
    return ImColor(a.Value.x + (b.Value.x - a.Value.x) * t,
                   a.Value.y + (b.Value.y - a.Value.y) * t,
                   a.Value.z + (b.Value.z - a.Value.z) * t,
                   a.Value.w + (b.Value.w - a.Value.w) * t);
}

static std::string getBindKeyName(int vk) {
    if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
    if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
    switch (vk) {
    case VK_TAB: return "Tab"; case VK_SHIFT: return "Shift"; case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt"; case VK_SPACE: return "Space"; case VK_RETURN: return "Enter";
    case VK_BACK: return "Back"; case VK_DELETE: return "Del"; case VK_ESCAPE: return "Esc";
    case VK_INSERT: return "Ins"; case VK_HOME: return "Home"; case VK_END: return "End";
    case VK_PRIOR: return "PgUp"; case VK_NEXT: return "PgDn"; case VK_CAPITAL: return "Caps";
    case VK_LEFT: return "<"; case VK_RIGHT: return ">"; case VK_UP: return "^"; case VK_DOWN: return "v";
    }
    if (vk >= VK_F1 && vk <= VK_F12) return "F" + std::to_string(vk - VK_F1 + 1);
    return "";
}

std::string ModernGui::getCategoryIcon(int idx) {
    // Unicode clean category representations
    const char* icons[] = { "[X]", "[M]", "[P]", "[V]", "[*]" };
    return idx < 5 ? icons[idx] : "[?]";
}

ImColor ModernGui::getGlassColor(float alpha, bool isHeader, bool isBody, bool isCard) {
    static auto* interfaceMod = gFeatureManager->mModuleManager->getModule<Interface>();
    static auto* clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    int themeIdx = clickGui->mTheme.as<int>();

    if (themeIdx == 1) {
        if (isHeader) return ImColor(255, 255, 255, (int)(255 * alpha));
        if (isBody) return ImColor(242, 242, 247, (int)(215 * alpha));
        if (isCard) return ImColor(255, 255, 255, (int)(235 * alpha));
        return ImColor(240, 240, 245, (int)(210 * alpha));
    }

    int interfaceTheme = 0;
    if (interfaceMod) {
        interfaceTheme = interfaceMod->mMode.as<int>();
    }

    ImColor baseTint(16, 16, 20);
    if (interfaceTheme == 0) { // Midnight
        baseTint = ImColor(14, 17, 24);
    } else if (interfaceTheme == 1) { // Slate
        baseTint = ImColor(16, 17, 19);
    } else if (interfaceTheme == 2) { // Ruby
        baseTint = ImColor(22, 13, 14);
    } else if (interfaceTheme == 3) { // Forest
        baseTint = ImColor(12, 19, 15);
    } else if (interfaceTheme == 4) { // Amethyst
        baseTint = ImColor(17, 13, 22);
    } else if (interfaceTheme == 5) { // Rainbow
        baseTint = ColorUtils::Rainbow(30.f, 0.4f, 0.15f, 0);
    }

    if (isHeader) {
        baseTint.Value.w = alpha * 0.98f;
    } else if (isBody) {
        baseTint.Value.w = alpha * 0.72f;
    } else if (isCard) {
        baseTint.Value.x += 0.04f;
        baseTint.Value.y += 0.04f;
        baseTint.Value.z += 0.04f;
        baseTint.Value.w = alpha * 0.82f;
    } else {
        baseTint.Value.w = alpha * 0.75f;
    }

    return baseTint;
}

ImColor ModernGui::getAccentColor(float offset, float alpha, int themeIdx) {
    ImColor c = ColorUtils::getThemedColor(offset);
    c.Value.w = alpha;
    return c;
}

// MAIN RENDERER FOR DRAGGABLE COLUMNS CLICKGUI
void ModernGui::render(float animation, float inScale, int& scrollDirection, float blur, float midclickRounding, bool isPressingShift) {
    static auto* interfaceMod = gFeatureManager->mModuleManager->getModule<Interface>();
    if (interfaceMod) {
        static Interface::FontType lastInterfaceFont = (Interface::FontType)-1;
        if (interfaceMod->mFont.mValue != lastInterfaceFont) {
            lastInterfaceFont = interfaceMod->mFont.mValue;
            std::string fontKey;
            switch (interfaceMod->mFont.mValue) {
                case Interface::FontType::ProductSans: fontKey = "product_sans"; break;
                case Interface::FontType::Mojangles:   fontKey = "mojangles"; break;
                case Interface::FontType::Comfortaa:   fontKey = "comfortaa"; break;
                case Interface::FontType::OpenSans:    fontKey = "open_sans"; break;
                case Interface::FontType::SFPro:       fontKey = "sf_pro_display"; break;
                case Interface::FontType::Sarabun:     fontKey = "sarabun_light"; break;
            }
            FontHelper::setCurrentFont(fontKey);
        }
    }

    FontHelper::pushPrefFont(false, false, false);
    ImVec2 screen = ImRenderUtils::getScreenSize();
    float deltaTime = ImGui::GetIO().DeltaTime;
    mGlobalTime += deltaTime;

    static auto* clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    int themeIdx = 0;

    // 1. DIM SCREEN BACKDROP
    auto drawList = ImGui::GetBackgroundDrawList();
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(screen.x, screen.y), IM_COL32(0, 0, 0, (int)(150 * animation * 0.38f)));
    
    // 2. APPLY GLASS BLUR
    if (blur > 0.01f) {
        ImRenderUtils::addBlur(ImVec4(0.f, 0.f, screen.x, screen.y), animation * blur, 0);
    }

    // 2.5 DRAW AMBIENT BACKGROUND PARTICLES
    updateAndDrawParticles(screen, animation, themeIdx, deltaTime);

    // 3. DRAW EXQUISITE AMBIENT BOTTOM NEON GLOW
    ImColor bottomGlowColor = getAccentColor(0.f, 0.38f * animation, themeIdx);
    float firstheight = (screen.y - screen.y / 3.2f);
    firstheight = MathUtils::lerp(screen.y, firstheight, inScale);
    ImRenderUtils::fillGradientOpaqueRectangle(
        ImVec4(0.f, firstheight, screen.x, screen.y),
        bottomGlowColor, bottomGlowColor, 0.3f * inScale, 0.0f);

    static std::vector<std::string> categories = ModuleCategoryNames;

    // 4. RESET ALIGNMENT IF RESIZED
    if (resetPosition && NOW - lastReset > 100) {
        catPositions.clear();
        resetPosition = false;
    }

    // 5. INITIALIZE POSITIONS ON FIRST LOAD
    if (catPositions.empty() && clickGui->mEnabled) {
        float centerX = screen.x / 2.f;
        float xPos = centerX - (categories.size() * (catWidth + catGap) / 2);
        for (size_t i = 0; i < categories.size(); i++) {
            CategoryPosition pos;
            pos.x = xPos;
            pos.y = 70.f;
            pos.x = std::round(pos.x / 2.f) * 2.f;
            pos.y = std::round(pos.y / 2.f) * 2.f;
            xPos += catWidth + catGap;
            catPositions.push_back(pos);
        }
    }

    // 6. PROCESS VELOCITY/DRAGGING INERTIA
    if (clickGui->mEnabled && !catPositions.empty()) {
        static ImVec2 dragOffset = ImVec2(0.f, 0.f);
        for (size_t i = 0; i < categories.size(); i++) {
            ImVec4 headerRect = ImVec4(catPositions[i].x, catPositions[i].y, catPositions[i].x + catWidth, catPositions[i].y + catHeight);
            headerRect = headerRect.scaleToPoint(ImVec4(screen.x / 2, screen.y / 2, screen.x / 2, screen.y / 2), inScale);

            if (ImGui::IsMouseDown(0)) {
                if (lastDragged == (int)i) {
                    catPositions[i].isDragging = true;
                } else if (lastDragged == -1 && isMouseOver(headerRect)) {
                    lastDragged = (int)i;
                    catPositions[i].isDragging = true;
                    dragOffset = ImVec2(ImGui::GetIO().MousePos.x - catPositions[i].x, ImGui::GetIO().MousePos.y - catPositions[i].y);
                }
            } else {
                if (lastDragged == (int)i) {
                    lastDragged = -1;
                }
                catPositions[i].isDragging = false;
            }

            if (catPositions[i].isDragging) {
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
                catPositions[i].dragVelocity = glm::vec2(mouseDelta.x, mouseDelta.y);
                catPositions[i].x = mousePos.x - dragOffset.x;
                catPositions[i].y = mousePos.y - dragOffset.y;
            } else {
                // Apply spring friction & momentum glide
                catPositions[i].x += catPositions[i].dragVelocity.x;
                catPositions[i].y += catPositions[i].dragVelocity.y;
                catPositions[i].dragVelocity *= 0.84f;
                if (glm::length(catPositions[i].dragVelocity) < 0.05f) {
                    catPositions[i].dragVelocity = glm::vec2(0.f, 0.f);
                }
            }

            // Lock to viewport with margin
            catPositions[i].x = MathUtils::clamp(catPositions[i].x, 8.f, screen.x - catWidth - 8.f);
            catPositions[i].y = MathUtils::clamp(catPositions[i].y, 8.f, screen.y - catHeight - 8.f);
        }
    }

    // 7. DRAW COLUMNS
    std::string tooltip = "";
    float textSize = inScale;
    float textHeight = ImGui::GetFont()->CalcTextSizeA(textSize * 18, FLT_MAX, -1, "").y;

    if (!catPositions.empty()) {
        for (size_t i = 0; i < categories.size(); i++) {
            CategoryPosition& catPos = catPositions[i];
            const auto& modsInCategory = gFeatureManager->mModuleManager->getModulesInCategory(i);
            
            // Filter modules based on search query
            std::vector<std::shared_ptr<Module>> filteredMods;
            for (const auto& mod : modsInCategory) {
                if (mSearchQuery.empty()) {
                    filteredMods.push_back(mod);
                } else {
                    std::string mName = mod->getName();
                    std::string mDesc = mod->mDescription;
                    std::transform(mName.begin(), mName.end(), mName.begin(), ::tolower);
                    std::transform(mDesc.begin(), mDesc.end(), mDesc.begin(), ::tolower);
                    
                    std::string query = mSearchQuery;
                    std::transform(query.begin(), query.end(), query.begin(), ::tolower);
                    
                    if (mName.find(query) != std::string::npos || mDesc.find(query) != std::string::npos) {
                        filteredMods.push_back(mod);
                    }
                }
            }

            // Left Click & Right Click on Header logic
            ImVec4 headerRect = ImVec4(catPos.x, catPos.y, catPos.x + catWidth, catPos.y + catHeight);
            headerRect = headerRect.scaleToPoint(ImVec4(screen.x / 2, screen.y / 2, screen.x / 2, screen.y / 2), inScale);

            if (isMouseOver(headerRect) && clickGui->mEnabled) {
                if (ImGui::IsMouseClicked(1)) {
                    catPos.isExtended = !catPos.isExtended;
                    ClientInstance::get()->playUi("random.click", 0.5f, 1.1f);
                }
                // Middle click aligns panel to default vertical line
                if (ImGui::IsMouseClicked(2)) {
                    float centerX = screen.x / 2.f;
                    float xPos = centerX - (categories.size() * (catWidth + catGap) / 2);
                    catPos.x = xPos + i * (catWidth + catGap);
                    catPos.y = 70.f;
                    catPos.dragVelocity = glm::vec2(0.f, 0.f);
                    ClientInstance::get()->playUi("random.orb", 0.6f, 1.2f);
                }
            }

            // Smooth spring collapse animation
            float targetExtendAnim = catPos.isExtended ? 1.f : 0.f;
            catPos.extendAnim = MathUtils::animate(targetExtendAnim, catPos.extendAnim, deltaTime * 12.f);
            catPos.extendAnim = MathUtils::clamp(catPos.extendAnim, 0.f, 1.f);

            // Compute total module height
            float contentHeight = 0.f;
            for (const auto& mod : filteredMods) {
                contentHeight += modHeight;
                if (mod->cAnim > 0.001f) {
                    for (const auto& setting : mod->mSettings) {
                        if (!setting->mIsVisible()) continue;
                        contentHeight += (modHeight * 0.8f) * mod->cAnim;
                        if (setting->mType == SettingType::Enum && setting->enumSlide > 0.001f) {
                            auto* es = reinterpret_cast<EnumSetting*>(setting);
                            contentHeight += (modHeight * 0.8f) * setting->enumSlide * (es->mValues.size() - 1);
                        }
                    }
                }
            }

            // Dynamic Column Vertical Scrolling calculations
            float maxVisibleHeight = screen.y - catPos.y - catHeight - 50.f;
            if (maxVisibleHeight < 200.f) maxVisibleHeight = 200.f;

            float visibleBodyHeight = contentHeight;
            bool hasScroll = false;
            if (contentHeight > maxVisibleHeight) {
                visibleBodyHeight = maxVisibleHeight;
                hasScroll = true;
            }

            float activeBodyHeight = (visibleBodyHeight + 10.f) * catPos.extendAnim;
            ImVec4 bodyRect = ImVec4(headerRect.x, headerRect.w, headerRect.z, headerRect.w + activeBodyHeight);
            ImVec4 combinedRect = ImVec4(headerRect.x, headerRect.y, headerRect.z, (catPos.extendAnim > 0.001f) ? bodyRect.w : headerRect.w);

            // Mouse wheel scroll physics
            if (catPos.isExtended && clickGui->mEnabled) {
                if (isMouseOver(combinedRect)) {
                    float wheel = ImGui::GetIO().MouseWheel;
                    if (wheel != 0.f) {
                        catPos.scrollEase -= wheel * 38.f;
                    }
                }
                float maxScroll = std::max(0.f, contentHeight - visibleBodyHeight);
                catPos.scrollEase = MathUtils::clamp(catPos.scrollEase, 0.f, maxScroll);
                catPos.yOffset = MathUtils::animate(catPos.scrollEase, catPos.yOffset, deltaTime * 12.f);
                catPos.yOffset = MathUtils::clamp(catPos.yOffset, 0.f, maxScroll);
            } else {
                catPos.scrollEase = 0.f;
                catPos.yOffset = 0.f;
            }

            // DRAW HEADER PANEL
            ImColor headerColor = getGlassColor(1.0f, true, false, false);
            ImColor accentCol = getAccentColor((float)i * 30.f, 0.9f * animation, themeIdx);

            // Premium drop shadow under the unified card
            ImRenderUtils::fillShadowRectangle(combinedRect, ImColor(0, 0, 0), animation * 0.38f, 14.f, 0, 8.f);

            // Header core rounded rectangle
            ImDrawFlags headerFlags = (catPos.extendAnim > 0.001f) ? ImDrawFlags_RoundCornersTop : 0;
            ImRenderUtils::fillRectangle(headerRect, headerColor, animation, 8.f, drawList, headerFlags);
            
            // Neon accent strip below header
            ImVec4 strip = ImVec4(headerRect.x, headerRect.w - 2.f * inScale, headerRect.z, headerRect.w);
            ImRenderUtils::fillRectangle(strip, accentCol, animation);

            // Draw Category Icon and Name
            std::string icon = getCategoryIcon(i);
            std::string catName = categories[i];
            
            // Accent the visual colors per theme
            ImColor headText = (themeIdx == 1) ? ImColor(30, 30, 35) : ImColor(240, 240, 245);
            headText.Value.w *= animation;
            
            // Draw Icon
            ImRenderUtils::drawText(ImVec2(headerRect.x + 8.f, headerRect.y + (catHeight - textHeight) / 2.f), icon, accentCol, textSize * 0.95f, animation, true);
            // Draw Name
            ImRenderUtils::drawText(ImVec2(headerRect.x + 36.f, headerRect.y + (catHeight - textHeight) / 2.f), catName, headText, textSize * 0.95f, animation, true);

            // Draw arrow collapse indicator on the right
            std::string arrow = catPos.isExtended ? "v" : ">";
            float arrowW = ImRenderUtils::getTextWidth(&arrow, textSize * 0.8f);
            ImColor arrowCol = headText; arrowCol.Value.w *= 0.5f;
            ImRenderUtils::drawText(ImVec2(headerRect.z - arrowW - 10.f, headerRect.y + (catHeight - textHeight) / 2.f + 1.f), arrow, arrowCol, textSize * 0.8f, animation, true);

            // DRAW COLUMN CONTENT BODY WITH SMOOTH HEIGHT EXPANSION
            if (catPos.extendAnim > 0.001f) {
                // Setup glassmorphism body
                ImColor bodyColor = getGlassColor(1.0f, false, true, false);
                ImRenderUtils::fillRectangle(bodyRect, bodyColor, animation, 8.f, drawList, ImDrawFlags_RoundCornersBottom);
                ImRenderUtils::addBlur(bodyRect, 8.f * animation * catPos.extendAnim, 8.f, drawList);
            }

            if (catPos.extendAnim > 0.001f) {
                // Clip viewport to avoid settings rendering outside panel during fold/scroll.
                // Subtract 10.f from the bottom clip boundary to protect the beautiful rounded bottom corners.
                drawList->PushClipRect(ImVec2(bodyRect.x, bodyRect.y), ImVec2(bodyRect.z, bodyRect.w - 10.f), true);

                float modY = 4.f - catPos.yOffset;
                bool isMouseInBody = isMouseOver(bodyRect);
                float rightPadding = hasScroll ? 10.f : 6.f;

                bool isOverScroll = false;
                if (hasScroll) {
                    ImVec4 scrollTrack = ImVec4(bodyRect.z - 5.f, bodyRect.y + 4.f, bodyRect.z - 2.f, bodyRect.w - 4.f);
                    ImVec4 scrollInteractionArea = ImVec4(scrollTrack.x - 6.f, scrollTrack.y, scrollTrack.z + 6.f, scrollTrack.w);
                    if (isMouseOver(scrollInteractionArea)) isOverScroll = true;
                }

                for (const auto& mod : filteredMods) {
                    ImVec4 modCard = ImVec4(bodyRect.x + 6.f, bodyRect.y + modY, bodyRect.z - rightPadding, bodyRect.y + modY + modHeight - 4.f);
                    modCard.x = std::floor(modCard.x); modCard.y = std::floor(modCard.y);
                    modCard.z = std::floor(modCard.z); modCard.w = std::floor(modCard.w);

                    // Dynamic hover tracking
                    bool modHov = isMouseInBody && isMouseOver(modCard) && clickGui->mEnabled && !displayColorPicker && !isOverScroll && !catPos.isDraggingScrollbar;
                    mod->cFade = MathUtils::animate(modHov ? 1.f : 0.f, mod->cFade, deltaTime * 16.f);

                    // Module Enabled animations
                    if (mod->mEnabled && mod->cScale < 1.f) mod->cScale = MathUtils::animate(1.f, mod->cScale, deltaTime * 26.f);
                    else if (!mod->mEnabled) mod->cScale = MathUtils::animate(1.f, mod->cScale, deltaTime * 16.f);

                    // Dynamic Burst expanding glow
                    if (mod->toggleBurst > 0.001f) {
                        mod->toggleBurst = MathUtils::animate(0.f, mod->toggleBurst, deltaTime * 4.5f);
                        ImColor burstColor = getAccentColor(modCard.y * 1.5f, mod->toggleBurst * animation * catPos.extendAnim, themeIdx);
                        ImRenderUtils::fillRectangle(modCard, burstColor, mod->toggleBurst * 0.16f, 8.f);
                    }

                    // Card Background interpolation
                    ImColor cardBg = getGlassColor(animation, false, false, true);
                    ImColor cardEnabledBg = getAccentColor(modCard.y * 2.f, 0.15f * animation, themeIdx);
                    
                    ImColor currentCardBg = mod->mEnabled ? lerpColors(cardBg, cardEnabledBg, 1.0f) : cardBg;
                    ImRenderUtils::fillRectangle(modCard, currentCardBg, animation, 8.f);

                    // Premium animated hover background & border glow overlay
                    if (mod->cFade > 0.001f) {
                        ImColor hoverLeft = getAccentColor(modCard.y * 1.5f, (themeIdx == 1 ? 0.12f : 0.16f) * mod->cFade * animation, themeIdx);
                        ImColor hoverRight = getAccentColor(modCard.y * 1.5f, (themeIdx == 1 ? 0.02f : 0.04f) * mod->cFade * animation, themeIdx);
                        ImRenderUtils::fillRoundedGradientRectangle(modCard, hoverLeft, hoverRight, 8.f, hoverLeft.Value.w, hoverRight.Value.w);

                        ImColor hoverOutline = getAccentColor(modCard.y * 1.5f, 0.45f * mod->cFade * animation, themeIdx);
                        ImRenderUtils::drawRoundRect(modCard, 0, 8.f, hoverOutline, animation * mod->cFade, 1.0f);
                    }

                    // Active neon left stripe
                    if (mod->mEnabled) {
                        ImColor neonAccent = getAccentColor(modCard.y * 2.f, animation, themeIdx);
                        ImRenderUtils::fillRectangle(ImVec4(modCard.x, modCard.y + 4.f, modCard.x + 3.f, modCard.w - 4.f), neonAccent, animation, 1.5f);
                    }

                    // Draw Module Name
                    std::string mName = mod->getName();
                    ImColor mColorText;
                    if (mod->mEnabled) {
                        mColorText = getAccentColor(modCard.y * 1.5f, animation, themeIdx);
                    } else {
                        ImColor normalCol = (themeIdx == 1) ? ImColor(50, 50, 55) : ImColor(160, 160, 175);
                        ImColor hoverCol = getAccentColor(modCard.y * 1.5f, animation, themeIdx);
                        mColorText = lerpColors(normalCol, hoverCol, mod->cFade);
                    }
                    mColorText.Value.w *= animation;
                    
                    float tx = modCard.x + 10.f;
                    if (mod->mEnabled) tx += 2.f * mod->cScale * animation; // slide visual effect when enabled
                    ImRenderUtils::drawText(ImVec2(tx, modCard.y + (modHeight - 4.f - textHeight) / 2.f), mName, mColorText, textSize * 0.85f, animation, true);

                    // Keybind text badge if configured (aligned to the right side of the card since the switch is gone!)
                    if (mod->mKey > 0 && !isBinding) {
                        std::string kName = getBindKeyName(mod->mKey);
                        if (!kName.empty()) {
                            float kSize = textSize * 0.65f;
                            float kW = ImRenderUtils::getTextWidth(&kName, kSize);
                            float kH = ImGui::GetFont()->CalcTextSizeA(kSize * 18, FLT_MAX, -1, kName.c_str()).y;
                            float rightX = modCard.z - 8.f;
                            ImVec4 kRect = ImVec4(rightX - kW - 6.f, modCard.y + (modHeight - 4.f - kH) / 2.f - 2.f, rightX, modCard.y + (modHeight - 4.f + kH) / 2.f + 2.f);
                            ImColor kBg = (themeIdx == 1) ? ImColor(220, 220, 230, (int)(180 * animation)) : ImColor(30, 30, 35, (int)(180 * animation));
                            ImRenderUtils::fillRectangle(kRect, kBg, animation, 3.f);
                            ImColor kTextC = (themeIdx == 1) ? ImColor(80, 80, 90) : ImColor(150, 150, 165);
                            kTextC.Value.w *= animation;
                            ImRenderUtils::drawText(ImVec2(kRect.x + 3.f, kRect.y + 1.f), kName, kTextC, kSize, animation, true);
                        }
                    }

                    // Click Actions
                    if (modHov) {
                        tooltip = mod->mDescription;
                        if (ImGui::IsMouseClicked(0)) {
                            // Left click toggles the module!
                            mod->toggle();
                            mod->cScale = 0.88f;
                            mod->toggleBurst = 1.f;
                            ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
                        }
                        // Right Click opens/closes settings directly
                        if (ImGui::IsMouseClicked(1) && !mod->mSettings.empty()) {
                            if (mod->cAnim < 0.05f || mod->cAnim > 0.95f) {
                                mod->showSettings = !mod->showSettings;
                                ClientInstance::get()->playUi("random.click", 0.5f, 1.1f);
                            }
                        }
                        // Middle Click triggers Keybind Binding
                        if (ImGui::IsMouseClicked(2)) {
                            lastMod = mod;
                            isBinding = true;
                            ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
                        }
                    }

                    // Animate Settings Panel sliding roll
                    float tgtAnim = mod->showSettings ? 1.f : 0.f;
                    mod->cAnim = MathUtils::animate(tgtAnim, mod->cAnim, deltaTime * (tgtAnim > mod->cAnim ? 14.f : 10.f));
                    mod->cAnim = MathUtils::clamp(mod->cAnim, 0.f, 1.f);

                    modY += modHeight;

                    // DRAW NESTED SETTINGS
                    if (mod->cAnim > 0.001f) {
                        for (auto* setting : mod->mSettings) {
                            if (!setting->mIsVisible()) {
                                setting->sliderEase = 0.f;
                                setting->enumSlide = 0.f;
                                continue;
                            }

                            float setRowH = modHeight * 0.8f * mod->cAnim;
                            float rightPaddingSet = hasScroll ? 16.f : 12.f;
                            ImVec4 setRect = ImVec4(bodyRect.x + 12.f, bodyRect.y + modY, bodyRect.z - rightPaddingSet, bodyRect.y + modY + setRowH - 2.f);
                            setRect.x = std::floor(setRect.x); setRect.y = std::floor(setRect.y);
                            setRect.z = std::floor(setRect.z); setRect.w = std::floor(setRect.w);

                            bool setHov = isMouseInBody && isMouseOver(setRect) && clickGui->mEnabled && !displayColorPicker && !isOverScroll && !catPos.isDraggingScrollbar;
                            setting->hoverAnim = MathUtils::lerp(setting->hoverAnim, setHov ? 1.f : 0.f, deltaTime * 18.f);
                            
                            // Highlight settings background on hover
                            if (setting->hoverAnim > 0.001f) {
                                ImColor hoverLeft = getAccentColor(setRect.y * 1.5f, (themeIdx == 1 ? 0.10f : 0.12f) * setting->hoverAnim * animation * mod->cAnim, themeIdx);
                                ImColor hoverRight = getAccentColor(setRect.y * 1.5f, (themeIdx == 1 ? 0.01f : 0.03f) * setting->hoverAnim * animation * mod->cAnim, themeIdx);
                                ImRenderUtils::fillRoundedGradientRectangle(setRect, hoverLeft, hoverRight, 4.f, hoverLeft.Value.w, hoverRight.Value.w);

                                ImColor hoverOutline = getAccentColor(setRect.y * 1.5f, 0.35f * setting->hoverAnim * animation * mod->cAnim, themeIdx);
                                ImRenderUtils::drawRoundRect(setRect, 0, 4.f, hoverOutline, animation * mod->cAnim * setting->hoverAnim, 1.0f);
                            }

                            // Dynamic Switch cases depending on Setting types
                            switch (setting->mType) {
                            case SettingType::Bool:
                                {
                                    BoolSetting* bs = reinterpret_cast<BoolSetting*>(setting);
                                    if (setHov) {
                                        tooltip = bs->mDescription;
                                        if (ImGui::IsMouseClicked(0)) {
                                            bs->mValue = !bs->mValue;
                                            ClientInstance::get()->playUi("random.pop", 0.6f, 1.1f);
                                        }
                                        if (ImGui::IsMouseClicked(2)) {
                                            lastBoolSetting = bs;
                                            isBoolSettingBinding = true;
                                            ClientInstance::get()->playUi("random.pop", 0.75f, 1.0f);
                                        }
                                    }

                                    // Render Checkbox / Mini Switch
                                    std::string sName = bs->mName;
                                    ImColor sTextCol;
                                    if (themeIdx == 1) {
                                        sTextCol = lerpColors(ImColor(40, 40, 45), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    } else {
                                        sTextCol = lerpColors(ImColor(200, 200, 210), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    }
                                    sTextCol.Value.w *= animation * mod->cAnim;
                                    ImRenderUtils::drawText(ImVec2(setRect.x + 8.f, setRect.y + (setRowH - textHeight * 0.85f) / 2.f), sName, sTextCol, textSize * 0.78f, animation * mod->cAnim, true);

                                    float bW = 24.f, bH = 12.f;
                                    float bX = setRect.z - bW - 8.f, bY = setRect.y + (setRowH - bH) / 2.f;
                                    bs->boolScale = MathUtils::animate(bs->mValue ? 1.f : 0.f, bs->boolScale, deltaTime * 18.f);
                                    
                                    ImColor bOff = (themeIdx == 1) ? ImColor(190, 190, 200) : ImColor(45, 45, 52);
                                    ImColor bOn = getAccentColor(setRect.y * 3.f, 0.9f * animation, themeIdx);
                                    ImRenderUtils::fillRectangle(ImVec4(bX, bY, bX + bW, bY + bH), lerpColors(bOff, bOn, bs->boolScale), animation * mod->cAnim, bH / 2.f);
                                    
                                    float bRad = 4.f;
                                    float bThumbX = MathUtils::lerp(bX + bRad + 2.f, bX + bW - bRad - 2.f, bs->boolScale);
                                    ImRenderUtils::fillCircle(ImVec2(bThumbX, bY + bH / 2.f), bRad * animation, ImColor(255, 255, 255), animation * mod->cAnim, 10);
                                    break;
                                }
                            case SettingType::Number:
                                {
                                    NumberSetting* ns = reinterpret_cast<NumberSetting*>(setting);
                                    if (setHov) tooltip = ns->mDescription;

                                    std::string sName = ns->mName;
                                    char valStr[16];
                                    sprintf_s(valStr, "%.2f", ns->mValue);
                                    std::string sValue = valStr;

                                    ImColor sTextCol;
                                    if (themeIdx == 1) {
                                        sTextCol = lerpColors(ImColor(40, 40, 45), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    } else {
                                        sTextCol = lerpColors(ImColor(200, 200, 210), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    }
                                    sTextCol.Value.w *= animation * mod->cAnim;
                                    ImColor sValueCol = (themeIdx == 1) ? ImColor(110, 110, 120) : ImColor(135, 135, 150);
                                    sValueCol.Value.w *= animation * mod->cAnim;

                                    ImRenderUtils::drawText(ImVec2(setRect.x + 8.f, setRect.y + 1.f), sName, sTextCol, textSize * 0.78f, animation * mod->cAnim, true);
                                    float vW = ImRenderUtils::getTextWidth(&sValue, textSize * 0.72f);
                                    ImRenderUtils::drawText(ImVec2(setRect.z - vW - 8.f, setRect.y + 1.f), sValue, sValueCol, textSize * 0.72f, animation * mod->cAnim, true);

                                    // Slider track
                                    float sTrackH = 3.f * inScale;
                                    ImVec4 sTrack = ImVec4(setRect.x + 8.f, setRect.w - sTrackH - 4.f, setRect.z - 8.f, setRect.w - 4.f);
                                    ImColor trackColor = (themeIdx == 1) ? ImColor(200, 200, 210) : ImColor(45, 45, 55);
                                    ImRenderUtils::fillRectangle(sTrack, trackColor, animation * mod->cAnim, 1.5f);

                                    float ratio = MathUtils::clamp((ns->mValue - ns->mMin) / (ns->mMax - ns->mMin), 0.f, 1.f);
                                    setting->sliderEase = MathUtils::animate(ratio * sTrack.getWidth(), setting->sliderEase, deltaTime * 12.f);
                                    setting->sliderEase = MathUtils::clamp(setting->sliderEase, 0.f, sTrack.getWidth());

                                    // Active slider fill
                                    ImColor activeFillColor = getAccentColor(setRect.y * 3.f, animation * mod->cAnim, themeIdx);
                                    if (setting->sliderEase > 0.5f) {
                                        ImRenderUtils::fillRectangle(ImVec4(sTrack.x, sTrack.y, sTrack.x + setting->sliderEase, sTrack.w), activeFillColor, animation * mod->cAnim, 1.5f);
                                    }

                                    // Circular slider handle knob
                                    float thumbX = sTrack.x + setting->sliderEase;
                                    float thumbY = sTrack.getCenter().y;
                                    float kRadius = 4.f * inScale;
                                    if (ns->isDragging) kRadius *= 1.25f;
                                    ImRenderUtils::fillCircle(ImVec2(thumbX, thumbY), kRadius * animation, activeFillColor, animation * mod->cAnim, 10);
                                    ImRenderUtils::fillShadowCircle(ImVec2(thumbX, thumbY), kRadius + 2.f, activeFillColor, animation * mod->cAnim * 0.35f, 5.f, 0);

                                    // Dragging handling
                                    if (setHov && ImGui::IsMouseDown(0)) {
                                        ns->isDragging = true;
                                        mLastDraggedSetting = setting;
                                    }
                                    if (ns->isDragging && mLastDraggedSetting == setting) {
                                        if (!ImGui::IsMouseDown(0)) {
                                            ns->isDragging = false;
                                        } else {
                                            float mouseRatio = MathUtils::clamp((ImRenderUtils::getMousePos().x - sTrack.x) / sTrack.getWidth(), 0.f, 1.f);
                                            float calculatedValue = mouseRatio * (ns->mMax - ns->mMin) + ns->mMin;
                                            ns->setValue(calculatedValue);
                                        }
                                    }
                                    // Right click drag snaps to midclick rounding increment
                                    if (ImGui::IsMouseDown(2) && ns->isDragging && mLastDraggedSetting == setting) {
                                        float mouseRatio = MathUtils::clamp((ImRenderUtils::getMousePos().x - sTrack.x) / sTrack.getWidth(), 0.f, 1.f);
                                        float calculatedValue = mouseRatio * (ns->mMax - ns->mMin) + ns->mMin;
                                        ns->mValue = std::round(calculatedValue / midclickRounding) * midclickRounding;
                                    }
                                    break;
                                }
                            case SettingType::Enum:
                                {
                                    EnumSetting* es = reinterpret_cast<EnumSetting*>(setting);
                                    if (setHov) tooltip = es->mDescription;

                                    std::string sName = es->mName;
                                    std::string currentVal = es->mValues[es->mValue];

                                    ImColor sTextCol;
                                    if (themeIdx == 1) {
                                        sTextCol = lerpColors(ImColor(40, 40, 45), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    } else {
                                        sTextCol = lerpColors(ImColor(200, 200, 210), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    }
                                    sTextCol.Value.w *= animation * mod->cAnim;
                                    ImColor sValCol = getAccentColor(setRect.y * 3.f, animation * mod->cAnim, themeIdx);

                                    ImRenderUtils::drawText(ImVec2(setRect.x + 8.f, setRect.y + (setRowH - textHeight * 0.8f) / 2.f), sName, sTextCol, textSize * 0.78f, animation * mod->cAnim, true);
                                    float vW = ImRenderUtils::getTextWidth(&currentVal, textSize * 0.75f);
                                    ImRenderUtils::drawText(ImVec2(setRect.z - vW - 8.f, setRect.y + (setRowH - textHeight * 0.8f) / 2.f), currentVal, sValCol, textSize * 0.75f, animation * mod->cAnim, true);

                                    // Staggered roll dropdown height
                                    float tgtEnum = es->enumExtended ? 1.f : 0.f;
                                    es->enumSlide = MathUtils::animate(tgtEnum, es->enumSlide, deltaTime * 12.f);
                                    es->enumSlide = MathUtils::clamp(es->enumSlide, 0.f, 1.f);

                                    float extraH = 0.f;
                                    if (es->enumSlide > 0.001f) {
                                        int nOptions = (int)es->mValues.size();
                                        float itemRowH = modHeight * 0.8f * es->enumSlide;
                                        int ri = 0;
                                        for (int j = 0; j < nOptions; j++) {
                                            if (j == es->mValue) continue;
                                            float iy = setRect.w + ri * itemRowH;
                                            ImVec4 optRect = ImVec4(setRect.x, iy, setRect.z, iy + itemRowH);
                                            optRect.x = std::floor(optRect.x); optRect.y = std::floor(optRect.y);
                                            optRect.z = std::floor(optRect.z); optRect.w = std::floor(optRect.w);

                                            extraH += itemRowH;
                                            if (optRect.y >= bodyRect.w) { ri++; continue; }

                                            float itemFade = MathUtils::clamp((es->enumSlide - ri * 0.04f) / 0.25f, 0.f, 1.f);
                                            
                                            // Glass row highlight
                                            ImColor itemRowBg = (themeIdx == 1) ? ImColor(220, 220, 230, (int)(150 * es->enumSlide * itemFade * animation)) : ImColor(20, 20, 25, (int)(150 * es->enumSlide * itemFade * animation));
                                            ImRenderUtils::fillRectangle(optRect, itemRowBg, animation * es->enumSlide * itemFade, 4.f);

                                            bool optHov = isMouseInBody && isMouseOver(optRect) && clickGui->mEnabled && !displayColorPicker;
                                            if (optHov) {
                                                ImColor optHoverLeft = getAccentColor(optRect.y * 1.5f, (themeIdx == 1 ? 0.12f : 0.16f) * es->enumSlide * itemFade * animation, themeIdx);
                                                ImColor optHoverRight = getAccentColor(optRect.y * 1.5f, (themeIdx == 1 ? 0.02f : 0.04f) * es->enumSlide * itemFade * animation, themeIdx);
                                                ImRenderUtils::fillRoundedGradientRectangle(optRect, optHoverLeft, optHoverRight, 4.f, optHoverLeft.Value.w, optHoverRight.Value.w);
                                            }

                                            std::string optName = es->mValues[j];
                                            float optNameH = ImGui::GetFont()->CalcTextSizeA(textSize * 0.72f * 18, FLT_MAX, -1, optName.c_str()).y;
                                            ImColor optTextC = (j == es->mValue) ? sValCol : ((themeIdx == 1) ? ImColor(60, 60, 65) : ImColor(140, 140, 155));
                                            optTextC.Value.w *= animation * es->enumSlide * itemFade;

                                            ImRenderUtils::drawText(ImVec2(optRect.x + 16.f, optRect.y + (itemRowH - optNameH) / 2.f), optName, optTextC, textSize * 0.72f, animation * es->enumSlide * itemFade, true);

                                            if (optHov && ImGui::IsMouseClicked(0)) {
                                                es->mValue = j;
                                                es->enumExtended = false;
                                                ClientInstance::get()->playUi("random.pop", 0.6f, 1.1f);
                                            }
                                            ri++;
                                        }
                                    }

                                    if (setHov) {
                                        if (ImGui::IsMouseClicked(0) && !es->enumExtended) {
                                            es->mValue = (es->mValue + 1) % (int)es->mValues.size();
                                            ClientInstance::get()->playUi("random.pop", 0.6f, 1.1f);
                                        }
                                        if (ImGui::IsMouseClicked(1)) {
                                            es->enumExtended = !es->enumExtended;
                                            ClientInstance::get()->playUi("random.click", 0.5f, 1.1f);
                                        }
                                    }

                                    modY += extraH;
                                    break;
                                }
                            case SettingType::Color:
                                {
                                    ColorSetting* cs = reinterpret_cast<ColorSetting*>(setting);
                                    if (setHov) tooltip = cs->mDescription;

                                    std::string sName = cs->mName;
                                    ImColor sTextCol;
                                    if (themeIdx == 1) {
                                        sTextCol = lerpColors(ImColor(40, 40, 45), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    } else {
                                        sTextCol = lerpColors(ImColor(200, 200, 210), getAccentColor(setRect.y * 1.5f, 1.0f, themeIdx), setting->hoverAnim);
                                    }
                                    sTextCol.Value.w *= animation * mod->cAnim;
                                    ImRenderUtils::drawText(ImVec2(setRect.x + 8.f, setRect.y + (setRowH - textHeight * 0.8f) / 2.f), sName, sTextCol, textSize * 0.78f, animation * mod->cAnim, true);

                                    // Circle showing selected color
                                    float cRadius = 7.f * inScale;
                                    float cX = setRect.z - cRadius - 10.f, cY = setRect.y + setRowH / 2.f;
                                    ImColor currentBadgeCol = cs->getAsImColor();
                                    currentBadgeCol.Value.w = animation * mod->cAnim;

                                    ImRenderUtils::fillCircle(ImVec2(cX, cY), cRadius, currentBadgeCol, animation * mod->cAnim, 12);
                                    ImRenderUtils::fillShadowCircle(ImVec2(cX, cY), cRadius + 2.f, currentBadgeCol, animation * mod->cAnim * 0.4f, 6.f, 0);

                                    if (setHov && ImGui::IsMouseClicked(0)) {
                                        displayColorPicker = !displayColorPicker;
                                        lastColorSetting = cs;
                                        ClientInstance::get()->playUi("random.pop", 0.6f, 1.1f);
                                    }
                                    break;
                                }
                            }

                            modY += setRowH;
                        }
                    }
                }

                drawList->PopClipRect();

                // Draw premium thin themed scrollbar
                if (hasScroll) {
                    ImVec4 scrollTrack = ImVec4(bodyRect.z - 5.f, bodyRect.y + 4.f, bodyRect.z - 2.f, bodyRect.w - 4.f);
                    ImColor trackBg = (themeIdx == 1) ? ImColor(220, 220, 230, (int)(100 * animation)) : ImColor(30, 30, 35, (int)(100 * animation));
                    drawList->AddRectFilled(ImVec2(scrollTrack.x, scrollTrack.y), ImVec2(scrollTrack.z, scrollTrack.w), trackBg, 1.5f);

                    float trackHeight = scrollTrack.w - scrollTrack.y;
                    float thumbHeight = std::max(15.f, (visibleBodyHeight / contentHeight) * trackHeight);
                    float maxScroll = contentHeight - visibleBodyHeight;
                    float scrollRatio = (maxScroll > 0.f) ? (catPos.yOffset / maxScroll) : 0.f;
                    float thumbY = scrollTrack.y + scrollRatio * (trackHeight - thumbHeight);

                    ImVec4 scrollThumb = ImVec4(scrollTrack.x, thumbY, scrollTrack.z, thumbY + thumbHeight);
                    ImColor thumbColor = getAccentColor((float)i * 20.f, 0.7f * animation * catPos.extendAnim, themeIdx);
                    drawList->AddRectFilled(ImVec2(scrollThumb.x, scrollThumb.y), ImVec2(scrollThumb.z, scrollThumb.w), thumbColor, 1.5f);

                    // Scrollbar Dragging Input
                    if (clickGui->mEnabled) {
                        ImVec4 scrollInteractionArea = ImVec4(scrollTrack.x - 6.f, scrollTrack.y, scrollTrack.z + 6.f, scrollTrack.w);
                        if (ImGui::IsMouseDown(0)) {
                            if (catPos.isDraggingScrollbar) {
                                float mouseY = ImGui::GetIO().MousePos.y;
                                float relativeY = mouseY - scrollTrack.y - thumbHeight / 2.f;
                                float ratio = MathUtils::clamp(relativeY / (trackHeight - thumbHeight), 0.f, 1.f);
                                catPos.scrollEase = ratio * maxScroll;
                            } else if (isMouseOver(scrollInteractionArea) && !catPos.isDragging) {
                                catPos.isDraggingScrollbar = true;
                            }
                        } else {
                            catPos.isDraggingScrollbar = false;
                        }
                    }
                } else {
                    catPos.isDraggingScrollbar = false;
                }
            }
            
            // Draw the unified neon glowing boundary outline on top of all contents to avoid being covered
            ImColor boundaryColor = getAccentColor((float)i * 25.f, (0.25f + 0.15f * catPos.extendAnim) * animation, themeIdx);
            ImRenderUtils::drawRoundRect(combinedRect, 0, 8.f, boundaryColor, animation, 1.2f);
        }
    }

    // 7.5 RENDER SEARCH BAR AND METRICS DASHBOARD
    renderSearchBar(screen, animation, themeIdx, deltaTime);
    renderDashboard(screen, animation, inScale, themeIdx, deltaTime);

    // 8. RENDER GLOWING OVERLAYS (TOOLTIPS, COLOR PICKERS, BINDINGS)
    renderTooltip(animation, inScale, deltaTime);
    renderBindings(animation, deltaTime);
    
    if (clickGui->mEnabled) {
        renderColorPicker(screen, animation, deltaTime);
    } else {
        displayColorPicker = false;
    }

    if (clickGui->mEnabled) scrollDirection = 0;
    ImGui::PopFont();
}

// PREMIUM FLOATING COLOR WHEEL PICKER OVERLAY
void ModernGui::renderColorPicker(ImVec2 screen, float animation, float deltaTime) {
    mColorPickerAnim = MathUtils::animate(displayColorPicker ? 1.f : 0.f, mColorPickerAnim, deltaTime * 10.f);
    if (mColorPickerAnim < 0.001f) return;

    float w = 340.f * mColorPickerAnim;
    float h = 300.f * mColorPickerAnim;
    float x = screen.x / 2.f - w / 2.f;
    float y = screen.y / 2.f - h / 2.f;
    ImVec4 cRect = ImVec4(x, y, x + w, y + h);

    float a = animation * mColorPickerAnim;
    
    // Draw exquisite blurred backdrop card
    ImRenderUtils::addBlur(cRect, 8.f * a, 10.f);
    static auto* clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    int themeIdx = clickGui->mTheme.as<int>();

    ImColor cardColor = getGlassColor(a, false, false, false);
    ImRenderUtils::fillRectangle(cRect, cardColor, a, 14.f);
    ImRenderUtils::fillShadowRectangle(cRect, ImColor(0, 0, 0), a * 0.4f, 18.f, 0, 14.f);

    // Glowing border outline
    ImColor borderOutline = getAccentColor(0.f, 0.4f * a, themeIdx);
    ImRenderUtils::drawRoundRect(cRect, 0, 14.f, borderOutline, a, 1.5f);

    FontHelper::pushPrefFont(false, false, false);
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.15f, 1.f));

    ImGui::Begin("##cg_colorpicker", &displayColorPicker, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
    
    if (lastColorSetting) {
        ImGui::ColorPicker4("##pickerwheel", lastColorSetting->mValue, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_PickerHueWheel);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.2f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.24f, 0.3f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.35f, 1.f));

    if (ImGui::Button("Close Picker", ImVec2(w - 28.f, 26))) {
        displayColorPicker = false;
        ClientInstance::get()->playUi("random.break", 0.6f, 1.1f);
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    FontHelper::popPrefFont();

    // Click outside color picker closes it
    if (ImGui::IsMouseClicked(0) && !isMouseOver(cRect)) {
        displayColorPicker = false;
    }
}

// BINDINGS LOGIC RENDERING
void ModernGui::renderBindings(float anim, float deltaTime) {
    if (!isBinding && !isBoolSettingBinding) return;

    if (isBinding && lastMod) {
        mTooltipText = "Binding [" + lastMod->getName() + "]... Press ESC to clear.";
        mTooltipAlpha = 1.f;
        mTooltipPos = ImRenderUtils::getMousePos();

        for (const auto& key : Keyboard::mPressedKeys) {
            if (key.second) {
                lastMod->mKey = (key.first == VK_ESCAPE) ? 0 : key.first;
                isBinding = false;
                ClientInstance::get()->playUi(key.first == VK_ESCAPE ? "random.break" : "random.orb", 0.75f, 1.0f);
                break;
            }
        }
    }

    if (isBoolSettingBinding && lastBoolSetting) {
        mTooltipText = "Binding [" + lastBoolSetting->mName + "]... Press ESC to clear.";
        mTooltipAlpha = 1.f;
        mTooltipPos = ImRenderUtils::getMousePos();

        for (const auto& key : Keyboard::mPressedKeys) {
            if (key.second) {
                lastBoolSetting->mKey = (key.first == VK_ESCAPE) ? 0 : key.first;
                isBoolSettingBinding = false;
                ClientInstance::get()->playUi(key.first == VK_ESCAPE ? "random.break" : "random.orb", 0.75f, 1.0f);
                break;
            }
        }
    }
}

// DYNAMIC FLOATING GLASSMORPHIC TOOLTIP RENDERER
void ModernGui::renderTooltip(float anim, float inScale, float deltaTime) {
    if (mTooltipText.empty()) {
        mTooltipAlpha = MathUtils::animate(0.f, mTooltipAlpha, deltaTime * 25.f);
        if (mTooltipAlpha < 0.001f) return;
    }

    float tSize = 0.74f * inScale;
    float padding = 8.f;
    float maxW = 230.f;

    std::vector<std::string> lines;
    std::string currentLine;
    float currentW = 0.f;
    float spaceW = ImGui::GetFont()->CalcTextSizeA(tSize * 18.f, FLT_MAX, 0, " ").x;
    std::istringstream stream(mTooltipText);
    std::string word;

    while (stream >> word) {
        float wordW = ImGui::GetFont()->CalcTextSizeA(tSize * 18.f, FLT_MAX, 0, word.c_str()).x;
        if (currentW + wordW + (currentLine.empty() ? 0.f : spaceW) > maxW && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = word;
            currentW = wordW;
        } else {
            if (!currentLine.empty()) {
                currentLine += " ";
                currentW += spaceW;
            }
            currentLine += word;
            currentW += wordW;
        }
    }
    if (!currentLine.empty()) lines.push_back(currentLine);
    if (lines.empty()) lines.push_back(mTooltipText);

    float lHeight = ImGui::GetFont()->CalcTextSizeA(tSize * 18.f, FLT_MAX, 0, "A").y;
    float tooltipW = maxW;
    for (const auto& line : lines) {
        float lineW = ImGui::GetFont()->CalcTextSizeA(tSize * 18.f, FLT_MAX, 0, line.c_str()).x;
        tooltipW = std::max(tooltipW, std::min(lineW, maxW));
    }
    float tooltipH = (float)lines.size() * lHeight;

    ImVec4 tooltipRect = ImVec4(mTooltipPos.x + 16.f, mTooltipPos.y + 16.f, mTooltipPos.x + 16.f + tooltipW + padding * 2.f, mTooltipPos.y + 16.f + tooltipH + padding * 2.f);
    float alphaVal = anim * mTooltipAlpha;
    if (alphaVal < 0.001f) return;

    auto frontList = ImGui::GetForegroundDrawList();
    ImRenderUtils::addBlur(tooltipRect, 5.f * alphaVal, 6.f, frontList);
    
    static auto* clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    int themeIdx = clickGui->mTheme.as<int>();

    ImColor tBgColor = getGlassColor(alphaVal, false, false, false);
    ImRenderUtils::fillRectangle(tooltipRect, tBgColor, alphaVal, 6.f, frontList);
    ImRenderUtils::fillShadowRectangle(tooltipRect, ImColor(0, 0, 0), alphaVal * 0.35f, 10.f, 0, 6.f, frontList);

    // Glowing left indicator line on tooltip
    ImColor borderGlow = getAccentColor(0.f, alphaVal * 0.8f, themeIdx);
    ImRenderUtils::fillRectangle(ImVec4(tooltipRect.x, tooltipRect.y, tooltipRect.x + 2.f, tooltipRect.w), borderGlow, alphaVal, 0.f, frontList);

    ImColor textColorVal = (themeIdx == 1) ? ImColor(30, 30, 35) : ImColor(240, 240, 245);
    textColorVal.Value.w = alphaVal;
    
    float textY = tooltipRect.y + padding;
    for (const auto& line : lines) {
        ImRenderUtils::drawText(ImVec2(tooltipRect.x + padding + 2.f, textY), line, textColorVal, tSize, alphaVal, true, 0, frontList);
        textY += lHeight;
    }

    mTooltipText = "";
    mTooltipAlpha = MathUtils::animate(0.f, mTooltipAlpha, deltaTime * 15.f);
}

// WINDOW RESIZE EVENT
void ModernGui::onWindowResizeEvent(WindowResizeEvent& event) {
    resetPosition = true;
    lastReset = NOW;
}

// PARTICLES IMPLEMENTATION
void ModernGui::updateAndDrawParticles(ImVec2 screen, float animation, int themeIdx, float deltaTime) {
    if (mParticles.empty()) {
        mParticles.resize(110);
        for (auto& p : mParticles) {
            p.pos = glm::vec2(MathUtils::randomFloat(0.f, screen.x), MathUtils::randomFloat(0.f, screen.y));
            p.vel = glm::vec2(MathUtils::randomFloat(-25.f, 25.f), MathUtils::randomFloat(-25.f, 25.f));
            p.size = MathUtils::randomFloat(1.5f, 3.5f);
            p.alpha = MathUtils::randomFloat(0.15f, 0.45f);
            p.fade = MathUtils::randomFloat(0.2f, 0.8f);
        }
    }

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    auto drawList = ImGui::GetBackgroundDrawList();
    ImColor baseColor = getAccentColor(0.f, 1.f, themeIdx);

    for (auto& p : mParticles) {
        p.pos += p.vel * deltaTime;

        if (p.pos.x < 0) p.pos.x = screen.x;
        if (p.pos.x > screen.x) p.pos.x = 0;
        if (p.pos.y < 0) p.pos.y = screen.y;
        if (p.pos.y > screen.y) p.pos.y = 0;

        float distToMouse = glm::distance(p.pos, glm::vec2(mousePos.x, mousePos.y));
        if (distToMouse < 200.f && distToMouse > 0.1f) {
            float force = (200.f - distToMouse) / 200.f * 45.f;
            glm::vec2 dir = glm::normalize(glm::vec2(mousePos.x, mousePos.y) - p.pos);
            p.pos += dir * force * deltaTime;
        }

        ImColor pColor = baseColor;
        pColor.Value.w = p.alpha * animation;
        drawList->AddCircleFilled(ImVec2(p.pos.x, p.pos.y), p.size, pColor);
    }

    for (size_t i = 0; i < mParticles.size(); i++) {
        for (size_t j = i + 1; j < mParticles.size(); j++) {
            float dist = glm::distance(mParticles[i].pos, mParticles[j].pos);
            if (dist < 110.f) {
                float alphaMultiplier = (110.f - dist) / 110.f;
                float lineAlpha = 0.15f * alphaMultiplier * animation;
                ImColor lineCol = baseColor;
                lineCol.Value.w = lineAlpha;
                drawList->AddLine(ImVec2(mParticles[i].pos.x, mParticles[i].pos.y),
                                  ImVec2(mParticles[j].pos.x, mParticles[j].pos.y),
                                  lineCol, 1.0f);
            }
        }
    }
}

// SEARCH BAR IMPLEMENTATION
void ModernGui::renderSearchBar(ImVec2 screen, float animation, int themeIdx, float deltaTime) {
    float barW = 320.f;
    float barH = 40.f;
    float barX = screen.x / 2.f - barW / 2.f;
    float barY = 15.f;
    ImVec4 barRect = ImVec4(barX, barY, barX + barW, barY + barH);

    bool hover = isMouseOver(barRect);
    if (hover && ImGui::IsMouseClicked(0)) {
        mSearchFocused = true;
    } else if (ImGui::IsMouseClicked(0) && !hover) {
        mSearchFocused = false;
    }

    auto drawList = ImGui::GetBackgroundDrawList();
    ImRenderUtils::addBlur(barRect, 8.f * animation, 8.f, drawList);

    ImColor bgCol = getGlassColor(animation, false, false, false);
    ImColor borderCol = getAccentColor(0.f, mSearchFocused ? 0.8f * animation : 0.25f * animation, themeIdx);

    ImRenderUtils::fillRectangle(barRect, bgCol, animation, 8.f, drawList);
    ImRenderUtils::drawRoundRect(barRect, 0, 8.f, borderCol, animation, 1.5f);
    ImRenderUtils::fillShadowRectangle(barRect, ImColor(0, 0, 0), 0.25f * animation, 8.f, 0, 8.f, drawList);

    std::string icon = "[*] ";
    ImColor iconCol = getAccentColor(30.f, 0.75f * animation, themeIdx);
    float textHeight = ImGui::GetFont()->CalcTextSizeA(16.f * animation, FLT_MAX, -1, "").y;
    ImRenderUtils::drawText(ImVec2(barX + 12.f, barY + (barH - textHeight) / 2.f), icon, iconCol, 0.9f * animation, animation, true, 0, drawList);

    if (mSearchFocused) {
        static float backspaceTimer = 0.f;
        if (Keyboard::mPressedKeys[VK_BACK]) {
            if (backspaceTimer == 0.f || backspaceTimer > 0.4f) {
                if (!mSearchQuery.empty()) {
                    while (!mSearchQuery.empty()) {
                        unsigned char c = mSearchQuery.back();
                        mSearchQuery.pop_back();
                        if ((c & 0xC0) != 0x80) {
                            break;
                        }
                    }
                }
                if (backspaceTimer == 0.f) backspaceTimer = 0.01f;
                else backspaceTimer = 0.33f;
            }
            backspaceTimer += deltaTime;
        } else {
            backspaceTimer = 0.f;
        }

        ImGuiIO& io = ImGui::GetIO();
        for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
            ImWchar c = io.InputQueueCharacters[i];
            if (c > 0 && c < 0x10000 && mSearchQuery.length() < 32) {
                if (c == '\b') {
                    // Handled
                } else if (c == 27) {
                    mSearchFocused = false;
                } else if (c >= 32) {
                    if (c < 0x80) {
                        mSearchQuery += (char)c;
                    } else if (c < 0x800) {
                        mSearchQuery += (char)(0xC0 | (c >> 6));
                        mSearchQuery += (char)(0x80 | (c & 0x3F));
                    } else {
                        mSearchQuery += (char)(0xE0 | (c >> 12));
                        mSearchQuery += (char)(0x80 | ((c >> 6) & 0x3F));
                        mSearchQuery += (char)(0x80 | (c & 0x3F));
                    }
                }
            }
        }
        io.InputQueueCharacters.resize(0);
    }

    std::string dispText = mSearchQuery;
    ImColor txtCol = (themeIdx == 1) ? ImColor(40, 40, 45) : ImColor(240, 240, 245);
    if (dispText.empty()) {
        dispText = "Search Modules...";
        txtCol = (themeIdx == 1) ? ImColor(150, 150, 160) : ImColor(100, 100, 115);
    }
    txtCol.Value.w *= animation;

    ImRenderUtils::drawText(ImVec2(barX + 40.f, barY + (barH - textHeight) / 2.f), dispText, txtCol, 0.85f * animation, animation, true, 0, drawList);

    if (mSearchFocused) {
        float blinkSpeed = 5.f;
        float blinkAlpha = (std::sin(mGlobalTime * blinkSpeed) + 1.f) * 0.5f;
        float textW = ImRenderUtils::getTextWidth(&mSearchQuery, 0.85f * animation);
        float cursorX = barX + 40.f + textW + 2.f;
        float cursorY = barY + 10.f;
        float cursorH = barH - 20.f;

        ImColor cursorCol = getAccentColor(10.f, blinkAlpha * animation, themeIdx);
        drawList->AddRectFilled(ImVec2(cursorX, cursorY), ImVec2(cursorX + 2.f, cursorY + cursorH), cursorCol);
    }
}

// METRICS DASHBOARD IMPLEMENTATION
void ModernGui::renderDashboard(ImVec2 screen, float animation, float inScale, int themeIdx, float deltaTime) {
    float dashW = 260.f;
    float dashH = 120.f;
    float dashX = 20.f;
    float dashY = screen.y - dashH - 20.f;
    
    dashY = MathUtils::lerp(screen.y, dashY, inScale);
    ImVec4 dashRect = ImVec4(dashX, dashY, dashX + dashW, dashY + dashH);

    auto drawList = ImGui::GetBackgroundDrawList();
    ImRenderUtils::addBlur(dashRect, 8.f * animation, 8.f, drawList);

    ImColor bgCol = getGlassColor(animation, false, false, false);
    ImColor borderCol = getAccentColor(10.f, 0.35f * animation, themeIdx);

    ImRenderUtils::fillRectangle(dashRect, bgCol, animation, 10.f, drawList);
    ImRenderUtils::drawRoundRect(dashRect, 0, 10.f, borderCol, animation, 1.5f);
    ImRenderUtils::fillShadowRectangle(dashRect, ImColor(0, 0, 0), 0.3f * animation, 10.f, 0, 10.f, drawList);

    ImVec4 headerStrip = ImVec4(dashRect.x, dashRect.y, dashRect.z, dashRect.y + 4.f);
    ImRenderUtils::fillRectangle(headerStrip, getAccentColor(0.f, animation, themeIdx), animation, 1.5f, drawList);

    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);
    char timeStr[16];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &ltm);

    int activeCount = 0;
    for (const auto& mod : gFeatureManager->mModuleManager->mModules) {
        if (mod->mEnabled) activeCount++;
    }

    float fps = ImGui::GetIO().Framerate;
    char fpsStr[16];
    sprintf_s(fpsStr, "%.0f", fps);

    auto* nameProtect = gFeatureManager->mModuleManager->getModule<NameProtect>();
    std::string nick = "Player";
    if (nameProtect && nameProtect->mEnabled) {
        nick = nameProtect->mNewName;
    } else {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (player) nick = player->getLocalName();
    }

    ImColor labelCol = (themeIdx == 1) ? ImColor(110, 110, 120) : ImColor(140, 140, 155);
    ImColor valCol = (themeIdx == 1) ? ImColor(30, 30, 35) : ImColor(240, 240, 245);
    labelCol.Value.w *= animation;
    valCol.Value.w *= animation;

    float textHeight = ImGui::GetFont()->CalcTextSizeA(14.f * animation, FLT_MAX, -1, "").y;
    float rowY = dashRect.y + 12.f;
    float colXVal = dashRect.x + 130.f;

    auto drawRow = [&](const std::string& label, const std::string& val, float yOffset) {
        ImRenderUtils::drawText(ImVec2(dashRect.x + 14.f, yOffset), label, labelCol, 0.8f * animation, animation, true, 0, drawList);
        ImRenderUtils::drawText(ImVec2(colXVal, yOffset), val, valCol, 0.8f * animation, animation, true, 0, drawList);
    };

    drawRow("\xD0\x9F\xD0\x9E\xD0\x9B\xD0\xAC\xD0\x97\xD0\x9E\xD0\x92\xD0\x90\xD0\xA2\xD0\x95\xD0\x9B\xD0\xAC:", nick, rowY); rowY += textHeight + 8.f;
    drawRow("\xD0\x90\xD0\x9A\xD0\xA2\xD0\x98\xD0\x92\xD0\x9D\xD0\xAB\xD0\x95:", std::to_string(activeCount), rowY); rowY += textHeight + 8.f;
    drawRow("\xD0\x9A\xD0\x90\xD0\x94\xD0\xA0\xD0\xAB (FPS):", fpsStr, rowY); rowY += textHeight + 8.f;
    drawRow("\xD0\x92\xD0\xA0\xD0\x95\xD0\x9C\xD0\xAF:", timeStr, rowY);
}