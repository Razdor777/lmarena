//
// См. NetworkConnectHook.hpp.
//

#include "NetworkConnectHook.hpp"

#include <SDK/SigManager.hpp>
#include <Features/Modules/Misc/QuickConnect.hpp>
#include <Utils/Logger.hpp>

std::unique_ptr<Detour> NetworkConnectHook::mDetour;

bool NetworkConnectHook::connectDetourFunc(void* networkSystem, const void* gameConnectionInfo,
                                           const void* secondInfo)
{
    try
    {
        QuickConnect::onNetworkConnect(networkSystem, gameConnectionInfo);
    }
    catch (...)
    {
        // Захват не должен мешать подключению ни при каких обстоятельствах.
    }

    return mOriginal(networkSystem, gameConnectionInfo, secondInfo);
}

void NetworkConnectHook::init()
{
    if (!SigManager::ClientNetworkSystem_connect)
    {
        spdlog::warn("[QuickConnect] ClientNetworkSystem::connect not found — hook skipped");
        return;
    }

    mDetour = std::make_unique<Detour>(
        "ClientNetworkSystem::connect",
        reinterpret_cast<void*>(SigManager::ClientNetworkSystem_connect),
        reinterpret_cast<void*>(&connectDetourFunc));

    mOriginal = mDetour->getOriginal<&connectDetourFunc>();
    QuickConnect::setConnectFunction(mOriginal);
}
