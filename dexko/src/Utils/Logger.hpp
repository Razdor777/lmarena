#pragma once
#include <string>

class Logger {
public:
    static inline bool initialized;
    static void initialize();
    static void deinitialize();

    static std::string getAnsiColor(float r, float g, float b);
    static std::string getAnsiColor(int r, int g, int b);

#define ANSI_COLOR_RESET "\033[0m"
#define CC Logger::getAnsiColor
};