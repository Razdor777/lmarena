#pragma once

#include <iostream>
#include <Solstice.hpp>
#include <thread>
#include <Windows.h>
#include <Utils/Logger.hpp>
#include <winrt/base.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>

bool __stdcall DllMain(const HMODULE hModule, const DWORD fdwReason, [[maybe_unused]] LPVOID lpReserved) {
    if(fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        // GDK/UWP-вариант (через CoreApplication::MainView) не используется:
        // вызов инициализации делался бы в обход CreateThread и разбирался бы
        // отдельно. Точка входа одна — Solstice::init.
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Solstice::init, hModule, NULL, 0);
    }
    return true;  // Successful DLL_PROCESS_ATTACH
}
