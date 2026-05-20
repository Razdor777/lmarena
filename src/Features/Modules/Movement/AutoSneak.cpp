#include "AutoSneak.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

// Экраны при которых клиентский sneak не нужно форсить
// (игра всё равно сбросит его после нашего события)
static bool isInUIScreen() {
    auto screenName = ClientInstance::get()->getScreenName();
    return screenName != "hud_screen";
}

void AutoSneak::onEnable()
{
    // ABSOLUTE_LAST — чтобы установить sneak ПОСЛЕ того как GUI/инвентарь его сбросит
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoSneak::onBaseTickEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &AutoSneak::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    // RenderEvent вызывается каждый кадр, даже когда открыт чат/инвентарь
    gFeatureManager->mDispatcher->listen<RenderEvent, &AutoSneak::onRenderEvent>(this);
}

void AutoSneak::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoSneak::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &AutoSneak::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AutoSneak::onRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto moveInput = player->getMoveInputComponent();
    if (moveInput) moveInput->mIsSneakDown = false;

    auto rawInput = player->getRawMoveInputComponent();
    if (rawInput) rawInput->mIsSneakDown = false;
}

void AutoSneak::forceSneak()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto moveInput = player->getMoveInputComponent();
    if (moveInput) moveInput->mIsSneakDown = true;

    auto rawInput = player->getRawMoveInputComponent();
    if (rawInput) rawInput->mIsSneakDown = true;
}

void AutoSneak::onBaseTickEvent(BaseTickEvent& event)
{
    // Клиентский sneak только в hud_screen.
    // В чате/инвентаре игра принудительно сбрасывает mIsSneakDown после наших событий,
    // поэтому просто не трогаем — серверная сторона (пакет) всё равно работает.
    if (!isInUIScreen())
        forceSneak();
}

void AutoSneak::onRenderEvent(RenderEvent& event)
{
    if (!isInUIScreen())
        forceSneak();
}

void AutoSneak::onPacketOutEvent(PacketOutEvent& event)
{
    // Серверная сторона работает ВСЕГДА (в том числе в чате и инвентаре),
    // потому что пакеты PlayerAuthInput отправляются независимо от UI-экрана.
    if (!mServerSide.mValue) return;

    if (event.mPacket->getId() == PacketID::PlayerAuthInput)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();
        if (!paip) return;

        paip->mInputData |= AuthInputAction::SNEAK_DOWN
                          | AuthInputAction::SNEAKING
                          | AuthInputAction::START_SNEAKING;

        // Убираем флаг остановки приседа — на случай если игра его выставила
        paip->mInputData &= ~AuthInputAction::STOP_SNEAKING;
    }
}