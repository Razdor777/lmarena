#include "FontHelper.hpp"
#include "Resources.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

// ─────────────────────────────────────────────────────────────────────────────
// FontHelper::load()
//
// Клиент использует ровно ОДИН текстовый шрифт интерфейса. Шрифты берутся НЕ
// через GET_RESOURCE, а из Карты ресурсов по имени — потому что набор файлов в
// resources/fonts у каждой сборки свой:
//
//   * в одном дереве лежат mntsb.ttf (Montserrat Bold) и nurik.ttf;
//   * в другом их нет, зато есть полный набор старых шрифтов.
//
// GET_RESOURCE — это extern-ссылка на символ, который создаёт `ld -r -b binary`
// на этапе сборки. Если файла нет, CMake не генерирует для него LOAD_RESOURCE,
// и любое упоминание шрифта напрямую ломает компиляцию (error C2065). Поэтому
// здесь только рантайм-поиск: нет файла — нет шрифта, а не ошибка сборки.
//
// Что используется:
//
//   1. mntsb — «родной» интерфейсный шрифт (Montserrat Bold), если файл есть.
//   2. Roboto — запасной интерфейсный шрифт и одновременно донор глифов:
//      в него добирается кириллица и типографика, которых может не хватать.
//   3. Tenacity Icons + Nurik — ИКОНОЧНЫЕ шрифты (не текст!). Их глифы лежат на
//      ASCII-кодах, поэтому если выбрать их как шрифт интерфейса, весь текст
//      превращается в иконки — именно это и происходит, когда Nurik выбирался
//      в Font.
//
// ВАЖНО: ключи в FontHelper::Fonts именно присваиваются (Fonts[key] = f), а не
// добавляются через emplace. emplace не перезаписывает уже занятый ключ, а ключ
// умеет занимать и сгенерированный Resources.cpp.
//
// Если не нашлось ни одного файла шрифта, грузится встроенный шрифт ImGui: без
// него getFont() вернул бы nullptr, а PushFont(nullptr) роняет меню.
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
    // Ресурс по «дружественному» имени из сгенерированного Resources.cpp
    // (имя файла без расширения, в нижнем регистре, дефисы → подчёркивания).
    const Resource* findResource(const char* friendlyName)
    {
        auto it = ResourceLoader::Resources.find(friendlyName);
        if (it == ResourceLoader::Resources.end()) return nullptr;
        if (!it->second.data() || it->second.size() == 0) return nullptr;
        return &it->second;
    }
}

void FontHelper::load()
{
    ResourceLoader::loadResources();

    auto& io = ImGui::GetIO();

    const Resource* mntsbRes  = findResource("mntsb");
    const Resource* robotoRes = findResource("roboto_regular");

    // Основной интерфейсный шрифт: mntsb, если он есть, иначе Roboto.
    const Resource* primaryRes = mntsbRes ? mntsbRes : robotoRes;

    if (!mntsbRes)
    {
        spdlog::warn("[FontHelper] resources/fonts/mntsb.ttf not found — falling back to {}",
                     robotoRes ? "Roboto" : "built-in ImGui font");
    }
    if (!robotoRes)
    {
        spdlog::warn("[FontHelper] resources/fonts/Roboto-Regular.ttf not found — "
                     "Cyrillic glyph merging is disabled");
    }

    // Что добираем из Roboto: кириллица + типографика, которой нет в базовых
    // диапазонах Mntsb (тире, кавычки, многоточие, №, стрелки).
    // Всё, что уже есть, остаётся нетронутым.
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
    // mergeCyrillic = false, когда шрифт сам является донором (Roboto).
    // Без default-аргументов: у MSVC с ними в лямбдах бывают сюрпризы, а вызывать
    // всё равно приходится явно.
    auto addFont = [&](const std::string& key, const Resource* res, float size, bool mergeCyrillic) -> ImFont* {
        if (!res) return nullptr;

        ImFontConfig cfg;
        cfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        cfg.FontDataOwnedByAtlas = false;

        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            res->data2(), static_cast<int>(res->size()), size, &cfg,
            io.Fonts->GetGlyphRangesCyrillic());

        // Донора самого в себя не вмерживаем: это лишний проход по атласу.
        if (mergeCyrillic && robotoRes && res != robotoRes) {
            ImFontConfig mergeCfg;
            mergeCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
            mergeCfg.FontDataOwnedByAtlas = false;
            mergeCfg.MergeMode            = true;
            io.Fonts->AddFontFromMemoryTTF(
                robotoRes->data2(), static_cast<int>(robotoRes->size()),
                size, &mergeCfg, sCyrillicFallback);
        }

        if (f) FontHelper::Fonts[key] = f;
        return f;
    };

    // ── Основной интерфейсный шрифт ───────────────────────────────────────────
    // Montserrat Bold уже тяжёлый, поэтому «bold»-варианты — это те же самые
    // указатели: pushPrefFont(true, true) не должен проваливаться в fallback.
    {
        ImFont* normal = addFont("mntsb", primaryRes, 20.f, primaryRes != robotoRes);
        ImFont* large  = addFont("mntsb_large", primaryRes, 42.f, primaryRes != robotoRes);

        if (normal) {
            Fonts["mntsb_bold"]       = normal;
            Fonts["mntsb_bold_large"] = large ? large : normal;
        }
    }

    // ── Roboto — служебный fallback, не отображается в списке шрифтов ─────────
    if (robotoRes) {
        addFont("roboto",       robotoRes, 20.f, false);
        addFont("roboto_large", robotoRes, 42.f, false);
    }

    // ── Иконочные шрифты ──────────────────────────────────────────────────────
    // Это НЕ текстовые шрифты: их глифы сидят на ASCII-кодах. Грузим с базовым
    // диапазоном + Private Use Area, кириллица им не нужна.
    static const ImWchar sIconRanges[] = { 0x0020, 0x00FF, 0xE000, 0xF8FF, 0x0000 };

    auto addIconFont = [&](const std::string& key, const Resource* res) {
        if (!res) return;

        ImFontConfig iconCfg;
        iconCfg.FontBuilderFlags     = ImGuiFreeTypeBuilderFlags_NoHinting;
        iconCfg.FontDataOwnedByAtlas = false;

        // НЕ называть эти переменные small/big: windows.h (rpcndr.h)
        // определяет #define small char — и весь файл перестаёт компилироваться.
        ImFont* iconFont20 = io.Fonts->AddFontFromMemoryTTF(
            res->data2(), static_cast<int>(res->size()), 20.f, &iconCfg, sIconRanges);
        if (iconFont20) Fonts[key] = iconFont20;

        ImFont* iconFont42 = io.Fonts->AddFontFromMemoryTTF(
            res->data2(), static_cast<int>(res->size()), 42.f, &iconCfg, sIconRanges);
        if (iconFont42) Fonts[key + "_large"] = iconFont42;
    };

    // Tenacity Icons — иконки ClickGui (чекбоксы, стрелки, шестерёнки).
    addIconFont("tenacity_icons", findResource("tenacity_icons"));

    // Nurik — набор из 31 иконки (Glyphter/Fontello, CFF, буквы A-Z подменены
    // картинками), поэтому его нельзя выбрать как Font, только как иконки.
    addIconFont("nurik_icons", findResource("nurik"));

    // ── Страховка ─────────────────────────────────────────────────────────────
    // Ни одного .ttf в ресурсах: подставляем встроенный шрифт ImGui, чтобы
    // getFont() никогда не вернул nullptr и меню не упало на PushFont(nullptr).
    if (Fonts.find("mntsb") == Fonts.end())
    {
        ImFont* builtin = io.Fonts->AddFontDefault();
        if (builtin)
        {
            for (const char* key : { "mntsb", "mntsb_large", "mntsb_bold", "mntsb_bold_large",
                                     "roboto", "roboto_large" })
            {
                Fonts[key] = builtin;
            }
        }

        spdlog::error("[FontHelper] no .ttf resources found — using built-in ImGui font "
                      "(Cyrillic will not render)");
    }
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

    // Последний рубеж: любой загруженный шрифт. Пустой Fonts бывает только если
    // не сработал даже AddFontDefault — тогда вернуть нечего.
    for (auto& [name, font] : Fonts) {
        if (font) {
            currentFontKey = name;
            return font;
        }
    }

    return nullptr;
}

void FontHelper::popPrefFont()
{
    ImGui::PopFont();
}
