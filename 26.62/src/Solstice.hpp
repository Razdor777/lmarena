#pragma once
#include <Windows.h>
#include <atomic>
#include <string>
#include <thread>
#include <memory>
#include <Features/Configs/PreferenceManager.hpp>
#include "spdlog/logger.h"
//
//


class Solstice {
public:
    /* Fields */
    static inline HMODULE mModule;
    static inline bool mInitialized = false;
    static inline bool mRequestEject = false;
    // Set the moment an eject starts. The render hook checks it first and hands the
    // frame straight back to the game, so no thread is inside our code while the
    // module is being unmapped.
    static inline std::atomic<bool> mUnloading = false;
    static inline int64_t mLastTick = 0;
    static inline std::shared_ptr<spdlog::logger> console;
    static inline std::shared_ptr<Preferences> Prefs;
    static inline std::string sHWID;
    static inline std::thread mThread;

    /* GAME FIELDS */ // GDK MOMENT
    static inline HWND GameWindow;
    static inline bool mReadyWindow = false; // taitor ukaeru yatu (gay)


    /* Functions */
    static void init(HMODULE hModule);
    static void shutdownThread();
    static DWORD WINAPI unloadThreadProc(LPVOID module);
};