#pragma once
#include <Windows.h>
#include <atomic>
#include <Features/Configs/PreferenceManager.hpp>
#include "spdlog/logger.h"
//
// Единый класс клиента. Раньше в базе 26.62 одновременно жили два имени
// (Dexko и Solstice) и два одинаковых .cpp; теперь точка входа одна —
// Solstice. Имя цели сборки при этом осталось Dexko (Dexko.dll).
//


class Solstice {
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

    /* GAME FIELDS */ // GDK MOMENT
    static inline HWND GameWindow;
    static inline bool mReadyWindow = false;

    /* Functions */
    static void init(HMODULE hModule);
    static void shutdownThread();
    static DWORD WINAPI unloadThreadProc(LPVOID module);
};
