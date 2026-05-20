#include "SCopyCommand.hpp"
#include <Features/Modules/Player/SchematicBuilder.hpp>

void SCopyCommand::execute(const std::vector<std::string>& args)
{
    auto module = gFeatureManager->mModuleManager->getModule<SchematicBuilder>();
    if (!module) {
        ChatUtils::displayClientMessage("§cSchematicBuilder module not found!");
        return;
    }
    
    if (!module->mEnabled) {
        ChatUtils::displayClientMessage("§cEnable SchematicBuilder module first!");
        return;
    }
    
    module->copySelection();
}

std::vector<std::string> SCopyCommand::getAliases() const { return {"sc"}; }
std::string SCopyCommand::getDescription() const { return "Copy selection to clipboard."; }
std::string SCopyCommand::getUsage() const { return "Usage: .scopy"; }