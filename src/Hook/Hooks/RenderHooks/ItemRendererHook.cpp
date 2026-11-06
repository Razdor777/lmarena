#include "ItemRendererHook.hpp"

#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>
#include <SDK/Minecraft/Rendering/BaseActorRenderContext.hpp>

void ItemRendererHook::init()
{
    mItemRendererDetour = std::make_unique<Detour>(
        "ItemRenderer::render",
        reinterpret_cast<void*>(SigManager::ItemRenderer_render),
        &ItemRendererHook::onRender);
    mItemRendererDetour->enable();
}

void* ItemRendererHook::onRender(ItemRenderer* _this, BaseActorRenderContext* ctx,
                                  void* a3, void* a4, void* a5, void* a6)
{
    auto original = mItemRendererDetour->getOriginal<&ItemRendererHook::onRender>();

    if (_this) sItemRenderer = _this;
    if (ctx)   sBarContext   = ctx;

    return original(_this, ctx, a3, a4, a5, a6);
}
