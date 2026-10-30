#pragma once

#include <string>

#include <Features/Modules/Module.hpp>
//
// Diagnostics — «почему ничего не работает», без IDA и без консоли.
//
// Модуль ничего не делает с игрой: он только слушает события и запоминает,
// какие из них вообще доходят, и по команде .diag показывает состояние
// клиента в этом запуске.
//
// Что видно:
//   * сколько сигнатур нашлось, а каких нет (по именам);
//   * какие перехваты создались и включились (журнал Detour);
//   * какие события реально приходят: тик, рендер, пакеты, нажатия клавиш;
//   * у каких модулей стоит галочка «включён», а какие реально запустили
//     onEnable (это две разные вещи — между ними стоит тик игры).
//
// Зачем: когда «всё сломано», причина всегда одна из четырёх — не тот билд, не
// нашлись сигнатуры, не встали хуки, не идёт тик. По этому списку видно, какая
// именно, за один заход.
//
class Diagnostics : public ModuleBase<Diagnostics>
{
public:
    // Всегда включён: он не влияет на игру, а спрашивают его как раз тогда,
    // когда что-то не работает и включить модуль руками может не получиться.
    Diagnostics() : ModuleBase("Diagnostics", "Reports what actually works in this build",
                               ModuleCategory::Misc, 0, true)
    {
        mNames = {
            {Lowercase,       "diagnostics"},
            {LowercaseSpaced, "diagnostics"},
            {Normal,          "Diagnostics"},
            {NormalSpaced,    "Diagnostics"}
        };
    }

    void onEnable() override;
    void onDisable() override;

    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);

    // Счётчики за всё время работы. Читает команда .diag.
    struct Counters
    {
        long long ticks       = 0;
        long long renders     = 0;
        long long packetsIn   = 0;
        long long packetsOut  = 0;
        long long tickMillis  = 0;   // сколько игрового времени прошло по тикам
    };

    static Counters counters();
    static void resetCounters();

    // Сколько времени (мс) прошло с последнего тика / рендера. -1 — не было ни
    // разу. Нужно, чтобы отличить «события не приходят» от «приходят редко».
    static long long millisSinceTick();
    static long long millisSinceRender();

    // Печатает отчёт в чат и возвращает его же текстом (для лога/буфера).
    static std::string report(bool includeModules);
};
