//
// Created by vastrakai on 6/25/2024.
//

#include "ChatUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <cctype>
#include <cstdint>

void ChatUtils::displayClientMessage(const std::string& msg)
{
    if (!msg.contains("\n"))
    {// FUCK YOU DONT CHANGE THE PREIX COLOR U NIGGER
        ClientInstance::get()->getGuiData()->displayClientMessageQueued("§aJuzdex§7 » §r" + msg);
        return;
    }

    std::string formattedMsg = "§aJuzdex§7 » §r";
    for (const auto& c : msg)
    {
        if (c == '\n')
            formattedMsg += "\n§aJuzdex§7 » §r";
        else
            formattedMsg += c;
    }
    ClientInstance::get()->getGuiData()->displayClientMessageQueued(formattedMsg);
}

void ChatUtils::displayClientMessageSub(const std::string& subcaption, const std::string& msg)
{
    ClientInstance::get()->getGuiData()->displayClientMessageQueued("§aJuzdex§7 » §7[" + subcaption + "§7] §r" + msg);
}

void ChatUtils::displayClientMessageRaw(const std::string& msg)
{
    ClientInstance::get()->getGuiData()->displayClientMessageQueued(msg);
}

// ─────────────────────────────────────────────────────────────────────────

namespace
{
    // Настоящий код цвета/стиля Minecraft: 0-9, a-f, k-o, r. После «§» байт
    // выбрасывается только если это действительно код, а не первая буква слова.
    bool chatUtilsIsCodeChar(unsigned char c)
    {
        if (c >= '0' && c <= '9') return true;

        const unsigned char lower = (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + 32) : c;
        if (lower >= 'a' && lower <= 'f') return true;

        return lower == 'k' || lower == 'l' || lower == 'm' ||
               lower == 'n' || lower == 'o' || lower == 'r';
    }
}

std::string ChatUtils::stripColorCodes(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); )
    {
        unsigned char c = (unsigned char)s[i];

        if (c == 0xC2 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0xA7)
        {
            i += 2; // 0xC2, 0xA7
            if (i < s.size() && chatUtilsIsCodeChar((unsigned char)s[i])) i += 1; // + сам код
            continue;
        }
        // Однобайтовый legacy §. ВАЖНО: 0xA7 — это ещё и хвостовой байт
        // кириллических символов ('Ч' = D0 A7), поэтому пропускаем байт как код
        // только если он не является продолжением UTF-8-последовательности.
        if (c == 0xA7 && (i == 0 || (unsigned char)s[i - 1] < 0xC0))
        {
            i += 1; // сам §
            if (i < s.size() && chatUtilsIsCodeChar((unsigned char)s[i])) i += 1; // + код
            continue;
        }
        out += s[i];
        ++i;
    }
    return out;
}

namespace {
    void trimWs(std::string& s)
    {
        while (!s.empty() && (s.back() == ' ' || s.back() == '\r' || s.back() == '\n' || s.back() == '\t'))
            s.pop_back();
        size_t start = s.find_first_not_of(" \r\n\t");
        s = (start == std::string::npos) ? "" : s.substr(start);
    }

    void appendLowerUtf8(std::string& out, const std::string& s)
    {
        for (size_t i = 0; i < s.size(); )
        {
            unsigned char c = (unsigned char)s[i];
            if (c < 0x80)
            {
                out += (char)std::tolower(c);
                ++i;
            }
            else if ((c & 0xE0) == 0xC0 && i + 1 < s.size())
            {
                unsigned char c2 = (unsigned char)s[i + 1];
                uint32_t cp = ((c & 0x1F) << 6) | (c2 & 0x3F);
                if (cp >= 0x410 && cp <= 0x42F) cp += 0x20;
                else if (cp == 0x401) cp = 0x451;
                out += (char)(0xC0 | (cp >> 6));
                out += (char)(0x80 | (cp & 0x3F));
                i += 2;
            }
            else
            {
                out += s[i];
                ++i;
            }
        }
    }
}

std::string ChatUtils::extractNickFromTag(const std::string& rawTag)
{
    std::string s = stripColorCodes(rawTag);
    trimWs(s);
    if (s.empty()) return "";

    auto pos = s.rfind("] ");
    if (pos != std::string::npos)
    {
        std::string nick = s.substr(pos + 2);
        trimWs(nick);
        if (!nick.empty()) return nick;
    }
    return s;
}

std::string ChatUtils::extractRankFromTag(const std::string& rawTag)
{
    std::string s = stripColorCodes(rawTag);
    trimWs(s);

    auto lb = s.find('[');
    auto rb = s.find(']');
    if (lb != std::string::npos && rb != std::string::npos && rb > lb)
    {
        std::string rank = s.substr(lb + 1, rb - lb - 1);
        trimWs(rank);
        return rank;
    }
    return "";
}

std::string ChatUtils::canonicalizeRankKey(const std::string& rankRaw)
{
    std::string stripped = stripColorCodes(rankRaw);
    trimWs(stripped);

    std::string lower;
    appendLowerUtf8(lower, stripped);

    std::string canon;
    canon.reserve(lower.size());
    for (char c : lower)
        if (c != ' ' && c != '-' && c != '\t') canon += c;

    return canon;
}