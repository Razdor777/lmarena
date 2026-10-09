//
// Dexko — init / shutdown (адаптировано из Solstice.cpp).
//
// Отличия от родителя:
//   - нет Auth (авторизации) и проверки обновлений через OAuthUtils;
//   - брендинг Dexko (заголовок окна, лог, чат);
//   - DeviceSpoof принудительно включён всегда.
//

#include "Dexko.hpp"

#include <fstream>
#include <Features/FeatureManager.hpp>
#include <Features/Configs/ConfigManager.hpp>
#include <Features/Modules/Misc/DeviceSpoof.hpp>
#include <Hook/HookManager.hpp>
#include <Hook/Hooks/RenderHooks/D3DHook.hpp>

#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>

#include <Utils/GameUtils/ChatUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftGame.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

#include "spdlog/sinks/stdout_color_sinks-inl.h"
#include <winrt/base.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <build_info.h>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/SysUtils/xorstr.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

#ifndef NOW
#define NOW std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()
#endif

#ifdef __DEBUG__
std::string title = "[dexko-" + std::string(DEXKO_BUILD_VERSION_SHORT) + "-" + std::string(DEXKO_BUILD_BRANCH) + "] [debug]";
#else
std::string title = "[dexko-" + std::string(DEXKO_BUILD_VERSION_SHORT) + "-" + std::string(DEXKO_BUILD_BRANCH) + "]";
#endif

void setTitle(std::string title)
{
    auto w = winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().CoreWindow().Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal, [title]() {
        winrt::Windows::UI::ViewManagement::ApplicationView::GetForCurrentView().Title(winrt::to_hstring(title));
    });
}

std::vector<unsigned char> gBpBytes = {0x1c}; // Defines the new offset for mInHandSlot
DEFINE_PATCH_FUNC(patchInHandSlot, SigManager::ItemInHandRenderer_renderItem_bytepatch2+2, gBpBytes);

// called using winrt::Windows::ApplicationModel::Core::CoreApplication::MainView().CoreWindow().Dispatcher().RunAsync(...)
void Dexko::init(HMODULE hModule)
{
    // Not doing this could cause crashes if you inject too soon
    while (ProcUtils::getModuleCount() < 130) std::this_thread::sleep_for(std::chrono::milliseconds(1));

    int64_t start = NOW;

    mModule = hModule;
    mInitialized = true;

#ifdef __DEBUG__
    Logger::initialize();
#endif

    console = spdlog::stdout_color_mt(CC(21, 207, 148) + "dexko" + ANSI_COLOR_RESET, spdlog::color_mode::automatic);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[" + CC(255, 135, 0) + "%H:%M:%S.%e" + ANSI_COLOR_RESET + "] [%n] [%^%l%$] %v");
    console_sink->set_level(spdlog::level::trace);
    console->set_level(spdlog::level::trace);
    console->set_pattern("[" + CC(255, 135, 0) + "%H:%M:%S.%e" + ANSI_COLOR_RESET + "] [%n] [%^%l%$] %v");
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(CC(21, 207, 148) + "dexko" + ANSI_COLOR_RESET, spdlog::sinks_init_list{console_sink}));

    console->info("Welcome to " + CC(0, 255, 0) + "Dexko" + ANSI_COLOR_RESET + "!"
#ifdef __DEBUG__
        + CC(255, 0, 0) + " [Debug] " + ANSI_COLOR_RESET
#endif
);

    spdlog::info("Minecraft version: {}", ProcUtils::getVersion());

    ExceptionHandler::init();

    FileUtils::validateDirectories();

    setTitle(title);

    sHWID = GET_HWID().toString();
    spdlog::info("HWID: {}", sHWID);

    if (MH_Initialize() != MH_OK)
    {
        console->critical("Failed to initialize MinHook!");
    }

    Prefs = PreferenceManager::load();

#ifdef __DEBUG__
    if (Prefs->mEnforceDebugging)
    {
        while (!IsDebuggerPresent())
        {
            MessageBoxA(nullptr, "Please attach a debugger to continue.\nThis message is being shown because of your preference settings.", "Dexko", MB_OK | MB_ICONERROR);
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

    if (failedSigs > 0)
    {
        console->critical("Failed to find {} signatures/offsets!", failedSigs);

#ifdef __DEBUG__
        {
            console->critical("Dexko should not be used in this state.");
            console->info("Type 'DEBUG' to continue, or press ENTER to exit.");
            std::string input;
            std::getline(std::cin, input);
            if (input != "DEBUG")
            {
                SigManager::deinitialize();
                OffsetProvider::deinitialize();
                Logger::deinitialize();

                setTitle("");
                FreeLibraryAndExitThread(hModule, 0);
            }
        }
#else
        {
            ExceptionHandler::makeCrashLog("Failed to find signatures/offsets!", 0xFF01);
            int* p = nullptr;
            *p = 0;
            exit(0);
        }
#endif
    }

    console->info("initialized signatures in {}ms", send - sstart);

    gFeatureManager = std::make_shared<FeatureManager>();
    gFeatureManager->init();

    console->info("initializing hooks...");
    HookManager::init(false);

    console->info("initialized in {}ms", NOW - start);

    ClientInstance::get()->getMinecraftGame()->playUi("beacon.activate", 1, 1.0f);
    ChatUtils::displayClientMessage("§adexko§7 » §aInitialized!");

    console->info("Press END to eject dll.");
    mLastTick = NOW;

    mThread = std::thread(&Dexko::shutdownThread);
    mThread.detach();
}

void Dexko::shutdownThread()
{
    bool firstCall = true;
    bool isLpValid = false;
    while (!mRequestEject)
    {
        if (firstCall)
        {
            NotifyUtils::notify("Dexko initialized!", 5.0f, Notification::Type::Info);
            firstCall = false;
        }

        if (!isLpValid && ClientInstance::get()->getLocalPlayer())
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

            // Device spoofing is mandatory for every injected session, even if
            // the loaded default config previously had it disabled.
            auto deviceSpoof = gFeatureManager->mModuleManager->getModule<DeviceSpoof>();
            if (deviceSpoof) deviceSpoof->setEnabled(true);
        }

        patchInHandSlot(ClientInstance::get()->getLocalPlayer() != nullptr);

        mLastTick = NOW;
        gFeatureManager->mModuleManager->onClientTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    mRequestEject = true;
    mUnloading.store(true);

    setTitle("");

    patchInHandSlot(false);

    HookManager::shutdown();

    // The hooks are gone, but the render thread may still be inside the frame it
    // started before that. Give it a few frames to get out before anything is freed.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    gFeatureManager->shutdown();

    console->warn("Shutting down...");

    if (auto* ci = ClientInstance::get())
    {
        if (auto* game = ci->getMinecraftGame())
            game->playUi("beacon.deactivate", 1, 1.0f);

        if (auto* guiData = ci->getGuiData())
            guiData->displayClientMessage("§adexko§7 » §cEjected!");
    }

    mInitialized = false;
    SigManager::deinitialize();
    OffsetProvider::deinitialize();

    Sleep(1000); // Give the user time to read the message

    Logger::deinitialize();

    // A dedicated thread owns the unload (see the long comment in Solstice.cpp):
    // it waits until this thread is definitely gone, then calls
    // FreeLibraryAndExitThread so we never return into unmapped code.
    HANDLE unloader = CreateThread(nullptr, 0, &Dexko::unloadThreadProc, mModule, 0, nullptr);
    if (unloader)
    {
        CloseHandle(unloader);
        ExitThread(0);
    }

    FreeLibraryAndExitThread(mModule, 0);
}

DWORD WINAPI Dexko::unloadThreadProc(LPVOID module)
{
    Sleep(250);

    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
    return 0;
}
