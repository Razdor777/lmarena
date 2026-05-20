#pragma once
//
// Created by vastrakai on 7/7/2024.
//

#include <Features/Modules/Module.hpp>

class ESP : public ModuleBase<ESP>
{
public:
    enum class Style {
        Style3D,
        Glow,
        Corners,
    };

    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "Visual style", Style::Glow, "3D", "Glow", "Corners");
    BoolSetting mRenderFilled = BoolSetting("Render Filled", "Render the ESP filled", true);
    BoolSetting mRenderLocal = BoolSetting("Render Local", "Render local player", false);
    BoolSetting mShowFriends = BoolSetting("Show Friends", "Render friends", true);
    BoolSetting mOuterGlow = BoolSetting("Outer Glow", "Add glow effect around box", true);
    NumberSetting mGlowSize = NumberSetting("Glow Size", "Glow radius", 25.0f, 5.0f, 60.0f, 1.0f);
    NumberSetting mCornerLength = NumberSetting("Corner Length", "Corner bracket length", 8.0f, 3.0f, 20.0f, 1.0f);
    BoolSetting mDebug = BoolSetting("Debug", "Display bots", false);

    ESP() : ModuleBase("ESP", "Draws a box around entities", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mStyle,
            &mRenderFilled,
            &mRenderLocal,
            &mShowFriends,
            &mOuterGlow,
            &mGlowSize,
            &mCornerLength
        );

        VISIBILITY_CONDITION(mGlowSize, mOuterGlow.mValue);
        VISIBILITY_CONDITION(mCornerLength, mStyle.mValue == Style::Corners);

        mNames = {
            {Lowercase, "esp"},
            {LowercaseSpaced, "esp"},
            {Normal, "ESP"},
            {NormalSpaced, "ESP"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        return mStyle.mValues[mStyle.as<int>()];
    }

private:
    void renderEntity(ImDrawList* drawList, Actor* actor, Actor* localPlayer, ImColor themeColor);
    void renderGlowBox(ImDrawList* drawList, const std::vector<ImVec2>& points, ImColor color, float glowSize);
    void renderCorners(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImColor color, float length, float thickness);
};