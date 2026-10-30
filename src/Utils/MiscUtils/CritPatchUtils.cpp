//
// CritPatch — запись и откат патча. См. CritPatchUtils.hpp.
//
// Почему с остановкой потоков. Мы правим инструкции в .text живой игры, а
// функцию удара может исполнять не только поток игры. Если другой поток в этот
// момент стоит ровно на байтах, которые мы перезаписываем, он выполнит
// половинку новой инструкции — и это падение. Поэтому: сначала готовим все
// байты, потом замораживаем остальные потоки, проверяем, что ни один не стоит
// рядом с местом записи, и только тогда пишем. Пока потоки заморожены, никаких
// выделений памяти не делаем (иначе мёртвая блокировка на куче).
//

#include "CritPatchUtils.hpp"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include <Utils/MemUtils.hpp>

namespace
{
    struct Applied
    {
        uintptr_t            address = 0;
        void*                original = nullptr;
        uint8_t              length  = 0;
    };

    std::vector<Applied>& appliedList()
    {
        static std::vector<Applied> list;
        return list;
    }

    // Остальные потоки процесса, остановленные на время записи.
    class ThreadFreeze
    {
    public:
        ThreadFreeze()
        {
            const DWORD processId = GetCurrentProcessId();
            const DWORD selfId    = GetCurrentThreadId();

            const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot == INVALID_HANDLE_VALUE) return;

            THREADENTRY32 entry{};
            entry.dwSize = sizeof(entry);

            if (Thread32First(snapshot, &entry) != FALSE)
            {
                do
                {
                    if (entry.th32OwnerProcessID != processId) continue;
                    if (entry.th32ThreadID == selfId) continue;

                    HANDLE thread = OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                        FALSE, entry.th32ThreadID);

                    if (thread != nullptr) mThreads.push_back(thread);
                } while (Thread32Next(snapshot, &entry) != FALSE);
            }

            CloseHandle(snapshot);

            // Как только начинаем останавливать — валидность зависит от каждого шага.
            mValid = true;
            for (HANDLE thread : mThreads)
            {
                if (SuspendThread(thread) == static_cast<DWORD>(-1))
                {
                    mValid = false;
                    return;
                }
                ++mSuspended;
            }
        }

        ~ThreadFreeze()
        {
            resume();
            for (HANDLE thread : mThreads) CloseHandle(thread);
        }

        ThreadFreeze(const ThreadFreeze&)            = delete;
        ThreadFreeze& operator=(const ThreadFreeze&) = delete;

        [[nodiscard]] bool valid() const { return mValid; }

        // Истина, если ни один поток не стоит внутри [begin, end).
        [[nodiscard]] bool outside(uintptr_t begin, uintptr_t end) const
        {
            for (size_t index = 0; index < mSuspended; ++index)
            {
                CONTEXT context{};
                context.ContextFlags = CONTEXT_CONTROL;

                if (GetThreadContext(mThreads[index], &context) == FALSE) return false;
                if (context.Rip >= begin && context.Rip < end) return false;
            }
            return true;
        }

    private:
        void resume()
        {
            while (mSuspended != 0) ResumeThread(mThreads[--mSuspended]);
        }

        std::vector<HANDLE> mThreads;
        size_t              mSuspended = 0;
        bool                mValid     = false;
    };

    // Пишет набор байтов, останавливая остальные потоки. Возвращает false, если
    // рядом с местом записи кто-то стоит (тогда лучше ничего не трогать).
    bool writeSafely(const std::vector<std::pair<uintptr_t, std::vector<uint8_t>>>& writes)
    {
        if (writes.empty()) return true;

        uintptr_t lo = writes.front().first;
        uintptr_t hi = lo + writes.front().second.size();
        for (const auto& [address, bytes] : writes)
        {
            lo = (address < lo) ? address : lo;
            hi = (address + bytes.size() > hi) ? address + bytes.size() : hi;
        }

        // Функция большая, поэтому берём с запасом: нас интересует «стоит ли
        // поток внутри этих байтов», а не вплотную к ним.
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            ThreadFreeze freeze;
            if (!freeze.valid()) return false;

            if (!freeze.outside(lo, hi))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;   // freeze размораживается в деструкторе
            }

            for (const auto& [address, bytes] : writes)
                MemUtils::writeBytes(address, bytes.data(), bytes.size());

            FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
            return true;
        }

        return false;
    }
}

bool critpatch::isApplied()
{
    return !appliedList().empty();
}

bool critpatch::apply(const Plan& plan)
{
    if (!plan.valid()) return false;
    if (isApplied()) return true;       // уже применён — второй раз не пишем

    // 1) Готовим всё, что требует памяти, ДО остановки потоков.
    std::vector<std::pair<uintptr_t, std::vector<uint8_t>>> writes;
    std::vector<Applied> backups;
    writes.reserve(plan.sites.size());
    backups.reserve(plan.sites.size());

    for (const Site& site : plan.sites)
    {
        bool ok = false;
        auto patched = patchedBytes(site, ok);
        if (!ok || patched.size() != site.length) return false;

        auto original = MemUtils::readBytes(site.address, site.length);
        if (original.size() != site.length) return false;

        auto* copy = static_cast<void*>(std::malloc(site.length));
        if (copy == nullptr) return false;
        std::memcpy(copy, original.data(), site.length);

        writes.push_back({ site.address, std::move(patched) });
        backups.push_back({ site.address, copy, site.length });
    }

    // 2) Запись.
    if (!writeSafely(writes))
    {
        for (const Applied& backup : backups) std::free(backup.original);
        return false;
    }

    // 3) Запоминаем, что откатывать.
    for (const Applied& backup : backups) appliedList().push_back(backup);
    return true;
}

void critpatch::restore()
{
    auto& list = appliedList();
    if (list.empty()) return;

    std::vector<std::pair<uintptr_t, std::vector<uint8_t>>> writes;
    writes.reserve(list.size());

    for (const Applied& entry : list)
    {
        const auto* bytes = static_cast<const uint8_t*>(entry.original);
        writes.push_back({ entry.address, std::vector<uint8_t>(bytes, bytes + entry.length) });
    }

    // В обратном порядке: если сайты когда-нибудь начнут пересекаться, откат
    // вернёт именно то, что было до патча.
    std::reverse(writes.begin(), writes.end());

    // Если рядом с местом записи кто-то стоит — не рискуем, патч остаётся.
    if (!writeSafely(writes))
    {
        spdlog::error("[Criticals] не удалось снять нативный патч: поток стоит в месте записи");
        return;
    }

    for (const Applied& entry : list) std::free(entry.original);
    list.clear();
}
