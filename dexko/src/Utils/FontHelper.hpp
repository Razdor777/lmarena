#pragma once

class FontHelper {
public:
    static inline std::map<std::string, ImFont*> Fonts;
    // Единственный реальный шрифт интерфейса — Mntsb (робото нужен только как
    // донор глифов, в списке шрифтов его нет — см. FontHelper::load()).
    static inline std::string currentFontKey = "mntsb";
    static inline float fontScale = 1.0f;

    static void load();
    static void pushPrefFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static ImFont* getFont(bool large = false, bool bold = false, bool mForcePSans = false);
    static void popPrefFont();
    static void setCurrentFont(const std::string& fontKey);
    static void setFontScale(float scale);

    static float getScaledSize(float baseSize);

    // Иконочный шрифт Nurik (в нём картинки, а не текст).
    // Возвращает nullptr, если шрифт не загрузился, поэтому вызывающий код
    // обязан это проверять.
    static ImFont* getIconFont(bool large = false);
};