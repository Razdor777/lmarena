//
//

#include "ChatUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

void ChatUtils::displayClientMessage(const std::string& msg)
{
    if (!msg.contains("\n"))
    {
        ClientInstance::get()->getGuiData()->displayClientMessageQueued("§aDexko§7 » §r" + msg);
        return;
    }

    std::string formattedMsg = "§aDexko§7 » §r";
    for (const auto& c : msg)
    {
        if (c == '\n')
            formattedMsg += "\n§aDexko§7 » §r";
        else
            formattedMsg += c;
    }
    ClientInstance::get()->getGuiData()->displayClientMessageQueued(formattedMsg);
}
