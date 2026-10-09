//
// Created by vastrakai on 6/25/2024.
//

#include "ChatUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

namespace
{
    constexpr const char* kPrefix = "§aDexko§7 » §r";
}

void ChatUtils::displayClientMessage(const std::string& msg)
{
    // Every line of a multi-line message gets the prefix.
    std::string formattedMsg = kPrefix;
    for (const auto& c : msg)
    {
        if (c == '\n')
            formattedMsg += std::string("\n") + kPrefix;
        else
            formattedMsg += c;
    }
    ClientInstance::get()->getGuiData()->displayClientMessageQueued(formattedMsg);
}
