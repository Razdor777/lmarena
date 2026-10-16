//
// AutoContract — меню «/job contract» проходит само (подробности в .hpp).
//
// Как это устроено: меню сервера — обычные формы Bedrock (ModalFormRequest с
// JSON: title / content / buttons). Узнаём своё меню по заголовку, решаем,
// какую кнопку нажать, прячем меню (пакет не доходит до игры) и через паузу
// отправляем серверу ответ ModalFormResponse с номером кнопки — ровно то же,
// что уходит, когда кнопку нажимаешь сам. Крестик — такой же ответ, только
// без кнопки, с причиной UserClosed.
//
// Сообщение сервера «Контракт выполнен!» в чате (TextPacket) запускает прогон
// сам: забрать награду и взять следующий контракт.
//

#include "AutoContract.hpp"

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/PacketID.hpp>
#include <SDK/Minecraft/Network/Packets/ModalFormResponsePacket.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/CommandUtils.hpp>

#include <algorithm>
#include <chrono>

// ── Включение / выключение ──────────────────────────────────────────────────

void AutoContract::onEnable()
{
    {
        std::lock_guard lock(mMutex);
        const uint64_t now = nowMs();
        mRunActive = false;
        mExpect = Expect::Nothing;
        mClick = {};
        mStatus.clear();
        mStartPending = true;
        mStartVerbose = true;
        mStartNotBefore = 0;
        mLastRunEnd = now;
        // Уже в мире — проверяем сразу. Модуль включился ещё при запуске игры
        // (из конфига) — ждём, пока зайдёшь на сервер и мир прогрузится.
        auto ci = ClientInstance::get();
        if (ci && ci->getLocalPlayer()) {
            mLastTick = now;
            mInWorldSince = now - kWorldSettleMs;
        } else {
            mLastTick = 0;
            mInWorldSince = now;
        }
    }
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoContract::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &AutoContract::onPacketInEvent>(this);
}

void AutoContract::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoContract::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &AutoContract::onPacketInEvent>(this);

    std::lock_guard lock(mMutex);
    mStartPending = false;
    mStartNotBefore = 0;
    mRunActive = false;
    mExpect = Expect::Nothing;
    mClick = {};
    mStatus.clear();
}

std::string AutoContract::getSettingDisplay()
{
    // Arraylist рисуется в другом потоке — только копия под замком.
    std::lock_guard lock(mMutex);
    return mStatus;
}

// ── Тик: нажатия по таймеру, таймауты, запуск ───────────────────────────────

void AutoContract::onBaseTickEvent(BaseTickEvent& event)
{
    (void)event;
    const uint64_t now = nowMs();
    Actions out;
    {
        std::lock_guard lock(mMutex);

        // Тиков долго не было (загрузка мира, переподключение): мир заново
        // прогружается, а ожидания сдвигаются на этот перерыв — таймауты не
        // должны срабатывать за время, пока игра стояла.
        const uint64_t gap = mLastTick ? now - mLastTick : 0;
        if (mLastTick == 0 || gap > kTickGapMs) {
            mInWorldSince = now;
            if (gap > 0) {
                mExpectSince += gap;
                if (mClick.active) mClick.dueAt += gap;
            }
        }
        mLastTick = now;

        if (mClick.active) {
            if (now >= mClick.dueAt) {
                out.press = true;
                out.formId = mClick.formId;
                out.button = mClick.button;
                if (!mClick.doneMessage.empty()) out.messages.push_back(std::move(mClick.doneMessage));
                mExpect = mClick.next;
                mExpectSince = now;
                mClick = {};
                if (mExpect == Expect::Nothing) endRunLocked(now);
            }
        } else if (mRunActive) {
            const uint64_t waited = now - mExpectSince;
            switch (mExpect) {
            case Expect::AnyForm:
                if (waited > kFirstFormMs)
                    giveUpLocked(now, "на /job contract меню не пришло", out);
                break;
            case Expect::AfterClaim:
                // Награду забрали, а список сервер сам не открыл — открываем.
                if (waited > kAfterClaimMs) {
                    out.command = true;
                    mExpect = Expect::AnyForm;
                    mExpectSince = now;
                }
                break;
            case Expect::AfterRefresh:
                // «Обновить список» не принёс новый список — открываем его
                // заново той же командой.
                if (waited > kRefreshMs) {
                    if (++mResends > kMaxResends) {
                        giveUpLocked(now, "список контрактов перестал приходить", out);
                    } else {
                        out.command = true;
                        mExpectSince = now;
                    }
                }
                break;
            case Expect::AfterTake:
                // Контракт выбран, «Текущий контракт» не пришёл — всё равно готово.
                if (waited > kAfterTakeMs) {
                    out.messages.push_back(takenMessageLocked());
                    endRunLocked(now);
                }
                break;
            case Expect::AfterClose:
                // После «Закрыть» меню «Работы» не открылось — закрывать нечего.
                if (waited > kAfterCloseMs) endRunLocked(now);
                break;
            case Expect::Nothing:
                endRunLocked(now);
                break;
            }
        }

        if (!mRunActive && !mClick.active && now - mInWorldSince >= kWorldSettleMs) {
            const uint64_t every = static_cast<uint64_t>(std::max(0.f, mRecheck.mValue)) * 60000ull;
            const bool due = mStartPending && now >= mStartNotBefore;
            if (due || (every > 0 && now - mLastRunEnd >= every)) {
                beginRunLocked(now, due && mStartVerbose);
                mStartPending = false;
                mStartNotBefore = 0;
                out.command = true;
            }
        }
    }
    perform(out);
}

// ── Пакеты: пришло меню ──────────────────────────────────────────────────────

void AutoContract::onPacketInEvent(PacketInEvent& event)
{
    if (!event.mPacket) return;
    const PacketID id = event.mPacket->getId();

    if (id == PacketID::ChangeDimension) {
        std::lock_guard lock(mMutex);
        mInWorldSince = nowMs();
        return;
    }

    // «<+> Контракт выполнен! Напишите - /job contracts чтобы забрать награду»:
    // через 1–2 с (как человек) пишем /job contract — дальше обычный прогон:
    // награда, новый тяжёлый контракт на алмазы, «Закрыть», крестик. Идёт
    // прогон — новый начнётся сразу после него.
    if (id == PacketID::Text) {
        const auto text = event.getPacket<TextPacket>();
        if (!text || !text->mAuthor.empty() || !isCompletedMessage(text->mMessage)) return;
        std::lock_guard lock(mMutex);
        std::uniform_int_distribution<int> wait(kChatDelayMinMs, kChatDelayMaxMs);
        const uint64_t at = nowMs() + static_cast<uint64_t>(wait(mRng));
        if (!mStartPending) {
            mStartVerbose = false;
            mStartNotBefore = at;
        } else {
            mStartNotBefore = std::max(mStartNotBefore, at);
        }
        mStartPending = true;
        return;
    }

    if (id != PacketID::ModalFormRequest) return;

    const auto packet = event.getPacket<ModalFormRequestPacket>();
    Form form;
    if (!parseForm(packet->mJSON, form)) return;
    const Menu menu = classify(form);
    if (menu == Menu::None) return; // чужое меню — показываем как есть

    Actions out;
    bool hide = false;
    {
        std::lock_guard lock(mMutex);
        hide = decideLocked(menu, form, packet->mFormId, nowMs(), out);
    }
    // На это меню ответит модуль — игроку его не показываем. Если решить не
    // получилось, меню показывается как обычно.
    if (hide) event.cancel();
    perform(out);
}

bool AutoContract::decideLocked(Menu menu, const Form& form, unsigned int formId, uint64_t now, Actions& out)
{
    // «Работы» (главное меню /job) — только посреди своего прогона: после
    // «Закрыть» сервер открывает его сам. Открыл /job ты — меню твоё.
    if (menu == Menu::Jobs && !mRunActive) return false;

    // Меню контрактов открыли руками (/job contract в чат) — это тоже прогон.
    if (!mRunActive) {
        beginRunLocked(now, true);
        mStartPending = false;
    }
    mResends = 0;
    mClick = {}; // новое меню заменило прежнее — старое нажатие не нужно

    if (++mForms > static_cast<int>(mMaxRefresh.mValue) + 30) {
        giveUpLocked(now, "слишком много меню подряд, остановился", out);
        return false;
    }

    switch (menu) {
    case Menu::Jobs: {
        // Ждём меню контрактов, а пришло главное — заходим в «Контракты».
        if (mExpect == Expect::AnyForm || mExpect == Expect::AfterClaim) {
            const int contracts = findButton(form, "контракт");
            if (contracts >= 0) {
                scheduleLocked(formId, contracts, Expect::AnyForm, now);
                return true;
            }
        }
        // После «Закрыть» — крестик, и прогон закончен. Пришло посреди
        // чего-то другого — тоже крестик, ждём дальше то же, что ждали.
        scheduleLocked(formId, kCloseButton, mExpect == Expect::AfterClose ? Expect::Nothing : mExpect, now);
        return true;
    }

    case Menu::Reward: {
        int button = findButton(form, "забрать");
        if (button < 0 && form.buttons.size() == 1 && form.buttonsLower[0].find("отмен") == std::string::npos)
            button = 0;
        if (button < 0 || ++mRewardTries > 3) {
            giveUpLocked(now, "награду забрать не вышло — меню показано", out);
            return false;
        }
        const std::string reward = field(form.content, form.contentLower, "награда:");
        out.messages.push_back(reward.empty()
            ? std::string("§7[AutoContract] §aКонтракт выполнен§7, забираю награду")
            : "§7[AutoContract] §aКонтракт выполнен§7, награда: §f" + reward);
        mStatus = "Claim";
        scheduleLocked(formId, button, Expect::AfterClaim, now);
        return true;
    }

    case Menu::List: {
        for (size_t i = 0; i < form.buttonsLower.size(); ++i) {
            if (!isWantedContract(form.buttonsLower[i])) continue;
            if (++mTakeTries > 3) {
                giveUpLocked(now, "контракт не берётся — меню показано", out);
                return false;
            }
            mTaken = form.buttons[i];
            mStatus = "Take";
            scheduleLocked(formId, static_cast<int>(i), Expect::AfterTake, now);
            return true;
        }

        const int refresh = findButton(form, "обновить");
        if (refresh < 0) {
            giveUpLocked(now, "нет кнопки «Обновить список» — меню показано", out);
            return false;
        }
        if (mRefreshes >= static_cast<int>(mMaxRefresh.mValue)) {
            giveUpLocked(now, fmt::format("за {} обновлений тяжёлый контракт на алмазную руду не выпал — выбери сам", mRefreshes), out);
            return false;
        }
        // Список тот же, что и в прошлый раз: обновление не сработало (у
        // сервера пауза между обновлениями) — следующее нажатие не сразу, а
        // через ~3 с, чтобы не долбить сервер.
        std::string list;
        for (const auto& b : form.buttonsLower) list += b + '\n';
        const bool same = list == mLastList;
        mLastList = std::move(list);

        ++mRefreshes;
        mStatus = fmt::format("Refresh {}", mRefreshes);
        scheduleLocked(formId, refresh, Expect::AfterRefresh, now, {}, same ? kSameListMs : 0);
        return true;
    }

    case Menu::Current: {
        const int button = findButton(form, "закрыть");
        if (button < 0) {
            giveUpLocked(now, "нет кнопки «Закрыть» — меню показано", out);
            return false;
        }
        const std::string goal = field(form.content, form.contentLower, "цель:");
        const std::string progress = field(form.content, form.contentLower, "прогресс:");
        if (!progress.empty()) mStatus = progress.substr(0, progress.find(" ("));

        std::string message;
        if (mExpect == Expect::AfterTake)
            message = takenMessageLocked();
        else if (mVerbose)
            message = fmt::format("§7[AutoContract] §fТекущий контракт: {} §7| прогресс §f{}",
                                  goal.empty() ? "?" : goal, progress.empty() ? "?" : progress);
        // После «Закрыть» сервер открывает «Работы» — его закроем крестиком.
        // Крестиком сам «Текущий контракт» не закрываем: на части серверов
        // крестик путается с первой кнопкой, а первая тут «Отменить контракт».
        scheduleLocked(formId, button, Expect::AfterClose, now, std::move(message));
        return true;
    }

    default:
        return false;
    }
}

// ── Состояние прогона ────────────────────────────────────────────────────────

void AutoContract::beginRunLocked(uint64_t now, bool verbose)
{
    mRunActive = true;
    mVerbose = verbose;
    mExpect = Expect::AnyForm;
    mExpectSince = now;
    mRefreshes = mTakeTries = mRewardTries = mResends = mForms = 0;
    mTaken.clear();
    mLastList.clear();
}

void AutoContract::endRunLocked(uint64_t now)
{
    mRunActive = false;
    mExpect = Expect::Nothing;
    mLastRunEnd = now;
}

void AutoContract::giveUpLocked(uint64_t now, const std::string& reason, Actions& out)
{
    out.messages.push_back("§7[AutoContract] §c" + reason);
    mClick = {};
    mStatus = "Stopped";
    endRunLocked(now);
}

void AutoContract::scheduleLocked(unsigned int formId, int button, Expect next, uint64_t now, std::string doneMessage, uint64_t baseMs)
{
    // Как человек: пауза Click Delay ±25%, каждый раз разная.
    std::uniform_real_distribution<float> jitter(0.75f, 1.25f);
    const float base = std::max(std::clamp(mClickDelay.mValue, 50.f, 5000.f), static_cast<float>(baseMs));
    const float ms = base * jitter(mRng);
    mClick.active = true;
    mClick.formId = formId;
    mClick.button = button;
    mClick.dueAt = now + static_cast<uint64_t>(ms);
    mClick.next = next;
    mClick.doneMessage = std::move(doneMessage);
}

std::string AutoContract::takenMessageLocked() const
{
    return fmt::format("§7[AutoContract] §aВзял контракт: §f{} §7(обновлений списка: {})",
                       mTaken.empty() ? "?" : mTaken, mRefreshes);
}

void AutoContract::perform(Actions& out)
{
    if (out.press) {
        if (out.button == kCloseButton) sendClose(out.formId);
        else sendButton(out.formId, out.button);
    }
    if (out.command) CommandUtils::executeCommand("/job contract");
    for (const auto& message : out.messages) ChatUtils::displayClientMessage(message);
}

void AutoContract::sendButton(unsigned int formId, int button)
{
    auto ci = ClientInstance::get();
    if (!ci || !ci->getLocalPlayer() || !ci->getPacketSender()) return;

    auto packet = MinecraftPackets::createPacket<ModalFormResponsePacket>();
    if (!packet) return;
    packet->mFormId = formId;

    MinecraftJson::Value json;
    json.mType = MinecraftJson::ValueType::Int;
    json.mValue.mInt = button;
    packet->mJSONResponse = json;

    ci->getPacketSender()->send(packet.get());
    spdlog::info("[AutoContract] form {} -> button {}", formId, button);
}

void AutoContract::sendClose(unsigned int formId)
{
    // Крестик: ответ без кнопки, причина UserClosed — то же, что шлёт игра.
    auto ci = ClientInstance::get();
    if (!ci || !ci->getLocalPlayer() || !ci->getPacketSender()) return;

    auto packet = MinecraftPackets::createPacket<ModalFormResponsePacket>();
    if (!packet) return;
    packet->mFormId = formId;
    packet->mFormCancelReason = ModalFormCancelReason::UserClosed;

    ci->getPacketSender()->send(packet.get());
    spdlog::info("[AutoContract] form {} -> closed", formId);
}

bool AutoContract::isCompletedMessage(const std::string& raw)
{
    // «<+> Контракт выполнен! Напишите - /job contracts чтобы забрать награду».
    // Фраза должна стоять в самом начале (перед ней только «<+>»): так чьё-то
    // сообщение в чате («Ник » контракт выполнен») прогон не запустит.
    const std::string text = lower(display(raw));
    const size_t at = text.find("контракт выполнен");
    if (at == std::string::npos || at > 12) return false;
    return text.substr(0, at).find("»") == std::string::npos;
}

uint64_t AutoContract::nowMs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// ── Разбор меню ──────────────────────────────────────────────────────────────

bool AutoContract::parseForm(const std::string& json, Form& form)
{
    try {
        const nlohmann::json j = nlohmann::json::parse(json, nullptr, false);
        if (j.is_discarded() || !j.is_object()) return false;

        // Меню с кнопками — "form". "modal" (да/нет) и "custom_form" не наши.
        const auto type = j.find("type");
        if (type == j.end() || !type->is_string() || type->get<std::string>() != "form") return false;

        // Обычно строка; скриптовые серверы (BDS) шлют {"rawtext":[{"text":...}]}.
        const auto text = [](const nlohmann::json& v) -> std::string {
            if (v.is_string()) return v.get<std::string>();
            if (!v.is_object()) return {};
            const auto raw = v.find("rawtext");
            if (raw != v.end() && raw->is_array()) {
                std::string out;
                for (const auto& part : *raw) {
                    if (!part.is_object()) continue;
                    const auto t = part.find("text");
                    if (t != part.end() && t->is_string()) out += t->get<std::string>();
                    const auto tr = part.find("translate");
                    if (tr != part.end() && tr->is_string()) out += tr->get<std::string>();
                }
                return out;
            }
            const auto t = v.find("text");
            return (t != v.end() && t->is_string()) ? t->get<std::string>() : std::string();
        };
        const auto get = [&](const char* key) -> std::string {
            const auto it = j.find(key);
            return it == j.end() ? std::string() : text(*it);
        };

        form.title = display(get("title"));
        form.titleLower = lower(form.title);
        form.content = display(get("content"));
        form.contentLower = lower(form.content);

        const auto buttons = j.find("buttons");
        if (buttons != j.end() && buttons->is_array()) {
            for (const auto& b : *buttons) {
                std::string t;
                if (b.is_object()) {
                    const auto it = b.find("text");
                    if (it != b.end()) t = text(*it);
                } else {
                    t = text(b);
                }
                form.buttons.push_back(display(t));
                form.buttonsLower.push_back(lower(form.buttons.back()));
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

AutoContract::Menu AutoContract::classify(const Form& form)
{
    const auto has = [](const std::string& s, std::string_view needle) {
        return s.find(needle) != std::string::npos;
    };
    const auto anyButton = [&](std::string_view needle) {
        return std::any_of(form.buttonsLower.begin(), form.buttonsLower.end(),
                           [&](const std::string& b) { return has(b, needle); });
    };

    if (form.buttons.empty()) return Menu::None;

    // Меню контрактов: «контракт» в заголовке или тексте. Иначе какое-нибудь
    // меню аукциона с «Обновить список» приняли бы за своё.
    if (has(form.titleLower, "контракт") || has(form.contentLower, "контракт")) {
        if (has(form.titleLower, "текущ") || anyButton("отменить контракт")) return Menu::Current;
        if (has(form.titleLower, "выполнен") || anyButton("забрать наград")) return Menu::Reward;
        if (has(form.titleLower, "доступн") || anyButton("обновить список")) return Menu::List;
    }

    // «Работы» — главное меню /job («Ваша работа Майнер | уровня 3 | ...»,
    // кнопки «Контракты», «Уволиться», ...). Обязательно с кнопкой
    // «Контракты»: какое-нибудь «Уволиться? Да/Нет» сюда не попадёт.
    if (has(form.titleLower, "работ") && anyButton("контракт") &&
        (has(form.contentLower, "ваша работа") || anyButton("уволиться")))
        return Menu::Jobs;
    return Menu::None;
}

bool AutoContract::isWantedContract(const std::string& b)
{
    // «Тяжёлый | Алмазная руда x197 | 1095 монет + 39 опыта работы»
    // (уже без цветов, в нижнем регистре, «ё» -> «е»).
    if (b.find("обновить") != std::string::npos || b.find("отмен") != std::string::npos) return false;
    if (b.find("тяжел") == std::string::npos) return false;

    // Именно «Алмазная руда», а не «Глубинносланцевая алмазная руда»: перед
    // названием — начало строки или разделитель «|».
    constexpr std::string_view ore = "алмазная руда";
    for (size_t p = b.find(ore); p != std::string::npos; p = b.find(ore, p + 1)) {
        size_t q = p;
        while (q > 0 && b[q - 1] == ' ') --q;
        if (q == 0 || b[q - 1] == '|') return true;
    }
    return false;
}

int AutoContract::findButton(const Form& form, std::string_view needle)
{
    // Кнопку с «отмен» («Отменить контракт») не возвращает никогда.
    for (size_t i = 0; i < form.buttonsLower.size(); ++i) {
        const std::string& b = form.buttonsLower[i];
        if (b.find(needle) != std::string::npos && b.find("отмен") == std::string::npos)
            return static_cast<int>(i);
    }
    return -1;
}

std::string AutoContract::field(const std::string& text, const std::string& lowerText, std::string_view key)
{
    // «... | Прогресс: 0/197 (0%) | ...» -> «0/197 (0%)»
    const size_t at = lowerText.find(key);
    if (at == std::string::npos) return {};
    const size_t begin = at + key.size();
    size_t end = text.find('|', begin);
    if (end == std::string::npos) end = text.size();
    const std::string value = text.substr(begin, end - begin);
    const size_t a = value.find_first_not_of(' ');
    if (a == std::string::npos) return {};
    const size_t z = value.find_last_not_of(' ');
    return value.substr(a, z - a + 1);
}

std::string AutoContract::display(const std::string& raw)
{
    // Без §-кодов, переносы строк и повторные пробелы -> один пробел.
    const std::string s = ChatUtils::stripColorCodes(raw);
    std::string out;
    out.reserve(s.size());
    bool space = false;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        const bool nbsp = c == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0xA0;
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || nbsp) {
            if (nbsp) ++i;
            space = !out.empty();
            continue;
        }
        if (space) {
            out += ' ';
            space = false;
        }
        out += static_cast<char>(c);
    }
    return out;
}

std::string AutoContract::lower(const std::string& text)
{
    // Нижний регистр для латиницы и кириллицы, «ё»/«Ё» -> «е». Длина в байтах
    // не меняется, поэтому позиции совпадают с исходной строкой.
    std::string out = text;
    for (size_t i = 0; i < out.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(out[i]);
        if (c >= 'A' && c <= 'Z') {
            out[i] = static_cast<char>(c + 32);
            continue;
        }
        if ((c == 0xD0 || c == 0xD1) && i + 1 < out.size()) {
            const unsigned char d = static_cast<unsigned char>(out[i + 1]);
            if ((d & 0xC0) != 0x80) continue;
            unsigned cp = ((c & 0x1Fu) << 6) | (d & 0x3Fu);
            if (cp >= 0x410 && cp <= 0x42F) cp += 0x20;      // А..Я -> а..я
            else if (cp == 0x401 || cp == 0x451) cp = 0x435; // Ё, ё -> е
            out[i] = static_cast<char>(0xC0 | (cp >> 6));
            out[i + 1] = static_cast<char>(0x80 | (cp & 0x3F));
            ++i;
        }
    }
    return out;
}
