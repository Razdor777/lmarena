#pragma once

#include "Features/Modules/Module.hpp"

class AirJump : public ModuleBase<AirJump>
{
public:
    AirJump() : ModuleBase<AirJump>("AirJump",
        "Allows you to jump in the air",
        ModuleCategory::Movement, 0, false)
    {
        mNames = {
            {Lowercase, "airjump"},
            {LowercaseSpaced, "air jump"},
            {Normal, "AirJump"},
            {NormalSpaced, "Air Jump"}
        };
    }

    bool mOldSpacePressed = false;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
};