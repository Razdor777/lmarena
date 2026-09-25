//
// Created by Solar on 11/18/2024.
//

#include "HoverTextRendererHook.hpp"

#include <SDK/Minecraft/mce.hpp>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ОБНОВЛЕНО: HoverTextRenderer::render -> src-client/mc/client/gui/controls/renderers/HoverTextRenderer.h:60

std::unique_ptr<Detour> HoverTextRendererHook::mHoverTextRendererRenderDetour;

void HoverTextRendererHook::onHoverTextRendererRender(HoverTextRenderer* _this, void* mRenderContext, ClientInstance* mClient, glm::vec4* mRenderAABB, int mPass)
{
    auto original = mHoverTextRendererRenderDetour->getOriginal<&HoverTextRendererHook::onHoverTextRendererRender>();

    if (HoverTextRender::mTimeDisplayed == 0) {
        HoverTextRender::mTimeDisplayed++;
    }

    HoverTextRender::mInfo.mText = _this->mFilteredContent;
    HoverTextRender::mInfo.mPos = glm::vec2(ImRenderUtils::getMousePos().x + 3, ImRenderUtils::getMousePos().y + 3);

    original(_this, mRenderContext, mClient, mRenderAABB, mPass);
}

void HoverTextRendererHook::init()
{
    mHoverTextRendererRenderDetour = std::make_unique<Detour>("HoverTextRenderer::render", reinterpret_cast<void*>(SigManager::HoverTextRenderer_render), &HoverTextRendererHook::onHoverTextRendererRender);
}
