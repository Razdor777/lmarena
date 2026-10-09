#include "FontHelper.hpp"
#include "Resources.hpp"

#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// FontHelper (Dexko)
//
// У движкового оригинала тут грузились Mntsb + Nurik, но этих файлов в
// resources уже нет (репо с ними не линкуется). Dexko использует то, что
// реально лежит в resources/fonts:
//
//   Roboto-Regular.ttf — единственный шрифт Dexko. Полный набор кириллицы,
//   поэтому «донор глифов» не нужен — загрузка проще оригинала.
//
// Ключи сознательно те же, что и в оригинале ("mntsb", "mntsb_bold",
// "roboto", ...): Interface и FontHelper::getFont() продолжают работать
// без изменений.
// ─────────────────────────────────────────────────────────────────────────────

void FontHelper::load()
{
    ResourceLoader::loadResources();

    auto& io = ImGui::GetIO();

    auto baseRes = GET_RESOURCE(fonts_Roboto_Regular_ttf);

    auto addFont = [&](const std::string& key, float size) -> ImFont* {
        ImFontConfig cfg;
        cfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        cfg.FontDataOwnedByAtlas = false;

        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            baseRes.data2(), static_cast<int>(baseRes.size()), size, &cfg,
            io.Fonts->GetGlyphRangesCyrillic());

        if (f) FontHelper::Fonts[key] = f;
        return f;
    };

    // ── Интерфейсный шрифт (ключи "mntsb*" — совместимость с движком) ────────
    {
        ImFont* normal = addFont("mntsb", 20.f);
        ImFont* large  = addFont("mntsb_large", 42.f);

        if (normal) {
            Fonts["mntsb_bold"]       = normal;
            Fonts["mntsb_bold_large"] = large ? large : normal;
        }
    }

    // ── Служебный fallback ────────────────────────────────────────────────────
    addFont("roboto",       20.f);
    addFont("roboto_large", 42.f);
}

// ─────────────────────────────────────────────────────────────────────────────

ImFont* FontHelper::getIconFont(bool large)
{
    // Иконочные шрифты в Dexko не грузятся (нужны только ClickGui-модулям).
    // Функция оставлена для совместимости интерфейса: вызывающий код обязан
    // проверять nullptr.
    (void)large;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────

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
    // mForcePSans остался только для совместимости: Product Sans удалён.
    (void)mForcePSans;

    const std::string& baseKey = currentFontKey;

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

    // Fallback на сам интерфейсный шрифт
    key = "mntsb";
    if (large) key += "_large";
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
