#pragma once
//
// HurtColorHook - Hooks RenderController::getOverlayColor
// to allow custom hurt flash colors
//

#include <Hook/Hook.hpp>

class HurtColorHook : public Hook {
public:
    HurtColorHook() : Hook() {
        mName = "HurtColorHook";
    }

    static std::unique_ptr<Detour> mDetour;

    // RenderController::getOverlayColor returns MCCColor* (float[4])
    // Args based on Flarial HurtColorHook:
    //   a1: void* (this/context)
    //   color: MCCColor* (output color, float[4])
    //   a3: void* (actor context, contains hurtTime at +0x38 in 1.21.40+)
    static void* onGetOverlayColor(void* a1, void* color, void* a3);

    void init() override;
    void shutdown() override;
};
