//
// Notifications — компактные тёмные карточки в правом углу.
//
// Стиль: мягкий вертикальный градиент подложки, тонкая рамка в цвет события,
// значок в тонированном квадрате (рисуется примитивами, без шрифтовых глифов)
// и полоска оставшегося времени в цвете события. Всё — AddRectFilled,
// AddRect, AddLine и AddText: никакого блюра и теней, кадр не нагружается.
//
// Что убрано по сравнению со старой версией (и почему):
//   • ImRenderUtils::addBlur на каждой карточке — самая дорогая операция кадра;
//   • двойной AddShadowRect «glow» на каждой карточке;
//   • shimmer-полоса, летающая рамка, таймер-кольцо, спарклы, эластичные пружины;
//   • ~20 настроек цвета/плотности/анимаций;
//   • повторный strip §-кодов и пересчёт ширин текста КАЖДЫЙ кадр.
//
// Карточка строится один раз (текст очищен, ширина посчитана), а в кадре
// остаются только рисующие вызовы.
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
    constexpr float kPadX      = 12.f;
    constexpr float kPadY      = 10.f;
    constexpr float kBadge     = 20.f;   // сторона квадрата со значком
    constexpr float kBadgeGap  = 10.f;   // от значка до текста
    constexpr float kCardGap   = 7.f;
    constexpr float kMargin    = 12.f;
    constexpr float kRounding  = 8.f;
    constexpr float kInTime    = 0.18f;
    constexpr float kOutTime   = 0.15f;
    constexpr float kFontSize  = 15.f;   // заголовок
    constexpr float kSubSize   = 12.5f;  // подпись
    constexpr float kMinWidth  = 168.f;
    constexpr float kMaxWidth  = 420.f;
    constexpr int   kMaxVisible = 6;
    constexpr size_t kMaxStored = 16;
    constexpr float kLineGap    = 3.f;   // между заголовком и подписью

    // Цвет события. Насыщенный, но не кислотный: на тёмной карточке видно
    // сразу, а в глазах не рябит.
    ImU32 accentColor(Notifications::Icon icon, float alpha)
    {
        const int a = static_cast<int>(std::clamp(alpha, 0.f, 1.f) * 255.f);
        switch (icon)
        {
            case Notifications::Icon::Success:  return IM_COL32(110, 231, 173, a); // мята — включено
            case Notifications::Icon::Disabled: return IM_COL32(148, 154, 168, a); // пепел — выключено
            case Notifications::Icon::Warning:  return IM_COL32(247, 196, 108, a); // янтарь
            case Notifications::Icon::Error:    return IM_COL32(247, 121, 121, a); // коралл
            case Notifications::Icon::Info:
            default:                            return IM_COL32(129, 178, 255, a); // лазурь
        }
    }

    // Значок рисуется линиями и точками. Это дешевле текста (не нужно искать
    // глиф в шрифте) и не зависит от выбранного шрифта клиента.
    void drawBadgeGlyph(ImDrawList* dl, Notifications::Icon icon, ImVec2 c, ImU32 color)
    {
        constexpr float th = 2.f;
        switch (icon)
        {
            case Notifications::Icon::Success:
                dl->AddLine({ c.x - 4.4f, c.y + 0.3f }, { c.x - 1.3f, c.y + 3.6f }, color, th);
                dl->AddLine({ c.x - 1.3f, c.y + 3.6f }, { c.x + 4.6f, c.y - 3.6f }, color, th);
                break;

            case Notifications::Icon::Error:
                dl->AddLine({ c.x - 3.8f, c.y - 3.8f }, { c.x + 3.8f, c.y + 3.8f }, color, th);
                dl->AddLine({ c.x + 3.8f, c.y - 3.8f }, { c.x - 3.8f, c.y + 3.8f }, color, th);
                break;

            case Notifications::Icon::Warning:
                dl->AddLine({ c.x, c.y - 4.4f }, { c.x, c.y + 1.2f }, color, th);
                dl->AddCircleFilled({ c.x, c.y + 4.0f }, 1.35f, color, 8);
                break;

            case Notifications::Icon::Disabled:
                dl->AddLine({ c.x - 4.0f, c.y }, { c.x + 4.0f, c.y }, color, th);
                break;

            case Notifications::Icon::Info:
            default:
                dl->AddCircleFilled({ c.x, c.y - 4.2f }, 1.35f, color, 8);
                dl->AddLine({ c.x, c.y - 1.3f }, { c.x, c.y + 4.2f }, color, th);
                break;
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

void Notifications::push(std::string title, Icon icon, float duration, std::string subtitle)
{
    Entry entry;
    entry.title    = cleanText(title);
    entry.subtitle = cleanText(subtitle);
    if (entry.title.empty() && entry.subtitle.empty()) return;
    if (entry.title.empty()) std::swap(entry.title, entry.subtitle);

    entry.icon     = icon;
    entry.duration = std::clamp(duration, 0.5f, 60.f);

    // Одно и то же уведомление не копится стопкой (перещёлкал модуль туда-сюда
    // — получил бы десять карточек): старую убираем и показываем заново, уже
    // с нулевым таймером. Так видно, что событие повторилось.
    const auto same = std::find_if(mEntries.begin(), mEntries.end(), [&](const Entry& e)
    {
        return e.title == entry.title && e.subtitle == entry.subtitle && e.icon == entry.icon;
    });
    if (same != mEntries.end()) mEntries.erase(same);

    mEntries.push_back(std::move(entry));

    // Спам событий не должен копить карточки бесконечно.
    while (mEntries.size() > kMaxStored)
        mEntries.erase(mEntries.begin());
}

void Notifications::onRenderEvent(RenderEvent& event)
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    const float  delta  = std::clamp(ImGui::GetIO().DeltaTime, 0.0001f, 0.1f);
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

    const float titleH = font->CalcTextSizeA(kFontSize, FLT_MAX, 0.f, "Ag").y;
    const float subH   = font->CalcTextSizeA(kSubSize,  FLT_MAX, 0.f, "Ag").y;
    const bool  fromBottom = mPosition.mValue == Position::BottomRight;

    float cursorY = fromBottom ? (screen.y - kMargin) : kMargin;
    int   drawn   = 0;

    // Новые карточки — ближе к углу; старые уходят вниз (или вверх).
    for (auto it = mEntries.rbegin(); it != mEntries.rend() && drawn < kMaxVisible; ++it, ++drawn)
    {
        Entry& entry = *it;

        const bool  hasSub = !entry.subtitle.empty();
        const float textH  = titleH + (hasSub ? (kLineGap + subH) : 0.f);
        const float cardH  = kPadY * 2.f + (std::max)(textH, kBadge);

        // Ширина считается один раз, при первом кадре карточки.
        if (entry.width <= 0.f)
        {
            entry.titleW = font->CalcTextSizeA(kFontSize, FLT_MAX, 0.f, entry.title.c_str()).x;
            entry.subtitleW = hasSub
                ? font->CalcTextSizeA(kSubSize, FLT_MAX, 0.f, entry.subtitle.c_str()).x
                : 0.f;

            const float content = (std::max)(entry.titleW, entry.subtitleW);
            entry.width = std::clamp(kPadX * 2.f + kBadge + kBadgeGap + content,
                                     kMinWidth, (std::min)(kMaxWidth, screen.x - kMargin * 2.f));
        }

        const float cardY = fromBottom ? (cursorY - cardH) : cursorY;
        cursorY = fromBottom ? (cardY - kCardGap) : (cardY + cardH + kCardGap);

        const float inT  = std::clamp(entry.life / kInTime, 0.f, 1.f);
        const float outT = entry.life > entry.duration
            ? std::clamp((entry.life - entry.duration) / kOutTime, 0.f, 1.f)
            : 0.f;

        // Вход — easeOutQuint (быстро появляется, мягко доезжает), выход —
        // easeInQuad (сразу срывается, дальше неспешно). Прозрачность чуть
        // быстрее геометрии, чтобы карточка не «висела» полупрозрачной.
        const float inv   = 1.f - inT;
        const float easeIn = 1.f - inv * inv * inv * inv * inv;
        const float easeOut = outT * outT;
        const float alpha = std::clamp(std::min(easeIn * 1.7f, 1.f) * (1.f - outT), 0.f, 1.f);
        if (alpha <= 0.01f) continue;

        const float hidden = entry.width + kMargin + 8.f;
        const float x = screen.x - kMargin - entry.width + (1.f - easeIn) * hidden + easeOut * hidden;
        const float y = cardY + (1.f - easeIn) * 6.f;

        const ImVec2 min(x, y);
        const ImVec2 max(x + entry.width, y + cardH);

        const ImU32 accent = accentColor(entry.icon, alpha);

        auto withAlpha = [&](ImU32 col, float mul)
        {
            const int a = static_cast<int>((col >> IM_COL32_A_SHIFT & 0xFF) * mul);
            return (col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(std::clamp(a, 0, 255)) << IM_COL32_A_SHIFT);
        };

        // ── подложка ──────────────────────────────────────────────────────
        // Плотный почти-чёрный фон: карточка читается на любом небе и не
        // требует блюра под собой.
        drawList->AddRectFilled(min, max, IM_COL32(17, 18, 22, static_cast<int>(245 * alpha)), kRounding);

        // Тонкая световая кромка сверху — объём без градиентного меша.
        drawList->AddLine({ min.x + kRounding, min.y + 0.5f }, { max.x - kRounding, min.y + 0.5f },
                          IM_COL32(255, 255, 255, static_cast<int>(16 * alpha)), 1.f);

        // Рамка в цвете события — тип читается по контуру, а не по тексту.
        drawList->AddRect(min, max, withAlpha(accent, 0.42f), kRounding, 0, 1.f);

        // ── значок ───────────────────────────────────────────────────────
        const float badgeX = min.x + kPadX;
        const float badgeY = y + (cardH - kBadge) * 0.5f;
        drawList->AddRectFilled({ badgeX, badgeY }, { badgeX + kBadge, badgeY + kBadge },
                                withAlpha(accent, 0.20f), 6.f);
        drawBadgeGlyph(drawList, entry.icon,
                       { badgeX + kBadge * 0.5f, badgeY + kBadge * 0.5f },
                       withAlpha(accent, 0.95f));

        // ── текст ────────────────────────────────────────────────────────
        const float textX   = badgeX + kBadge + kBadgeGap;
        const float textTop = y + (cardH - textH) * 0.5f;

        drawList->AddText(font, kFontSize, { textX, textTop },
                          IM_COL32(244, 246, 250, static_cast<int>(255 * alpha)),
                          entry.title.c_str());

        if (hasSub)
        {
            drawList->AddText(font, kSubSize, { textX, textTop + titleH + kLineGap },
                              IM_COL32(150, 156, 170, static_cast<int>(235 * alpha)),
                              entry.subtitle.c_str());
        }

        // ── время жизни ──────────────────────────────────────────────────
        const float remain = std::clamp(1.f - entry.life / (std::max)(entry.duration, 0.001f), 0.f, 1.f);
        if (remain > 0.01f && outT <= 0.f)
        {
            const float inset = 1.f;
            const float barW  = (entry.width - inset * 2.f) * remain;
            drawList->AddRectFilled({ min.x + inset, max.y - 2.6f },
                                    { min.x + inset + barW, max.y - 1.0f },
                                    withAlpha(accent, 0.55f), 1.f);
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

    push("Connecting", Icon::Info, mDuration.mValue, *event.mServerAddress);
}

void Notifications::onNotifyEvent(NotifyEvent& event)
{
    const Notification& notification = event.mNotification;

    Icon icon = Icon::Info;
    if (notification.mType == Notification::Type::Warning) icon = Icon::Warning;
    else if (notification.mType == Notification::Type::Error) icon = Icon::Error;

    std::string title = notification.mMessage;
    std::string subtitle;

    // Другие части клиента до сих пор формируют сообщения вида "Aura enabled".
    // Разделяем их, чтобы карточка выглядела как toggle-карточка.
    if (icon == Icon::Info)
    {
        constexpr const char* kEnabled  = " enabled";
        constexpr const char* kDisabled = " disabled";

        const std::string clean = cleanText(title);
        const size_t enabledLen  = 8; // strlen(kEnabled)
        const size_t disabledLen = 9; // strlen(kDisabled)

        if (clean.size() > enabledLen &&
            clean.compare(clean.size() - enabledLen, enabledLen, kEnabled) == 0)
        {
            title    = clean.substr(0, clean.size() - enabledLen);
            subtitle = "enabled";
            icon     = Icon::Success;
        }
        else if (clean.size() > disabledLen &&
                 clean.compare(clean.size() - disabledLen, disabledLen, kDisabled) == 0)
        {
            title    = clean.substr(0, clean.size() - disabledLen);
            subtitle = "disabled";
            icon     = Icon::Disabled;
        }
    }

    push(title, icon, std::clamp(notification.mDuration, 0.5f, 60.f), subtitle);
}
