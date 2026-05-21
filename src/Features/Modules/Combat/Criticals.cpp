//
// Criticals.cpp
//
// Три метода в порядке надёжности:
//
// 1. ActorFlags::Critical (0xD) — прямой флаг крита в ECS.
//    Ставим его каждый тик. Клиент сам включает его в пакеты.
//    Самый чистый способ — движок сам обрабатывает.
//
// 2. FallDistance + velocity.y + OnGround — компонентный метод.
//    Меняем клиентские компоненты ДО того как движок собирает
//    PlayerAuthInputPacket. Сервер получает легитимный пакет.
//
// 3. PlayerAuthInputPacket патч — дополнительный фикс полей пакета
//    на случай если движок не подхватил изменения компонентов.
//

#include "Criticals.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Actor/Components/FallDistanceComponent.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>

void Criticals::onEnable() {
    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &Criticals::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Criticals::onPacketOutEvent,
        nes::event_priority::ABSOLUTE_LAST>(this);
    mTickCounter    = 0;
    mSavedVelocityY = 0.f;
    mWasOnGround    = true;
}

void Criticals::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,  &Criticals::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Criticals::onPacketOutEvent>(this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // Восстанавливаем всё
    auto* sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity.y = mSavedVelocityY;
    player->setFallDistance(0.f);
    if (mWasOnGround) player->setOnGround(true);

    // Снимаем флаг Critical
    player->setStatusFlag(ActorFlags::Critical, false);
}

void Criticals::onBaseTickEvent(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    // Не трогаем в воде / плавании / элитры
    if (player->getStatusFlag(ActorFlags::Swimming)) return;
    if (player->getStatusFlag(ActorFlags::Gliding))  return;
    if (player->getStatusFlag(ActorFlags::Riding))   return;

    auto* sv = player->getStateVectorComponent();
    if (!sv) return;

    mSavedVelocityY = sv->mVelocity.y;
    mWasOnGround    = player->isOnGround();
    mTickCounter++;

    // ── Метод 1: Прямой флаг Critical ─────────────────────────────────────
    // ActorFlags::Critical = 0xD — движок сам читает этот флаг при атаке
    player->setStatusFlag(ActorFlags::Critical, true);

    // ── Метод 2: Компонентный — FallDistance + velocity + OnGround ─────────
    // Чередуем тики: вверх → вниз → вверх → вниз
    // Сервер получает через PlayerAuthInput: posDelta.y меняет знак каждый тик
    if (mTickCounter % 2 == 0) {
        // "Вверх" тик
        sv->mVelocity.y = mYOffset.mValue;
        player->setFallDistance(0.f);
        player->setOnGround(false);
    } else {
        // "Вниз" тик — крит условие
        sv->mVelocity.y = -mYOffset.mValue;
        player->setFallDistance(0.11f); // > 0 → сервер видит падение
        player->setOnGround(false);
    }
}

void Criticals::onPacketOutEvent(PacketOutEvent& event) {
    if (!event.mPacket) return;
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // ── Метод 3: Патчим PlayerAuthInputPacket ─────────────────────────────
    if (event.mPacket->getId() == PacketID::PlayerAuthInput) {
        auto* pkt = event.getPacket<PlayerAuthInputPacket>();
        if (!pkt) return;

        // Убираем vertical collision — иначе сервер думает что мы на земле
        pkt->mInputData &= ~AuthInputAction::VERTICAL_COLLISION;

        if (mTickCounter % 2 == 0) {
            pkt->mPos.y      += mYOffset.mValue;
            pkt->mPosDelta.y  = mYOffset.mValue;
        } else {
            pkt->mPosDelta.y  = -mYOffset.mValue;
        }
    }
}
