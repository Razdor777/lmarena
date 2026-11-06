#pragma once
#include <Windows.h>
#include <Features/Configs/PreferenceManager.hpp>
#include "spdlog/logger.h"
//
//


class Dexko {
public:
    /* Fields */
    static inline HMODULE mModule;
    static inline bool mInitialized = false;
    static inline bool mRequestEject = false;
    static inline int64_t mLastTick = 0;
    static inline std::shared_ptr<spdlog::logger> console;
    static inline std::shared_ptr<Preferences> Prefs;
    static inline std::thread mThread;

    /* GAME FIELDS */ // GDK MOMENT
    static inline HWND GameWindow;
    static inline bool mReadyWindow = false; // taitor ukaeru yatu (gay)


    /* Functions */
    static void init(HMODULE hModule);
    static void shutdownThread();
};