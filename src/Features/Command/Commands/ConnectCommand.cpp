//
// .connect <ip[:port]>
//

#include "ConnectCommand.hpp"

#include <Features/Modules/Misc/QuickConnect.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void ConnectCommand::execute(const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        ChatUtils::displayClientMessage("§c" + getUsage());
        return;
    }

    QuickConnect::join(args[1]);
}

std::vector<std::string> ConnectCommand::getAliases() const
{
    return { "join" };
}

std::string ConnectCommand::getDescription() const
{
    return "Connects to a server by ip:port (same as QuickConnect)";
}

std::string ConnectCommand::getUsage() const
{
    return "Usage: .connect <ip[:port]>   e.g. .connect 109.248.4.159:19133";
}
