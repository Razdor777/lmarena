#pragma once
//
// Created by vastrakai on 6/24/2024.
//

#include <Windows.h>
#include <atomic>
#include <thread>
#include <Features/Configs/PreferenceManager.hpp>
#include "spdlog/logger.h"

class Solstice {
public:
    /* Fields */
    static inline HWND GameWindow = nullptr;
    static inline HMODULE mModule = nullptr;
    static inline bool mInitialized = false;
    static inline bool mRequestEject = false;
    static inline bool mReadyWindow = false;
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
