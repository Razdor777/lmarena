#pragma once
//
// ChatBinds.hpp — чат-бинды: клавиша -> текст в чат.
//
// Настраиваются в ClickGui (кнопка "Chat Binds" сверху): текст, например
// "/spawn", и клавиша, например T. Нажал T в игре -> ушло "/spawn".
//   "/..."  — команда сервера (как если набрать её в чате);
//   "...."  — команда клиента (.tp, .config ...), выполняется здесь же и на
//             сервер не уходит;
//   иначе  — обычное сообщение в чат.
// Срабатывает только в игре (не в меню, не в чате, не в ClickGui), на нажатие
// (зажатая клавиша не повторяет); игре забинденная клавиша не отдаётся — T не
// откроет ещё и ванильный чат.
//
// Хранится отдельно от конфигов — Solstice\chat_binds.json, сохраняется сразу
// при каждом изменении: бинды не пропадут при смене/загрузке конфига.
//
// Потокобезопасно: клавиши приходят из KeyHook (поток ввода), правки — из
// ClickGui (поток рендера), а отправка — из Actor::baseTick (поток игры):
// handleKey только ставит текст в очередь, flushPending его отправляет.
//

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct ChatBind
{
    std::string text;
    int key = 0; // VK, 0 — не назначена
};

class ChatBinds
{
public:
    static constexpr size_t kMaxBinds = 32;
    static constexpr size_t kMaxText = 256;

    static std::vector<ChatBind> list();
    static size_t count();
    static int  add();                                 // новый пустой, индекс (-1 — лимит)
    static void setText(size_t index, const std::string& text);
    static void setKey(size_t index, int vk);          // та же клавиша у другого бинда снимается
    static void remove(size_t index);

    // Из KeyHook, до того как клавиша уйдёт игре. true — клавиша забиндена и
    // съедена (игре её не отдавать). Сам текст уходит позже, в flushPending.
    static bool handleKey(int vk, bool down);
    // Из BaseTickHook (поток игры): отправить то, что накопил handleKey.
    static void flushPending();

    static void send(const std::string& text);
    static std::string keyName(int vk);

private:
    static void loadLocked();
    static void saveLocked();
    // Члены класса, а не свободные функции в .cpp: при unity-сборке соседние
    // .cpp склеиваются в один, и одноимённые свободные функции конфликтуют.
    static std::string bindsPath();
    static std::string trimmed(const std::string& s);

    static inline std::mutex sMutex;
    static inline std::vector<ChatBind> sBinds;
    static inline std::vector<std::string> sPending;   // нажато, ещё не отправлено
    static inline bool sLoaded = false;
    static inline std::array<bool, 256> sDown{};       // клавиша уже зажата (без автоповтора)
    static inline std::array<uint64_t, 256> sLastFire{};
};
