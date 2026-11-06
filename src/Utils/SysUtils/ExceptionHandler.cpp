//
// Created by vastrakai on 7/7/2024.
//
// Краш-репорт. Что было не так раньше:
//   - стек в crash.log был стеком самого обработчика краша, а не упавшего
//     потока (StackWalk64 получал регистры из RtlCaptureContext);
//   - не было ни адреса, по которому обращались, ни того, какой модуль в этот
//     момент работал, ни последних строк лога;
//   - всё делалось прямо в упавшем потоке: при переполнении стека или краше
//     под чужой блокировкой отчёт сам падал/зависал.
// Теперь упавший поток только копирует регистры и «хлебные крошки» (какой
// обработчик события какого модуля сейчас выполняется — их ставит
// nes::dispatcher::trigger) и ждёт, а отчёт пишет отдельный поток со свежим
// стеком: Juzdex\crash.log (дописывается) + Juzdex\Crashes\crash_*.dmp.
//
// Чего UnhandledExceptionFilter не ловит вообще: повреждение кучи
// (0xC0000374) и прочие fast-fail — Windows убивает процесс сразу. Такие
// краши видны в «Просмотре событий» Windows (Журналы Windows -> Приложение,
// «Ошибка приложения»): там модуль и код исключения.
//

#include "ExceptionHandler.hpp"

#include <build_info.h>
#include <Windows.h>
#include <DbgHelp.h>

#include <Solstice.hpp>
#include <Features/FeatureManager.hpp>
#include <Utils/FileUtils.hpp>
#include <Utils/ProcUtils.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "StackWalker.hpp"

// Именованное пространство, не анонимное: при unity-сборке соседние .cpp
// склеиваются в один, и одноимённые функции из анонимных пространств
// конфликтуют.
namespace CrashReportImpl {

constexpr int kMaxCrumbs = nes::detail::max_trigger_crumbs;
constexpr DWORD kFatalCode = 0xE0000BAD; // не исключение процессора: abort, purecall...
constexpr DWORD kReportTimeoutMs = 20000; // дольше отчёт не ждём (вдруг завис)

// Всё про краш — статически: в упавшем потоке ничего не выделяем.
struct CrashState {
    char kind[128];
    char extra[1024];
    bool hasRecord;          // настоящее исключение (а не abort/purecall)
    EXCEPTION_RECORD record;
    CONTEXT context;         // регистры упавшего потока
    EXCEPTION_POINTERS pointers;
    bool hasPointers;
    DWORD threadId;
    HANDLE thread;           // настоящий хэндл упавшего потока
    unsigned roles;
    nes::detail::trigger_crumb crumbs[kMaxCrumbs];
    int crumbDepth;
    bool showBox;
    wchar_t summary[2048];   // текст окна с сообщением
};

CrashState gCrash{};
std::atomic<bool> gReporting{false};
HANDLE gReportDone = nullptr;
bool gInstalled = false;
uint64_t gStartedAt = 0;
std::string gGameVersion = "?";

LPTOP_LEVEL_EXCEPTION_FILTER gPrevFilter = nullptr;
_purecall_handler gPrevPurecall = nullptr;
_invalid_parameter_handler gPrevInvalidParam = nullptr;
void(__cdecl* gPrevAbort)(int) = SIG_DFL;

thread_local unsigned tRoles = 0;

// Последние строки лога — в памяти, на диск только при краше.
class RingSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit RingSink(size_t lines) : mLines(lines) {}

    std::vector<std::string> snapshot()
    {
        // Упавший поток мог упасть посреди записи в лог (держит мьютекс) —
        // тогда без последних строк, но не зависаем.
        std::unique_lock<std::mutex> lock(this->mutex_, std::defer_lock);
        for (int i = 0; i < 50 && !lock.try_lock(); i++) Sleep(10);
        if (!lock.owns_lock()) return {"(the log is locked by the crashed thread)"};
        std::vector<std::string> out;
        const size_t n = mLines.size();
        const size_t count = (std::min)(mNext, n);
        for (size_t i = mNext - count; i < mNext; i++) out.push_back(mLines[i % n]);
        return out;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        std::string& slot = mLines[mNext % mLines.size()];
        slot.assign(formatted.data(), formatted.size());
        while (!slot.empty() && (slot.back() == '\n' || slot.back() == '\r')) slot.pop_back();
        mNext++;
    }
    void flush_() override {}

private:
    std::vector<std::string> mLines;
    size_t mNext = 0;
};
std::shared_ptr<RingSink> gRing;

// ---------------------------------------------------------------- helpers

void copyText(char* dst, size_t size, const char* src)
{
    if (!dst || size == 0) return;
    size_t i = 0;
    if (src)
        for (; i + 1 < size && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}

// SEH-обёртка: в функции с __try не может быть объектов с деструкторами,
// поэтому всё «опасное» (чтение чужих структур, когда память уже могла быть
// испорчена) зовётся через неё.
bool guarded(void (*fn)(void*), void* arg)
{
    __try
    {
        fn(arg);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

std::wstring widen(const std::string& s, UINT codePage)
{
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(codePage, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>((std::max)(n, 0)), L'\0');
    if (n > 0) MultiByteToWideChar(codePage, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string narrowUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>((std::max)(n, 0)), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

// Пути из FileUtils — в ANSI-кодировке (кириллица в имени пользователя), а
// crash.log — в UTF-8.
std::string acpToUtf8(const std::string& s) { return narrowUtf8(widen(s, CP_ACP)); }

// Цвета консоли (\x1b[...m) и цветовые коды Minecraft (§x) — из лога вон.
std::string plainText(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[')
        {
            i += 2;
            while (i < s.size() && !((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))) i++;
            continue;
        }
        if (static_cast<unsigned char>(s[i]) == 0xC2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0xA7)
        {
            i += 2; // "§" + символ цвета
            continue;
        }
        out += s[i];
    }
    return out;
}

std::string timestamp(const char* format)
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    char buf[64] = {};
    std::strftime(buf, sizeof(buf), format, &tm);
    return buf;
}

const char* codeName(DWORD code)
{
    switch (code)
    {
    case 0xC0000005: return "ACCESS_VIOLATION";
    case 0xC0000006: return "IN_PAGE_ERROR";
    case 0xC000001D: return "ILLEGAL_INSTRUCTION";
    case 0xC0000025: return "NONCONTINUABLE_EXCEPTION";
    case 0xC000008C: return "ARRAY_BOUNDS_EXCEEDED";
    case 0xC000008E: return "FLOAT_DIVIDE_BY_ZERO";
    case 0xC0000094: return "INTEGER_DIVIDE_BY_ZERO";
    case 0xC0000095: return "INTEGER_OVERFLOW";
    case 0xC0000096: return "PRIVILEGED_INSTRUCTION";
    case 0xC00000FD: return "STACK_OVERFLOW";
    case 0xC0000374: return "HEAP_CORRUPTION";
    case 0xC0000409: return "STACK_BUFFER_OVERRUN / FAST_FAIL";
    case 0xC0000417: return "INVALID_CRUNTIME_PARAMETER";
    case 0xC0000420: return "ASSERTION_FAILURE";
    case 0x80000002: return "DATATYPE_MISALIGNMENT";
    case 0x80000003: return "BREAKPOINT";
    case 0x80000004: return "SINGLE_STEP";
    case 0xE06D7363: return "C++ EXCEPTION";
    case kFatalCode: return "SOLSTICE FATAL";
    default: return "UNKNOWN";
    }
}

std::string roleNames(unsigned roles)
{
    std::string s;
    auto add = [&](const char* name) {
        if (!s.empty()) s += " + ";
        s += name;
    };
    if (roles & ExceptionHandler::Game) add("game thread (ticks, packets)");
    if (roles & ExceptionHandler::Render) add("render thread (Present, ImGui)");
    if (roles & ExceptionHandler::Input) add("input thread (keyboard/mouse)");
    if (roles & ExceptionHandler::Background) add("Solstice background thread");
    if (s.empty()) s = "engine thread (no Solstice hook ran on it before)";
    return s;
}

// ---- C++ исключение: тип и what() (структуры MSVC EH для x64, всё по RVA)
struct CxxThrowInfo { unsigned attributes; int pmfnUnwind; int pForwardCompat; int pCatchableTypeArray; };
struct CxxCatchableTypeArray { int nCatchableTypes; int arrayOfCatchableTypes[1]; };
struct CxxPMD { int mdisp; int pdisp; int vdisp; };
struct CxxCatchableType { unsigned properties; int pType; CxxPMD thisDisplacement; int sizeOrOffset; int copyFunction; };
struct CxxTypeDescriptor { const void* pVFTable; void* spare; char name[1]; };

void readCxxException(const EXCEPTION_RECORD& r, char* type, size_t typeLen, char* what, size_t whatLen)
{
    type[0] = 0;
    what[0] = 0;
    if (r.NumberParameters < 4) return;
    const ULONG_PTR magic = r.ExceptionInformation[0];
    if (magic != 0x19930520 && magic != 0x19930521 && magic != 0x19930522 && magic != 0x01994000) return;
    const char* object = reinterpret_cast<const char*>(r.ExceptionInformation[1]);
    const auto* info = reinterpret_cast<const CxxThrowInfo*>(r.ExceptionInformation[2]);
    const auto base = static_cast<uintptr_t>(r.ExceptionInformation[3]);
    __try
    {
        if (info && base)
        {
            const auto* arr = reinterpret_cast<const CxxCatchableTypeArray*>(base + info->pCatchableTypeArray);
            for (int i = 0; i < arr->nCatchableTypes && i < 32; i++)
            {
                const auto* ct = reinterpret_cast<const CxxCatchableType*>(base + arr->arrayOfCatchableTypes[i]);
                const auto* td = reinterpret_cast<const CxxTypeDescriptor*>(base + ct->pType);
                if (i == 0)
                {
                    // ".?AVout_of_range@std@@" -> "class std::out_of_range". 0x2800 —
                    // UNDNAME_32_BIT_DECODE | 0x2000 («только тип», как type_info::name).
                    if (!UnDecorateSymbolName(td->name + 1, type, static_cast<DWORD>(typeLen), 0x2800))
                        copyText(type, typeLen, td->name);
                }
                if (object && !what[0] && ct->thisDisplacement.pdisp < 0 &&
                    std::strcmp(td->name, ".?AVexception@std@@") == 0)
                {
                    const auto* ex = reinterpret_cast<const std::exception*>(object + ct->thisDisplacement.mdisp);
                    copyText(what, whatLen, ex->what());
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

std::string describeException(const EXCEPTION_RECORD& r)
{
    std::string s = fmt::format("0x{:08X} {}", static_cast<unsigned>(r.ExceptionCode), codeName(r.ExceptionCode));
    switch (r.ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR:
        if (r.NumberParameters >= 2)
        {
            const auto op = r.ExceptionInformation[0];
            const auto addr = static_cast<uint64_t>(r.ExceptionInformation[1]);
            const char* what = op == 0 ? "read from" : op == 1 ? "write to" : op == 8 ? "execute at" : "access to";
            s += fmt::format(" - {} 0x{:X}", what, addr);
            if (op == 8)
                s += " (jump to a bad address: dangling function pointer/vtable or unloaded code)";
            else if (addr < 0x10000)
                s += fmt::format(" (null pointer, field offset 0x{:X})", addr);
            else if (addr == ~0ull || addr >= 0x0000800000000000ull)
                s += " (garbage pointer: freed/uninitialised memory)";
        }
        break;
    case 0xE06D7363: {
        char type[256], what[512];
        readCxxException(r, type, sizeof(type), what, sizeof(what));
        s += fmt::format(" - {}", type[0] ? type : "unknown type");
        if (what[0]) s += fmt::format(": \"{}\"", what);
        break;
    }
    case 0xC0000409:
        if (r.NumberParameters >= 1)
            s += fmt::format(" (fast fail code {})", static_cast<uint64_t>(r.ExceptionInformation[0]));
        break;
    default:
        break;
    }
    return s;
}

struct NameQuery {
    const void* instance;
    std::string* out;
};

// Чей обработчик: модуль (по адресу объекта) или что-то ещё.
std::string listenerName(const void* instance)
{
    if (!instance) return "lambda/static handler";
    std::string out = fmt::format("object 0x{:X}", reinterpret_cast<uintptr_t>(instance));
    NameQuery query{instance, &out};
    guarded(
        [](void* p) {
            auto* nq = static_cast<NameQuery*>(p);
            if (!gFeatureManager) return;
            if (gFeatureManager->mModuleManager)
                for (const auto& m : gFeatureManager->mModuleManager->getModules())
                    if (static_cast<const void*>(m.get()) == nq->instance)
                    {
                        *nq->out = "module " + m->mName;
                        return;
                    }
            if (gFeatureManager->mCommandManager &&
                static_cast<const void*>(gFeatureManager->mCommandManager.get()) == nq->instance)
                *nq->out = "CommandManager";
        },
        &query);
    return out;
}

std::string enabledModules()
{
    std::string list;
    struct Ctx {
        std::string* list;
        int on, total;
    } ctx{&list, 0, 0};
    guarded(
        [](void* p) {
            auto* c = static_cast<Ctx*>(p);
            if (!gFeatureManager || !gFeatureManager->mModuleManager) return;
            for (const auto& m : gFeatureManager->mModuleManager->getModules())
            {
                c->total++;
                if (!m->mEnabled) continue;
                c->on++;
                if (!c->list->empty()) *c->list += ", ";
                *c->list += m->mName;
            }
        },
        &ctx);
    if (ctx.total == 0) return "(FeatureManager not initialized)";
    return fmt::format("{} of {}: {}", ctx.on, ctx.total, list);
}

void appendToCrashLog(const std::string& text)
{
    const std::string dir = FileUtils::getSolsticeDir();
    const std::string path = dir + "crash.log";
    std::error_code ec;
    // Не даём файлу расти бесконечно: больше 2 МБ — в crash.old.log.
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 2u * 1024u * 1024u)
    {
        std::filesystem::remove(dir + "crash.old.log", ec);
        std::filesystem::rename(path, dir + "crash.old.log", ec);
    }
    std::ofstream file(path, std::ios::app | std::ios::binary);
    if (file.is_open())
    {
        file << text;
        file.flush();
    }
}

// Минидамп для Visual Studio (Файл -> Открыть -> .dmp, «Отладка только с
// машинным кодом», PDB из папки сборки). Храним 5 последних.
std::string writeMinidump()
{
    const std::string dir = FileUtils::getSolsticeDir() + "Crashes\\";
    CreateDirectoryA(dir.c_str(), nullptr);
    try
    {
        std::vector<std::filesystem::directory_entry> old;
        for (const auto& e : std::filesystem::directory_iterator(dir))
            if (e.is_regular_file() && e.path().extension() == ".dmp") old.push_back(e);
        std::sort(old.begin(), old.end(),
                  [](const auto& a, const auto& b) { return a.last_write_time() > b.last_write_time(); });
        for (size_t i = 4; i < old.size(); i++)
        {
            std::error_code ec;
            std::filesystem::remove(old[i].path(), ec);
        }
    }
    catch (...)
    {
    }

    const std::string path = dir + "crash_" + timestamp("%Y-%m-%d_%H-%M-%S") + ".dmp";
    HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return fmt::format("(could not create the dump file, error {})", GetLastError());

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = gCrash.threadId;
    mei.ExceptionPointers = &gCrash.pointers;
    mei.ClientPointers = FALSE;
    const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory |
                                                 MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
    const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
                                      gCrash.hasPointers ? &mei : nullptr, nullptr, nullptr);
    const DWORD err = GetLastError();
    CloseHandle(file);
    if (!ok)
    {
        DeleteFileA(path.c_str());
        return fmt::format("(MiniDumpWriteDump failed, 0x{:X})", static_cast<unsigned>(err));
    }
    return acpToUtf8(path);
}

void writeReport()
{
    CrashState& c = gCrash;
    std::string text;
    text.reserve(16384);
    text += "================ Crash " + timestamp("%Y-%m-%d %H:%M:%S") + " ================\n";
    text += std::string("Kind: ") + c.kind + "\n";
    std::string exception;
    if (c.hasRecord)
    {
        exception = describeException(c.record);
        text += "Exception: " + exception + "\n";
    }
    if (c.extra[0]) text += std::string("Details: ") + c.extra + "\n";

    StackWalker walker;
    const std::vector<std::string> stack =
        walker.ShowCallstack(c.thread ? c.thread : GetCurrentThread(), &c.context);
    std::string where;
    if (c.hasRecord)
    {
        where = walker.describe(reinterpret_cast<uint64_t>(c.record.ExceptionAddress));
    }
    else
    {
        // abort/purecall: регистры сняты в самом обработчике — «где» — это
        // первый кадр стека не из обработчика и не из CRT/системы.
        static const char* const kSkip[] = {"CrashReportImpl", "ucrtbase", "vcruntime", "KERNELBASE",
                                            "ntdll",           "kernel32", "abort",     "terminate",
                                            "raise",           "purecall", "invalid_parameter"};
        for (const auto& line : stack)
        {
            bool skip = false;
            for (const char* k : kSkip)
                if (line.find(k) != std::string::npos) skip = true;
            if (!skip)
            {
                where = line.size() > 4 ? line.substr(4) : line;
                break;
            }
        }
        if (where.empty()) where = "(see the call stack)";
    }
    text += "Where: " + where + "\n";
    text += fmt::format("Thread: {} - {}\n", c.threadId, roleNames(c.roles));

    // Какой обработчик события какого модуля выполнялся (внешний -> внутренний).
    std::string handler;
    text += "Solstice event handlers running on this thread (outer -> inner):\n";
    if (c.crumbDepth <= 0) text += "  none - the crash was NOT inside a module's event handler\n";
    const int depth = (std::min)(c.crumbDepth, kMaxCrumbs);
    for (int i = 0; i < depth; i++)
    {
        const auto& crumb = c.crumbs[i];
        std::string event = crumb.event ? std::string(crumb.event, crumb.event_len) : std::string("?");
        for (const char* prefix : {"class ", "struct "})
            if (event.rfind(prefix, 0) == 0) event.erase(0, std::strlen(prefix));
        const std::string who = listenerName(crumb.instance);
        text += "  " + event + " -> " + who + "\n";
        handler = who + " (" + event + ")";
    }
    if (c.crumbDepth > kMaxCrumbs) text += fmt::format("  ... and {} more nested\n", c.crumbDepth - kMaxCrumbs);

    const CONTEXT& r = c.context;
    text += fmt::format("Registers: RIP={:016X} RSP={:016X} RBP={:016X}\n"
                        "           RAX={:016X} RBX={:016X} RCX={:016X} RDX={:016X}\n"
                        "           RSI={:016X} RDI={:016X} R8={:016X} R9={:016X}\n",
                        r.Rip, r.Rsp, r.Rbp, r.Rax, r.Rbx, r.Rcx, r.Rdx, r.Rsi, r.Rdi, r.R8, r.R9);

    text += "Call stack:\n";
    for (const auto& line : stack) text += "  " + line + "\n";
    text += "Symbols: " + acpToUtf8(walker.symbolStatus()) + "\n";
    text += "Enabled modules: " + enabledModules() + "\n";
    text += fmt::format("Uptime: {} s since injection\n", (GetTickCount64() - gStartedAt) / 1000);
    text += std::string("Build: ") + SOLSTICE_BUILD_VERSION + " (" + SOLSTICE_BUILD_BRANCH + ") \"" +
            SOLSTICE_BUILD_COMMIT_MESSAGE + "\"\n";
    text += "Minecraft: " + gGameVersion + "\n";
    text += "Last log lines:\n";
    if (gRing)
        for (const auto& line : gRing->snapshot()) text += "  " + plainText(line) + "\n";
    else
        text += "  (log capture is not installed)\n";

    // Сначала текст на диск — дамп может и не получиться.
    appendToCrashLog(text);
    const std::string dump = writeMinidump();
    appendToCrashLog("Dump: " + dump + "\n\n");

    // Окно: коротко и по-русски, подробности — в файле.
    std::string box = "Minecraft упал (Solstice).\n\n";
    box += "Причина: " + (exception.empty() ? std::string(c.kind) : exception) + "\n";
    if (c.extra[0]) box += std::string("Подробности: ") + c.extra + "\n";
    box += "Где: " + where + "\n";
    box += "Модуль: " + (handler.empty() ? std::string("— (не в обработчике модуля)") : handler) + "\n";
    box += "Поток: " + roleNames(c.roles) + "\n\n";
    box += "Отчёт дописан в файл:\n";
    std::wstring wbox = widen(box, CP_UTF8) + widen(FileUtils::getSolsticeDir() + "crash.log", CP_ACP) +
                        L"\n\nПришли последний блок из этого файла — по нему видно причину.";
    const size_t n = (std::min)(wbox.size(), sizeof(c.summary) / sizeof(wchar_t) - 1);
    wmemcpy(c.summary, wbox.data(), n);
    c.summary[n] = L'\0';
}

void writeReportGuarded()
{
    if (!guarded([](void*) { writeReport(); }, nullptr))
    {
        // Сам отчёт упал (память совсем испорчена) — хотя бы одна строка.
        char line[256];
        std::snprintf(line, sizeof(line), "================ Crash (report failed) ================\nKind: %s\n\n",
                      gCrash.kind);
        guarded([](void* text) { appendToCrashLog(static_cast<const char*>(text)); }, line);
        const wchar_t* msg = L"Minecraft упал (Solstice). Отчёт записать не удалось.";
        const size_t n = (std::min)(std::wcslen(msg), sizeof(gCrash.summary) / sizeof(wchar_t) - 1);
        wmemcpy(gCrash.summary, msg, n);
        gCrash.summary[n] = L'\0';
    }
}

DWORD WINAPI reporterThread(LPVOID)
{
    writeReportGuarded();
    if (gReportDone) SetEvent(gReportDone);
    if (gCrash.showBox && gCrash.summary[0])
        MessageBoxW(nullptr, gCrash.summary, L"Solstice — краш", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    return 0;
}

// Отчёт — в отдельном потоке со свежим стеком; упавший поток ждёт.
void runReporter(bool allowInline)
{
    if (gReportDone) ResetEvent(gReportDone);
    HANDLE thread = CreateThread(nullptr, 1u << 20, reporterThread, nullptr, 0, nullptr);
    if (thread)
    {
        if (!gReportDone || WaitForSingleObject(gReportDone, kReportTimeoutMs) == WAIT_OBJECT_0)
            WaitForSingleObject(thread, gCrash.showBox ? INFINITE : 5000); // окно ждёт нажатия
        CloseHandle(thread);
        return;
    }
    if (allowInline)
    {
        writeReportGuarded();
        if (gCrash.showBox && gCrash.summary[0])
            MessageBoxW(nullptr, gCrash.summary, L"Solstice — краш", MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
}

// Только копирование в статическую память — без выделений.
void captureThread(const char* kind, const char* extra)
{
    CrashState& c = gCrash;
    copyText(c.kind, sizeof(c.kind), kind);
    copyText(c.extra, sizeof(c.extra), extra);
    c.threadId = GetCurrentThreadId();
    c.thread = nullptr;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &c.thread, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    c.roles = tRoles;
    const auto& crumbs = nes::current_trigger_crumbs();
    c.crumbDepth = crumbs.depth;
    for (int i = 0; i < kMaxCrumbs; i++) c.crumbs[i] = crumbs.items[i];
    c.hasRecord = false;
    c.hasPointers = false;
    c.showBox = true;
    c.summary[0] = L'\0';
}

LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS ep)
{
    if (gReporting.exchange(true))
    {
        // Второй краш, пока пишется первый (другой поток или сам отчёт) —
        // ждём первый; процесс всё равно завершится.
        Sleep(kReportTimeoutMs + 5000);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    captureThread("Unhandled exception", nullptr);
    if (ep && ep->ExceptionRecord)
    {
        gCrash.record = *ep->ExceptionRecord;
        gCrash.hasRecord = true;
    }
    if (ep && ep->ContextRecord) gCrash.context = *ep->ContextRecord;
    gCrash.pointers.ExceptionRecord = &gCrash.record;
    gCrash.pointers.ContextRecord = &gCrash.context;
    gCrash.hasPointers = gCrash.hasRecord;

    // При переполнении стека в этом потоке места почти нет: только поток-отчёт.
    const bool stackOverflow = gCrash.hasRecord && gCrash.record.ExceptionCode == EXCEPTION_STACK_OVERFLOW;
    runReporter(!stackOverflow);

    // Свой обработчик краша мог быть и у игры — пусть тоже отработает.
    if (gPrevFilter) return gPrevFilter(ep);
    return EXCEPTION_CONTINUE_SEARCH;
}

// abort(), pure virtual call, неверный параметр CRT: исключения нет, регистры
// берём здесь же. Дальше процесс всё равно завершается.
[[noreturn]] void fatalHere(const char* kind, const std::string& extra)
{
    if (!gReporting.exchange(true))
    {
        captureThread(kind, extra.c_str());
        RtlCaptureContext(&gCrash.context);
        gCrash.record = EXCEPTION_RECORD{};
        gCrash.record.ExceptionCode = kFatalCode;
        gCrash.record.ExceptionAddress = reinterpret_cast<PVOID>(gCrash.context.Rip);
        gCrash.pointers.ExceptionRecord = &gCrash.record;
        gCrash.pointers.ContextRecord = &gCrash.context;
        gCrash.hasPointers = true;
        runReporter(true);
    }
    else
    {
        Sleep(kReportTimeoutMs + 5000);
    }
    TerminateProcess(GetCurrentProcess(), 3);
    for (;;) Sleep(1000);
}

std::string currentExceptionText()
{
    try
    {
        if (auto e = std::current_exception()) std::rethrow_exception(e);
    }
    catch (const std::exception& ex)
    {
        return std::string("uncaught std::exception: ") + ex.what();
    }
    catch (...)
    {
        return "uncaught non-std C++ exception";
    }
    return {};
}

void __cdecl onPurecall()
{
    fatalHere("Pure virtual function call (an object was used after it was destroyed)", "");
}

void __cdecl onInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                unsigned int line, uintptr_t)
{
    std::string extra;
    if (function) extra += narrowUtf8(function);
    if (file) extra += fmt::format(" ({}:{})", narrowUtf8(file), line);
    if (expression) extra += ": " + narrowUtf8(expression);
    fatalHere("CRT invalid parameter", extra);
}

void __cdecl onSigAbrt(int)
{
    fatalHere("abort() / std::terminate()", currentExceptionText());
}

} // namespace CrashReportImpl

// Снаружи пространства — только с явным CrashReportImpl:: (без using
// namespace: при unity-сборке он виден соседним .cpp и путает имена).
void ExceptionHandler::init()
{
    namespace C = CrashReportImpl;
    if (C::gInstalled) return;
    C::gInstalled = true;
    C::gStartedAt = GetTickCount64();
    C::gReportDone = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    try
    {
        C::gGameVersion = ProcUtils::getVersion();
    }
    catch (...)
    {
    }

    // Последние 200 строк лога — в память (в отчёт попадут при краше).
    try
    {
        C::gRing = std::make_shared<C::RingSink>(200);
        C::gRing->set_pattern("%H:%M:%S.%e [%l] %v");
        if (auto logger = spdlog::default_logger()) logger->sinks().push_back(C::gRing);
        if (Solstice::console && Solstice::console != spdlog::default_logger())
            Solstice::console->sinks().push_back(C::gRing);
    }
    catch (...)
    {
        C::gRing.reset();
    }

    C::gPrevFilter = SetUnhandledExceptionFilter(C::TopLevelExceptionHandler);
    if (C::gPrevFilter == C::TopLevelExceptionHandler) C::gPrevFilter = nullptr;
    C::gPrevPurecall = _set_purecall_handler(C::onPurecall);
    C::gPrevInvalidParam = _set_invalid_parameter_handler(C::onInvalidParameter);
    C::gPrevAbort = signal(SIGABRT, C::onSigAbrt);
}

void ExceptionHandler::shutdown()
{
    namespace C = CrashReportImpl;
    if (!C::gInstalled) return;
    C::gInstalled = false;
    SetUnhandledExceptionFilter(C::gPrevFilter);
    _set_purecall_handler(C::gPrevPurecall);
    _set_invalid_parameter_handler(C::gPrevInvalidParam);
    signal(SIGABRT, C::gPrevAbort == SIG_ERR ? SIG_DFL : C::gPrevAbort);
}

void ExceptionHandler::markThread(ThreadRole role) noexcept
{
    CrashReportImpl::tRoles |= role;
}

void ExceptionHandler::makeCrashLog(const std::string& text, DWORD exceptionCode)
{
    namespace C = CrashReportImpl;
    std::string entry = "================ " + C::timestamp("%Y-%m-%d %H:%M:%S") + " ================\n";
    entry += text + "\n";
    entry += fmt::format("Code: 0x{:X}\n", static_cast<unsigned>(exceptionCode));
    entry += "Enabled modules: " + C::enabledModules() + "\n";
    entry += std::string("Build: ") + SOLSTICE_BUILD_VERSION + " (" + SOLSTICE_BUILD_BRANCH + ")\n";
    entry += "Minecraft: " + (C::gGameVersion == "?" ? ProcUtils::getVersion() : C::gGameVersion) + "\n\n";
    C::appendToCrashLog(entry);
}
