#pragma once

class FontHelper {
public:
    static inline std::map<std::string, ImFont*> Fonts;
    static inline std::string currentFontKey = "open_sans";
    static inline float fontScale = 1.0f;

    static void load();
    static void pushPrefFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static ImFont* getFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static void popPrefFont();
    static void setCurrentFont(const std::string& fontKey);
    static void setFontScale(float scale);

    // Получить масштабированный размер текста
    static float getScaledSize(float baseSize);
};