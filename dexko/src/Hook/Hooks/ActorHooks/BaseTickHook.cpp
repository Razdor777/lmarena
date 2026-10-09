//
// Created by vastrakai on 6/25/2024.
//

#include "BaseTickHook.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/OffsetProvider.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

std::unique_ptr<Detour> BaseTickHook::mDetour = nullptr;

void BaseTickHook::onBaseTick(Actor* actor)
{
    auto oFunc = mDetour->getOriginal<&onBaseTick>();
    if (actor != ClientInstance::get()->getLocalPlayer()) return oFunc(actor);

    std::vector<std::string> messages;
    {
        std::lock_guard lock(mQueueMutex);
        messages.swap(mQueuedMessages);
    }

    if (!messages.empty())
    {
        std::string messageStr;
        for (auto& message : messages) messageStr += message + "\n";
        ClientInstance::get()->getGuiData()->displayClientMessage(messageStr);
    }

    if (auto supplies = actor->getSupplies())
    {
        supplies->mInHandSlot = supplies->mSelectedSlot;
    }

    static bool once = false;
    if (!once)
    {
        once = true;

        auto holder = nes::make_holder<BaseTickInitEvent>(actor);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    auto holder = nes::make_holder<BaseTickEvent>(actor);
    gFeatureManager->mDispatcher->trigger(holder);

    return oFunc(actor);
}

void BaseTickHook::init()
{
    mDetour = std::make_unique<Detour>("Actor::baseTick", reinterpret_cast<void*>(ClientInstance::get()->getLocalPlayer()->vtable[OffsetProvider::Actor_baseTick]), reinterpret_cast<void*>(&BaseTickHook::onBaseTick));
    mDetour->enable();
}
