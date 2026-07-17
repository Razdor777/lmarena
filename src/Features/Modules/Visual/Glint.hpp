#pragma once
#include <Features/Modules/Module.hpp>
#include <glm/glm.hpp>

//
// Glint — customize the enchantment glint.
//
// Implemented on top of RenderItemInHandHook, which hooks
// mce::framebuilder::RenderItemInHandDescription's constructor.
// That description carries documented glint fields:
//   mGlintColor @ 0x8C, mGlintAlpha @ 0x9C
// so we can safely restyle the glint of enchanted items without
// inventing unverified function signatures.
//
class Glint : public ModuleBase<Glint> {
public:
    Glint();

    BoolSetting mRainbow     = BoolSetting("Rainbow", "Animated rainbow glint color", false);
    ColorSetting mColor      = ColorSetting("Color", "Glint color", 0.75f, 0.25f, 1.0f, 1.0f);
    NumberSetting mSaturation = NumberSetting("Saturation", "Glint saturation multiplier", 1.0f, 0.0f, 3.0f, 0.05f);
    NumberSetting mAlpha     = NumberSetting("Alpha", "Glint alpha multiplier", 1.0f, 0.0f, 2.0f, 0.05f);
    BoolSetting mShowInGui   = BoolSetting("Affect GUI", "Also restyle glint in inventory/GUI", true);

    void onEnable() override;
    void onDisable() override;
    void onRenderItemInHandDesc(class RenderItemInHandDescriptionEvent& event);
};
