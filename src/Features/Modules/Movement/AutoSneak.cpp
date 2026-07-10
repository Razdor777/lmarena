#include "AutoSneak.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

void AutoSneak::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoSneak::onBaseTickEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &AutoSneak::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &AutoSneak::onRenderEvent>(this);
}

void AutoSneak::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoSneak::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &AutoSneak::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AutoSneak::onRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (auto moveInput = player->getMoveInputComponent())
        moveInput->mIsSneakDown = false;

    if (auto rawInput = player->getRawMoveInputComponent())
        rawInput->mIsSneakDown = false;

    try { player->setStatusFlag(ActorFlags::Sneaking, false); } catch (...) {}
}

void AutoSneak::forceSneak()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // Клиентская сторона (визуал у себя)
    if (auto moveInput = player->getMoveInputComponent())
        moveInput->mIsSneakDown = true;

    if (auto rawInput = player->getRawMoveInputComponent())
        rawInput->mIsSneakDown = true;

    try { player->setStatusFlag(ActorFlags::Sneaking, true); } catch (...) {}
}

void AutoSneak::onBaseTickEvent(BaseTickEvent& event)
{
    forceSneak();
}

void AutoSneak::onRenderEvent(RenderEvent& event)
{
    forceSneak();
}

void AutoSneak::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() != PacketID::PlayerAuthInput)
        return;

    auto paip = event.getPacket<PlayerAuthInputPacket>();
    if (!paip) return;

    // Всегда форсим серверный сник (независимо от mServerSide)
    paip->mInputData |= AuthInputAction::SNEAK_DOWN
                     | AuthInputAction::SNEAKING
                     | AuthInputAction::START_SNEAKING;

    paip->mInputData &= ~AuthInputAction::STOP_SNEAKING;
}