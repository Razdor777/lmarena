# Dexko — исходники

game version: 1.21.44 | сборка: cmake (см. ../README.md)

Структура:

- `Features/Command` — команды чата (`.` в начале строки).
- `Features/Configs` — конфиги и preferences.
- `Features/Events` — события, которые рассылает dispatcher.
- `Features/Modules` — модули (`Combat`, `Misc`, `Visual`).
- `Hook` — MinHook-хуки, которые кормят события: `BaseTickHook`, `D3DHook`,
  `PacketSendHook`, `SetupAndRenderHook`, `ActorRenderDispatcherHook`,
  `HoverTextRendererHook`, `ConnectionRequestHook`, `PreGameHook`, `KeyHook`.
- `SDK` — оффсеты, сигнатуры и обёртки над классами игры.
- `Utils` — утилиты (рендер, чат, файлы, память).
