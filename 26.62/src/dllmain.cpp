#pragma once

#include <iostream>
#include <Dexko.hpp>
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

        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Solstice::init, hModule, NULL, 0);
        /* // GDK Moment
        winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().CoreWindow().Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, [hModule]()
        {
<<<<<<< HEAD:26.62/src/dllmain.cpp
            Solstice::init(hModule);
        });*/
=======
            Dexko::init(hModule);
        });
>>>>>>> 0694274eb35c890fa59f1d8936575574a67bc5de:dexko/src/dllmain.cpp
    }
    return true;  // Successful DLL_PROCESS_ATTACH
}
