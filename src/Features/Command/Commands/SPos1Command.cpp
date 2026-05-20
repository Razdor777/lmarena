//
// SPos1Command implementation
//

#include "SPos1Command.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Features/Modules/Player/SchematicBuilder.hpp>

void SPos1Command::execute(const std::vector<std::string>& args)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        ChatUtils::displayClientMessage("§cPlayer not found!");
        return;
    }

    glm::ivec3 pos;
    
    if (args.size() >= 4) {
        try {
            pos.x = std::stoi(args[1]);
            pos.y = std::stoi(args[2]);
            pos.z = std::stoi(args[3]);
        } catch (...) {
            ChatUtils::displayClientMessage("§cInvalid coordinates!");
            return;
        }
    } else {
        // Позиция блока под ногами
        glm::vec3 playerPos = *player->getPos();
        pos = glm::ivec3(
            static_cast<int>(std::floor(playerPos.x)),
            static_cast<int>(std::floor(playerPos.y)) - 1,
            static_cast<int>(std::floor(playerPos.z))
        );
    }

    SchematicBuilder::sPos1 = pos;
    SchematicBuilder::sPos1Set = true;
    
    ChatUtils::displayClientMessage("§aSchematic Pos1 set to: §f{}, {}, {}", pos.x, pos.y, pos.z);
    
    if (SchematicBuilder::sPos1Set && SchematicBuilder::sPos2Set) {
        glm::ivec3 min(
            std::min(SchematicBuilder::sPos1.x, SchematicBuilder::sPos2.x),
            std::min(SchematicBuilder::sPos1.y, SchematicBuilder::sPos2.y),
            std::min(SchematicBuilder::sPos1.z, SchematicBuilder::sPos2.z)
        );
        glm::ivec3 max(
            std::max(SchematicBuilder::sPos1.x, SchematicBuilder::sPos2.x),
            std::max(SchematicBuilder::sPos1.y, SchematicBuilder::sPos2.y),
            std::max(SchematicBuilder::sPos1.z, SchematicBuilder::sPos2.z)
        );
        glm::ivec3 size = max - min + glm::ivec3(1);
        int volume = size.x * size.y * size.z;
        ChatUtils::displayClientMessage("§7Selection: §f{}x{}x{} §7(§f{} §7blocks)", 
            size.x, size.y, size.z, volume);
    }
}

std::vector<std::string> SPos1Command::getAliases() const
{
    return {"sp1"};
}

std::string SPos1Command::getDescription() const
{
    return "Set position 1 for SchematicBuilder.";
}

std::string SPos1Command::getUsage() const
{
    return "Usage: .spos1 [x y z]";
}