//
// Created by vastrakai on 6/25/2024.
//

#include "BaseTickHook.hpp"

#include <unordered_map>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/ActorOwnerComponent.hpp>
#include <SDK/Minecraft/Actor/Components/RuntimeIDComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MobHurtTimeComponent.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/EntityHurtEvent.hpp>
#include <Features/ChatBinds/ChatBinds.hpp>
#include <Features/Modules/ModuleToggle.hpp>
#include <SDK/OffsetProvider.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

std::unique_ptr<Detour> BaseTickHook::mDetour;

void BaseTickHook::onBaseTick(Actor* actor)
{
    auto oFunc = mDetour->getOriginal<&onBaseTick>();
    if (actor != ClientInstance::get()->getLocalPlayer()) return oFunc(actor);

    ExceptionHandler::markThread(ExceptionHandler::Game);

    ModuleToggle::fromGameThread();

    mQueueMutex.lock();
    auto messages = mQueuedMessages;
    if (!messages.empty())
    {
        if (ChatUtils::sSink)
        {
            for (auto& message : messages) ChatUtils::sSink(message);
        }
        else if (auto* guiData = ClientInstance::get()->getGuiData())
        {
            std::string messageStr = "";
            for (auto& message : messages) messageStr += message + "\n";
            guiData->displayClientMessage(messageStr);
        }

        mQueuedMessages.clear();
    }
    mQueueMutex.unlock();

    if (auto supplies = actor->getSupplies())
    {
        supplies->mInHandSlot = supplies->mSelectedSlot;
    }

    std::vector<QueuedPacket> packets;
    {
        std::lock_guard<std::mutex> lock(mPacketQueueMutex);
        packets.swap(mQueuedPackets);
    }
    if (auto* sender = ClientInstance::get()->getPacketSender())
    {
        for (auto& [mTime, mPacket, mBypassHook] : packets)
        {
            spdlog::trace("Sending packet with ID: {} [queued {}ms ago] [{}]", magic_enum::enum_name(mPacket->getId()), NOW - mTime, mBypassHook ? "bypassing hook" : "not bypassing hook");
            if (mBypassHook) sender->sendToServer(mPacket.get());
            else sender->send(mPacket.get());
        }
    }

    ChatBinds::flushPending();

    static bool once = false;
    if (!once)
    {
        once = true;
        auto holder = nes::make_holder<BaseTickInitEvent>(actor);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    // Native 26.62 EntityHurtEvent dispatch via ECS MobHurtTimeComponent transitions
    if (actor->mContext.mRegistry && !SigManager::Health_entityHurt)
    {
        struct HurtState { int hurtTime = 0; float hp = 20.f; };
        static std::unordered_map<int64_t, HurtState> sHurtStates;

        for (auto&& [entId, owner, rid, hurt] : actor->mContext.mRegistry->view<ActorOwnerComponent, RuntimeIDComponent, MobHurtTimeComponent>().each())
        {
            Actor* ent = owner.mActor;
            if (!ent) continue;

            auto& prev = sHurtStates[rid.mRuntimeID];
            float curHp = ent->getHealth() + ent->getAbsorption();
            if (hurt.mHurtTime > prev.hurtTime && hurt.mHurtTime >= 8)
            {
                int dmg = static_cast<int>(std::max(1.0f, std::round(prev.hp - curHp)));
                auto hurtHolder = nes::make_holder<EntityHurtEvent>(ent, nullptr, dmg, true, false);
                gFeatureManager->mDispatcher->trigger(hurtHolder);
            }
            prev.hurtTime = hurt.mHurtTime;
            prev.hp = curHp;
        }
        if (sHurtStates.size() > 512) sHurtStates.clear();
    }

    auto holder = nes::make_holder<BaseTickEvent>(actor);
    gFeatureManager->mDispatcher->trigger(holder);

    return oFunc(actor);
}

void BaseTickHook::init()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    uintptr_t** vtable = player->vtable;
    mDetour = std::make_unique<Detour>("Actor::baseTick", vtable[OffsetProvider::Actor_baseTick], &BaseTickHook::onBaseTick);
    mDetour->enable();
}
