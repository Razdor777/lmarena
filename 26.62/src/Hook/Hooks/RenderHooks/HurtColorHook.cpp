//
// HurtColorHook Implementation
// Hooks RenderController::getOverlayColor to fire HurtColorEvent
//

#include "HurtColorHook.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Events/HurtColorEvent.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> HurtColorHook::mDetour;

void* HurtColorHook::onGetOverlayColor(void* a1, void* color, void* a3) {
    auto original = mDetour->getOriginal<&HurtColorHook::onGetOverlayColor>();

    // Call original to get the default hurt color
    void* result = original(a1, color, a3);

    // result is MCCColor* (same as color param) — float[4] RGBA
    float* colorData = reinterpret_cast<float*>(result);

    // Fire event so modules can modify the color
    auto holder = nes::make_holder<HurtColorEvent>(colorData);
    gFeatureManager->mDispatcher->trigger(holder);

    return result;
}

void HurtColorHook::init() {
    uintptr_t addr = SigManager::HurtColor;
    if (addr == 0) {
        spdlog::warn("[HurtColorHook] Signature not found! Hook disabled.");
        return;
    }

    mDetour = std::make_unique<Detour>(
        "HurtColor",
        reinterpret_cast<void*>(addr),
        &HurtColorHook::onGetOverlayColor
    );
    mDetour->enable();

    spdlog::info("[HurtColorHook] Initialized at 0x{:X}", addr);
}

void HurtColorHook::shutdown() {
    if (mDetour) mDetour->restore();
    spdlog::info("[HurtColorHook] Shutdown");
}
