#pragma once
//
// Created by jcazm on 7/27/2024.
//
#include <Hook/Hook.hpp>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class ItemStack;
class ItemRenderer;
class BaseActorRenderContext;
class ActorRenderData;

// Deferred GUI item icons: modules (e.g. InventoryHUD) queue what they want
// drawn, and SetupAndRenderHook flushes the queue once per frame through
// ItemRenderer::renderGuiItemNew. Ported from the 1.21.44 tree.
struct PendingItemIcon
{
    ItemStack* stack = nullptr;
    glm::vec2  pos   = glm::vec2(0.f);
    float      scale = 1.f;
};

namespace ItemIconQueue
{
    inline std::vector<PendingItemIcon>& pending() { static std::vector<PendingItemIcon> v; v.reserve(64); return v; }
    inline std::vector<PendingItemIcon>& drawing() { static std::vector<PendingItemIcon> v; v.reserve(64); return v; }
    inline void push(const PendingItemIcon& icon) { if (pending().size() >= 64) return; pending().push_back(icon); }
    inline void swapBuffers() { drawing().clear(); drawing().swap(pending()); }
}

class ItemRendererHook : public Hook {
public:
    ItemRendererHook() : Hook() {
        mName = "ItemRenderer::render";
    }

    static std::unique_ptr<Detour> mDetour;

    // Captured from ItemRenderer::render and reused for deferred icon drawing.
    static inline ItemRenderer*           sItemRenderer = nullptr;
    static inline BaseActorRenderContext* sBarContext   = nullptr;

    static ItemRenderer*           getItemRenderer() { return sItemRenderer; }
    static BaseActorRenderContext* getBarContext()   { return sBarContext; }
    // symbol: void ItemRenderer::render(ItemRenderer* _this, BaseActorRenderContext* renderContext, ActorRenderData* actorRenderData)
    static void render(class ItemRenderer* _this, class BaseActorRenderContext* renderContext, struct ActorRenderData* actorRenderData);
    void init() override;
};
