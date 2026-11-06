//
// Created by vastrakai on 9/21/2024.
//

#include "CustomChat.hpp"

#include <Features/Events/KeyEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/Keyboard.hpp>
#include <Utils/NurikIcons.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace
{
    constexpr float kFadeTime  = 0.45f;   // плавное исчезновение
    constexpr float kPadX      = 8.f;
    constexpr float kPadY      = 6.f;
    constexpr float kRounding  = 5.f;
    constexpr size_t kMaxStored = 128;

    // Ванильная палитра §-кодов (0-9, a-f).
    const ImU32 kMcColors[16] = {
        IM_COL32(  0,   0,   0, 255), IM_COL32(  0,   0, 170, 255), IM_COL32(  0, 170,   0, 255), IM_COL32(  0, 170, 170, 255),
        IM_COL32(170,   0,   0, 255), IM_COL32(170,   0, 170, 255), IM_COL32(255, 170,   0, 255), IM_COL32(170, 170, 170, 255),
        IM_COL32( 85,  85,  85, 255), IM_COL32( 85,  85, 255, 255), IM_COL32( 85, 255,  85, 255), IM_COL32( 85, 255, 255, 255),
        IM_COL32(255,  85,  85, 255), IM_COL32(255,  85, 255, 255), IM_COL32(255, 255,  85, 255), IM_COL32(255, 255, 255, 255),
    };

    inline bool isUtf8Continuation(unsigned char c) { return (c & 0xC0) == 0x80; }
    inline bool isUtf8Lead(unsigned char c) { return c >= 0xC0; }

    // Какой символ даёт клавиша при текущей раскладке. Нужно, чтобы чат открывал
    // именно «/»: на русской раскладке слэш — это Shift+\, а точка, которая лежит
    // на той же физической клавише в US-раскладке, открывать чат не должна.
    unsigned int ccVirtualKeyChar(int vk)
    {
        const int scanCode = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
        BYTE keyState[256] = { 0 };
        GetKeyboardState(keyState);

        WCHAR translation[8] = { 0 };
        const int result = ToUnicode(static_cast<UINT>(vk), static_cast<UINT>(scanCode), keyState, translation, 8, 0);
        if (result <= 0) return 0;

        return static_cast<unsigned int>(translation[0]);
    }

    // Настоящий код цвета/стиля Minecraft: 0-9, a-f, k-o, r.
    // После «§» выбрасываем байт ТОЛЬКО если это действительно код: иначе
    // одинокий «§» в конце строки съел бы первую букву следующего слова.
    bool customChatIsCodeChar(unsigned char c)
    {
        if (c >= '0' && c <= '9') return true;

        const unsigned char lower = (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + 32) : c;
        if (lower >= 'a' && lower <= 'f') return true;

        return lower == 'k' || lower == 'l' || lower == 'm' ||
               lower == 'n' || lower == 'o' || lower == 'r';
    }

    // Свой UTF-8-энкодер: ImTextCharToUtf8 объявлен в imgui_internal.h, а тянуть
    // внутренний заголовок ради четырёх строк не хочется.
    void ccAppendUtf8(std::string& out, unsigned int c)
    {
        if (c < 0x80)
        {
            out += static_cast<char>(c);
        }
        else if (c < 0x800)
        {
            out += static_cast<char>(0xC0 | (c >> 6));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
        else if (c < 0x10000)
        {
            out += static_cast<char>(0xE0 | (c >> 12));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (c >> 18));
            out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }

    // Применяет анимационную прозрачность к уже готовому цвету строки.
    ImU32 ccWithAlpha(ImU32 color, float alpha)
    {
        const int base = static_cast<int>((color >> IM_COL32_A_SHIFT) & 0xFF);
        const int a = std::clamp(static_cast<int>(base * std::clamp(alpha, 0.f, 1.f)), 0, 255);
        return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
    }

    // Тёмные §-цвета (§0, §1, §4) на чёрной подложке не видно вообще, поэтому
    // совсем тёмные оттенки слегка подмешиваются к белому. Оттенок сохраняется.
    ImU32 ccReadable(ImU32 color)
    {
        const float r = static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xFF);
        const float g = static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xFF);
        const float b = static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xFF);

        const float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
        if (luminance >= 72.f) return color;

        const float t = (72.f - luminance) / 72.f * 0.6f;
        auto mix = [t](float v) { return static_cast<int>(v + (255.f - v) * t); };
        return IM_COL32(mix(r), mix(g), mix(b), 255);
    }

    int ccHexIndex(char code)
    {
        if (code >= '0' && code <= '9') return code - '0';
        if (code >= 'a' && code <= 'f') return code - 'a' + 10;
        if (code >= 'A' && code <= 'F') return code - 'A' + 10;
        return -1;
    }

    // Убирает §-коды, управляющие символы и лишние пробелы.
    //
    // ВАЖНО (это и был баг с пропавшей буквой): одиночный байт 0xA7 НЕЛЬЗЯ считать
    // «§», если перед ним стоит ведущий байт UTF-8 — иначе у кириллицы отрезается
    // последний байт символа. 'Ч' = D0 A7, поэтому раньше из «Чат донатеров»
    // получалось «ат донатеров».
    std::string ccStripCodes(const std::string& src, bool& hadCodes)
    {
        hadCodes = false;

        std::string out;
        out.reserve(src.size());

        for (size_t i = 0; i < src.size(); )
        {
            const unsigned char c = static_cast<unsigned char>(src[i]);

            // UTF-8 «§» = C2 A7 + сам код цвета/стиля
            if (c == 0xC2 && i + 1 < src.size() && static_cast<unsigned char>(src[i + 1]) == 0xA7)
            {
                hadCodes = true;
                i += 2;
                if (i < src.size() && customChatIsCodeChar(static_cast<unsigned char>(src[i]))) i += 1;
                continue;
            }

            // Однобайтовый legacy «§» (0xA7). Пропускаем только если это не хвост
            // UTF-8-последовательности.
            if (c == 0xA7 && (i == 0 || !isUtf8Lead(static_cast<unsigned char>(src[i - 1]))))
            {
                hadCodes = true;
                i += 1;
                if (i < src.size() && customChatIsCodeChar(static_cast<unsigned char>(src[i]))) i += 1;
                continue;
            }

            // Управляющие символы от сервера
            if (c < 0x20 && c != '\t') { i += 1; continue; }

            out += src[i++];
        }

        // Схлопываем пробелы и обрезаем края
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

    // Разбирает строку с §-кодами на цветные куски. Стили (l/k/m/n/o) игнорируются,
    // цвет остаётся прежним — так же ведёт себя ванильный чат.
    void ccParseColored(const std::string& raw, std::vector<std::pair<ImU32, std::string>>& out)
    {
        const ImU32 base = IM_COL32(246, 246, 248, 255);
        ImU32 current = base;

        std::string buffer;
        auto flush = [&]()
        {
            if (!buffer.empty()) { out.emplace_back(current, buffer); buffer.clear(); }
        };

        auto applyCode = [&](char code)
        {
            if (code == 'r' || code == 'R') { current = base; return; }
            const int index = ccHexIndex(code);
            if (index >= 0) current = ccReadable(kMcColors[index]);
        };

        for (size_t i = 0; i < raw.size(); )
        {
            const unsigned char c = static_cast<unsigned char>(raw[i]);

            if (c == 0xC2 && i + 1 < raw.size() && static_cast<unsigned char>(raw[i + 1]) == 0xA7)
            {
                flush();
                i += 2;
                if (i < raw.size() && customChatIsCodeChar(static_cast<unsigned char>(raw[i]))) { applyCode(raw[i]); i += 1; }
                continue;
            }

            if (c == 0xA7 && (i == 0 || !isUtf8Lead(static_cast<unsigned char>(raw[i - 1]))))
            {
                flush();
                i += 1;
                if (i < raw.size() && customChatIsCodeChar(static_cast<unsigned char>(raw[i]))) { applyCode(raw[i]); i += 1; }
                continue;
            }

            if (c < 0x20 && c != '\t') { i += 1; continue; }

            buffer += raw[i++];
        }

        flush();

        if (out.empty()) out.emplace_back(base, std::string{});
    }

    ImVec2 ccMeasure(ImFont* font, float size, const std::string& text)
    {
        return font->CalcTextSizeA(size, FLT_MAX, 0.f, text.c_str());
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void CustomChat::registerHudElement()
{
    if (mHudRegistered || !mElement || !HudEditor::gInstance) return;

    HudEditor::gInstance->registerElement(mElement.get());
    mHudRegistered = true;
}

bool CustomChat::isChatOpen()
{
    auto* client = ClientInstance::get();
    return client && client->getScreenName() == "chat_screen";
}

void CustomChat::onEnable()
{
    mMessages.clear();
    mInput.clear();

    registerHudElement();
    if (mElement) mElement->mVisible = true;

    // Пока модуль включён, клиентские сообщения (вывод команд, «Juzdex » ...»)
    // идут сюда, а не в ванильный чат.
    ChatUtils::sSink = &CustomChat::onClientMessage;

    gFeatureManager->mDispatcher->listen<PacketInEvent, &CustomChat::onPacketInEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &CustomChat::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<KeyEvent, &CustomChat::onKeyEvent>(this);
    gFeatureManager->mDispatcher->listen<MouseEvent, &CustomChat::onMouseEvent>(this);
}

void CustomChat::onDisable()
{
    // Если модуль выключили прямо во время набора, обязательно возвращаем игру в
    // нормальное состояние: иначе ввод останется заблокированным, а курсор отпущенным.
    if (mInputActive) closeInput();

    ChatUtils::sSink = nullptr;

    mMessages.clear();
    mInput.clear();

    if (mElement) mElement->mVisible = false;

    gFeatureManager->mDispatcher->deafen<PacketInEvent, &CustomChat::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &CustomChat::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<KeyEvent, &CustomChat::onKeyEvent>(this);
    gFeatureManager->mDispatcher->deafen<MouseEvent, &CustomChat::onMouseEvent>(this);
}

bool CustomChat::isInGame() const
{
    auto* client = ClientInstance::get();
    return client && client->getLocalPlayer() && client->getScreenName() == "hud_screen";
}

void CustomChat::onKeyEvent(KeyEvent& event)
{
    if (!event.mPressed) return;

    // Пока идёт набор, клавиши разбирает InputQueue/ImGui — здесь делать нечего.
    if (mInputActive) return;

    // Открывать чат можно только в игре: если открыт инвентарь или меню, клавиши
    // принадлежат им. Проверяем ещё и захват мыши — иначе мы отбирали бы клавиши у
    // режима вроде редактора HUD, где мышь специально отпущена.
    if (!isInGame()) return;
    if (!ClientInstance::get()->getMouseGrabbed()) return;

    if (ImGui::GetCurrentContext())
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard || io.WantTextInput) return;
    }

    const int vk = event.mKey;

    // T и Enter — как в ванильном чате. Виртуальные клавиши, раскладка не важна.
    if (vk == 'T' || vk == VK_RETURN)
    {
        openInput(false);
        event.cancel();
        return;
    }

    // «/» открывает чат с уже подставленным слэшем (так делает и ванильный чат).
    // Смотрим на настоящий символ клавиши, а не на её код: на русской раскладке
    // слэш — это Shift+\, а точка на «американской» клавише чат открывать не должна.
    if (ccVirtualKeyChar(vk) == '/')
    {
        openInput(true);
        event.cancel();
    }
}

void CustomChat::onMouseEvent(MouseEvent& event)
{
    // Во время набора мышь полностью наша: иначе клик ломал бы/ставил блоки, а
    // движение мыши крутило камеру. Клавиши и позиция курсора при этом всё равно
    // уходят в ImGui (MouseHook делает это до проверки отмены).
    if (mInputActive) event.cancel();
}

void CustomChat::openInput(bool slash)
{
    if (mInputActive) return;

    mInputActive = true;
    mInput = slash ? "/" : "";
    mOpenedAtTime = ImGui::GetCurrentContext() ? static_cast<float>(ImGui::GetTime()) : 0.f;

    // Клавиша открытия уже успела отдать свой символ в ImGui (KeyHook кормит ImGui
    // до нас), поэтому гасим очередь символов и пару кадров её не читаем — иначе в
    // строке ввода сразу появлялась бы «t» или «/».
    if (ImGui::GetCurrentContext())
    {
        ImGui::GetIO().InputQueueCharacters.resize(0);
        mSkipChars = 2;
    }

    ChatUtils::sChatInputActive = true;

    // Клавиши, зажатые в момент открытия (частый случай — W на бегу), иначе
    // остались бы «нажатыми» для клиентских модулей, которые читают это состояние.
    Keyboard::mPressedKeys.clear();

    // Курсор отпускаем, как это делает ванильный чат: игрок должен видеть, что
    // управление сейчас у чата.
    if (auto* client = ClientInstance::get()) client->releaseMouse();
}

void CustomChat::closeInput(int swallowKey)
{
    if (!mInputActive) return;

    mInputActive = false;
    mInput.clear();

    ChatUtils::sChatInputActive = false;

    // Гасим ОТПУСКАНИЕ клавиши, которой закрыли чат: ванильный Minecraft
    // открывает чат по отпусканию Enter, и если оно уйдёт в игру, оригинальный
    // экран откроется поверх нашего сразу после отправки.
    if (swallowKey != 0 && ImGui::GetCurrentContext())
    {
        ChatUtils::sChatSwallowKey      = swallowKey;
        ChatUtils::sChatSwallowDeadline = ImGui::GetTime() + 2.f;
    }

    if (auto* client = ClientInstance::get()) client->grabMouse();
}

void CustomChat::sendInput(const std::string& message)
{
    std::string text = message;

    // Обрезаем пробелы по краям: случайный пробел в начале увел бы текст не туда.
    const size_t begin = text.find_first_not_of(" \t");
    const size_t end = text.find_last_not_of(" \t");
    text = (begin == std::string::npos) ? std::string{} : text.substr(begin, end - begin + 1);

    if (text.empty()) return;

    mLastSent = text;
    mLastSentTime = static_cast<float>(ImGui::GetTime());

    // Отправляем через тот же путь, что и ванильный чат: PacketSendHook триггерит
    // ChatEvent, поэтому команды (.bind, .config) разберёт CommandManager и на
    // сервер они не улетят, а обычный текст уходит как сообщение.
    PacketUtils::sendChatMessage(text);

    // Обычное сообщение показываем сразу: ждать ответа сервера нельзя, у него не
    // всегда есть эхо. Сам ответ сервера потом отбрасываем по mLastSent.
    // Для команд ничего не показываем — вместо этого придёт их вывод.
    if (!text.empty() && text[0] != '.') addMessage("§7» §f" + text);
}

void CustomChat::onClientMessage(const std::string& message)
{
    if (!gFeatureManager || !gFeatureManager->mModuleManager) return;

    auto* chat = gFeatureManager->mModuleManager->getModule<CustomChat>();
    if (!chat || !chat->mEnabled) return;

    chat->addMessage(message);
}

void CustomChat::addMessage(const std::string& message)
{
    // Одно клиентское сообщение может содержать несколько строк (вывод .help,
    // .config и т.п.) — рисуем их как отдельные строки чата.
    size_t start = 0;
    while (start <= message.size())
    {
        size_t end = message.find('\n', start);
        if (end == std::string::npos) end = message.size();

        pushMessage(message.substr(start, end - start));

        if (end == message.size()) break;
        start = end + 1;
    }
}

void CustomChat::pushMessage(const std::string& message)
{
    bool hadCodes = false;
    const std::string plain = ccStripCodes(message, hadCodes);
    if (plain.empty()) return;

    // Повтор подряд — увеличиваем счётчик вместо новой строки.
    if (!mMessages.empty() && mMessages.back().plain == plain)
    {
        ChatMessage& last = mMessages.back();
        last.count++;
        last.age = 0.f;
        last.lifeTime = mMaxLifeTime.as<float>();
        last.alpha = (std::max)(last.alpha, 0.35f);
        return;
    }

    ChatMessage entry;
    entry.raw = message;   // §-коды нужны, если включён Colorful
    entry.plain = plain;
    entry.lifeTime = mMaxLifeTime.as<float>();
    mMessages.push_back(std::move(entry));

    while (mMessages.size() > kMaxStored)
        mMessages.erase(mMessages.begin());
}

void CustomChat::updateMessages(float delta)
{
    for (auto& message : mMessages)
    {
        message.age += delta;

        float visibility = 1.f;
        if (message.age > message.lifeTime)
            visibility = (std::max)(0.f, 1.f - (message.age - message.lifeTime) / kFadeTime);

        message.alpha = (std::min)(1.f, message.alpha + delta * 9.f) * visibility;
    }

    std::erase_if(mMessages, [](const ChatMessage& message)
    {
        return message.age > message.lifeTime + kFadeTime;
    });
}

void CustomChat::updateInput()
{
    ImGuiIO& io = ImGui::GetIO();

    // KeyHook кормит ImGui символами ДО того, как отдать (или не отдать) их игре,
    // поэтому настоящие Unicode-коды — включая кириллицу — приходят именно сюда.
    // Пару кадров после открытия очередь не читаем: там ещё лежит символ клавиши,
    // которой чат открыли.
    if (mSkipChars > 0)
    {
        --mSkipChars;
        io.InputQueueCharacters.resize(0);
    }
    else
    {
        for (ImWchar c : io.InputQueueCharacters)
        {
            if (c >= 32 && c != 127 && mInput.size() < 512)
                ccAppendUtf8(mInput, static_cast<unsigned int>(c));
        }
        io.InputQueueCharacters.resize(0);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !mInput.empty())
    {
        // Стираем целый UTF-8 символ, а не один байт.
        size_t cut = mInput.size() - 1;
        while (cut > 0 && isUtf8Continuation(static_cast<unsigned char>(mInput[cut]))) --cut;
        mInput.erase(cut);
    }

    // Клавиша открытия доезжает до ImGui с задержкой в один кадр (очередь событий
    // разбирается в NewFrame), поэтому в первый десяток кадров Enter/Esc
    // игнорируем: иначе чат, открытый по Enter, закрывался бы тем же Enter.
    if (static_cast<float>(ImGui::GetTime()) - mOpenedAtTime >= 0.1f)
    {
        // Esc — закрыть без отправки (в отличие от ванильного чата, где Esc сначала
        // закрывает подсказки — подсказок у нас нет).
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            closeInput(VK_ESCAPE);
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
        {
            // Сначала забираем текст, потом закрываем чат (closeInput() очищает строку).
            const std::string text = mInput;
            closeInput(VK_RETURN);
            sendInput(text);
        }
    }
}

void CustomChat::buildLines(ChatMessage& message, ImFont* font, float fontSize, float maxWidth, bool colorful)
{
    struct Word {
        ImU32       color;
        std::string text;
        float       width;
    };

    std::vector<Word> words;

    // Раскладываем текст на «слова с хвостовыми пробелами» и сразу измеряем их
    // текущим шрифтом — так перенос не зависит от того, какой шрифт выбран.
    auto appendWords = [&](ImU32 color, const std::string& text)
    {
        size_t i = 0;
        while (i < text.size())
        {
            const size_t start = i;
            while (i < text.size() && text[i] != ' ') ++i;
            while (i < text.size() && text[i] == ' ') ++i;

            std::string word = text.substr(start, i - start);
            if (word.empty()) continue;

            Word entry;
            entry.color = color;
            entry.width = ccMeasure(font, fontSize, word).x;
            entry.text  = std::move(word);
            words.push_back(std::move(entry));
        }
    };

    if (colorful && !message.raw.empty())
    {
        std::vector<std::pair<ImU32, std::string>> painted;
        ccParseColored(message.raw, painted);
        for (auto& piece : painted) appendWords(piece.first, piece.second);
    }
    else
    {
        appendWords(IM_COL32(246, 246, 248, 255), message.plain);
    }

    std::vector<ChatLine> lines;
    ChatLine current;
    float widest = 0.f;

    auto flushLine = [&]()
    {
        if (current.runs.empty()) return;
        widest = (std::max)(widest, current.width);
        lines.push_back(std::move(current));
        current = ChatLine{};
    };

    auto pushRun = [&](ImU32 color, const std::string& text, float width)
    {
        if (!current.runs.empty() && current.runs.back().color == color)
        {
            current.runs.back().text += text;
            current.runs.back().width += width;
        }
        else
        {
            TextRun run;
            run.color = color;
            run.text  = text;
            run.width = width;
            current.runs.push_back(std::move(run));
        }
        current.width += width;
    };

    for (const Word& word : words)
    {
        // Слово длиннее всей строки (длинная ссылка, ник без пробелов) — режем
        // по символам, чтобы ничего не вылезало за чёрную подложку.
        if (word.width > maxWidth)
        {
            flushLine();

            size_t start = 0;
            while (start < word.text.size())
            {
                size_t end = start + 1;
                while (end < word.text.size() && isUtf8Continuation(static_cast<unsigned char>(word.text[end]))) ++end;

                float width = ccMeasure(font, fontSize, word.text.substr(start, end - start)).x;
                while (end < word.text.size())
                {
                    size_t next = end + 1;
                    while (next < word.text.size() && isUtf8Continuation(static_cast<unsigned char>(word.text[next]))) ++next;

                    const float candidate = ccMeasure(font, fontSize, word.text.substr(start, next - start)).x;
                    if (candidate > maxWidth) break;

                    end = next;
                    width = candidate;
                }

                pushRun(word.color, word.text.substr(start, end - start), width);
                flushLine();
                start = end;
            }
            continue;
        }

        if (!current.runs.empty() && current.width + word.width > maxWidth)
            flushLine();

        pushRun(word.color, word.text, word.width);
    }
    flushLine();

    // Счётчик повторов — приписка к последней строке сообщения.
    if (message.count > 1 && !lines.empty())
    {
        const std::string suffix = "  x" + std::to_string(message.count);
        const float suffixWidth = ccMeasure(font, fontSize, suffix).x;

        TextRun run;
        run.color = IM_COL32(150, 152, 158, 255);
        run.text  = suffix;
        run.width = suffixWidth;

        lines.back().runs.push_back(std::move(run));
        lines.back().width += suffixWidth;
        widest = (std::max)(widest, lines.back().width);
    }

    message.lines         = std::move(lines);
    message.cacheWidth    = maxWidth;
    message.cacheFontSize = fontSize;
    message.cacheCount    = message.count;
    message.cacheColorful = colorful;
}

void CustomChat::onRenderEvent(RenderEvent& event)
{
    if (!mHudRegistered) registerHudElement();

    if (!ImGui::GetCurrentContext()) return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    const ImGuiIO& io = ImGui::GetIO();
    const float delta = std::clamp(io.DeltaTime, 0.0001f, 0.1f);

    updateMessages(delta);

    // Набор сообщения держим сами: ванильный экран чата не открывается (его
    // клавиши перехватывает onKeyEvent), поэтому ввод и отправка — наши.
    // updateInput() обязан работать всегда, пока идёт набор: он же обрабатывает
    // Esc/Enter. Настройка Input Box скрывает только РИСОВАНИЕ полосы, а не ввод —
    // иначе с выключенной настройкой чат нельзя было бы ни закрыть, ни отправить.
    const bool typing = mInputActive;
    if (typing) updateInput();
    else if (!mInput.empty()) mInput.clear();

    // Страховка от «залипшего» ввода: если игра всё же вышла из hud_screen
    // (её открыл другой экран) — отпускаем клавиатуру и мышь обратно.
    if (mInputActive && !isInGame()) closeInput();

    // Ванильный экран чата мы не открываем, но если он открылся как-то иначе —
    // всё равно рисуем свою полосу ввода поверх его поля, чтобы не было двух.
    const bool showStrip = (mInputActive || isChatOpen()) && mInputBox.mValue;

    const bool sample = mElement && mElement->mSampleMode;
    if (mMessages.empty() && !showStrip && !sample) return;

    FontHelper::pushPrefFont(false);
    ImFont* font = ImGui::GetFont();
    if (!font)
    {
        ImGui::PopFont();
        return;
    }

    const float fontSize   = std::clamp(mFontSize.mValue, 10.f, 28.f);
    const float lineHeight = ccMeasure(font, fontSize, "Ag").y;
    const float lineStep   = lineHeight + std::clamp(mLineSpacing.mValue, 0.f, 12.f);

    const ImVec2 display = io.DisplaySize;
    const ImVec2 origin  = mElement ? mElement->getPos() : ImVec2(8.f, display.y * 0.4f);

    // Ширина панели задаётся настройкой, но никогда не выходит за экран.
    const float panelWidth = std::clamp(mWidth.mValue, 160.f,
                                        (std::max)(220.f, display.x - origin.x - 8.f));
    const float contentWidth = (std::max)(64.f, panelWidth - kPadX * 2.f);
    const int   maxLines = static_cast<int>(std::clamp(mMaxLines.mValue, 3.f, 30.f));

    // Область чата: её высота задана настройкой Lines и НЕ меняется от сообщений,
    // иначе панель дёргалась бы на каждой строке.
    const float boxHeight = static_cast<float>(maxLines) * lineStep + kPadY * 2.f;

    // Позиция области. Если она не влезает по высоте/ширине (например, в hud.json
    // сохранена позиция под СТАРЫЙ размер панели), область поднимается/сдвигается
    // сама — иначе нижняя (самая свежая) строка чата оказалась бы за экраном.
    float boxTop  = origin.y;
    float boxLeft = origin.x;
    if (boxTop + boxHeight > display.y - 2.f)
        boxTop = (std::max)(4.f, display.y - 2.f - boxHeight);
    if (boxLeft + panelWidth > display.x - 2.f)
        boxLeft = (std::max)(2.f, display.x - 2.f - panelWidth);

    if (mElement)
    {
        mElement->mSize = { panelWidth, boxHeight };
        mElement->mVisible = true;

        // Позицию пишем обратно, чтобы рамка в HudEditor совпадала с реальной
        // областью чата (иначе после самоподъёма они разъезжаются на пару строк).
        if (boxTop != origin.y || boxLeft != origin.x)
            mElement->setFromPos(glm::vec2(boxLeft, boxTop));
    }

    // ── Отбираем то, что реально влезет: снизу вверх, не разрезая сообщение ──
    struct Visible {
        ChatMessage* message;
        float        height;
    };

    std::vector<Visible> visible;
    visible.reserve(32);

    float contentHeight = 0.f;
    int   usedLines = 0;

    for (auto it = mMessages.rbegin(); it != mMessages.rend(); ++it)
    {
        ChatMessage& message = *it;
        if (message.alpha <= 0.01f) continue;

        if (message.cacheWidth != contentWidth || message.cacheFontSize != fontSize ||
            message.cacheCount != message.count || message.cacheColorful != mColorful.mValue)
        {
            buildLines(message, font, fontSize, contentWidth, mColorful.mValue);
        }
        if (message.lines.empty()) continue;

        const int messageLines = static_cast<int>(message.lines.size());
        if (!visible.empty() && usedLines + messageLines > maxLines) break;

        contentHeight += static_cast<float>(messageLines) * lineStep;
        usedLines += messageLines;
        visible.push_back({ &message, static_cast<float>(messageLines) * lineStep });
    }

    // ── Панель сообщений ──────────────────────────────────────────────────────
    // Низ панели жёстко прибит к низу области, поэтому новые сообщения появляются
    // снизу и толкают старые вверх — как в ванильном чате. Раньше панель росла
    // вниз от верхней точки и самые свежие строки уезжали за нижний край экрана.
    if (!visible.empty() || sample)
    {
        const bool placeholder = visible.empty();
        const float panelHeight = placeholder
            ? lineStep * 2.f + kPadY * 2.f
            : (std::min)(contentHeight + kPadY * 2.f, boxHeight);

        const float panelBottom = boxTop + boxHeight;
        const ImVec2 panelMin(boxLeft, panelBottom - panelHeight);
        const ImVec2 panelMax(boxLeft + panelWidth, panelBottom);

        // Сплошная чёрная подложка: сообщение всегда читается и никогда не вылезает
        // за неё — ширина считается по самому длинному видимому сообщению.
        drawList->AddRectFilled(panelMin, panelMax, IM_COL32(0, 0, 0, 178), kRounding);
        drawList->AddRect(panelMin, panelMax, IM_COL32(255, 255, 255, 20), kRounding, 0, 1.f);

        if (placeholder)
        {
            // В HudEditor панель видно всегда — иначе её нельзя было бы поставить.
            if (sample)
            {
                const float hintY = panelMax.y - kPadY - lineHeight;
                drawList->AddText(font, fontSize, { panelMin.x + kPadX, hintY },
                                  IM_COL32(246, 246, 248, 235), "Custom chat");
                drawList->AddText(font, fontSize, { panelMin.x + kPadX, hintY - lineStep },
                                  IM_COL32(150, 152, 158, 235), "move me with HudEditor");
            }
        }
        else
        {
            // Самое старое сообщение — сверху, самое свежее — в самом низу панели.
            float y = panelMax.y - kPadY - lineHeight;

            for (const Visible& item : visible)
            {
                // Идём от свежих к старым, поэтому строки внутри сообщения тоже
                // раскладываем снизу вверх: последняя строка рисуется ниже всех.
                for (auto line = item.message->lines.rbegin(); line != item.message->lines.rend(); ++line)
                {
                    float x = panelMin.x + kPadX;
                    for (const TextRun& run : line->runs)
                    {
                        if (run.text.empty()) continue;
                        drawList->AddText(font, fontSize, { x, y },
                                          ccWithAlpha(run.color, item.message->alpha), run.text.c_str());
                        x += run.width;
                    }
                    y -= lineStep;
                }
            }
        }
    }

    // ── Строка ввода ──────────────────────────────────────────────────────────
    // Рисуется во всю ширину экрана там, где обычно поле ввода ванильного чата.
    if (showStrip)
    {
        const float stripHeight = lineHeight + kPadY * 2.f;
        const ImVec2 stripMin(2.f, display.y - stripHeight - 2.f);
        const ImVec2 stripMax(display.x - 2.f, display.y - 2.f);

        drawList->AddRectFilled(stripMin, stripMax, IM_COL32(0, 0, 0, 238), kRounding);
        drawList->AddRect(stripMin, stripMax, IM_COL32(255, 255, 255, 26), kRounding, 0, 1.f);

        float textX = stripMin.x + kPadX;
        if (ImFont* icons = FontHelper::getIconFont())
        {
            const ImVec2 glyphSize = ccMeasure(icons, NurikIcons::kGlyphSize, NurikIcons::kKeyboard);
            drawList->AddText(icons, NurikIcons::kGlyphSize,
                              { textX, stripMin.y + (stripHeight - glyphSize.y) * 0.5f },
                              IM_COL32(150, 152, 158, 235), NurikIcons::kKeyboard);
            textX += glyphSize.x + 8.f;
        }

        const float textY = stripMin.y + kPadY;
        // ImGui::GetTime() вместо io.Time: в этой версии ImGui поля Time у ImGuiIO нет.
        const bool  caret = std::fmod(static_cast<float>(ImGui::GetTime()), 1.f) < 0.5f;

        if (mInput.empty())
        {
            drawList->AddText(font, fontSize, { textX, textY }, IM_COL32(255, 255, 255, 92),
                              "Type a message...   (Enter to send, Esc to close)");
            if (caret)
                drawList->AddRectFilled({ textX - 1.f, textY }, { textX + 1.f, textY + lineHeight },
                                        IM_COL32(255, 255, 255, 200));
        }
        else
        {
            drawList->AddText(font, fontSize, { textX, textY }, IM_COL32(246, 246, 248, 245), mInput.c_str());
            if (caret)
            {
                const float typed = ccMeasure(font, fontSize, mInput).x;
                drawList->AddRectFilled({ textX + typed + 1.f, textY }, { textX + typed + 3.f, textY + lineHeight },
                                        IM_COL32(255, 255, 255, 200));
            }
        }
    }

    ImGui::PopFont();
}

void CustomChat::onPacketInEvent(PacketInEvent& event)
{
    if (event.isCancelled()) return;
    if (event.mPacket->getId() != PacketID::Text) return;

    auto* client = ClientInstance::get();
    if (!client || !client->getLocalPlayer()) return;

    auto textPacket = event.getPacket<TextPacket>();
    if (!textPacket) return;

    // Эхо собственного сообщения. Своё сообщение мы уже показали при отправке,
    // поэтому ответ сервера с тем же текстом отбрасываем — иначе строка была бы
    // видна дважды. Проверяем ТОЛЬКО текст сообщения: сверка с ником (author)
    // случайно гасила чужие пакеты, в тексте которых попадалось наше слово.
    // Сверяем по точному совпадению тела (включая case): «привет» и «Привет» —
    // разные строки, зато чужие сообщения с тем же словом внутри не теряются.
    if (!mLastSent.empty() && static_cast<float>(ImGui::GetTime()) - mLastSentTime < 3.f)
    {
        bool ignored = false;
        const std::string body = ccStripCodes(textPacket->mMessage, ignored);

        // Вариант 1: сервер вернул тело 1-в-1 (ванильный Bedrock).
        // Вариант 2: сервер вклеил ник в текст ("Ник: сообщение") — тогда наше
        // сообщение лежит «хвостом» сразу за разделителем. Проверка хвоста (а не
        // поиск по всей строке) нужна, чтобы не гасить чужие сообщения, где наше
        // слово просто встретилось в середине.
        bool isEcho = body == mLastSent;
        if (!isEcho && body.size() > mLastSent.size())
        {
            const size_t at = body.size() - mLastSent.size();
            if (body.compare(at, mLastSent.size(), mLastSent) == 0)
            {
                const char sep = body[at - 1];
                isEcho = (sep == ' ' || sep == ':' || sep == '>' || 0xC2 == static_cast<unsigned char>(body[at - 1]));
            }
        }

        if (isEcho)
        {
            event.setCancelled(true);
            return;
        }
    }

    // JSON-варианты пакета (TextObject*) нам не нужны — из них получалась каша.
    switch (textPacket->mType)
    {
        case TextPacketType::TextObject:
        case TextPacketType::TextObjectWhisper:
        case TextPacketType::TextObjectAnnouncement:
            event.setCancelled(true);
            return;
        default:
            break;
    }

    // Сохраняем СЫРОЙ текст: §-коды нужны для Colorful, а очищенный вариант
    // addMessage посчитает сам (и по нему же ищет повторы).
    std::string message;

    if (textPacket->mType == TextPacketType::Chat)
    {
        bool ignored = false;
        const std::string author = ccStripCodes(textPacket->mAuthor, ignored);
        const std::string plainBody = ccStripCodes(textPacket->mMessage, ignored);

        // Серверы обычно уже добавляют ник в текст сообщения, поэтому префикс
        // приклеиваем только если ника в тексте нет.
        if (!author.empty() && plainBody.find(author) == std::string::npos)
            message = "<" + author + "> " + textPacket->mMessage;
        else
            message = textPacket->mMessage;
    }
    else if (textPacket->mType == TextPacketType::Translate || textPacket->mLocalize)
    {
        message = textPacket->mMessage;

        // В Translate-пакетах вместо текста шаблон с %s, а параметры лежат рядом.
        size_t paramIndex = 0;
        size_t position = 0;
        while (paramIndex < textPacket->mParams.size() &&
               (position = message.find("%s", position)) != std::string::npos)
        {
            const std::string& value = textPacket->mParams[paramIndex++];
            message.replace(position, 2, value);
            position += value.size();
        }
    }
    else
    {
        message = textPacket->mMessage;
    }

    addMessage(message);

    // Главное: не даём ванильному чату показать это же сообщение,
    // иначе на экране было бы два списка сообщений.
    event.setCancelled(true);
}
