#pragma once

//
// QuickConnect — подключение к серверу по ip:port в один клик.
//
// Что делает: берёт адрес из настройки и сразу поднимает обычное сетевое
// подключение клиента — то же, что происходит при нажатии «Играть» в списке
// серверов, только без похода в меню. Ничего не подменяет и не патчит пакеты.
//
// Откуда данные (IDA, 1.21.x):
//   ClientNetworkSystem::connect(const GameConnectionInfo&, const GameConnectionInfo&)
//     bool __fastcall(void* this, const void* info, const void* secondInfo)
//     sub_141B7FEF0  — сигнатура лежит в SigManager::ClientNetworkSystem_connect
//   GameConnectionInfo: 0x1A0 байт, std::string адреса на +0x08,
//     uint32 порт на +0x48 (видно в sub_1401DB030, где собираются "IP"/"Port").
//
// Почему нужен «прогрев»: указатель на ClientNetworkSystem клиент нигде не
// хранит в открытом виде — он достаётся виртуальным вызовом MinecraftGame.
// Поэтому мы запоминаем его, когда игра подключается сама (хук на connect),
// и дальше переиспользуем. За один запуск достаточно один раз зайти на любой
// сервер обычным способом.
//
class QuickConnect : public ModuleBase<QuickConnect>
{
public:
    StringSetting mServer = StringSetting(
        "Server", "ip:port, например 109.248.4.159:19133", "109.248.4.159:19133");

    QuickConnect() : ModuleBase("QuickConnect", "Connect to a server by ip:port", ModuleCategory::Misc, 0, false)
    {
        addSettings(&mServer);

        mNames = {
            {Lowercase,       "quickconnect"},
            {LowercaseSpaced, "quick connect"},
            {Normal,          "QuickConnect"},
            {NormalSpaced,    "Quick Connect"}
        };
    }

    void onEnable() override;

    std::string getSettingDisplay() override { return mServer.mValue; }

    // Хук ClientNetworkSystem::connect: запоминает сеть и рабочее описание
    // сервера, чтобы потом собрать своё по тому же образцу.
    static void onNetworkConnect(void* networkSystem, const void* gameConnectionInfo);

    // Оригинал connect, который отдаёт детур (нужен, чтобы не ловить свой же
    // вызов повторно и не затирать шаблон синтетическими данными).
    static void setConnectFunction(bool (*fn)(void*, const void*, const void*));

    // Основное действие. Возвращает false, если подключиться не удалось —
    // причина всегда пишется в чат.
    static bool join(const std::string& address);

    static bool isReady();

    // «host:port» последнего штатного подключения — по нему AccountManager
    // переподключается после смены ника. Пустая строка, если ещё ничего не было.
    static std::string lastAddress();
};
