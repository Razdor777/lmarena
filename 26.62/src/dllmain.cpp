#include <Windows.h>
#include <Dexko.hpp>

bool __stdcall DllMain(const HMODULE hModule, const DWORD fdwReason, [[maybe_unused]] LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Dexko::init, hModule, NULL, 0);
    }
    return true;  // Successful DLL_PROCESS_ATTACH
}
