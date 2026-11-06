//
//

#include "SetupAndRenderHook.hpp"

#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Rendering/LevelRenderer.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>

#include "D3DHook.hpp"
#include "ItemRendererHook.hpp"
#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>

std::unique_ptr<Detour> SetupAndRenderHook::mSetupAndRenderDetour;
std::unique_ptr<Detour> SetupAndRenderHook::mDrawImageDetour;

void* SetupAndRenderHook::onSetupAndRender(void* screenView, void* mcuirc)
{
    auto ci = ClientInstance::get();
    auto Clickgui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    if (Clickgui->mEnabled)ci->releaseMouse();
    auto original = mSetupAndRenderDetour->getOriginal<&SetupAndRenderHook::onSetupAndRender>();

    static bool once = false;
    if (!once)
    {
        once = true;
        initVt(mcuirc);
    }


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

    // Flush deferred GUI item icons (queued by modules such as InventoryHUD).
    ItemIconQueue::swapBuffers();

    if (!ItemIconQueue::drawing().empty())
    {
        ItemRenderer*           ir  = ItemRendererHook::getItemRenderer();
        BaseActorRenderContext* bar = ItemRendererHook::getBarContext();

        if (ir && bar)
        {
            for (const auto& ic : ItemIconQueue::drawing())
            {
                if (!ic.stack || !ic.stack->mItem || ic.scale <= 0.f) continue;
                ir->renderGuiItemNew(bar, ic.stack, 0, ic.pos.x, ic.pos.y, 1.0f, ic.scale, false);
            }
        }
    }

    ItemIconQueue::drawing().clear();

    return original(screenView, mcuirc);
}

void* SetupAndRenderHook::onDrawImage(void* context, mce::TexturePtr* texture, glm::vec2* pos, glm::vec2* size, glm::vec2* uv,
    mce::Color* color, void* unk)
{
    auto original = mDrawImageDetour->getOriginal<&SetupAndRenderHook::onDrawImage>();

    //nes::event_holder<DrawImageEvent> holder = nes::make_holder<DrawImageEvent>(context, texture, pos, size, uv, color);
    //gFeatureManager->mDispatcher->trigger(holder);
    //if (holder->isCancelled()) return nullptr;

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
