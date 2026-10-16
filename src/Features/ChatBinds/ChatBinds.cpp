//
// ChatBinds.cpp — см. ChatBinds.hpp
//
#include "ChatBinds.hpp"

#include <Features/Command/CommandManager.hpp>
#include <Features/Events/ChatEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/FileUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/CommandUtils.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <Windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace
{
    std::string bindsPath() { return FileUtils::getSolsticeDir() + "chat_binds.json"; }

    std::string trimmed(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return {};
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
    }
}

void ChatBinds::loadLocked()
{
    if (sLoaded) return;
    sLoaded = true;
    sBinds.clear();
    try
    {
        const std::string path = bindsPath();
        if (!FileUtils::fileExists(path)) return;
        std::ifstream in(path);
        nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_array()) return;
        for (auto& e : j)
        {
            if (!e.is_object() || sBinds.size() >= kMaxBinds) continue;
            ChatBind b;
            b.text = e.value("text", std::string());
            b.key  = std::clamp(e.value("key", 0), 0, 255);
            if (b.text.size() > kMaxText) b.text.resize(kMaxText);
            sBinds.push_back(std::move(b));
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("[ChatBinds] load failed: {}", e.what());
    }
}

void ChatBinds::saveLocked()
{
    try
    {
        nlohmann::json j = nlohmann::json::array();
        for (auto& b : sBinds) j.push_back({ {"text", b.text}, {"key", b.key} });
        const std::string path = bindsPath();
        const std::string tmp = path + ".tmp";
        {
            std::ofstream out(tmp, std::ios::trunc);
            out << j.dump(2);
        }
        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        if (ec)
        {
            std::filesystem::remove(path, ec);
            std::filesystem::rename(tmp, path, ec);
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("[ChatBinds] save failed: {}", e.what());
    }
}

std::vector<ChatBind> ChatBinds::list()
{
    std::lock_guard lk(sMutex);
    loadLocked();
    return sBinds;
}

size_t ChatBinds::count()
{
    std::lock_guard lk(sMutex);
    loadLocked();
    return sBinds.size();
}

int ChatBinds::add()
{
    std::lock_guard lk(sMutex);
    loadLocked();
    if (sBinds.size() >= kMaxBinds) return -1;
    sBinds.push_back({});
    saveLocked();
    return static_cast<int>(sBinds.size()) - 1;
}

void ChatBinds::setText(size_t index, const std::string& text)
{
    std::lock_guard lk(sMutex);
    loadLocked();
    if (index >= sBinds.size()) return;
    std::string t = trimmed(text);
    if (t.size() > kMaxText) t.resize(kMaxText);
    if (sBinds[index].text == t) return;
    sBinds[index].text = std::move(t);
    saveLocked();
}

void ChatBinds::setKey(size_t index, int vk)
{
    std::lock_guard lk(sMutex);
    loadLocked();
    if (index >= sBinds.size()) return;
    vk = std::clamp(vk, 0, 255);
    // одна клавиша — один бинд: у другого она снимается
    if (vk != 0)
        for (size_t i = 0; i < sBinds.size(); i++)
            if (i != index && sBinds[i].key == vk) sBinds[i].key = 0;
    sBinds[index].key = vk;
    saveLocked();
}

void ChatBinds::remove(size_t index)
{
    std::lock_guard lk(sMutex);
    loadLocked();
    if (index >= sBinds.size()) return;
    sBinds.erase(sBinds.begin() + static_cast<std::ptrdiff_t>(index));
    saveLocked();
}

bool ChatBinds::handleKey(int vk, bool down)
{
    if (vk <= 0 || vk > 255) return false;

    std::string text;
    {
        std::lock_guard lk(sMutex);
        loadLocked();
        // Бинд без текста клавишу не забирает (назначил T, а текст ещё не
        // ввёл — T по-прежнему открывает чат).
        auto it = std::find_if(sBinds.begin(), sBinds.end(),
                               [vk](const ChatBind& b) { return b.key == vk && !b.text.empty(); });
        if (it == sBinds.end())
        {
            sDown[vk] = false;
            return false;
        }

        // Отпускание: съедаем, только если съели нажатие (игра его не видела).
        if (!down)
        {
            const bool ours = sDown[vk];
            sDown[vk] = false;
            return ours;
        }

        // Нажатие: только в игре — не в меню/инвентаре/ClickGui (мышь
        // свободна) и не в ванильном чате.
        auto* ci = ClientInstance::get();
        if (!ci || !ci->getLocalPlayer() || ci->getMouseGrabbed() || ci->getScreenName() == "chat_screen")
            return false;

        if (sDown[vk]) return true;      // автоповтор зажатой клавиши
        sDown[vk] = true;

        // защита от дребезга / случайного двойного нажатия
        const uint64_t now = static_cast<uint64_t>(NOW);
        if (now - sLastFire[vk] < 300) return true;
        sLastFire[vk] = now;
        text = it->text;
    }
    send(text); // без блокировки: отправка может вызвать другие хуки
    return true;
}

void ChatBinds::send(const std::string& raw)
{
    const std::string text = trimmed(raw);
    if (text.empty()) return;

    if (text[0] == '.')
    {
        // Команда клиента — выполняем здесь, на сервер ничего не уходит.
        if (gFeatureManager && gFeatureManager->mCommandManager)
        {
            ChatEvent ev(text);
            gFeatureManager->mCommandManager->handleCommand(ev);
        }
        return;
    }
    if (text[0] == '/')
    {
        CommandUtils::executeCommand(text);
        return;
    }
    PacketUtils::sendChatMessage(text);
}

std::string ChatBinds::keyName(int vk)
{
    switch (vk)
    {
    case 0: return "none";
    case VK_TAB: return "TAB";       case VK_SPACE: return "SPACE";   case VK_RETURN: return "ENTER";
    case VK_BACK: return "BKSP";     case VK_CAPITAL: return "CAPS";  case VK_ESCAPE: return "ESC";
    case VK_DELETE: return "DEL";    case VK_INSERT: return "INS";    case VK_HOME: return "HOME";
    case VK_END: return "END";       case VK_PRIOR: return "PGUP";    case VK_NEXT: return "PGDN";
    case VK_UP: return "UP";         case VK_DOWN: return "DOWN";     case VK_LEFT: return "LEFT";
    case VK_RIGHT: return "RIGHT";
    case VK_SHIFT: case VK_LSHIFT: return "SHIFT";       case VK_RSHIFT: return "RSHIFT";
    case VK_CONTROL: case VK_LCONTROL: return "CTRL";    case VK_RCONTROL: return "RCTRL";
    case VK_MENU: case VK_LMENU: return "ALT";           case VK_RMENU: return "RALT";
    case VK_XBUTTON1: return "MB4";  case VK_XBUTTON2: return "MB5";
    case VK_OEM_MINUS: return "-";   case VK_OEM_PLUS: return "=";    case VK_OEM_COMMA: return ",";
    case VK_OEM_PERIOD: return ".";  case VK_OEM_1: return ";";       case VK_OEM_2: return "/";
    case VK_OEM_3: return "`";       case VK_OEM_4: return "[";       case VK_OEM_6: return "]";
    case VK_OEM_5: return "\\";      case VK_OEM_7: return "'";
    default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return "NUM" + std::to_string(vk - VK_NUMPAD0);
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, static_cast<char>(vk));
    return "K" + std::to_string(vk);
}
