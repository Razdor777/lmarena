#include "ReachCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Combat/Reach.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
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

    bool isOn(const std::string& s)
    {
        return s == "on" || s == "true" || s == "enable" || s == "1" || s == "yes";
    }

    bool isOff(const std::string& s)
    {
        return s == "off" || s == "false" || s == "disable" || s == "0" || s == "no";
    }

    float parseNumber(const std::string& in, bool& ok)
    {
        try
        {
            size_t used = 0;
            const float v = std::stof(in, &used);
            ok = (used == in.size());
            return v;
        }
        catch (...)
        {
            ok = false;
            return 0.f;
        }
    }
}

void ReachCommand::execute(const std::vector<std::string>& args)
{
    auto* reach = gFeatureManager->mModuleManager->getModule<Reach>();
    if (!reach)
    {
        ChatUtils::displayClientMessage("§cReach module not available.");
        return;
    }

    // ── .reach ───────────────────────────────────────────────────────────────
    if (args.size() < 2 || lower(args[1]) == "info" || lower(args[1]) == "status")
    {
        ChatUtils::displayClientMessage(
            "§7Reach is " + std::string(reach->mEnabled ? "§aon" : "§coff") +
            "§7, combat §6" + fmt::format("{:.2f}", reach->mCombatReach.mValue) +
            "§7, block §6" + fmt::format("{:.2f}", reach->mBlockReach.mValue) +
            "§7, force " + std::string(reach->mForce.mValue ? "§aon" : "§coff"));
        ChatUtils::displayClientMessage(
            "§7Usage: §e.reach <{:.1f}-{:.1f}>§7, §e.reach force <on|off>",
            reach->mCombatReach.mMin, reach->mCombatReach.mMax);
        ChatUtils::displayClientMessage(
            "§7Note: without §eforce§7 the engine itself caps the value around §67§7 blocks.");
        return;
    }

    const std::string first = lower(args[1]);

    // ── .reach force <on|off> ────────────────────────────────────────────────
    if (first == "force")
    {
        if (args.size() < 3)
        {
            ChatUtils::displayClientMessage(
                "§7Force range is " + std::string(reach->mForce.mValue ? "§aon" : "§coff") +
                "§7. Usage: §e.reach force <on|off>");
            return;
        }

        const std::string what = lower(args[2]);
        if (!isOn(what) && !isOff(what))
        {
            ChatUtils::displayClientMessage("§cInvalid value §6" + args[2] + "§c. Use §eon§c/§eoff§c.");
            return;
        }

        reach->mForce.mValue = isOn(what);
        if (!reach->mEnabled) reach->setEnabled(true);

        if (reach->mForce.mValue)
            ChatUtils::displayClientMessage(
                "§aForce range §aon§a: the value you set is written over the engine's own. "
                "If hits stop registering, turn it back off with §e.reach force off§a.");
        else
            ChatUtils::displayClientMessage("§aForce range §coff§a: back to the vanilla clamp.");
        return;
    }

    // ── .reach reset ─────────────────────────────────────────────────────────
    if (first == "reset")
    {
        reach->mCombatReach.setValue(3.0f);
        if (!reach->mEnabled) reach->setEnabled(true);
        ChatUtils::displayClientMessage("§aCombat reach reset to §63.00§a.");
        return;
    }

    // ── .reach <value> ───────────────────────────────────────────────────────
    bool ok = false;
    float value = parseNumber(args[1], ok);
    if (!ok)
    {
        ChatUtils::displayClientMessage("§cInvalid number §6" + args[1] + "§c.");
        return;
    }

    const float clamped = std::clamp(value, reach->mCombatReach.mMin, reach->mCombatReach.mMax);
    reach->mCombatReach.setValue(clamped);

    // The detour only takes effect while the module is on.
    if (!reach->mEnabled) reach->setEnabled(true);

    if (clamped != value)
    {
        ChatUtils::displayClientMessage(
            "§eValue clamped to §6{:.2f}§e (range {:.1f}-{:.1f}).",
            clamped, reach->mCombatReach.mMin, reach->mCombatReach.mMax);
    }

    ChatUtils::displayClientMessage(
        "§aCombat Reach set to §6{:.2f}", reach->mCombatReach.mValue);

    if (reach->mCombatReach.mValue > 7.0f && !reach->mForce.mValue)
    {
        ChatUtils::displayClientMessage(
            "§7Values above §67§7 are capped by the engine's own range check. "
            "To use §6{:.2f}§7 try §e.reach force on§7.",
            reach->mCombatReach.mValue);
    }
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
    return "Usage: .reach <value> | .reach force <on|off> | .reach reset";
}
