//
// Created by vastrakai on 7/7/2024.
//

#include "SetupAndRenderHook.hpp"

#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Rendering/LevelRenderer.hpp>
#include <SDK/Minecraft/Rendering/BaseActorRenderContext.hpp>
#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>

#include "D3DHook.hpp"
#include "ItemRendererHook.hpp"

std::unique_ptr<Detour> SetupAndRenderHook::mSetupAndRenderDetour;
std::unique_ptr<Detour> SetupAndRenderHook::mDrawImageDetour;

void* SetupAndRenderHook::onSetupAndRender(void* screenView, void* mcuirc)
{
    auto ci = ClientInstance::get();
    auto clickGui = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<ClickGui>()
        : nullptr;
    if (ci && clickGui && clickGui->mEnabled)
        ci->releaseMouse();

    auto original = mSetupAndRenderDetour->getOriginal<&SetupAndRenderHook::onSetupAndRender>();

    static bool once = false;
    if (!once)
    {
        once = true;
        initVt(mcuirc);
    }

    if (!ci) return original(screenView, mcuirc);

    auto player = ci->getLocalPlayer();

    glm::vec3 origin = glm::vec3(0, 0, 0);
    glm::vec3 playerPos = glm::vec3(0, 0, 0);

    if (player && ci->getLevelRenderer() && ci->getLevelRenderer()->getRendererPlayer() && player->getRenderPositionComponent())
    {
        origin = *ci->getLevelRenderer()->getRendererPlayer()->getCameraPos();
        playerPos = player->getRenderPositionComponent()->mPosition;
    }

    if (D3DHook::FrameTransforms)
        D3DHook::FrameTransforms->push({ ci->getViewMatrix(), origin, playerPos, ci->getFov() });

    ItemIconQueue::swapBuffers();

    if (!ItemIconQueue::drawing().empty() && SigManager::ItemRenderer_renderGuiItemNew != 0)
    {
        ItemRenderer* ir = ItemRendererHook::getItemRenderer();
        BaseActorRenderContext* bar = ItemRendererHook::getBarContext();

        if (ir && bar)
        {
            for (const auto& ic : ItemIconQueue::drawing())
            {
                if (!ic.stack || !ic.stack->getItem() || ic.scale <= 0.f) continue;

                ir->renderGuiItemNew(
                    bar,
                    ic.stack,
                    0,
                    ic.pos.x,
                    ic.pos.y,
                    1.0f,
                    ic.scale,
                    false);
            }
        }
    }

    ItemIconQueue::drawing().clear();

    return original(screenView, mcuirc);
}

void* SetupAndRenderHook::onDrawImage(void* context, mce::TexturePtr* texture, glm::vec2* pos, glm::vec2* size,
    glm::vec2* uv, mce::Color* color, void* unk)
{
    auto original = mDrawImageDetour->getOriginal<&SetupAndRenderHook::onDrawImage>();
    return original(context, texture, pos, size, uv, color, unk);
}

void SetupAndRenderHook::initVt(void* mcuirc)
{
    // DrawImage vtable index changed in 26.62; intentionally left unhooked as in 26.62 base.
    (void)mcuirc;
}

void SetupAndRenderHook::init()
{
    mSetupAndRenderDetour = std::make_unique<Detour>("ScreenView::setupAndRender", reinterpret_cast<void*>(SigManager::ScreenView_setupAndRender), &SetupAndRenderHook::onSetupAndRender);
}
