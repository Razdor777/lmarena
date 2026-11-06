//
// Created by vastrakai on 6/29/2024.
//

#include "MouseHook.hpp"

#include <Features/Events/MouseEvent.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>

std::unique_ptr<Detour> MouseHook::mDetour = nullptr;

static void* thisMouse = nullptr;

void MouseHook::onMouse(void* _this, char actionButtonId, int buttonData, short x, short y, short dx, short dy,
    uint8_t forceMotionlessPointer)
{
    auto oFunc = mDetour->getOriginal<&onMouse>();
    thisMouse = _this;

    if (forceMotionlessPointer == 2)
    {
        forceMotionlessPointer = true;
        return oFunc(_this, actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
    }

    auto holder = nes::make_holder<MouseEvent>(_this, actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
    if (gFeatureManager && gFeatureManager->mDispatcher)
        gFeatureManager->mDispatcher->trigger(holder);

    mButtonStates[actionButtonId] = buttonData != 0;

    bool cancel = holder->isCancelled() || ChatUtils::sChatInputActive;

    if (ImGui::GetCurrentContext() != nullptr)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindow* window = ImGui::GetCurrentContext()->HoveredWindow;

        if (window && io.MouseDown[0] && actionButtonId == 1)
            cancel = true;

        if (!io.WantCaptureMouse && !cancel)
        {
            return oFunc(_this, actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
        }
    }
    else if (!cancel)
    {
        return oFunc(_this, actionButtonId, buttonData, x, y, dx, dy, forceMotionlessPointer);
    }
}

void MouseHook::simulateMouseInput(char actionButtonId, int buttonData, short x, short y, short dx, short dy)
{
    if (!thisMouse || !mDetour) return;
    onMouse(thisMouse, actionButtonId, buttonData, x, y, dx, dy, 2);
}

void MouseHook::init()
{
    mDetour = std::make_unique<Detour>("MouseDevice::feed", reinterpret_cast<void*>(SigManager::MouseDevice_feed), &MouseHook::onMouse);
}
