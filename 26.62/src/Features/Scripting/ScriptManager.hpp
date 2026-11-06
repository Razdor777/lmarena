//
// Luau scripting runtime for Solstice.
//
// Layout (created by FileUtils::validateDirectories):
//   <SolsticeDir>\Scripts\Modules\<name>.luau    -> module scripts (own tab in ClickGui)
//   <SolsticeDir>\Scripts\Commands\<name>.luau   -> command scripts (.command registration)
//
// Scripts are plain Luau chunks. A module script gets exactly one global table:
//   local module = { name = "...", description = "...", author = "...", enabled = false }
//   function module:onEnable()   end
//   function module:onDisable()  end
//   function module:onTick()     end
//   function module:onRender()   end
//   return module
//
// A command script is registered from inside the chunk:
//   registerCommand("name", "description", function(args) ... end)
//
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>

struct lua_State;

enum class ScriptType {
    Module,
    Command
};

// One loaded chunk. Holds its own lua_State so a broken script can be reloaded
// (or unloaded) without tearing down every other script.
class Script {
public:
    ScriptType mType = ScriptType::Module;
    std::string mName;         // file stem, also the script-side module name
    std::string mDescription;
    std::string mAuthor;
    std::string mPath;
    bool mEnabled = false;

    lua_State* mLua = nullptr;
    int mModuleRef = 0;        // registry ref to the module table (LUA_NOREF when absent)
    int mOnEnableRef = 0;
    int mOnDisableRef = 0;
    int mOnTickRef = 0;
    int mOnRenderRef = 0;

    // Command scripts only
    std::vector<std::string> mCommandNames;

    ~Script();
};

class ScriptManager {
public:
    std::vector<std::shared_ptr<Script>> mScripts;
    std::atomic<bool> mLoadingScripts = false;

    void init();
    void shutdown();

    // Disk discovery + (re)load. Safe to call at runtime ("scripts reload").
    void reloadAll();
    bool loadScript(const std::string& path);

    // Drops and reloads a single script by name (module or command).
    bool reloadScript(const std::string& name);

    std::vector<std::shared_ptr<Script>>& getScripts() { return mScripts; }
    std::shared_ptr<Script> getScript(const std::string& name);

    // Fires onEnable/onDisable, mirrored as a ModuleScriptStateChangeEvent so
    // ToggleSounds and Notifications react the same way they do for C++ modules.
    void setScriptEnabled(const std::shared_ptr<Script>& script, bool enabled);

    // Called from the client tick / render hooks.
    void onClientTick();
    void onRenderEvent();

    // True if a command script handled the invocation.
    bool findAndExecCommand(const std::string& name, const std::vector<std::string>& args);

    // Number of script-provided commands, for HelpCommand.
    size_t getCommandCount() const;

private:
    std::mutex mMutex;

    lua_State* createState();
    bool runChunk(Script& script, const std::string& source);
    void extractModuleTable(Script& script);
    void registerApi(lua_State* L, Script* owner);

    void unloadScript(const std::shared_ptr<Script>& script);

    static int l_registerCommand(lua_State* L);
    static int l_log(lua_State* L);
    static int l_getModule(lua_State* L);
    static int l_isModuleEnabled(lua_State* L);
    static int l_setModuleEnabled(lua_State* L);
    static int l_getClientMessage(lua_State* L); // placeholder no-op kept for parity
};
