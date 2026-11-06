-- Example Solstice module script.
-- Drop this file into:
--   %LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Solstice\Scripts\Modules\
-- then run ".scripts reload" and toggle it on in the ClickGui (F6 opens the script list).

local module = {
    name        = "ExampleScript",
    description = "Logs every second it stays enabled",
    author      = "you",
}

local elapsed = 0

function module:onEnable()
    elapsed = 0
    log("ExampleScript enabled")
end

function module:onDisable()
    log("ExampleScript disabled")
end

function module:onTick()
    elapsed = elapsed + 1
    if elapsed % 20 == 0 then
        log(string.format("ExampleScript: %.1fs", elapsed / 20))
    end
end

function module:onRender()
    -- Called once per frame while enabled; draw with the standard Luau libs.
end

return module
