#pragma once
//
// AutoContract — меню «/job contract» проходит само:
//   «Контракт выполнен»   -> «Забрать награду», потом снова /job contract;
//   «Доступные контракты» -> «Обновить список», пока не выпадет
//                            «Тяжёлый | Алмазная руда ...», и берёт его;
//   «Текущий контракт»    -> «Закрыть». «Отменить контракт» не нажимается
//                            никогда.
// Пока модуль включён, эти три меню не показываются: на них отвечает модуль,
// с паузой Click Delay перед каждым нажатием. Чужие меню не трогаются. Если
// что-то пошло не так (нет нужной кнопки, лимит обновлений), меню просто
// показывается — дальше решаешь сам.
//
// Когда запускается:
//   - при включении модуля (сам пишет /job contract);
//   - когда /job contract пишешь ты (или чат-бинд);
//   - раз в Recheck минут, если Recheck > 0: забрать выполненный контракт и
//     взять новый, пока копаешь.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <vector>

class AutoContract : public ModuleBase<AutoContract> {
public:
    NumberSetting mClickDelay = NumberSetting("Click Delay", "Pause before each button press, ms (randomized +-25%)", 500.f, 150.f, 2000.f, 50.f);
    NumberSetting mMaxRefresh = NumberSetting("Max Refresh", "Stop refreshing after this many tries and show the menu", 100.f, 5.f, 500.f, 5.f);
    NumberSetting mRecheck = NumberSetting("Recheck", "Every N minutes run /job contract again: claim a finished contract and take a new one (0 = off)", 0.f, 0.f, 30.f, 1.f);

    AutoContract() : ModuleBase("AutoContract", "/job contract: claims the reward, refreshes until a Heavy Diamond Ore contract, takes it and closes the menu", ModuleCategory::Misc, 0, false) {
        addSetting(&mClickDelay);
        addSetting(&mMaxRefresh);
        addSetting(&mRecheck);

        mNames = {
            {Lowercase, "autocontract"},
            {LowercaseSpaced, "auto contract"},
            {Normal, "AutoContract"},
            {NormalSpaced, "Auto Contract"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    std::string getSettingDisplay() override;

    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketInEvent(class PacketInEvent& event);

private:
    enum class Menu : uint8_t { None, Reward, List, Current };
    // Чего ждём после последнего нажатия.
    enum class Expect : uint8_t { Nothing, AnyForm, AfterClaim, AfterRefresh, AfterTake };

    // Паузы и лимиты, мс.
    static constexpr uint64_t kWorldSettleMs = 3000; // после входа в мир — не сразу
    static constexpr uint64_t kTickGapMs     = 2000; // тиков не было дольше — была загрузка
    static constexpr uint64_t kFirstFormMs   = 6000; // ответ на /job contract
    static constexpr uint64_t kAfterClaimMs  = 2500; // после награды список сам не открылся
    static constexpr uint64_t kRefreshMs     = 5000; // после «Обновить список» нет нового списка
    static constexpr uint64_t kAfterTakeMs   = 4000; // после выбора нет «Текущий контракт»
    static constexpr uint64_t kSameListMs    = 3000; // список не изменился — сервер просит подождать
    static constexpr int      kMaxResends    = 3;

    // Меню после разбора: текст без §-кодов (для сообщений) и он же в нижнем
    // регистре с «ё» -> «е» (для поиска). Длины строк совпадают байт в байт.
    struct Form {
        std::string title, titleLower;
        std::string content, contentLower;
        std::vector<std::string> buttons, buttonsLower;
    };

    // Нажатие, которое ждёт своей паузы.
    struct Click {
        bool active = false;
        unsigned int formId = 0;
        int button = -1;
        uint64_t dueAt = 0;
        Expect next = Expect::Nothing;
        std::string doneMessage; // в чат, когда нажали
    };

    // Что сделать после того, как отпустили mMutex.
    struct Actions {
        bool press = false;
        unsigned int formId = 0;
        int button = -1;
        bool command = false;
        std::vector<std::string> messages;
    };

    mutable std::mutex mMutex;
    bool mStartPending = false; // прогон по включению модуля
    bool mRunActive = false;
    bool mVerbose = false;      // прогон запустил игрок -> прогресс в чат
    Expect mExpect = Expect::Nothing;
    uint64_t mExpectSince = 0;
    uint64_t mLastRunEnd = 0;
    uint64_t mInWorldSince = 0;
    uint64_t mLastTick = 0;
    int mRefreshes = 0;
    int mTakeTries = 0;
    int mRewardTries = 0;
    int mResends = 0;
    int mForms = 0;
    Click mClick;
    std::string mTaken;
    std::string mLastList;      // кнопки прошлого списка — заметить «обновить» без обновления
    std::string mStatus;
    std::mt19937 mRng{std::random_device{}()};

    void beginRunLocked(uint64_t now, bool verbose);
    void endRunLocked(uint64_t now);
    void giveUpLocked(uint64_t now, const std::string& reason, Actions& out);
    void scheduleLocked(unsigned int formId, int button, Expect next, uint64_t now, std::string doneMessage = {}, uint64_t baseMs = 0);
    bool decideLocked(Menu menu, const Form& form, unsigned int formId, uint64_t now, Actions& out);
    std::string takenMessageLocked() const;
    static void perform(Actions& out);

    static void sendButton(unsigned int formId, int button);
    static uint64_t nowMs();
    static bool parseForm(const std::string& json, Form& form);
    static Menu classify(const Form& form);
    static bool isWantedContract(const std::string& lowerText);
    static int findButton(const Form& form, std::string_view needle);
    static std::string field(const std::string& text, const std::string& lowerText, std::string_view key);
    static std::string display(const std::string& raw);
    static std::string lower(const std::string& text);
};
