#include "HtpCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/HitParticles.hpp>
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
}

void HtpCommand::execute(const std::vector<std::string>& args)
{
    auto* hp = gFeatureManager->mModuleManager->getModule<HitParticles>();
    if (!hp)
    {
        ChatUtils::displayClientMessage("§cHitParticles module not available.");
        return;
    }

    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§aCurrent style: §6" +
            hp->mStyle.mValues[hp->mStyle.as<int>()]);
        ChatUtils::displayClientMessage("§7Styles: §e" +
            [&]() {
                std::string list;
                for (size_t i = 0; i < hp->mStyle.mValues.size(); i++)
                {
                    list += hp->mStyle.mValues[i];
                    if (i + 1 < hp->mStyle.mValues.size()) list += "§7, §e";
                }
                return list;
            }());
        ChatUtils::displayClientMessage("§7Usage: §e.htp <style> §7(e.g. §e.htp sparks§7)");
        return;
    }

    const std::string wanted = lower(args[1]);
    int idx = -1;
    for (int i = 0; i < static_cast<int>(hp->mStyle.mValues.size()); i++)
    {
        if (lower(hp->mStyle.mValues[i]) == wanted)
        {
            idx = i;
            break;
        }
    }

    if (idx < 0)
    {
        ChatUtils::displayClientMessage("§cUnknown style §6" + args[1] + "§c.");
        return;
    }

    hp->mStyle.setValue(static_cast<HitParticles::Style>(idx));

    if (!hp->mEnabled)
        hp->setEnabled(true);

    ChatUtils::displayClientMessage("§aHitParticles style set to §6" +
                                    hp->mStyle.mValues[idx]);
}

std::vector<std::string> HtpCommand::getAliases() const
{
    return { "hitparticles" };
}

std::string HtpCommand::getDescription() const
{
    return "Changes the HitParticles burst style.";
}

std::string HtpCommand::getUsage() const
{
    return "Usage: .htp <sparks/fire/hearts/blood/critical/frost/soul/toxic>";
}
