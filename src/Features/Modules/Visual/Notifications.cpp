//
// Notifications — минималистичные чёрные карточки с цветной полоской-акцентом.
//
// Что убрано по сравнению со старой версией (и почему):
//   • ImRenderUtils::addBlur на каждой карточке — самая дорогая операция кадра;
//   • двойной AddShadowRect «glow» на каждой карточке;
//   • shimmer-полоса, летающая рамка, таймер-кольцо, спарклы, эластичные пружины;
//   • ~20 настроек цвета/плотности/анимаций;
//   • повторный strip §-кодов и пересчёт ширин текста КАЖДЫЙ кадр;
//   • ВСЕ пиктограммы (галочки, кресты, «!», «i», глифы шрифтов) — вместо них
//     у левого края карточки стоит цветная полоска: тип события читается по цвету,
//     и карточка остаётся чистой даже на 15 px текста.
//
// Теперь карточка строится один раз (текст очищен, ширина посчитана), а в кадре
// остаются только AddRectFilled / AddRect / AddText.
//

#include "Notifications.hpp"

#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Events/NotifyEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Utils/FontHelper.hpp>

#include <algorithm>
#include <string>

namespace
{
    // Метрики фиксированы — никаких настроек масштаба и плотности.
    constexpr float kPadX      = 10.f;
    constexpr float kPadY      = 7.f;
    constexpr float kStripW    = 3.f;    // толщина цветной полоски-акцента
    constexpr float kStripGap  = 10.f;   // от полоски до текста
    constexpr float kCardGap   = 4.f;
    constexpr float kMargin       = 8.f;
    constexpr float kRounding    = 3.f;
    constexpr float kInTime      = 0.14f;
    constexpr float kOutTime     = 0.12f;
    constexpr float kFontSize    = 15.f;
    constexpr int   kMaxVisible  = 6;
    constexpr size_t kMaxStored  = 16;

    // Цвет полоски-акцента. Оттенки пастельные и приглушённые: они не спорят с
    // чёрной карточкой, но тип события читается сразу. Пиктограмм больше нет —
    // цвет и подпись справа («enabled» / «disabled») говорят сами за себя.
    ImU32 accentColor(Notifications::Icon icon, float alpha)
    {
        const int a = static_cast<int>(std::clamp(alpha, 0.f, 1.f) * 255.f);
        switch (icon)
        {
            case Notifications::Icon::Success:  return IM_COL32(150, 226, 172, a); // мята — включено
            case Notifications::Icon::Disabled: return IM_COL32(146, 149, 158, a); // серый — выключено
            case Notifications::Icon::Warning:  return IM_COL32(240, 200, 120, a); // янтарь — предупреждение
            case Notifications::Icon::Error:    return IM_COL32(240, 138, 138, a); // коралл — ошибка
            case Notifications::Icon::Info:
            default:                            return IM_COL32(198, 214, 246, a); // холодный белый — инфо
        }
    }

    // Настоящий код цвета/стиля Minecraft: 0-9, a-f, k-o, r. После «§» байт
    // выбрасывается только если это код — иначе одинокий «§» съедал бы букву.
    bool notificationsIsCodeChar(unsigned char c)
    {
        if (c >= '0' && c <= '9') return true;

        const unsigned char lower = (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + 32) : c;
        if (lower >= 'a' && lower <= 'f') return true;

        return lower == 'k' || lower == 'l' || lower == 'm' ||
               lower == 'n' || lower == 'o' || lower == 'r';
    }

    // Убирает §-коды (включая UTF-8 «Â§»), служебные символы и лишние пробелы.
    // ВАЖНО: строки кэшируются на момент создания карточки, поэтому серверные
    // префиксы и разноцветные ники не превращаются в «§» в интерфейсе.
    std::string cleanText(const std::string& src)
    {
        std::string out;
        out.reserve(src.size());

        for (size_t i = 0; i < src.size(); )
        {
            const unsigned char c = static_cast<unsigned char>(src[i]);

            // UTF-8 «§» = C2 A7
            if (c == 0xC2 && i + 1 < src.size() && static_cast<unsigned char>(src[i + 1]) == 0xA7)
            {
                i += 2;
                // за кодом цвета может идти сам код (0-9a-fk-or)
                if (i < src.size() && notificationsIsCodeChar(static_cast<unsigned char>(src[i]))) i += 1;
                continue;
            }

            // Одиночный 0xA7 (кодировка Latin-1) — то же самое, но только если это
            // не хвостовой байт UTF-8: 'Ч' = D0 A7, и такой байт трогать нельзя.
            if (c == 0xA7 && (i == 0 || static_cast<unsigned char>(src[i - 1]) < 0xC0))
            {
                i += 1;
                if (i < src.size() && notificationsIsCodeChar(static_cast<unsigned char>(src[i]))) i += 1;
                continue;
            }

            // Управляющие символы, которые часто прилетают с серверов
            if (c < 0x20 && c != '\t') { i += 1; continue; }

            out += src[i++];
        }

        // Обрезаем края и схлопываем повторные пробелы
        std::string trimmed;
        trimmed.reserve(out.size());
        bool lastSpace = true;
        for (char ch : out)
        {
            const bool isSpace = (ch == ' ' || ch == '\t');
            if (isSpace) { if (!lastSpace) trimmed += ' '; }
            else         { trimmed += ch; }
            lastSpace = isSpace;
        }
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
        return trimmed;
    }
}

void Notifications::onEnable()
{
    gFeatureManager->mDispatcher->listen<NotifyEvent,            &Notifications::onNotifyEvent>(this);
    gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent, &Notifications::onModuleStateChange>(this);
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent, &Notifications::onConnectionRequestEvent>(this);
}

void Notifications::onDisable()
{
    gFeatureManager->mDispatcher->deafen<NotifyEvent,            &Notifications::onNotifyEvent>(this);
    gFeatureManager->mDispatcher->deafen<ModuleStateChangeEvent, &Notifications::onModuleStateChange>(this);
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent, &Notifications::onConnectionRequestEvent>(this);

    mEntries.clear();
}

void Notifications::push(std::string text, Icon icon, float duration, std::string state)
{
    Entry entry;
    entry.text = cleanText(text);
    if (entry.text.empty()) return;

    entry.state    = std::move(state);
    entry.icon     = icon;
    entry.duration = std::clamp(duration, 0.5f, 60.f);

    mEntries.push_back(std::move(entry));

    // Спам событий не должен копить карточки бесконечно.
    while (mEntries.size() > kMaxStored)
        mEntries.erase(mEntries.begin());
}

void Notifications::onRenderEvent(RenderEvent& event)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    const float delta  = std::clamp(ImGui::GetIO().DeltaTime, 0.0001f, 0.1f);
    const ImVec2 screen = ImGui::GetIO().DisplaySize;

    for (auto& entry : mEntries)
        entry.life += delta;

    std::erase_if(mEntries, [](const Entry& entry)
    {
        return entry.life > entry.duration + kOutTime;
    });

    if (mEntries.empty()) return;

    FontHelper::pushPrefFont(false);
    ImFont* font = ImGui::GetFont();
    if (!font)
    {
        ImGui::PopFont();
        return;
    }

    const float lineHeight = font->CalcTextSizeA(kFontSize, FLT_MAX, 0.f, "Ag").y;
    const float cardHeight = (std::max)(26.f, lineHeight + kPadY * 2.f);
    const bool  fromBottom = mPosition.mValue == Position::BottomRight;

    float cursorY = fromBottom ? (screen.y - kMargin - cardHeight) : kMargin;
    int   drawn   = 0;

    // Новые карточки рисуются первыми — они всегда ближе к углу.
    for (auto it = mEntries.rbegin(); it != mEntries.rend() && drawn < kMaxVisible; ++it, ++drawn)
    {
        Entry& entry = *it;

        // Ширина считается один раз, при первом кадре карточки.
        if (entry.width <= 0.f)
        {
            const float textW  = font->CalcTextSizeA(kFontSize, FLT_MAX, 0.f, entry.text.c_str()).x;
            const float stateW = entry.state.empty()
                ? 0.f
                : font->CalcTextSizeA(kFontSize, FLT_MAX, 0.f, entry.state.c_str()).x + 6.f;

            entry.textW = textW;
            entry.width = kPadX * 2.f + kStripW + kStripGap + textW + stateW;
            entry.width = std::clamp(entry.width, 96.f,
                                     (std::max)(120.f, screen.x - kMargin * 2.f));
        }

        const float cardY = cursorY;
        cursorY += fromBottom ? -(cardHeight + kCardGap) : (cardHeight + kCardGap);

        const float inT  = std::clamp(entry.life / kInTime, 0.f, 1.f);
        const float outT = entry.life > entry.duration
            ? std::clamp((entry.life - entry.duration) / kOutTime, 0.f, 1.f)
            : 0.f;

        const float alpha = inT * (1.f - outT);
        if (alpha <= 0.01f) continue;

        const float easeIn = 1.f - (1.f - inT) * (1.f - inT);
        const float hidden = entry.width + kMargin;

        const float x = screen.x - kMargin - entry.width
                      + (1.f - easeIn) * hidden
                      + outT * hidden;
        const float y = cardY + (1.f - easeIn) * 4.f;

        const ImVec2 min(x, y);
        const ImVec2 max(x + entry.width, y + cardHeight);

        // ── сплошная чёрная подложка ──────────────────────────────────────────
        drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, static_cast<int>(238 * alpha)), kRounding);
        drawList->AddRect(min, max, IM_COL32(255, 255, 255, static_cast<int>(24 * alpha)),
                          kRounding, 0, 1.f);

        // ── цветная полоска-акцент вместо пиктограммы ────────────────────────
        // Свободно стоящая вертикальная полоска с отступом от краёв — как полоска
        // справа в Arraylist. Один прямоугольник, ничего не ломается на малых размерах.
        const float stripX   = min.x + kPadX - 3.f;
        const float stripTop = y + 4.f;
        const float stripBot = max.y - 4.f;
        drawList->AddRectFilled({ stripX, stripTop }, { stripX + kStripW, stripBot },
                                accentColor(entry.icon, alpha), 1.5f);

        // ── текст ─────────────────────────────────────────────────────────────
        const float textX = stripX + kStripW + kStripGap;
        const float textY = y + (cardHeight - lineHeight) * 0.5f;

        drawList->AddText(font, kFontSize, { textX, textY },
                          IM_COL32(246, 246, 248, static_cast<int>(255 * alpha)),
                          entry.text.c_str());

        if (!entry.state.empty())
        {
            drawList->AddText(font, kFontSize, { textX + entry.textW + 6.f, textY },
                              IM_COL32(142, 145, 152, static_cast<int>(255 * alpha)),
                              entry.state.c_str());
        }

        // ── тонкая линия оставшегося времени ─────────────────────────────────
        const float remain = std::clamp(1.f - entry.life / (std::max)(entry.duration, 0.001f), 0.f, 1.f);
        if (remain > 0.01f && outT <= 0.f)
        {
            const float barW = (entry.width - 2.f) * remain;
            drawList->AddRectFilled({ min.x + 1.f, max.y - 1.6f },
                                    { min.x + 1.f + barW, max.y - 0.6f },
                                    IM_COL32(255, 255, 255, static_cast<int>(58 * alpha)));
        }
    }

    ImGui::PopFont();
}

void Notifications::onModuleStateChange(ModuleStateChangeEvent& event)
{
    if (event.isCancelled() || !mShowOnToggle.mValue) return;
    if (!event.mModule) return;

    push(event.mModule->getName(),
         event.mEnabled ? Icon::Success : Icon::Disabled,
         mDuration.mValue,
         event.mEnabled ? "enabled" : "disabled");
}

void Notifications::onConnectionRequestEvent(ConnectionRequestEvent& event)
{
    if (!mShowOnJoin.mValue) return;
    if (!event.mServerAddress) return;

    push("Connecting to " + *event.mServerAddress + "...", Icon::Info, mDuration.mValue);
}

void Notifications::onNotifyEvent(NotifyEvent& event)
{
    const Notification& notification = event.mNotification;

    Icon icon = Icon::Info;
    if (notification.mType == Notification::Type::Warning) icon = Icon::Warning;
    else if (notification.mType == Notification::Type::Error) icon = Icon::Error;

    std::string text  = notification.mMessage;
    std::string state;

    // Другие части клиента до сих пор формируют сообщения вида "Aura enabled".
    // Разделяем их, чтобы карточка выглядела как toggle-карточка.
    if (icon == Icon::Info)
    {
        constexpr const char* kEnabled  = " enabled";
        constexpr const char* kDisabled = " disabled";

        const std::string clean = cleanText(text);
        const size_t enabledLen  = 8; // strlen(kEnabled)
        const size_t disabledLen = 9; // strlen(kDisabled)

        if (clean.size() > enabledLen &&
            clean.compare(clean.size() - enabledLen, enabledLen, kEnabled) == 0)
        {
            text  = clean.substr(0, clean.size() - enabledLen);
            state = "enabled";
            icon  = Icon::Success;
        }
        else if (clean.size() > disabledLen &&
                 clean.compare(clean.size() - disabledLen, disabledLen, kDisabled) == 0)
        {
            text  = clean.substr(0, clean.size() - disabledLen);
            state = "disabled";
            icon  = Icon::Disabled;
        }
    }

    const float duration = notification.mDuration > 0.f ? notification.mDuration : mDuration.mValue;
    push(std::move(text), icon, duration, std::move(state));
}
