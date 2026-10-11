#pragma once

class ExceptionHandler {
public:
    static void init();
    static void makeCrashLog(const std::string& text, DWORD exceptionCode);
};