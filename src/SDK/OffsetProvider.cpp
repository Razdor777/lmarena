//
//

#include "OffsetProvider.hpp"

#include <Solstice.hpp>
#include <Utils/MemUtils.hpp>
#include <libhat.hpp>
#include <mutex>

#define NOW std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

namespace {
    std::mutex sOffsetMutex;
}

hat::scan_result OffsetProvider::scanSig(hat::signature_view sig, const std::string& name, int offset)
{
    auto minecraft = hat::process::get_process_module();
    auto result = hat::find_pattern(sig, ".text", minecraft);

    std::lock_guard<std::mutex> lock(sOffsetMutex);
    mSigScanCount++;

    if (!result.has_result()) {
        mSigs[name] = 0;
        return {};
    }

    mSigs[name] = reinterpret_cast<uintptr_t>(result.get()) + offset;
    return result;
}

void OffsetProvider::initialize()
{
    int64_t start = NOW;
    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(mSigInitializers.size()); i++)
    {
        mSigInitializers[i]();
    }
    uint64_t end = NOW;

    for (const auto& sig : mSigs)
    {
        if (sig.second != 0)
        {
            Solstice::console->info("[offsets] found {} @ {}", sig.first, MemUtils::getMbMemoryString(sig.second));
        }
    }

    for (const auto& sig : mSigs)
    {
        if (sig.second == 0)
        {
            Solstice::console->critical("[offsets] failed to find {}", sig.first);
        }
    }

#ifndef __DEBUG__
    auto mc = hat::process::get_process_module().address();
    auto mcSize = hat::process::get_process_module().get_module_data().size();

    for (const auto& sig : mSigs) {
        if (sig.second == 0) {
            ExceptionHandler::makeCrashLog("An error occurred while initializing offset: " + sig.first, 0xFF02);
            __fastfail(0);
            std::exit(0);
        }
        if (sig.second < mc || sig.second > mc + mcSize) {
            ExceptionHandler::makeCrashLog("An error occurred while initializing: Offset sig out of bounds (" + sig.first + ")!", 0xFF00);
            __fastfail(0);
            std::exit(0);
        }
    }
#endif

    Solstice::console->info("[offsets] initialized in {}ms, {} total sigs scanned", end - start, mSigScanCount);
    mIsInitialized = true;
}

void OffsetProvider::deinitialize()
{
    spdlog::info("[offsets] deinitializing...");

    mSigInitializers.clear();
    mSigs.clear();

    mIsInitialized = false;
}
