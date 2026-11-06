#include "AdminPanel.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/FileUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

namespace admin_panel_detail {

constexpr std::array<const char*, 5> kProtectedNicks = {
    "juzzik01x",
    "xjuzzikx",
    "presty01x",
    "qahzoo",
    "juzzik1x",
};

void trim(std::string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        value.clear();
        return;
    }

    const size_t last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
}

// Настоящий код цвета/стиля Minecraft: 0-9, a-f, k-o, r.
static bool adminIsCodeChar(unsigned char c) {
    if (c >= '0' && c <= '9') return true;

    const unsigned char lower = (c >= 'A' && c <= 'Z') ? (unsigned char)(c + 32) : c;
    if (lower >= 'a' && lower <= 'f') return true;

    return lower == 'k' || lower == 'l' || lower == 'm' ||
           lower == 'n' || lower == 'o' || lower == 'r';
}

std::string stripColors(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size();) {
        const unsigned char current = static_cast<unsigned char>(value[i]);
        if (current == 0xC2 && i + 1 < value.size() &&
            static_cast<unsigned char>(value[i + 1]) == 0xA7) {
            i += 2; // \xC2 + \xA7 — это сам «§»
            if (i < value.size() && adminIsCodeChar(static_cast<unsigned char>(value[i]))) i += 1;
            continue;
        }
        // Одиночный байт «§», но только если это не хвост UTF-8-символа:
        // 'Ч' = D0 A7, и такой байт выбрасывать нельзя.
        if (current == 0xA7 && (i == 0 || static_cast<unsigned char>(value[i - 1]) < 0xC0)) {
            i += 1;
            if (i < value.size() && adminIsCodeChar(static_cast<unsigned char>(value[i]))) i += 1;
            continue;
        }

        result.push_back(value[i]);
        ++i;
    }

    return result;
}

std::string lowercaseNick(std::string value) {
    trim(value);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool contains(const std::string& value, const std::string& part) {
    return value.find(part) != std::string::npos;
}

std::string extractNick(const std::string& rawTag) {
    std::string clean = stripColors(rawTag);
    trim(clean);
    if (clean.empty()) return {};

    const size_t bracket = clean.rfind(']');
    if (bracket != std::string::npos) {
        std::string nick = clean.substr(bracket + 1);
        trim(nick);
        if (!nick.empty()) return nick;
    }

    return clean;
}

std::string extractRank(const std::string& rawTag) {
    std::string clean = stripColors(rawTag);
    trim(clean);

    const size_t left = clean.find('[');
    if (left == std::string::npos) return {};

    const size_t right = clean.find(']', left + 1);
    if (right == std::string::npos || right <= left + 1) return {};

    std::string rank = clean.substr(left + 1, right - left - 1);
    trim(rank);
    return rank;
}

uint64_t parseCooldownMilliseconds(const std::string& cleanMessage) {
    // Один и тот же парсер подходит сообщениям вроде:
    // "... снова через 10 минут" и "... снова через 9 минут, 58 секунд".
    size_t marker = cleanMessage.find("использовать снова через");
    if (marker == std::string::npos)
        marker = cleanMessage.find("использовать команду снова через");
    if (marker == std::string::npos)
        marker = cleanMessage.find("use this command again in");

    // Запасной вариант для немного другого текста сервера:
    // "Эту команду можно будет повторно использовать через ...".
    if (marker == std::string::npos && contains(cleanMessage, "команд") &&
        (contains(cleanMessage, "снова") || contains(cleanMessage, "повтор") ||
         contains(cleanMessage, "можно"))) {
        marker = cleanMessage.find("через");
    }
    if (marker == std::string::npos) return 0;

    const std::string tail = cleanMessage.substr(marker);
    uint64_t seconds = 0;

    for (size_t i = 0; i < tail.size();) {
        if (!std::isdigit(static_cast<unsigned char>(tail[i]))) {
            ++i;
            continue;
        }

        uint64_t number = 0;
        while (i < tail.size() && std::isdigit(static_cast<unsigned char>(tail[i]))) {
            const unsigned digit = static_cast<unsigned>(tail[i] - '0');
            if (number <= (std::numeric_limits<uint64_t>::max() - digit) / 10)
                number = number * 10 + digit;
            ++i;
        }

        const size_t unitStart = i;
        while (i < tail.size() && !std::isdigit(static_cast<unsigned char>(tail[i]))) ++i;
        const std::string unit = tail.substr(unitStart, i - unitStart);

        if (contains(unit, "час") || contains(unit, "hour"))
            seconds += number * 60 * 60;
        else if (contains(unit, "мин") || contains(unit, "minute"))
            seconds += number * 60;
        else if (contains(unit, "сек") || contains(unit, "second"))
            seconds += number;
    }

    return seconds * 1000;
}

bool isSuccessMessage(const std::string& cleanMessage, const std::string& nick,
                      AdminPanelAction action) {
    // Никнеймы Minecraft состоят из ASCII-символов, поэтому для ника достаточно
    // ASCII lower-case. Русские служебные фразы сервера остаются без изменений.
    std::string messageLower = cleanMessage;
    std::transform(messageLower.begin(), messageLower.end(), messageLower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string nickLower = lowercaseNick(nick);
    if (nickLower.empty() || !contains(messageLower, nickLower)) return false;

    switch (action) {
        case AdminPanelAction::Ban:
        case AdminPanelAction::BanIp:
        case AdminPanelAction::BanOff:
            return contains(cleanMessage, "был забанен") ||
                   contains(cleanMessage, "забанен игроком") ||
                   contains(messageLower, " was banned") ||
                   contains(messageLower, "banned player");
        case AdminPanelAction::Mute:
            return contains(cleanMessage, "был замучен") ||
                   contains(cleanMessage, "получил мут") ||
                   contains(cleanMessage, "выдан мут") ||
                   contains(messageLower, " was muted");
        case AdminPanelAction::Kick:
            return contains(cleanMessage, "был кикнут") ||
                   contains(cleanMessage, "был выгнан") ||
                   contains(cleanMessage, "кикнут игроком") ||
                   contains(messageLower, " was kicked");
    }

    return false;
}

} // namespace admin_panel_detail

void AdminPanel::onEnable() {
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AdminPanel::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &AdminPanel::onPacketInEvent>(this);

    mQueue.clear();
    mQueued.clear();
    mProcessed.clear();
    mRankCache.clear();
    mLastScanTime = 0;
    mLastSentTime = 0;
    mCooldownUntil = 0;
    mAwaitingResponse = false;
    mLastTarget = {};
    mSentCount = 0;
    mWhitelistSkipCount = 0;
    mImmunitySkipCount = 0;

    mActiveAction = mAction.as<AdminPanelAction>();
    mActiveBanTime = std::max(0, static_cast<int>(std::round(mBanTime.mValue)));

    mWhitelistFile = FileUtils::getSolsticeDir() + "AdminPanelWhitelist.txt";
    ensureWhitelistFile();
    loadWhitelist();

    ChatUtils::displayClientMessage(
        "§a[AdminPanel] §fВключён. Команда: §e" + actionName(mActiveAction) +
        "§f, причина: §e" + REASON + "§f. Обход идёт от высшего ранга к низшему.");
    ChatUtils::displayClientMessage(
        "§7[AdminPanel] Белый список: §f" + mWhitelistFile);
}

void AdminPanel::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AdminPanel::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &AdminPanel::onPacketInEvent>(this);

    mQueue.clear();
    mQueued.clear();
    mAwaitingResponse = false;

    ChatUtils::displayClientMessage(
        "§c[AdminPanel] §fВыключен. Отправлено команд: §e" +
        std::to_string(mSentCount) + "§f, белый список: §e" +
        std::to_string(mWhitelistSkipCount) + "§f, иммунитет: §e" +
        std::to_string(mImmunitySkipCount) + "§f.");
}

void AdminPanel::onBaseTickEvent(BaseTickEvent& event) {
    if (!event.mActor) return;

    // Обновляем список онлайна. Новые игроки тоже попадут в очередь, но каждый
    // ник обрабатывается только один раз до следующего включения модуля.
    if (mLastScanTime == 0 || NOW - mLastScanTime >= 1000) {
        mLastScanTime = NOW;
        loadWhitelist();
        scanPlayers();
    }

    // Пока ждём ответ на последнюю команду, следующую цель не трогаем. Это
    // не даёт позднему сообщению о КД привязаться уже к другому игроку.
    // Успешный ответ (например, "был забанен") снимает ожидание раньше.
    if (mAwaitingResponse) {
        if (NOW - mLastSentTime <= 5000) return;
        mAwaitingResponse = false; // У сервера может не быть сообщения для kick/mute.
    }

    if (mCooldownUntil != 0) {
        if (NOW < mCooldownUntil) return;
        mCooldownUntil = 0;
        ChatUtils::displayClientMessage("§a[AdminPanel] §fКД закончилось, очередь продолжена.");
    }

    if (mQueue.empty()) return;

    const uint64_t delayMs = static_cast<uint64_t>(mDelay.mValue * 1000.f);
    if (!mLastTarget.nick.empty() && NOW - mLastSentTime < delayMs) return;

    // Файл можно отредактировать даже во время работы. Перед самой отправкой
    // перечитываем его ещё раз, чтобы ник из свежего whitelist не пострадал.
    loadWhitelist();

    while (!mQueue.empty()) {
        AdminPanelTarget target = mQueue.front();
        mQueue.pop_front();

        const std::string key = admin_panel_detail::lowercaseNick(target.nick);
        mQueued.erase(key);

        if (isWhitelisted(target.nick)) {
            mProcessed.insert(key);
            ++mWhitelistSkipCount;
            spdlog::info("[AdminPanel] Whitelist skipped {}", target.nick);
            continue;
        }

        if (!isAllowedByCommand(target.rank)) {
            mProcessed.insert(key);
            ++mImmunitySkipCount;
            spdlog::info("[AdminPanel] Immunity skipped {} ({})", target.nick, rankName(target.rank));
            continue;
        }

        const std::string command = buildCommand(target.nick);
        if (command.empty()) return;

        PacketUtils::sendChatMessage(command);
        mProcessed.insert(key);
        mLastTarget = target;
        mLastSentTime = NOW;
        mAwaitingResponse = true;
        ++mSentCount;

        spdlog::info("[AdminPanel] Sent {} for {} ({})", actionName(mActiveAction),
                     target.nick, rankName(target.rank));
        return; // Не больше одной команды за тик.
    }
}

void AdminPanel::onPacketInEvent(PacketInEvent& event) {
    if (event.mPacket->getId() != PacketID::Text || !mAwaitingResponse) return;
    if (NOW - mLastSentTime > 10000) return;

    auto packet = event.getPacket<TextPacket>();
    if (!packet || packet->mMessage.empty()) return;

    const std::string clean = admin_panel_detail::stripColors(packet->mMessage);
    uint64_t cooldownMs = admin_panel_detail::parseCooldownMilliseconds(clean);
    if (cooldownMs == 0) {
        // Подтверждение успеха вида "Игрок Steve был забанен..." разрешает
        // перейти к следующей цели, не ожидая защитный пятисекундный таймаут.
        if (admin_panel_detail::isSuccessMessage(
                clean, mLastTarget.nick, mActiveAction)) {
            mAwaitingResponse = false;
            spdlog::info("[AdminPanel] Server confirmed {} for {}",
                         actionName(mActiveAction), mLastTarget.nick);
        }
        return;
    }

    // Команда, после которой сервер показал КД, не сработала. Возвращаем цель
    // в очередь и повторяем её после окончания указанного сервером времени.
    const std::string key = admin_panel_detail::lowercaseNick(mLastTarget.nick);
    mProcessed.erase(key);

    loadWhitelist();
    if (!mLastTarget.nick.empty() && !isWhitelisted(mLastTarget.nick) &&
        !mQueued.contains(key)) {
        mQueue.push_back(mLastTarget);
        mQueued.insert(key);
        sortQueue();
    }

    // Небольшой запас, чтобы не отправить повтор ровно на границе серверного КД.
    mCooldownUntil = NOW + cooldownMs + 1000;
    mAwaitingResponse = false;

    const uint64_t totalSeconds = cooldownMs / 1000;
    ChatUtils::displayClientMessage(
        "§e[AdminPanel] §fСервер сообщил КД §e" +
        std::to_string(totalSeconds / 60) + " мин " +
        std::to_string(totalSeconds % 60) +
        " сек§f. Цель §b" + mLastTarget.nick + " §fвозвращена в очередь.");
}

void AdminPanel::scanPlayers() {
    for (const auto& target : getOnlinePlayers()) {
        if (!isRankSelected(target.rank)) continue;
        enqueueTarget(target);
    }
}

std::vector<AdminPanelTarget> AdminPanel::getOnlinePlayers() {
    std::vector<AdminPanelTarget> result;

    auto client = ClientInstance::get();
    if (!client) return result;

    auto localPlayer = client->getLocalPlayer();
    if (!localPlayer) return result;

    auto level = localPlayer->getLevel();
    if (!level) return result;

    std::string localNick;
    try {
        localNick = admin_panel_detail::extractNick(localPlayer->getRawName());
    } catch (...) {
    }
    const std::string localKey = admin_panel_detail::lowercaseNick(localNick);

    // Сначала берём форматированные NameTag актёров — там находится префикс.
    for (Actor* actor : level->getRuntimeActorList()) {
        if (!actor || actor == localPlayer || !actor->isValid() || !actor->isPlayer()) continue;

        std::string rawTag;
        try {
            rawTag = actor->getRawName();
        } catch (...) {
            continue;
        }
        if (rawTag.empty()) continue;

        const std::string nick = admin_panel_detail::extractNick(rawTag);
        const AdminPanelRank rank = parseRank(admin_panel_detail::extractRank(rawTag));
        const std::string key = admin_panel_detail::lowercaseNick(nick);
        if (!key.empty() && key != localKey && rank != AdminPanelRank::Unknown)
            mRankCache[key] = rank;
    }

    std::unordered_set<std::string> added;
    auto playerList = level->getPlayerList();
    if (!playerList) return result;

    for (auto& [uuid, entry] : *playerList) {
        if (entry.mName.empty()) continue;

        const std::string nick = admin_panel_detail::extractNick(entry.mName);
        const std::string key = admin_panel_detail::lowercaseNick(nick);
        if (key.empty() || key == localKey || added.contains(key)) continue;

        AdminPanelRank rank = AdminPanelRank::Unknown;
        const auto cached = mRankCache.find(key);
        if (cached != mRankCache.end())
            rank = cached->second;
        else
            rank = parseRank(admin_panel_detail::extractRank(entry.mName));

        // Как и в WhoisCollector, отсутствие префикса означает обычного Player.
        if (rank == AdminPanelRank::Unknown &&
            admin_panel_detail::extractRank(entry.mName).empty())
            rank = AdminPanelRank::Player;

        if (rank == AdminPanelRank::Unknown) continue;

        result.push_back({nick, rank});
        added.insert(key);
    }

    std::stable_sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return static_cast<int>(left.rank) > static_cast<int>(right.rank);
    });
    return result;
}

void AdminPanel::enqueueTarget(const AdminPanelTarget& target) {
    const std::string key = admin_panel_detail::lowercaseNick(target.nick);
    if (key.empty() || mQueued.contains(key) || mProcessed.contains(key)) return;

    if (isWhitelisted(target.nick)) {
        mProcessed.insert(key);
        ++mWhitelistSkipCount;
        return;
    }

    if (!isAllowedByCommand(target.rank)) {
        mProcessed.insert(key);
        ++mImmunitySkipCount;
        return;
    }

    mQueue.push_back(target);
    mQueued.insert(key);
    sortQueue();
}

void AdminPanel::sortQueue() {
    std::stable_sort(mQueue.begin(), mQueue.end(), [](const auto& left, const auto& right) {
        return static_cast<int>(left.rank) > static_cast<int>(right.rank);
    });
}

bool AdminPanel::isRankSelected(AdminPanelRank rank) const {
    if (mSelectAll.mValue) return rank != AdminPanelRank::Unknown;

    switch (rank) {
        case AdminPanelRank::Developer: return mRankDeveloper.mValue;
        case AdminPanelRank::Creator: return mRankCreator.mValue;
        case AdminPanelRank::Founder: return mRankFounder.mValue;
        case AdminPanelRank::God: return mRankGod.mValue;
        case AdminPanelRank::AntiGrief: return mRankAntiGrief.mValue;
        case AdminPanelRank::Sovereign: return mRankSovereign.mValue;
        case AdminPanelRank::Operator: return mRankOperator.mValue;
        case AdminPanelRank::Admin: return mRankAdmin.mValue;
        case AdminPanelRank::Moderator: return mRankModerator.mValue;
        case AdminPanelRank::Creative: return mRankCreative.mValue;
        case AdminPanelRank::Premium: return mRankPremium.mValue;
        case AdminPanelRank::Fly: return mRankFly.mValue;
        case AdminPanelRank::TempFly: return mRankTempFly.mValue;
        case AdminPanelRank::Player: return mRankPlayer.mValue;
        default: return false;
    }
}

bool AdminPanel::isAllowedByCommand(AdminPanelRank rank) const {
    if (rank == AdminPanelRank::Unknown) return false;

    int maximum = static_cast<int>(AdminPanelRank::Developer);
    switch (mActiveAction) {
        case AdminPanelAction::Mute:
        case AdminPanelAction::Kick:
            // Админа можно, Оператора и выше нельзя.
            maximum = static_cast<int>(AdminPanelRank::Admin);
            break;
        case AdminPanelAction::Ban:
        case AdminPanelAction::BanIp:
            // Повелителя можно, Анти-Гриф и выше — только banoff.
            maximum = static_cast<int>(AdminPanelRank::Sovereign);
            break;
        case AdminPanelAction::BanOff:
            // banoff может применяться к любому рангу.
            maximum = static_cast<int>(AdminPanelRank::Developer);
            break;
    }

    return static_cast<int>(rank) <= maximum;
}

std::string AdminPanel::buildCommand(const std::string& nick) const {
    switch (mActiveAction) {
        case AdminPanelAction::Ban:
            return "/ban " + nick + " " + std::to_string(mActiveBanTime) + " " + REASON;
        case AdminPanelAction::BanIp:
            return "/banip " + nick + " 0 " + REASON;
        case AdminPanelAction::BanOff:
            return "/banoff " + nick + " 0 " + REASON;
        case AdminPanelAction::Mute:
            return "/mute " + nick + " 20 " + REASON;
        case AdminPanelAction::Kick:
            return "/kick " + nick + " " + REASON;
    }
    return {};
}

void AdminPanel::ensureWhitelistFile() {
    if (FileUtils::fileExists(mWhitelistFile)) return;

    std::ofstream file(mWhitelistFile);
    if (!file.is_open()) {
        spdlog::warn("[AdminPanel] Не удалось создать whitelist: {}", mWhitelistFile);
        return;
    }

    for (const char* nick : admin_panel_detail::kProtectedNicks)
        file << nick << '\n';

    spdlog::info("[AdminPanel] Создан whitelist: {}", mWhitelistFile);
}

void AdminPanel::loadWhitelist() {
    mWhitelist.clear();

    // Эти пять ников защищены всегда, даже если пользователь случайно удалил
    // их из файла. Сравнение ников выполняется без учёта регистра.
    for (const char* nick : admin_panel_detail::kProtectedNicks)
        mWhitelist.insert(admin_panel_detail::lowercaseNick(nick));

    std::ifstream file(mWhitelistFile);
    if (!file.is_open()) return;

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        // Некоторые версии Блокнота добавляют UTF-8 BOM в начало файла.
        if (firstLine && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
        }
        firstLine = false;

        admin_panel_detail::trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Разрешаем писать комментарий после ника: Nick # причина.
        const size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        const std::string nick = admin_panel_detail::lowercaseNick(line);
        if (!nick.empty()) mWhitelist.insert(nick);
    }
}

bool AdminPanel::isWhitelisted(const std::string& nick) const {
    return mWhitelist.contains(admin_panel_detail::lowercaseNick(nick));
}

AdminPanelRank AdminPanel::parseRank(const std::string& rankNameValue) {
    std::string clean = admin_panel_detail::stripColors(rankNameValue);
    admin_panel_detail::trim(clean);

    std::string asciiLower = clean;
    std::transform(asciiLower.begin(), asciiLower.end(), asciiLower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    const auto has = [&](const std::string& russian, const std::string& english) {
        return admin_panel_detail::contains(clean, russian) ||
               admin_panel_detail::contains(asciiLower, english);
    };

    if (has("Разработчик", "developer") || admin_panel_detail::contains(asciiLower, "dev"))
        return AdminPanelRank::Developer;
    if (has("Создатель", "sozdatel") || admin_panel_detail::contains(asciiLower, "creator"))
        return AdminPanelRank::Creator;
    if (has("Основатель", "osnovatel") || admin_panel_detail::contains(asciiLower, "founder"))
        return AdminPanelRank::Founder;
    if (has("Бог", "bog") || admin_panel_detail::contains(asciiLower, "god"))
        return AdminPanelRank::God;
    if (admin_panel_detail::contains(clean, "Анти-Гриф") ||
        admin_panel_detail::contains(clean, "Анти-гриф") ||
        admin_panel_detail::contains(clean, "Антигриф") ||
        admin_panel_detail::contains(clean, "Анти гриф") ||
        admin_panel_detail::contains(asciiLower, "antigrif") ||
        admin_panel_detail::contains(asciiLower, "antigrief") ||
        admin_panel_detail::contains(asciiLower, "antigriif"))
        return AdminPanelRank::AntiGrief;
    if (has("Повелитель", "povelitel") ||
        admin_panel_detail::contains(asciiLower, "sovereign") ||
        admin_panel_detail::contains(asciiLower, "lord"))
        return AdminPanelRank::Sovereign;
    if (has("Оператор", "operator"))
        return AdminPanelRank::Operator;
    if (admin_panel_detail::contains(clean, "Администратор") ||
        admin_panel_detail::contains(clean, "Админ") ||
        admin_panel_detail::contains(asciiLower, "administrator") ||
        admin_panel_detail::contains(asciiLower, "admin"))
        return AdminPanelRank::Admin;
    if (has("Модератор", "moderator"))
        return AdminPanelRank::Moderator;
    if (has("Креатив", "creativ"))
        return AdminPanelRank::Creative;
    if (has("Премиум", "premium"))
        return AdminPanelRank::Premium;

    // Временный флай обязательно проверяется раньше обычного Флай.
    if (admin_panel_detail::contains(clean, "Временный флай") ||
        admin_panel_detail::contains(clean, "Временный Флай") ||
        admin_panel_detail::contains(asciiLower, "tempfly") ||
        admin_panel_detail::contains(asciiLower, "temp fly") ||
        admin_panel_detail::contains(asciiLower, "temporaryfly"))
        return AdminPanelRank::TempFly;
    if (has("Флай", "fly"))
        return AdminPanelRank::Fly;
    if (admin_panel_detail::contains(clean, "Игрок") ||
        admin_panel_detail::contains(asciiLower, "player") || clean.empty())
        return AdminPanelRank::Player;

    return AdminPanelRank::Unknown;
}

std::string AdminPanel::rankName(AdminPanelRank rank) {
    switch (rank) {
        case AdminPanelRank::Developer: return "Разработчик";
        case AdminPanelRank::Creator: return "Создатель";
        case AdminPanelRank::Founder: return "Основатель";
        case AdminPanelRank::God: return "Бог";
        case AdminPanelRank::AntiGrief: return "Анти-Гриф";
        case AdminPanelRank::Sovereign: return "Повелитель";
        case AdminPanelRank::Operator: return "Оператор";
        case AdminPanelRank::Admin: return "Админ";
        case AdminPanelRank::Moderator: return "Модератор";
        case AdminPanelRank::Creative: return "Креатив";
        case AdminPanelRank::Premium: return "Премиум";
        case AdminPanelRank::Fly: return "Флай";
        case AdminPanelRank::TempFly: return "Временный флай";
        case AdminPanelRank::Player: return "Player";
        default: return "Unknown";
    }
}

std::string AdminPanel::actionName(AdminPanelAction action) {
    switch (action) {
        case AdminPanelAction::Ban: return "ban";
        case AdminPanelAction::BanIp: return "banip";
        case AdminPanelAction::BanOff: return "banoff";
        case AdminPanelAction::Mute: return "mute";
        case AdminPanelAction::Kick: return "kick";
    }
    return "unknown";
}

std::string AdminPanel::getSettingDisplay() {
    const AdminPanelAction shownAction = mEnabled ? mActiveAction : mAction.as<AdminPanelAction>();
    if (mCooldownUntil > NOW) return actionName(shownAction) + " | Cooldown";
    if (!mQueue.empty())
        return actionName(shownAction) + " | Queue: " + std::to_string(mQueue.size());
    return actionName(shownAction) + " | Sent: " + std::to_string(mSentCount);
}