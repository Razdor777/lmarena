//
// Pos2Command - Set position 2 for AutoPatrol
//

#include "Pos2Command.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Features/Modules/Movement/AutoPatrol.hpp>

void Pos2Command::execute(const std::vector<std::string>& args)
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

    AutoPatrol::sPos2 = pos;
    AutoPatrol::sPos2Set = true;
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "§aPos2 set to §7" << pos.x << ", " << pos.y << ", " << pos.z;
    ChatUtils::displayClientMessage(ss.str());
}

std::vector<std::string> Pos2Command::getAliases() const
{
    return {"p2"};
}

std::string Pos2Command::getDescription() const
{
    return "Set position 2 for AutoPatrol module.";
}

std::string Pos2Command::getUsage() const
{
    return "Usage: .pos2 [x y z] - Sets pos2 to current position or specified coords";
}