//
// ScriptsCommand — `scripts list | reload [name] | enable <name> | disable <name>`
//

#include "ScriptsCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void ScriptsCommand::execute(const std::vector<std::string>& args)
{
    auto& sm = gFeatureManager->mScriptManager;
    if (!sm)
    {
        ChatUtils::displayClientMessage("§cThe script manager is not running.");
        return;
    }

    const std::string sub = args.size() > 1 ? args[1] : "list";

    if (sub == "list")
    {
        const auto& scripts = sm->getScripts();
        if (scripts.empty())
        {
            ChatUtils::displayClientMessage("§7No scripts found in §eScripts\\Modules§7 and §eScripts\\Commands§7.");
            return;
        }

        ChatUtils::displayClientMessage("§6Scripts (§e{}§6):", scripts.size());
        for (const auto& script : scripts)
        {
            if (!script) continue;
            const char* kind = script->mType == ScriptType::Module ? "module" : "command";
            const char* state = script->mType == ScriptType::Module
                                    ? (script->mEnabled ? "§aon§7" : "§coff§7")
                                    : "§7-";
            ChatUtils::displayClientMessage("§6- §e{} §7({}) {}", script->mName, kind, state);
        }
        return;
    }

    if (sub == "reload")
    {
        if (args.size() > 2)
        {
            const bool ok = sm->reloadScript(args[2]);
            ChatUtils::displayClientMessage(ok
                ? "§aReloaded script §e{}§a."
                : "§cNo script named §e{}§c.", args[2]);
            return;
        }

        sm->reloadAll();
        ChatUtils::displayClientMessage("§aReloaded §e{}§a scripts.", sm->getScripts().size());
        return;
    }

    if (sub == "enable" || sub == "disable")
    {
        if (args.size() < 3)
        {
            ChatUtils::displayClientMessage("§cUsage: §e.scripts {} <name>", sub);
            return;
        }

        auto script = sm->getScript(args[2]);
        if (!script || script->mType != ScriptType::Module)
        {
            ChatUtils::displayClientMessage("§cNo module script named §e{}§c.", args[2]);
            return;
        }

        sm->setScriptEnabled(script, sub == "enable");
        ChatUtils::displayClientMessage("§aScript §e{}§a {}.", script->mName, sub == "enable" ? "enabled" : "disabled");
        return;
    }

    ChatUtils::displayClientMessage("§cUsage: §e.scripts list§c | §e.scripts reload [name]§c | §e.scripts enable|disable <name>");
}

std::vector<std::string> ScriptsCommand::getAliases() const
{
    return { "script" };
}

std::string ScriptsCommand::getDescription() const
{
    return "Manage Luau scripts.";
}

std::string ScriptsCommand::getUsage() const
{
    return ".scripts <list|reload|enable|disable> [name]";
}
