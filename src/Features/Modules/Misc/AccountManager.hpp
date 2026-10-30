#pragma once

//
// AccountManager — переключение ника прямо из ClickGui.
//
// Сервер оффлайн (109.248.4.159:19133), Xbox-авторизации нет, поэтому ник
// задаётся в ConnectionRequest — ровно там же, где SkinSpoofer подменяет скин.
// Клиент кладёт в пакет входа thirdPartyName и platformOfflineID; для оффлайн
// сервера имя игрока берётся из thirdPartyName.
//
// Смена ника видна только на новом подключении: пакет входа уже отправлен,
// переименовать себя «на лету» нельзя. Если включён Reconnect и QuickConnect
// уже знает последний адрес — модуль сам переподключится, чтобы получилось
// «нажал профиль → ник поменялся».
//
class AccountManager : public ModuleBase<AccountManager>
{
public:
    enum class Profile { One, Two, Three, Four };

    EnumSettingT<Profile> mProfile = EnumSettingT<Profile>(
        "Profile", "Активный ник", Profile::One, "1", "2", "3", "4");

    StringSetting mNick1 = StringSetting("Nick 1", "Ник первого профиля", "");
    StringSetting mNick2 = StringSetting("Nick 2", "Ник второго профиля", "");
    StringSetting mNick3 = StringSetting("Nick 3", "Ник третьего профиля", "");
    StringSetting mNick4 = StringSetting("Nick 4", "Ник четвёртого профиля", "");

    BoolSetting mReconnect = BoolSetting(
        "Reconnect", "Сразу переподключаться на последний сервер, чтобы ник применился", true);

    AccountManager() : ModuleBase("AccountManager", "Switch nickname profiles", ModuleCategory::Misc, 0, true)
    {
        addSettings(&mProfile, &mNick1, &mNick2, &mNick3, &mNick4, &mReconnect);

        VISIBILITY_CONDITION(mNick1, mProfile.mValue == Profile::One);
        VISIBILITY_CONDITION(mNick2, mProfile.mValue == Profile::Two);
        VISIBILITY_CONDITION(mNick3, mProfile.mValue == Profile::Three);
        VISIBILITY_CONDITION(mNick4, mProfile.mValue == Profile::Four);

        mNames = {
            {Lowercase,       "accountmanager"},
            {LowercaseSpaced, "account manager"},
            {Normal,          "AccountManager"},
            {NormalSpaced,    "Account Manager"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);

    std::string getSettingDisplay() override { return activeNick(); }

    // Ник активного профиля. Пустая строка — профиль не заполнен, ничего не
    // подменяем (иначе можно случайно зайти с пустым ником).
    std::string activeNick() const;

private:
    std::string  mAppliedNick;
    int64_t      mLastApplyAttempt = 0;
};
