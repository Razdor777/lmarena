# Dexko

Лёгкий автономный клиент для Minecraft Bedrock (Windows, x64), собранный на базе
движка Solstice, но **полностью самодостаточный**: папка `dexko/` содержит всё
своё — исходники, библиотеки и ресурсы. Ничего из родительского репо при
сборке не используется.

## Сборка

```cmd
cmake -B dexko\build -S dexko
cmake --build dexko\build --config Release
```

Результат: `dexko\build\Release\Dexko.dll`. Eject — клавиша **END**.

Требования: MSVC (C++26), CMake ≥ 3.28, MinGW `ld` из MSYS2
(`pacman -S mingw-w64-x86_64-binutils`) для запаковки ресурсов.
Внешние зависимости (spdlog, magic_enum, glm, nlohmann_json, cryptopp)
скачиваются CPM автоматически при первой конфигурации.

Перенос проекта = копирование папки `dexko/` целиком.

## Что внутри

**Модули** (всего 6, ClickGUI нет):

| Модуль | По умолчанию | Заметки |
|---|---|---|
| `DeviceSpoof` | **всегда включён** | форсится при каждом запуске, конфигом не выключить |
| `CustomCrosshair` | включён | дефолты по заданному конфигу (CsGo, Full, Scale 0.8, Size 3.5, Gap 6.5, Thickness 1.28, Expand 6.0, Sway 3.5, Rotation Sway 1.10, Rotation Lag 0.95) |
| `HitParticles` | включён | дефолты по заданному конфигу (Critical, Theme Color, Amount 8, Spread 0.45, Speed 0.2, Size 3.5, Gravity 1.00, Drag 2.0, Lifetime 0.90, все тумблеры on) |
| `Reach` | выключен | **только combat reach** (block reach вырезан) |
| `Interface` | инфраструктурный | палитра темы (Color Mode = Theme Color у HitParticles) |
| `AntiBot` | инфраструктурный | фильтр ботов (ActorUtils рассчитывает на него) |

**Команды** (префикс `.`):

| Команда | Что делает |
|---|---|
| `.reach` | текущий combat reach |
| `.reach 3.00` | поставить combat reach = 3.00 и включить (диапазон 3.0–7.0) |
| `.reach off` | выключить Reach |
| `.htp` | текущий стиль + список |
| `.htp sparks` `.htp fire` `.htp hearts` ... | сменить стиль (blood/sparks/critical/hearts/fire/frost/soul/toxic) |
| `.htp off` | выключить HitParticles |
| `.config save/load/list` | конфиги (`RoamingState\Dexko\Configs\`) |
| `.t <модуль>` | вкл/выкл модуль |
| `.help` | список команд |

## Структура

```
dexko/
├── CMakeLists.txt              # автономная сборка Dexko.dll
├── GenerateBuildInfo.cmake     # DEXKO_BUILD_* (+ алиасы SOLSTICE_* для форков движка)
├── cmake/CPM.cmake
├── include/                    # ImGui, Kiero, MinHook, libhat, entt, nes (свои копии)
├── resources/fonts/            # только Roboto-Regular (текст с кириллицей)
└── src/
    ├── Dexko.* / Solstice.hpp  # главный класс; Solstice.hpp = алиас для форков движка
    ├── dllmain.cpp  pch.hpp
    ├── glm/                    # вендоренная glm (как в исходном репо)
    ├── SDK/                    # SDK игры (SigManager, OffsetProvider, клиенты, пакеты)
    ├── Hook/                   # Detour/Hook/HookManager + 6 хуков (см. ниже)
    ├── Features/
    │   ├── Events/  Configs/   # события, конфиги/преференции
    │   ├── Command/            # CommandManager (5 команд)
    │   └── Modules/            # ModuleManager + 6 модулей
    └── Utils/                  # утилиты движка (FileUtils/FontHelper — форки Dexko)
```

**Хуки** (только необходимые):
`KeyHook` (END/кейбинды) · `D3DHook` (ImGui + RenderEvent) · `BaseTickHook`
(BaseTickEvent) · `PacketSendHook` (ChatEvent для команд + PacketOutEvent для
HitParticles) · `ConnectionRequestHook` (DeviceSpoof) ·
`PacketReceiveHook`/`ActorRenderDispatcherHook` скомпилированы, но не
регистрируются (нужны для линковки Util'ов/Interface, событий не дают).

## Форки движка (отличия от Solstice)

| Файл | Отличие |
|---|---|
| `src/Dexko.cpp`, `Solstice.hpp`, `dllmain.cpp` | свой брендинг; без Auth и проверки обновлений; DeviceSpoof форсится всегда |
| `src/Features/Modules/ModuleManager.cpp` | только 6 модулей |
| `src/Features/Command/CommandManager.cpp` | только 5 команд (+ свои `.reach`, `.htp`) |
| `src/Hook/HookManager.cpp` | только нужные хуки |
| `src/Hook/Hooks/MiscHooks/KeyHook.cpp` | без ClickGui |
| `src/Features/Modules/Visual/Interface.cpp` | без ClickGui-зависимости и мёртвого renderHoverText |
| `src/Features/Modules/Misc/AntiBot.cpp` | без модуля Teams |
| `src/Features/Command/Commands/ConfigCommand.cpp` | без Notifications |
| `src/Utils/FileUtils.cpp` | данные в `RoamingState\Dexko\` |
| `src/Utils/FontHelper.cpp` | Roboto вместо отсутствующих в репо Mntsb/Nurik |
| `src/Features/Modules/Combat/Reach.*` | combat-only |
| `src/Features/Modules/Visual/CustomCrosshair.hpp`, `HitParticles.hpp` | включены по умолчанию, дефолты под заданный конфиг |

## Как добавить модуль/команду

- Модуль: положить `.cpp/.hpp` в `src/Features/Modules/...` и добавить 2 строки
  в `src/Features/Modules/ModuleManager.cpp`.
- Команда: положить в `src/Features/Command/Commands/` и добавить 2 строки в
  `src/Features/Command/CommandManager.cpp`.
