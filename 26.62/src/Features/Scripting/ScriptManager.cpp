//
// Luau scripting runtime for Solstice. See ScriptManager.hpp for the script layout.
//

#include "ScriptManager.hpp"

#include <lua.h>
#include <lualib.h>
#include <luacode.h>

#include <Features/FeatureManager.hpp>
#include <Features/Modules/ModuleManager.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Utils/FileUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Script
// ---------------------------------------------------------------------------

Script::~Script()
{
    if (mLua)
    {
        lua_close(mLua);
        mLua = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

namespace
{
    constexpr int kNoRef = LUA_NOREF;

    std::string directoryFor(const ScriptType type)
    {
        return FileUtils::getSolsticeDir() +
               (type == ScriptType::Module ? "Scripts\\Modules\\" : "Scripts\\Commands\\");
    }

    ScriptType typeFromPath(const std::string& path)
    {
        return path.find("\\Commands\\") != std::string::npos ? ScriptType::Command : ScriptType::Module;
    }

    bool hasSuffix(const std::string& s, const char* suffix)
    {
        const size_t n = std::strlen(suffix);
        return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
    }

    // Reads the whole file; returns false when it is missing or empty.
    bool readFile(const std::string& path, std::string& out)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        std::ostringstream ss;
        ss << file.rdbuf();
        out = ss.str();
        return !out.empty();
    }

    // Every script stashes its owner under this registry key so C closures can
    // find the script they belong to (Luau has no lua_getextraspace).
    constexpr const char* kOwnerKey = "__solstice_owner";

    Script* ownerOf(lua_State* L)
    {
        lua_getfield(L, LUA_REGISTRYINDEX, kOwnerKey);
        auto* owner = static_cast<Script*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return owner;
    }

    bool callField(lua_State* L, const Script& script, int ref)
    {
        if (ref == kNoRef) return true;

        lua_getref(L, ref);
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return true;
        }

        // self = the module table
        if (script.mModuleRef != kNoRef) lua_getref(L, script.mModuleRef);
        else lua_pushnil(L);

        const int status = lua_pcall(L, 1, 0, 0);
        if (status != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            spdlog::error("[script] {}: {}", script.mName, err ? err : "unknown error");
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    int traceback(lua_State* L)
    {
        const char* msg = lua_tostring(L, 1);
        luaL_traceback(L, L, msg ? msg : "error", 1);
        return 1;
    }
}

// ---------------------------------------------------------------------------
// Luau-facing API
// ---------------------------------------------------------------------------

int ScriptManager::l_log(lua_State* L)
{
    const char* msg = luaL_optstring(L, 1, "");
    spdlog::info("[script] {}", msg);
    return 0;
}

int ScriptManager::l_getModule(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const auto* manager = gFeatureManager ? gFeatureManager->mModuleManager.get() : nullptr;
    const auto* mod = manager ? manager->getModule(name) : nullptr;
    if (!mod) lua_pushnil(L);
    else lua_pushboolean(L, mod->mEnabled);
    return 1;
}

int ScriptManager::l_isModuleEnabled(lua_State* L)
{
    return l_getModule(L);
}

int ScriptManager::l_setModuleEnabled(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const bool enabled = lua_toboolean(L, 2) != 0;
    if (gFeatureManager && gFeatureManager->mModuleManager)
    {
        if (auto* mod = gFeatureManager->mModuleManager->getModule(name))
            mod->setEnabled(enabled);
    }
    return 0;
}

int ScriptManager::l_getClientMessage(lua_State* L)
{
    const char* msg = luaL_optstring(L, 1, "");
    ChatUtils::displayClientMessage(msg);
    return 0;
}

int ScriptManager::l_registerCommand(lua_State* L)
{
    Script* script = ownerOf(L);
    if (!script)
    {
        luaL_error(L, "registerCommand called outside a script");
        return 0;
    }

    const char* name = luaL_checkstring(L, 1);
    if (!name || !*name)
    {
        luaL_error(L, "registerCommand: name must be a non-empty string");
        return 0;
    }
    luaL_checktype(L, 3, LUA_TFUNCTION);

    // Stash the handler under a per-script registry key.
    const std::string key = std::string("solstice!cmd!") + script->mName + "!" + name;
    lua_pushvalue(L, 3);
    lua_setfield(L, LUA_REGISTRYINDEX, key.c_str());

    if (std::find(script->mCommandNames.begin(), script->mCommandNames.end(), name) == script->mCommandNames.end())
        script->mCommandNames.emplace_back(name);

    return 0;
}

void ScriptManager::registerApi(lua_State* L, Script* owner)
{
    lua_pushlightuserdata(L, owner);
    lua_setfield(L, LUA_REGISTRYINDEX, kOwnerKey);

    struct Entry { const char* name; lua_CFunction fn; };
    const Entry entries[] = {
        { "log",               &ScriptManager::l_log },
        { "getModule",         &ScriptManager::l_getModule },
        { "isModuleEnabled",   &ScriptManager::l_isModuleEnabled },
        { "setModuleEnabled",  &ScriptManager::l_setModuleEnabled },
        { "sendClientMessage", &ScriptManager::l_getClientMessage },
        { "registerCommand",   &ScriptManager::l_registerCommand },
    };

    for (const auto& e : entries)
    {
        lua_pushcfunction(L, e.fn, e.name);
        lua_setglobal(L, e.name);
    }

    // Luau keeps print; route it to the client log instead of stdout.
    lua_pushcfunction(L, &ScriptManager::l_log, "print");
    lua_setglobal(L, "print");
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

void ScriptManager::extractModuleTable(Script& script)
{
    lua_State* L = script.mLua;
    if (!L || !lua_istable(L, -1)) return;

    script.mModuleRef = lua_ref(L, -1);

    auto readString = [&](const char* field) -> std::string {
        lua_getfield(L, -1, field);
        const char* value = lua_tostring(L, -1);
        std::string result = value ? value : "";
        lua_pop(L, 1);
        return result;
    };

    script.mDescription = readString("description");
    script.mAuthor = readString("author");

    lua_getfield(L, -1, "name");
    if (const char* declared = lua_tostring(L, -1); declared && *declared)
        script.mName = declared;
    lua_pop(L, 1);

    auto grabMethod = [&](const char* field) -> int {
        lua_getfield(L, -1, field);
        int ref = lua_isfunction(L, -1) ? lua_ref(L, -1) : kNoRef;
        lua_pop(L, 1);
        return ref;
    };

    script.mOnEnableRef = grabMethod("onEnable");
    script.mOnDisableRef = grabMethod("onDisable");
    script.mOnTickRef = grabMethod("onTick");
    script.mOnRenderRef = grabMethod("onRender");
}

bool ScriptManager::runChunk(Script& script, const std::string& source)
{
    lua_State* L = script.mLua;
    if (!L) return false;

    size_t bytecodeSize = 0;
    char* bytecode = luau_compile(source.data(), source.size(), nullptr, &bytecodeSize);
    if (!bytecode || bytecodeSize == 0)
    {
        spdlog::error("[script] {}: compilation failed", script.mName);
        if (bytecode) std::free(bytecode);
        return false;
    }

    const std::string chunkName = "@" + script.mPath;
    const int loaded = luau_load(L, chunkName.c_str(), bytecode, bytecodeSize, 0);
    std::free(bytecode);

    if (loaded != 0)
    {
        const char* err = lua_tostring(L, -1);
        spdlog::error("[script] {}: {}", script.mName, err ? err : "load failed");
        lua_pop(L, 1);
        return false;
    }

    lua_pushcfunction(L, &traceback, "traceback");
    const int errHandler = lua_gettop(L) - 1;

    const int status = lua_pcall(L, 0, 1, errHandler);
    if (status != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        spdlog::error("[script] {}: {}", script.mName, err ? err : "runtime error");
        lua_pop(L, 2); // error + handler
        return false;
    }

    lua_remove(L, errHandler);

    // A module chunk must return its table; a command chunk usually returns nil.
    if (script.mType == ScriptType::Module)
    {
        if (!lua_istable(L, -1))
        {
            spdlog::error("[script] {}: module script did not return a table", script.mName);
            lua_pop(L, 1);
            return false;
        }
        extractModuleTable(script);
    }
    lua_pop(L, 1);

    return true;
}

bool ScriptManager::loadScript(const std::string& path)
{
    std::string source;
    if (!readFile(path, source))
    {
        spdlog::warn("[script] cannot read {}", path);
        return false;
    }

    auto script = std::make_shared<Script>();
    script->mPath = path;
    script->mType = typeFromPath(path);

    const size_t slash = path.find_last_of("\\/");
    const std::string fileName = slash == std::string::npos ? path : path.substr(slash + 1);
    script->mName = fileName;
    for (const char* ext : { ".luau", ".lua" })
        if (hasSuffix(script->mName, ext)) script->mName.resize(script->mName.size() - std::strlen(ext));

    script->mLua = createState();
    if (!script->mLua) return false;

    registerApi(script->mLua, script.get());

    if (!runChunk(*script, source))
    {
        spdlog::error("[script] failed to load {} ({})", script->mName,
                      script->mType == ScriptType::Module ? "module" : "command");
        return false;
    }

    spdlog::info("[script] loaded {} ({})", script->mName,
                 script->mType == ScriptType::Module ? "module" : "command");

    mScripts.push_back(script);
    return true;
}

lua_State* ScriptManager::createState()
{
    lua_State* L = luaL_newstate();
    if (!L) return nullptr;
    luaL_openlibs(L);
    return L;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ScriptManager::reloadAll()
{
    std::lock_guard lock(mMutex);
    mLoadingScripts = true;

    mScripts.clear();

    for (const ScriptType type : { ScriptType::Module, ScriptType::Command })
    {
        const std::string dir = directoryFor(type);
        for (const auto& file : FileUtils::listFiles(dir))
        {
            if (!hasSuffix(file, ".luau") && !hasSuffix(file, ".lua")) continue;
            loadScript(dir + file);
        }
    }

    mLoadingScripts = false;
    spdlog::info("[script] {} scripts loaded", mScripts.size());
}

void ScriptManager::init()
{
    FileUtils::createDirectory(FileUtils::getSolsticeDir() + "Scripts\\Modules\\");
    FileUtils::createDirectory(FileUtils::getSolsticeDir() + "Scripts\\Commands\\");

    reloadAll();

    if (mScripts.empty())
        spdlog::info("[script] no scripts found in {}Scripts", FileUtils::getSolsticeDir());
}

void ScriptManager::shutdown()
{
    std::lock_guard lock(mMutex);
    for (auto& script : mScripts)
    {
        if (script->mEnabled) setScriptEnabled(script, false);
    }
    mScripts.clear();
}

bool ScriptManager::reloadScript(const std::string& name)
{
    std::lock_guard lock(mMutex);

    for (auto it = mScripts.begin(); it != mScripts.end(); ++it)
    {
        const auto& script = *it;
        const bool nameMatch = script->mName == name ||
            std::find(script->mCommandNames.begin(), script->mCommandNames.end(), name) != script->mCommandNames.end();
        if (!nameMatch) continue;

        const std::string path = script->mPath;
        if (script->mEnabled) setScriptEnabled(script, false);

        mScripts.erase(it);
        return loadScript(path);
    }
    return false;
}

std::shared_ptr<Script> ScriptManager::getScript(const std::string& name)
{
    for (const auto& script : mScripts)
    {
        if (script->mName == name) return script;
        if (std::find(script->mCommandNames.begin(), script->mCommandNames.end(), name) != script->mCommandNames.end())
            return script;
    }
    return nullptr;
}

void ScriptManager::setScriptEnabled(const std::shared_ptr<Script>& script, const bool enabled)
{
    if (!script || script->mType != ScriptType::Module) return;
    if (script->mEnabled == enabled) return;

    // Mirror the C++ module path so ToggleSounds / Notifications react identically.
    // ModuleScript is only ever used as an opaque pointer by the current
    // listeners (they read mEnabled/mWasEnabled), so handing them the Script
    // address is safe; add a real ModuleScript base if a listener ever
    // dereferences it.
    if (gFeatureManager && gFeatureManager->mDispatcher)
    {
        auto holder = nes::make_holder<ModuleScriptStateChangeEvent>(
            reinterpret_cast<ModuleScript*>(script.get()), enabled, script->mEnabled);
        gFeatureManager->mDispatcher->trigger(holder);
        if (holder->isCancelled()) return;
    }

    script->mEnabled = enabled;
    callField(script->mLua, *script, enabled ? script->mOnEnableRef : script->mOnDisableRef);
}

void ScriptManager::onClientTick()
{
    for (const auto& script : mScripts)
    {
        if (script->mEnabled && script->mType == ScriptType::Module)
            callField(script->mLua, *script, script->mOnTickRef);
    }
}

void ScriptManager::onRenderEvent()
{
    for (const auto& script : mScripts)
    {
        if (script->mEnabled && script->mType == ScriptType::Module)
            callField(script->mLua, *script, script->mOnRenderRef);
    }
}

bool ScriptManager::findAndExecCommand(const std::string& name, const std::vector<std::string>& args)
{
    for (const auto& script : mScripts)
    {
        if (std::find(script->mCommandNames.begin(), script->mCommandNames.end(), name) == script->mCommandNames.end())
            continue;

        lua_State* L = script->mLua;
        if (!L) return false;

        const std::string key = std::string("solstice!cmd!") + script->mName + "!" + name;
        lua_getfield(L, LUA_REGISTRYINDEX, key.c_str());
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return false;
        }

        lua_createtable(L, static_cast<int>(args.size()), 0);
        for (int i = 0; i < static_cast<int>(args.size()); ++i)
        {
            lua_pushstring(L, args[i].c_str());
            lua_rawseti(L, -2, i + 1);
        }

        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            spdlog::error("[script] {}: {}", name, err ? err : "command failed");
            lua_pop(L, 1);
        }
        return true;
    }
    return false;
}

size_t ScriptManager::getCommandCount() const
{
    size_t count = 0;
    for (const auto& script : mScripts) count += script->mCommandNames.size();
    return count;
}
