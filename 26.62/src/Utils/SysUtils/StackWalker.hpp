#pragma once
//
// Created by vastrakai on 7/7/2024.
//
// Стек вызовов для краш-репорта (dbghelp). dbghelp не потокобезопасен: в один
// момент времени — один экземпляр (его создаёт только краш-репорт).
//

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

class StackWalker {
public:
    StackWalker();
    ~StackWalker();

    // Стек потока по его регистрам. Регистры копируются (StackWalk64 их
    // портит), так что pContext можно передавать прямо из EXCEPTION_POINTERS.
    // hThread — настоящий хэндл этого потока (не псевдо-хэндл другого потока).
    std::vector<std::string> ShowCallstack(HANDLE hThread, PCONTEXT pContext, int maxFrames = 64);

    // "Solstice.dll+0x1A2B3C OreMinerV2::processSub (OreMinerV2.cpp:452)".
    // returnAddress — адрес возврата (кадры стека, кроме нулевого): строку
    // ищем по адресу-1, иначе она указывала бы на следующую за вызовом.
    std::string describe(uint64_t address, bool returnAddress = false);

    // Загружены ли символы (PDB) Solstice.dll; если нет — где его искали.
    std::string symbolStatus();

private:
    bool mInitialized = false;
};
