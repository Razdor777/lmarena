#pragma once
#include <string>
//
// Created by vastrakai on 6/25/2024.
//

class ChatUtils {
public:
    static void displayClientMessage(const std::string& msg);
    template<typename... Args>
    static void displayClientMessage(fmt::format_string<Args...> fmt, Args&&... args)
    {
        displayClientMessage(fmt::format(fmt, std::forward<Args>(args)...));
    }
};
