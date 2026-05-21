#pragma once

class FontHelper {
public:
    static inline std::map<std::string, ImFont*> Fonts;
    static inline std::string currentFontKey = "roboto";
    static inline float fontScale = 1.0f;

    static void load();
    static void pushPrefFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static ImFont* getFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static void popPrefFont();
    static void setCurrentFont(const std::string& fontKey);
    static void setFontScale(float scale);

    static float getScaledSize(float baseSize);
};