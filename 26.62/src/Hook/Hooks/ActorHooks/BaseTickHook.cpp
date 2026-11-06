//
//

#include "BaseTickHook.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Modules/ModuleToggle.hpp>
#include <SDK/OffsetProvider.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

std::unique_ptr<Detour> BaseTickHook::mDetour = nullptr;

void BaseTickHook::onBaseTick(Actor* actor)
{
    auto oFunc = mDetour->getOriginal<&onBaseTick>();
    if (actor != ClientInstance::get()->getLocalPlayer()) return oFunc(actor);

    mQueueMutex.lock();
    auto messages = mQueuedMessages;
    if (!messages.empty())
    {
        std::string messageStr = "";
        for (auto& message : messages) messageStr += message + "\n";
        ClientInstance::get()->getGuiData()->displayClientMessage(messageStr);
        mQueuedMessages.clear();
    }
    mQueueMutex.unlock();

    if (auto supplies = actor->getSupplies())
    {
        supplies->mInHandSlot = supplies->mSelectedSlot;
    }
    if (ClientInstance::get()->getPacketSender()) {
        for (auto& [mTime, mPacket, mBypassHook] : mQueuedPackets)
        {
            spdlog::trace("Sending packet with ID: {} [queued {}ms ago] [{}]", magic_enum::enum_name(mPacket->getId()), NOW - mTime, mBypassHook ? "bypassing hook" : "not bypassing hook");
            if (mBypassHook) ClientInstance::get()->getPacketSender()->sendToServer(mPacket.get());
            else ClientInstance::get()->getPacketSender()->send(mPacket.get());
        }
        mQueuedPackets.clear();
    }


    // onEnable/onDisable модулей — здесь, в потоке игры (см. ModuleToggle.hpp).
    // Раньше это делал фоновый поток Solstice::shutdownThread, из-за чего
    // включение/выключение модулей иногда роняло игру.
    ModuleToggle::fromGameThread();

    auto holder = nes::make_holder<BaseTickEvent>(actor);
    gFeatureManager->mDispatcher->trigger(holder);

    return oFunc(actor);
}

void BaseTickHook::init()
{
    mDetour = std::make_unique<Detour>("Actor::baseTick", reinterpret_cast<void*>(ClientInstance::get()->getLocalPlayer()->vtable[OffsetProvider::Actor_baseTick]), &BaseTickHook::onBaseTick);
    mDetour->enable();
}
