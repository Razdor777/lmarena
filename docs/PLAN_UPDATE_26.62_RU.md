# План перехода на 26.62 (1.26.5x)

## Статус на сейчас

| Этап | Состояние |
|---|---|
| Конфликты слияния и дубль `Solstice/Dexko` | ✅ снято (0 маркеров в `src/` и `26.62/`) |
| Единое имя клиента (`Solstice`), build-info макросы, `.log`/`.dll` | ✅ |
| CMake `26.62/`: один таргет `Solstice`, DirectXTK, шейдеры, фильтр исходников | ✅ |
| Слой совместимости SDK (методы, поля, `StringSetting`, `customData`) | ✅ |
| Combat (13 модулей) | ✅ перенесено, зарегистрировано, адаптировано |
| Player (37 модулей, включая `Regen`; + трекеры `CPSTracker`/`HealthTracker`/`PlayerTracker`/`Schematic`) | ✅ перенесено, зарегистрировано (`RegenRecode` в исходниках пустой — переносить нечего) |
| Movement (29 модулей) | ✅ перенесено, зарегистрировано |
| Misc (48 модулей) | ✅ перенесено, зарегистрировано |
| Visual (73 модуля, включая `Ambience`, `SmartXRay`, `Trails`, `TazikHat`) | ✅ перенесено, зарегистрировано |
| Регистрация модулей | ⚠️ 197 из 198: `FastEat` и `ItemPhysics` ждут живых сигнатур (их патчи пишут байты по адресу), `NoPause` — заглушка, в базе был пустой файл |
| Команды (54: schematic, pos1/pos2, spammer, fakechat, skin, playeroffset) | ✅ перенесено, зарегистрировано |
| GUI: новый ClickGui (`ModernDropdown` 1516 строк + `CombatantGui`), `ChatBinds` | ✅ перенесено |
| Interface: живой градиент темы, палитры Aurora/Abyss/Ember/Blossom/Prism, шрифт Mntsb | ✅ перенесено |
| Утилиты: `ItemAtlas`, `ItemTextures`, `TexturePtr`, `ClientPack`, `CubemapGenerator`, `NurikIcons` | ✅ перенесено |
| Хуки: `HealthHooks`, `ProjectileHooks`, `HurtColorHook`, `AmbienceHook`, ховер-детур инвентаря | ✅ добавлено (детуры встают только при живой сигнатуре) |
| События: `ProjectileHitEvent`, `HurtColorEvent`, `ContainerSlotHoveredEvent` | ✅ добавлено |
| Полнота: модули, которые в базе были «беднее» корня | ✅ 17 модулей заменены (Scaffold 64 настройки, OreMiner, BlockESP, RobloxCamera, TargetHUD, LevelInfo, JumpCircles, ESP, Tracers, Arraylist, Velocity, CustomChat, Nametags, ClickTp, DestroyProgress, NameProtect, Glint) |
| Сервисные файлы: `ExceptionHandler` (отчёт о крашах), `ConfigManager` (бэкап битых конфигов), `ChatUtils` | ✅ заменены полными версиями |
| Ресурсы | ✅ `tools/gen_resources.py` пересобирает `Resources.hpp/.cpp` из `26.62/resources` |
| Найденные при переносе дыры | ✅ `Interface::onLevelRenderEvent`, `ExceptionHandler.hpp`, `BaseTickInitEvent`, `ItemRendererHook`/`ItemIconQueue`, `resizeBuffers` в `D3DHook`, `ChatUtils::sSink` — см. `docs/PORT_LOG_26.62_RU.md` §7 |
| Сигнатуры под 1.26.5x | ⚠️ 6 «догадок» (`Actor_hurt`, `Projectile_onHit`, `HurtColor`, `ContainerScreenController_onContainerSlotHovered`, `ItemStack_fromDescriptor`, `ContainerScreenController_tryBeginTransition`) + 5 Ambience ждут скана (`docs/SIGS_26.62_RU.md`) |

Сборку целиком проверить негде: под рукой нет MSVC/Windows SDK, поэтому каждый
шаг проверяется статически — `tools/check_includes.py` (резолвятся ли все
`#include`), `tools/check_members.py` (нет ли обращений к членам, которых в новой
базе не существует), `tools/check_undefined.py` (объявлено, но без тела —
линковка), `tools/check_event_visibility.py` (виден ли тип события в `listen<>`)
и `tools/check_enums.py` (значение перечисления, которого нет).

```bash
python3 tools/sdk_compat.py   --out docs/SDK_COMPAT_26.62_RU.md   # что сломалось в SDK
python3 tools/check_includes.py 26.62/src                         # битые include
python3 tools/check_members.py --vocab-exclude Features -- 26.62/src   # члены без объявления
python3 tools/sig_checklist.py --out docs/SIGS_26.62_RU.md          # что ждёт скана
python3 tools/check_undefined.py 26.62/src                          # объявлено, но не определено
python3 tools/check_event_visibility.py                             # listen<> видит тип события
python3 tools/check_enums.py 26.62/src                              # Mode::X, где X не объявлен
python3 tools/gen_resources.py --check                              # ресурсы актуальны
```

Короткий ответ на «что тебе показать»: **ничего показывать не надо** — обе базы
уже лежат в этом репозитории, и по ним видно всё, что нужно решить.

| Что | Где | Состояние |
|---|---|---|
| Juzdex / Solstice (моя сборка на 1.21.44) | корень репо: `src/`, `include/`, `resources/`, `CMakeLists.txt` | 1255 файлов в `src/`, 198 модулей |
| Dexko (свежая основа под 26.62 / 1.26.5x) | `26.62/` | 640 файлов в `src/`, 126 модулей |

Свежая основа внутри помечена `// 1.21.131`, `// 1.26.51`, `// 1.26.0` — это и есть
то, до чего надо обновляться. Плюс в `26.62/include` есть `Luau`, которого в корне
нет (там новый скриптовый GUI), — ещё один повод строить поверх `26.62`, а не
наоборот.

---

## 0. Документы по этому плану

| Документ | Что внутри |
|---|---|
| `docs/SDK_COMPAT_26.62_RU.md` | автоотчёт `tools/sdk_compat.py`: 6 SDK-заголовков только у нас, 8 только в новой базе, 46 пропавших идентификаторов, из них 18 реально используются — с файлом и номером строки |
| `docs/COMBAT_PORT_26.62_RU.md` | Combat (приоритет): построчные правки, готовые замены API, список сигнатур |

Пересчитать отчёт после любых правок:

```bash
python3 tools/sdk_compat.py --out docs/SDK_COMPAT_26.62_RU.md
```

---

## 1. Что уже не так с репозиторием (это надо починить до переноса)

1. **Незакрытые конфликты в коммите.**
   - `26.62/`: 34 маркера в 13 файлах (`CMakeLists.txt`, `Dexko.cpp`, `Solstice.cpp`,
     `dllmain.cpp`, `Module.hpp`, `D3DHook.cpp/hpp`, `Reach.*`, `KeyHook.cpp`,
     `BlockSource.cpp`, `MemUtils.hpp`, `Utils.hpp`) — это слияние базы Dexko с
     моим форком.
   - корневой `src/`: 82 маркера в 11 файлах (`OreMinerV2.cpp/hpp`, `Trails.cpp`,
     `WorldParticles.cpp`, `CombatantGui.cpp`, `TazikHat.cpp`, `Kagune.cpp`,
     `Tracers.cpp`, `JumpCircles.cpp`, `InfiniteAura.hpp`).

   В таком виде **ни одна из двух сборок не компилируется**, а `git` считает
   рабочее дерево чистым. Первый шаг — выбрать сторону в каждом конфликте (это
   можно сделать детерминированно, обычно «моя сторона» = логика моего модуля).
2. **Дубликаты файлов в `26.62/src`**: `Solstice.cpp` и `Dexko.cpp` (и `.hpp`) —
   один и тот же файл под двумя именами (отличаются только шапкой конфликта).
   `file(GLOB_RECURSE src/*.cpp)` соберёт **оба** → двойные определения символов.
   Оставить одно имя.
3. **Мусор в `src/`**: `crash.log`, `project_code.html`, `merge_to_pdf.py`,
   `WhoisData.txt`, `WhoisDebug.txt`, `Untitled-1.txt`, `claude-sonnet-4-6.ini`,
   `src/.freebuff/`. В сборку не попадает, но в истории мешает.

---

## 2. Масштаб переноса (по факту)

Из 594 общих файлов **385 отличаются** — просто «скопировать мою папку в новую
основу» не выйдет. Только в корне: **210 файлов**, из них 148 — модули,
23 — команды, 8 — хуки, 6 — SDK, 5 — события, 5 — утилиты.
Только в `26.62`: 37 файлов (новые хуки `WndProc`, `NetherNet`, `LevelRenderer`,
`SetupShaderParameter`, `GrabMouse`; IRC/ScriptingGui/ptr-хелперы и т.д.).

### 2.1 Что нельзя перенести механически

| Подсистема | Числа | Почему ломается | Что делать |
|---|---|---|---|
| `SDK/SigManager.hpp` | мой 128 `DEFINE_SIG` / 94 в новой | диф 576 строк | 46 сигнатур есть только у меня; 4 из них реально используются: `HurtColor`, `HudCursorRenderer_render` (CustomCrosshair), `ContainerScreenController_onContainerSlotHovered`, `ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil` (ItemUseDelayFix). Их надо **заново снять по новому бинарнику** |
| `SDK/OffsetProvider.hpp` | 108 / 133 | диф 295 строк; у меня весь Flarial_* -блок и сигнал-скан индексов, в новой базе — жёсткие константы vtable (`ClientInstance_getLocalPlayer = 30`, `Level_getPlayerList = 319`, `Actor_baseTick = 25`…) | брать **всю** таблицу из `26.62` (она снята с нового exe), поверх добавлять только то, чего нет |
| Индексы vtable / оффсеты полей | — | сдвинулись целиком (`Actor_mLevel 0x200`, `ClientInstance_mMinecraftSim 0x1D0`, `Actor_mGameMode 0xac8`, …) | здесь только новая база, «мои» значения .44 недействительны |
| `PacketID.hpp` | 202 записи | совпадает 1:1 — **не трогать** | оставить как есть |
| `ComponentHashes.hpp` | 689 строк | файлы **идентичны** | оставить как есть |
| Runtime ID блоков в `OreMinerV2` | 16/661, 56/660, 129/662 | ID контента сдвигаются каждый апдейт | сверить с dump'ом новой версии |
| Архитектура модулей | 198 против 126 | в новой базе API ядра другой, см. ниже | см. 2.2 |

### 2.2 Реальный дрейф API ядра (это то, что даст ошибки компиляции)

- **ItemStack**: убраны `getDamage`, `getMaxDamage`, `getRemainingDurability`,
  `hasDurability`, `getDurabilityPercent`, `isArmorEnchantable`, `fromDescriptor`
  — около **46 обращений** в моих модулях. В новой базе вместо них
  `get`, `getId`, `gatherEnchants`, `reinit`, разбор тегов.
- **GameMode**: убраны `useItem`, `useItemOn`, `startBuildBlock`,
  `continueBuildBlock`, `stopBuildBlock`, `continueDestroyBlock`, `getPickRange`
  → новые `attack`, `buildBlock`, `destroyBlock`, `interact`, `baseUseItem`,
  `releaseUsingItem`, `getDestroyRate`. Это Aura / Scaffold / FastMine / Nuker /
  ChestAura и почти вся «игровая» механика.
- **Actor**: убран `getOffhandContainer` (10 мест), `setGameType` (4),
  `getHunger` (1); добавились `getMobJumpComponent`, `setPos`.
- **Level**: убраны `addParticle` (2 места), `getBlockPalette` (3) — палитра
  переехала в отдельный `BlockPalette` (`BlockPalette_mLevel`,
  `BlockLegacy_mTileName`, `BlockLegacy_mMaterial` появились).
- **Хуки**: `Actor_baseTick` теперь берётся по vtable-индексу из `DEFINE_FIELD`,
  а не сигнатурой; появились хуки под GDK/UWP (`WndProcHook`, `GrabMouseHook`).

### 2.3 Что переносится почти «как есть»

- чистая логика модулей: настройки, тайминги, рендер через ImGui, конфиги;
- `Features/Events/*` (добавить недостающие 5 событий);
- `Utils/*`, `Command*` (23 моих команды: S* build-команды, Spammer, FakeChat и т.д.);
- `ChatBinds`, `CombatantGui`, `ItemAtlas`, `ClientPack`, `CubemapGenerator`
  — только подправить вызовы SDK из 2.2.

---

## 3. Порядок работ (7 этапов, каждый — отдельный коммит)

> **Состояние на 2026-11-06:** шаги 0–5 выполнены (см. «Статус на сейчас» и
> `docs/PORT_LOG_26.62_RU.md`). Остались только шаг 6 (проверка в игре) и
> доснятие сигнатур — оба требуют `Minecraft.Windows.exe` 1.26.5x.

0. **Убрать конфликты и дубликаты** (п.1) — иначе дальше нечего собирать.
   Проверка: `grep -rn '^<<<<<<<' src 26.62/src` → пусто, в `26.62/src` один
   `Solstice.cpp`.
1. **Определиться с базой**: сборка живёт в `26.62/`, `CMakeLists.txt` — от
   `26.62` (там правильный `__DEBUG__` только для Debug, Luau, новые зависимости).
   Старый корень оставить как архив/референс (или слить в `legacy/`).
2. **Ядро SDK**: `SigManager.hpp` + `OffsetProvider.hpp` из `26.62` целиком.
   Доснять 4 нужные сигнатуры по новому exe (нужен `Minecraft.Windows.exe`
   1.26.5x и IDA/Ghidra; могу написать IDA-скрипт, который проверит мои 116
   паттернов по новому бинарнику и выдаст список «нашлось / не нашлось /
   несколько совпадений»).
3. **Слой SDK** (`Minecraft/*`): перенести новый SDK как есть, затем по одному
   править вызовы в моих модулях (начать с `ItemStack` и `GameMode` — они дадут
   больше всего ошибок).
4. **Хуки**: 8 моих хуков (`HealthHooks`, `ProjectileHooks`, `AmbienceHook`,
   `HurtColorHook`, `ContainerScreenControllerHook`, `CustomCrosshair`-хук и др.)
   перевесить на новые адреса; 5 новых хуков из `26.62` подключить.
5. **Модули**: 148 файлов, партиями по категориям (Combat → Player → Movement →
   Misc → Visual). После каждой партии — сборка, чтобы не копить ошибки.
6. **Контент-ID и проверка в игре** по чек-листу ниже, потом обновить
   `docs/UPDATE_1.21.70_RU.md` → `UPDATE_26.62_RU.md`.

План Б (если времени мало): сначала собрать **тонкий клиент** — ядро + Combat +
Movement + ClickGui/Arraylist + конфиги (~60 модулей), а визуал и редкие модули
портировать после того, как клиент запустится в игре.

---

## 4. Чек-лист приёмки после сборки

1. `[signatures] failed to find …` в логе — пусто (в Release клиент падает сразу,
   это специально).
2. `.help`, `.config`, `.bind` — работают; конфиги читаются старые.
3. Телепорт: `LastTP`, `ChainTP`, `InfiniteAura` — не улетает в 0,0,0.
4. Пакеты: чат, атака (`PacketSendHook`, `vtable[2]` вручную — проверить).
5. Рендер: ESP, Tracers, ClickGui, Arraylist, Nametags, ItemESP (атлас иконок
   у `ItemRenderer` переехал — см. `ItemRenderer_getAtlasIconInfo/getIconUV`).
6. Модули контента: `OreMiner`, `ChestAura`, `Nuker` — находят блоки (если «работает,
   но ничего не находит» — ID блоков).
7. Sound/scoreboard/queue-логика (`KickSounds`, `AutoQueue`, `AutoVote`).

---

## 5. Что мне нужно от тебя, чтобы это не встало

1. **Решение по базе**: строим в `26.62/` (рекомендую) — да/нет.
2. **Приоритет модулей**: какие 10–20 фич обязаны жить, остальное — потом.
3. **Бинарник 1.26.5x** (`Minecraft.Windows.exe`, желательно с PDB) или экспорт
   из IDA: без него шаг 2 (сигнатуры) физически не сделать — я могу написать
   только скрипт-проверку.
4. **Дампы новой версии**, если есть: список оффсетов/компонентов, `component_sizes.json`
   от 26.62, runtime ID блоков.
