#pragma once
// ============================================================================
//  Dexko — главный класс клиента.
//
//  Движковые части родительского репо (SigManager, хуки и т.д.) ссылаются на
//  класс с именем Solstice. dexko/src/Solstice.hpp делает `using Solstice = Dexko`,
//  поэтому весь движок работает с этим классом без единой правки.
// ============================================================================

#include <Windows.h>
#include <atomic>
#include <Features/Configs/PreferenceManager.hpp>
#include "spdlog/logger.h"

class Dexko {
public:
    /* Fields */
    static inline HMODULE mModule;
    static inline bool mInitialized = false;
    static inline bool mRequestEject = false;
    // Set the moment an eject starts. The render hook checks it first and hands
    // the frame straight back to the game, so no thread can still be executing
    // our code while the module is being unloaded.
    static inline std::atomic<bool> mUnloading = false;
    static inline int64_t mLastTick = 0;
    static inline std::shared_ptr<spdlog::logger> console;
    static inline std::shared_ptr<Preferences> Prefs;
    static inline std::string sHWID;
    static inline std::thread mThread;

    /* Functions */
    static void init(HMODULE hModule);
    static void shutdownThread();
    static DWORD WINAPI unloadThreadProc(LPVOID module);
};
