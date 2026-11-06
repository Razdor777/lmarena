//
//

#include "AimbotCommand.hpp"

#include <Utils/GameUtils/ChatUtils.hpp>

void AimbotCommand::execute(const std::vector<std::string>& args)
{
    Module* module = gFeatureManager->mModuleManager->getModule("Aimbot");
    if (module == nullptr)
    {
        ChatUtils::displayClientMessage("§cAimbot is not available in this build.");
        return;
    }

    module->toggle();
    const bool newState = !module->mEnabled; // mEnabled updates on the next tick
    ChatUtils::displayClientMessage("§aAimbot is now " + std::string(newState ? "§aenabled" : "§cdisabled") + "§a!");
}

std::vector<std::string> AimbotCommand::getAliases() const
{
    return {"ab"};
}

std::string AimbotCommand::getDescription() const
{
    return "Toggles the Aimbot module";
}

std::string AimbotCommand::getUsage() const
{
    return "Usage: .aimbot";
}
