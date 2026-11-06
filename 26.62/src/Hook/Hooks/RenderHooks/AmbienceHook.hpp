#pragma once
//
// AmbienceHook - Hooks for custom sky, fog, clouds, sun/moon rendering
//

#include <Hook/Hook.hpp>

class AmbienceHook : public Hook {
public:
    AmbienceHook() : Hook() {
        mName = "AmbienceHook";
    }

    // Detours for each render function
    static std::unique_ptr<Detour> mSkyRenderDetour;
    static std::unique_ptr<Detour> mEndSkyRenderDetour;
    static std::unique_ptr<Detour> mChunkRenderDetour;
    static std::unique_ptr<Detour> mSunMoonRenderDetour;
    static std::unique_ptr<Detour> mCloudRenderDetour;

    // Hook functions
    static __int64 onSkyRender(__int64 a1, uint64_t* a2, uint64_t* a3);
    static __int64 onEndSkyRender(__int64 a1, uint64_t* a2, uint64_t* a3);
    static __int64 onChunkRender(__int64* a1, __int64 a2, __int64 a3);
    static __int64 onSunMoonRender(__int64 a1, __int64 a2, uint64_t* a3);
    static char onCloudRender(__int64 a1, uint64_t* a2, __int64 a3);

    void init() override;
    void shutdown() override;
};