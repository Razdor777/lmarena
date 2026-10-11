#pragma once
#include <Hook/Hooks/ActorHooks/BaseTickHook.hpp>

class ProcUtils {
public:
    static int getModuleCount();
    static HWND getMinecraftWindow();
    static std::vector<std::wstring> getModulePaths();
    static std::string getVersion();
};