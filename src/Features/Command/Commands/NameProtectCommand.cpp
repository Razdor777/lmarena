//
// Created by alteik on 13/10/2024.
//

#include "NameProtectCommand.hpp"
#include <Features/Modules/Visual/NameProtect.hpp>

void NameProtectCommand::execute(const std::vector<std::string>& args)
{
    auto nameProtect = gFeatureManager->mModuleManager->getModule<NameProtect>();
    if(!nameProtect) return;

    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§c" + getUsage());
        return;
    }

    nameProtect->mNewName = args[1];
    ChatUtils::displayClientMessage("successfuly changed name to: §a" + nameProtect->mNewName);
}

std::vector<std::string> NameProtectCommand::getAliases() const
{
    return {"sn", "name", "nick"};
}

std::string NameProtectCommand::getDescription() const
{
    return "Changes your protected nickname.";
}

std::string NameProtectCommand::getUsage() const
{
    return "Usage: .name <name>";
}
