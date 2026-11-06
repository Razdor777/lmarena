#pragma once
//
// WhoisCollector — автоматически делает /whois на игроков выбранных рангов,
// сохраняет данные в файл и находит совпадения по IP/CID.
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <fstream>
#include <chrono>

// Список всех рангов сервера (в том порядке как на сайте)
// Индексы используются в BoolSetting ниже
static const std::vector<std::string> WHOIS_ALL_RANKS = {
    "Разработчик",
    "Создатель",
    "Основатель",
    "Бог",
    "Анти-Гриф",
    "Повелитель",
    "Оператор",
    "Админ",
    "Модератор",
    "Креатив",
    "Премиум",
    "Флай",
    "Временный флай",
    "Player"
};

// Данные об одном игроке полученные от whois
struct WhoisEntry {
    std::string name;
    std::string ip;
    std::string cid;
    std::string rank;
    std::string ping;
    uint64_t collectedAt = 0;
};

class WhoisCollector : public ModuleBase<WhoisCollector> {
public:
    // ── Настройки рангов ─────────────────────────────────────────────────
    BoolSetting mSelectAll      = BoolSetting("Все ранги", "Делать whois на всех игроков", true);
    BoolSetting mRankDev        = BoolSetting("Разработчик", "Включить ранг Разработчик", false);
    BoolSetting mRankCreator    = BoolSetting("Создатель", "Включить ранг Создатель", false);
    BoolSetting mRankFounder    = BoolSetting("Основатель", "Включить ранг Основатель", false);
    BoolSetting mRankGod        = BoolSetting("Бог", "Включить ранг Бог", false);
    BoolSetting mRankAntiGrief  = BoolSetting("Анти-Гриф", "Включить ранг Анти-Гриф", false);
    BoolSetting mRankSovereign  = BoolSetting("Повелитель", "Включить ранг Повелитель", false);
    BoolSetting mRankOperator   = BoolSetting("Оператор", "Включить ранг Оператор", false);
    BoolSetting mRankAdmin      = BoolSetting("Админ", "Включить ранг Админ", false);
    BoolSetting mRankModerator  = BoolSetting("Модератор", "Включить ранг Модератор", false);
    BoolSetting mRankCreative   = BoolSetting("Креатив", "Включить ранг Креатив", false);
    BoolSetting mRankPremium    = BoolSetting("Премиум", "Включить ранг Премиум", false);
    BoolSetting mRankFly        = BoolSetting("Флай", "Включить ранг Флай", false);
    BoolSetting mRankTempFly    = BoolSetting("Временный флай", "Включить ранг Временный флай", false);
    BoolSetting mRankPlayer     = BoolSetting("Player", "Включить обычных игроков без доната", false);

    // ── Общие настройки ──────────────────────────────────────────────────
    NumberSetting mDelay        = NumberSetting("Задержка (сек)",   "Задержка между отправкой whois в секундах", 1.f, 0.1f, 10.f, 0.1f);
    BoolSetting   mRescan       = BoolSetting  ("Rescan",           "Повторно делать whois и уведомлять об изменении IP/CID", false);
    BoolSetting   mNotifyNewPlayer = BoolSetting("Уведомлять",      "Показывать сообщение при сборе данных нового игрока", true);
    // RAW-лог: записывает ВСЕ входящие TextPacket в файл WhoisDebug.txt
    BoolSetting   mDebugLog     = BoolSetting  ("RAW лог",         "Сохранять все пакеты сервера в WhoisDebug.txt", false);

    WhoisCollector() : ModuleBase("WhoisCollector",
        "Автоматически собирает данные whois игроков выбранных рангов и сохраняет в файл",
        ModuleCategory::Misc, 0, false)
    {
        addSettings(
            &mSelectAll,
            &mRankDev, &mRankCreator, &mRankFounder, &mRankGod,
            &mRankAntiGrief, &mRankSovereign, &mRankOperator, &mRankAdmin,
            &mRankModerator, &mRankCreative, &mRankPremium, &mRankFly,
            &mRankTempFly, &mRankPlayer,
            &mDelay, &mRescan, &mNotifyNewPlayer, &mDebugLog
        );

        // Когда "Все ранги" выключен — показываем отдельные ранги
        VISIBILITY_CONDITION(mRankDev,       !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankCreator,   !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankFounder,   !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankGod,       !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankAntiGrief, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankSovereign, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankOperator,  !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankAdmin,     !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankModerator, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankCreative,  !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankPremium,   !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankFly,       !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankTempFly,   !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankPlayer,    !mSelectAll.mValue);

        mNames = {
            {Lowercase,       "whoiscollector"},
            {LowercaseSpaced, "whois collector"},
            {Normal,          "WhoisCollector"},
            {NormalSpaced,    "Whois Collector"}
        };
    }

    // ── Внутреннее состояние ─────────────────────────────────────────────
    std::deque<std::string> mWhoisQueue;
    uint64_t mLastSentTime = 0;
    std::unordered_set<std::string> mAlreadyQueried;
    std::unordered_map<std::string, WhoisEntry> mCollected;

    // Кеш для связывания ников и рангов из NameTag
    std::unordered_map<std::string, std::string> mRankCache;

    // Путь к файлам
    std::string mDataFile;       // основной файл
    std::string mMatchFile;      // файл совпадений IP/CID
    std::string mDebugFile;      // файл RAW-лога

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketInEvent(class PacketInEvent& event);

    // Проверяет подходит ли ранг под выбранные настройки
    bool isRankEnabled(const std::string& rankName) const;

    // Сохраняет запись в основной файл
    void saveEntry(const WhoisEntry& entry);

    // Проверяет совпадение по IP или CID и при необходимости дописывает в файл совпадений
    void checkAndSaveMatches(const WhoisEntry& newEntry);

    // Получает список пар {ник, ранг} онлайн-игроков
    struct PlayerInfo { std::string nick; std::string rank; };
    std::vector<PlayerInfo> getOnlinePlayers();

    // (Устаревший вариант — оставлен для совместимости)
    std::vector<std::string> getOnlinePlayerNames();

    std::string getSettingDisplay() override {
        return "Collected: " + std::to_string(mCollected.size());
    }
};