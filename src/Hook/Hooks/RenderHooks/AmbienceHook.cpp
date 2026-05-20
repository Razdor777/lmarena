//
// AmbienceHook Implementation
//

#include "AmbienceHook.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Ambience.hpp>
#include <SDK/SigManager.hpp>

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

// ============ SKY RENDER HOOK ============
__int64 AmbienceHook::onSkyRender(__int64 a1, uint64_t* a2, uint64_t* a3) {
    auto original = mSkyRenderDetour->getOriginal<&AmbienceHook::onSkyRender>();

    auto mod = getAmbienceModule();
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

    mSkyRenderDetour = std::make_unique<Detour>(
        "SkyRender",
        reinterpret_cast<void*>(getRVA(AmbienceAddresses::SkyRender)),
        &AmbienceHook::onSkyRender
    );

    mEndSkyRenderDetour = std::make_unique<Detour>(
        "EndSkyRender",
        reinterpret_cast<void*>(getRVA(AmbienceAddresses::EndSkyRender)),
        &AmbienceHook::onEndSkyRender
    );

    mChunkRenderDetour = std::make_unique<Detour>(
        "ChunkRender",
        reinterpret_cast<void*>(getRVA(AmbienceAddresses::ChunkRender)),
        &AmbienceHook::onChunkRender
    );

    mSunMoonRenderDetour = std::make_unique<Detour>(
        "SunMoonRender",
        reinterpret_cast<void*>(getRVA(AmbienceAddresses::SunMoonRender)),
        &AmbienceHook::onSunMoonRender
    );

    mCloudRenderDetour = std::make_unique<Detour>(
        "CloudRender",
        reinterpret_cast<void*>(getRVA(AmbienceAddresses::CloudRender)),
        &AmbienceHook::onCloudRender
    );

    // Enable all detours
    mSkyRenderDetour->enable();
    mEndSkyRenderDetour->enable();
    mChunkRenderDetour->enable();
    mSunMoonRenderDetour->enable();
    mCloudRenderDetour->enable();

    spdlog::info("AmbienceHook initialized successfully!");
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