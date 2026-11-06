//
// Created by vastrakai on 6/24/2024.
//

#include "Solstice.hpp"

#include <fstream>
#include <Features/FeatureManager.hpp>
#include <Features/Configs/ConfigManager.hpp>
#include <Features/Modules/Misc/DeviceSpoof.hpp>
#include <Features/IRC/IrcClient.hpp>
#include <Features/Modules/Misc/IRC.hpp>
#include <Hook/HookManager.hpp>
#include <Hook/Hooks/RenderHooks/D3DHook.hpp>

#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftGame.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

#include "spdlog/sinks/stdout_color_sinks-inl.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <winrt/base.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <build_info.h>
#include <Utils/OAuthUtils.hpp>
#include <Utils/SysUtils/xorstr.hpp>

#include <wininet.h>
#pragma comment(lib, "wininet.lib")

#ifdef __DEBUG__
std::string title = "[Juzdex] [" + std::string(SOLSTICE_BUILD_VERSION_SHORT) + "-" + std::string(SOLSTICE_BUILD_BRANCH) + "] [debug]";
#elif __PRIVATE_BUILD__
std::string title = "[Juzdex] [" + std::string(SOLSTICE_BUILD_VERSION_SHORT) + "-" + std::string(SOLSTICE_BUILD_BRANCH) + "] [private]";
#else
std::string title = "[Juzdex] [" + std::string(SOLSTICE_BUILD_VERSION_SHORT) + "-" + std::string(SOLSTICE_BUILD_BRANCH) + "]";
#endif

void setTitle(std::string titleStr)
{
    if (!titleStr.empty())
    {
        titleStr += " - ";
    }
    titleStr += "Minecraft";

    if (!Solstice::GameWindow)
    {
        Solstice::GameWindow = ProcUtils::getMinecraftWindow();
    }

    if (Solstice::GameWindow)
    {
        int s = MultiByteToWideChar(CP_UTF8, 0, titleStr.c_str(), -1, nullptr, 0);
        std::wstring wtitle(s, 0);
        MultiByteToWideChar(CP_UTF8, 0, titleStr.c_str(), -1, wtitle.data(), s);
        SetWindowTextW(Solstice::GameWindow, wtitle.c_str());
    }
    Solstice::mReadyWindow = true;
}

std::vector<unsigned char> gBpBytes = {0x1c}; // 26.62 offset for mInHandSlot
DEFINE_PATCH_FUNC(patchInHandSlot, SigManager::ItemInHandRenderer_renderItem_bytepatch2 + 2, gBpBytes);

void Solstice::init(HMODULE hModule)
{
    while (ProcUtils::getModuleCount() < 130)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Solstice::GameWindow = FindWindowA(nullptr, "Minecraft");
    if (!Solstice::GameWindow)
    {
        Solstice::GameWindow = ProcUtils::getMinecraftWindow();
    }

    int64_t start = NOW;

    mModule = hModule;
    mInitialized = true;

#ifdef __DEBUG__
    Logger::initialize();
#endif

    console = spdlog::stdout_color_mt(CC(21, 207, 148) + "solstice" + ANSI_COLOR_RESET, spdlog::color_mode::automatic);

    std::string logFile = FileUtils::getSolsticeDir() + xorstr_("solstice.log");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[" + CC(255, 135, 0) + "%H:%M:%S.%e" + ANSI_COLOR_RESET + "] [%n] [%^%l%$] %v");
    console_sink->set_level(spdlog::level::trace);
    console->set_level(spdlog::level::trace);
    console->set_pattern("[" + CC(255, 135, 0) + "%H:%M:%S.%e" + ANSI_COLOR_RESET + "] [%n] [%^%l%$] %v");

    if (FileUtils::fileExists(logFile))
    {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
        file_sink->set_pattern("[%H:%M:%S.%e] [%n] [%l] %v");
        file_sink->set_level(spdlog::level::trace);
        spdlog::set_default_logger(std::make_shared<spdlog::logger>(CC(21, 207, 148) + "solstice" + ANSI_COLOR_RESET, spdlog::sinks_init_list{console_sink, file_sink}));
    }
    else
    {
        spdlog::set_default_logger(std::make_shared<spdlog::logger>(CC(21, 207, 148) + "solstice" + ANSI_COLOR_RESET, spdlog::sinks_init_list{console_sink}));
    }

    console->info("Welcome to " + CC(0, 255, 0) + "Juzdex (Solstice 26.62)" + ANSI_COLOR_RESET + "!"
#ifdef __DEBUG__
        + CC(255, 0, 0) + " [Debug] " + ANSI_COLOR_RESET
#endif
    );

    spdlog::info("Minecraft version: {}", ProcUtils::getVersion());

    ExceptionHandler::init();
    FileUtils::validateDirectories();

    setTitle(title);

    if (MH_Initialize() != MH_OK)
    {
        console->critical("Failed to initialize MinHook!");
    }

    Prefs = PreferenceManager::load();

    HWND hwnd = ProcUtils::getMinecraftWindow();
    if (!Solstice::GameWindow && hwnd)
    {
        Solstice::GameWindow = hwnd;
    }

#ifdef __DEBUG__
    if (Prefs->mEnforceDebugging)
    {
        while (!IsDebuggerPresent())
        {
            MessageBoxA(nullptr, "Please attach a debugger to continue.\nThis message is being shown because of your preference settings.", "Solstice", MB_OK | MB_ICONERROR);
            spdlog::info("Waiting for debugger...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#endif

    console->info("initializing signatures...");
    int64_t sstart = NOW;
    OffsetProvider::initialize();
    SigManager::initialize();
    int64_t send = NOW;

    int failedSigs = 0;

    for (const auto& [sig, result] : SigManager::mSigs)
    {
        if (result == 0)
        {
            console->critical("[signatures] Failed to find signature: {}", sig);
            failedSigs++;
        }
    }

    for (const auto& [sig, result] : OffsetProvider::mSigs)
    {
        if (result == 0)
        {
            console->critical("[offsets] Failed to find offset: {}", sig);
            failedSigs++;
        }
    }

#ifdef __DEBUG__
    if (failedSigs > 0)
    {
        console->warn("{} signatures/offsets failed in Debug mode. Continuing...", failedSigs);
    }
#endif

    console->info("initialized signatures in {}ms", send - sstart);

    if (auto ci = ClientInstance::get())
    {
        console->info("clientinstance addr @ 0x{:X}", reinterpret_cast<uintptr_t>(ci));
        console->info("mcgame from clientinstance addr @ 0x{:X}", reinterpret_cast<uintptr_t>(ci->getMinecraftGame()));
        console->info("localplayer addr @ 0x{:X}", reinterpret_cast<uintptr_t>(ci->getLocalPlayer()));
    }

    gFeatureManager = std::make_shared<FeatureManager>();
    gFeatureManager->init();

    console->info("initializing hooks...");
    HookManager::init(false);

    console->info("initialized in {}ms", NOW - start);

    if (auto ci = ClientInstance::get())
    {
        if (auto mcGame = ci->getMinecraftGame())
            mcGame->playUi("beacon.activate", 1, 1.0f);
    }
    ChatUtils::displayClientMessage("Initialized!");

    console->info("Press END to eject dll.");
    mLastTick = NOW;

    mThread = std::thread(&Solstice::shutdownThread);
    mThread.detach();
}

void Solstice::shutdownThread()
{
    bool firstCall = true;
    bool isLpValid = false;
    while (!mRequestEject)
    {
        if (firstCall)
        {
            NotifyUtils::notify("Juzdex initialized!", 5.0f, Notification::Type::Info);
            firstCall = false;
        }

        if (!isLpValid && ClientInstance::get() && ClientInstance::get()->getLocalPlayer())
        {
            isLpValid = true;
            HookManager::init(true); // Initialize the base tick hook

            if (!Prefs->mDefaultConfigName.empty())
            {
                if (ConfigManager::configExists(Prefs->mDefaultConfigName))
                {
                    ConfigManager::loadConfig(Prefs->mDefaultConfigName);
                }
                else
                {
                    console->warn("Default config does not exist! Clearing default config...");
                    NotifyUtils::notify("Default config does not exist! Clearing default config...", 10.0f, Notification::Type::Warning);
                    Prefs->mDefaultConfigName = "";
                    PreferenceManager::save(Prefs);
                }
            }
            else
            {
                console->warn("No default config set!");
            }

            // Device spoofing is mandatory for every injected session
            auto deviceSpoof = gFeatureManager->mModuleManager->getModule<DeviceSpoof>();
            if (deviceSpoof) deviceSpoof->setEnabled(true);
        }

        patchInHandSlot(ClientInstance::get() && ClientInstance::get()->getLocalPlayer() != nullptr);

        mLastTick = NOW;
        gFeatureManager->mModuleManager->onClientTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    mRequestEject = true;
    mUnloading.store(true);

    setTitle("");

    patchInHandSlot(false);

    HookManager::shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    gFeatureManager->shutdown();

    console->warn("Shutting down...");

    if (auto* ci = ClientInstance::get())
    {
        if (auto* game = ci->getMinecraftGame())
            game->playUi("beacon.deactivate", 1, 1.0f);

        if (auto* guiData = ci->getGuiData())
            guiData->displayClientMessage("§asolstice§7 » §cEjected!");
    }

    mInitialized = false;
    SigManager::deinitialize();
    OffsetProvider::deinitialize();

    Sleep(1000);

    Logger::deinitialize();

    HANDLE unloader = CreateThread(nullptr, 0, &Solstice::unloadThreadProc, mModule, 0, nullptr);
    if (unloader)
    {
        CloseHandle(unloader);
        ExitThread(0);
    }

    FreeLibraryAndExitThread(mModule, 0);
}

DWORD WINAPI Solstice::unloadThreadProc(LPVOID module)
{
    Sleep(250);
    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
    return 0;
}
