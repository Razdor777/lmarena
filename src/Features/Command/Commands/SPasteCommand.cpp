#include "SPasteCommand.hpp"
#include <Features/Modules/Player/SchematicBuilder.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

void SPasteCommand::execute(const std::vector<std::string>& args)
{
    auto module = gFeatureManager->mModuleManager->getModule<SchematicBuilder>();
    if (!module || !module->mEnabled) {
        ChatUtils::displayClientMessage("§cEnable SchematicBuilder module first!");
        return;
    }
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    glm::ivec3 pastePos;
    
    if (args.size() >= 4) {
        try {
            pastePos.x = std::stoi(args[1]);
            pastePos.y = std::stoi(args[2]);
            pastePos.z = std::stoi(args[3]);
        } catch (...) {
            ChatUtils::displayClientMessage("§cInvalid coordinates!");
            return;
        }
    } else {
        glm::vec3 pos = *player->getPos();
        pastePos = glm::ivec3(
            static_cast<int>(std::floor(pos.x)),
            static_cast<int>(std::floor(pos.y)),
            static_cast<int>(std::floor(pos.z))
        );
    }
    
    module->preparePaste(pastePos);
}

std::vector<std::string> SPasteCommand::getAliases() const { return {}; }
std::string SPasteCommand::getDescription() const { return "Paste clipboard at position (shows preview)."; }
std::string SPasteCommand::getUsage() const { return "Usage: .spaste [x y z]"; }