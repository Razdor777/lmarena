//
// .diag — см. DiagCommand.hpp.
//

#include "DiagCommand.hpp"

#include <Features/Modules/Misc/Diagnostics.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void DiagCommand::execute(const std::vector<std::string>& args)
{
    if (args.size() >= 2 && args[1] == "reset")
    {
        Diagnostics::resetCounters();
        ChatUtils::displayClientMessage("§a[diag] §fСчётчики обнулены.");
        return;
    }

    Diagnostics::report(true);
}

std::vector<std::string> DiagCommand::getAliases() const
{
    return { "diagnostics" };
}

std::string DiagCommand::getDescription() const
{
    return "Shows which signatures, hooks and events actually work in this build";
}

std::string DiagCommand::getUsage() const
{
    return "Usage: .diag | .diag reset";
}
