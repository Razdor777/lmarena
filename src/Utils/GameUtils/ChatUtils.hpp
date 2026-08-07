#pragma once
#include <string>
//
// Created by vastrakai on 6/25/2024.
//

class ChatUtils {
public:
    static void displayClientMessage(const std::string& msg);
    template<typename... Args>
    static void displayClientMessage(fmt::format_string<Args...> fmt, Args&&... args)
    {
        displayClientMessage(fmt::format(fmt, std::forward<Args>(args)...));
    }
    static void displayClientMessageSub(const std::string& subcaption, const std::string& msg);
    static void displayClientMessageRaw(const std::string& msg);
    template<typename... Args>
    static void displayClientMessageRaw(fmt::format_string<Args...> fmt, Args&&... args)
    {
        displayClientMessageRaw(fmt::format(fmt, std::forward<Args>(args)...));
    }

    // ── Работа с §-форматированием (правильно, с учётом UTF-8) ──────────
    // Убирает ВСЕ §-коды из строки. § в UTF-8 = 2 байта (0xC2 0xA7),
    // после которых идёт 1 байт кода цвета/стиля — итого 3 байта на код.
    // Также поддерживает legacy однобайтовый §=0xA7.
    static std::string stripColorCodes(const std::string& s);

    // Извлекает чистый ник из тега вида "§7[§ePlayer§7] §r§7_poop_boy§r" -> "_poop_boy"
    static std::string extractNickFromTag(const std::string& rawTag);

    // Извлекает текст ранга из тега вида "§7[§ePlayer§7] §r..." -> "Player"
    static std::string extractRankFromTag(const std::string& rawTag);

    // Приводит ранг к каноническому виду для точного сравнения:
    // нижний регистр (с кириллицей), без пробелов и дефисов.
    static std::string canonicalizeRankKey(const std::string& rankRaw);
};