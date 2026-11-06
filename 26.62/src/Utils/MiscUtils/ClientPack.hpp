#pragma once
//
// ClientPack — установка своей папки текстур как ресурс-пака Bedrock.
//
// Зачем именно так. В 1.21.44 текстуры неба и кубемапы резолвятся обычным путём
// игры — по ресурс-пакам (см. RE_NOTES.md §2.3): загрузчик граней строит путь
// `textures/environment/<имя>_cubemap/cubemap_<i>`, а подсистема ресурсов
// спрашивает паки, есть ли такой путь. Значит кастомное небо можно получить
// БЕЗ детуров — достаточно положить свой пак с теми же путями.
//
// Принцип, которого здесь держимся: НИЧЕГО НЕ ВЫДУМЫВАЕМ ПРО ФОРМАТЫ.
//   * есть manifest.json в исходной папке — копируем его как есть; нет — пишем
//     свой минимальный, по официальной схеме;
//   * valid_known_packs.json существует — берём СХЕМУ из его первой записи и
//     добавляем такую же (меняем только path/uuid/version, если эти поля там
//     есть);
//   * global_resource_packs.json существует — так же зеркалим его шаблон, а
//     если файла нет — создаём его по схеме, снятой с живой установки 1.21.44
//     (массив из одной записи с полями pack_id и version), поля происхождения
//     там не участвуют;
//   * плюс прописываем пак в world_resource_packs.json самого свежего мира —
//     это применяется без перезапуска игры.
//
// ГДЕ ЛЕЖАТ СПИСКИ. Не в корне com.mojang, а в com.mojang/minecraftpe (проверено
// на живой установке: в корне com.mojang таких файлов нет вообще). Именно из-за
// этого прошлая версия молча ничего не регистрировала и вечно советовала
// «включи пак вручную». Оба места проверяются, приоритет у minecraftpe.
//
// Так пак не ломает настройки игры: перед изменением любого существующего файла
// делается .bak, а придуманных форматов не создаётся.
//
#include <filesystem>
#include <string>
#include <vector>

namespace ClientPack
{
    struct InstallRequest
    {
        std::string packName = "Solstice Sky";
        std::filesystem::path sourceFolder;        // что ставим
        std::filesystem::path comMojangOverride;   // пусто = найти автоматически
        // Грани cubemap_0..5 ставятся в overworld; end — только если попросили.
        // nether_cubemap игра не поддерживает, поэтому этого варианта здесь нет.
        bool includeEnd = false;
    };

    struct InstallResult
    {
        bool ok = false;
        std::string message;
        std::filesystem::path comMojang;
        std::filesystem::path packPath;
        int filesCopied = 0;
        bool registeredKnown = false;   // попал в valid_known_packs.json
        bool registeredGlobal = false;  // попал в global_resource_packs.json
        // Пак прописан в world_resource_packs.json миров — это путь, который
        // применяется БЕЗ перезапуска игры (списки читаются при загрузке мира).
        // Прописываем во ВСЕ миры, а не только в самый свежий: иначе пак не
        // появляется в том мире, куда игрок заходит на самом деле.
        bool registeredWorld = false;
        int worldCount = 0;             // в сколько миров прописались
        std::string worldName;          // первый из них (для уведомления)
        // Где реально лежат списки паков: на Windows это подпапка minecraftpe
        // внутри com.mojang (в корне com.mojang их нет вовсе).
        std::filesystem::path listFolder;
    };

    // ── Опции графики игры, без которых кубемапа небо НЕ рисует ────────────
    //
    // Это главная причина «пак поставил, а небо не поменялось», и она видна
    // только в minecraftpe/options.txt:
    //   * gfx_fancyskies:0      — небо рисуется плоским цветом, кубемапа
    //                             (и наш пак) не сэмплится вообще;
    //   * gfx_fancygraphics:0   — на Windows fancy skies идут вместе с ним;
    //   * gfx_raytracing:1      — рейтрейсинг/deferred сам рисует небо,
    //                             кубемапа из пака при этом не используется.
    // Поэтому перед установкой пака значения проверяются и (если разрешено)
    // правятся. Правка best-effort: файл игры, поэтому рядом остаётся .bak, а
    // если игра перезапишет его сама при выходе — то же самое можно включить
    // руками в Настройки → Видео.
    struct OptionsReport
    {
        bool ok = false;                       // options.txt найден и прочитан
        std::filesystem::path optionsPath;
        std::string fancySkiesOld;             // "0"/"1", пусто = ключа нет
        std::string fancyGraphicsOld;
        std::string rayTracingOld;
        bool fancySkiesChanged = false;
        bool fancyGraphicsChanged = false;
        bool rayTracingChanged = false;
        bool wrote = false;                    // файл перезаписан
        std::string message;

        bool rayTracingOn() const { return rayTracingOld == "1" || rayTracingOld == "true"; }
        bool fancySkiesOn() const { return fancySkiesOld == "1" || fancySkiesOld == "true"; }
        bool changed() const { return fancySkiesChanged || fancyGraphicsChanged || rayTracingChanged; }
    };

    // Включает fancy skies (и fancy graphics заодно), при желании гасит
    // рейтрейсинг. Ничего не выдумывает: ключей нет — создаёт их, значения
    // остальных байт файла не трогает.
    OptionsReport ensureSkyOptions(const std::filesystem::path& comMojangOverride,
                                   bool enableFancyGraphics,
                                   bool disableRayTracing);

    // Ищет папку данных Bedrock (com.mojang) среди известных пакетов Windows.
    // Пусто — не нашли; модуль это покажет и предложит вписать путь вручную.
    std::filesystem::path findComMojang();

    // Папка по умолчанию, которую модуль предлагает пользователю.
    std::filesystem::path defaultSourceFolder();

    // Все миры с level.dat (обычно это 1-3 папки в minecraftWorlds).
    std::vector<std::filesystem::path> worldDirs(const std::filesystem::path& comMojang);

    InstallResult install(const InstallRequest& request);

    // Убирает наш пак: записи в json-файлах и саму папку пака.
    bool uninstall(const std::string& packName,
                   const std::filesystem::path& comMojangOverride,
                   std::string* messageOut);
}
