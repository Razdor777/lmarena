//
// Created by vastrakai on 6/24/2024.
//

#include "Logger.hpp"

#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <io.h>

void Logger::initialize()
{
    if (initialized)
        return;

#ifdef __DEBUG__
    if (!GetConsoleWindow())
    {
        if (!AllocConsole())
            return;
    }

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hConsole || hConsole == INVALID_HANDLE_VALUE)
        return;

    SetConsoleTitleA("Solstice Debug Console");

    FILE* fpOut = nullptr;
    FILE* fpErr = nullptr;
    FILE* fpIn  = nullptr;

    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpErr, "CONOUT$", "w", stderr);
    freopen_s(&fpIn,  "CONIN$",  "r", stdin);

    std::ios::sync_with_stdio(true);

    DWORD mode = 0;
    if (GetConsoleMode(hConsole, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }

    initialized = true;
#else
    initialized = true;
#endif
}

void Logger::deinitialize()
{
#ifdef __DEBUG__
    if (!initialized)
        return;

    fflush(stdout);
    fflush(stderr);

    fclose(stdout);
    fclose(stderr);
    fclose(stdin);

    FreeConsole();
#endif

    initialized = false;
    hConsole = nullptr;
}

bool Logger::hasConsole()
{
#ifdef __DEBUG__
    return GetConsoleWindow() != nullptr;
#else
    return false;
#endif
}

std::string Logger::getAnsiColor(float r, float g, float b)
{
    return "\033[38;2;" +
        std::to_string(static_cast<int>(r * 255)) + ";" +
        std::to_string(static_cast<int>(g * 255)) + ";" +
        std::to_string(static_cast<int>(b * 255)) + "m";
}

std::string Logger::getAnsiColor(int r, int g, int b)
{
    return "\033[38;2;" +
        std::to_string(r) + ";" +
        std::to_string(g) + ";" +
        std::to_string(b) + "m";
}