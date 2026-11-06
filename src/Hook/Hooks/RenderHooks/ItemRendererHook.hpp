#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Hook/Hook.hpp"
#include "Hook/Detour.hpp"

class ItemStack;
class ItemRenderer;
class BaseActorRenderContext;

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

class ItemRendererHook : public Hook
{
public:
    static inline std::unique_ptr<Detour> mItemRendererDetour = nullptr;
    static inline ItemRenderer*           sItemRenderer = nullptr;
    static inline BaseActorRenderContext* sBarContext   = nullptr;

    static ItemRenderer*           getItemRenderer() { return sItemRenderer; }
    static BaseActorRenderContext* getBarContext()   { return sBarContext; }

    ItemRendererHook() { mName = "ItemRendererHook"; }

    void init() override;

    static void* onRender(ItemRenderer* _this, BaseActorRenderContext* ctx,
                           void* a3, void* a4, void* a5, void* a6);
};
