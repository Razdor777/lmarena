//
// Created by vastrakai on 7/7/2024.
//

#include "RenderUtils.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

#include "ColorUtils.hpp"
#include "MathUtils.hpp"

void RenderUtils::drawOutlinedAABB(const AABB& aabb, bool filled, const ImColor& color)
{
    std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(aabb);

    auto drawList = ImGui::GetBackgroundDrawList();

    if (filled) drawList->AddConvexPolyFilled(imPoints.data(), imPoints.size(), ImColor(color.Value.x, color.Value.y, color.Value.z, 0.25f));
    drawList->AddPolyline(imPoints.data(), imPoints.size(), color, 0, 2.0f);
}

bool RenderUtils::worldToScreen(glm::vec3 pos, ImVec2& screenPos)
{
    // This is the single per-frame entry point of every visual module, and it
    // used to dereference both ClientInstance and GuiData raw. During a swap
    // chain resize (F11, dragging the window, minimise/restore) and while a
    // world is loading, GuiData is briefly gone — that is the crash that
    // happened when you resized the window. Returning false is safe: every
    // caller already treats a failed projection as "do not draw".
    auto* ci = ClientInstance::get();
    if (!ci) return false;

    auto* guiData = ci->getGuiData();
    if (!guiData) return false;

    auto corrected = transform.mMatrix;
    glm::vec2 screen;
    bool result = corrected.OWorldToScreen(transform.mOrigin, pos, screen, transform.mFov, guiData->mResolution);
    screenPos = ImVec2(screen.x, screen.y);
    return result;
}