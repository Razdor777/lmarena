#pragma once
//
// Created by vastrakai on 10/4/2024.
// Rewritten: Premium Tracers with gradient, glow, distance
//

class Tracers : public ModuleBase<Tracers> {
public:
    enum class CenterPoint {
        Top,
        Center,
        Bottom
    };
    
    enum class Style {
        Line,       // Simple line
        Gradient,   // Color gradient from player to target
        Glow,       // Line with glow effect
        Arrow,      // Line with arrowhead
    };

    EnumSettingT<CenterPoint> mCenterPoint = EnumSettingT<CenterPoint>("Center Point", "Origin point on screen", CenterPoint::Center, "Top", "Center", "Bottom");
    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "Tracer visual style", Style::Gradient, "Line", "Gradient", "Glow", "Arrow");
    BoolSetting mRenderLocal = BoolSetting("Render Local", "Render on local player", false);
    BoolSetting mShowFriends = BoolSetting("Show Friends", "Render on friends", true);
    BoolSetting mShowDistance = BoolSetting("Distance", "Show distance at endpoint", true);
    NumberSetting mThickness = NumberSetting("Thickness", "Line thickness", 2.0f, 0.5f, 5.0f, 0.5f);
    NumberSetting mGlowSize = NumberSetting("Glow Size", "Glow radius", 8.0f, 2.0f, 20.0f, 1.0f);

    Tracers() : ModuleBase("Tracers", "Draws a line to every entity", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mCenterPoint,
            &mStyle,
            &mRenderLocal,
            &mShowFriends,
            &mShowDistance,
            &mThickness,
            &mGlowSize
        );
        
        VISIBILITY_CONDITION(mGlowSize, mStyle.mValue == Style::Glow);

        mNames = {
            {Lowercase, "tracers"},
            {LowercaseSpaced, "tracers"},
            {Normal, "Tracers"},
            {NormalSpaced, "Tracers"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    
    std::string getSettingDisplay() override {
        return mStyle.mValues[mStyle.as<int>()];
    }
};