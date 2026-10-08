//
// AmbienceHook Implementation
//

#include "AmbienceHook.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Ambience.hpp>
#include <SDK/SigManager.hpp>

#include <atomic>
#include <cstdio>

// Define detour pointers
std::unique_ptr<Detour> AmbienceHook::mSkyRenderDetour;
std::unique_ptr<Detour> AmbienceHook::mEndSkyRenderDetour;
std::unique_ptr<Detour> AmbienceHook::mChunkRenderDetour;
std::unique_ptr<Detour> AmbienceHook::mSunMoonRenderDetour;
std::unique_ptr<Detour> AmbienceHook::mCloudRenderDetour;

// Signature addresses (you need to add these to SigManager.hpp)
// These are based on the reverse engineering we did earlier, показаты?
// UPDATE THESE FOR YOUR GAME VERSION!

namespace AmbienceAddresses {
    // Base address for calculation
    constexpr uintptr_t IMAGE_BASE = 0x140000000;

    // Function addresses from our reverse engineering
    constexpr uintptr_t SkyRender = 0x14438FD10;
    constexpr uintptr_t EndSkyRender = 0x14438B710;
    constexpr uintptr_t ChunkRender = 0x144384390;
    constexpr uintptr_t SunMoonRender = 0x1443913C0;
    constexpr uintptr_t CloudRender = 0x144386D70;
}

static Ambience* getAmbienceModule() {
    static Ambience* mod = nullptr;
    if (!mod && gFeatureManager && gFeatureManager->mModuleManager) {
        mod = gFeatureManager->mModuleManager->getModule<Ambience>();
    }
    return mod;
}

// Calculate RVA from absolute address
static uintptr_t getRVA(uintptr_t addr) {
    static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA("Minecraft.Windows.exe"));
    return base + (addr - AmbienceAddresses::IMAGE_BASE);
}

// ============ DIAGNOSTICS ============
//
// Небо / солнце-луна / облака могут «ничего не делать» по трём разным причинам, и
// различить их можно только по логу:
//   1) MH_CreateHook не встал — адрес из другого билда, функция-хук не вызовется
//      ни разу (в логе будет FAILED и первые байты по адресу);
//   2) адрес верный, но рендер в этом билде идёт мимо этой функции — вызовов нет
//      при успешном детуре;
//   3) функция вызывается, но структура передаётся другая — счётчик растёт,
//      картинка не меняется, и тогда нужен дамп, чтобы найти настоящие оффсеты.
//
// Поэтому здесь: счётчик вызовов, первый вызов в лог, и (по настройке Debug Log в
// модуле Ambience) разовый дамп начала структуры.
namespace AmbienceDiag
{
    struct Counter
    {
        std::atomic<uint32_t> calls{0};
        std::atomic<bool> dumped{false};
    };

    inline Counter sky, endSky, chunk, sunMoon, cloud;

    inline bool isInsideGame(uintptr_t addr, size_t size)
    {
        HMODULE mod = GetModuleHandleA("Minecraft.Windows.exe");
        if (!mod) return false;

        auto base = reinterpret_cast<uintptr_t>(mod);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(mod);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const uintptr_t end = base + nt->OptionalHeader.SizeOfImage;

        return addr >= base && addr + size <= end;
    }

    // 0x00 / 0xCC / 0x90 по адресу — это выравнивающий мусор, а не код.
    inline bool looksLikeCode(uintptr_t addr)
    {
        const uint8_t first = *reinterpret_cast<const uint8_t*>(addr);
        return first != 0x00 && first != 0xCC && first != 0x90;
    }

    inline void dumpFloats(const char* name, const void* ptr, size_t bytes)
    {
        if (!ptr) return;

        const auto* raw = static_cast<const uint8_t*>(ptr);
        spdlog::warn("AmbienceHook [{}] struct dump: {} bytes at {}", name, bytes, ptr);

        // Строки по 8 float: цвета и параметры видно сразу, по смещениям.
        for (size_t off = 0; off + 32 <= bytes; off += 32) {
            const float* f = reinterpret_cast<const float*>(raw + off);
            spdlog::warn("  +0x{:03X} f: {:.3f} {:.3f} {:.3f} {:.3f} | {:.3f} {:.3f} {:.3f} {:.3f}",
                         off, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
        }

        // Плюс сырые байты начала: по ним видно флаги и указатели.
        for (size_t off = 0; off + 16 <= bytes && off < 0x80; off += 16) {
            spdlog::warn("  +0x{:03X} b: {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}  {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}",
                         off,
                         raw[off + 0], raw[off + 1], raw[off + 2], raw[off + 3],
                         raw[off + 4], raw[off + 5], raw[off + 6], raw[off + 7],
                         raw[off + 8], raw[off + 9], raw[off + 10], raw[off + 11],
                         raw[off + 12], raw[off + 13], raw[off + 14], raw[off + 15]);
        }
    }

    inline void trace(const std::string& name, Counter& counter, const void* ptr, Ambience* mod, size_t dumpBytes = 0x180)
    {
        const uint32_t n = counter.calls.fetch_add(1) + 1;

        // Первый вызов, затем редкие отметки — чтобы не залить лог.
        if (n == 1 || n == 60 || (n % 600) == 0) {
            spdlog::info("AmbienceHook [{}] call #{} struct={}", name, n, ptr);
        }

        if (n == 1 && mod && mod->isDebugLog() && ptr) {
            bool expected = false;
            if (counter.dumped.compare_exchange_strong(expected, true)) {
                dumpFloats(name.c_str(), ptr, dumpBytes);
            }
        }
    }

    // Ставит детур только если адрес правда похож на код внутри игры. Иначе
    // MinHook подменял бы пролог в мусоре, и модуль молча ничего не делал.
    inline bool prepare(const char* name, uintptr_t absolute, void* detour, std::unique_ptr<Detour>& slot)
    {
        slot.reset();

        const uintptr_t rva = absolute - AmbienceAddresses::IMAGE_BASE;

        if (!isInsideGame(absolute, 16)) {
            spdlog::critical("AmbienceHook target {}: 0x{:X} (RVA 0x{:X}) is OUTSIDE Minecraft.Windows.exe — detour skipped",
                             name, absolute, rva);
            return false;
        }

        char hex[64] = {};
        const auto* p = reinterpret_cast<const uint8_t*>(absolute);
        for (int i = 0; i < 16; ++i) snprintf(hex + i * 3, 4, "%02X ", p[i]);

        spdlog::warn("AmbienceHook target {}: 0x{:X} (RVA 0x{:X}) first bytes: {}", name, absolute, rva, hex);

        if (!looksLikeCode(absolute)) {
            spdlog::critical("AmbienceHook target {}: 0x{:X} looks like padding (first byte 0x{:02X}) — detour skipped",
                             name, absolute, p[0]);
            return false;
        }

        slot = std::make_unique<Detour>(name, reinterpret_cast<void*>(absolute), detour);

        // mOriginalFunc остаётся null, если MH_CreateHook не сработал.
        if (slot->mOriginalFunc == nullptr) {
            slot.reset();
            return false;
        }

        return true;
    }
}

// ============ SKY RENDER HOOK ============
__int64 AmbienceHook::onSkyRender(__int64 a1, uint64_t* a2, uint64_t* a3) {
    auto original = mSkyRenderDetour->getOriginal<&AmbienceHook::onSkyRender>();

    auto mod = getAmbienceModule();
    AmbienceDiag::trace("onSkyRender", AmbienceDiag::sky, a3, mod);
    if (mod && mod->shouldModifySky() && a3) {
        // Sky color at offset +8 (a3 + 1)
        float* skyColor = reinterpret_cast<float*>(a3 + 1);
        float* modSkyColor = mod->getSkyColor();
        skyColor[0] = modSkyColor[0];
        skyColor[1] = modSkyColor[1];
        skyColor[2] = modSkyColor[2];
        skyColor[3] = modSkyColor[3];

        // Fog color at offset +24 (a3 + 3)
        if (mod->shouldModifyFog()) {
            float* fogColor = reinterpret_cast<float*>(a3 + 3);
            float* modFogColor = mod->getFogColor();
            fogColor[0] = modFogColor[0];
            fogColor[1] = modFogColor[1];
            fogColor[2] = modFogColor[2];
            fogColor[3] = modFogColor[3];
        }
    }

    return original(a1, a2, a3);
}

// ============ END SKY RENDER HOOK ============
__int64 AmbienceHook::onEndSkyRender(__int64 a1, uint64_t* a2, uint64_t* a3) {
    auto original = mEndSkyRenderDetour->getOriginal<&AmbienceHook::onEndSkyRender>();

    auto mod = getAmbienceModule();
    AmbienceDiag::trace("onEndSkyRender", AmbienceDiag::endSky, a3, mod);
    if (mod && mod->shouldModifyEndSky() && a3) {
        // End sky color at offset +36
        float* skyColor = reinterpret_cast<float*>(reinterpret_cast<char*>(a3) + 36);
        float* modColor = mod->getEndSkyColor();
        skyColor[0] = modColor[0];
        skyColor[1] = modColor[1];
        skyColor[2] = modColor[2];
        skyColor[3] = modColor[3];
    }

    return original(a1, a2, a3);
}

// ============ CHUNK RENDER HOOK (FOG) ============
__int64 AmbienceHook::onChunkRender(__int64* a1, __int64 a2, __int64 a3) {
    auto original = mChunkRenderDetour->getOriginal<&AmbienceHook::onChunkRender>();

    auto mod = getAmbienceModule();
    AmbienceDiag::trace("onChunkRender", AmbienceDiag::chunk, reinterpret_cast<const void*>(a3), mod);
    if (mod && mod->shouldModifyFog() && a3) {
        if (mod->shouldDisableFog()) {
            // Disable fog by setting huge distance
            float* fogControl = reinterpret_cast<float*>(a3 + 120);
            fogControl[0] = 10000.0f;  // fogStart
            fogControl[1] = 10001.0f;  // fogEnd
            fogControl[2] = 0.0f;      // density
            fogControl[3] = 0.0f;      // height
        } else {
            // Custom fog color at offset +104
            float* fogParams = reinterpret_cast<float*>(a3 + 104);
            float* modFogColor = mod->getFogColor();
            fogParams[0] = modFogColor[0];
            fogParams[1] = modFogColor[1];
            fogParams[2] = modFogColor[2];
            fogParams[3] = modFogColor[3];

            // Custom fog distance at offset +120
            float* fogControl = reinterpret_cast<float*>(a3 + 120);
            fogControl[0] = mod->getFogStart();
            fogControl[1] = mod->getFogEnd();
            fogControl[2] = mod->getFogDensity();
            // fogControl[3] = height (keep original)
        }
    }

    return original(a1, a2, a3);
}

// ============ SUN/MOON RENDER HOOK ============
__int64 AmbienceHook::onSunMoonRender(__int64 a1, __int64 a2, uint64_t* a3) {
    auto original = mSunMoonRenderDetour->getOriginal<&AmbienceHook::onSunMoonRender>();

    auto mod = getAmbienceModule();
    AmbienceDiag::trace("onSunMoonRender", AmbienceDiag::sunMoon, a3, mod);
    if (mod && mod->shouldModifySunMoon() && a3) {
        // Sun/Moon color at offset +32
        float* sunMoonColor = reinterpret_cast<float*>(reinterpret_cast<char*>(a3) + 32);

        // Check if it's moon (flag at offset +114)
        bool isMoon = *reinterpret_cast<char*>(reinterpret_cast<char*>(a3) + 114) != 0;

        float* modColor = isMoon ? mod->getMoonColor() : mod->getSunColor();
        sunMoonColor[0] = modColor[0];
        sunMoonColor[1] = modColor[1];
        sunMoonColor[2] = modColor[2];
        sunMoonColor[3] = modColor[3];
    }

    return original(a1, a2, a3);
}

// ============ CLOUD RENDER HOOK ============
char AmbienceHook::onCloudRender(__int64 a1, uint64_t* a2, __int64 a3) {
    auto original = mCloudRenderDetour->getOriginal<&AmbienceHook::onCloudRender>();

    auto mod = getAmbienceModule();
    AmbienceDiag::trace("onCloudRender", AmbienceDiag::cloud, reinterpret_cast<const void*>(a3), mod);
    if (mod && mod->shouldModifyClouds()) {
        // Skip cloud render if disabled
        if (mod->shouldDisableClouds()) {
            return 0;
        }

        if (a3) {
            // Cloud color at offset +72
            float* cloudColor = reinterpret_cast<float*>(a3 + 72);
            float* modColor = mod->getCloudColor();
            cloudColor[0] = modColor[0];
            cloudColor[1] = modColor[1];
            cloudColor[2] = modColor[2];
            cloudColor[3] = modColor[3];

            // Cloud distance at offset +92
            float* distControl = reinterpret_cast<float*>(a3 + 92);
            *distControl = mod->getCloudDistance();
        }
    }

    return original(a1, a2, a3);
}

// ============ INITIALIZATION ============
void AmbienceHook::init() {
    // Create detours for each render function
    // NOTE: You may need to add signatures to SigManager.hpp instead of using hardcoded addresses
    // Адреса ниже — под один конкретный билд; prepare() отсекает явно неверные
    // (вне модуля / выравнивающий мусор) и печатает первые байты для сигнатур.

    const bool skyOk = AmbienceDiag::prepare("SkyRender", getRVA(AmbienceAddresses::SkyRender), &AmbienceHook::onSkyRender, mSkyRenderDetour);
    const bool endSkyOk = AmbienceDiag::prepare("EndSkyRender", getRVA(AmbienceAddresses::EndSkyRender), &AmbienceHook::onEndSkyRender, mEndSkyRenderDetour);
    const bool chunkOk = AmbienceDiag::prepare("ChunkRender", getRVA(AmbienceAddresses::ChunkRender), &AmbienceHook::onChunkRender, mChunkRenderDetour);
    const bool sunMoonOk = AmbienceDiag::prepare("SunMoonRender", getRVA(AmbienceAddresses::SunMoonRender), &AmbienceHook::onSunMoonRender, mSunMoonRenderDetour);
    const bool cloudOk = AmbienceDiag::prepare("CloudRender", getRVA(AmbienceAddresses::CloudRender), &AmbienceHook::onCloudRender, mCloudRenderDetour);

    // Enable detours that actually installed
    if (mSkyRenderDetour) mSkyRenderDetour->enable();
    if (mEndSkyRenderDetour) mEndSkyRenderDetour->enable();
    if (mChunkRenderDetour) mChunkRenderDetour->enable();
    if (mSunMoonRenderDetour) mSunMoonRenderDetour->enable();
    if (mCloudRenderDetour) mCloudRenderDetour->enable();

    spdlog::warn("AmbienceHook summary: SkyRender={} EndSkyRender={} ChunkRender={} SunMoonRender={} CloudRender={} (FAILED = hook not installed, function will never fire)",
                 skyOk ? "OK" : "FAILED",
                 endSkyOk ? "OK" : "FAILED",
                 chunkOk ? "OK" : "FAILED",
                 sunMoonOk ? "OK" : "FAILED",
                 cloudOk ? "OK" : "FAILED");
}

void AmbienceHook::shutdown() {
    // Restore all detours
    if (mSkyRenderDetour) mSkyRenderDetour->restore();
    if (mEndSkyRenderDetour) mEndSkyRenderDetour->restore();
    if (mChunkRenderDetour) mChunkRenderDetour->restore();
    if (mSunMoonRenderDetour) mSunMoonRenderDetour->restore();
    if (mCloudRenderDetour) mCloudRenderDetour->restore();

    spdlog::info("AmbienceHook shutdown complete");
}