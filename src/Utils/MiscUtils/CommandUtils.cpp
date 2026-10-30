//
// Created by vastrakai on 7/5/2024.
//

#include "CommandUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/CommandRequestPacket.hpp>

#include "spdlog/spdlog.h"

void CommandUtils::executeCommand(const std::string& command)
{
    // Сюда приходят чат-бинды и AutoContract — в том числе в момент, когда мир
    // ещё грузится или уже выгружается. Раньше здесь не проверялись ни
    // ClientInstance, ни getPacketSender(): вызов в такой момент — нулевой
    // указатель и краш без сообщения.
    auto ci = ClientInstance::get();
    if (!ci) return;

    auto player = ci->getLocalPlayer();
    if (!player) return;

    auto sender = ci->getPacketSender();
    if (!sender) return;

    auto crq = MinecraftPackets::createPacket<CommandRequestPacket>();
    if (!crq) return;

    crq->mCommand = command;
    crq->mOrigin.mPlayerId = player->getRuntimeID();
    sender->send(crq.get());
    spdlog::info("Sent command: {}", command);
}
