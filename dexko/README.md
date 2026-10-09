# Dexko

Лёгкий клиент для Minecraft Bedrock (Windows, x64) — **проект внутри проекта**.
Собирается в отдельный **Dexko.dll**, использует движок родительского репо
(SDK, хуки, система модулей/команд), но содержит только минимум функций.

## Что внутри

**Модули** (ClickGUI нет вообще):

| Модуль | Состояние по умолчанию | Заметки |
|---|---|---|
| `DeviceSpoof` | **всегда включён** | форсится при каждом запуске, конфигом не выключить |
| `CustomCrosshair` | включён | дефолты = настройки со скриншотов (Style: CsGo, Dynamic: Full, Scale 0.8, Size 3.5, Gap 6.5, Thickness 1.28, Expand 6.0, Sway 3.5, Rotation Sway 1.10, Rotation Lag 0.95 и т.д.) |
| `HitParticles` | включён | дефолты = со скриншотов (Style: Critical, Color Mode: Theme Color, Amount 8, Spread 0.45, Speed 0.2, Size 3.5, Gravity 1.00, Drag 2.0, Lifetime 0.90, все тумблеры on) |
| `Reach` | выключен | **только combat reach**, включается командой `.reach` |

Плюс два инфраструктурных модуля (без них движок не живёт, пользовательски не тогглятся):
- `Interface` — палитра темы (нужен для Color Mode = Theme Color у HitParticles)
- `AntiBot` — фильтр ботов (ActorUtils жёстко рассчитывает на него)

**Команды** (префикс `.`):

| Команда | Что делает |
|---|---|
| `.reach` | показать текущий combat reach |
| `.reach 3.00` | поставить combat reach = 3.00 и включить Reach (диапазон 3.0–7.0) |
| `.reach off` | выключить Reach |
| `.htp` | показать текущий стиль HitParticles и список |
| `.htp sparks` `.htp fire` `.htp hearts` `.htp critical` | сменить стиль (всего: blood / sparks / critical / hearts / fire / frost / soul / toxic) |
| `.htp off` | выключить HitParticles |
| `.config save <имя>` / `.config load <имя>` / `.config list` | конфиги (лежат в `%LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Dexko\Configs\`) |
| `.t <модуль>` | вкл/выкл модуль вручную |
| `.help` | список команд |

Eject — клавиша **END**.

## Сборка

Требуется то же, что и для родительского проекта: MSVC, CMake ≥ 3.28,
MinGW `ld` из MSYS2 (`pacman -S mingw-w64-x86_64-binutils`) для запаковки ресурсов.

```cmd
cmake -B dexko\build -S dexko
cmake --build dexko\build --config Release
```

Результат: `dexko\build\Release\Dexko.dll`. Инжект — как обычно (CommandLineInjector и т.п.).

## Структура (что копировать)

Весь «продукт» Dexko лежит в **этой папке** — её можно целиком скопировать в
другой checkout этого же репо:

```
dexko/
├── CMakeLists.txt                  # отдельный таргет Dexko (движок берёт из ../src, ../include, ../resources)
├── GenerateBuildInfo.cmake         # DEXKO_BUILD_* (+ алиасы SOLSTICE_BUILD_* для движка)
├── README.md
└── src/
    ├── Dexko.hpp / Dexko.cpp       # главный класс (init/shutdown, брендинг)
    ├── Solstice.hpp                # алиас: движок продолжает видеть "Solstice" = Dexko
    ├── dllmain.cpp                 # входная точка
    ├── Features/
    │   ├── Command/
    │   │   ├── CommandManager.cpp  # свой список команд
    │   │   └── Commands/
    │   │       ├── ReachCommand.*  # .reach
    │   │       └── HtpCommand.*    # .htp
    │   └── Modules/
    │       ├── ModuleManager.cpp   # свой список модулей
    │       ├── Combat/Reach.*      # combat-only
    │       ├── Misc/DeviceSpoof.*  # always-on
    │       └── Visual/             # CustomCrosshair, HitParticles (дефолты со скриншотов)
    └── Utils/
        └── FileUtils.cpp           # форк: данные в RoamingState\Dexko\
```

Движковые части (`src/SDK`, `src/Hook`, `src/Utils`, инфраструктура
`Features`, `include/`, `resources/`) **не копируются** — подпроект ссылается
на них по относительным путям `../`.

## Как добавить/убрать модуль или команду

- Модуль: пара строк в `src/Features/Modules/ModuleManager.cpp` (`#include` + `emplace_back`).
  Файлы модуля можно класть прямо в `dexko/src/Features/Modules/...` (как здесь).
- Команда: `#include` + `ADD_COMMAND(...)` в `src/Features/Command/CommandManager.cpp`.

## Отличия от Solstice по поведению

- Нет авторизации (Auth) и проверки обновлений.
- Нет ClickGUI, ArrayList и прочих HUD-модулей — только перечисленное выше.
- Конфиги/логи — в `RoamingState\Dexko\` (не перемешиваются с Juzdex/Solstice).
- Дефолты CustomCrosshair/HitParticles выставлены под твой конфиг со скриншотов,
  дальше всё сохраняется через `.config save`.
