//
//

#include "BaseTickHook.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/ChatBinds/ChatBinds.hpp>
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
    mQueuedMessages.clear();
    mQueueMutex.unlock();

    if (!messages.empty())
    {
        // Если включён CustomChat — клиентские сообщения идут в него (он сам
        // решает, что и как рисовать). Иначе — в ванильный чат, как раньше.
        // Без этой ветки вывод команд игнорировал CustomChat: текст уходил
        // мимо него, и в перехваченном чате ничего не появлялось.
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
    }

    // Один раз на мир: RobloxCamera и другие модули проверяют здесь наличие
    // компонентов актора (после загрузки их ещё нет — раньше проверка молча
    // не выполнялась вовсе).
    static bool baseTickInitDone = false;
    if (!baseTickInitDone)
    {
        baseTickInitDone = true;
        auto initHolder = nes::make_holder<BaseTickInitEvent>(actor);
        gFeatureManager->mDispatcher->trigger(initHolder);
    }

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

    // Чат-бинды: клавишу ловит KeyHook, а отправка — здесь, в потоке игры,
    // как и обычный чат.
    ChatBinds::flushPending();


    auto holder = nes::make_holder<BaseTickEvent>(actor);
    gFeatureManager->mDispatcher->trigger(holder);

    return oFunc(actor);
}

void BaseTickHook::init()
{
    mDetour = std::make_unique<Detour>("Actor::baseTick", reinterpret_cast<void*>(ClientInstance::get()->getLocalPlayer()->vtable[OffsetProvider::Actor_baseTick]), &BaseTickHook::onBaseTick);
    mDetour->enable();
}
