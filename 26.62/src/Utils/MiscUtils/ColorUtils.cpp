#include "ColorUtils.hpp"

#include <algorithm>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Interface.hpp>

namespace {
    constexpr float kPi = 3.14159265f;

    bool finite4(const ImColor& c)
    {
        return std::isfinite(c.Value.x) && std::isfinite(c.Value.y)
            && std::isfinite(c.Value.z) && std::isfinite(c.Value.w);
    }

    ImColor sanitize(ImColor c)
    {
        if (!finite4(c)) return ImColor(1.f, 1.f, 1.f, 1.f);
        c.Value.x = std::clamp(c.Value.x, 0.f, 1.f);
        c.Value.y = std::clamp(c.Value.y, 0.f, 1.f);
        c.Value.z = std::clamp(c.Value.z, 0.f, 1.f);
        c.Value.w = std::clamp(c.Value.w, 0.f, 1.f);
        return c;
    }

    ImColor lerpColor(const ImColor& a, const ImColor& b, float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return ImColor(
            a.Value.x + (b.Value.x - a.Value.x) * t,
            a.Value.y + (b.Value.y - a.Value.y) * t,
            a.Value.z + (b.Value.z - a.Value.z) * t,
            a.Value.w + (b.Value.w - a.Value.w) * t
        );
    }

    // Refined, adult color palettes: harmonious, sophisticated, and comfortable in dark mode
    const std::vector<ImColor>& paletteFor(Interface::ColorTheme theme)
    {
        static const std::vector<ImColor> aurora{
            ImColor(68, 192, 174),  // Refined Arctic Mint
            ImColor(82, 140, 226),  // Muted Nordic Azure
            ImColor(136, 118, 220), // Elegant Lavender Slate
            ImColor(202, 112, 170)  // Soft Rose Plum
        };
        static const std::vector<ImColor> abyss{
            ImColor(52, 108, 226),  // Deep Naval Sapphire
            ImColor(46, 170, 210),  // Steel Glacier Cyan
            ImColor(54, 184, 162),  // Ocean Seafoam
            ImColor(82, 124, 214)   // Muted Cobalt Sky
        };
        static const std::vector<ImColor> ember{
            ImColor(226, 88, 64),   // Warm Terracotta / Burnt Crimson
            ImColor(228, 142, 60),  // Warm Muted Amber
            ImColor(210, 80, 110),  // Deep Antique Coral
            ImColor(222, 182, 80)   // Muted Antique Gold
        };
        static const std::vector<ImColor> blossom{
            ImColor(222, 126, 164), // Dusky Sakura
            ImColor(170, 134, 216), // Muted Wisteria Mauve
            ImColor(220, 160, 186), // Soft Pastel Blush
            ImColor(134, 152, 216)  // Slate Periwinkle
        };

        switch (theme) {
        case Interface::Abyss:   return abyss;
        case Interface::Ember:   return ember;
        case Interface::Blossom: return blossom;
        default:                 return aurora;
        }
    }
}

ImColor ColorUtils::Rainbow(float seconds, float saturation, float brightness, int index)
{
    const float safeSeconds = std::max(seconds, 0.05f);
    const int period = std::max(1, static_cast<int>(safeSeconds * 1000.f));
    float hue = static_cast<float>((NOW + index) % period) / static_cast<float>(period);
    return ImColor::HSV(hue, std::clamp(saturation, 0.f, 1.f), std::clamp(brightness, 0.f, 1.f));
}

ImColor ColorUtils::LerpColors(float seconds, float index, const std::vector<ImColor>& colors, uint64_t ms)
{
    if (colors.empty()) return ImColor(255, 255, 255, 255);
    if (colors.size() == 1) return colors[0];

    seconds = std::max(seconds, 0.05f);
    const double period = 10000.0 / static_cast<double>(seconds);
    const double now = static_cast<double>(ms == 0 ? NOW : ms);
    const double idx = std::isfinite(index) ? static_cast<double>(index) : 0.0;

    double angle = std::fmod(now + idx, period);
    if (angle < 0.0) angle += period;

    const double seg = period / static_cast<double>(colors.size());
    int i = static_cast<int>(angle / seg) % static_cast<int>(colors.size());
    if (i < 0) i += static_cast<int>(colors.size());

    float t = static_cast<float>((angle / seg) - static_cast<double>(i));
    t = std::clamp(t, 0.f, 1.f);
    return lerpColor(colors[i], colors[(i + 1) % colors.size()], t);
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

float ColorUtils::getLuminance(const ImColor& color)
{
    auto lin = [](float c) { return c <= 0.03928f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); };
    return 0.2126f * lin(color.Value.x) + 0.7152f * lin(color.Value.y) + 0.0722f * lin(color.Value.z);
}

ImColor ColorUtils::getContrastTextColor(const ImColor& backdrop, float alpha)
{
    float lum = getLuminance(backdrop);
    return lum > 0.58f ? ImColor(0.06f, 0.06f, 0.08f, alpha) : ImColor(0.97f, 0.97f, 1.f, alpha);
}

static ImColor getBaseColor(float index, uint64_t ms)
{
    auto* ui = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<Interface>()
        : nullptr;
    if (!ui) return ImColor(255, 255, 255, 255);

    const uint64_t sampleTime = ms == 0 ? NOW : ms;
    const auto theme = ui->mMode.mValue;
    const float speed = std::max(ui->mColorSpeed.mValue, 0.05f);
    const float flow = ui->mGradientFlow.mValue
        ? std::clamp(ui->mFlowDepth.mValue, 0.f, 1.f) : 0.f;
    const float sat = std::clamp(ui->mSaturation.mValue, 0.f, 1.f);
    const float safeIndex = std::isfinite(index) ? index : 0.f;

    ImColor color;
    if (theme == Interface::Prism) {
        float staticHue = std::fmod(safeIndex * 0.0018f, 1.f);
        if (staticHue < 0.f) staticHue += 1.f;
        float animatedHue = std::fmod(staticHue + static_cast<float>(sampleTime) * 0.000020f * speed, 1.f);
        if (animatedHue < 0.f) animatedHue += 1.f;
        ImColor still = ImColor::HSV(staticHue, sat * 0.82f, 0.94f);
        ImColor moving = ImColor::HSV(animatedHue, sat * 0.82f, 0.94f);
        color = lerpColor(still, moving, flow);
    } else {
        const auto& colors = paletteFor(theme);
        const float paletteIndex = safeIndex * 9.f;
        ImColor still = ColorUtils::LerpColors(speed, paletteIndex, colors, 1);
        ImColor moving = ColorUtils::LerpColors(speed, paletteIndex, colors, sampleTime);
        color = lerpColor(still, moving, flow);

        float h, s, l, r, g, b;
        ColorUtils::rgbToHsl(color.Value.x, color.Value.y, color.Value.z, h, s, l);
        ColorUtils::hslToRgb(h, std::clamp(s * sat, 0.f, 1.f), l, r, g, b);
        color = ImColor(r, g, b, color.Value.w);
    }
    return sanitize(color);
}

static ImColor applyWave(const ImColor& color, float index, uint64_t ms)
{
    auto* im = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<Interface>()
        : nullptr;
    if (!im || !im->mColorWave.mValue) return color;

    float speed = im->mWaveSpeed.mValue;
    float spacing = std::max(im->mWaveSpacing.mValue, 1.f);
    float phase = std::fmod(index / spacing + static_cast<float>(ms) * 0.001f * speed * 0.25f, 1.f);
    if (!std::isfinite(phase)) return color;
    float shift = std::sin(phase * kPi * 2.f) * 0.045f;

    float h, s, l, r, g, b;
    ColorUtils::rgbToHsl(color.Value.x, color.Value.y, color.Value.z, h, s, l);
    ColorUtils::hslToRgb(h + shift, s, l, r, g, b);
    return ImColor(r, g, b, color.Value.w);
}

static ImColor applyPulse(const ImColor& color, uint64_t ms)
{
    auto* im = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<Interface>()
        : nullptr;
    if (!im || !im->mPulse.mValue) return color;

    float speed = im->mPulseSpeed.mValue;
    float depth = std::clamp(im->mPulseStrength.mValue, 0.f, 0.6f);
    float pulse = 0.5f * (1.f + std::sin(static_cast<float>(ms) * 0.001f * speed * kPi));
    float k = 1.f + (pulse - 0.5f) * 2.f * depth;

    return ImColor(
        std::clamp(color.Value.x * k, 0.f, 1.f),
        std::clamp(color.Value.y * k, 0.f, 1.f),
        std::clamp(color.Value.z * k, 0.f, 1.f),
        color.Value.w);
}

static ImColor applyShimmer(const ImColor& color, float index, uint64_t ms)
{
    auto* im = gFeatureManager && gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<Interface>()
        : nullptr;
    if (!im || !im->mShimmer.mValue) return color;

    float t = std::fmod(static_cast<float>(ms) * 0.001f * 0.5f + std::fmod(index, 1000.f) * 0.0007f, 4.f);
    if (!std::isfinite(t) || t > 0.45f) return color;

    float s = std::sin((t / 0.45f) * kPi);
    float k = 1.f + s * 0.22f;

    return ImColor(
        std::clamp(color.Value.x * k, 0.f, 1.f),
        std::clamp(color.Value.y * k, 0.f, 1.f),
        std::clamp(color.Value.z * k, 0.f, 1.f),
        color.Value.w);
}

ImColor ColorUtils::getThemedColor(float index, uint64_t ms)
{
    const uint64_t sampleTime = ms == 0 ? NOW : ms;
    ImColor color = getBaseColor(index, sampleTime);
    color = applyWave(color, index, sampleTime);
    color = applyShimmer(color, index, sampleTime);
    color = applyPulse(color, sampleTime);
    return sanitize(color);
}

ImColor ColorUtils::getStaticAccentColor(float index)
{
    // Chrome only: palette + optional flow. No wave/pulse/shimmer —
    // those were applied to every panel/icon and blew up on theme swaps.
    return sanitize(getBaseColor(index, NOW));
}

ImColor ColorUtils::getStaticAccentColorAlpha(float index, float alpha)
{
    ImColor c = getStaticAccentColor(index);
    c.Value.w = std::clamp(alpha, 0.f, 1.f);
    return c;
}

namespace
{
    // Настоящий код цвета/стиля Minecraft: 0-9, a-f, k-o, r.
    bool colorUtilsIsCodeChar(unsigned char c)
    {
        if (c >= '0' && c <= '9') return true;

        const unsigned char lower = (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + 32) : c;
        if (lower >= 'a' && lower <= 'f') return true;

        return lower == 'k' || lower == 'l' || lower == 'm' ||
               lower == 'n' || lower == 'o' || lower == 'r';
    }
}

std::string ColorUtils::removeColorCodes(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); )
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == 0xC2 && i + 2 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0xA7)
        {
            // Выбрасываем байт кода только если это реально код цвета/стиля.
            i += colorUtilsIsCodeChar(static_cast<unsigned char>(text[i + 2])) ? 3 : 2;
            continue;
        }
        // 0xA7 бывает и хвостовым байтом кириллицы ('Ч' = D0 A7) — в таком
        // случае это не «§», и его нельзя выбрасывать вместе с соседним байтом.
        if (c == 0xA7 && (i == 0 || static_cast<unsigned char>(text[i - 1]) < 0xC0))
        {
            i += 1;
            if (i < text.size() && colorUtilsIsCodeChar(static_cast<unsigned char>(text[i]))) i += 1;
            continue;
        }
        out += text[i++];
    }
    return out.empty() ? text : out;
}
