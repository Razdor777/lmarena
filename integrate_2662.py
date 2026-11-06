# -*- coding: utf-8 -*-
# Финальная интеграция Dexko 26.62: регистрации, Dexko.cpp, сигнатуры, ребрендинг, аудит
import re, pathlib, subprocess

BASE = pathlib.Path('/home/user/lmarena/26.62')
ROOT = pathlib.Path('/home/user/lmarena')

def rd(p): return p.read_text(encoding='utf-8', errors='replace')
def wr(p, t): p.write_text(t, encoding='utf-8')

# ============ 1. Волна-2: конфликты -> сторона юзера (его фичи) ============
PAT = re.compile(r'<<<<<<< /tmp/mine\n(.*?)\|\|\|\|\|\|\| /tmp/older\n(.*?)=======\n(.*?)>>>>>>> /tmp/yours\n?', re.S)
def norm(t): return t if (t.endswith('\n') or t == '') else t + '\n'
for f in ['src/Features/Modules/Visual/ClickGui.cpp','src/Features/Modules/Visual/ClickGui.hpp',
          'src/Features/Modules/Visual/CustomChat.cpp','src/Features/Modules/Player/Freecam.cpp',
          'src/Features/Modules/Player/Freecam.hpp','src/Features/Modules/Visual/Glint.cpp',
          'src/Features/Modules/Visual/Glint.hpp','src/Features/Modules/Visual/Nametags.cpp',
          'src/Features/Modules/Visual/Nametags.hpp','src/Features/Modules/Visual/RobloxCamera.cpp',
          'src/Features/Modules/Visual/TargetHUD.cpp']:
    p = BASE/f; t = rd(p)
    new, n = PAT.subn(lambda m: m.group(1), t)
    wr(p, new); print(f"волна2: {pathlib.Path(f).name} — {n} конфликтов -> сторона юзера")

# ============ 2. ModuleManager: версия юзера + 5 новых модулей базы ============
mm = rd(ROOT/'src/Features/Modules/ModuleManager.cpp')
inc_anchor = re.compile(r'(^#include "Visual/UpdateForm\.hpp".*\n)', re.M)
assert inc_anchor.search(mm)
mm = inc_anchor.sub(r'\1// --- Dexko: новые модули базы 26.62 ---\n'
                    '#include "Player/AutoEat.hpp"\n#include "Movement/AutoWalk.hpp"\n'
                    '#include "Visual/PlaceHighlights.hpp"\n#include "Player/Surround.hpp"\n'
                    '#include "Player/ZipLine.hpp"\n', mm, count=1)
emp_anchor = re.compile(r'(^\s*mModules\.emplace_back\(std::make_shared<AutoPath>\(\)\);.*\n)', re.M)
assert emp_anchor.search(mm)
mm = emp_anchor.sub(r'\1  // --- Dexko: новые модули базы 26.62 ---\n'
                    '  mModules.emplace_back(std::make_shared<AutoEat>());\n'
                    '  mModules.emplace_back(std::make_shared<AutoWalk>());\n'
                    '  mModules.emplace_back(std::make_shared<PlaceHighlights>());\n'
                    '  mModules.emplace_back(std::make_shared<Surround>());\n'
                    '  mModules.emplace_back(std::make_shared<ZipLine>());\n', mm, count=1)
wr(BASE/'src/Features/Modules/ModuleManager.cpp', mm)
print("ModuleManager.cpp: версия юзера + 5 новых модулей базы")

# ============ 3. CommandManager: база + команды юзера ============
cm = rd(BASE/'src/Features/Command/CommandManager.cpp')
inc_a = re.compile(r'(^#include "Commands/TeleportCommand\.hpp".*\n)', re.M)
assert inc_a.search(cm)
cm = inc_a.sub(r'\1// --- Dexko: свои команды ---\n'
               '#include "Commands/SpammerCommand.hpp"\n#include "Commands/FakeChatCommand.hpp"\n'
               '#include "Commands/Pos1Command.hpp"\n#include "Commands/Pos2Command.hpp"\n'
               '#include "Commands/PlayerOffsetCommand.hpp"\n#include "Commands/SPos1Command.hpp"\n'
               '#include "Commands/SPos2Command.hpp"\n#include "Commands/SCopyCommand.hpp"\n'
               '#include "Commands/SPasteCommand.hpp"\n#include "Commands/SBuildCommand.hpp"\n'
               '#include "Commands/SCancelCommand.hpp"\n', cm, count=1)
adds = list(re.finditer(r'^[ \t]*ADD_COMMAND\(\w+\);[^\n]*\n', cm, re.M))
assert adds
last = adds[-1]
cm = cm[:last.end()] + ('        // --- Dexko: свои команды ---\n'
    '        ADD_COMMAND(SpammerCommand);\n        ADD_COMMAND(FakeChatCommand);\n'
    '        ADD_COMMAND(Pos1Command);\n        ADD_COMMAND(Pos2Command);\n'
    '        ADD_COMMAND(PlayerOffsetCommand);\n        ADD_COMMAND(SPos1Command);\n'
    '        ADD_COMMAND(SPos2Command);\n        ADD_COMMAND(SCopyCommand);\n'
    '        ADD_COMMAND(SPasteCommand);\n        ADD_COMMAND(SBuildCommand);\n'
    '        ADD_COMMAND(SCancelCommand);\n') + cm[last.end():]
wr(BASE/'src/Features/Command/CommandManager.cpp', cm)
print("CommandManager.cpp: база + 11 команд юзера")

# ============ 4. HookManager: база + хуки юзера ============
hm = rd(BASE/'src/Hook/HookManager.cpp')
inc_h = re.compile(r'(^#include "Hooks/ClientInstanceHooks/GrabMouseHook\.hpp".*\n)', re.M)
assert inc_h.search(hm)
hm = inc_h.sub(r'\1// --- Dexko: свои хуки ---\n'
               '#include "Hooks/ActorHooks/ProjectileHooks.hpp"\n'
               '#include "Hooks/RenderHooks/AmbienceHook.hpp"\n'
               '#include "Hooks/RenderHooks/HurtColorHook.hpp"\n', hm, count=1)
hk_a = re.compile(r'(^\s*ADD_HOOK\(LevelRendererHook\);.*\n)', re.M)
assert hk_a.search(hm)
hm = hk_a.sub(r'\1\t\t// --- Dexko: свои хуки ---\n'
              '\t\tADD_HOOK(ProjectileHooks);\n\t\tADD_HOOK(AmbienceHook);\n'
              '\t\tADD_HOOK(HurtColorHook);\n', hm, count=1)
wr(BASE/'src/Hook/HookManager.cpp', hm)
print("HookManager.cpp: база + 3 хука юзера")

# ============ 5. Dexko.hpp: mUnloading + unloadThreadProc ============
dh = rd(BASE/'src/Dexko.hpp')
dh = dh.replace('#include "spdlog/logger.h"', '#include "spdlog/logger.h"\n#include <atomic>')
dh = dh.replace('    static inline std::thread mThread;',
    '''    static inline std::thread mThread;
    // Set the moment an eject starts. The render hook checks it first and hands
    // the frame straight back to the game, so no thread can still be executing
    // our code while the module is being unloaded.
    static inline std::atomic<bool> mUnloading = false;''')
dh = dh.replace('    static void shutdownThread();',
    '    static void shutdownThread();\n    static DWORD WINAPI unloadThreadProc(LPVOID module);')
wr(BASE/'src/Dexko.hpp', dh)
print("Dexko.hpp: + mUnloading, + unloadThreadProc")

# ============ 6. Dexko.cpp: авторизация, DeviceSpoof, latestHash, eject ============
dc = rd(BASE/'src/Dexko.cpp')
dc = dc.replace('#include <Features/Configs/ConfigManager.hpp>',
                '#include <Features/Configs/ConfigManager.hpp>\n#include <Features/Modules/Misc/DeviceSpoof.hpp>')
# авторизация (как в корне, #ifdef __PRIVATE_BUILD__)
auth_block = '''
#ifdef __PRIVATE_BUILD__
#ifndef __PRIVATE_DEBUG__
#pragma message("Note: This is a private build. Please do not share this build with anyone.")
    Auth auth;
    auth.init();
    if(!auth.isPrivateUser())
    {
        auth.exit();
    }
#endif
#endif

'''
i = dc.find('/* // fard')
assert i > 0, "якорь /* // fard не найден"
# вставляем перед строкой, содержащей якорь
ls = dc.rfind('\n', 0, i) + 1
dc = dc[:ls] + auth_block + dc[ls:]
# latestHash: реальная проверка обновлений (как в корне)
assert 'latestHash = "Invalid";' in dc
dc = dc.replace('latestHash = "Invalid";', 'latestHash = OAuthUtils::getLatestCommitHash();')
# DeviceSpoof обязателен (как в корне)
spoof_block = '''        // Device spoofing is mandatory for every injected session, even if
        // the loaded default config previously had it disabled.
        auto deviceSpoof = gFeatureManager->mModuleManager->getModule<DeviceSpoof>();
        if (deviceSpoof) deviceSpoof->setEnabled(true);

'''
anchor = '        patchInHandSlot(ClientInstance::get()->getLocalPlayer() != nullptr);'
assert anchor in dc
dc = dc.replace(anchor, spoof_block + anchor, 1)
# eject: версия юзера (безопасная выгрузка через отдельный поток)
i = dc.find('//setTitle("");')
assert i > 0
j = dc.rfind('mRequestEject = true;', 0, i)
j = dc.rfind('\n', 0, j) + 1
eject = '''    mRequestEject = true;
    mUnloading.store(true);

    setTitle("");

    patchInHandSlot(false);

    HookManager::shutdown();

    // The hooks are gone, but the render thread may still be inside the frame it
    // started before that. Give it a few frames to get out before anything is
    // freed.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    gFeatureManager->shutdown();

    // Shutdown
    console->warn("Shutting down...");

    // Both of these used to be raw dereferences inside the eject path — pulling
    // the module while the game was between worlds (GuiData gone) crashed right
    // here, before anything was even unloaded.
    if (auto* ci = ClientInstance::get())
    {
        if (auto* game = ci->getMinecraftGame())
            game->playUi("beacon.deactivate", 1, 1.0f);

        if (auto* guiData = ci->getGuiData())
            guiData->displayClientMessage("§asolstice§7 » §cEjected!");
    }

    mInitialized = false;
    SigManager::deinitialize();
    OffsetProvider::deinitialize();

    // wait for threads to finish
    Sleep(1000); // Give the user time to read the message

    Logger::deinitialize();

    // ---- unload ---------------------------------------------------------
    //
    // This used to be a plain `FreeLibrary(mModule)` right here, and that is the
    // uninject crash: the DLL gets unmapped while *this* thread is still
    // executing code inside it, so the very next instruction (the function
    // epilogue, the std::thread teardown, the CRT) jumps into freed memory.
    //
    // A dedicated thread owns the unload instead. It waits until this thread is
    // definitely gone, then calls FreeLibraryAndExitThread, which frees the
    // module and terminates the thread inside kernel32 without ever returning
    // into unmapped code. This thread ends with ExitThread for the same reason.
    HANDLE unloader = CreateThread(nullptr, 0, &Solstice::unloadThreadProc, mModule, 0, nullptr);
    if (unloader)
    {
        CloseHandle(unloader);
        ExitThread(0);
    }

    // Thread creation failed: unload from here, but still never return.
    FreeLibraryAndExitThread(mModule, 0);
}

DWORD WINAPI Solstice::unloadThreadProc(LPVOID module)
{
    // Give the ejecting thread time to actually terminate (it calls ExitThread,
    // so nothing of ours is left running by then).
    Sleep(250);

    FreeLibraryAndExitThread(static_cast<HMODULE>(module), 0);
    return 0;
}
'''
dc = dc[:j] + eject
wr(BASE/'src/Dexko.cpp', dc)
print("Dexko.cpp: + auth, + DeviceSpoof, + OAuthUtils, eject пере-работан")

# ============ 7. SigManager.hpp: кастомные сигнатуры юзера ============
root_sigs = rd(ROOT/'src/SDK/SigManager.hpp')
new_sigs = rd(BASE/'src/SDK/SigManager.hpp')
root_names = set(re.findall(r'DEFINE_SIG\((\w+),', root_sigs))
new_names = set(re.findall(r'DEFINE_SIG\((\w+),', new_sigs))
custom = sorted(root_names - new_names)
lines = []
for name in custom:
    m = re.search(r'^[ \t]*//?[ \t]*DEFINE_SIG\(' + name + r',.*$', root_sigs, re.M)
    lines.append(m.group(0) if m else f'\t// TODO: {name} — строка не найдена в старом SigManager.hpp')
sig_block = ('\n\t// ================================================================\n'
    '\t// DEXKO: кастомные сигнатуры (перенесены с 1.21.44).\n'
    '\t// TODO(26.62): паттерны сняты со старой версии игры — часть из них НЕ\n'
    '\t// найдётся на 26.62. Ненайденные надо переснять с нового\n'
    '\t// Minecraft.Windows.exe (заметки: docs/RE_notes_supervazhno.txt).\n'
    '\t// Пока сигнатура == 0, зависимые модули не включать!\n'
    '\t// ================================================================\n'
    + '\n'.join(lines) + '\n\n')
anchor = '\tstatic void initialize();'
assert anchor in new_sigs
new_sigs = new_sigs.replace(anchor, sig_block + anchor, 1)
wr(BASE/'src/SDK/SigManager.hpp', new_sigs)
print(f"SigManager.hpp: + {len(custom)} кастомных сигнатур юзера")

# ============ 8. OffsetProvider.hpp: кастомные оффсеты юзера ============
root_off = rd(ROOT/'src/SDK/OffsetProvider.hpp')
new_off = rd(BASE/'src/SDK/OffsetProvider.hpp')
root_on = set(re.findall(r'DEFINE_(?:INDEX_FIELD_TYPED|INDEX_FIELD|FIELD)\(\s*(\w+)', root_off))
new_on = set(re.findall(r'DEFINE_(?:INDEX_FIELD_TYPED|INDEX_FIELD|FIELD)\(\s*(\w+)', new_off))
custom_o = sorted(root_on - new_on)
lines = []
for name in custom_o:
    m = re.search(r'^[ \t]*//?[ \t]*DEFINE_(?:INDEX_FIELD_TYPED|INDEX_FIELD|FIELD)\(\s*' + name + r'\s*[,)].*$', root_off, re.M)
    lines.append(m.group(0) if m else f'\t// TODO: {name} — строка не найдена в старом OffsetProvider.hpp')
off_block = ('\n\t// ================================================================\n'
    '\t// DEXKO: кастомные оффсеты (перенесены с 1.21.44).\n'
    '\t// TODO(26.62): сняты со старой версии — перепроверить на 26.62!\n'
    '\t// ================================================================\n'
    + '\n'.join(lines) + '\n\n')
anchor = 'static void initialize();'
assert anchor in new_off
new_off = new_off.replace(anchor, off_block + anchor, 1)
wr(BASE/'src/SDK/OffsetProvider.hpp', new_off)
print(f"OffsetProvider.hpp: + {len(custom_o)} кастомных оффсетов юзера")

# ============ 9. CMake: __DEBUG__ всегда (как в корне, чтобы кастомные сиги не роняли клиент) ============
cmk = rd(BASE/'CMakeLists.txt')
anchor = 'include_directories(.)'
assert anchor in cmk
cmk = cmk.replace(anchor, ('# Dexko (как в старом проекте): __DEBUG__ определён всегда, чтобы ненайденные\n'
    '# кастомные сигнатуры не убивали клиент через __fastfail (SigManager::initialize).\n'
    'add_compile_definitions(__DEBUG__)\n\n' + anchor), 1)
wr(BASE/'CMakeLists.txt', cmk)
print("CMakeLists.txt: __DEBUG__ безусловно (как в корне)")

print("\n=== интеграция завершена ===")
