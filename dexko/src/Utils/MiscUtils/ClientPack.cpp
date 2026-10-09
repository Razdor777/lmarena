#include "ClientPack.hpp"

#include <Utils/FileUtils.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// ── ПОЧЕМУ У ПОМОЩНИКОВ ПРЕФИКС `pack` ────────────────────────────────────
// Проект собирается с CMAKE_UNITY_BUILD: несколько .cpp склеиваются в одну
// единицу трансляции, и имена из анонимного пространства имён становятся
// видимыми в глобальной области. Слишком общие имена (`toLower`, `copyTree`,
// `readJsonArray`) могут совпасть с чужими из той же склейки, а компилятор на
// такое отвечает «reference to ... is ambiguous» — проверено на мини-проекте,
// см. .freebuff/tmp-unity. Префикс `pack` делает совпадение невозможным.
// Псевдоним `json` оставлен как есть: в проекте он в таком виде больше нигде не
// объявляется, а переименовывать его пришлось бы во всех использованиях.
namespace
{
    using json = nlohmann::json;

    constexpr const char* kPackKnownFile = "valid_known_packs.json";
    constexpr const char* kPackGlobalFile = "global_resource_packs.json";

    // Пакеты Bedrock на Windows. Порядок = приоритет поиска: сначала обычная
    // игра, потом бета и предпросмотр.
    const char* kPackFolders[] = {
        "Microsoft.MinecraftUWP_8wekyb3d8bbwe",
        "Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe",
        "Microsoft.MinecraftPreview_8wekyb3d8bbwe",
    };

    // Для сопоставления имён без учёта регистра: Windows-файловая система
    // регистр прощает, а игра — нет (внутри пака поиск по индексу), поэтому
    // "Cubemap_0.PNG" от пользователя надо принять, а положить всё равно
    // строчными именами.
    std::string packToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Имя папки пака: только безопасные символы, иначе файловая система
    // Windows может отказать (пробелы допустимы, но лучше без них).
    std::string packFolderName(const std::string& name)
    {
        std::string out;
        out.reserve(name.size());
        for (char c : name)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if (std::isalnum(u) || c == '_' || c == '-')
                out += c;
            else if (c == ' ' || c == '.')
                out += '_';
        }
        if (out.empty()) out = "SolsticePack";
        return out;
    }

    // Детерминированный UUID из имени пака: один и тот же пак при переустановке
    // даёт один и тот же uuid, поэтому в списках игры не появляются дубликаты.
    std::string packUuid(const std::string& name)
    {
        auto fnv1 = [](const std::string& s, uint64_t basis)
        {
            uint64_t h = basis;
            for (unsigned char c : s)
            {
                h *= 0x100000001B3ull;   // тот же прайм, что в игре (см. RE_NOTES §2.4)
                h ^= c;
            }
            return h;
        };

        const uint64_t a = fnv1(name, 0xCBF29CE484222325ull);
        const uint64_t b = fnv1(name + "|solstice", a);

        char buf[64] = {};
        snprintf(buf, sizeof(buf),
                 "%08X-%04X-%04X-%04X-%012llX",
                 static_cast<unsigned>(a >> 32) & 0xFFFFFFFFu,
                 static_cast<unsigned>(a >> 16) & 0xFFFFu,
                 static_cast<unsigned>(a) & 0xFFFFu,
                 static_cast<unsigned>(b >> 48) & 0xFFFFu,
                 static_cast<unsigned long long>(b & 0xFFFFFFFFFFFFull));
        return buf;
    }

    bool packCopyFile(const std::filesystem::path& from, const std::filesystem::path& to, int* counter)
    {
        std::error_code ec;
        std::filesystem::create_directories(to.parent_path(), ec);
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            spdlog::warn("[ClientPack] не скопировал {} -> {}: {}", from.string(), to.string(), ec.message());
            return false;
        }
        if (counter) ++(*counter);
        return true;
    }

    bool packCopyTree(const std::filesystem::path& from, const std::filesystem::path& to, int* counter)
    {
        std::error_code ec;
        if (!std::filesystem::exists(from, ec)) return false;

        // Пути в Bedrock-ресурсах строчные: внутри архива поиск идёт по
        // индексу, а не по файловой системе Windows, поэтому "Clouds.PNG"
        // и "clouds.png" для игры — разные пути. Проверяем остаток пути и
        // предупреждаем: это самая частая причина «пак поставил, а небо не
        // поменялось», и по логу она видна сразу.
        int upperCasePaths = 0;
        std::vector<std::string> examples;

        for (std::filesystem::recursive_directory_iterator it(from, ec), end; it != end && !ec; it.increment(ec))
        {
            if (!it->is_regular_file(ec)) continue;
            const auto relative = std::filesystem::relative(it->path(), from, ec);
            if (ec) continue;

            const std::string rel = relative.generic_string();
            if (std::any_of(rel.begin(), rel.end(), [](unsigned char c) { return std::isupper(c) != 0; }))
            {
                ++upperCasePaths;
                if (examples.size() < 3) examples.push_back(rel);
            }

            packCopyFile(it->path(), to / relative, counter);
        }

        if (upperCasePaths > 0)
        {
            spdlog::warn("[ClientPack] в копируемом дереве {} путь(ей) с заглавными буквами, например: {} — "
                         "в Bedrock путь чувствителен к регистру, файл может не найтись",
                         upperCasePaths, examples.empty() ? std::string("?") : examples.front());
        }

        return true;
    }

    std::string packReadWholeFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return {};
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }

    // Читает json-массив. Не массив, битый файл или пусто -> nullopt: в этом
    // случае мы НЕ пишем файл вообще, чтобы не испортить настройки игры.
    std::optional<json> packReadJsonArray(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path)) return std::nullopt;

        const std::string text = packReadWholeFile(path);
        if (text.empty()) return std::nullopt;

        try
        {
            json parsed = json::parse(text);
            if (!parsed.is_array() || parsed.empty()) return std::nullopt;
            if (!parsed.front().is_object()) return std::nullopt;
            return parsed;
        }
        catch (const std::exception& e)
        {
            spdlog::warn("[ClientPack] {} не читается как json ({}), файл не трогаем",
                         path.string(), e.what());
            return std::nullopt;
        }
    }

    // Та же логика, что у json-карты, но для текстовых файлов игры
    // (options.txt): сначала .bak, потом запись.
    bool packWriteWholeFile(const std::filesystem::path& path, const std::string& text)
    {
        std::error_code ec;

        if (std::filesystem::exists(path, ec))
        {
            std::filesystem::path backup = path;
            backup += ".solstice.bak";
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing, ec);
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        return out.good();
    }

    bool packWriteJsonArray(const std::filesystem::path& path, const json& value)
    {
        std::error_code ec;

        // Бэкап: если что-то пойдёт не так, настройки игры восстановимы.
        if (std::filesystem::exists(path, ec))
        {
            std::filesystem::path backup = path;
            backup += ".solstice.bak";
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing, ec);
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << value.dump(2);
        return out.good();
    }

    // Версия в паке бывает и строкой, и массивом — повторяем ТОТ ЖЕ тип, что в
    // схеме игры, чтобы не сломать разбор.
    void packMirrorVersion(json& entry, const char* key)
    {
        if (!entry.contains(key)) return;

        if (entry[key].is_array()) entry[key] = json::array({ 1, 0, 0 });
        else if (entry[key].is_string()) entry[key] = "1.0.0";
    }

    // Запись описывает ВСТРОЕННЫЙ пак игры (vanilla и подобные), а не папку на
    // диске. Такая запись годится как шаблон схемы, но её поля происхождения
    // нам переносить нельзя — см. packDropOriginKeys.
    bool packLooksBuiltIn(const json& entry)
    {
        if (!entry.is_object()) return true;

        if (entry.contains("file_system") && entry["file_system"].is_string())
        {
            const std::string value = packToLower(entry["file_system"].get<std::string>());
            if (value == "raw" || value == "builtin" || value == "built_in" || value == "system")
                return true;
        }

        if (entry.contains("path") && entry["path"].is_string())
        {
            const std::string path = packToLower(entry["path"].get<std::string>());
            if (path.find("vanilla") != std::string::npos) return true;
        }

        return false;
    }

    // Поля, которые описывают ОТКУДА пак взялся, а не что в нём лежит. Наш пак —
    // обычная папка в resource_packs, поэтому пометку встроенного пака
    // (file_system: "Raw" и подобное) переносить нельзя: с ней игра пойдёт
    // искать нас внутри своих встроенных ресурсов, а не на диске.
    void packDropOriginKeys(json& entry)
    {
        for (const char* key : { "file_system", "is_trusted", "has_scripts" })
            entry.erase(key);   // отсутствующий ключ — no-op
    }

    // Где на самом деле лежат списки паков.
    //
    // Это была вторая причина, по которой пак «устанавливался, но не работал»:
    // мы искали global_resource_packs.json и valid_known_packs.json в КОРНЕ
    // com.mojang, а на живой установке 1.21.44 они лежат в com.mojang/minecraftpe
    // (в корне таких файлов нет вовсе). Итог — регистрация молча пропускалась,
    // а модуль вечно советовал «включи пак вручную».
    //
    // Проверяем оба места (приоритет у minecraftpe, то есть у того, что реально
    // используется), а если файла нет нигде — возвращаем путь рядом с настройками
    // игры, чтобы его можно было создать.
    std::filesystem::path packListFile(const std::filesystem::path& comMojang, const char* fileName)
    {
        std::error_code ec;

        const auto nearSettings = comMojang / "minecraftpe" / fileName;
        if (std::filesystem::exists(nearSettings, ec)) return nearSettings;

        const auto inRoot = comMojang / fileName;
        if (std::filesystem::exists(inRoot, ec)) return inRoot;

        return nearSettings;
    }

    // Все миры с level.dat, отсортированные по времени level.dat (свежий первый).
    //
    // Раньше пак прописывался только в самый свежий мир, и это была отдельная
    // причина «не работает»: игрок заходит в другой мир — там нашего пака нет.
    // Миров обычно один-два, поэтому прописываемся во все сразу.
    std::vector<std::filesystem::path> packWorldDirs(const std::filesystem::path& comMojang)
    {
        std::error_code ec;
        std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> found;

        const auto worlds = comMojang / "minecraftWorlds";
        if (!std::filesystem::exists(worlds, ec)) return {};

        for (const auto& entry : std::filesystem::directory_iterator(worlds, ec))
        {
            if (!entry.is_directory(ec)) continue;

            const auto level = entry.path() / "level.dat";
            if (!std::filesystem::exists(level, ec)) continue;

            found.emplace_back(std::filesystem::last_write_time(level, ec), entry.path());
        }

        std::sort(found.begin(), found.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        std::vector<std::filesystem::path> result;
        result.reserve(found.size());
        for (auto& [time, path] : found) result.push_back(std::move(path));
        return result;
    }

    // Прописывает пак в конкретном мире (world_resource_packs.json).
    //
    // Зачем, если есть global_resource_packs.json: глобальный список игра читает
    // на СТАРТЕ, то есть после установки нужен перезапуск. Список мира читается
    // при ЗАГРУЗКЕ мира — достаточно выйти в меню и зайти обратно, а если пак
    // ставится в момент, когда мир уже открыт, он подхватится на следующем входе.
    bool packAddToWorld(const std::filesystem::path& world, const std::string& uuid,
                        std::string* worldNameOut)
    {
        std::error_code ecWorld;
        if (world.empty() || !std::filesystem::exists(world / "level.dat", ecWorld)) return false;

        const auto path = world / "world_resource_packs.json";

        json array = json::array();
        if (auto existing = packReadJsonArray(path)) array = *existing;

        const std::string want = packToLower(uuid);
        for (const auto& entry : array)
        {
            if (!entry.is_object()) continue;
            if (entry.contains("pack_id") && entry["pack_id"].is_string() &&
                packToLower(entry["pack_id"].get<std::string>()) == want)
            {
                if (worldNameOut) *worldNameOut = world.filename().string();
                return true;   // уже прописан в этом мире
            }
        }

        json entry;
        entry["pack_id"] = uuid;
        entry["version"] = json::array({ 1, 0, 0 });
        array.push_back(entry);

        if (!packWriteJsonArray(path, array)) return false;

        if (worldNameOut) *worldNameOut = world.filename().string();
        return true;
    }

    // Тот же пак — во ВСЕ миры.
    int packAddToAllWorlds(const std::filesystem::path& comMojang, const std::string& uuid,
                           std::string* firstNameOut)
    {
        int count = 0;
        for (const auto& world : packWorldDirs(comMojang))
        {
            std::string name;
            if (!packAddToWorld(world, uuid, &name)) continue;

            if (count == 0 && firstNameOut) *firstNameOut = name;
            ++count;
        }
        return count;
    }

    // ── options.txt ──────────────────────────────────────────────────────
    // Ищем ключ ровно как строку «начало строки + ключ + двоеточие», поэтому
    // gfx_toggleclouds не совпадёт с gfx_toggleclouds_x, а значения других
    // настроек остаются байт-в-байт прежними (включая переводы строк: у файла
    // игры может быть CRLF, и переписывать его целиком нельзя).
    bool packFindOption(const std::string& text, const char* key,
                        size_t* valueStart, size_t* valueEnd)
    {
        const size_t keyLen = std::strlen(key);
        size_t pos = 0;

        while ((pos = text.find(key, pos)) != std::string::npos)
        {
            const bool atLineStart = pos == 0 || text[pos - 1] == '\n' || text[pos - 1] == '\r';
            const size_t afterKey = pos + keyLen;

            if (atLineStart && afterKey < text.size() && text[afterKey] == ':')
            {
                const size_t begin = afterKey + 1;
                size_t end = text.find_first_of("\r\n", begin);
                if (end == std::string::npos) end = text.size();

                if (valueStart) *valueStart = begin;
                if (valueEnd) *valueEnd = end;
                return true;
            }

            pos = afterKey;
        }

        return false;
    }
}

namespace ClientPack
{
    std::filesystem::path findComMojang()
    {
        std::error_code ec;

        // ── 1. Тот же пакет, в который заинжекчен клиент ────────────────────
        // Данные клиента лежат в RoamingState пакета игры
        // (см. FileUtils::getRoamingStatePath), а com.mojang — в LocalState
        // того же пакета, то есть рядом. Это самый надёжный путь: он не
        // зависит ни от LOCALAPPDATA, ни от имени пакета — если клиент
        // запущен, значит пакет существует с гарантией.
        //
        // Внимание на арифметику пути: getRoamingStatePath() отдаёт строку с
        // ВИСЯЩИМ разделителем в конце ("...\\RoamingState\\"), и один
        // parent_path() убирает только этот разделитель, а не сам каталог.
        // Проверено: path("a\\RoamingState\\").parent_path() == "a\\RoamingState".
        // Поэтому до пакета добираемся ДВУМЯ parent_path().
        //
        // Про пакет: имя пакета здесь то же, в которое пишет свои логи сам
        // клиент (FileUtils::getJuzdexDir строится из той же строки), так что
        // если клиент работает — каталог существует. Это верно для обычного
        // UWP-пакета; Beta/Preview отсеются проверкой существования ниже и
        // будут найдены перебором.
        try
        {
            const std::filesystem::path roaming(FileUtils::getRoamingStatePath());
            if (!roaming.empty())
            {
                const auto packageRoot = roaming.parent_path().parent_path();
                const auto candidate = packageRoot / "LocalState" / "games" / "com.mojang";
                if (std::filesystem::exists(candidate / "resource_packs", ec) ||
                    std::filesystem::exists(candidate / "minecraftWorlds", ec))
                {
                    return candidate;
                }
            }
        }
        catch (...)
        {
            // падаём на обычный поиск ниже
        }

        // ── 2. Перебор известных имён пакетов ───────────────────────────────
        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (!localAppData) return {};

        for (const char* package : kPackFolders)
        {
            std::filesystem::path candidate = std::filesystem::path(localAppData) / "Packages" / package /
                                              "LocalState" / "games" / "com.mojang";
            if (std::filesystem::exists(candidate / "resource_packs", ec) ||
                std::filesystem::exists(candidate / "minecraftWorlds", ec))
            {
                return candidate;
            }
        }

        return {};
    }

    std::filesystem::path defaultSourceFolder()
    {
        try
        {
            return std::filesystem::path(FileUtils::getSolsticeDir()) / "Sky";
        }
        catch (...)
        {
            return {};
        }
    }

    InstallResult install(const InstallRequest& request)
    {
        InstallResult result;

        try
        {
            // ── 1. Куда ставить ────────────────────────────────────────────────
            result.comMojang = request.comMojangOverride.empty() ? findComMojang()
                                                                 : request.comMojangOverride;
            if (result.comMojang.empty())
            {
                result.message = "Не нашёл папку com.mojang. Впиши путь вручную в настройке «com.mojang Path».";
                return result;
            }
            if (!std::filesystem::exists(result.comMojang))
            {
                result.message = "Путь com.mojang не существует: " + result.comMojang.string();
                return result;
            }

            if (request.sourceFolder.empty() || !std::filesystem::exists(request.sourceFolder))
            {
                result.message = "Папка с текстурами не найдена: " + request.sourceFolder.string() +
                                 "\nПоложи туда cubemap_0..5.png (грани) или готовое дерево textures\\...";
                return result;
            }

            std::error_code ec;

            // Один проход по исходной папке: имена без учёта регистра, чтобы
            // пользователь мог положить Cubemap_0.PNG и это сработало.
            std::unordered_map<std::string, std::filesystem::path> rootFiles;
            std::filesystem::path texturesDir;
            for (const auto& entry : std::filesystem::directory_iterator(request.sourceFolder, ec))
            {
                const std::string name = entry.path().filename().string();
                if (entry.is_directory(ec))
                {
                    if (packToLower(name) == "textures")
                        texturesDir = entry.path();
                    continue;
                }
                if (entry.is_regular_file(ec))
                    rootFiles.emplace(packToLower(name), entry.path());
            }

            if (!rootFiles.empty() || !texturesDir.empty())
            {
                std::string listed;
                size_t shown = 0;
                for (const auto& [name, path] : rootFiles)
                {
                    if (shown++ >= 8) { listed += "..."; break; }
                    listed += name + " ";
                }
                spdlog::info("[ClientPack] в {}{} файлов: {}", request.sourceFolder.string(),
                             texturesDir.empty() ? "" : " (+ папка textures)", listed);
            }

            const std::string folder = packFolderName(request.packName);
            result.packPath = result.comMojang / "resource_packs" / folder;
            spdlog::info("[ClientPack] целевой пак: {} (com.mojang = {})", result.packPath.string(),
                         result.comMojang.string());

            std::filesystem::create_directories(result.packPath, ec);
            if (ec)
            {
                result.message = "Не смог создать папку пака: " + ec.message();
                return result;
            }

            int copied = 0;

            // ── 2. Грани кубемапы из корня папки ──────────────────────────────
            // Игра просит пути textures/environment/<имя>_cubemap/cubemap_N.png,
            // имя окружения берётся из объекта окружения (RE_NOTES §2.2).
            //
            // Только overworld и (по желанию) end. nether_cubemap игра не
            // поддерживает — этого нет в ванильном паке, и в трекере Mojang
            // висит открытая просьба «разрешить менять nether кубемапу так же,
            // как это уже работает для end и overworld». Писать в
            // nether_cubemap бессмысленно: файлы займут место, а эндерское
            // небо в аду всё равно не изменится.
            std::vector<std::string> envs = { "overworld" };
            if (request.includeEnd)
                envs.push_back("end");

            int facesFound = 0;
            for (const auto& env : envs)
            {
                for (int face = 0; face < 6; ++face)
                {
                    const auto found = rootFiles.find("cubemap_" + std::to_string(face) + ".png");
                    if (found == rootFiles.end()) continue;

                    const auto target = result.packPath / "textures" / "environment" /
                                        (env + "_cubemap") / ("cubemap_" + std::to_string(face) + ".png");
                    if (packCopyFile(found->second, target, &copied) && env == "overworld") ++facesFound;
                }
            }

            // ── 3. Готовое дерево textures/... копируем как есть ─────────────
            // Так поддерживается что угодно (облака, звёзды, скайплейн, меню),
            // без угадывания путей на нашей стороне.
            bool treeCopied = false;
            if (!texturesDir.empty())
                treeCopied = packCopyTree(texturesDir, result.packPath / "textures", &copied);

            // ── 4. manifest ──────────────────────────────────────────────────
            const auto targetManifest = result.packPath / "manifest.json";
            const auto manifestIt = rootFiles.find("manifest.json");
            const auto iconIt = rootFiles.find("pack_icon.png");

            std::filesystem::create_directories(result.packPath, ec);
            if (manifestIt != rootFiles.end())
            {
                packCopyFile(manifestIt->second, targetManifest, &copied);
            }
            else
            {
                json header;
                header["name"] = request.packName;
                header["description"] = "Solstice client pack";
                header["uuid"] = packUuid(request.packName);
                header["version"] = json::array({ 1, 0, 0 });
                header["min_engine_version"] = json::array({ 1, 21, 0 });

                json module;
                module["description"] = "Solstice textures";
                module["type"] = "resources";
                module["uuid"] = packUuid(request.packName + "|module");
                module["version"] = json::array({ 1, 0, 0 });

                json manifest;
                manifest["format_version"] = 2;
                manifest["header"] = header;
                manifest["modules"] = json::array({ module });

                std::ofstream out(targetManifest, std::ios::binary | std::ios::trunc);
                if (out) { out << manifest.dump(2); ++copied; }
            }

            if (iconIt != rootFiles.end())
                packCopyFile(iconIt->second, result.packPath / "pack_icon.png", &copied);

            result.filesCopied = copied;

            if (copied == 0)
            {
                result.message = "В папке " + request.sourceFolder.string() +
                                 " нет ни cubemap_0..5.png, ни textures\\... — устанавливать нечего.";
                return result;
            }

            // ── 5. Регистрация в списках игры (строго по её же схеме) ────────
            // Схему не выдумываем, а берём запись из файла самой игры — но не
            // любую. В начале списка обычно лежат ВСТРОЕННЫЕ паки (vanilla), и у
            // них есть поля происхождения (file_system: "Raw" и подобное),
            // которых у папки на диске быть не должно: с такой пометкой игра
            // пойдёт искать наш пак среди встроенных ресурсов и не найдёт.
            // Поэтому шаблоном выбираем запись, НЕ помеченную как встроенная, а
            // если такой нет — берём встроенную, но без полей происхождения.
            // Что именно мы положили, печатаем в лог: если пак не появится в
            // списке, эта строка и будет разгадкой.
            const std::string uuid = packUuid(request.packName);
            const std::string relPath = "resource_packs/" + folder;

            const auto knownPath = packListFile(result.comMojang, kPackKnownFile);
            if (auto known = packReadJsonArray(knownPath))
            {
                const json* tmpl = nullptr;
                for (const auto& e : *known)
                {
                    if (e.is_object() && e.contains("path") && !packLooksBuiltIn(e)) { tmpl = &e; break; }
                }

                const bool builtInTemplate = (tmpl == nullptr);
                if (!tmpl)
                {
                    for (const auto& e : *known)
                    {
                        if (e.is_object() && e.contains("path")) { tmpl = &e; break; }
                    }
                }

                if (!tmpl)
                {
                    // Ни одной записи с "path" — схему не угадываем вообще.
                    spdlog::warn("[ClientPack] в {} нет записи с полем path — формат не разбираю, "
                                 "включи пак вручную в Global Resources",
                                 kPackKnownFile);
                }
                else
                {
                    json entry = *tmpl;
                    if (builtInTemplate) packDropOriginKeys(entry);

                    entry["path"] = relPath;
                    entry["uuid"] = uuid;
                    packMirrorVersion(entry, "version");

                    spdlog::info("[ClientPack] шаблон {} {}: {}", kPackKnownFile,
                                 builtInTemplate ? "(встроенный пак, поля происхождения сняты)"
                                                 : "(пак с диска)",
                                 entry.dump());
                    spdlog::info("[ClientPack] список паков: {}", knownPath.string());

                    const bool already = std::any_of(known->begin(), known->end(),
                        [&](const json& e)
                        {
                            if (!e.is_object()) return false;
                            if (e.contains("uuid") && e["uuid"].is_string())
                                return e["uuid"].get<std::string>() == uuid;
                            return e.contains("path") && e["path"].is_string() &&
                                   e["path"].get<std::string>() == relPath;
                        });

                    if (!already)
                    {
                        known->push_back(entry);
                        result.registeredKnown = packWriteJsonArray(knownPath, *known);
                    }
                    else
                    {
                        result.registeredKnown = true;
                    }
                }
            }
            else
            {
                spdlog::info("[ClientPack] {} отсутствует или пуст ({}) — пак надо добавить вручную",
                             kPackKnownFile, knownPath.string());
            }
            result.listFolder = knownPath.parent_path();

            const auto globalPath = packListFile(result.comMojang, kPackGlobalFile);
            if (auto globalPacks = packReadJsonArray(globalPath))
            {
                // Здесь запись ссылается на пак только по идентификатору, без
                // описания происхождения, поэтому шаблон безопасен как есть.
                json entry = globalPacks->front();
                if (!entry.contains("pack_id"))
                {
                    spdlog::warn("[ClientPack] в {} нет поля pack_id — формат не разбираю, "
                                 "включи пак вручную",
                                 kPackGlobalFile);
                }
                else
                {
                    entry["pack_id"] = uuid;
                    packMirrorVersion(entry, "version");

                    const bool already = std::any_of(globalPacks->begin(), globalPacks->end(),
                        [&](const json& e)
                        {
                            return e.is_object() && e.contains("pack_id") && e["pack_id"].is_string() &&
                                   e["pack_id"].get<std::string>() == uuid;
                        });

                    if (!already)
                    {
                        globalPacks->push_back(entry);
                        result.registeredGlobal = packWriteJsonArray(globalPath, *globalPacks);
                    }
                    else
                    {
                        result.registeredGlobal = true;
                    }
                }
            }
            else if (!std::filesystem::exists(globalPath))
            {
                // Файла нет вообще: создаём по схеме, снятой с живой установки
                // 1.21.44 — массив записей с pack_id и version, не более. Это
                // единственный случай, когда мы пишем файл с нуля, и только тот
                // файл, которого не было (портить нечего, .bak делать не с чего).
                json fresh = json::array();

                json entry;
                entry["pack_id"] = uuid;
                entry["version"] = json::array({ 1, 0, 0 });
                fresh.push_back(entry);

                std::error_code ec2;
                std::filesystem::create_directories(globalPath.parent_path(), ec2);
                result.registeredGlobal = packWriteJsonArray(globalPath, fresh);

                spdlog::info("[ClientPack] {} не было — создан заново: {}",
                             kPackGlobalFile, globalPath.string());
            }
            else
            {
                spdlog::info("[ClientPack] {} есть, но не разобран как список паков ({}) — "
                             "включи пак вручную в Global Resources",
                             kPackGlobalFile, globalPath.string());
            }

            // ── 5б. Тот же пак — во все миры ────────────────────────────────
            // Это путь, который применяется без перезапуска игры: список мира
            // читается при его загрузке. Прописываем все миры, а не только
            // самый свежий, иначе в другом мире пака просто нет.
            result.worldCount = packAddToAllWorlds(result.comMojang, uuid, &result.worldName);
            result.registeredWorld = result.worldCount > 0;
            if (result.registeredWorld)
                spdlog::info("[ClientPack] пак прописан в {} мир(ах), первый — {} (world_resource_packs.json)",
                             result.worldCount, result.worldName);

            // ── 6. Итог ───────────────────────────────────────────────────────
            result.ok = true;
            result.message = "Пак \"" + request.packName + "\" установлен в " + result.packPath.string() +
                             "\nфайлов: " + std::to_string(copied) +
                             (facesFound > 0 ? ", граней кубемапы: " + std::to_string(facesFound) +
                                                   " в " + std::to_string(envs.size()) + " окружения(й)"
                                             : std::string(", граней кубемапы не найдено")) +
                             (treeCopied ? "" : " (дерева textures\\ не было)") +
                             "\nсписки паков: " + result.listFolder.string() +
                             "\nvalid_known_packs: " + (result.registeredKnown ? "да" : "нет") +
                             ", global: " + (result.registeredGlobal ? "да" : "нет") +
                             ", мир " + (result.registeredWorld ? result.worldName : std::string("нет")) +
                             "\nКак применить: выйди в меню и зайди в мир заново (список мира "
                             "подхватывается при загрузке). Перезапуск игры нужен только для "
                             "глобального списка.";

            spdlog::info("[ClientPack] {}", result.message);
            return result;
        }
        catch (const std::exception& e)
        {
            result.ok = false;
            result.message = std::string("Ошибка установки пака: ") + e.what();
            spdlog::error("[ClientPack] {}", result.message);
            return result;
        }
    }

    std::vector<std::filesystem::path> worldDirs(const std::filesystem::path& comMojang)
    {
        return packWorldDirs(comMojang);
    }

    OptionsReport ensureSkyOptions(const std::filesystem::path& comMojangOverride,
                                   bool enableFancyGraphics,
                                   bool disableRayTracing)
    {
        OptionsReport report;

        try
        {
            const auto comMojang = comMojangOverride.empty() ? findComMojang() : comMojangOverride;
            if (comMojang.empty())
            {
                report.message = "com.mojang не найден";
                return report;
            }

            report.optionsPath = comMojang / "minecraftpe" / "options.txt";
            if (!std::filesystem::exists(report.optionsPath))
            {
                report.message = "options.txt не найден: " + report.optionsPath.string();
                return report;
            }

            std::string text = packReadWholeFile(report.optionsPath);
            if (text.empty())
            {
                report.message = "options.txt пустой: " + report.optionsPath.string();
                return report;
            }

            report.ok = true;

            auto readOption = [&](const char* key)
            {
                size_t begin = 0, end = 0;
                return packFindOption(text, key, &begin, &end) ? text.substr(begin, end - begin)
                                                               : std::string();
            };

            auto setOption = [&](const char* key, const char* value)
            {
                size_t begin = 0, end = 0;
                if (!packFindOption(text, key, &begin, &end)) return false;
                if (text.compare(begin, end - begin, value) == 0) return false;
                text.replace(begin, end - begin, value);
                return true;
            };

            report.fancySkiesOld = readOption("gfx_fancyskies");
            report.fancyGraphicsOld = readOption("gfx_fancygraphics");
            report.rayTracingOld = readOption("gfx_raytracing");

            // Ключа может не быть в старых версиях — тогда дописываем строку в
            // конец, сохраняя перевод строки файла.
            if (report.fancySkiesOld.empty())
            {
                const bool crlf = text.find("\r\n") != std::string::npos;
                if (!text.empty() && text.back() != '\n') text += crlf ? "\r\n" : "\n";
                text += "gfx_fancyskies:1";
                text += crlf ? "\r\n" : "\n";
                report.fancySkiesChanged = true;
            }
            else
            {
                report.fancySkiesChanged = setOption("gfx_fancyskies", "1");
            }

            if (enableFancyGraphics && !report.fancyGraphicsOld.empty())
                report.fancyGraphicsChanged = setOption("gfx_fancygraphics", "1");

            if (disableRayTracing && report.rayTracingOn())
                report.rayTracingChanged = setOption("gfx_raytracing", "0");

            if (report.changed())
                report.wrote = packWriteWholeFile(report.optionsPath, text);

            auto arrow = [](const std::string& before, const std::string& after)
            {
                if (before == after) return before + (before.empty() ? "нет ключа" : " (уже так)");
                return before + " -> " + after;
            };

            report.message = "options.txt " + report.optionsPath.string() +
                             ": fancy skies " + arrow(report.fancySkiesOld, readOption("gfx_fancyskies")) +
                             ", fancy graphics " + arrow(report.fancyGraphicsOld, readOption("gfx_fancygraphics")) +
                             ", raytracing " + arrow(report.rayTracingOld, readOption("gfx_raytracing")) +
                             ", записан: " + (report.wrote ? "да" : "нет");

            spdlog::info("[ClientPack] {}", report.message);
            return report;
        }
        catch (const std::exception& e)
        {
            report.message = std::string("Ошибка правки options.txt: ") + e.what();
            spdlog::error("[ClientPack] {}", report.message);
            return report;
        }
    }

    bool uninstall(const std::string& packName,
                   const std::filesystem::path& comMojangOverride,
                   std::string* messageOut)
    {
        try
        {
            const auto comMojang = comMojangOverride.empty() ? findComMojang() : comMojangOverride;
            if (comMojang.empty())
            {
                if (messageOut) *messageOut = "com.mojang не найден — нечего удалять.";
                return false;
            }

            const std::string folder = packFolderName(packName);
            const std::string uuid = packUuid(packName);
            const std::string relPath = "resource_packs/" + folder;

            int removedEntries = 0;

            auto stripFrom = [&](const std::filesystem::path& path, const char* idKey)
            {
                auto arr = packReadJsonArray(path);
                if (!arr) return;

                const size_t before = arr->size();
                json kept = json::array();
                for (const auto& entry : *arr)
                {
                    bool ours = false;
                    if (entry.is_object())
                    {
                        if (entry.contains(idKey) && entry[idKey].is_string() &&
                            entry[idKey].get<std::string>() == uuid)
                            ours = true;
                        if (entry.contains("path") && entry["path"].is_string() &&
                            entry["path"].get<std::string>() == relPath)
                            ours = true;
                    }
                    if (!ours) kept.push_back(entry);
                }

                if (kept.size() != before)
                {
                    removedEntries += static_cast<int>(before - kept.size());
                    if (kept.empty())
                    {
                        // Пустой список — просто оставляем пустой массив.
                        kept = json::array();
                    }
                    packWriteJsonArray(path, kept);
                }
            };

            stripFrom(packListFile(comMojang, kPackKnownFile), "uuid");
            stripFrom(packListFile(comMojang, kPackGlobalFile), "pack_id");

            // Миры могут не найтись — тогда ничего не чистим, чтобы случайно не
            // прочитать файл относительно текущего каталога процесса.
            for (const auto& world : packWorldDirs(comMojang))
                stripFrom(world / "world_resource_packs.json", "pack_id");

            std::error_code ec;
            const auto packPath = comMojang / "resource_packs" / folder;
            const bool hadFolder = std::filesystem::exists(packPath, ec);
            if (hadFolder)
                std::filesystem::remove_all(packPath, ec);

            if (messageOut)
            {
                *messageOut = "Пак \"" + packName + "\" удалён: записей из списков " +
                              std::to_string(removedEntries) +
                              (hadFolder ? ", папка удалена" : ", папки не было");
            }
            spdlog::info("[ClientPack] удаление: записей {} , папка {}", removedEntries, hadFolder ? "да" : "нет");
            return true;
        }
        catch (const std::exception& e)
        {
            if (messageOut) *messageOut = std::string("Ошибка удаления пака: ") + e.what();
            spdlog::error("[ClientPack] ошибка удаления: {}", e.what());
            return false;
        }
    }
}
