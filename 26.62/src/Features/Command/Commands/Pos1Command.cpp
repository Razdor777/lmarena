//
// Pos1Command - Set position 1 for AutoPatrol
//

#include "Pos1Command.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Features/Modules/Movement/AutoPatrol.hpp>

void Pos1Command::execute(const std::vector<std::string>& args)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (player == nullptr)
    {
        ChatUtils::displayClientMessage("§cPlayer not found!");
        return;
    }

    glm::vec3 pos = *player->getPos();
    
    // Если указаны координаты вручную
    if (args.size() == 4)
    {
        try
        {
            pos.x = std::stof(args[1]);
            pos.y = std::stof(args[2]);
            pos.z = std::stof(args[3]);
        }
        catch (const std::exception& e)
        {
            ChatUtils::displayClientMessage("§cInvalid coordinates!");
            return;
        }
    }

    AutoPatrol::sPos1 = pos;
    AutoPatrol::sPos1Set = true;
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "§aPos1 set to §7" << pos.x << ", " << pos.y << ", " << pos.z;
    ChatUtils::displayClientMessage(ss.str());
}

std::vector<std::string> Pos1Command::getAliases() const
{
    return {"p1"};
}

std::string Pos1Command::getDescription() const
{
    return "Set position 1 for AutoPatrol module.";
}

std::string Pos1Command::getUsage() const
{
    return "Usage: .pos1 [x y z] - Sets pos1 to current position or specified coords";
}