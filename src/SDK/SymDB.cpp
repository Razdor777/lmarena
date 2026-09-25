//
// Реализация SymDB. Парсер намеренно терпимый: точная схема symdb зависит от версии
// bedrock-runtime-data, поэтому читаем файлы как текст и вытаскиваем пары
// "имя" / "адрес" независимо от порядка ключей и уровня вложенности.
//
#include "SymDB.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace fs = std::filesystem;

SymDB& SymDB::instance() {
    static SymDB db;
    return db;
}

// ────────────────────────────── нормализация имён ──────────────────────────────

// "?setPosition@Actor@@UEAAXAEBVVec3@@@Z" -> "actor::setposition"
std::string SymDB::demangleMsvc(const std::string& m) {
    if (m.empty() || m[0] != '?')
        return {};
    std::string body = m.substr(1);
    // отрезаем всё после "@@" — дальше идут типы
    auto end = body.find("@@");
    if (end != std::string::npos)
        body = body.substr(0, end);

    std::vector<std::string> parts;
    std::string cur;
    for (char c : body) {
        if (c == '@') {
            if (!cur.empty()) {
                parts.push_back(cur);
                cur.clear();
            }
            if (parts.size() >= 2)
                break;
        } else {
            cur += c;
        }
    }
    if (!cur.empty())
        parts.push_back(cur);
    if (parts.empty())
        return {};

    std::string name = parts[0];
    // убираем суффиксы операторов и служебные префиксы
    if (!name.empty() && name[0] == '_')
        name = name.substr(1);
    if (parts.size() < 2)
        return name;
    // parts[1] может быть классом, а может числом (для статических символов)
    const std::string& scope = parts[1];
    if (!scope.empty() && std::all_of(scope.begin(), scope.end(),
                                      [](unsigned char c) { return std::isdigit(c); }))
        return name;
    return scope + "::" + name;
}

std::string SymDB::normalize(const std::string& raw) {
    std::string s;
    if (!raw.empty() && raw[0] == '?') {
        s = demangleMsvc(raw);
    }
    if (s.empty()) {
        s = raw;
        // "Actor::setPosition(int)" -> "actor::setposition"
        auto paren = s.find('(');
        if (paren != std::string::npos)
            s = s.substr(0, paren);
    }
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    // убираем пробелы ("Actor :: setPosition")
    s.erase(std::remove_if(s.begin(), s.end(),
                           [](unsigned char c) { return std::isspace(c); }),
            s.end());
    return s;
}

// ────────────────────────────── загрузка ──────────────────────────────

static bool parseNumber(const std::string& v, uintptr_t& out) {
    if (v.empty())
        return false;
    try {
        if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
            out = (uintptr_t)std::stoull(v.substr(2), nullptr, 16);
            return true;
        }
        out = (uintptr_t)std::stoull(v);
        return true;
    } catch (...) {
        return false;
    }
}

size_t SymDB::load(const std::string& dirUtf8) {
    if (dirUtf8.empty())
        return 0;
    std::error_code ec;
    if (!fs::exists(dirUtf8, ec))
        return 0;

    size_t before = mEntries.size();

    for (fs::recursive_directory_iterator it(dirUtf8, ec), end; it != end; it.increment(ec)) {
        if (!it->is_regular_file())
            continue;
        const auto ext = it->path().extension().string();
        if (ext != ".json" && ext != ".jsonl" && ext != ".txt")
            continue;

        std::ifstream f(it->path(), std::ios::binary);
        if (!f)
            continue;
        std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (text.size() > 64u * 1024u * 1024u)  // не тянем гигабайты в память
            continue;

        // Ищем пары "ключ": значение, где ключ похож на имя, и рядом есть адрес.
        // Работаем по тексту: схема symdb может быть любой вложенности.
        const size_t n = text.size();
        size_t pos = 0;
        while (pos < n) {
            size_t nameKey = text.find("\"name\"", pos);
            if (nameKey == std::string::npos)
                break;
            size_t colon = text.find(':', nameKey);
            if (colon == std::string::npos)
                break;
            size_t q1 = text.find('"', colon + 1);
            if (q1 == std::string::npos)
                break;
            size_t q2 = text.find('"', q1 + 1);
            if (q2 == std::string::npos)
                break;
            std::string raw = text.substr(q1 + 1, q2 - q1 - 1);
            pos = q2 + 1;

            // Адрес ищем ПОСЛЕ имени, до следующего "name" (иначе прилипает адрес
            // соседней записи). Если после имени адреса нет — смотрим перед именем.
            const char* keys[] = {"\"rva\"", "\"address\"", "\"addr\"", "\"value\"", "\"offset\""};
            size_t nextName = text.find("\"name\"", q2);
            size_t fEnd = std::min(n, nextName == std::string::npos ? q2 + 512 : nextName);
            size_t prevName = text.rfind("\"name\"", nameKey > 1 ? nameKey - 1 : 0);

            auto tryKeys = [&](size_t from, size_t to) -> uintptr_t {
                for (const char* key : keys) {
                    size_t k = text.find(key, from);
                    while (k != std::string::npos && k < to) {
                        size_t c2 = text.find(':', k);
                        if (c2 == std::string::npos || c2 >= to)
                            break;
                        size_t v1 = text.find_first_not_of(" \t\r\n", c2 + 1);
                        if (v1 == std::string::npos || v1 >= to)
                            break;
                        std::string val;
                        if (text[v1] == '"') {
                            size_t v2 = text.find('"', v1 + 1);
                            if (v2 == std::string::npos || v2 >= to)
                                break;
                            val = text.substr(v1 + 1, v2 - v1 - 1);
                        } else {
                            size_t v2 = text.find_first_of(",}\n \t", v1);
                            val = text.substr(v1, (v2 == std::string::npos ? to : v2) - v1);
                        }
                        uintptr_t addr = 0;
                        if (parseNumber(val, addr))
                            return addr;
                        k = text.find(key, k + 1);
                    }
                }
                return (uintptr_t)0;
            };

            uintptr_t addr = tryKeys(q2, fEnd);
            if (!addr && prevName != std::string::npos)
                addr = tryKeys(prevName, nameKey);
            if (!addr)
                addr = tryKeys((nameKey > 512) ? nameKey - 512 : 0, nameKey);

            if (!raw.empty() && addr) {
                Entry e;
                e.raw = raw;
                e.rva = addr;
                e.norm = normalize(raw);
                if (!e.norm.empty() && mIndex.find(e.norm) == mIndex.end()) {
                    mIndex[e.norm] = mEntries.size();
                    mEntries.push_back(std::move(e));
                }
            }
        }
    }

    return mEntries.size() - before;
}

size_t SymDB::loadNextToModule() {
#ifdef _WIN32
    char path[MAX_PATH]{};
    if (!GetModuleFileNameA(nullptr, path, MAX_PATH))
        return 0;
    fs::path exe(path);
    for (const char* cand : {"bedrock_runtime_data", "tools/data/bedrock_runtime_data"}) {
        fs::path dir = exe.parent_path() / cand;
        std::error_code ec;
        if (fs::exists(dir, ec)) {
            if (size_t n = load(dir.string()))
                return n;
        }
    }
#endif
    return 0;
}

// ────────────────────────────── поиск ──────────────────────────────

uintptr_t SymDB::moduleBase() const {
    if (mBase)
        return mBase;
#ifdef _WIN32
    mBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
#else
    mBase = 0;
#endif
    return mBase;
}

uintptr_t SymDB::find(const std::string& name) const {
    if (name.empty() || mEntries.empty())
        return 0;

    std::string q = normalize(name);

    // 1) точное совпадение
    auto it = mIndex.find(q);
    if (it != mIndex.end())
        return moduleBase() + mEntries[it->second].rva;

    // 2) mangled-имя целиком
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    it = mIndex.find(lower);
    if (it != mIndex.end())
        return moduleBase() + mEntries[it->second].rva;

    // 3) вхождение: "actor::setposition" содержится в ключе или наоборот
    for (const auto& e : mEntries) {
        if (e.norm.size() >= q.size() && e.norm.compare(e.norm.size() - q.size(), q.size(), q) == 0 &&
            (e.norm.size() == q.size() || e.norm[e.norm.size() - q.size() - 1] == ':'))
            return moduleBase() + e.rva;
    }
    // 4) последняя часть имени (без класса), если она уникальна
    auto sep = q.rfind("::");
    std::string tail = (sep == std::string::npos) ? q : q.substr(sep + 2);
    if (tail.size() >= 4) {
        uintptr_t found = 0;
        size_t hits = 0;
        for (const auto& e : mEntries) {
            auto s = e.norm.rfind("::");
            std::string et = (s == std::string::npos) ? e.norm : e.norm.substr(s + 2);
            if (et == tail) {
                found = moduleBase() + e.rva;
                if (++hits > 1)
                    break;
            }
        }
        if (hits == 1)
            return found;
    }
    return 0;
}

// ────────────────────────────── диагностика ──────────────────────────────

bool SymDB::inspect(const std::string& outPath, size_t maxEntries) const {
    std::ofstream out(outPath, std::ios::binary);
    if (!out)
        return false;
    out << "SymDB: записей " << mEntries.size() << "\r\n";
    out << "база модуля: 0x" << std::hex << moduleBase() << std::dec << "\r\n\r\n";
    out << "первые записи (raw -> нормализованное имя -> rva):\r\n";
    for (size_t i = 0; i < mEntries.size() && i < maxEntries; ++i) {
        const auto& e = mEntries[i];
        out << "  " << e.raw << "  ->  " << e.norm << "  ->  0x" << std::hex << e.rva << std::dec << "\r\n";
    }
    return true;
}
