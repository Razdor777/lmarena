#pragma once
#include <string>
#include <Windows.h>

class Logger {
public:
    static inline bool initialized = false;
    static inline HANDLE hConsole = nullptr;

    static void initialize();
    static void deinitialize();
    static bool hasConsole();

    static std::string getAnsiColor(float r, float g, float b);
    static std::string getAnsiColor(int r, int g, int b);

#define ANSI_COLOR_RESET "\033[0m"
#define CC Logger::getAnsiColor
};