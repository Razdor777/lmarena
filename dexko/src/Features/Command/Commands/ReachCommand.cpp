//
// ReachCommand — `.reach`.
//
// Beyond setting the range it can now flip the Full Override mode, which is the
// switch that actually lets the client use values above the game's own clamp
// (see Reach.cpp for why the plain value is capped around 7).
//
//   .reach                  -> current range + mode
//   .reach <3.00-7.00>      -> set range, enables Reach
//   .reach <value> full     -> set range + Full Override
//   .reach <value> min      -> set range + the game's own clamp
//   .reach full | min       -> flip the mode only
//

#include "ReachCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Combat/Reach.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <spdlog/fmt/fmt.h>
#include <algorithm>
#include <cctype>

namespace
{
    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool isFull(const std::string& s) { return s == "full" || s == "override" || s == "over" || s == "bypass" || s == "on"; }
    bool isMin(const std::string& s) { return s == "min" || s == "clamp" || s == "safe" || s == "off"; }

    void printStatus(Reach* reach)
    {
        ChatUtils::displayClientMessage(fmt::format("§7Combat reach: §6{:.2f}§7, block reach: §6{:.2f}",
                                                    reach->mCombatReach.mValue, reach->mBlockReach.mValue));
        ChatUtils::displayClientMessage(std::string("§7Mode: ") + (reach->mFullOverride.mValue
                                            ? "§aFull Override§7 (the game's clamp is bypassed)"
                                            : "§eGame clamp§7 (values above ~7 are cut back)"));
    }
}

void ReachCommand::execute(const std::vector<std::string>& args)
{
    auto* reach = gFeatureManager->mModuleManager->getModule<Reach>();
    if (!reach)
    {
        ChatUtils::displayClientMessage("§cReach is not available in this build.");
        return;
    }

    if (args.size() < 2)
    {
        printStatus(reach);
        ChatUtils::displayClientMessage("§7Usage: §e.reach <value> [full|min] §7or §e.reach full §7/ §e.reach min");
        return;
    }

    const std::string first = lower(args[1]);

    // Mode-only forms: `.reach full`, `.reach min`
    if (isFull(first) || isMin(first))
    {
        reach->mFullOverride.setValue(isFull(first));
        if (!reach->mEnabled) reach->setEnabled(true);
        printStatus(reach);
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
        ChatUtils::displayClientMessage("§7Usage: §e.reach <value> [full|min]");
        return;
    }

    // Keep the knob sane: below 0.5 the client cannot click anything, and above
    // 50 the server rejects every hit anyway.
    const float clamped = std::clamp(value, 0.5f, 50.f);
    if (clamped != value)
        ChatUtils::displayClientMessage(fmt::format("§eValue clamped to §6{:.2f}§e.", clamped));

    reach->mCombatReach.setValue(clamped);

    bool modeChanged = false;
    if (args.size() >= 3)
    {
        const std::string second = lower(args[2]);
        if (isFull(second)) { reach->mFullOverride.setValue(true);  modeChanged = true; }
        else if (isMin(second)) { reach->mFullOverride.setValue(false); modeChanged = true; }
        else
        {
            ChatUtils::displayClientMessage("§eUnknown option §6" + args[2] + "§e — use §e full §eor §emin§e.");
        }
    }

    // The reach detour only takes effect while the module is on.
    if (!reach->mEnabled) reach->setEnabled(true);

    ChatUtils::displayClientMessage(fmt::format("§aCombat reach set to §6{:.2f}§a.", reach->mCombatReach.mValue));

    if (modeChanged || reach->mFullOverride.mValue)
        printStatus(reach);

    if (value > 7.f && !reach->mFullOverride.mValue)
    {
        ChatUtils::displayClientMessage(fmt::format("§7Note: without §eFull Override §7the game clamps this back to ~7 "
                                                    "blocks. Try §e.reach {:.0f} full§7.", value));
    }

    if (reach->mFullOverride.mValue)
        ChatUtils::displayClientMessage("§7Client-side override is on; a server can still reject hits beyond its own limit.");
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
    return "Usage: .reach <value> [full|min]";
}
