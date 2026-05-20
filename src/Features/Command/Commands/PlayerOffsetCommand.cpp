#include "PlayerOffsetCommand.hpp"
#include <Features/Modules/ModuleManager.hpp>
#include <Features/Modules/Visual/PlayerPositionOffset.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/StringUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

void PlayerOffsetCommand::execute(const std::vector<std::string>& args)
{
    auto module = gFeatureManager->mModuleManager->getModule<PlayerPositionOffset>();
    if (!module)
    {
        ChatUtils::displayClientMessage("§cМодуль PlayerPositionOffset не найден!");
        return;
    }

    // args[0] = "playeroffset", args[1] = action, args[2] = имя
    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§c" + getUsage());
        return;
    }

    std::string action = StringUtils::toLower(args[1]);

    if (action == "set")
    {
        if (args.size() < 3)
        {
            ChatUtils::displayClientMessage("§cУкажите имя игрока!");
            return;
        }

        auto targetList = ActorUtils::getActorList(true, true);
        Actor* target = nullptr;
        std::string targetName = StringUtils::toLower(args[2]);

        // First try exact match
        for (auto& actor : targetList)
        {
            if (StringUtils::toLower(actor->getRawName()) == targetName)
            {
                target = actor;
                break;
            }
        }

        // If no exact match, try partial/substring match
        if (!target)
        {
            std::vector<Actor*> matches;
            for (auto& actor : targetList)
            {
                std::string name = StringUtils::toLower(actor->getRawName());
                if (name.find(targetName) != std::string::npos)
                {
                    matches.push_back(actor);
                }
            }

            if (matches.size() == 1)
            {
                target = matches[0];
            }
            else if (matches.size() > 1)
            {
                ChatUtils::displayClientMessage("§eНайдено несколько игроков для '§6" + args[2] + "§e':");
                for (auto& m : matches)
                {
                    ChatUtils::displayClientMessage("  §7- §f" + m->getRawName());
                }
                ChatUtils::displayClientMessage("§eУточните имя!");
                return;
            }
        }

        if (target == nullptr)
        {
            ChatUtils::displayClientMessage("§cНе удалось найти игрока: " + args[2] + "!");
            return;
        }

        module->setTargetPlayer(target->getRawName(), target->getRuntimeID());

        if (!module->mEnabled)
        {
            module->toggle();
            ChatUtils::displayClientMessage("§aМодуль автоматически включен");
        }
        return;
    }
    else if (action == "clear")
    {
        module->clearTarget();
        return;
    }
    else if (action == "get")
    {
        std::string current = module->getTargetPlayer();
        if (current.empty())
        {
            ChatUtils::displayClientMessage("§eЦель не установлена");
        }
        else
        {
            ChatUtils::displayClientMessage("§eТекущая цель: §f" + current);
        }
        return;
    }

    ChatUtils::displayClientMessage("§c" + getUsage());
}