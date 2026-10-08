#include "FontHelper.hpp"
#include "Resources.hpp"

#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// FontHelper::load()
//
// Клиент использует ровно ОДИН текстовый шрифт — Mntsb (Montserrat Bold).
// Все остальные декоративные шрифты (Comfortaa, Mojangles, Open Sans,
// Product Sans, SF Pro, Sarabun, Roboto Display и т.д.) удалены из resources —
// они только раздували атлас и ломали кириллицу.
//
// Осталось три вещи, и ни одна из них не выбирается в меню как «шрифт»:
//
//   1. Mntsb — сам интерфейсный шрифт. Он уже содержит кириллицу, но
//      нескольких букв/знаков в нём нет, поэтому в него дополнительно
//      вмерживается Roboto (см. ниже).
//   2. Roboto — «донор» глифов. НЕ показывается в списке шрифтов, нужен
//      только чтобы в Mntsb гарантированно были все кириллические буквы
//      и типографика (тире, кавычки, многоточие, №).
//   3. Tenacity Icons + Nurik — ИКОНОЧНЫЕ шрифты (не текст!). Их глифы
//      лежат на ASCII-кодах, поэтому если выбрать их как шрифт интерфейса,
//      весь текст превращается в иконки — именно это и происходит, когда
//      Nurik выбирался в Font.
//
// ВАЖНО: ключи в FontHelper::Fonts именно присваиваются (Fonts[key] = f), а не
// добавляются через emplace. emplace не перезаписывает уже занятый ключ, а ключ
// умеет занимать и сгенерированный Resources.cpp.
// ─────────────────────────────────────────────────────────────────────────────

void FontHelper::load()
{
    ResourceLoader::loadResources();

    auto& io = ImGui::GetIO();

    auto mntsbRes  = GET_RESOURCE(fonts_mntsb_ttf);
    // Roboto — единственный оставшийся «служебный» шрифт: поставщик
    // кириллических глифов для Mntsb. В списке шрифтов его нет.
    auto robotoRes = GET_RESOURCE(fonts_Roboto_Regular_ttf);

    // Что добираем из Roboto: кириллица + типографика, которой нет в базовых
    // диапазонах Mntsb (тире, кавычки, многоточие, №, стрелки).
    // Всё, что в Mntsb уже есть, остаётся нетронутым.
    static const ImWchar sCyrillicFallback[] = {
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0x2000, 0x206F, // General punctuation (dashes, quotes, ellipsis)
        0x2116, 0x2116, // №
        0x2190, 0x2193, // Arrows
        0x0000,
    };

    // Загружает шрифт и регистрирует его под ключом key.
    // mergeCyrillic = false только для самого Roboto (он уже источник).
    // Без default-аргументов: у MSVC с ними в лямбдах бывают сюрпризы, а вызывать
    // всё равно приходится явно.
    auto addFont = [&](const std::string& key, const Resource& res, float size, bool mergeCyrillic) -> ImFont* {
        ImFontConfig cfg;
        cfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        cfg.FontDataOwnedByAtlas = false;

        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), size, &cfg,
            io.Fonts->GetGlyphRangesCyrillic());

        if (mergeCyrillic) {
            ImFontConfig mergeCfg;
            mergeCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
            mergeCfg.FontDataOwnedByAtlas = false;
            mergeCfg.MergeMode            = true;
            io.Fonts->AddFontFromMemoryTTF(
                robotoRes.data2(), static_cast<int>(robotoRes.size()),
                size, &mergeCfg, sCyrillicFallback);
        }

        if (f) FontHelper::Fonts[key] = f;
        return f;
    };

    // ── Mntsb — единственный интерфейсный шрифт ───────────────────────────────
    // Montserrat Bold уже тяжёлый, поэтому «bold»-варианты — это те же самые
    // указатели: pushPrefFont(true, true) не должен проваливаться в fallback.
    {
        ImFont* normal = addFont("mntsb", mntsbRes, 20.f, true);
        ImFont* large  = addFont("mntsb_large", mntsbRes, 42.f, true);

        if (normal) {
            Fonts["mntsb_bold"]       = normal;
            Fonts["mntsb_bold_large"] = large ? large : normal;
        }
    }

    // ── Roboto — служебный fallback, не отображается в списке шрифтов ─────────
    addFont("roboto",       robotoRes, 20.f, false);
    addFont("roboto_large", robotoRes, 42.f, false);

    // ── Иконочные шрифты ──────────────────────────────────────────────────────
    // Это НЕ текстовые шрифты: их глифы сидят на ASCII-кодах. Грузим с базовым
    // диапазоном + Private Use Area, кириллица им не нужна.
    static const ImWchar sIconRanges[] = { 0x0020, 0x00FF, 0xE000, 0xF8FF, 0x0000 };

    auto addIconFont = [&](const std::string& key, const Resource& res) {
        ImFontConfig iconCfg;
        iconCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        iconCfg.FontDataOwnedByAtlas = false;

        // НЕ называть эти переменные small/big: windows.h (rpcndr.h)
        // определяет #define small char — и весь файл перестаёт компилироваться.
        ImFont* iconFont20 = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), 20.f, &iconCfg, sIconRanges);
        if (iconFont20) Fonts[key] = iconFont20;

        ImFont* iconFont42 = io.Fonts->AddFontFromMemoryTTF(
            res.data2(), static_cast<int>(res.size()), 42.f, &iconCfg, sIconRanges);
        if (iconFont42) Fonts[key + "_large"] = iconFont42;
    };

    // Tenacity Icons — иконки ClickGui (чекбоксы, стрелки, шестерёнки).
    addIconFont("tenacity_icons", GET_RESOURCE(fonts_Tenacity_Icons_ttf));

    // Nurik — оказался НЕ текстовым шрифтом, а набором из 31 иконки, сгенерированным
    // в Glyphter/Fontello (CFF, без цифр и пробела, буквы A-Z подменены картинками).
    // Поэтому его больше нельзя выбрать как Font, но сам файл оставлен и теперь
    // доступен как иконочный шрифт "nurik_icons".
    addIconFont("nurik_icons", GET_RESOURCE(fonts_nurik_ttf));
}

// ─────────────────────────────────────────────────────────────────────────────

ImFont* FontHelper::getIconFont(bool large)
{
    const std::string key = large ? "nurik_icons_large" : "nurik_icons";
    auto it = Fonts.find(key);
    return (it != Fonts.end()) ? it->second : nullptr;
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
    // mForcePSans остался только для совместимости с ModernDropdown: Product Sans
    // удалён, поэтому «форсированный» шрифт теперь такой же, как обычный.
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

    // Fallback на сам интерфейсный шрифт — кириллица в нём гарантирована merge'ом.
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
