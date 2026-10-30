#pragma once

//
// Хук ClientNetworkSystem::connect.
//
// Единственная задача — отдать QuickConnect указатель на сеть клиента и
// рабочий образец GameConnectionInfo. Поведение игры не меняется: вызов
// уходит в оригинал ровно с теми же аргументами.
//
// Адрес функции: SigManager::ClientNetworkSystem_connect
//   bool __fastcall(void* this, const GameConnectionInfo&, const GameConnectionInfo&)
//   sub_141B7FEF0
//
// Если сигнатура не нашлась (другая сборка), хук просто не ставится —
// QuickConnect тогда честно скажет, что функция подключения не найдена.
//

#include <Hook/Hook.hpp>

class NetworkConnectHook : public Hook
{
public:
    NetworkConnectHook() : Hook()
    {
        mName = "ClientNetworkSystem::connect";
    }

    static std::unique_ptr<Detour> mDetour;
    static inline bool (*mOriginal)(void*, const void*, const void*) = nullptr;

    static bool connectDetourFunc(void* networkSystem, const void* gameConnectionInfo, const void* secondInfo);
    void init() override;
};
