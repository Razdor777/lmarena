//
// Created by vastrakai on 7/1/2024.
//

#include "ColorUtils.hpp"

#include <regex>
#include <algorithm>
#include <cmath>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Interface.hpp>

ImColor ColorUtils::Rainbow(float seconds, float saturation, float brightness, int index)
{
    float hue = ((NOW + index) % (int)(seconds * 1000)) / (float)(seconds * 1000);
    float r, g, b = 0;
    return ImColor::HSV(hue, saturation, brightness);
}

ImColor ColorUtils::LerpColors(float seconds, float index, std::vector<ImColor> colors, uint64_t ms) {
    if (colors.empty()) return { 255, 255, 255, 255};
    float time = 10000.0f / seconds;
    auto angle = static_cast<float>(((ms == 0 ? NOW : ms) + static_cast<int>(index)) % static_cast<int>(time));
    float segmentTime = time / colors.size();

    int segmentIndex = static_cast<int>(angle / segmentTime);
    float segmentIndexFloat = angle / segmentTime - segmentIndex;

    ImColor startColor = colors[segmentIndex];
    ImColor endColor = colors[(segmentIndex + 1) % colors.size()];
    return startColor.Lerp(endColor, segmentIndexFloat);
}

ImColor ColorUtils::getThemedColor(float index, uint64_t ms)
{
    auto daInterface = gFeatureManager->mModuleManager->getModule<Interface>();
    if (!daInterface) return { 255, 255, 255, 255 };

    auto theme = daInterface->mMode.mValue;
    auto colors = Interface::ColorThemes[theme];
    if (theme == Interface::Rainbow) return Rainbow(daInterface->mColorSpeed.mValue, daInterface->mSaturation.mValue, 1.f, index);
    else if (theme == Interface::Custom)
    {
        colors = daInterface->getCustomColors();
    }

    return LerpColors(daInterface->mColorSpeed.mValue, index, colors, ms);

}

std::string ColorUtils::removeColorCodes(const std::string& text)
{
    static std::regex colorCodeRegex("§[0-9a-z]");
    static std::string emptyString = "";
    return std::regex_replace(text, colorCodeRegex, emptyString);
}

// ==== ported from the 1.21.44 master ====

ImColor ColorUtils::getStaticAccentColor(float index)
{
    // В мастере это «палитра без волны/пульсации». Здесь палитру задаёт Interface,
    // поэтому просто берём обычный themed-цвет — он и так стабилен по времени.
    return getThemedColor(index);
}

ImColor ColorUtils::getStaticAccentColorAlpha(float index, float alpha)
{
    ImColor c = getStaticAccentColor(index);
    c.Value.w = std::clamp(alpha, 0.f, 1.f);
    return c;
}

void ColorUtils::rgbToHsl(float r, float g, float b, float& h, float& s, float& l)
{
    r = std::clamp(r, 0.f, 1.f);
    g = std::clamp(g, 0.f, 1.f);
    b = std::clamp(b, 0.f, 1.f);
    float mx = std::max({ r, g, b });
    float mn = std::min({ r, g, b });
    l = (mx + mn) * 0.5f;
    if (mx <= mn + 1e-6f) { h = 0.f; s = 0.f; return; }

    float d = mx - mn;
    s = l > 0.5f ? d / std::max(2.f - mx - mn, 1e-6f) : d / std::max(mx + mn, 1e-6f);

    if (mx == r)      h = (g - b) / d + (g < b ? 6.f : 0.f);
    else if (mx == g) h = (b - r) / d + 2.f;
    else              h = (r - g) / d + 4.f;
    h /= 6.f;
}

void ColorUtils::hslToRgb(float h, float s, float l, float& r, float& g, float& b)
{
    h = h - std::floor(h);
    if (!std::isfinite(h)) h = 0.f;
    s = std::clamp(s, 0.f, 1.f);
    l = std::clamp(l, 0.f, 1.f);
    if (s <= 0.f) { r = g = b = l; return; }

    auto hue2rgb = [](float p, float q, float t) {
        if (t < 0.f) t += 1.f;
        if (t > 1.f) t -= 1.f;
        if (t < 1.f / 6.f) return p + (q - p) * 6.f * t;
        if (t < 1.f / 2.f) return q;
        if (t < 2.f / 3.f) return p + (q - p) * (2.f / 3.f - t) * 6.f;
        return p;
    };

    float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
    float p = 2.f * l - q;
    r = hue2rgb(p, q, h + 1.f / 3.f);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1.f / 3.f);
}

ImColor ColorUtils::adjustLightness(const ImColor& color, float targetL)
{
    float h, s, l, r, g, b;
    rgbToHsl(color.Value.x, color.Value.y, color.Value.z, h, s, l);
    hslToRgb(h, s, std::clamp(targetL, 0.f, 1.f), r, g, b);
    return ImColor(r, g, b, color.Value.w);
}

ImColor ColorUtils::lighten(const ImColor& color, float amount)
{
    float h, s, l, r, g, b;
    rgbToHsl(color.Value.x, color.Value.y, color.Value.z, h, s, l);
    hslToRgb(h, s, std::clamp(l + amount, 0.f, 1.f), r, g, b);
    return ImColor(r, g, b, color.Value.w);
}

ImColor ColorUtils::saturate(const ImColor& color, float amount)
{
    float h, s, l, r, g, b;
    rgbToHsl(color.Value.x, color.Value.y, color.Value.z, h, s, l);
    hslToRgb(h, std::clamp(s * amount, 0.f, 1.f), l, r, g, b);
    return ImColor(r, g, b, color.Value.w);
}
// ==== end of ported block ====
