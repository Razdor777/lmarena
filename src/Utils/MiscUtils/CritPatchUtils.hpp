#pragma once
//
// CritPatch — поиск в нативном коде игры цепочки проверок «можно ли засчитать
// критический удар» и её отключение.
//
// Что ищем. В функции обработки удара проверки идут подряд и все «плохие»
// исходы ведут в одно и то же место — на байт сразу за ПОСЛЕДНИМ условным
// переходом. Последний же переход, наоборот, уходит далеко вперёд, в блок, где
// крит считается. В 1.26.51 это выглядит так (смещения от начала функции):
//
//   +0x1E4  0F 85 92 01 00 00   -> +0x37C
//   +0x1F8  0F 86 7E 01 00 00   -> +0x37C
//   +0x29D  0F 82 D9 00 00 00   -> +0x37C
//   +0x2AD  0F 85 C9 00 00 00   -> +0x37C
//   +0x34D  72 2D               -> +0x37C
//   +0x35F  75 1B               -> +0x37C
//   +0x36C  75 0E               -> +0x37C
//   +0x376  0F 85 96 03 00 00   -> +0x712   (крит-блок)
//   +0x37C  <- сюда ведут все семь предыдущих
//
// Из этого и получается «сигнатура», только не из байтов, а из структуры:
// семь условных переходов делят одну цель, и эта цель — ровно тот адрес, на
// который «проваливается» восьмой переход. Такое совпадение случайно не
// встречается, зато переживает пересборку игры: RVA меняются, а форма — нет.
//
// Патч: семь переходов забиваются NOP (перестают уводить в «не крит»), восьмой
// превращается в безусловный jmp (всегда в крит-блок). jmp короче на байт,
// поэтому смещение считается ОТ АДРЕСА + 5, а шестой байт — 0x90.
//
// Здесь только арифметика над байтами — без Windows, чтобы это можно было
// проверить тестом на синтетическом буфере.
//

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace critpatch
{
    struct Site
    {
        uintptr_t address = 0;
        uint8_t   length  = 0;      // 2 — short jcc, 6 — near jcc
        uintptr_t target  = 0;
        bool      force   = false;  // восьмой переход: станет безусловным jmp
    };

    struct Plan
    {
        std::vector<Site> sites;
        uintptr_t abortBlock = 0;   // общая цель семи переходов («не крит»)
        uintptr_t critBlock  = 0;   // цель восьмого (крит)

        [[nodiscard]] bool valid() const { return sites.size() >= 6 && critBlock != 0; }
    };

    inline bool isNearJcc(const uint8_t* p)
    {
        return p[0] == 0x0F && p[1] >= 0x80 && p[1] <= 0x8F;
    }

    inline bool isShortJcc(const uint8_t* p)
    {
        return p[0] >= 0x70 && p[0] <= 0x7F;
    }

    inline int32_t readRel32(const uint8_t* p)
    {
        int32_t value = 0;
        std::memcpy(&value, p + 2, sizeof(value));
        return value;
    }

    // Насколько далеко вверх ищем «соседей» с той же целью. В 1.26.51 верхний
    // переход на 0x169 байт выше кластера, 0x400 — с большим запасом.
    inline constexpr size_t kMaxBack   = 0x400;
    inline constexpr size_t kMinSites  = 5;      // меньше — это не наша цепочка
    inline constexpr size_t kMaxCritGap = 0x2000; // цель восьмого: не ближе 0x80 и не дальше

    inline bool find(const uint8_t* code, size_t size, uintptr_t base, Plan& plan, std::string& why)
    {
        plan = {};
        why.clear();

        if (code == nullptr || size < 0x40)
        {
            why = "сегмент кода пуст";
            return false;
        }

        size_t bestScore = 0;

        for (size_t i = 0; i + 6 <= size; ++i)
        {
            if (!isNearJcc(code + i)) continue;

            const uintptr_t at   = base + i;
            const uintptr_t fall = at + 6;
            const uintptr_t tgt  = static_cast<uintptr_t>(static_cast<int64_t>(fall) + readRel32(code + i));

            // Крит-блок должен быть заметно дальше и не улетать за пределы разумного.
            if (tgt <= fall + 0x80 || tgt > fall + kMaxCritGap) continue;

            std::vector<Site> found;
            const size_t lo = (i > kMaxBack) ? i - kMaxBack : 0;

            for (size_t j = lo; j < i; ++j)
            {
                if (isNearJcc(code + j) && j + 6 <= size)
                {
                    const uintptr_t target = static_cast<uintptr_t>(
                        static_cast<int64_t>(base + j + 6) + readRel32(code + j));
                    if (target == fall) found.push_back({ base + j, 6, fall, false });
                }
                else if (isShortJcc(code + j) && j + 2 <= size)
                {
                    const int8_t disp = static_cast<int8_t>(code[j + 1]);
                    const uintptr_t target = static_cast<uintptr_t>(
                        static_cast<int64_t>(base + j + 2) + disp);
                    if (target == fall) found.push_back({ base + j, 2, fall, false });
                }
            }

            if (found.size() < kMinSites || found.size() <= bestScore) continue;

            // Перекрытия: short jcc, случайно попавший внутрь операнда near jcc,
            // дал бы «лишнего соседа». Такие наборы отбрасываем.
            bool overlap = false;
            for (size_t a = 1; a < found.size(); ++a)
            {
                if (found[a].address < found[a - 1].address + found[a - 1].length)
                {
                    overlap = true;
                    break;
                }
            }
            if (overlap) continue;

            bestScore = found.size();

            plan.sites       = found;
            plan.sites.push_back({ at, 6, tgt, true });
            plan.abortBlock  = fall;
            plan.critBlock   = tgt;
        }

        if (bestScore == 0)
        {
            why = "нет цепочки переходов с общей целью";
            return false;
        }

        return true;
    }

    // Байты, которыми сайт заменяется. Для jmp — E9 + rel32 (от адреса +5).
    inline std::vector<uint8_t> patchedBytes(const Site& site, bool& ok)
    {
        ok = true;

        if (!site.force)
        {
            if (site.length != 2 && site.length != 6) { ok = false; return {}; }
            return std::vector<uint8_t>(site.length, 0x90);
        }

        const int64_t delta = static_cast<int64_t>(site.target) - static_cast<int64_t>(site.address + 5);
        if (delta < INT32_MIN || delta > INT32_MAX) { ok = false; return {}; }

        const int32_t rel = static_cast<int32_t>(delta);
        std::vector<uint8_t> out = {
            0xE9,
            static_cast<uint8_t>(rel & 0xFF),
            static_cast<uint8_t>((rel >> 8) & 0xFF),
            static_cast<uint8_t>((rel >> 16) & 0xFF),
            static_cast<uint8_t>((rel >> 24) & 0xFF)
        };
        if (site.length == 6) out.push_back(0x90);
        return out;
    }

    bool apply(const Plan& plan);
    void restore();
    [[nodiscard]] bool isApplied();
}
