//
// Created by vastrakai on 11/9/2024.
// Fixed: now uses ClickTp-style straightLineTP instead of client-only setPosition
//

#include "TeleportCommand.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ClickTp-style packet TP: sends MovePlayerPacket chain with PositionMode::Teleport
static void serverTeleport(glm::vec3 from, glm::vec3 to, float stepDist = 8.0f)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    auto sender = ClientInstance::get()->getPacketSender();
    if (!player || !sender) return;

    auto comp = player->getActorRotationComponent();
    glm::vec2 rot = comp ? glm::vec2(comp->mPitch, comp->mYaw) : glm::vec2(0.f);
    float yHead = comp ? comp->mYaw : 0.f;

    auto makePacket = [&](glm::vec3 pos) {
        auto pk = MinecraftPackets::createPacket<MovePlayerPacket>();
        pk->mPos = pos;
        pk->mPlayerID = player->getRuntimeID();
        pk->mRot = rot;
        pk->mYHeadRot = yHead;
        pk->mResetPosition = PositionMode::Teleport;
        pk->mOnGround = true;
        pk->mRidingID = -1;
        pk->mCause = TeleportationCause::Unknown;
        pk->mSourceEntityType = ActorType::Player;
        pk->mTick = 0;
        return pk;
    };

    float dist = glm::distance(from, to);
    if (dist < 0.01f) {
        sender->sendToServer(makePacket(to).get());
    } else {
        glm::vec3 dir = glm::normalize(to - from);
        glm::vec3 cur = from;
        while (glm::distance(cur, to) > stepDist) {
            cur += dir * stepDist;
            sender->sendToServer(makePacket(cur).get());
        }
        sender->sendToServer(makePacket(to).get());
    }

    // Move client-side too
    player->setPosition(to);
    auto sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
}

void TeleportCommand::execute(const std::vector<std::string>& args)
{
    auto player = ClientInstance::get()->getLocalPlayer();

    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§c" + getUsage());
        return;
    }

    if (args.size() == 2)
    {
        auto targetList = ActorUtils::getActorList(true, true);
        Actor* target = nullptr;
        std::string targetName = StringUtils::toLower(args[1]);

        // First try exact match
        for (auto& actor : targetList)
        {
            if (StringUtils::toLower(actor->getRawName()) == targetName)
            {
                target = actor;
                break;
            }
        }

        // If no exact match, try partial/substring match
        if (!target)
        {
            std::vector<Actor*> matches;
            for (auto& actor : targetList)
            {
                std::string name = StringUtils::toLower(actor->getRawName());
                if (name.find(targetName) != std::string::npos)
                {
                    matches.push_back(actor);
                }
            }

            if (matches.size() == 1)
            {
                target = matches[0];
            }
            else if (matches.size() > 1)
            {
                ChatUtils::displayClientMessage("§eMultiple players found for '§6" + args[1] + "§e':");
                for (auto& m : matches)
                {
                    ChatUtils::displayClientMessage("  §7- §f" + m->getRawName());
                }
                ChatUtils::displayClientMessage("§eBe more specific!");
                return;
            }
        }

        if (target == nullptr)
        {
            ChatUtils::displayClientMessage("§cCould not find player: " + args[1] + "!");
            return;
        }

        glm::vec3 from = *player->getPos();
        glm::vec3 to = *target->getPos();
        serverTeleport(from, to);
        ChatUtils::displayClientMessage("§aTeleported to " + target->getRawName() + "!");
        return;
    }
    else if (args.size() != 4)
    {
        ChatUtils::displayClientMessage("§c" + getUsage());
        return;
    }

    try
    {
        auto comp = player->getActorRotationComponent();
        glm::vec2 rotation = { comp->mPitch, comp->mYaw };

        glm::vec3 pos = *player->getPos();
        glm::vec3 from = pos;
        std::string xStr = args[1];
        std::string yStr = args[2];
        std::string zStr = args[3];

        // if the string only contains a symbol, append 0 to it
        if (xStr.size() == 1) xStr += "0";
        if (yStr.size() == 1) yStr += "0";
        if (zStr.size() == 1) zStr += "0";

        // if number starts with ~, add to current position
        // if number starts with ^, make the pos relative to the player's rotation
        if (xStr[0] == '~')
        {
            pos.x += std::stof(xStr.substr(1));
        }
        else if (xStr[0] == '^')
        {
            pos.x += std::stof(xStr.substr(1)) * cos(glm::radians(rotation.y + 90));
            pos.z += std::stof(xStr.substr(1)) * sin(glm::radians(rotation.y + 90));
        }
        else
        {
            pos.x = std::stof(xStr);
        }

        if (yStr[0] == '~')
        {
            pos.y += std::stof(yStr.substr(1));
        }
        else if (yStr[0] == '^')
        {
            pos.y += std::stof(yStr.substr(1)) * sin(glm::radians(rotation.x));
        }
        else
        {
            pos.y = std::stof(yStr);
        }

        if (zStr[0] == '~')
        {
            pos.z += std::stof(zStr.substr(1));
        }
        else if (zStr[0] == '^')
        {
            pos.x += std::stof(zStr.substr(1)) * cos(glm::radians(rotation.y + 90));
            pos.z += std::stof(zStr.substr(1)) * sin(glm::radians(rotation.y + 90));
        }
        else
        {
            pos.z = std::stof(zStr);
        }

        serverTeleport(from, pos);
        ChatUtils::displayClientMessage("§aTeleported to " + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + "!");
    } catch (const std::exception& e)
    {
        ChatUtils::displayClientMessage("§cInvalid coordinates entered!");
        return;
    }
}

std::vector<std::string> TeleportCommand::getAliases() const
{
    return { "tp" };
}

std::string TeleportCommand::getDescription() const
{
    return "Teleport to a player or coordinates (server-side TP).";
}

std::string TeleportCommand::getUsage() const
{
    return "Usage: teleport <player> OR teleport <x> <y> <z> (Also supports ~ and ^ operators)";
}
