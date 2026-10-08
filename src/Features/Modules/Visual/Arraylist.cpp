#include "Arraylist.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kRightMargin = 8.f;
    constexpr float kTopMargin   = 10.f;
    constexpr float kBarWidth    = 2.f;
    constexpr float kBarGap      = 5.f;   // расстояние от текста до полоски
    constexpr float kNameGap     = 6.f;   // расстояние между названием и режимом
    constexpr float kModeRatio   = 0.82f; // режим рисуется чуть меньше названия

    // Цвет из HSL. Оттенок здесь — координата на круге: 0 и 1 — один и тот же
    // цвет, поэтому волну можно двигать бесконечно и она зациклится без рывка.
    ImColor arraylistHsl(float h, float s, float l, float alpha = 1.f)
    {
        h = h - std::floor(h);

        float r = 0.f, g = 0.f, b = 0.f;
        ColorUtils::hslToRgb(h, std::clamp(s, 0.f, 1.f), std::clamp(l, 0.f, 1.f), r, g, b);
        return ImColor(r, g, b, alpha);
    }
}

Arraylist::Palette Arraylist::paletteFor(float t, float time) const
{
    t = MathUtils::clamp(t, 0.f, 1.f);

    // ── Основа пресета: оттенок, насыщенность, светлота и ширина градиента ────
    // Ширина градиента = на сколько «разъезжаются» оттенки от первой строки к
    // последней. Именно она превращает список в переливающуюся радугу, а не в
    // один сплошной цвет.
    float baseHue = 0.55f, sat = 0.62f, light = 0.76f;
    float span = MathUtils::clamp(mGradient.mValue, 0.f, 0.6f);

    switch (mTheme.as<Preset>())
    {
        // Ice: воздушно-голубой, книзу уходит в сиреневый — тот самый вид с референса.
        // Оттенок намеренно не совпадает с акцентом темы Interface (бирюза ~0.47),
        // иначе Auto и Ice выглядели бы одинаково.
        case Preset::Ice:      baseHue = 0.58f; sat = 0.92f; light = 0.79f; break; // голубой
        case Preset::Amethyst: baseHue = 0.73f; sat = 0.82f; light = 0.79f; break; // фиолетовый
        case Preset::Emerald:  baseHue = 0.40f; sat = 0.72f; light = 0.75f; break; // изумрудный
        case Preset::Gold:     baseHue = 0.12f; sat = 0.90f; light = 0.77f; break; // золотой
        case Preset::Crimson:  baseHue = 0.99f; sat = 0.84f; light = 0.77f; break; // алый
        case Preset::Mono:
            // Mono — единственный статичный пресет: без радуги, только приятная
            // серая шкала от белого к тёмно-серому.
            baseHue = 0.60f; sat = 0.04f; light = 0.92f - t * 0.42f;
            span = 0.f;
            break;
        case Preset::Custom:
        {
            // Custom: оттенок из первого цвета, а ширина волны — разница оттенков
            // между Primary и Secondary (но не меньше градиента из настроек).
            float ph = 0.f, ps = 0.f, pl = 0.f;
            float sh = 0.f, ss = 0.f, sl = 0.f;
            ColorUtils::rgbToHsl(mPrimaryColor.mValue[0], mPrimaryColor.mValue[1], mPrimaryColor.mValue[2], ph, ps, pl);
            ColorUtils::rgbToHsl(mSecondaryColor.mValue[0], mSecondaryColor.mValue[1], mSecondaryColor.mValue[2], sh, ss, sl);

            // Насыщенность и светлота второго цвета не нужны — он задаёт только
            // оттенок, до которого доезжает градиент.
            (void)ss;
            (void)sl;

            float diff = sh - ph;
            diff = diff - std::floor(diff + 0.5f);
            if (diff < 0.f) diff = -diff;

            baseHue = ph;
            sat = (std::max)(ps, 0.35f);
            light = MathUtils::clamp((std::max)(pl, 0.55f), 0.45f, 0.86f);
            span = MathUtils::clamp((std::max)(span, diff), 0.05f, 0.6f);
            break;
        }
        case Preset::Auto:
        default:
        {
            // Auto подхватывает тему Interface: берём оттенок её акцента, но
            // подтягиваем светлоту к пастельной — иначе градиент получается грязным.
            float h = 0.f, s = 0.f, l = 0.f;
            const ImColor accent = ColorUtils::getStaticAccentColor(0.f);
            ColorUtils::rgbToHsl(accent.Value.x, accent.Value.y, accent.Value.z, h, s, l);

            baseHue = h;
            sat = MathUtils::clamp(s, 0.45f, 0.95f);
            light = MathUtils::clamp(l + 0.14f, 0.62f, 0.84f);
            break;
        }
    }

    // ── Волна ────────────────────────────────────────────────────────────────
    // Оттенок строки = оттенок темы + её место в списке + лёгкое КОЛЕБАНИЕ.
    //
    // Раньше сюда просто прибавлялось время (baseHue + time * speed), и это была
    // ошибка: за ~20 секунд весь список прокручивался через весь цветовой круг,
    // поэтому любая тема через полминуты выглядела одинаково-радужной, и смена
    // темы была незаметна. Теперь время двигает оттенок туда-обратно (sin) в
    // пределах ±0.06 — список переливается, но остаётся узнаваемо «ледяным»,
    // «золотым» и т.д. Скорость 0 = статичный градиент.
    const float speed = std::clamp(mFlowSpeed.mValue, 0.f, 3.f);

    // Волна БЕЖИТ по списку: t сидит в фазе, поэтому оттенок не просто качается
    // весь сразу, а как будто течёт сверху вниз — именно этот перелив и просили.
    // Амплитуда ±0.10 видна даже тогда, когда в списке одна-две строки (раньше
    // на одной строке перелива не было вообще — градиент, который шёл от позиции
    // строки, при единственной строке ничего не давал).
    const float flow = std::sin(time * (0.55f + speed * 0.55f) - t * 3.1f) * 0.10f * speed;
    const float hue = baseHue + t * span + flow;

    Palette palette;

    // Название — светлый пастельный тон темы: сверху почти белый, книзу плотнее и
    // насыщеннее. Список читается на любом фоне и ведёт цветом сверху вниз, как на
    // референсе (там верхние строки почти белые, нижние — цветные).
    palette.name = arraylistHsl(hue,
                                std::clamp(sat * (0.52f + 0.34f * t), 0.f, 1.f),
                                MathUtils::clamp(light + 0.09f - 0.11f * t, 0.50f, 0.95f));

    // Полоска справа — самый плотный оттенок темы: по ней видно, какая тема
    // выбрана, даже когда в списке всего одна строка. В референсе полоски тоже
    // цветные и заметно насыщеннее самого текста.
    palette.bar = arraylistHsl(hue,
                               MathUtils::clamp(sat + 0.10f, 0.f, 1.f),
                               MathUtils::clamp(light - 0.18f, 0.35f, 0.90f));

    // Режим («Instant», «Hive») — почти белый серый с лёгким оттенком темы: он не
    // должен спорить с названием модуля.
    palette.mode = arraylistHsl(hue, 0.16f, MathUtils::clamp(light * 0.93f, 0.45f, 0.86f));
    return palette;
}

void Arraylist::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Arraylist::onRenderEvent>(this);
}

void Arraylist::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Arraylist::onRenderEvent>(this);

    for (auto& mod : gFeatureManager->mModuleManager->getModules())
    {
        mod->mArrayListAnim = 0.f;
        mod->pos.y = -999.f;
    }
}

namespace
{
    struct Row
    {
        Module*     mod   = nullptr;
        std::string name;
        std::string mode;
        float nameW = 0.f;
        float modeW = 0.f;
        float totalW = 0.f;
        float y = -999.f;
    };
}

void Arraylist::onRenderEvent(RenderEvent& event)
{
    const float delta = ImGui::GetIO().DeltaTime;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    FontHelper::pushPrefFont(false, false, false);
    ImFont* font = ImGui::GetFont();
    if (!font)
    {
        ImGui::PopFont();
        return;
    }

    const float fontSize = std::clamp(mFontSize.mValue, 8.f, 30.f);
    const float modeSize = fontSize * kModeRatio;
    const float rowHeight = fontSize * 1.18f;
    const float textRight = display.x - kRightMargin - kBarWidth - kBarGap;

    // ── Анимация появления/исчезновения ───────────────────────────────────────
    for (auto& mod : gFeatureManager->mModuleManager->getModules())
    {
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;

        const float target = mod->mEnabled ? 1.f : 0.f;
        const float speed  = target > mod->mArrayListAnim ? 12.f : 7.f;
        mod->mArrayListAnim = MathUtils::clamp(MathUtils::lerp(mod->mArrayListAnim, target, delta * speed), 0.f, 1.f);
    }

    // ── Сбор строк ────────────────────────────────────────────────────────────
    std::vector<Row> rows;
    rows.reserve(48);

    for (auto& mod : gFeatureManager->mModuleManager->getModules())
    {
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;
        if (mod->mArrayListAnim < 0.01f) continue;

        Row row;
        row.mod  = mod.get();
        row.name = mod->getName();
        row.mode = mShowMode.mValue ? mod->getSettingDisplayText() : std::string{};

        row.nameW = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, row.name.c_str()).x;
        row.modeW = row.mode.empty() ? 0.f : font->CalcTextSizeA(modeSize, FLT_MAX, 0.f, row.mode.c_str()).x;
        row.totalW = row.nameW + (row.mode.empty() ? 0.f : kNameGap + row.modeW);
        row.y = mod->pos.y;

        rows.push_back(std::move(row));
    }

    // Классика: самое длинное название сверху.
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.totalW > b.totalW; });

    // ── Раскладка ─────────────────────────────────────────────────────────────
    float currentY = kTopMargin;
    for (auto& row : rows)
    {
        if (row.y < -900.f) row.y = currentY;
        row.y = MathUtils::lerp(row.y, currentY, std::min(delta * 16.f, 1.f));
        if (std::abs(row.y - currentY) < 0.1f) row.y = currentY;
        currentY += rowHeight * row.mod->mArrayListAnim;
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool clickable = ImGui::IsMouseClicked(0);

    // ── Рендер ────────────────────────────────────────────────────────────────
    // Время нужно и волне (палитра), и ей же для плавности перелива.
    const float time = static_cast<float>(ImGui::GetTime());

    size_t index = 0;
    for (auto& row : rows)
    {
        const float alpha = row.mod->mArrayListAnim;
        if (alpha < 0.01f) { index++; continue; }

        const float position = rows.size() > 1
            ? static_cast<float>(index) / static_cast<float>(rows.size() - 1)
            : 0.f;
        const Palette palette = paletteFor(position, time);

        const float x = textRight - row.totalW;
        const float y = row.y;
        // Название и режим набраны разным кеглем. Чтобы режим не «висел» ниже или
        // выше, опускаем его ровно на разницу их базовых линий: базовые линии
        // обоих текстов совпадают при любом размере шрифта.
        const float modeY = y + font->Ascent * (fontSize - modeSize) / font->FontSize;

        // Наведение: строка просто светлеет, без подъёмов и теней.
        const bool hovered = mouse.x >= x - 4.f && mouse.x <= textRight &&
                             mouse.y >= y - 1.f && mouse.y <= y + rowHeight;

        auto tint = [alpha, hovered](const ImColor& base) {
            ImColor c = hovered ? ColorUtils::lighten(base, 0.08f) : base;
            c.Value.w = alpha;
            return c;
        };

        // Название с лёгкой тенью — список читается на любом фоне.
        drawList->AddText(font, fontSize, { x + 1.f, y + 1.f },
                          ImColor(0.f, 0.f, 0.f, 0.45f * alpha), row.name.c_str());
        drawList->AddText(font, fontSize, { x, y }, tint(palette.name), row.name.c_str());

        if (!row.mode.empty())
        {
            drawList->AddText(font, modeSize, { x + row.nameW + kNameGap, modeY },
                              tint(palette.mode), row.mode.c_str());
        }

        // Полоска-акцент на всю высоту строки.
        const float barX = textRight + kBarGap;
        drawList->AddRectFilled({ barX, y }, { barX + kBarWidth, y + rowHeight - 2.f }, tint(palette.bar));

        if (hovered && clickable)
            row.mod->toggle();

        row.mod->pos.y = row.y;
        index++;
    }

    ImGui::PopFont();
}
