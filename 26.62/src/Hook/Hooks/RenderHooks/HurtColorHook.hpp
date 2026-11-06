#pragma once
#include <Hook/Hook.hpp>
//
// HurtColorHook — детур RenderController::getOverlayColor. Даёт HurtColorEvent,
// на котором висит HitColor (кастомный цвет вспышки урона).
//

class HurtColorHook : public Hook {
public:
    HurtColorHook() : Hook() {
        mName = "RenderController::getOverlayColor";
    }

    static std::unique_ptr<Detour> mDetour;

    // RCX = RenderController*, RDX = MCCColor* (out), R8 = что-то ещё.
    // Возвращает тот же MCCColor* — float[4] RGBA, который модули правят.
    static void* onGetOverlayColor(void* renderController, void* color, void* a3);

    void init() override;
};
