#pragma once
#include <Features/FeatureManager.hpp>
#include <Features/Events/NotifyEvent.hpp>

//
// Notifications — минималистичные чёрные карточки с цветной полоской-акцентом.
//
// Здесь сознательно НЕТ настроек цвета, свечения, шиммера, пульсации, углов,
// плотности, анимаций и прочего. Всё это удалено вместе с блюром и двойными
// тенями, которые и создавали лаги на каждом кадре.
//
// Осталось ровно четыре настройки: когда показывать, сколько держать и в каком
// углу. Внешний вид фиксирован: сплошной чёрный, белый текст, серая подпись,
// тонкая линия времени и цветная полоска-акцент слева (тип события — по её цвету:
// мятная — включено, серая — выключено, янтарная — предупреждение, коралловая —
// ошибка, холодная белая — инфо).
//
class Notifications : public ModuleBase<Notifications> {
public:
    enum class Position {
        TopRight,
        BottomRight,
    };

    // Тип события. Пиктограмм больше нет — вместо них цветная полоска-акцент
    // слева на карточке (цвет — accentColor в Notifications.cpp).
    enum class Icon {
        Success,
        Disabled,
        Warning,
        Error,
        Info,
    };

    BoolSetting mShowOnToggle = BoolSetting("Show on toggle", "Show a notification when a module is toggled", true);
    BoolSetting mShowOnJoin = BoolSetting("Show on join", "Show a notification when you join a server", true);
    NumberSetting mDuration = NumberSetting("Duration", "How long toggle / join notifications stay on screen", 3.f, 1.f, 10.f, 0.5f);
    EnumSettingT<Position> mPosition = EnumSettingT<Position>("Position", "Which corner the cards appear in", Position::TopRight, "Top Right", "Bottom Right");

    Notifications() : ModuleBase("Notifications", "Shows notifications on module toggle and other events", ModuleCategory::Visual, 0, true) {
        addSettings(&mShowOnToggle, &mShowOnJoin, &mDuration, &mPosition);

        mNames = {
            {Lowercase, "notifications"},
            {LowercaseSpaced, "notifications"},
            {Normal, "Notifications"},
            {NormalSpaced, "Notifications"}
        };

        gFeatureManager->mDispatcher->listen<RenderEvent, &Notifications::onRenderEvent, nes::event_priority::VERY_LAST>(this);
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    void onModuleStateChange(ModuleStateChangeEvent& event);
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
    void onNotifyEvent(class NotifyEvent& event);

private:
    // One card. Everything that can be precomputed is precomputed ONCE, when the
    // card is created — the render loop must not allocate strings or re-parse
    // colour codes every frame (that was a big part of the stutter).
    struct Entry {
        std::string text;       // already stripped of § codes
        std::string state;      // "enabled" / "disabled" / "" — drawn dimmed
        Icon        icon = Icon::Info;
        float       duration = 3.f;
        float       life = 0.f;  // seconds on screen
        float       width = 0.f; // measured lazily on the first frame
        float       textW = 0.f; // width of text (without the dimmed state word)
    };

    std::vector<Entry> mEntries;

    void push(std::string text, Icon icon, float duration, std::string state = {});
};
