// HookManager.cpp — Dexko (stripped hook set)
//

#include "HookManager.hpp"

#include <MinHook.h>

#include "Hooks/ActorHooks/BaseTickHook.hpp"
#include "Hooks/MiscHooks/KeyHook.hpp"
#include "Hooks/MiscHooks/PreGameHook.hpp"
#include "Hooks/NetworkHooks/ConnectionRequestHook.hpp"
#include "Hooks/NetworkHooks/PacketReceiveHook.hpp"
#include "Hooks/NetworkHooks/PacketSendHook.hpp"
#include "Hooks/RenderHooks/ActorRenderDispatcherHook.hpp"
#include "Hooks/RenderHooks/D3DHook.hpp"
#include "Hooks/RenderHooks/HoverTextRendererHook.hpp"
#include "Hooks/RenderHooks/SetupAndRenderHook.hpp"

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
        ADD_HOOK(SetupAndRenderHook);
        ADD_HOOK(D3DHook);
        ADD_HOOK(ConnectionRequestHook);
        ADD_HOOK(PacketReceiveHook);
        ADD_HOOK(PacketSendHook);
        ADD_HOOK(ActorRenderDispatcherHook);
        ADD_HOOK(HoverTextRendererHook);
        ADD_HOOK(PreGameHook);

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
