-- Example Solstice command script.
-- Drop this file into:
--   %LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Solstice\Scripts\Commands\
-- then run ".scripts reload" and use ".hello <text>".

registerCommand("hello", "Says hello to the given name", function(args)
    local who = args[1] or "world"
    sendClientMessage("[§6Script§7] §aHello, §e" .. who .. "§a!")
end)
