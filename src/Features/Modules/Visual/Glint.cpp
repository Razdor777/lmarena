//
// Glint.cpp — recolor/restyle the enchantment glint
//
// Uses the existing RenderItemInHandHook (registered in HookManager) — the
// hooked frame-description constructor exposes mGlintColor / mGlintAlpha.
// No custom signatures = nothing to silently break on game updates.
//

#include "Glint.hpp"

#include <Features/Events/RenderItemInHandDescriptionEvent.hpp>
#include <Hook/Hooks/RenderHooks/RenderItemInHandHook.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

Glint::Glint()
    : ModuleBase("Glint", "Customize the enchantment glint (color, saturation, alpha)",
                 ModuleCategory::Visual, 0, false)
{
    addSettings(&mRainbow, &mColor, &mSaturation, &mAlpha, &mShowInGui);

    VISIBILITY_CONDITION(mColor, !mRainbow.mValue);

    mNames = {
        {Lowercase, "glint"},
        {LowercaseSpaced, "glint"},
        {Normal, "Glint"},
        {NormalSpaced, "Glint"}
    };
}

void Glint::onEnable()
{
    gFeatureManager->mDispatcher
        ->listen<RenderItemInHandDescriptionEvent, &Glint::onRenderItemInHandDesc>(this);
}

void Glint::onDisable()
{
    gFeatureManager->mDispatcher
        ->deafen<RenderItemInHandDescriptionEvent, &Glint::onRenderItemInHandDesc>(this);
}

// Scale saturation of an RGB color via HSV
static glm::vec3 saturate(glm::vec3 rgb, float mul)
{
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(rgb.x, rgb.y, rgb.z, h, s, v);
    s = MathUtils::clamp(s * mul, 0.f, 1.f);
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
    return { r, g, b };
}

void Glint::onRenderItemInHandDesc(RenderItemInHandDescriptionEvent& event)
{
    if (!event.mThis) return;
    if (event.mIsDrawingUI && !mShowInGui.mValue) return;

    glm::vec3 color;
    if (mRainbow.mValue)
    {
        ImColor c = ColorUtils::getThemedColor(0.f);
        color = { c.Value.x, c.Value.y, c.Value.z };
    }
    else
    {
        ImColor c = mColor.getAsImColor();
        color = { c.Value.x, c.Value.y, c.Value.z };
    }

    color = saturate(color, mSaturation.mValue);

    event.mThis->setGlintColor(color);
    event.mThis->setGlintAlpha(
        MathUtils::clamp(event.mThis->getGlintAlpha() * mAlpha.mValue, 0.f, 1.f));
}
