#include "CrosshairCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/CustomCrosshair.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <algorithm>
#include <cctype>

void CrosshairCommand::execute(const std::vector<std::string>& args)
{
    auto* ch = gFeatureManager->mModuleManager->getModule<CustomCrosshair>();
    if (!ch)
    {
        ChatUtils::displayClientMessage("§cCustomCrosshair module not available.");
        return;
    }

    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§7Usage: §e.crosshair <on/off>");
        return;
    }

    std::string a = args[1];
    std::transform(a.begin(), a.end(), a.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (a == "on" || a == "enable" || a == "1")
    {
        ch->setEnabled(true);
        ChatUtils::displayClientMessage("§aCustomCrosshair enabled.");
    }
    else if (a == "off" || a == "disable" || a == "0")
    {
        ch->setEnabled(false);
        ChatUtils::displayClientMessage("§cCustomCrosshair disabled.");
    }
    else
    {
        ChatUtils::displayClientMessage("§cUnknown argument §6" + args[1] + "§c.");
        ChatUtils::displayClientMessage("§7Usage: §e.crosshair <on/off>");
    }
}

std::vector<std::string> CrosshairCommand::getAliases() const
{
    return { "ch" };
}

std::string CrosshairCommand::getDescription() const
{
    return "Turns the custom crosshair on or off.";
}

std::string CrosshairCommand::getUsage() const
{
    return "Usage: .crosshair <on/off>";
}
