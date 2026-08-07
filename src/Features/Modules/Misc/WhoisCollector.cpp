//
// WhoisCollector.cpp
// Автоматически делает whois на игроков выбранных рангов.
// Сохраняет данные в два файла:
// WhoisData.txt — все собранные записи
// WhoisMatches.txt — пары игроков с совпадающим IP или CID
//

#include "WhoisCollector.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/FileUtils.hpp>

#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

// ─── helpers ────────────────────────────────────────────────────────────────

// Правильно убирает пробелы, табуляцию и символы переноса строк с обоих концов строки
static void trim(std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        str.clear();
        return;
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    str = str.substr(first, (last - first + 1));
}

// Правильно убирает §X-коды из строки (Minecraft color codes).
// § в UTF-8 = \xC2\xA7 (2 байта), за которыми следует 1 символ кода цвета.
// Итого за каждый §X нужно пропустить 3 байта (\xC2 + \xA7 + code).
static std::string stripColors(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];

        // UTF-8 § = 0xC2 0xA7
        if (c == 0xC2 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0xA7) {
            // Пропускаем: \xC2, \xA7, и код цвета (следующий байт)
            i += 3;
            continue;
        }
        // Legacy/Latin-1 § = 0xA7 напрямую
        if (c == 0xA7) {
            i += 2; // пропускаем § и код цвета
            continue;
        }
        out += s[i];
        ++i;
    }
    return out;
}

// Извлекает чистый ник из полного тега вида "[Ранг] Ник§r".
// Формат сервера: §7[§ePlayer§7] §r§7_poop_boy§r
// После stripColors: "[Player] _poop_boy"
// Возвращаемый результат: "_poop_boy"
static std::string extractNick(const std::string& rawTag) {
    // Сначала снимаем все §X коды
    std::string stripped = stripColors(rawTag);
    trim(stripped);

    if (stripped.empty()) return "";

    // Ищем последнее вхождение "]" — ник идёт после него
    auto pos = stripped.rfind(']');
    if (pos != std::string::npos) {
        std::string nick = stripped.substr(pos + 1);
        trim(nick);
        if (!nick.empty()) return nick;
    }

    // Если скобок нет — возвращаем всю строку как есть (уже stripped)
    return stripped;
}

// Разрезает строку по первому вхождению sep, возвращает часть после sep
static std::string afterFirst(const std::string& str, const std::string& sep) {
    auto pos = str.find(sep);
    if (pos == std::string::npos) return "";
    return str.substr(pos + sep.size());
}

// Проверяет содержит ли строка подстроку (без учёта регистра — ASCII)
static bool containsStr(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;
    for (size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
        if (haystack.substr(i, needle.size()) == needle) return true;
    }
    return false;
}

// ─── onEnable ────────────────────────────────────────────────────────────────

void WhoisCollector::onEnable() {
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &WhoisCollector::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &WhoisCollector::onPacketInEvent>(this);

    mWhoisQueue.clear();
    mLastSentTime = 0;

    // Пути к файлам: в папку клиента (RoamingState/Juzdex/)
    std::string dir = FileUtils::getSolsticeDir();
    mDataFile  = dir + "WhoisData.txt";
    mMatchFile = dir + "WhoisMatches.txt";
    mDebugFile = dir + "WhoisDebug.txt";

    // Логируем путь чтобы пользователь знал где искать файлы
    spdlog::info("[WhoisCollector] Файлы будут сохранены в: {}", dir);
    ChatUtils::displayClientMessage("§7[WhoisCollector] Файлы: §f" + dir);

    // Загружаем ранее собранные данные чтобы не делать повторный whois
    if (!mRescan.mValue) {
        std::ifstream f(mDataFile);
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                // Новый формат: "Ник - IP - CID - Ранг"
                auto pos = line.find(" - ");
                if (pos != std::string::npos) {
                    std::string name = line.substr(0, pos);
                    trim(name);
                    if (!name.empty())
                        mAlreadyQueried.insert(name);
                }
                // Старый формат: "Ник: Steve"
                else if (containsStr(line, "Ник: ")) {
                    std::string name = afterFirst(line, "Ник: ");
                    trim(name);
                    if (!name.empty())
                        mAlreadyQueried.insert(name);
                }
            }
        }
    } else {
        // rescan — сбрасываем список уже опрошенных
        mAlreadyQueried.clear();
    }

    ChatUtils::displayClientMessage("§a[WhoisCollector] §fВключён.");
    if (!mAlreadyQueried.empty() && !mRescan.mValue) {
        ChatUtils::displayClientMessage("§7Уже собраны данные §e" +
            std::to_string(mAlreadyQueried.size()) + "§7 игрок(ов). Повторно пропускаем.");
    }
}

// ─── onDisable ───────────────────────────────────────────────────────────────

void WhoisCollector::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &WhoisCollector::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &WhoisCollector::onPacketInEvent>(this);

    mWhoisQueue.clear();

    ChatUtils::displayClientMessage("§c[WhoisCollector] §fВыключен. Собрано §e" +
        std::to_string(mCollected.size()) + "§f записей.");
}

// ─── isRankEnabled ───────────────────────────────────────────────────────────

bool WhoisCollector::isRankEnabled(const std::string& rankName) const {
    if (mSelectAll.mValue) return true;

    // Убираем § форматирование и пробелы
    std::string clean = stripColors(rankName);
    trim(clean);

    // Проверяем вхождение подстроки
    auto has = [&](const std::string& needle) {
        return containsStr(clean, needle);
    };

    // Разработчик / Developer
    if (has("Разработчик") || has("Developer") || has("Dev"))
        return mRankDev.mValue;

    // Создатель / Sozdatel
    if (has("Создатель") || has("Sozdatel") || has("Creator"))
        return mRankCreator.mValue;

    // Основатель / Osnovatel
    if (has("Основатель") || has("Osnovatel") || has("Founder"))
        return mRankFounder.mValue;

    // Бог / Bog
    if (has("Бог") || has("Bog") || has("God"))
        return mRankGod.mValue;

    // Анти-гриф / AntiGrif
    if (has("Анти-гриф") || has("Анти-Гриф") || has("Антигриф") || has("Анти гриф") ||
        has("AntiGrif")  || has("AntiGrief")  || has("Antigriif"))
        return mRankAntiGrief.mValue;

    // Повелитель / Povelitel
    if (has("Повелитель") || has("Povelitel") || has("Sovereign") || has("Lord"))
        return mRankSovereign.mValue;

    // Оператор / Operator
    if (has("Оператор") || has("Operator"))
        return mRankOperator.mValue;

    // Администратор / Админ / Admin
    if (has("Администратор") || has("Administrator") || has("Админ") || has("Admin"))
        return mRankAdmin.mValue;

    // Модератор / Moderator
    if (has("Модератор") || has("Moderator"))
        return mRankModerator.mValue;

    // Креатив / Creativ
    if (has("Креатив") || has("Creativ") || has("Creative"))
        return mRankCreative.mValue;

    // Премиум / Premium
    if (has("Премиум") || has("Premium"))
        return mRankPremium.mValue;

    // Временный флай / TempFly
    if (has("Временный флай") || has("Временный Флай") ||
        has("TempFly") || has("Temp fly") || has("TemporaryFly"))
        return mRankTempFly.mValue;

    // Флай / Fly
    if (has("Флай") || has("Fly"))
        return mRankFly.mValue;

    // Player — обычный игрок
    if (has("Player") || has("Игрок"))
        return mRankPlayer.mValue;

    // Пустой ранг — обычный игрок
    if (clean.empty())
        return mRankPlayer.mValue;

    return false;
}

// ─── extractRank ─────────────────────────────────────────────────────────────

// Извлекает ранг из тега вида "[Ранг] Ник".
static std::string extractRank(const std::string& rawTag) {
    std::string stripped = stripColors(rawTag);
    trim(stripped);

    // Формат: "[Ранг] Ник" — ищем первое '[' и первое ']' после него
    auto lb = stripped.find('[');
    if (lb != std::string::npos) {
        auto rb = stripped.find(']', lb);
        if (rb != std::string::npos && rb > lb) {
            std::string rank = stripped.substr(lb + 1, rb - lb - 1);
            trim(rank);
            return rank;
        }
    }
    return "";
}

// ─── getOnlinePlayers ────────────────────────────────────────────────────────

std::vector<WhoisCollector::PlayerInfo> WhoisCollector::getOnlinePlayers() {
    std::vector<PlayerInfo> result;
    auto ci = ClientInstance::get();
    if (!ci) return result;

    auto player = ci->getLocalPlayer();
    if (!player) return result;

    auto level = player->getLevel();
    if (!level) return result;

    std::string localNick;
    try { localNick = extractNick(player->getRawName()); } catch (...) {}

    // Шаг 1: Обновляем кэш рангов из actor list
    auto actors = level->getRuntimeActorList();
    for (Actor* actor : actors) {
        if (!actor || !actor->isValid() || !actor->isPlayer()) continue;
        if (actor == player) continue;
        std::string rawTag;
        try { rawTag = actor->getRawName(); } catch (...) { continue; }
        if (rawTag.empty()) continue;
        std::string nick = extractNick(rawTag);
        std::string rank = extractRank(rawTag);
        if (!nick.empty() && nick != localNick && !rank.empty())
            mRankCache[nick] = rank;
    }

    // Шаг 2: Основной список из PlayerList
    auto playerList = level->getPlayerList();
    if (playerList) {
        for (auto& [uuid, entry] : *playerList) {
            if (entry.mName.empty()) continue;

            std::string nick = extractNick(entry.mName);
            if (nick.empty() || nick == localNick) continue;

            PlayerInfo info;
            info.nick = nick;

            auto it = mRankCache.find(nick);
            if (it != mRankCache.end()) {
                info.rank = it->second;
            } else {
                info.rank = extractRank(entry.mName);
            }

            result.push_back(std::move(info));
        }
    }

    return result;
}

// Устаревшая перегрузка — только имена
std::vector<std::string> WhoisCollector::getOnlinePlayerNames() {
    std::vector<std::string> names;
    for (auto& p : getOnlinePlayers()) names.push_back(p.nick);
    return names;
}

// ─── onBaseTickEvent ─────────────────────────────────────────────────────────

void WhoisCollector::onBaseTickEvent(BaseTickEvent& event) {
    auto player = event.mActor;
    if (!player) return;

    // Проверяем онлайн раз в 3 секунды
    static uint64_t lastCheck = 0;
    if (NOW - lastCheck > 3000) {
        lastCheck = NOW;
        auto online = getOnlinePlayers();
        for (auto& info : online) {
            // Если уже чекали или он уже в очереди - пропускаем
            if (!mRescan.mValue && mAlreadyQueried.count(info.nick)) continue;
            if (std::find(mWhoisQueue.begin(), mWhoisQueue.end(), info.nick) != mWhoisQueue.end()) continue;
            
            // Проверяем по предварительному рангу из таба
            if (!isRankEnabled(info.rank)) continue;

            mWhoisQueue.push_back(info.nick);
        }
    }

    if (mWhoisQueue.empty()) return;

    // Задержка отправки
    uint64_t delayMs = static_cast<uint64_t>(mDelay.mValue * 1000.0f);
    if (NOW - mLastSentTime >= delayMs) {
        std::string nick = mWhoisQueue.front();
        mWhoisQueue.pop_front();

        // Сразу отмечаем, чтобы не добавить в очередь повторно
        mAlreadyQueried.insert(nick);
        PacketUtils::sendChatMessage("/whois " + nick);
        mLastSentTime = NOW;
        spdlog::info("[WhoisCollector] Sent queued whois for {}", nick);
    }
}

// ─── onPacketInEvent ─────────────────────────────────────────────────────────

void WhoisCollector::onPacketInEvent(PacketInEvent& event) {
    if (event.mPacket->getId() != PacketID::Text) return;

    auto tp = event.getPacket<TextPacket>();
    if (!tp) return;
    const std::string& msg = tp->mMessage;
    if (msg.empty()) return;

    // RAW Лог
    if (mDebugLog.mValue) {
        std::ofstream dbg(mDebugFile, std::ios::app);
        if (dbg.is_open()) {
            dbg << "[RAW]   " << msg << "\n";
            dbg << "[CLEAN] " << stripColors(msg) << "\n";
            dbg << "---\n";
        }
    }

    std::string cleanMsg = stripColors(msg);

    // Если это сообщение с ответом от whois (весь ответ приходит одним пакетом)
    std::string search1 = "Информация о игроке ";
    std::string search2 = "Информация об игроке ";
    size_t pos = cleanMsg.find(search1);
    if (pos == std::string::npos) pos = cleanMsg.find(search2);

    if (pos != std::string::npos) {
        // Парсим ник
        size_t nameStart = pos + (cleanMsg.find(search1) != std::string::npos ? search1.size() : search2.size());
        size_t nameEnd = cleanMsg.find('\n', nameStart);
        std::string nick = (nameEnd == std::string::npos) ? cleanMsg.substr(nameStart) : cleanMsg.substr(nameStart, nameEnd - nameStart);
        trim(nick);
        if (nick.empty()) return;

        // Парсим данные
        WhoisEntry entry;
        entry.name = nick;
        entry.collectedAt = NOW;
        entry.rank = "Player"; // Дефолт, если нет строки Донат

        auto extract = [&](const std::string& key) -> std::string {
            auto kpos = cleanMsg.find(key);
            if (kpos == std::string::npos) return "";
            size_t start = kpos + key.size();
            size_t end = cleanMsg.find('\n', start);
            std::string val = (end == std::string::npos) ? cleanMsg.substr(start) : cleanMsg.substr(start, end - start);
            trim(val);
            return val;
        };

        entry.ip = extract("IP:");
        entry.cid = extract("CID:");
        entry.ping = extract("Ping:");
        std::string rankRaw = extract("Донат:");
        if (!rankRaw.empty()) entry.rank = rankRaw;

        // Проверяем валидность
        if (entry.ip.empty() && entry.cid.empty()) return;

        // Проверяем ранг
        if (!isRankEnabled(entry.rank)) {
            spdlog::info("[WhoisCollector] Пропускаем {} (ранг: '{}')", nick, entry.rank);
            ChatUtils::displayClientMessage("§e[WhoisCollector] §7Пропущен §b" + nick + " §7(ранг §e" + stripColors(entry.rank) + "§7 не включен в меню)");
            mAlreadyQueried.insert(nick);
            return;
        }

        // Сохраняем
        bool isNewPlayer = (mCollected.find(nick) == mCollected.end());
        if (mRescan.mValue && !isNewPlayer) {
            auto& old = mCollected[nick];
            bool ipChanged  = (!entry.ip.empty()  && old.ip  != entry.ip);
            bool cidChanged = (!entry.cid.empty() && old.cid != entry.cid);
            if (ipChanged || cidChanged) {
                std::string note = "§e[WhoisCollector] §fИзменения у §b" + nick + "§f:";
                if (ipChanged)  note += " IP: §c" + old.ip  + "§f→§a" + entry.ip;
                if (cidChanged) note += " CID: §c" + old.cid + "§f→§a" + entry.cid;
                ChatUtils::displayClientMessage(note);
                old.ip = entry.ip; old.cid = entry.cid;
                old.rank = entry.rank; old.collectedAt = entry.collectedAt;
                std::ofstream f(mDataFile, std::ios::app);
                if (f.is_open()) {
                    f << "\n[RESCAN UPDATE] Ник: " << nick << "\n";
                    if (ipChanged)  f << "  IP изменён: " << old.ip << " -> " << entry.ip << "\n";
                    if (cidChanged) f << "  CID изменён: " << old.cid << " -> " << entry.cid << "\n";
                }
            }
        } else if (isNewPlayer) {
            mCollected[nick] = entry;
            saveEntry(entry);
            checkAndSaveMatches(entry);
            if (mNotifyNewPlayer.mValue) {
                ChatUtils::displayClientMessage(
                    "§a[WhoisCollector] §fСохранён: §b" + nick +
                    " §7| IP: §f" + entry.ip +
                    " §7| CID: §f" + entry.cid +
                    " §7| Ранг: §e" + stripColors(entry.rank));
            }
        }
        
        mAlreadyQueried.insert(nick);
    }
}

// ─── saveEntry ───────────────────────────────────────────────────────────────

void WhoisCollector::saveEntry(const WhoisEntry& entry) {
    std::ofstream f(mDataFile, std::ios::app);
    if (!f.is_open()) {
        spdlog::warn("[WhoisCollector] Не удалось открыть файл: {}", mDataFile);
        return;
    }

    f << entry.name << " - " << entry.ip << " - " << entry.cid << " - " << stripColors(entry.rank) << "\n";
    spdlog::info("[WhoisCollector] Saved entry for {}", entry.name);
}

// ─── checkAndSaveMatches ────────────────────────────────────────────────────

void WhoisCollector::checkAndSaveMatches(const WhoisEntry& newEntry) {
    for (auto& [name, old] : mCollected) {
        if (name == newEntry.name) continue;
        if (old.ip.empty() && old.cid.empty()) continue;

        bool ipMatch  = (!newEntry.ip.empty()  && !old.ip.empty()  && newEntry.ip  == old.ip);
        bool cidMatch = (!newEntry.cid.empty() && !old.cid.empty() && newEntry.cid == old.cid);

        if (!ipMatch && !cidMatch) continue;

        std::string matchType = ipMatch && cidMatch ? "IP+CID" : (ipMatch ? "IP" : "CID");

        ChatUtils::displayClientMessage(
            "§c[WhoisCollector] §fСОВПАДЕНИЕ §e" + matchType + "§f: §b" +
            newEntry.name + " §f& §b" + name);

        // Сохраняем в файл совпадений
        std::ofstream f(mMatchFile, std::ios::app);
        if (f.is_open()) {
            f << "============================\n";
            f << "Тип совпадения: " << matchType << "\n";
            f << "\n";
            f << "Игрок 1: " << newEntry.name << "\n";
            f << "  Ранг: "  << stripColors(newEntry.rank) << "\n";
            f << "  IP: "    << newEntry.ip   << "\n";
            f << "  CID: "   << newEntry.cid  << "\n";
            f << "\n";
            f << "Игрок 2: " << old.name      << "\n";
            f << "  Ранг: "  << stripColors(old.rank)     << "\n";
            f << "  IP: "    << old.ip        << "\n";
            f << "  CID: "   << old.cid       << "\n";
            f << "\n";
        }

        spdlog::warn("[WhoisCollector] Match found ({}) between {} and {}", matchType, newEntry.name, name);
    }
}