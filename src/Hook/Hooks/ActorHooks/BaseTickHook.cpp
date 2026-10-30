//
// Created by vastrakai on 6/25/2024.
//

#include "BaseTickHook.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/ChatBinds/ChatBinds.hpp>
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

    ExceptionHandler::markThread(ExceptionHandler::Game);

    // onEnable/onDisable модулей — здесь, в потоке игры (см. ModuleToggle.hpp).
    ModuleToggle::fromGameThread();

    mQueueMutex.lock();
    auto messages = mQueuedMessages;
    if (!messages.empty())
    {
        // Если включён CustomChat — клиентские сообщения идут в него (он сам
        // решает, что и как рисовать). Иначе — как раньше, в ванильный чат.
        if (ChatUtils::sSink)
        {
            for (auto& message : messages) ChatUtils::sSink(message);
        }
        else
        {
            std::string messageStr = "";
            for (auto& message : messages) messageStr += message + "\n";
            ClientInstance::get()->getGuiData()->displayClientMessage(messageStr);
        }

        mQueuedMessages.clear();
    }
    mQueueMutex.unlock();

    if (auto supplies = actor->getSupplies())
    {
        supplies->mInHandSlot = supplies->mSelectedSlot;
    }

    // Очередь забираем целиком и отправляем уже свою копию: send() без обхода
    // хука вызывает PacketOutEvent, и модуль оттуда может поставить в очередь
    // ещё пакет. Раньше это был emplace_back в вектор, по которому прямо сейчас
    // шёл цикл (перевыделение памяти под ногами -> краш); теперь такой пакет
    // просто уйдёт в следующем тике.
    std::vector<QueuedPacket> packets;
    {
        std::lock_guard<std::mutex> lock(mPacketQueueMutex);
        packets.swap(mQueuedPackets);
    }
    for (auto& [mTime, mPacket, mBypassHook] : packets)
    {
        spdlog::trace("Sending packet with ID: {} [queued {}ms ago] [{}]", magic_enum::enum_name(mPacket->getId()), NOW - mTime, mBypassHook ? "bypassing hook" : "not bypassing hook");
        if (mBypassHook) ClientInstance::get()->getPacketSender()->sendToServer(mPacket.get());
        else ClientInstance::get()->getPacketSender()->send(mPacket.get());
    }

    // Чат-бинды: клавишу ловит KeyHook, а сама отправка — здесь, в потоке
    // игры, как и обычный чат.
    ChatBinds::flushPending();

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
    mDetour = std::make_unique<Detour>("Actor::baseTick", reinterpret_cast<void*>(ClientInstance::get()->getLocalPlayer()->vtable[OffsetProvider::Actor_baseTick]), &BaseTickHook::onBaseTick);
    mDetour->enable();
}
