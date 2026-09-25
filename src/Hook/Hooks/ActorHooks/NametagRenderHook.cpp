//
// Created by vastrakai on 11/10/2024.
//

#include "NametagRenderHook.hpp"
#include <Features/Events/NametagRenderEvent.hpp>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] РЕВЕРС (в 1.26 нет): Unknown::renderNametag -> NameTagRenderObject / NameTagRenderer  (src-client/mc/deps/minecraft_renderer/objects/NameTagRenderObject.h, src-client/mc/client/gui/controls/renderers/NameTagRenderer.h) — в 1.26 неймтеги — объекты рендера, отдельной функции нет

std::unique_ptr<Detour> NametagRenderHook::mRenderDetour;

void* NametagRenderHook::render(void* a1, void* a2, void* a3, void* a4, Actor* actor, void* a6, glm::vec3* pos, bool unknown,
    float deltaThing, mce::Color* color)
{
    auto original = mRenderDetour->getOriginal<&NametagRenderHook::render>();

    glm::vec3 posCopy = *pos;

    auto holder = nes::make_holder<NametagRenderEvent>(actor, pos, unknown, deltaThing, color);
    gFeatureManager->mDispatcher->trigger(holder);


    auto result = original(a1, a2, a3, a4, actor, a6, pos, unknown, deltaThing, color);
    if (holder->mCancelled) *pos = posCopy;

    return result;
}

void NametagRenderHook::init()
{
    mRenderDetour = std::make_unique<Detour>("Unknown::renderNametag", reinterpret_cast<void*>(SigManager::Unknown_renderNametag), &NametagRenderHook::render);
}
