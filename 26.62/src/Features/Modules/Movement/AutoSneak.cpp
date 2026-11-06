#include "AutoSneak.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
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
    if (!player || !player->isValid()) return;

    // PERSIST_SNEAK is per-packet, so it dies with the module by itself — we only
    // have to drop the local fake. The client's own state machine sends the next
    // real sneak/un-sneak transition on its own, so there is nothing to resync.
    if (auto moveInput = player->getMoveInputComponent())
        moveInput->mIsSneakDown = false;

    if (auto rawInput = player->getRawMoveInputComponent())
        rawInput->mIsSneakDown = false;

    try { player->setStatusFlag(ActorFlags::Sneaking, false); } catch (...) {}
}

void AutoSneak::forceSneak()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->isValid()) return;

    // Client side: the input flag is what actually drives the sneak state, so this
    // both keeps us low and makes the game itself keep reporting sneaking.
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
    // Runs while screens are open too, so the sneak survives chat / inventory.
    forceSneak();
}

void AutoSneak::onPacketOutEvent(PacketOutEvent& event)
{
    if (!mServerSide.mValue) return;
    if (!event.mPacket) return;

    const PacketID id = event.mPacket->getId();

    // ─── PlayerAuthInput: how the server learns about sneak in 1.19.60+ ───────
    if (id == PacketID::PlayerAuthInput)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();
        if (!paip) return;

        // SNEAK_DOWN / SNEAKING = "it is held", START_SNEAKING = the transition,
        // and PERSIST_SNEAK is the one that matters here: it tells the server to
        // KEEP sneaking even when the input goes empty — which is exactly what the
        // client does when a chat/inventory screen opens (it clears the movement
        // input, which used to stand the player up for everyone else).
        paip->mInputData |= AuthInputAction::SNEAK_DOWN
                         | AuthInputAction::SNEAKING
                         | AuthInputAction::START_SNEAKING
                         | AuthInputAction::PERSIST_SNEAK;

        // Never let a stop/toggle bit through — those are what un-sneak us.
        paip->mInputData &= ~(AuthInputAction::STOP_SNEAKING | AuthInputAction::SNEAK_TOGGLE_DOWN);
        return;
    }

    // ─── PlayerAction: the legacy sneak transition ────────────────────────────
    // Some builds still announce the state change with an action packet instead of
    // (or in addition to) the auth input. Standing up must never reach the server.
    if (id == PacketID::PlayerAction)
    {
        auto action = event.getPacket<PlayerActionPacket>();
        if (!action) return;

        if (action->mAction == PlayerActionType::StopSneaking)
        {
            event.mCancelled = true;
            spdlog::debug("[AutoSneak] blocked StopSneaking at ({}, {}, {})",
                action->mPos.x, action->mPos.y, action->mPos.z);
        }
    }
}
