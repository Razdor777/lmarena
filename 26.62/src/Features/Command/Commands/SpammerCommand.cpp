#include "SpammerCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Misc/Spammer.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void SpammerCommand::execute(const std::vector<std::string>& args)
{
    // Склеиваем все аргументы обратно, чтобы пробелы внутри < > не терялись
    std::string full;
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (i > 0) full += " ";
        full += args[i];
    }

    // Ищем строго между первой '<' и первой '>'
    size_t lt = full.find('<');
    size_t gt = full.find('>');

    if (lt == std::string::npos || gt == std::string::npos || gt <= lt)
    {
        ChatUtils::displayClientMessage("§cUsage: .spam message <text>");
        return;
    }

    std::string msg = full.substr(lt + 1, gt - lt - 1);

    auto spammer = gFeatureManager->mModuleManager->getModule<Spammer>();
    if (!spammer)
    {
        ChatUtils::displayClientMessage("§cSpammer module not found!");
        return;
    }

    spammer->setMessage(msg);
    ChatUtils::displayClientMessage("§aSpammer message set to: §f" + msg);
}

std::vector<std::string> SpammerCommand::getAliases() const
{
    return {};
}

std::string SpammerCommand::getDescription() const
{
    return "Set spammer message";
}

std::string SpammerCommand::getUsage() const
{
    return ".spam message <text>";
}