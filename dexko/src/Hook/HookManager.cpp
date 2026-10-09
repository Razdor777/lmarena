//
// HookManager — форк движкового файла.
// Dexko вешает только те хуки, которые нужны его модулям:
//   KeyHook              — END (eject), кейбинды модулей, KeyEvent
//   D3DHook              — ImGui + RenderEvent (CustomCrosshair, HitParticles, Interface)
//   BaseTickHook         — BaseTickEvent (Reach, AntiBot, Interface)
//   PacketSendHook       — ChatEvent (команды) + PacketOutEvent (HitParticles)
//   ConnectionRequestHook— ConnectionRequestEvent (DeviceSpoof)
//
// ActorRenderDispatcherHook скомпилирован (на него ссылается Interface.cpp),
// но НЕ регистрируется — событие просто не срабатывает.
//

#include "HookManager.hpp"

#include <MinHook.h>

#include "Hooks/ActorHooks/BaseTickHook.hpp"
#include "Hooks/MiscHooks/KeyHook.hpp"
#include "Hooks/NetworkHooks/ConnectionRequestHook.hpp"
#include "Hooks/NetworkHooks/PacketSendHook.hpp"
#include "Hooks/RenderHooks/D3DHook.hpp"
#include "Hooks/RenderHooks/ActorRenderDispatcherHook.hpp"


#define ADD_HOOK(hook) hooks.emplace_back(std::make_shared<hook>())

void HookManager::init(bool initLp)
{
    if (initLp)
    {
        std::vector<std::shared_ptr<Hook>> hooks;
        ADD_HOOK(BaseTickHook);
        for (auto& hook : hooks)
        {
            hook->init();
            mHooks.emplace_back(hook);
        }
    }
    else
    {
        std::vector<std::shared_ptr<Hook>> hooks;
        ADD_HOOK(KeyHook);
        ADD_HOOK(D3DHook);
        ADD_HOOK(ConnectionRequestHook);
        ADD_HOOK(PacketSendHook);

        for (auto& hook : hooks)
        {
            hook->init();
            mHooks.emplace_back(hook);
        }

        MH_EnableHook(MH_ALL_HOOKS);
    }
}

void HookManager::shutdown()
{
    for (auto& hook : mHooks)
    {
        hook->shutdown();
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    mHooks.clear();
}
