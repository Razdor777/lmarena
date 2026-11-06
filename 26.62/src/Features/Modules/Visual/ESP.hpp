#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/Interface.hpp>

class ESP : public ModuleBase<ESP>
{
public:
    enum class BoxMode {
        None,
        Full2D,
        Corners2D,
        Box3D
    };

    EnumSettingT<BoxMode> mBoxMode = EnumSettingT<BoxMode>("Box Mode", "How the box is drawn", BoxMode::Corners2D, "None", "Full 2D", "Corners 2D", "3D");
    
    BoolSetting mSyncInterface = BoolSetting("Sync Interface", "Use colors from Interface module", true);
    ColorSetting mColor = ColorSetting("Color", "Color of the ESP when not synced", 1.0f, 0.3f, 0.3f, 1.0f);
    ColorSetting mTeamColor = ColorSetting("Team Color", "Color for players on your team", 0.0f, 1.0f, 0.3f, 1.0f);
    BoolSetting mShimmer = BoolSetting("Shimmer", "Makes the color gently pulse and shift", true);
    
    BoolSetting mRenderFilled = BoolSetting("Render Filled", "Fill the box with transparent color", true);
    BoolSetting mShadow = BoolSetting("Shadow", "Draw a shadow behind the box for better visibility", true);
    NumberSetting mLineWidth = NumberSetting("Line Width", "Thickness of the lines", 1.5f, 0.5f, 5.f, 0.1f);
    NumberSetting mCornerSize = NumberSetting("Corner Size", "Length of the corners (percentage)", 0.3f, 0.1f, 0.5f, 0.05f);
    BoolSetting mInvisible = BoolSetting("Invisible", "Render ESP for invisible entities", false);

    ESP() : ModuleBase("ESP", "See entities through walls", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mBoxMode,
            &mSyncInterface,
            &mColor,
            &mTeamColor,
            &mShimmer,
            &mRenderFilled,
            &mShadow,
            &mLineWidth,
            &mCornerSize,
            &mInvisible
        );

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

private:
    ImColor getEntityColor(Actor* actor, float alpha);
};