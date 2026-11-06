//
// Created by vastrakai on 7/7/2024.
//
// Раньше стек в crash.log был не тем: StackWalk64 получал регистры самого
// обработчика краша (RtlCaptureContext), а не упавшего потока, и символы
// грузились до SymInitialize. Теперь — регистры из EXCEPTION_POINTERS, и у
// каждого кадра модуль+смещение, функция и файл:строка (если есть PDB).
//

#include "StackWalker.hpp"

#include <DbgHelp.h>
#include <spdlog/spdlog.h>

// Именованное пространство, не анонимное: при unity-сборке соседние .cpp
// склеиваются, одноимённые функции из анонимных пространств конфликтуют.
namespace StackWalkerImpl {
HMODULE moduleOf(uint64_t address)
{
    HMODULE mod = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(static_cast<uintptr_t>(address)), &mod))
        return nullptr;
    return mod;
}

std::string fileNameOnly(const char* path)
{
    if (!path) return {};
    const char* name = path;
    for (const char* p = path; *p; ++p)
        if (*p == '\\' || *p == '/') name = p + 1;
    return name;
}

std::string moduleFileName(HMODULE mod)
{
    char path[MAX_PATH] = {};
    if (!mod || !GetModuleFileNameA(mod, path, MAX_PATH)) return "?";
    return fileNameOnly(path);
}

// Наш собственный модуль (Solstice.dll / как бы ни назывался файл).
HMODULE selfModule()
{
    static const char marker = 0;
    return moduleOf(reinterpret_cast<uint64_t>(&marker));
}

bool hasPdb(HANDLE proc, uint64_t address)
{
    IMAGEHLP_MODULE64 mi{};
    mi.SizeOfStruct = sizeof(mi);
    if (!SymGetModuleInfo64(proc, address, &mi)) return false;
    return mi.SymType == SymPdb || mi.SymType == SymDia;
}
} // namespace StackWalkerImpl

static bool sOwnsDbgHelp = false; // SymInitialize сделали мы (значит, и SymCleanup)

StackWalker::StackWalker()
{
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS |
                  SYMOPT_NO_PROMPTS);

    // PDB ищем рядом с DLL; путь, записанный в самой DLL при сборке, dbghelp
    // пробует и сам.
    std::string searchPath;
    char dll[MAX_PATH] = {};
    if (GetModuleFileNameA(StackWalkerImpl::selfModule(), dll, MAX_PATH))
    {
        searchPath = dll;
        const auto slash = searchPath.find_last_of("\\/");
        if (slash != std::string::npos) searchPath.resize(slash);
    }

    if (SymInitialize(GetCurrentProcess(), searchPath.empty() ? nullptr : searchPath.c_str(), TRUE))
    {
        mInitialized = true;
        sOwnsDbgHelp = true;
    }
    else if (GetLastError() == ERROR_INVALID_PARAMETER)
    {
        // Кто-то в процессе уже инициализировал dbghelp — пользуемся этим.
        mInitialized = true;
        sOwnsDbgHelp = false;
        if (!searchPath.empty()) SymSetSearchPath(GetCurrentProcess(), searchPath.c_str());
        SymRefreshModuleList(GetCurrentProcess());
    }
}

StackWalker::~StackWalker()
{
    if (mInitialized && sOwnsDbgHelp) SymCleanup(GetCurrentProcess());
    sOwnsDbgHelp = false;
}

std::string StackWalker::describe(uint64_t address, bool returnAddress)
{
    const uint64_t lookup = returnAddress && address > 0 ? address - 1 : address;
    HMODULE mod = StackWalkerImpl::moduleOf(lookup);
    std::string out = mod ? fmt::format("{}+0x{:X}", StackWalkerImpl::moduleFileName(mod),
                                        address - reinterpret_cast<uint64_t>(mod))
                          : fmt::format("0x{:X}", address);
    if (!mInitialized) return out;

    HANDLE proc = GetCurrentProcess();
    alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + 512] = {};
    auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 511;
    DWORD64 disp = 0;
    if (SymFromAddr(proc, lookup, &disp, sym))
    {
        // Без PDB (сама игра, системные DLL) имя — ближайший экспорт, и это
        // может быть совсем другая функция: пишем его, только если рядом.
        if (StackWalkerImpl::hasPdb(proc, lookup))
            out += fmt::format(" {}", sym->Name);
        else if (disp < 0x400)
            out += fmt::format(" {}+0x{:X}", sym->Name, disp);
    }

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD lineDisp = 0;
    if (SymGetLineFromAddr64(proc, lookup, &lineDisp, &line) && line.FileName)
        out += fmt::format(" ({}:{})", StackWalkerImpl::fileNameOnly(line.FileName), line.LineNumber);
    return out;
}

std::vector<std::string> StackWalker::ShowCallstack(HANDLE hThread, PCONTEXT pContext, int maxFrames)
{
    std::vector<std::string> stackTrace;
    if (!pContext) return stackTrace;

    CONTEXT context = *pContext; // StackWalk64 меняет регистры по ходу
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rsp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    uint64_t lastPc = 0, lastSp = 0;
    for (int i = 0; i < maxFrames; i++)
    {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), hThread, &frame, &context, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        const uint64_t pc = frame.AddrPC.Offset;
        if (pc == 0) break;
        if (i > 0 && pc == lastPc && frame.AddrStack.Offset == lastSp) break; // зациклился
        lastPc = pc;
        lastSp = frame.AddrStack.Offset;
        stackTrace.push_back(fmt::format("#{:<2} {}", i, describe(pc, i > 0)));
    }

    if (stackTrace.empty())
        stackTrace.push_back(fmt::format("#0  {} (stack could not be walked)", describe(pContext->Rip)));
    return stackTrace;
}

std::string StackWalker::symbolStatus()
{
    HMODULE self = StackWalkerImpl::selfModule();
    const std::string name = StackWalkerImpl::moduleFileName(self);
    if (!mInitialized) return name + ": dbghelp is not available";

    HANDLE proc = GetCurrentProcess();
    const auto base = reinterpret_cast<uint64_t>(self);
    IMAGEHLP_MODULE64 mi{};
    mi.SizeOfStruct = sizeof(mi);
    if (!SymGetModuleInfo64(proc, base, &mi)) return name + ": no module info";
    if (mi.SymType == SymDeferred)
    {
        // Отложенная загрузка: первый запрос символа грузит PDB.
        alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + 256] = {};
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 disp = 0;
        SymFromAddr(proc, base + 0x1000, &disp, sym);
        mi = {};
        mi.SizeOfStruct = sizeof(mi);
        SymGetModuleInfo64(proc, base, &mi);
    }
    if (mi.SymType == SymPdb || mi.SymType == SymDia)
        return fmt::format("{}: PDB loaded ({})", name, mi.LoadedPdbName);
    return fmt::format("{}: PDB NOT loaded (expected at {}) - only module+offset, no function names/lines", name,
                       mi.CVData[0] ? mi.CVData : "?");
}
