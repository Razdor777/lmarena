#pragma once
//
// NoPause — в базе 1.21.44 файл пустой (0 байт): модуль был зарегистрирован в
// ModuleManager, но кода у него не было. В 26.62 та же ситуация ломала сборку:
// #include "Misc/NoPause.hpp" + make_shared<NoPause>() без класса — ошибка
// компиляции. Заглушка оставлена, чтобы регистрация осталась на месте, когда
// появится реализация. Никакой логики (и никаких патчей) здесь нет.
//
// Что должен делать NoPause: не давать игре ставить себя на паузу, когда окно
// теряет фокус (в Bedrock это делает ScreenView через mPauseOnLostFocus).
// Сигнатуры под 1.26.x для этого не сняты.

#include <Features/Modules/Module.hpp>

class NoPause : public ModuleBase<NoPause> {
public:
    NoPause() : ModuleBase("NoPause", "Keeps the game running when the window loses focus", ModuleCategory::Misc, 0, false) {
        mNames = {
            {Lowercase, "nopause"},
            {LowercaseSpaced, "no pause"},
            {Normal, "NoPause"},
            {NormalSpaced, "No Pause"}
        };
    }
};
