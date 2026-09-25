//
// Created by vastrakai on 7/7/2024.
//

#include "SetupAndRenderHook.hpp"

#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/LevelRenderer.hpp>
#include <Features/Events/DrawImageEvent.hpp>

#include "D3DHook.hpp"

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ОБНОВЛЕНО: MinecraftUIRenderContext::drawImage -> src-client/mc/client/renderer/screen/MinecraftUIRenderContext.h:299
// [1.26] ОБНОВЛЕНО: ScreenView::setupAndRender -> src-client/mc/client/gui/screens/ScreenView.h:319 — в 1.26 это ScreenView::render; LeviLamina уже хукает его и даёт Before/AfterUIRenderEvent (src-client/ll/api/event/render) | переименовано: render

std::unique_ptr<Detour> SetupAndRenderHook::mSetupAndRenderDetour;
std::unique_ptr<Detour> SetupAndRenderHook::mDrawImageDetour;

void* SetupAndRenderHook::onSetupAndRender(void* screenView, void* mcuirc)
{
    auto original = mSetupAndRenderDetour->getOriginal<&SetupAndRenderHook::onSetupAndRender>();

    static bool once = false;
    if (!once)
    {
        once = true;
        initVt(mcuirc);
    }

    auto ci = ClientInstance::get();
    if (!ci) return original(screenView, mcuirc);

    auto player = ClientInstance::get()->getLocalPlayer();

    glm::vec3 origin = glm::vec3(0, 0, 0);
    glm::vec3 playerPos = glm::vec3(0, 0, 0);

    if (player && ci->getLevelRenderer())
    {
        origin = *ci->getLevelRenderer()->getRendererPlayer()->getCameraPos();
        playerPos = player->getRenderPositionComponent()->mPosition;
    }

    if (D3DHook::FrameTransforms) D3DHook::FrameTransforms->push({ ci->getViewMatrix(), origin, playerPos, ci->getFov() });

    return original(screenView, mcuirc);
}

void* SetupAndRenderHook::onDrawImage(void* context, mce::TexturePtr* texture, glm::vec2* pos, glm::vec2* size, glm::vec2* uv,
    mce::Color* color, void* unk)
{
    auto original = mDrawImageDetour->getOriginal<&SetupAndRenderHook::onDrawImage>();

    nes::event_holder<DrawImageEvent> holder = nes::make_holder<DrawImageEvent>(context, texture, pos, size, uv, color);
    gFeatureManager->mDispatcher->trigger(holder);
    if (holder->isCancelled()) return nullptr;

    return original(context, texture, pos, size, uv, color, unk);
}

void SetupAndRenderHook::initVt(void* ctx)
{
    const auto vtable = *static_cast<uintptr_t**>(ctx);
    mDrawImageDetour = std::make_unique<Detour>("MinecraftUIRenderContext::drawImage", reinterpret_cast<void*>(vtable[OffsetProvider::MinecraftUIRenderContext_drawImage]), &SetupAndRenderHook::onDrawImage);
    mDrawImageDetour->enable();
}

void SetupAndRenderHook::init()
{
    mSetupAndRenderDetour = std::make_unique<Detour>("ScreenView::setupAndRender", reinterpret_cast<void*>(SigManager::ScreenView_setupAndRender), &SetupAndRenderHook::onSetupAndRender);
}
