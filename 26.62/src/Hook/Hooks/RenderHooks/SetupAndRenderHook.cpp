//
// Created by vastrakai on 7/7/2024.
//

#include "SetupAndRenderHook.hpp"

#include <cstdio>
#include <fstream>

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

static void NDrawLog(const char* fmt, ...)
{
    char buf[512];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::ofstream out("C:\\native_draw.log", std::ios::app);
    out << buf << '\n';
}

void* SetupAndRenderHook::onSetupAndRender(void* screenView, void* mcuirc)
{
    auto ci = ClientInstance::get();
    auto Clickgui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    if (Clickgui->mEnabled)ci->releaseMouse();
    auto original = mSetupAndRenderDetour->getOriginal<&SetupAndRenderHook::onSetupAndRender>();

    static bool once = false;
    if (!once) { once = true; initVt(mcuirc); }


    if (!ci) return original(screenView, mcuirc);

    auto player = ci->getLocalPlayer();

    glm::vec3 origin    = glm::vec3(0, 0, 0);
    glm::vec3 playerPos = glm::vec3(0, 0, 0);

    if (player && ci->getLevelRenderer())
    {
        origin    = *ci->getLevelRenderer()->getRendererPlayer()->getCameraPos();
        playerPos = player->getRenderPositionComponent()->mPosition;
    }

    if (D3DHook::FrameTransforms) D3DHook::FrameTransforms->push({ ci->getViewMatrix(), origin, playerPos, ci->getFov() });

    // ─────────────────────────────────────────────────────────────────────
    //  ИКОНКИ ИЗ ПАМЯТИ ИГРЫ — путь Flarial:
    //  ItemRenderer::renderGuiItemNew(ctx, stack, mode, x, y, opacity, scale, ench)
    //  Он сам делает атлас → UV → iconBlit. Никаких getIconUV / TexturePtr.
    // ─────────────────────────────────────────────────────────────────────
    ItemIconQueue::swapBuffers();

    if (!ItemIconQueue::drawing().empty())
    {
        ItemRenderer*           ir  = ItemRendererHook::getItemRenderer();
        BaseActorRenderContext* bar = ItemRendererHook::getBarContext();

        NDrawLog("[frame] queue=%zu ir=%llx bar=%llx",
                 ItemIconQueue::drawing().size(),
                 (unsigned long long)ir,
                 (unsigned long long)bar);

        if (ir && bar)
        {
            for (const auto& ic : ItemIconQueue::drawing())
            {
                if (!ic.stack || !ic.stack->mItem) { NDrawLog("  skip: null stack"); continue; }
                if (ic.scale <= 0.f)              { NDrawLog("  skip: scale<=0"); continue; }

                Item* itemPtr = ic.stack->getItem();
                NDrawLog("  draw: item=%s pos=%.1f,%.1f scale=%.3f",
                         itemPtr ? itemPtr->mName.c_str() : "?",
                         ic.pos.x, ic.pos.y, ic.scale);

                ir->renderGuiItemNew(
                    bar,
                    ic.stack,
                    0,              // mode
                    ic.pos.x,
                    ic.pos.y,
                    1.0f,           // opacity
                    ic.scale,
                    false);         // enchanted
            }
        }
    }

    ItemIconQueue::drawing().clear();
    // ─────────────────────────────────────────────────────────────────────

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
