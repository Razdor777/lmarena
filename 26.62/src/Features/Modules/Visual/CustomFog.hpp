#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>

class CustomFog : public ModuleBase<CustomFog> {
public:
    // Собственные статики — не зависят от Ambience
    static inline bool sNoFog = false;
    static inline float sFogColorR = 0.0f;
    static inline float sFogColorG = 0.0f;
    static inline float sFogColorB = 0.0f;
    static inline float sFogColorA = 1.0f;

    enum class FogMode {
        NoFog,
        ColorOnly,
        SilentHill
    };

    EnumSettingT<FogMode> mMode = EnumSettingT<FogMode>(
        "Mode", "Fog manipulation mode",
        FogMode::NoFog,
        "No Fog", "Color Only", "Silent Hill"
    );

    NumberSetting mRed   = NumberSetting("Red",   "Fog Red Component",   255.0f, 0.0f, 255.0f, 1.0f);
    NumberSetting mGreen = NumberSetting("Green", "Fog Green Component", 0.0f,   0.0f, 255.0f, 1.0f);
    NumberSetting mBlue  = NumberSetting("Blue",  "Fog Blue Component",  0.0f,   0.0f, 255.0f, 1.0f);

    CustomFog() : ModuleBase("CustomFog", "Modify game fog density and color", ModuleCategory::Visual, 0, false) {
        addSettings(&mMode, &mRed, &mGreen, &mBlue);

        VISIBILITY_CONDITION(mRed,   mMode.mValue != FogMode::NoFog);
        VISIBILITY_CONDITION(mGreen, mMode.mValue != FogMode::NoFog);
        VISIBILITY_CONDITION(mBlue,  mMode.mValue != FogMode::NoFog);

        mNames = {
            {Lowercase,       "customfog"},
            {LowercaseSpaced, "custom fog"},
            {Normal,          "CustomFog"},
            {NormalSpaced,    "Custom Fog"}
        };
    }

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        return mMode.mValues[mMode.as<int>()];
    }
};