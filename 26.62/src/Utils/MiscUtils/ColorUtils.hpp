#pragma once
//
// Created by vastrakai on 7/1/2024.
//

#include <chrono>
#include <vector>

#define NOW std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

class ColorUtils {
public:
    static ImColor Rainbow(float seconds, float saturation, float brightness, int index);
    static ImColor LerpColors(float seconds, float index, const std::vector<ImColor>& colors, uint64_t ms = 0);
    static ImColor getThemedColor(float index, uint64_t ms = 0);
    static std::string removeColorCodes(const std::string& text);

    // ── Themed accents (used by Arraylist, ClickGui, Notifications...) ──
    // Themed color with only the wave applied (no pulse/shimmer) — for UI chrome
    // that shouldn't strobe while clicking.
    static ImColor getStaticAccentColor(float index = 0.f);
    // Pre-multiplied alpha variants of the static accent, ready for draw calls.
    static ImColor getStaticAccentColorAlpha(float index, float alpha);
    // Picks black or white text depending on the backdrop luminance.
    static ImColor getContrastTextColor(const ImColor& backdrop, float alpha = 1.f);
    // Relative luminance of a color (WCAG-ish), 0..1.
    static float getLuminance(const ImColor& color);

    // Low-level helpers, also handy for modules that build custom gradients.
    static ImColor adjustLightness(const ImColor& color, float targetL); // absolute 0..1
    static ImColor lighten(const ImColor& color, float amount);          // relative -1..1
    static ImColor saturate(const ImColor& color, float amount);
    static void rgbToHsl(float r, float g, float b, float& h, float& s, float& l);
    static void hslToRgb(float h, float s, float l, float& r, float& g, float& b);
};