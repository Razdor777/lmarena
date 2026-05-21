#include "FontHelper.hpp"
#include "Resources.hpp"

void FontHelper::load()
{
    ResourceLoader::loadResources();

    // Roboto есть в Resources.hpp как LOAD_RESOURCE, но CMake не создал ImFont — грузим вручную
    ImFontConfig font_config;
    font_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting;
    font_config.FontDataOwnedByAtlas = false;

    auto robotoRes = GET_RESOURCE(fonts_Roboto_Regular_ttf);
    FontHelper::Fonts.emplace("roboto", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        robotoRes.data2(), robotoRes.size(), 20, &font_config,
        ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()));
    FontHelper::Fonts.emplace("roboto_large", ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        robotoRes.data2(), robotoRes.size(), 42, &font_config,
        ImGui::GetIO().Fonts->GetGlyphRangesCyrillic()));
}

void FontHelper::setCurrentFont(const std::string& fontKey)
{
    if (Fonts.find(fontKey) != Fonts.end()) {
        currentFontKey = fontKey;
    }
}

void FontHelper::setFontScale(float scale)
{
    fontScale = std::clamp(scale, 0.5f, 2.0f);
}

float FontHelper::getScaledSize(float baseSize)
{
    return baseSize * fontScale;
}

void FontHelper::pushPrefFont(bool large, bool bold, bool mForcePSans)
{
    auto font = getFont(large, bold, mForcePSans);
    ImGui::PushFont(font);
}

ImFont* FontHelper::getFont(bool large, bool bold, bool mForcePSans)
{
    std::string baseKey;

    if (mForcePSans) {
        baseKey = "product_sans";
    } else {
        baseKey = currentFontKey;
    }

    if (bold) {
        std::string key = baseKey + "_bold";
        if (large) key += "_large";
        auto it = Fonts.find(key);
        if (it != Fonts.end() && it->second) {
            return it->second;
        }
    }

    std::string key = baseKey;
    if (large) key += "_large";
    auto it = Fonts.find(key);
    if (it != Fonts.end() && it->second) {
        return it->second;
    }

    // Фолбэк на roboto (кириллица гарантирована)
    key = large ? "roboto_large" : "roboto";
    it = Fonts.find(key);
    if (it != Fonts.end() && it->second) {
        return it->second;
    }

    // Последний фолбэк
    key = large ? "comfortaa_large" : "comfortaa";
    it = Fonts.find(key);
    if (it != Fonts.end() && it->second) {
        return it->second;
    }

    return nullptr;
}

void FontHelper::popPrefFont()
{
    ImGui::PopFont();
}