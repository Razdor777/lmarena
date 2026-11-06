#pragma once
//
// Created by vastrakai on 7/1/2024.
//

#include <chrono>

#define NOW std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()

class ColorUtils {
public:
    static ImColor Rainbow(float seconds, float saturation, float brightness, int index);
    static ImColor LerpColors(float seconds, float index, std::vector<ImColor> colors, uint64_t ms = 0);
    static ImColor getThemedColor(float index, uint64_t ms = 0);
    static std::string removeColorCodes(const std::string& text);

    // ==== ported from the 1.21.44 master ====
    // Используются портированными модулями (Glint, ESP, TargetHUD, CombatantGui,
    // InfiniteAura) для акцентных цветов интерфейса.
    static ImColor getStaticAccentColor(float index = 0.f);
    static ImColor getStaticAccentColorAlpha(float index, float alpha);
    static ImColor saturate(const ImColor& color, float amount);
    static ImColor lighten(const ImColor& color, float amount);
    static ImColor adjustLightness(const ImColor& color, float targetL);
    static void rgbToHsl(float r, float g, float b, float& h, float& s, float& l);
    static void hslToRgb(float h, float s, float l, float& r, float& g, float& b);
    // ==== end of ported block ====

};
