//
// Dexko CommandManager — форк движкового CommandManager с урезанным списком.
//
// Команды Dexko:
//   .help                — список команд
//   .t <module>          — вкл/выкл модуль (GUI нет, поэтому это основной тумблер)
//   .config save/load/…  — конфиги
//   .reach <3.0-7.0|off> — combat reach
//   .htp <style|off>     — стиль HitParticles (sparks/fire/hearts/…)
//

#include <algorithm>
#include <sstream>
#include <Features/FeatureManager.hpp>
#include <Features/Command/CommandManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/StringUtils.hpp>

#include <Features/Command/Command.hpp>
#include <Features/Command/Commands/HelpCommand.hpp>
#include <Features/Command/Commands/ToggleCommand.hpp>
#include <Features/Command/Commands/ConfigCommand.hpp>
#include "Commands/ReachCommand.hpp"
#include "Commands/HtpCommand.hpp"

#include "Features/Events/ChatEvent.hpp"
#include "spdlog/spdlog.h"

void CommandManager::init()
{
    ADD_COMMAND(HelpCommand);
    ADD_COMMAND(ToggleCommand);
    ADD_COMMAND(ConfigCommand);
    ADD_COMMAND(ReachCommand);
    ADD_COMMAND(HtpCommand);

    // Look for any commands that have duplicate names
    for (size_t i = 0; i < mCommands.size(); i++)
    {
        for (size_t j = i + 1; j < mCommands.size(); j++)
        {
            auto names1 = mCommands[i]->getNames();
            auto names2 = mCommands[j]->getNames();

            for (const auto& name1 : names1)
            {
                for (const auto& name2 : names2)
                {
                    if (name1 == name2)
                    {
                        spdlog::error("Commands [{}] and [{}] have the same name: {}", names1[0], names2[0], name1);
                        throw std::runtime_error("Duplicate command names: " + std::string(name1));
                    }
                }
            }
        }
    }

    gFeatureManager->mDispatcher->listen<ChatEvent, &CommandManager::handleCommand>(this);
}

void CommandManager::shutdown()
{
    gFeatureManager->mDispatcher->deafen<ChatEvent, &CommandManager::handleCommand>(this);
    mCommands.clear();

    spdlog::info("Successfully shut down CommandManager");
}

void CommandManager::handleCommand(ChatEvent& event)
{
    std::string command = event.getMessage();

    if (command[0] != '.')
        return;

    event.setCancelled(true);

    // Remove the dot from the command
    std::string cmd = command.substr(1);

    // Split the command into arguments
    const std::vector<std::string> args = StringUtils::split(cmd, ' ');

    if (args.empty())
        return;

    // Find the command
    const std::string_view commandName = args[0];

    auto it = std::ranges::find_if(mCommands, [=](const auto& command)
    {
        return command->matchName(commandName);
    });

    if (it == mCommands.end())
    {
        if (!event.mSpecial) ChatUtils::displayClientMessage(xorstr_("§cThe command §6'") + std::string(commandName) + xorstr_("'§c does not exist!"));
        return;
    }

    // Execute the command
    spdlog::info("Executing command: {}", commandName);
    (*it)->execute(args);
}

std::vector<Command*> CommandManager::getCommands() const
{
    std::vector<Command*> commands;
    commands.reserve(mCommands.size());

    for (const auto& command : mCommands)
    {
        commands.push_back(command.get());
    }

    return commands;
}
