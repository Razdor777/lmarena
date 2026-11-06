// SCancelCommand.cpp
#include "SCancelCommand.hpp"
#include <Features/Modules/Player/SchematicBuilder.hpp>

void SCancelCommand::execute(const std::vector<std::string>& args) {
    auto module = gFeatureManager->mModuleManager->getModule<SchematicBuilder>();
    if (module) {
        module->cancelPaste();
    }
}