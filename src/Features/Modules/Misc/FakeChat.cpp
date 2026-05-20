#include "FakeChat.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void FakeChat::onEnable() {
    gFeatureManager->mDispatcher->listen<PacketInEvent, &FakeChat::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &FakeChat::onRenderEvent>(this);
}

void FakeChat::onDisable() {
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &FakeChat::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &FakeChat::onRenderEvent>(this);
}

// ════════════════════════════════════════
//  Автоматически запоминаем ВСЕ сообщения
// ════════════════════════════════════════

void FakeChat::onPacketInEvent(PacketInEvent& event) {
    if (event.isCancelled()) return;
    if (event.mPacket->getId() != PacketID::Text) return;

    auto textPacket = event.getPacket<TextPacket>();

    // Debug
    if (mDebugMode) {
        ChatUtils::displayClientMessage("§8[DBG] §7type=§f" +
            std::to_string(static_cast<int>(textPacket->mType)) +
            " §7author=§f" +
            (textPacket->mAuthor.empty() ? "(empty)" : textPacket->mAuthor));
        std::string preview = textPacket->mMessage;
        if (preview.length() > 120) preview = preview.substr(0, 120) + "...";
        ChatUtils::displayClientMessage("§8[DBG] §7msg=§r" + preview);
    }

    // Сохраняем в историю
    if (!textPacket->mMessage.empty()) {
        mHistory.push_back(textPacket->mMessage);
        if (mHistory.size() > MAX_HISTORY) {
            mHistory.pop_front();
        }
    }
}

bool FakeChat::clonePrefix(const std::string& sourceName, const std::string& targetName) {
    // Проверяем что есть сохранённый префикс для source
    auto it = mPrefixes.find(sourceName);
    if (it == mPrefixes.end()) {
        ChatUtils::displayClientMessage("§c[FakeChat] §fNo prefix saved for §e" + sourceName);
        ChatUtils::displayClientMessage("§7First do: .fc said " + sourceName + " <what they said>");
        return false;
    }

    std::string newPrefix = it->second;

    // Ищем имя source в префиксе и заменяем на target
    size_t namePos = newPrefix.find(sourceName);
    if (namePos == std::string::npos) {
        // Попробуем без учёта регистра — на всякий случай
        // Но обычно имя есть как есть
        ChatUtils::displayClientMessage("§c[FakeChat] §fCouldn't find name §e" +
            sourceName + " §fin the prefix string");
        ChatUtils::displayClientMessage("§7Prefix: §r" + newPrefix);
        return false;
    }

    // Заменяем ВСЕ вхождения имени source на target
    // (на некоторых серверах ник может быть в префиксе дважды)
    while (namePos != std::string::npos) {
        newPrefix.replace(namePos, sourceName.length(), targetName);
        namePos = newPrefix.find(sourceName, namePos + targetName.length());
    }

    // Сохраняем новый префикс
    mPrefixes[targetName] = newPrefix;

    ChatUtils::displayClientMessage("§a[FakeChat] §fCloned prefix!");
    ChatUtils::displayClientMessage("§7Source: §e" + sourceName);
    ChatUtils::displayClientMessage("§7Target: §e" + targetName);
    ChatUtils::displayClientMessage("§7New prefix: §r" + newPrefix + "§8...");
    ChatUtils::displayClientMessage("");
    ChatUtils::displayClientMessage("§aNow use:");
    ChatUtils::displayClientMessage("§f  .fc now " + targetName + " <fake message>");
    ChatUtils::displayClientMessage("§f  .fc send " + targetName + " <sec> <fake message>");

    return true;
}

// ════════════════════════════════════════
//  Таймер — отправка запланированных
// ════════════════════════════════════════

void FakeChat::onRenderEvent(RenderEvent& event) {
    if (mQueue.empty()) return;

    auto now = std::chrono::steady_clock::now();

    for (auto it = mQueue.begin(); it != mQueue.end(); ) {
        if (now >= it->sendAt) {
            // Просто пишем в ванильный чат — БЕЗ пакетов, БЕЗ крашей
            ChatUtils::displayClientMessageRaw(it->text);
            it = mQueue.erase(it);
        } else {
            ++it;
        }
    }
}

// ════════════════════════════════════════
//  Поиск prefix в истории
//
//  История содержит:
//  "§aⓁ§r §7... §7[§l§cРазработчик§r§7] §l§4LX2213 §8» §fкамерамен топ"
//
//  playerName = "LX2213"
//  saidText   = "камерамен топ"
//
//  Находим "камерамен топ" в строке → всё ДО него = prefix
//  prefix = "§aⓁ§r §7... §7[§l§cРазработчик§r§7] §l§4LX2213 §8» §f"
// ════════════════════════════════════════

bool FakeChat::findPrefix(const std::string& playerName, const std::string& saidText) {
    // Ищем с конца (самые новые сообщения)
    for (auto it = mHistory.rbegin(); it != mHistory.rend(); ++it) {
        const std::string& msg = *it;

        // Сообщение должно содержать И имя игрока И текст
        if (msg.find(playerName) == std::string::npos) continue;
        if (msg.find(saidText) == std::string::npos) continue;

        // Нашли! Prefix = всё до текста
        size_t textPos = msg.find(saidText);
        mPrefixes[playerName] = msg.substr(0, textPos);

        ChatUtils::displayClientMessage("§a[FakeChat] §fPrefix saved for §e" + playerName + "§f!");
        ChatUtils::displayClientMessage("§7Prefix: §r" + mPrefixes[playerName] + "§8...");
        ChatUtils::displayClientMessage("");
        ChatUtils::displayClientMessage("§aNow use:");
        ChatUtils::displayClientMessage("§f  .fc send " + playerName + " <sec> <fake message>");
        ChatUtils::displayClientMessage("§f  .fc now " + playerName + " <fake message>");
        return true;
    }

    ChatUtils::displayClientMessage("§c[FakeChat] §fMessage not found in history!");
    ChatUtils::displayClientMessage("§7Make sure §e" + playerName + " §7has written something recently.");
    ChatUtils::displayClientMessage("§7And type EXACTLY what they said (plain text, no colors).");
    return false;
}

void FakeChat::schedule(const std::string& fullText, float delaySec) {
    ScheduledMsg msg;
    msg.text = fullText;
    msg.sendAt = std::chrono::steady_clock::now() +
                 std::chrono::milliseconds(static_cast<int64_t>(delaySec * 1000.0f));
    mQueue.push_back(msg);
}

void FakeChat::cancelAll() {
    size_t count = mQueue.size();
    mQueue.clear();
    ChatUtils::displayClientMessage("§a[FakeChat] §fCancelled " +
        std::to_string(count) + " message(s)");
}