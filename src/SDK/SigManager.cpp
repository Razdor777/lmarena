//
// Created by vastrakai on 6/24/2024.
//

#include "SigManager.hpp"

#include <iostream>
#include <Solstice.hpp>
#include <Utils/Logger.hpp>
#include <Utils/MemUtils.hpp>
#include <chrono>
#include <omp.h>
#include <libhat.hpp>

#define NOW std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

hat::scan_result SigManager::scanSig(hat::signature_view sig, const std::string& name, int offset)
{
    mSigScanCount.fetch_add(1, std::memory_order_relaxed);

    auto minecraft = hat::process::get_process_module();
    auto result = hat::find_pattern(sig, ".text", minecraft);

    if (!result.has_result()) {
        mSigs[name] = 0;
        return {};
    }

    if (offset == 0) mSigs[name] = reinterpret_cast<uintptr_t>(result.get());
    else mSigs[name] = reinterpret_cast<uintptr_t>(result.rel(offset));

    return result;
}

void SigManager::initialize()
{
    int64_t start = NOW;
    #pragma omp parallel for
    for (int i = 0; i < (int)mSigInitializers.size(); i++) {
        mSigInitializers[i]();
    }

    // Pass 2: the primary patterns above are the ones verified on 1.21.44. Every
    // signature that also has a pattern recorded from the 26.62 build gets a
    // second chance here, but only if pass 1 left it unresolved. A binding that
    // already worked is never overwritten.
    int64_t pass2Start = NOW;
    #pragma omp parallel for
    for (int i = 0; i < (int)mFallbackInitializers.size(); i++) {
        if (mFallbackInitializers[i] == nullptr) continue;
        mFallbackInitializers[i]();
    }
    int64_t pass2End = NOW;

    // A name counts as "recovered" only when pass 1 missed it and the 26.62
    // pattern found it. Where pass 1 already succeeded the fallback is ignored.
    int recovered = 0;
    for (const auto& [key, altAddr] : mSigs) {
        auto sep = key.rfind(":alt");
        if (sep == std::string::npos) continue;
        std::string base = key.substr(0, sep);
        auto prim = mSigs.find(base);
        if (prim != mSigs.end() && prim->second == 0 && altAddr != 0) recovered++;
    }

    int64_t end = NOW;
    int64_t diff = end - start;

    for (const auto& sig : mSigs) {
        if (sig.first.find(":alt") != std::string::npos) continue;
        if (sig.second != 0) Solstice::console->info("[signatures] found {} @ {}", sig.first, MemUtils::getMbMemoryString(sig.second));
    }

    // signatures that only resolved through the 26.62 fallback pattern
    for (const auto& sig : mSigs) {
        if (sig.first.find(":alt") == std::string::npos || sig.second == 0) continue;
        std::string base = sig.first.substr(0, sig.first.find(":alt"));
        auto prim = mSigs.find(base);
        if (prim == mSigs.end() || prim->second == 0) {
            Solstice::console->warn("[signatures] {} resolved via 26.62 fallback only", base);
        }
    }

    for (const auto& sig : mSigs) {
        if (sig.second == 0 && sig.first.find(":alt") == std::string::npos) Solstice::console->critical("[signatures] failed to find {}", sig.first);
    }
    Solstice::console->info("[signatures] pass 2 (26.62 patterns) recovered {} of {} in {}ms",
                            recovered, (int)mFallbackInitializers.size(), pass2End - pass2Start);

#ifndef __DEBUG__
    auto mc = hat::process::get_process_module().address();
    auto mcSize = hat::process::get_process_module().get_module_data().size();

    // Terminate if we failed to find a signature.
    // ":alt" entries are bookkeeping for the pass-2 scan and are expected to be
    // zero whenever the primary pattern already resolved, so they are skipped.
    for (const auto& sig : mSigs) {
        if (sig.first.find(":alt") != std::string::npos) continue;
        // Make a crash log if the sig is out of bounds
        if (sig.second < mc || sig.second > mc + mcSize) {
            ExceptionHandler::makeCrashLog("An error occurred while initializing: Signature out of bounds!", 0xFF00);
            __fastfail(0);
            std::exit(0);
        }

        if (sig.second == 0) {
            ExceptionHandler::makeCrashLog("An error occurred while initializing!", 0xFF02);
            __fastfail(0);
            std::exit(0);
        }

    }
#endif

    Solstice::console->info("[signatures] initialized in {}ms, {} total sigs scanned", diff, mSigScanCount.load());
    mIsInitialized = true;
}

void SigManager::deinitialize()
{
    spdlog::info("[signatures] deinitializing...");
    mSigInitializers.clear();
    mSigs.clear();

    mIsInitialized = false;
}
