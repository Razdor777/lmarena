//
// Created by jcazm on 7/27/2024.
//

#include "ItemRendererHook.hpp"
#include <SDK/Minecraft/Actor/ActorRenderData.hpp>
#include <SDK/Minecraft/Rendering/BaseActorRenderContext.hpp>
#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>
#include <Features/Events/ItemRendererEvent.hpp>

std::unique_ptr<Detour> ItemRendererHook::mDetour;

void ItemRendererHook::render(ItemRenderer* _this, BaseActorRenderContext* renderContext, ActorRenderData* actorRenderData)
{
    auto original = mDetour->getOriginal<&ItemRendererHook::render>();

    // ==== ported from the 1.21.44 master ====
    // Кэшируем указатели: InventoryHUD и ItemAtlas берут их через геттеры.
    if (_this) sItemRenderer = _this;
    if (renderContext) sBarContext = renderContext;
    // ==== end of ported block ====

    auto holder = nes::make_holder<ItemRendererEvent>(_this, renderContext, actorRenderData);
    gFeatureManager->mDispatcher->trigger(holder);

    original(_this, renderContext, actorRenderData);
}

void ItemRendererHook::init()
{
    auto func = reinterpret_cast<void*>(SigManager::ItemRenderer_render);
    if (!func) return;

    // enable() вручную не нужен: HookManager::init() заканчивается
    // MH_EnableHook(MH_ALL_HOOKS), он и включает все созданные детуры.
    mDetour = std::make_unique<Detour>("ItemRenderer::render", func, &render);
}
