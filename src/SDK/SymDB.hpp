#pragma once
//
// SymDB — поиск адресов по имени вместо поиска по байтам кода.
//
// Зачем: в проекте 123 сигнатуры вида "48 89 ? ? ? 57 48 83 EC ?" — это машинный код
// конкретной сборки игры. На каждой новой версии они перестают находиться.
// В bedrock-runtime-data (пакет данных LeviLamina) лежит symdb: соответствие
// имя символа -> адрес. Имя не меняется от версии к версии так часто, как байты.
//
// Как пользоваться:
//   1. Скачать bedrock-runtime-data нужной версии, например 26.51.1-client.6
//      (релизы: github.com/LiteLDev/bedrock-runtime-data)
//   2. Распаковать, чтобы рядом с Minecraft.Windows.exe (или в каталоге, указанном
//      в SymDB::load) лежала папка bedrock_runtime_data
//   3. SymDB сам разберёт файлы; если что-то не так — вызови inspect() и посмотри
//      symdb_inspect.txt: там будет показано, как выглядят данные на самом деле.
//
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class SymDB {
public:
    struct Entry {
        std::string raw;    // как записано в symdb (обычно mangled: ?setPosition@Actor@@...)
        uintptr_t   rva{};  // адрес относительно начала модуля
        std::string norm;   // нормализованное имя: "actor::setposition"
    };

    static SymDB& instance();

    /// Рекурсивно читает *.json (и *.jsonl) из каталога. Возвращает число записей.
    size_t load(const std::string& dirUtf8);
    /// То же, но ищет каталог рядом с exe: "bedrock_runtime_data".
    size_t loadNextToModule();

    bool   loaded() const { return !mEntries.empty(); }
    size_t size() const { return mEntries.size(); }

    /// Адрес символа по имени. Принимает и "Actor::setPosition", и mangled-имя.
    uintptr_t find(const std::string& name) const;

    /// Грубая проверка: есть ли символ вообще.
    bool contains(const std::string& name) const { return find(name) != 0; }

    /// Дамп структуры данных в файл (для диагностики, если формат не распознан).
    bool inspect(const std::string& outPath, size_t maxEntries = 200) const;

    const std::vector<Entry>& entries() const { return mEntries; }

private:
    SymDB() = default;

    static std::string normalize(const std::string& raw);
    static std::string demangleMsvc(const std::string& mangled);

    uintptr_t moduleBase() const;

    std::vector<Entry> mEntries;
    std::unordered_map<std::string, size_t> mIndex;  // norm -> индекс в mEntries
    mutable uintptr_t mBase{};
};
