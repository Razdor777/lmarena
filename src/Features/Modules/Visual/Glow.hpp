#pragma once

//
// Glow — мягкое свечение вокруг игрока.
//
// Почему свой, а не ванильный: в Bedrock у актора нет флага «светящийся», а
// единственная строка с похожим именем — HideGlowOutline — относится к
// BLOCK-актору таблички (sub_143D13910: IgnoreLighting, HideGlowOutline,
// SignTextColor, PersistFormatting, TextOwner, Text, FilteredText,
// FrontText/BackText, IsWaxed). К игрокам она отношения не имеет.
// Поэтому контур рисуем сами тем же способом, что ESP: тот же проекционный
// путь (MathUtils::getImBoxPoints), только вместо одной линии — несколько
// проходов с падающей прозрачностью, из-за чего край «плывёт».
//
class Glow : public ModuleBase<Glow>
{
public:
    NumberSetting mRange = NumberSetting("Range", "До какого расстояния светить", 64.f, 4.f, 256.f, 1.f);
    NumberSetting mSpread = NumberSetting("Spread", "Насколько далеко расходится свечение", 3.f, 1.f, 10.f, 1.f);
    BoolSetting mFill = BoolSetting("Fill", "Лёгкая заливка внутри контура", false);

    ColorSetting mColor = ColorSetting("Color", "Цвет свечения", 1.0f, 1.0f, 1.0f, 0.55f);

    Glow() : ModuleBase("Glow", "Soft glow around players", ModuleCategory::Visual, 0, false)
    {
        addSettings(&mRange, &mSpread, &mFill, &mColor);

        mNames = {
            {Lowercase,       "glow"},
            {LowercaseSpaced, "glow"},
            {Normal,          "Glow"},
            {NormalSpaced,    "Glow"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override
    {
        return std::to_string(static_cast<int>(mRange.mValue)) + "m";
    }
};
