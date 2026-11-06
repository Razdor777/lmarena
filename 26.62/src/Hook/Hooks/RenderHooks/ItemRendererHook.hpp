#pragma once
//
// Created by jcazm on 7/27/2024.
//
#include <Hook/Hook.hpp>

class ItemRenderer;
class BaseActorRenderContext;

class ItemRendererHook : public Hook {
public:
    ItemRendererHook() : Hook() {
        mName = "ItemRenderer::render";
    }

    static std::unique_ptr<Detour> mDetour;

    // ==== ported from the 1.21.44 master ====
    // Последний ItemRenderer и BaseActorRenderContext, которые игра отдала в
    // render(). По ним InventoryHUD/ItemAtlas рисуют иконки предметов, когда
    // другого доступа к рендереру нет.
    static inline ItemRenderer*           sItemRenderer = nullptr;
    static inline BaseActorRenderContext* sBarContext   = nullptr;

    static ItemRenderer*           getItemRenderer() { return sItemRenderer; }
    static BaseActorRenderContext* getBarContext()   { return sBarContext; }
    // ==== end of ported block ====

    // symbol: void ItemRenderer::render(ItemRenderer* _this, BaseActorRenderContext* renderContext, ActorRenderData* actorRenderData)
    static void render(class ItemRenderer* _this, class BaseActorRenderContext* renderContext, struct ActorRenderData* actorRenderData);
    void init() override;
};
