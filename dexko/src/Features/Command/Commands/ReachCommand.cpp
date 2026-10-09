#include "ReachCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Combat/Reach.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <algorithm>

void ReachCommand::execute(const std::vector<std::string>& args)
{
    auto* reach = gFeatureManager->mModuleManager->getModule<Reach>();
    if (!reach)
    {
        ChatUtils::displayClientMessage("§cReach module not available.");
        return;
    }

    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage(
            "§aCombat Reach: §6{:.2f}", reach->mCombatReach.mValue);
        ChatUtils::displayClientMessage("§7Usage: §e.reach <value> §7(e.g. §e.reach 4.5§7)");
        return;
    }

    float value;
    try
    {
        value = std::stof(args[1]);
    }
    catch (...)
    {
        ChatUtils::displayClientMessage("§cInvalid number §6" + args[1] + "§c.");
        return;
    }

    if (value < 0.f)
    {
        ChatUtils::displayClientMessage("§cReach cannot be negative.");
        return;
    }

    reach->mCombatReach.setValue(value);

    // The reach detour only takes effect while the module is on.
    if (!reach->mEnabled)
        reach->setEnabled(true);

    ChatUtils::displayClientMessage(
        "§aCombat Reach set to §6{:.2f}", reach->mCombatReach.mValue);
}

std::vector<std::string> ReachCommand::getAliases() const
{
    return { "r" };
}

std::string ReachCommand::getDescription() const
{
    return "Sets the combat (attack) reach range.";
}

std::string ReachCommand::getUsage() const
{
    return "Usage: .reach <value>";
}
