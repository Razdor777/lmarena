//
// QuickConnect — см. комментарий в QuickConnect.hpp.
//

#include "QuickConnect.hpp"

#include <Windows.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/Logger.hpp>

namespace
{
    // GameConnectionInfo (0x1A0 байт) — из sub_1401DB030 и sub_140205A10:
    // std::string хоста на +0x08, uint32 порт на +0x48. Остальное не трогаем,
    // а копируем из рабочего образца.
    constexpr size_t kInfoSize    = 0x1A0;
    constexpr size_t kHostOffset  = 0x08;
    constexpr size_t kPortOffset  = 0x48;

    std::mutex gMutex;
    void*      gNetworkSystem = nullptr;
    std::array<uint8_t, kInfoSize> gTemplate{};
    bool       gTemplateValid = false;
    std::string gTemplateHost;
    int        gTemplatePort = 0;

    bool (*gConnectFn)(void*, const void*, const void*) = nullptr;

    // MSVC std::string: буфер/указатель (16), size (8), capacity (8).
    void writeStdString(uint8_t* base, const std::string& value)
    {
        auto* size = reinterpret_cast<size_t*>(base + 16);
        auto* cap  = reinterpret_cast<size_t*>(base + 24);

        if (value.size() <= 15)
        {
            std::memset(base, 0, 32);
            std::memcpy(base, value.data(), value.size());
            *size = value.size();
            *cap  = 15;
            return;
        }

        auto* buf = static_cast<char*>(std::malloc(value.size() + 1));
        if (!buf) return;

        std::memcpy(buf, value.data(), value.size());
        buf[value.size()] = '\0';

        *reinterpret_cast<char**>(base) = buf;
        *size = value.size();
        *cap  = value.size();
    }

    std::string readStdString(const uint8_t* base)
    {
        auto* size = reinterpret_cast<const size_t*>(base + 16);
        auto* cap  = reinterpret_cast<const size_t*>(base + 24);

        // Мусор в поле ёмкости — молча выходим, а не читаем случайный адрес.
        if (*cap > 0x1000 || *size > *cap) return {};

        const char* data = (*cap > 15)
            ? *reinterpret_cast<char* const*>(base)
            : reinterpret_cast<const char*>(base);

        if (!data) return {};

        std::string out(data, *size);
        if (out.find('\0') != std::string::npos) return {};

        return out;
    }

    bool parseAddress(const std::string& raw, std::string& host, int& port)
    {
        std::string text = raw;
        while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.erase(text.begin());
        while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n'))
            text.pop_back();

        if (text.empty()) return false;

        port = 19132;

        const auto colon = text.rfind(':');
        if (colon != std::string::npos && text.find(':') == colon) // ровно одно двоеточие
        {
            const std::string portText = text.substr(colon + 1);
            if (portText.empty()) return false;

            int value = 0;
            for (const char c : portText)
            {
                if (c < '0' || c > '9') return false;
                value = value * 10 + (c - '0');
                if (value > 65535) return false;
            }

            port = value;
            text = text.substr(0, colon);
        }

        if (text.empty()) return false;
        if (port <= 0 || port > 65535) return false;

        host = text;
        return true;
    }

    // Вызов connect: вынесен отдельно, потому что __try нельзя использовать в
    // функции, которой нужен разворот стека (в join() есть std::string и lock_guard).
    bool callConnectRaw(void* networkSystem, const void* info, const void* secondInfo)
    {
        __try
        {
            return gConnectFn(networkSystem, info, secondInfo);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

void QuickConnect::setConnectFunction(bool (*fn)(void*, const void*, const void*))
{
    gConnectFn = fn;
}

bool QuickConnect::isReady()
{
    std::lock_guard lock(gMutex);
    return gNetworkSystem != nullptr && gTemplateValid;
}

void QuickConnect::onNetworkConnect(void* networkSystem, const void* gameConnectionInfo)
{
    if (!networkSystem || !gameConnectionInfo) return;

    // Запоминаем сети только «сырое» подключение самой игры; свой синтетический
    // вызов идёт через оригинал, поэтому сюда не попадает.
    const auto* bytes = static_cast<const uint8_t*>(gameConnectionInfo);

    const std::string host = readStdString(bytes + kHostOffset);
    const int port = *reinterpret_cast<const int32_t*>(bytes + kPortOffset);

    std::lock_guard lock(gMutex);

    gNetworkSystem = networkSystem;

    std::memcpy(gTemplate.data(), bytes, kInfoSize);
    gTemplateValid = true;
    gTemplateHost  = host;
    gTemplatePort  = port;

    spdlog::info("[QuickConnect] captured network system {} ({}:{})",
                 networkSystem, host.empty() ? "<empty>" : host, port);
}

bool QuickConnect::join(const std::string& address)
{
    std::string host;
    int port = 0;

    if (!parseAddress(address, host, port))
    {
        ChatUtils::displayClientMessage(
            "§c[QuickConnect] §fНе понял адрес §6" + address +
            "§f. Нужно §6ip:port§f, например §6109.248.4.159:19133§f.");
        return false;
    }

    if (!gConnectFn)
    {
        ChatUtils::displayClientMessage(
            "§c[QuickConnect] §fФункция подключения не найдена — этот билд игры не совпал по сигнатуре. "
            "Скинь версию клиента, я поправлю.");
        return false;
    }

    std::array<uint8_t, kInfoSize> info{};
    std::array<uint8_t, kInfoSize> second{};
    void* networkSystem = nullptr;

    {
        std::lock_guard lock(gMutex);

        if (!gNetworkSystem || !gTemplateValid)
        {
            ChatUtils::displayClientMessage(
                "§c[QuickConnect] §fПока не знаю сеть клиента. Зайди на любой сервер обычным способом "
                "§6один раз за запуск§f — дальше подключение будет в один клик.");
            return false;
        }

        info          = gTemplate;
        networkSystem = gNetworkSystem;
    }

    writeStdString(info.data() + kHostOffset, host);
    *reinterpret_cast<int32_t*>(info.data() + kPortOffset) = port;

    ChatUtils::displayClientMessage(
        "§a[QuickConnect] §fПодключаюсь к §6" + host + ":" + std::to_string(port) + "§f…");

    const bool ok = callConnectRaw(networkSystem, info.data(), second.data());

    if (!ok)
    {
        ChatUtils::displayClientMessage(
            "§c[QuickConnect] §fПодключиться не удалось: сервер не принял запрос или адрес недоступен.");
    }

    return ok;
}

void QuickConnect::onEnable()
{
    // Модуль — разовое действие: включил в ClickGui → ушёл запрос → выключился.
    // setEnabled() здесь только меняет mWantedState, поэтому рекурсии нет:
    // ModuleManager сам вызовет onDisable() следующим тиком.
    setEnabled(false);

    const std::string address = mServer.mValue;
    join(address);
}
