# Порт Solstice 1.21.44 → 26.62 (1.26.62)

Коротко: **структура порта сделана**, дальше — сборка на Windows и правка того, что
вылезет по символам (это нормальный этап, бинарника игры у нас нет).

- **Мастер (твоё)**: `src/` — Solstice под 1.21.44, ~90 своих модулей.
- **База (донор)**: `26.62/` — обновлённый SDK (сигнатуры/оффсеты помечены 1.21.131 / 1.26.0 / 1.26.51).
- **Работа идёт в `26.62/`** — туда перенесены твои модули, хуки, утилиты, ресурсы.

---

## 1. Что уже сделано

### Конфликты
- Убраны **все** маркеры конфликтов: 30 в базе + 73 в 8 файлах, которые пришли из мастера
  (`OreMinerV2.cpp` — 13, `Trails.cpp` — 17, `WorldParticles.cpp` — 14, `CombatantGui.cpp` — 15 и т.д.).
- Нейминг `Dexko` → `Solstice` в 14 файлах; удалены побайтовые дубли `Dexko.cpp` / `Dexko.hpp`.

### CMake / сборка
- `project(Solstice)`, таргет `Solstice`, шейдер-таргет `SolsticeShaders`, `GenerateBuildInfo.cmake` даёт `SOLSTICE_*`.
- Убран `DirectXTK` (его нет в CPM этой базы и он нигде не инклудится).
- Unity-build выключен (глоб), с таргета снят — иначе 356 `.cpp` склеятся и посыпятся на анонимных namespace.
- `RuntimeObject.lib` вместо `WindowsApp.lib`/`OneCore.lib` (как в мастер-ветке).

### Таблицы, привязанные к версии
- `SDK/SigManager.hpp`: +48 сигнатур из мастера, 17 раскомментированы. Итог: **107 активных**.
- `SDK/OffsetProvider.hpp`: +33 оффсета, 12 раскомментированы. Итог: **114 активных**.
- Оба файла помечены блоком `// ==== ported from the 1.21.44 master — re-verify on 1.26.62 ====`.
- Починены дубликаты имён (иначе не собиралось): `GameSession_mEventCallback`, `ClientInputMappingFactory_mKeyboardRemappingLayout`.

### SDK-пробелы (то, что дёргает твой код, а в базе не было)
- `ItemStack::getDamage/getMaxDamage/getRemainingDurability/getDurabilityPercent/hasDurability`
- `Item::getMaxDamage()/isBlockItem()` (vtable 35 / 15)
- `ContainerScreenController::_tryBeginTransition`
- `Actor::getOffhandContainer()`, `Actor::getHunger()`, `Actor::setGameType()` (+ `PlayerHunger` в enum)
- `Module::hasCustomData/serializeCustomData/deserializeCustomData` — вернул виртуальные хуки
  из мастера и подключил их в `Module::serialize()` и `ModuleManager::deserialize()`.
- Удалён устаревший дубль `KeyboardMouseSettings.*` (в 1.26.x класс переименован в
  `KeyboardRemappingLayout`), `GhostMode` переведён на новый заголовок.

### Регистрация
- `ModuleManager`: +49 модулей из мастера (Aimbot, InfiniteAura, LastTP, OreMinerV2, Schematic,
  Kagune, TazikHat, Trails, ChatBinds-панель и т.д.).
- `HookManager`: зарегистрированы перенесённые хуки (`HealthHooks`, `ProjectileHooks`,
  `BaseTickHook`, `SetupAndRenderHook`, `AmbienceHook`, `HurtColorHook`).
- Ресурсы мастера скопированы в `26.62/resources` (+164 файла, всего 191).


### Модули: чья версия взята

Правило: **если у тебя модуль полнее — берём твой; если в новой базе он новее — остаётся базовый.**

Скопировано из мастера (62 файла, 31 модуль): InfiniteAura, AutoClicker, HitBoxes, TriggerBot,
OreMiner, Scaffold, ClickTp, Freecam, ChestStealer, ZipLine, Surround, CustomChat, Nametags,
RobloxCamera, Notifications, LevelInfo, BlockESP, TargetHUD, DestroyProgress, JumpCircles,
Tracers, ESP, NameProtect, Glint, ItemESP, PlaceHighlights, Velocity, AirJump, TargetStrafe,
AntiBot, Spammer.

Осталось базовым (в новой базе версия новее/богаче — там появились настройки, которых у тебя
в 1.21.44 нет): `Aura`, `Arraylist`, `Interface`, `ClickGui`, `Criticals`, `OreMinerV2`,
`Setting`, `ModuleManager`, `Reach`, `Disabler`, `Fly`, `Phase`, `Derp`, `AutoQueue`,
`HudEditor`, `Animations`, `SafeWalk`, `TestModule`, `Kagune`, `Trails`, `TazikHat`,
`WorldParticles` (последние три склеены из обеих версий при разрешении конфликтов).

Что именно потерялось в этих базовых версиях, если сравнивать с твоим 1.21.44
(если что-то из этого нужно — скажи, добавлю точечно):
- **Aura**: `Show ESP`, `ESP Style`, `ESP Alpha`, `Trail Length`, `Attack Range`, `Aim Range`,
  `Aim Y`, `Aim Random`, `Reaction`, `Rot Speed`, `Through Walls`, `TargetStrafe`.
- **Arraylist**: `Theme`, `Primary`, `Secondary`, `Gradient`, `Flow Speed`, `Show Mode`.
- **ClickGui**: `UI Scale`, `Panel Glow`, `Parallax`, `Motion Grid`, `Ambient Orbs`,
  `Cursor Trail`, `Effect Intensity`, `Show Hints`.
- **Interface**: `Gradient Flow`, `Flow Speed`, `Flow Depth`, `Wave Speed`, `Wave Spacing`,
  `Color Wave`, `Pulse*`, `Shimmer`, `Glow Boost`.
- **Criticals**: `Height`, `Only On Ground`.

### Шрифты и ресурсы
- `FontHelper` перенесён целиком: один интерфейсный шрифт Mntsb + merge кириллицы из Roboto,
  иконочные Tenacity/Nurik. `Resources.cpp` больше не регистрирует TTF (иначе каждый шрифт
  попадал в атлас дважды и ключ занимала копия без кириллицы).
- `26.62/include/ImGui/CMakeLists.txt` ссылался на несуществующий таргет `Dexko` → `Solstice`.
- `imgui_freetype.cpp` не собираем: нужны `ft2build.h` + `lunasvg`, которых в дереве нет
  (флаг `ImGuiFreeTypeBuilderFlags_NoHinting` без этого просто игнорируется).

### Удалено как мусор
- `KeyboardMouseSettings.{cpp,hpp}` — в 1.26.x класс переименован в `KeyboardRemappingLayout`
  (были дубли `ClientInputHandler`/`ClientInputMappingFactory`, линковка бы упала).
- Пустые `RegenRecode.{cpp,hpp}`, `NoPause.{cpp,hpp}`, `VSync.cpp`, случайно скопированные
  `src/cmake/CPM.cmake` и `src/GenerateBuildInfo.cmake`.
- `ItemPhysics` в базе целиком закомментирован (`/* ... */`) и не зарегистрирован — там те же
  glm-сигнатуры, что не находятся на новом билде. Оставлено выключенным.

## 1.1. Итоговые цифры после последнего прохода

| что | сколько |
|---|---|
| модулей зарегистрировано | **193** |
| `.hpp` в `26.62/src` | 486 |
| команд | **31** |
| хуков | **25** |
| активных сигнатур | **107** |
| активных оффсетов | **114** |
| `.cpp` в `26.62/src` | 352 |

### Что чинилось в этом проходе (всё из-за смены базы, не из-за твоего кода)

**SDK: переименованные поля.** В 1.26.x у `SubChunk` поля называются иначе
(`blockReadPtr` → `mBlocksReadPtr[0]`, `subchunkIndex` → `mAbsoluteIndex`). Твой
`OreMiner`, `OreMinerV2`, `BlockESP`, `SmartXRay`, `InfiniteChestAura`, `BlockUtils`
обращались к старым именам — в `SubChunk.hpp` добавлены поля-адаптеры
(`__declspec(property)`), так что код читается как раньше.

**SDK: недостающие методы.** `ItemStack::fromDescriptor` (+ `Level::getBlockPalette`),
`TeleportCommand::sLastUse`, `ColorUtils::{getStaticAccentColor, saturate, HSL-хелперы}`,
`BaseTickInitEvent`.

**Хуки.**
- `KeyHook` теперь целиком твой: сначала кормит ImGui (чтобы кириллица в чате
  работала — `ToUnicode` вместо `ToAscii`), гасит клавиши при вводе в CustomChat,
  поддерживает ChatBinds. Цепочка ввода в 26.62 другая (`WndProcHook` вместо детура
  `Keyboard::feed`), поэтому `onKey` возвращает `bool` — «погасить клавишу» — а
  `WndProcHook` его уважает.
- `ContainerScreenControllerHook` — версия с хуком наведения на слот, иначе
  `JavaInventory` и `ContainerActions` не работали бы (они зовут `isScreenActive()`).
- `ItemRendererHook` — теперь и событие базовое, и кэш `ItemRenderer`/`BaseActorRenderContext`
  для `InventoryHUD`/`ItemAtlas`.
- `BaseTickHook` — версия с мьютексом очереди пакетов (в базе гонка), плюс диспатч
  `BaseTickInitEvent`.

**Шрифты.** `FontHelper::load()` грузит Mntsb и мержит кириллицу из Roboto; TTF больше
не регистрируются дважды в `Resources.cpp` (раньше ключ занимала копия без кириллицы,
и русский текст мог рисоваться знаками вопроса).

**Команды.** 12 команд лежали в дереве, но не были зарегистрированы — `.spammer`,
`.fakechat`, `.pos1/.pos2`, `.sbuild`, `.scopy`, `.spaste`, `.scancel`, `.skin` и др.
молча ничего не делали.

**Конфиги.** `ConfigManager` из мастера: битый `config.json` больше не роняет клиент —
делается `.corrupted.<время>.json` и грузятся дефолты.

---

## 2. Что осталось (нужен Windows + VS2022)

Порядок тот же, что в `docs/UPDATE_1.21.70_RU.md`:

1. Собрать **Debug** (`cmake --preset x64-debug` → build). Debug не убивает процесс на
   ненайденной подписи — просто пишет в лог.
2. Запустить игру, открыть лог, собрать все строки:
   ```
   [signatures] failed to find <Имя>
   [offsets]    failed to find <Имя>
   ```
3. Прислать этот список — по нему точечно пойдём по сигнатурам 1.26.62.

### Заведомо «красные» зоны
- **`glm_rotate`, `glm_rotateRef`, `glm_translateRef`, `glm_translateRef2`** — в мастере они были
  **закомментированы**, но `ItemPhysics.cpp` их использует. Я включил их как есть (паттерны из
  мастера). Скорее всего, не найдутся — тогда `ItemPhysics` надо либо чинить по новому SDK, либо выключить.
- Сигнатуры мастера с пометками 1.21.44 вряд ли совпадут с 1.26.62 один-в-один; часть найдётся,
  часть — нет. Это ожидаемо.
- `OreMinerV2.hpp`: в мастере точка с запятой `Debug`-настроек была закомментирована — при
  первом запуске проверь, что все настройки читаются из конфига.
- `D3DHook` теперь берёт `wnd` из `sd.OutputWindow` и не подменяет оконную процедуру —
  WndProc-хук ставится отдельно (`WndProcHook`), так и в базе.

---

## 3. Быстрые команды сборки

```bat
cmake -S 26.62 -B 26.62\out\build\x64-debug -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build 26.62\out\build\x64-debug --config Debug --target Solstice -- -m
```

Требования: MSVC x64, CMake ≥ 3.28, MSYS2 `ld` в PATH (нужен для встраивания ресурсов), сеть (CPM).

Готовый файл: `26.62\out\build\x64-debug\Debug\Solstice.dll`.
