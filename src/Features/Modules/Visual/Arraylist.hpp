#pragma once
#include <Features/Modules/Module.hpp>

//
// Arraylist — простой правый список активных модулей.
//
// Вид (как на референсе): справа по вертикали,
//     «Название   режим»  ▌
// где название окрашено переливающимся градиентом, режим — приглушённый тон того
// же оттенка, а справа тонкая полоска-акцент.
//
// Название и режим набраны разным кеглем, но выровнены ПО БАЗОВОЙ ЛИНИИ — иначе
// режим «висит» выше или ниже названия.
// Никаких фонов, свечений, шиммера, пульсаций и блюра.
//
// Настройки: видимость, режим, пресет темы (у arraylist свои, независимые от
// Interface), два цвета для пресета Custom, ширина градиента, скорость перелива
// и размер шрифта.
//
class Arraylist : public ModuleBase<Arraylist>
{
public:
    enum class ModuleVisibility {
        All,
        Bound,
    };

    // Пресеты темы для arraylist. Auto подхватывает палитру Interface,
    // остальные — фиксированные цвета, чтобы список не зависел от общей темы.
    enum class Preset {
        Auto = 0,
        Ice,
        Amethyst,
        Emerald,
        Gold,
        Crimson,
        Mono,
        Custom,
    };

    // Цвета одной строки. Палитра НЕ сплошная: paletteFor() получает позицию
    // строки в списке (0 — верхняя, 1 — нижняя) и время. От позиции идёт сам
    // градиент темы (как на референсе — верхние строки почти белые, нижние
    // цветные), а время гонит по списку волну оттенка (t сидит в фазе), поэтому
    // перелив виден даже на одной строке. Тема при этом остаётся собой: лёд —
    // ледяным, золото — золотым.
    struct Palette {
        ImColor name;
        ImColor mode;
        ImColor bar;
    };

    EnumSettingT<ModuleVisibility> mVisibility = EnumSettingT("Visibility", "Which modules are shown", ModuleVisibility::All, "All", "Bound");
    BoolSetting mShowMode = BoolSetting("Show Mode", "Show the module's mode next to its name", true);
    EnumSettingT<Preset> mTheme = EnumSettingT<Preset>("Theme", "Arraylist-only theme preset. Auto follows the Interface theme", Preset::Auto, "Auto", "Ice", "Amethyst", "Emerald", "Gold", "Crimson", "Mono", "Custom");
    ColorSetting mPrimaryColor = ColorSetting("Primary", "Hue the gradient starts from (Custom theme only)", 0.78f, 0.55f, 1.00f, 1.00f);
    ColorSetting mSecondaryColor = ColorSetting("Secondary", "Hue the gradient ends at (Custom theme only)", 0.62f, 0.63f, 0.68f, 1.00f);
    NumberSetting mGradient = NumberSetting("Gradient", "How far the hue travels from the top row to the bottom row", 0.30f, 0.05f, 0.60f, 0.01f);
    NumberSetting mFlowSpeed = NumberSetting("Flow Speed", "How strongly the colours shimmer (0 = static gradient)", 1.f, 0.f, 3.f, 0.1f);
    NumberSetting mFontSize = NumberSetting("Font Size", "Text size of the list", 15.f, 8.f, 30.f, 0.5f);

    Arraylist() : ModuleBase("Arraylist", "Displays a list of active modules", ModuleCategory::Visual, 0, true)
    {
        addSettings(&mVisibility, &mShowMode, &mTheme, &mPrimaryColor, &mSecondaryColor, &mGradient, &mFlowSpeed, &mFontSize);

        VISIBILITY_CONDITION(mPrimaryColor, mTheme.mValue == Preset::Custom);
        VISIBILITY_CONDITION(mSecondaryColor, mTheme.mValue == Preset::Custom);

        mNames = {
            {Lowercase, "arraylist"},
            {LowercaseSpaced, "array list"},
            {Normal, "Arraylist"},
            {NormalSpaced, "Array List"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override
    {
        const int index = mTheme.as<int>();
        if (index < 0 || index >= static_cast<int>(mTheme.mValues.size())) return "";
        return mTheme.mValues[static_cast<size_t>(index)];
    }

private:
    // t — положение строки (0 = первая, 1 = последняя), time — секунды с запуска.
    Palette paletteFor(float t, float time) const;
};
