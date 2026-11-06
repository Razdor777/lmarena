//
// Created by vastrakai on 6/24/2024.
//

#include <Solstice.hpp>
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Solstice::init), hinstDLL, 0, nullptr);
    }
    return TRUE;
}
