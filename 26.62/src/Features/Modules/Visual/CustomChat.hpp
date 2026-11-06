#pragma once
//
// Created by vastrakai on 9/21/2024.
//

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Module.hpp>

#include "HudEditor.hpp"

//
// CustomChat — свой чат вместо ванильного.
//
// Как это работает целиком:
//   • Входящие TextPacket-ы не доходят до ванильного чата, а клиентские
//     сообщения («Juzdex » ...», вывод команд) перехватывает BaseTickHook и
//     отдаёт сюда же через ChatUtils::sSink — в игре остаётся ровно один список.
//   • Ванильный экран чата вообще НЕ открывается. Модуль сам перехватывает
//     клавиши открытия (T, Enter, «/»), сам рисует строку ввода, сам отправляет
//     сообщение. Раньше ванильный экран прятался пропуском ScreenView::setupAndRender,
//     но внутри этого вызова живёт ВЕСЬ ввод экрана (набор, Enter, Esc) — без него
//     чат не закрывался и не отправлял сообщения. Поэтому теперь экран не
//     открывается, а не «прячется».
//   • На время набора ChatUtils::sChatInputActive = true, и KeyHook/MouseHook не
//     отдают ввод игре: персонаж стоит, камера не крутится, клик не ломает блок.
//     Клавиши при этом уходят в ImGui, и символы (в том числе кириллица) попадают
//     прямо в нашу строку ввода.
//   • Отпускание клавиши, которой закрыли чат (Enter/Esc), тоже гасится: игра
//     открывает ванильный чат именно по отпусканию Enter — без этого оригинал
//     открывался бы поверх нашего сразу после отправки.
//
// Раскладка (из-за чего раньше сообщения уезжали за нижний край экрана):
//   • панель ПРИБИТА К НИЗУ своей области (HudElement), а не растёт вниз;
//   • новые сообщения появляются снизу и толкают старые вверх;
//   • высота области = Lines строк, поэтому ничего не уезжает за экран;
//   • двигается панель в HudEditor — её можно поставить хоть в левый нижний угол.
//
// Что важно знать про текст:
//   • §-коды вырезаются с учётом UTF-8. Раньше одиночный байт 0xA7 считался «§»
//     и вместе с ним съедался следующий байт — из-за этого пропадала первая буква
//     («Чат донатеров» превращалось в «ат донатеров»: 'Ч' = D0 A7).
//   • Длинные сообщения переносятся по словам, а слишком длинные слова — по
//     символам, поэтому ничего не вылезает за чёрный фон.
//   • Colorful возвращает §-цвета как в игре (белый по умолчанию — если выключить,
//     сообщение рисуется одним цветом, но без мусорных символов).
//   • Кириллица: KeyHook отдаёт ImGui настоящие Unicode-символы (ToUnicode, а не
//     ToAscii), поэтому русские буквы больше не превращаются в «?».
//
class CustomChat : public ModuleBase<CustomChat> {
public:
    NumberSetting mMaxLifeTime = NumberSetting("Life Time", "How long a message stays on screen (seconds)", 6, 1, 15, 1);

    // ── Размер чата ───────────────────────────────────────────────────────────
    // Ширина, количество строк, кегль и расстояние между строками — это и есть
    // «размер» чата. Высота области считается из Lines, поэтому панель всегда
    // одной и той же высоты и не прыгает при появлении новых сообщений.
    NumberSetting mWidth       = NumberSetting("Width", "Width of the chat panel in pixels", 420.f, 160.f, 900.f, 10.f);
    NumberSetting mMaxLines    = NumberSetting("Lines", "How many lines of chat are visible at once", 10.f, 3.f, 30.f, 1.f);
    NumberSetting mFontSize    = NumberSetting("Font Size", "Text size of the chat", 19.f, 10.f, 28.f, 1.f);
    NumberSetting mLineSpacing = NumberSetting("Line Spacing", "Extra space between two lines", 2.f, 0.f, 12.f, 1.f);

    BoolSetting   mColorful    = BoolSetting("Colorful", "Render §-colour codes from the server like the vanilla chat does", true);
    BoolSetting   mInputBox    = BoolSetting("Input Box", "Draw the custom input line while typing", true);

    // Один цветной кусок строки (сообщение приходит с §-кодами, их может быть много).
    struct TextRun {
        ImU32       color = IM_COL32_WHITE;
        std::string text;
        float       width = 0.f;
    };

    struct ChatLine {
        std::vector<TextRun> runs;
        float width = 0.f;
    };

    struct ChatMessage {
        std::string raw;                 // как пришло с сервера (с §, если они есть)
        std::string plain;               // тот же текст без § — для поиска повторов
        int         count = 1;           // сколько раз подряд повторилось
        float       lifeTime = 6.f;      // сколько живёт именно это сообщение
        float       age = 0.f;           // сколько уже на экране
        float       alpha = 0.f;         // анимация появления/исчезновения
        std::vector<ChatLine> lines;     // кэш разбивки по строкам

        // Ключ кэша: при смене размера шрифта, ширины или счётчика разбивка пересобирается.
        float cacheWidth    = -1.f;
        float cacheFontSize = -1.f;
        int   cacheCount    = 0;
        bool  cacheColorful = false;
    };

    std::vector<ChatMessage> mMessages;

    CustomChat() : ModuleBase("CustomChat", "A clean, Solstice-themed chat", ModuleCategory::Visual, 0, false) {
        mNames = {
            {Lowercase, "customchat"},
            {LowercaseSpaced, "custom chat"},
            {Normal, "CustomChat"},
            {NormalSpaced, "Custom Chat"},
        };

        addSettings(&mMaxLifeTime, &mWidth, &mMaxLines, &mFontSize, &mLineSpacing,
                    &mColorful, &mInputBox);

        // Панель чата двигается в HudEditor, как любой другой HUD-элемент.
        // Позиция по умолчанию — левый низ: панель прибита к низу своей области,
        // поэтому её просто утягивают в левый нижний угол и она там остаётся.
        mElement = std::make_unique<HudElement>();
        mElement->mPos = { 8.f, 420.f };
        mElement->mParentTypeIdentifier = const_cast<char*>(ModuleBase<CustomChat>::getTypeID());
        registerHudElement();
    }

    void addMessage(const std::string& message);

    // Callback для ChatUtils::sSink — сюда приходят клиентские сообщения
    // (вывод команд, «Juzdex » ...»). Статический, потому что сокет хранит
    // обычный указатель на функцию.
    static void onClientMessage(const std::string& message);

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    void onPacketInEvent(class PacketInEvent& event);

    // Перехват клавиш открытия чата (T, Enter, «/») — ванильный экран не открываем.
    void onKeyEvent(class KeyEvent& event);

    // Пока идёт набор, мышь тоже наша: иначе клик ломал бы блоки, а движение
    // крутило камеру.
    void onMouseEvent(class MouseEvent& event);

    std::unique_ptr<HudElement> mElement;

private:
    bool mHudRegistered = false;

    // true — игрок набирает сообщение в нашем чате (вместо открытого ванильного).
    bool mInputActive = false;

    // Пара кадров после открытия не читаем очередь символов: клавиша открытия
    // успевает отдать свой символ в ImGui раньше, чем мы узнаём, что чат открыт,
    // и без этого в строке ввода оказывалась лишняя «t».
    int mSkipChars = 0;

    // Момент открытия (секунды ImGui). Клавиша открытия доезжает до ImGui с
    // задержкой в один кадр (очередь событий разбирается в NewFrame), поэтому
    // Enter/Esc в первый десяток кадров после открытия игнорируются — иначе чат,
    // открытый по Enter, закрывался бы сразу же тем же Enter.
    float mOpenedAtTime = -100.f;

    // Строка ввода нашего чата. Отправляет её сам модуль, ванильный экран в этом
    // не участвует.
    std::string mInput;

    // Последнее отправленное сообщение: по нему гасим эхо сервера, чтобы своё же
    // сообщение не появилось дважды.
    std::string mLastSent;
    float       mLastSentTime = -100.f;

    void registerHudElement();
    static bool isChatOpen();
    bool isInGame() const;
    void openInput(bool slash);
    void closeInput(int swallowKey = 0);
    void sendInput(const std::string& text);
    void updateInput();
    void updateMessages(float delta);
    void pushMessage(const std::string& message);

    // Пересобирает строки сообщения (перенос по словам + цвет) под текущий шрифт.
    void buildLines(ChatMessage& message, ImFont* font, float fontSize, float maxWidth, bool colorful);
};
