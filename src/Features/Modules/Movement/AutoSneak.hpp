#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>  // Добавили

class AutoSneak : public ModuleBase<AutoSneak>
{
public:
    BoolSetting mServerSide = BoolSetting("Server Side", "Also sneak server-sidedly via packets", true);

    AutoSneak() : ModuleBase("AutoSneak", "Automatically sneaks at all times", ModuleCategory::Movement, 0, false)
    {
        addSettings(&mServerSide);

        mNames = {
            {Lowercase, "autosneak"},
            {LowercaseSpaced, "auto sneak"},
            {Normal, "AutoSneak"},
            {NormalSpaced, "Auto Sneak"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onRenderEvent(class RenderEvent& event);  // Добавили

private:
    void forceSneak();  // Вынесли установку сника сюда
};