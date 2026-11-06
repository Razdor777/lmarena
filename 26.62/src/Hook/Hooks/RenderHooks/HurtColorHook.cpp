//
// HurtColorHook — источник HurtColorEvent в 26.62.
//
// В отличие от старой сборки адрес берётся из SigManager::HurtColor (RefSig на
// вызов, а не на саму функцию). Сигнатуры под 26.62 пока нет: пока она не
// снята, детур не ставится и HitColor просто не перекрашивает вспышку.
//

#include "HurtColorHook.hpp"

#include <Features/Events/HurtColorEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> HurtColorHook::mDetour;

void* HurtColorHook::onGetOverlayColor(void* renderController, void* color, void* a3) {
    auto original = mDetour->getOriginal<&HurtColorHook::onGetOverlayColor>();

    // Сначала зовём оригинал — он и заполняет цвет.
    void* result = original(renderController, color, a3);

    // result — MCCColor* (тот же, что color): float[4] RGBA.
    float* colorData = reinterpret_cast<float*>(result);

    if (colorData) {
        auto holder = nes::make_holder<HurtColorEvent>(colorData);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return result;
}

void HurtColorHook::init() {
    const uintptr_t addr = SigManager::HurtColor;
    if (addr == 0) {
        spdlog::warn("[HitColor] RenderController::getOverlayColor signature not resolved - hurt color hook disabled");
        return;
    }

    mDetour = std::make_unique<Detour>("RenderController::getOverlayColor",
                                       reinterpret_cast<void*>(addr),
                                       &HurtColorHook::onGetOverlayColor);
    mDetour->enable();

    spdlog::info("[HitColor] overlay-color hook installed at 0x{:X}", addr);
}
