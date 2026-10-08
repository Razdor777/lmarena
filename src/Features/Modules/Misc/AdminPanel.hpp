#pragma once

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Modules/Module.hpp>

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Порядок рангов MineOre: от самого низкого к самому высокому.
// Числовое значение используется и для сортировки, и для проверки иммунитета.
enum class AdminPanelRank : int {
    Unknown = -1,
    Player = 0,
    TempFly,
    Fly,
    Premium,
    Creative,
    Moderator,
    Admin,
    Operator,
    Sovereign,
    AntiGrief,
    God,
    Founder,
    Creator,
    Developer,
};

enum class AdminPanelAction : int {
    Ban = 0,
    BanIp,
    BanOff,
    Mute,
    Kick,
};

struct AdminPanelTarget {
    std::string nick;
    AdminPanelRank rank = AdminPanelRank::Unknown;
};

class AdminPanel : public ModuleBase<AdminPanel> {
public:
    // За одно включение выполняется один выбранный тип наказания.
    EnumSettingT<AdminPanelAction> mAction = EnumSettingT<AdminPanelAction>(
        "Команда", "Команда, которая будет массово применена при включении",
        AdminPanelAction::Ban, "ban", "banip", "banoff", "mute", "kick");

    // Для mute серверу всегда отправляется 20, для banip/banoff — 0,
    // у kick времени нет. Обычный ban можно настроить отдельно.
    NumberSetting mBanTime = NumberSetting(
        "Время ban (мин)", "0 означает навсегда", 0.f, 0.f, 1000000.f, 1.f);
    NumberSetting mDelay = NumberSetting(
        "Задержка (сек)", "Минимальная задержка между командами", 1.f, 0.2f, 10.f, 0.1f);

    // Ранги расположены в меню от самого высокого к самому низкому.
    BoolSetting mSelectAll = BoolSetting(
        "Все ранги", "Выбрать все ранги (иммунитет команды всё равно учитывается)", false);
    BoolSetting mRankDeveloper = BoolSetting("Разработчик", "Выбрать Разработчиков", false);
    BoolSetting mRankCreator = BoolSetting("Создатель", "Выбрать Создателей", false);
    BoolSetting mRankFounder = BoolSetting("Основатель", "Выбрать Основателей", false);
    BoolSetting mRankGod = BoolSetting("Бог", "Выбрать ранг Бог", false);
    BoolSetting mRankAntiGrief = BoolSetting("Анти-Гриф", "Выбрать Анти-Гриф", false);
    BoolSetting mRankSovereign = BoolSetting("Повелитель", "Выбрать Повелителей", false);
    BoolSetting mRankOperator = BoolSetting("Оператор", "Выбрать Операторов", false);
    BoolSetting mRankAdmin = BoolSetting("Админ", "Выбрать Админов", false);
    BoolSetting mRankModerator = BoolSetting("Модератор", "Выбрать Модераторов", false);
    BoolSetting mRankCreative = BoolSetting("Креатив", "Выбрать Креатив", false);
    BoolSetting mRankPremium = BoolSetting("Премиум", "Выбрать Премиум", false);
    BoolSetting mRankFly = BoolSetting("Флай", "Выбрать Флай", false);
    BoolSetting mRankTempFly = BoolSetting("Временный флай", "Выбрать Временный флай", false);
    BoolSetting mRankPlayer = BoolSetting("Player", "Выбрать обычных игроков", false);

    AdminPanel()
        : ModuleBase("AdminPanel",
                     "Массово применяет выбранную админ-команду к игрокам выбранных рангов",
                     ModuleCategory::Misc, 0, false) {
        addSettings(
            &mAction, &mBanTime, &mDelay, &mSelectAll,
            &mRankDeveloper, &mRankCreator, &mRankFounder, &mRankGod,
            &mRankAntiGrief, &mRankSovereign, &mRankOperator, &mRankAdmin,
            &mRankModerator, &mRankCreative, &mRankPremium, &mRankFly,
            &mRankTempFly, &mRankPlayer);

        VISIBILITY_CONDITION(mBanTime, mAction.mValue == AdminPanelAction::Ban);
        VISIBILITY_CONDITION(mRankDeveloper, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankCreator, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankFounder, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankGod, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankAntiGrief, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankSovereign, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankOperator, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankAdmin, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankModerator, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankCreative, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankPremium, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankFly, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankTempFly, !mSelectAll.mValue);
        VISIBILITY_CONDITION(mRankPlayer, !mSelectAll.mValue);

        mNames = {
            {Lowercase, "adminpanel"},
            {LowercaseSpaced, "admin panel"},
            {Normal, "AdminPanel"},
            {NormalSpaced, "Admin Panel"},
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);

    std::string getSettingDisplay() override;

private:
    static constexpr const char* REASON = "67";

    std::deque<AdminPanelTarget> mQueue;
    std::unordered_set<std::string> mQueued;
    std::unordered_set<std::string> mProcessed;
    std::unordered_set<std::string> mWhitelist;
    std::unordered_map<std::string, AdminPanelRank> mRankCache;

    std::string mWhitelistFile;
    AdminPanelAction mActiveAction = AdminPanelAction::Ban;
    int mActiveBanTime = 0;

    uint64_t mLastScanTime = 0;
    uint64_t mLastSentTime = 0;
    uint64_t mCooldownUntil = 0;
    bool mAwaitingResponse = false;
    AdminPanelTarget mLastTarget;

    size_t mSentCount = 0;
    size_t mWhitelistSkipCount = 0;
    size_t mImmunitySkipCount = 0;

    void scanPlayers();
    std::vector<AdminPanelTarget> getOnlinePlayers();
    void enqueueTarget(const AdminPanelTarget& target);
    void sortQueue();

    bool isRankSelected(AdminPanelRank rank) const;
    bool isAllowedByCommand(AdminPanelRank rank) const;
    std::string buildCommand(const std::string& nick) const;

    void ensureWhitelistFile();
    void loadWhitelist();
    bool isWhitelisted(const std::string& nick) const;

    static AdminPanelRank parseRank(const std::string& rankName);
    static std::string rankName(AdminPanelRank rank);
    static std::string actionName(AdminPanelAction action);
};