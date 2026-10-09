//
// Dexko — .reach command implementation.
//

#include "ReachCommand.hpp"

#include <algorithm>
#include <cstdio>
#include <Features/Modules/Combat/Reach.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/StringUtils.hpp>

void ReachCommand::execute(const std::vector<std::string>& args)
{
    auto* reach = gFeatureManager->mModuleManager->getModule<Reach>();
    if (!reach)
    {
        ChatUtils::displayClientMessage("§cReach module is not registered?!");
        return;
    }

    // .reach — status
    if (args.size() < 2)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", reach->mCombatReach.mValue);
        ChatUtils::displayClientMessage(
            "§adexko§7 » §fCombat reach: §6" + std::string(buf) +
            "§7 (" + (reach->mEnabled ? "§aon" : "§coff") + "§7)");
        ChatUtils::displayClientMessage("§7Usage: §6" + getUsage());
        return;
    }

    // .reach off
    if (StringUtils::equalsIgnoreCase(args[1], "off") || StringUtils::equalsIgnoreCase(args[1], "disable"))
    {
        reach->setEnabled(false);
        ChatUtils::displayClientMessage("§adexko§7 » §cReach disabled.");
        return;
    }

    // .reach <value>
    try
    {
        // Accept both "3" and "3.00"; also tolerate a comma as the decimal separator
        std::string raw = args[1];
        std::replace(raw.begin(), raw.end(), ',', '.');
        size_t parsed = 0;
        const float value = std::stof(raw, &parsed);
        if (parsed != raw.size())
            throw std::invalid_argument("trailing garbage");

        const float clamped = std::clamp(value, reach->mCombatReach.mMin, reach->mCombatReach.mMax);
        reach->mCombatReach.setValue(clamped);
        reach->setEnabled(true);

        if (clamped != value)
            ChatUtils::displayClientMessage("§adexko§7 » §eValue clamped to the allowed range.");

        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", clamped);
        ChatUtils::displayClientMessage(
            "§adexko§7 » §fCombat reach set to §6" + std::string(buf) + "§f, reach §aenabled§f.");
    }
    catch (const std::exception&)
    {
        ChatUtils::displayClientMessage("§cInvalid value: §6" + args[1] + "§c. " + getUsage());
    }
}

std::vector<std::string> ReachCommand::getAliases() const { return {}; }

std::string ReachCommand::getDescription() const { return "Sets the combat attack range"; }

std::string ReachCommand::getUsage() const { return ".reach <3.0 - 7.0 | off>"; }
