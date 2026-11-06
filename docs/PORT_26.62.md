# Порт проекта на Minecraft 26.62

Дата начала: 2026-11-06. Стратегия (выбрана владельцем): **база = `26.62/`, в неё
переносятся кастомные модули из корня (`src/`, Solstice / 1.21.44)**. Новых
сигнатур извне нет — работаем с тем, что уже лежит в `26.62/src/SDK`.

## Что такое папка `26.62/` на самом деле

Это не чистый апстрим, а результат неудачного слияния двух веток
(HEAD = Solstice-сторона, `0694274...` = Dexko-сторона):

- главный класс в обоих `Solstice.hpp` и `Dexko.hpp` назывался `Dexko`, а
  `Solstice.cpp` и `Dexko.cpp` были одним и тем же файлом => двойные символы
  при сборке через `file(GLOB_RECURSE src/*.cpp)`;
- 13 файлов содержали маркеры конфликтов `<<<<<<<`;
- класс `Dexko` потерял поля, на которые ссылается собственный код
  (`mUnloading` в `D3DHook`, `sHWID` в `IrcClient`, `unloadThreadProc` в
  `Dexko.cpp`);
- `FileUtils::getDexkoDir()` вызывался (`Dexko.cpp`, `PreferenceManager`),
  но нигде не был объявлен.

При этом SDK в `26.62/` действительно обновлён под новую версию игры, а не
скопирован со старой: из 69 общих сигнатур паттерн изменился у 64, из 60
общих оффсетов — у 21. Обратная сторона: база урезана в стиле Dexko
(94 сигнатуры против 126 в старом проекте, 84 оффсета против 104).

## Этап 1 — база приведена в цельное состояние (сделано)

Каноническое имя клиента — **Dexko** (так называется CMake-проект и DLL).

| Файл | Решение |
|---|---|
| `26.62/CMakeLists.txt` | Взята Dexko-сторона; определены оба макроса `DEXKO_VERSION` и `SOLSTICE_VERSION` |
| `26.62/GenerateBuildInfo.cmake` | В `build_info.h` добавлены алиасы `SOLSTICE_BUILD_*` -> `DEXKO_BUILD_*` |
| `26.62/src/Dexko.hpp` | Возвращены `mUnloading` (atomic), `sHWID`, `unloadThreadProc`; добавлен `using Solstice = Dexko;` |
| `26.62/src/Solstice.hpp` | Заменён на шим: `#include "Dexko.hpp"` |
| `26.62/src/Solstice.cpp` | **Удалён** (дубль `Dexko.cpp`) |
| `26.62/src/Dexko.cpp` | Dexko-сторона всех ханков (безопасная выгрузка через отдельный поток); при eject выставляется `mUnloading` |
| `26.62/src/dllmain.cpp` | Переписан начисто: `Dexko::init` через `CreateThread` |
| `Hook/Hooks/MiscHooks/KeyHook.cpp` | Слияние: `Dexko.hpp` + `ClickGui.hpp` (тело использует `getModule<ClickGui>`) |
| `Features/Modules/Module.hpp` | Dexko-сторона (поля анимаций ArrayList) |
| `Features/Modules/Combat/Reach.*` | `onEnable`/`onBaseTick` — логика `mCombatPtr` (согласована внутри файла), в `onDisable` освобождается `mCombatPtr`; добавлен член `mDetour`. **Известно: настройка Full Override пока ничего не делает** — detour-код в `onEnable` закомментирован ещё до слияния, чинить отдельным шагом с проверкой в игре |
| `Hook/Hooks/RenderHooks/D3DHook.*` | Слияние: текстуры из встроенных ресурсов (нужны `Watermark`/`TargetHUD`/`RenderUtils`) + `present/resizeBuffers(IDXGISwapChain3*)` и `kiero::bind(140/145, 8/13)` Dexko-стороны + guard `mUnloading` + сохранён `WindowResizeEvent` (его слушает ClickGui) |
| `SDK/Minecraft/World/BlockSource.cpp` | Оставлены include-ы HEAD (`LevelChunk` нужен телу) |
| `Utils/MemUtils.hpp` | Оставлен `<source_location>` (реально используется ниже по файлу) |
| `Utils/Utils.hpp` | Оставлены `ChatGPT/MessageTemplate/AudioUtils` (файлы есть, код используется) |
| `Utils/FileUtils.*` | Добавлен `getDexkoDir()` = `getSolsticeDir()` — одна папка данных на весь клиент, конфиги/префы не разбегаются |

Проверка после этапа: маркеров конфликтов в `26.62/` не осталось; внутри
самой базы ссылок на несуществующие сигнатуры/оффсеты нет (единственные
находки статического поиска — закомментированный код в
`ClientInstance::getFov` и `ItemStackRequestPacket.hpp`, плюс двойная
квалификация `SigManager::SigManager::` в `ItemPhysics.cpp:89-90` — на заметку
при первой сборке).

## Что переносим (этап 2): 182 файла из старого `src/Features`

- **Combat (6 модулей):** Aimbot, ArrowTP, GhostMode, InfiniteChestAura, LastTP, PearlStopper
- **Misc (16):** AdminPanel, ArmorSlotUnlock, AutoContract, AutoLoot, FakeChat, ItemDupe, ItemUseDelayFix, JavaInventory, JavaInventoryHotkeys, NoPause, PlayerLogger, SkinChanger, SkinSpoofer, SpectatorDetector, VoiceChat, WhoisCollector
- **Movement (3):** AutoPatrol, AutoSneak, NoClip
- **Player (11):** AutoArmor, Cage, ChainTP, ContainerActions, OreMinerV2, PlayerTracker, Regen, RegenRecode, RegionFill, SchematicBuilder (+Schematic.hpp), SpawnerAura, SwitchBallAim
- **Visual (36):** AfterImage, Ambience, AmbientCubes, ArmorHUD, ArrowRecovery, BoneEsp, BowHitMarker, CustomCrosshair, CustomFog, DamageLog, EnemyIndicator, EntityRadar, FakePlayer, Goofy, HealthBar, HealthDisplay, HitColor, HitEffects, HitParticles, InventoryHUD, Kagune, NearbyPlayers, OpponentReach, PathTracer, PlayerHUD, PlayerPositionOffset, PotionHUD, ProjectileTracer, ReachCounter, SkyPack, SmartXRay, SoulSpiral, TazikHat, TeamHealthBars, Trails, VSync, WorldParticles
- **Команды (11):** FakeChat, PlayerOffset, Pos1, Pos2, SBuild, SCancel, SCopy, SPaste, SPos1, SPos2, Spammer (+ header-only Skin)
- **Прочее:** ChatBinds (+GUI-панель), CombatantGui, ModuleToggle, события
  ContainerSlotHovered, EntityHurt, HurtColor, ProjectileHit, SetupAndRender

Порядок переноса: события -> Combat/Movement/Player -> Misc -> Visual -> команды/GUI.
Каждый модуль: копирование, `Solstice::` работает через алиас, регистрация в
`ModuleManager.cpp` (и в `CommandManager.cpp` для команд), проверка по списку ниже.

## Известные дыры (нужны сигнатуры/хуки под 26.62, в базе их нет)

Твоим модулям не хватает всего двух сигнатур — обе надо искать по новому
бинарнику игры (или вырезать/переписать зависящий код):

1. `SigManager::ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil` — `Misc/ItemUseDelayFix`
2. `SigManager::HudCursorRenderer_render` — `Visual/CustomCrosshair`

Нет трёх событий (и, видимо, хуков, которые их публикуют):

1. `EntityHurtEvent`
2. `HurtColorEvent` (+ сигны `HurtColor` в новой базе тоже нет)
3. `ProjectileHitEvent`

Также в новой базе нет части старых сигнатур, которые используют **общие**
(не уникальные) модули старого проекта — но эти модули в `26.62/` уже
переписаны под новый SDK, поэтому старые версии поверх копировать нельзя:
общие файлы всегда берём из `26.62/`, переносим только уникальные.

## Как проверяем (цикл на каждый этап)

Собрать в песочнице Arena нельзя (нужен MSVC + Windows SDK + сама игра), поэтому:

```bat
cmake -S 26.62 -B 26.62/build -G "Visual Studio 17 2022" -A x64
cmake --build 26.62/build --config Release --target Dexko
```

1. Владелец собирает локально, присылает первые ~50 строк ошибок компилятора.
2. Чиню по ошибкам, повторяем до чистой сборки.
3. Инжект в Minecraft 26.62, из лога нужны строки `[signatures] failed to find ...`
   и `[offsets] Failed to find offset: ...` — по ним закрываем дыры из списка выше.
