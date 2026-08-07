#include "FontHelper.hpp"
#include "Resources.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// FontHelper::load()
//
// Загружает ВСЕ шрифты с поддержкой кириллицы.
// Для шрифтов у которых нет кириллических глифов (Sarabun, Mojangles)
// используется MergeMode: сначала загружается основной шрифт для Latin/ASCII,
// затем Roboto «вмерживается» поверх него для кириллических символов.
// ─────────────────────────────────────────────────────────────────────────────

void FontHelper::load()
{
    ResourceLoader::loadResources();

    auto& io = ImGui::GetIO();

    // Базовая конфигурация
    ImFontConfig cfg;
    cfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
    cfg.FontDataOwnedByAtlas = false;

    // Roboto загружаем заранее — он нужен как источник кириллических глифов
    // при merge-загрузке других шрифтов
    auto robotoRes = GET_RESOURCE(fonts_Roboto_Regular_ttf);

    // Диапазоны символов для merge-режима кириллицы (0x0400–0x052F)
    static const ImWchar sCyrillicOnly[] = { 0x0400, 0x052F, 0x0000 };

    // ── Вспомогательные лямбды ────────────────────────────────────────────────

    // Загружает шрифт с полным кириллическим диапазоном
    // (для шрифтов у которых кириллица есть: Comfortaa, OpenSans, ProductSans, SFPro)
    auto addFull = [&](const std::string& key, const Resource& res, float size) -> ImFont* {
        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), size, &cfg,
            io.Fonts->GetGlyphRangesCyrillic());
        if (f) FontHelper::Fonts.emplace(key, f);
        return f;
    };

    // Загружает шрифт + подмешивает Roboto для кириллицы
    // (для шрифтов у которых нет кириллики: Mojangles, Sarabun)
    auto addWithCyrillicMerge = [&](const std::string& key, const Resource& res, float size) -> ImFont* {
        // Шаг 1: основной шрифт с Latin Extended диапазоном
        static const ImWchar sLatinExtended[] = { 0x0020, 0x024F, 0x0000 };
        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), size, &cfg, sLatinExtended);

        // Шаг 2: подмешиваем Roboto с кириллицей в тот же ImFont
        ImFontConfig mergeCfg;
        mergeCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        mergeCfg.FontDataOwnedByAtlas = false;
        mergeCfg.MergeMode            = true;
        io.Fonts->AddFontFromMemoryTTF(
            robotoRes.data2(), static_cast<int>(robotoRes.size()),
            size, &mergeCfg, sCyrillicOnly);

        if (f) FontHelper::Fonts.emplace(key, f);
        return f;
    };

    // ── Comfortaa ─────────────────────────────────────────────────────────────
    {
        auto res     = GET_RESOURCE(fonts_Comfortaa_ttf);
        auto resBold = GET_RESOURCE(fonts_Comfortaa_Bold_ttf);
        addFull("comfortaa",            res,     20.f);
        addFull("comfortaa_large",      res,     42.f);
        addFull("comfortaa_bold",       resBold, 20.f);
        addFull("comfortaa_bold_large", resBold, 42.f);
    }

    // ── Open Sans ─────────────────────────────────────────────────────────────
    {
        auto res     = GET_RESOURCE(fonts_Open_Sans_ttf);
        auto resBold = GET_RESOURCE(fonts_Open_Sans_Bold_ttf);
        addFull("open_sans",            res,     20.f);
        addFull("open_sans_large",      res,     42.f);
        addFull("open_sans_bold",       resBold, 20.f);
        addFull("open_sans_bold_large", resBold, 42.f);
    }

    // ── Product Sans ──────────────────────────────────────────────────────────
    // ВАЖНО: Product Sans — проприетарный шрифт Google, кириллики НЕТ в TTF!
    // Используем merge с Roboto чтобы кириллица работала в ClickGui
    // (ModernDropdown форсирует Product Sans через mForcePSans=true)
    {
        auto res     = GET_RESOURCE(fonts_Product_Sans_ttf);
        auto resBold = GET_RESOURCE(fonts_Product_Sans_Bold_ttf);
        addWithCyrillicMerge("product_sans",            res,     20.f);
        addWithCyrillicMerge("product_sans_large",      res,     42.f);
        addWithCyrillicMerge("product_sans_bold",       resBold, 20.f);
        addWithCyrillicMerge("product_sans_bold_large", resBold, 42.f);
    }

    // ── SF Pro Display ────────────────────────────────────────────────────────
    {
        auto res     = GET_RESOURCE(fonts_SF_Pro_Display_ttf);
        auto resBold = GET_RESOURCE(fonts_SF_Pro_Display_Bold_ttf);
        addFull("sf_pro_display",            res,     20.f);
        addFull("sf_pro_display_large",      res,     42.f);
        addFull("sf_pro_display_bold",       resBold, 20.f);
        addFull("sf_pro_display_bold_large", resBold, 42.f);
    }

    // ── Sarabun Light (нет кириллики → merge с Roboto) ────────────────────────
    {
        auto res = GET_RESOURCE(fonts_Sarabun_Light_ttf);
        addWithCyrillicMerge("sarabun_light",            res, 20.f);
        addWithCyrillicMerge("sarabun_light_large",      res, 42.f);
        // bold = тот же файл (отдельного bold Sarabun нет в ресурсах)
        addWithCyrillicMerge("sarabun_light_bold",       res, 20.f);
        addWithCyrillicMerge("sarabun_light_bold_large", res, 42.f);
    }

    // ── Mojangles (нет кириллики → merge с Roboto) ────────────────────────────
    {
        auto res     = GET_RESOURCE(fonts_Mojangles_ttf);
        auto resBold = GET_RESOURCE(fonts_Mojangles_Bold_ttf);
        addWithCyrillicMerge("mojangles",            res,     20.f);
        addWithCyrillicMerge("mojangles_large",      res,     42.f);
        addWithCyrillicMerge("mojangles_bold",       resBold, 20.f);
        addWithCyrillicMerge("mojangles_bold_large", resBold, 42.f);
    }

    // ── Roboto (дефолт + гарантированная кириллица) ───────────────────────────
    // Загружается ПОСЛЕДНИМ → ImGui выберет его как шрифт по умолчанию
    addFull("roboto",            robotoRes, 20.f);
    addFull("roboto_large",      robotoRes, 42.f);
    addFull("roboto_bold",       robotoRes, 20.f); // отдельного bold нет
    addFull("roboto_bold_large", robotoRes, 42.f);

    // ── Tenacity Icons (иконочный шрифт для ClickGui) ─────────────────────────
    // Загружаем только с базовым диапазоном (это icon font, кириллица не нужна)
    {
        auto res = GET_RESOURCE(fonts_Tenacity_Icons_ttf);
        static const ImWchar sIconRanges[] = { 0x0020, 0x00FF, 0xE000, 0xF8FF, 0x0000 };
        ImFontConfig iconCfg;
        iconCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        iconCfg.FontDataOwnedByAtlas = false;
        ImFont* iconFont = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), 20.f, &iconCfg, sIconRanges);
        if (iconFont) FontHelper::Fonts.emplace("tenacity_icons", iconFont);
        ImFont* iconFontLarge = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), 42.f, &iconCfg, sIconRanges);
        if (iconFontLarge) FontHelper::Fonts.emplace("tenacity_icons_large", iconFontLarge);
    }
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

    // Fallback на roboto (кириллица гарантирована)
    key = large ? "roboto_large" : "roboto";
    it = Fonts.find(key);
    if (it != Fonts.end() && it->second) {
        return it->second;
    }

    // Последний fallback — comfortaa
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