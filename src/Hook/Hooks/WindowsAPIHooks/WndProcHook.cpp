#include "WndProcHook.hpp"
#include "Hook/Hooks/MiscHooks/KeyHook.hpp"
#include "Hook/Hooks/MiscHooks/MouseHook.hpp"
#include "Hook/Hooks/ClientInstanceHooks/GrabMouseHook.hpp"

#include <Solstice.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProcHook::WndProcCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (Solstice::mUnloading.load())
    {
        return CallWindowProcW(originalWndProc, hWnd, msg, wParam, lParam);
    }

    auto clickGui = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<ClickGui>()
        : nullptr;
    bool uiCapturingMouse = (clickGui && clickGui->mEnabled) || ChatUtils::sChatInputActive;

    // Feed ImGui Win32 handler FIRST so WM_CHAR (UTF-16 / Cyrillic), mouse clicks,
    // scroll wheel, and cursor updates reach ClickGui and CustomChat reliably.
    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP)
    {
        bool isDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        bool isRepeat = isDown && ((lParam & (1 << 30)) != 0);

        if (!isRepeat)
        {
            KeyHook::onKey(static_cast<uint32_t>(wParam), isDown);
        }
        else if (ChatUtils::sChatInputActive || (ImGui::GetCurrentContext() && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)))
        {
            KeyHook::mCurrentCancel = true;
        }

        if (KeyHook::mCurrentCancel)
        {
            return 0;
        }
    }

    if (msg == WM_CHAR || msg == WM_SYSCHAR)
    {
        if (ChatUtils::sChatInputActive || (ImGui::GetCurrentContext() && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)))
        {
            return 0;
        }
    }

    switch (msg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_INPUT:
    {
        if (uiCapturingMouse || (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse))
        {
            GrabMouseHook::ShitNow = true;
            return 0;
        }
        GrabMouseHook::ShitNow = false;
        break;
    }
    default:
        break;
    }

    return CallWindowProcW(originalWndProc, hWnd, msg, wParam, lParam);
}

void WndProcHook::init()
{
    while (!Solstice::mReadyWindow)
    {
        Sleep(10);
    }
    HWND targetWnd = Solstice::GameWindow ? Solstice::GameWindow : ProcUtils::getMinecraftWindow();
    if (!targetWnd)
    {
        spdlog::error("[WndProcHook] Failed to locate Minecraft window!");
        return;
    }
    Solstice::GameWindow = targetWnd;
    originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(targetWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProcHook::WndProcCallback))
    );
}

void WndProcHook::shutdown()
{
    if (Solstice::GameWindow && originalWndProc)
    {
        SetWindowLongPtrW(Solstice::GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc));
        originalWndProc = nullptr;
    }
}
