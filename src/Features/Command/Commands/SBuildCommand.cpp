// SBuildCommand.cpp
#include "SBuildCommand.hpp"
#include <Features/Modules/Player/SchematicBuilder.hpp>

void SBuildCommand::execute(const std::vector<std::string>& args) {
    auto module = gFeatureManager->mModuleManager->getModule<SchematicBuilder>();
    if (!module || !module->mEnabled) {
        ChatUtils::displayClientMessage("§cEnable SchematicBuilder module first!");
        return;
    }
    module->startBuilding();
}