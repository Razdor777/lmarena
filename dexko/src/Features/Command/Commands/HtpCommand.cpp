//
// Dexko — .htp command implementation.
//

#include "HtpCommand.hpp"

#include <Features/Modules/Visual/HitParticles.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/StringUtils.hpp>

void HtpCommand::execute(const std::vector<std::string>& args)
{
    auto* htp = gFeatureManager->mModuleManager->getModule<HitParticles>();
    if (!htp)
    {
        ChatUtils::displayClientMessage("§cHitParticles module is not registered?!");
        return;
    }

    const std::vector<std::string> styleNames = {
        "blood", "sparks", "critical", "hearts", "fire", "frost", "soul", "toxic"
    };

    // .htp — status
    if (args.size() < 2)
    {
        std::string list;
        for (size_t i = 0; i < styleNames.size(); i++)
        {
            if (i) list += "§7, §6";
            list += (static_cast<int>(i) == htp->mStyle.as<int>() ? "§a" : "§6") + styleNames[i];
        }

        ChatUtils::displayClientMessage(
            "§adexko§7 » §fHitParticles style: §6" + styleNames[htp->mStyle.as<int>()] +
            "§7 (" + (htp->mEnabled ? "§aon" : "§coff") + "§7)");
        ChatUtils::displayClientMessage("§7Styles: " + list);
        ChatUtils::displayClientMessage("§7Usage: §6" + getUsage());
        return;
    }

    // .htp off
    if (StringUtils::equalsIgnoreCase(args[1], "off") || StringUtils::equalsIgnoreCase(args[1], "disable"))
    {
        htp->setEnabled(false);
        ChatUtils::displayClientMessage("§adexko§7 » §cHitParticles disabled.");
        return;
    }

    // .htp <style>
    for (size_t i = 0; i < styleNames.size(); i++)
    {
        if (StringUtils::equalsIgnoreCase(args[1], styleNames[i]))
        {
            htp->mStyle.setValue(static_cast<int>(i));
            htp->setEnabled(true);
            ChatUtils::displayClientMessage(
                "§adexko§7 » §fHitParticles style set to §6" + styleNames[i] + "§f, module §aenabled§f.");
            return;
        }
    }

    ChatUtils::displayClientMessage("§cUnknown style: §6" + args[1] + "§c. " + getUsage());
}

std::vector<std::string> HtpCommand::getAliases() const { return {"hitparticles"}; }

std::string HtpCommand::getDescription() const { return "Changes the HitParticles style"; }

std::string HtpCommand::getUsage() const { return ".htp <blood | sparks | critical | hearts | fire | frost | soul | toxic | off>"; }
