#include "Glint.hpp"
#include <SDK/SigManager.hpp>
#include <Features/Modules/ModuleManager.hpp>

struct RefCountBlock {
    void* vtable;
    uint32_t ref1;
    uint32_t ref2;
};

static uint8_t fake_glint_data[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static RefCountBlock fake_refcount = { nullptr, 9999, 9999 };

Glint::Glint() : ModuleBase("Glint", "Forces the enchantment glint on all items", ModuleCategory::Visual, 0, false) {
    instance = this;
    addSettings(&mRainbow, &mColor, &mAlpha);
}

__int64 __fastcall Glint::onGetGlintComponent(__int64 a1, __int64 a2, void* a3) {
    auto original = mDetour->getOriginal<&onGetGlintComponent>();
    if (!original) return 0;

    __int64 result = original(a1, a2, a3);

    if (Glint::instance && Glint::instance->mEnabled) {
        if (*(void**)a1 == nullptr) {
            *(void**)a1 = &fake_glint_data;
            *(RefCountBlock**)(a1 + 8) = &fake_refcount;
        }
    }

    return result;
}

void Glint::onEnable() {
    uintptr_t addr = SigManager::GlintComponentGetter;
    if (addr) {
        mDetour = std::make_unique<Detour>("GlintComponentGetter", reinterpret_cast<void*>(addr), reinterpret_cast<void*>(&onGetGlintComponent));
        mDetour->enable();
    }
    Module::onEnable();
}

void Glint::onDisable() {
    if (mDetour) {
        mDetour->restore();
        mDetour.reset();
    }
    Module::onDisable();
}