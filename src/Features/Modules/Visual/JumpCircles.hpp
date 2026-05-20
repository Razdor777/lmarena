#pragma once

//
// Created by alteik on 26/10/2024.
// Rewritten: Premium glow rings with fade + ripple
//

class JumpCircles : public ModuleBase<JumpCircles>
{
public:

    struct Circle {
        glm::vec3 position;
        float radius;
        float maxRadius;
        ImVec4 color;
        float glowAmount;
        float opacity;
        uint64_t startTime;
    };

    enum class Style {
        Ripple,   // Expanding glow ring with fade
        Pulse,    // Pulsing circle with beat
        Shockwave // Fast expanding thin ring
    };

    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "Circle visual style", Style::Ripple, "Ripple", "Pulse", "Shockwave");
    NumberSetting mSpeed = NumberSetting("Speed", "Expansion speed", 0.04f, 0.01f, 0.20f, 0.01f);
    NumberSetting mMaxRadius = NumberSetting("Max Radius", "Maximum ring radius", 2.5f, 0.5f, 6.0f, 0.1f);
    NumberSetting mRings = NumberSetting("Rings", "Number of concentric rings", 6, 1, 20, 1);
    NumberSetting mGlowAmount = NumberSetting("Glow", "Glow intensity", 30, 0, 100, 1);
    NumberSetting mOpacity = NumberSetting("Opacity", "Base opacity", 0.7f, 0.f, 1.f, 0.01f);
    NumberSetting mLifeTime = NumberSetting("Life time (ms)", "Time to render circle", 3000, 500, 10000, 100);
    NumberSetting mThickness = NumberSetting("Thickness", "Ring line thickness", 2.5f, 0.5f, 6.0f, 0.5f);

    JumpCircles() : ModuleBase("JumpCircles", "Render circle when u jumping", ModuleCategory::Visual, 0, false) {

        addSettings(
            &mStyle,
            &mSpeed,
            &mMaxRadius,
            &mRings,
            &mGlowAmount,
            &mOpacity,
            &mLifeTime,
            &mThickness
        );

        mNames = {
            {Lowercase, "jumpcircles"},
            {LowercaseSpaced, "jump circles"},
            {Normal, "JumpCircles"},
            {NormalSpaced, "Jump Circles"}
        };
    }

    std::vector<Circle> circles;
    uint64_t lastAddTime = 0;

    float toRadians(float deg);
    void addCircle(const glm::vec3& pos);

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    
    std::string getSettingDisplay() override {
        return mStyle.mValues[mStyle.as<int>()];
    }
};
