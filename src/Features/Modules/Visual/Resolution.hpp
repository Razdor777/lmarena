#pragma once

//
// Resolution — кастомное разрешение окна Minecraft.
//
// Что делает: меняет размер клиентской области окна (и, по желанию, убирает
// рамку). Игра сама подхватывает WM_SIZE и пересоздаёт swap chain под новый
// размер — то есть рендер реально идёт в выбранном разрешении, а не просто
// растягивается картинка.
//
// Что НЕ делает: не рендерит в 720p и не растягивает на 1080p (для этого
// нужен перехват IDXGISwapChain::ResizeBuffers и своя перерисовка кадра —
// отдельная задача, без данных из IDA её делать нельзя).
//
// При выключении возвращает исходный размер окна и его стиль.
//

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Modules/Module.hpp>

class Resolution : public ModuleBase<Resolution>
{
public:
    enum class Mode { Windowed, Borderless };

    enum class Preset { P1280x720, P1366x768, P1600x900, P1920x1080, P2560x1440, Custom };

    EnumSettingT<Mode> mMode = EnumSettingT<Mode>(
        "Mode", "Windowed keeps the frame, borderless removes it",
        Mode::Windowed, "Windowed", "Borderless");

    EnumSettingT<Preset> mPreset = EnumSettingT<Preset>(
        "Preset", "Common resolutions, or Custom to set your own",
        Preset::P1600x900, "1280x720", "1366x768", "1600x900", "1920x1080", "2560x1440", "Custom");

    NumberSetting mWidth  = NumberSetting("Width",  "Custom width in pixels",  1600.f, 640.f, 3840.f, 1.f);
    NumberSetting mHeight = NumberSetting("Height", "Custom height in pixels",  900.f, 360.f, 2160.f, 1.f);

    Resolution() : ModuleBase("Resolution", "Custom window resolution", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mMode,
            &mPreset,
            &mWidth,
            &mHeight
        );

        VISIBILITY_CONDITION(mWidth,  mPreset.mValue == Preset::Custom);
        VISIBILITY_CONDITION(mHeight, mPreset.mValue == Preset::Custom);

        mNames = {
            {Lowercase,       "resolution"},
            {LowercaseSpaced, "resolution"},
            {Normal,          "Resolution"},
            {NormalSpaced,    "Resolution"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);

    std::string getSettingDisplay() override
    {
        int w = 0, h = 0;
        resolveSize(w, h);
        return std::to_string(w) + "x" + std::to_string(h);
    }

private:
    void resolveSize(int& w, int& h) const;
    void apply();
    void restore();

    bool    mApplied      = false;
    bool    mSavedState   = false;
    int     mLastW        = 0;
    int     mLastH        = 0;
    bool    mLastBorderless = false;
};
