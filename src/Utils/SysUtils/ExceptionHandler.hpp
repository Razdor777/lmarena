#pragma once
//
// Created by vastrakai on 7/7/2024.
//
// Краш-репорты. При любом падении игры с Solstice в Juzdex\crash.log
// дописывается отчёт: что за исключение, в какой функции (с файлом и строкой,
// если PDB доступен), какой модуль и на каком событии работал в этот момент,
// какой это поток, стек вызовов, последние строки лога и включённые модули.
// Рядом, в Juzdex\Crashes\, кладётся минидамп (.dmp) — его можно открыть в
// Visual Studio вместе с PDB и увидеть точную строку.
//

#include <Windows.h>
#include <string>

class ExceptionHandler {
public:
    // Роль потока — в отчёте видно, в каком потоке упало (игра / рендер / ввод).
    enum ThreadRole : unsigned {
        Game       = 1u << 0, // Actor::baseTick: тики модулей, пакеты
        Render     = 1u << 1, // Present: ImGui, RenderEvent
        Input      = 1u << 2, // Keyboard::feed / MouseDevice::feed
        Background = 1u << 3, // поток Solstice (включение модулей в меню, конфиги)
    };

    static void init();
    // Перед выгрузкой DLL (END): вернуть прежний обработчик, иначе следующий
    // краш процесса прыгнул бы в уже выгруженный код.
    static void shutdown();
    static void makeCrashLog(const std::string& text, DWORD exceptionCode);

    // Дёшево (одна проверка thread_local), можно звать каждый тик/кадр.
    static void markThread(ThreadRole role) noexcept;
};
