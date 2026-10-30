#pragma once
//
// CritPatch — поиск в нативном коде игры цепочки проверок «можно ли засчитать
// критический удар» и её отключение.
//
// ─── Откуда это взялось ─────────────────────────────────────────────────────
//
// В 1.26.51 (по присланному рабочему модулю) проверки критов — восемь условных
// переходов внутри одной функции обработки удара. Семь «плохих» исходов ведут в
// одно и то же место, а восьмой уходит далеко вперёд, в крит-блок:
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
// Ключевое: цель семи переходов — ровно байт за последним переходом. Это не
// случайность, а свойство кода: проверки идут подряд, а «не крит» — это код
// сразу после последней проверки.
//
// ─── Что здесь ──────────────────────────────────────────────────────────────
//
// Поэтому детектор не привязан ни к RVA, ни к байтам: он ищет форму. RVA у
// каждой сборки свои, и уже проверено, что побайтовый перенос с 1.26.51 на
// 1.26.10 не работает — совпадают только величины смещений, и те в 86 разных
// функциях.
//
// Патч: семь переходов забиваются NOP (перестают уводить в «не крит»), восьмой
// превращается в безусловный jmp в крит-блок. jmp короче на байт, поэтому
// смещение считается ОТ АДРЕСА + 5, а шестой байт — 0x90.
//
// Здесь только арифметика над байтами — без Windows, чтобы это можно было
// проверить тестом на синтетическом буфере.
//

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>
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

    // Найденный кандидат: цепочка переходов + куда ведёт крит.
    struct Candidate
    {
        uintptr_t anchor    = 0;    // последний переход (станет безусловным jmp)
        uintptr_t abortBlock = 0;   // = anchor + длина перехода
        uintptr_t critBlock = 0;    // цель последнего перехода
        std::vector<Site> sites;    // остальные переходы, по возрастанию адреса
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

    inline uintptr_t jccTarget(uintptr_t at, const uint8_t* p)
    {
        if (p[0] >= 0x70 && p[0] <= 0x7F)
        {
            const int8_t disp = static_cast<int8_t>(p[1]);
            return static_cast<uintptr_t>(static_cast<int64_t>(at) + 2 + disp);
        }
        if (p[0] == 0x0F && p[1] >= 0x80 && p[1] <= 0x8F)
        {
            return static_cast<uintptr_t>(static_cast<int64_t>(at) + 6 + readRel32(p));
        }
        return 0;
    }

    // Насколько далеко вверх ищем «соседей» с той же целью. В 1.26.51 верхний
    // переход на 0x169 байт выше кластера, 0x400 — с большим запасом.
    inline constexpr size_t kMaxBack     = 0x400;
    inline constexpr size_t kMinSites    = 5;      // для применения
    inline constexpr size_t kScanMinSites = 3;     // для диагностического скана
    inline constexpr size_t kMaxCritGap  = 0x2000; // цель восьмого: не ближе 0x80 и не дальше

    // Раскладка сайтов в 1.26.51 относительно последнего перехода. Адреса
    // съезжают, а расстояния между проверками — нет.
    inline constexpr intptr_t kReferenceFingerprint[] = {
        -0x192, -0x17E, -0xD9, -0xC9, -0x29, -0x17, -0xA
    };
    inline constexpr size_t kReferenceFingerprintSize =
        sizeof(kReferenceFingerprint) / sizeof(kReferenceFingerprint[0]);

    // "-0x192 -0x17E -0xD9 ..." — смещения остальных переходов от anchor.
    inline std::string fingerprint(const Candidate& candidate)
    {
        std::string out;
        char buffer[24];

        for (const Site& site : candidate.sites)
        {
            const intptr_t delta = static_cast<intptr_t>(site.address)
                                 - static_cast<intptr_t>(candidate.anchor);
            std::snprintf(buffer, sizeof(buffer), "%s0x%llX",
                          (delta < 0 ? "-" : "+"),
                          static_cast<unsigned long long>(delta < 0 ? -delta : delta));
            if (!out.empty()) out += ' ';
            out += buffer;
        }
        return out;
    }

    // Насколько найденная раскладка похожа на эталон: доля переходов, стоящих на
    // «своих» расстояниях. 1.0 — один в один.
    inline float similarity(const Candidate& candidate)
    {
        if (candidate.sites.empty() || kReferenceFingerprintSize == 0) return 0.f;

        size_t hits = 0;
        for (const Site& site : candidate.sites)
        {
            const intptr_t delta = static_cast<intptr_t>(site.address)
                                 - static_cast<intptr_t>(candidate.anchor);
            for (const intptr_t reference : kReferenceFingerprint)
            {
                if (delta == reference) { ++hits; break; }
            }
        }
        return static_cast<float>(hits) / static_cast<float>(candidate.sites.size());
    }

    // Полный проход по .text: собирает всех кандидатов, лучшие — первыми.
    //
    // Один проход по байтам ищет только «якоря» — дальние jcc вперёд на
    // 0x80..0x2000 байт (их немного), и лишь вокруг них смотрит назад. Поэтому
    // это не O(размер * 0x400), а O(размер) + немного.
    inline std::vector<Candidate> scan(const uint8_t* code, size_t size, uintptr_t base,
                                       size_t maxResults = 8)
    {
        std::vector<Candidate> out;
        if (code == nullptr || size < 0x40) return out;

        // Проход 1: якоря. Дальний условный переход вперёд на 0x80..0x2000 байт.
        // Таких в .text немного, поэтому память под них не проблема.
        struct Anchor { size_t offset; };
        std::vector<Anchor> anchors;
        anchors.reserve(1024);

        // memchr вместо побайтового прохода: .text у Bedrock — сотни мегабайт,
        // а 0x0F встречается нечасто, так что прыжки по найденным байтам в разы
        // быстрее честного цикла.
        const uint8_t* cursor = code;
        const uint8_t* limit  = code + size - 6;
        while (cursor < limit)
        {
            cursor = static_cast<const uint8_t*>(
                std::memchr(cursor, 0x0F, static_cast<size_t>(limit - cursor)));
            if (cursor == nullptr) break;

            const uint8_t cc = cursor[1];
            ++cursor;
            if (cc < 0x80 || cc > 0x8F) continue;

            const size_t i = static_cast<size_t>(cursor - code) - 1;
            const int32_t rel = readRel32(code + i);
            if (rel <= static_cast<int32_t>(0x80)) continue;
            if (rel > static_cast<int32_t>(kMaxCritGap)) continue;

            anchors.push_back({ i });
        }

        // Проход 2: вокруг каждого якоря смотрим назад. Сосед — любой переход,
        // который ведёт ровно на байт за якорем.
        for (const Anchor& anchor : anchors)
        {
            const size_t  i     = anchor.offset;
            const uintptr_t at  = base + i;
            const uintptr_t fall = at + 6;

            Candidate candidate;
            candidate.anchor     = at;
            candidate.abortBlock = fall;
            candidate.critBlock  = jccTarget(at, code + i);

            const size_t lo = (i > kMaxBack) ? i - kMaxBack : 0;
            for (size_t j = lo; j < i; ++j)
            {
                const uintptr_t site = base + j;
                uint8_t length = 0;

                if (isShortJcc(code + j) && j + 2 <= size)      length = 2;
                else if (isNearJcc(code + j) && j + 6 <= size)  length = 6;
                else continue;

                if (jccTarget(site, code + j) != fall) continue;
                candidate.sites.push_back({ site, length, fall, false });
            }

            if (candidate.sites.size() < kScanMinSites) continue;

            // Перекрытия: short jcc, случайно попавший внутрь операнда near jcc,
            // дал бы «лишнего соседа». Такой набор отбрасываем.
            bool overlap = false;
            for (size_t k = 1; k < candidate.sites.size(); ++k)
            {
                if (candidate.sites[k].address <
                    candidate.sites[k - 1].address + candidate.sites[k - 1].length)
                {
                    overlap = true;
                    break;
                }
            }
            if (overlap) continue;

            out.push_back(std::move(candidate));
            if (out.size() >= 1024) break;             // предохранитель
        }

        // Лучшие — первыми: больше переходов, потом больше похоже на эталон.
        std::sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b)
        {
            if (a.sites.size() != b.sites.size()) return a.sites.size() > b.sites.size();
            return similarity(a) > similarity(b);
        });

        if (out.size() > maxResults) out.resize(maxResults);
        return out;
    }

    inline bool makePlan(const Candidate& candidate, Plan& plan)
    {
        plan = {};
        if (candidate.anchor == 0 || candidate.critBlock == 0) return false;
        if (candidate.sites.size() + 1 < 6) return false;

        plan.sites      = candidate.sites;
        plan.sites.push_back({ candidate.anchor, 6, candidate.critBlock, true });
        plan.abortBlock = candidate.abortBlock;
        plan.critBlock  = candidate.critBlock;
        return plan.valid();
    }

    // Первый кандидат с достаточным числом переходов — то, что можно применять.
    inline bool find(const uint8_t* code, size_t size, uintptr_t base, Plan& plan, std::string& why)
    {
        plan = {};
        why.clear();

        const auto candidates = scan(code, size, base, 16);
        for (const Candidate& candidate : candidates)
        {
            if (candidate.sites.size() < kMinSites) continue;
            if (makePlan(candidate, plan)) return true;
        }

        why = candidates.empty()
            ? "нет цепочки переходов с общей целью"
            : "цепочки есть, но ни в одной не хватает проверок (" +
              std::to_string(candidates.front().sites.size()) + ", нужно " +
              std::to_string(kMinSites) + ")";
        return false;
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
