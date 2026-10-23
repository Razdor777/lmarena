//
// Resolution — см. комментарий в Resolution.hpp.
//

#include "Resolution.hpp"

#include <Windows.h>

#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/ProcUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

namespace
{
    HWND  gWindow      = nullptr;
    RECT  gOriginalRect{};
    LONG_PTR gOriginalStyle = 0;
    bool  gSaved       = false;

    bool saveWindowState()
    {
        if (gSaved) return gWindow != nullptr;

        HWND wnd = ProcUtils::getMinecraftWindow();
        if (!wnd || !IsWindow(wnd)) return false;

        gWindow = wnd;
        if (!GetWindowRect(gWindow, &gOriginalRect)) { gWindow = nullptr; return false; }

        gOriginalStyle = GetWindowLongPtrA(gWindow, GWL_STYLE);
        gSaved = true;
        return true;
    }
}

void Resolution::resolveSize(int& w, int& h) const
{
    switch (mPreset.mValue)
    {
        case Preset::P1280x720:  w = 1280; h = 720;  break;
        case Preset::P1366x768:  w = 1366; h = 768;  break;
        case Preset::P1600x900:  w = 1600; h = 900;  break;
        case Preset::P1920x1080: w = 1920; h = 1080; break;
        case Preset::P2560x1440: w = 2560; h = 1440; break;
        case Preset::Custom:
        default:
            w = static_cast<int>(mWidth.mValue);
            h = static_cast<int>(mHeight.mValue);
            break;
    }

    if (w < 320) w = 320;
    if (h < 240) h = 240;
}

void Resolution::apply()
{
    if (!saveWindowState())
    {
        ChatUtils::displayClientMessage("§c[Resolution] §fОкно Minecraft не найдено — меняю его один раз при следующем тике.");
        return;
    }

    int w = 0, h = 0;
    resolveSize(w, h);

    const bool borderless = (mMode.mValue == Mode::Borderless);

    if (borderless)
    {
        // Рамку убираем только на время работы модуля: исходный стиль уже сохранён.
        SetWindowLongPtrA(gWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(gWindow, HWND_TOP, 0, 0, w, h,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        // Оконный режим: возвращаем сохранённый стиль и пересчитываем размер
        // рамки, иначе клиентская область окажется меньше запрошенной.
        SetWindowLongPtrA(gWindow, GWL_STYLE, gOriginalStyle);

        RECT rect{ 0, 0, w, h };
        const LONG_PTR style = GetWindowLongPtrA(gWindow, GWL_STYLE);
        if (style & WS_CAPTION)
            AdjustWindowRect(&rect, static_cast<DWORD>(style), FALSE);

        SetWindowPos(gWindow, nullptr, 0, 0,
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOZORDER);
    }

    mLastW          = w;
    mLastH          = h;
    mLastBorderless = borderless;
    mApplied        = true;
}

void Resolution::restore()
{
    if (!gSaved || !gWindow || !IsWindow(gWindow)) return;

    SetWindowLongPtrA(gWindow, GWL_STYLE, gOriginalStyle);
    SetWindowPos(gWindow, nullptr,
                 gOriginalRect.left, gOriginalRect.top,
                 gOriginalRect.right - gOriginalRect.left,
                 gOriginalRect.bottom - gOriginalRect.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOZORDER);
}

void Resolution::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Resolution::onBaseTickEvent>(this);

    mApplied = false;
    apply();
}

void Resolution::onDisable()
{
    if (mApplied) restore();
    mApplied = false;

    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Resolution::onBaseTickEvent>(this);
}

void Resolution::onBaseTickEvent(BaseTickEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    if (!event.mActor || event.mActor != ci->getLocalPlayer()) return;

    int w = 0, h = 0;
    resolveSize(w, h);
    const bool borderless = (mMode.mValue == Mode::Borderless);

    // Настроек-коллбеков в клиенте нет, поэтому изменения ловим здесь: 20 раз
    // в секунду сравнить два числа дешевле, чем подписываться на каждую строку.
    if (!mApplied || w != mLastW || h != mLastH || borderless != mLastBorderless)
        apply();
}
