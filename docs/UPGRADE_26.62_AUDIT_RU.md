# Аудит: переезд с Solstice 1.21.44 на базу 26.62

Дата аудита: 2026-11-06. Сравнивались `src/` (текущая база) и `26.62/src/`.

---

## 1. Что реально лежит в репозитории

| Путь | Что это | Объём |
|---|---|---|
| `src/`, `include/`, `CMakeLists.txt` | твоя текущая база — **Solstice** под **1.21.44** | 1 255 файлов, ~140 000 строк |
| `26.62/` | **другая** база — **Dexko**, внутри SDK под ~26.51 / 1.21.131 | 640 файлов, ~58 500 строк |
| `dexko/` | маленький самостоятельный клиент (Reach, Crosshair, HitParticles) на 1.21.44 | ~40 файлов |

Ключевой момент: `26.62/` — **не «мой Solstice, обновлённый до 26.62»**. Это другой скелет:

- `project(Dexko)`, `Dexko.cpp` / `Dexko.hpp`, строки `Dexko` в консоли, лог `dexko.log`, дефайны `DEXKO_*`;
- он **сильно обрезан** относительно твоего: 640 файлов против 1 255 и 58 500 строк против 140 000;
- в нём есть то, чего у тебя нет: Luau-скрипты (`ScriptManager`, `ScriptingGui`), IRC-клиент, `AutoWalk`, `AutoEat`, новые хуки (`NetherNet`, `SetupShaderParameter`, `GrabMouse`, `WndProc`);
- и нет ~50 твоих модулей и кучи утилит.

То есть задача — не «обновить версию», а **пересадить твои ~140к строк на чужой, более тонкий скелет**, либо наоборот — затащить в свой скелет только версионный слой (SDK/хуки/сигнатуры).

---

## 2. Важное про «свежесть» 26.62

`26.62` = Minecraft Bedrock **26.62** (внутренняя 1.26.6201.0), вышла **4 ноября 2026**, хотфикс — 5 ноября. Сервер 1.26.62.1, протокол 2225. То есть да, это буквально два дня назад.

**Но SDK внутри `26.62/` снят не с 26.62.** Смотри комментарии:

- `SignManager.hpp` — 10 пометок `26.51`;
- `OffsetProvider.hpp` — 6 пометок `1.21.131` (= 16.12.2025), 1 пометка `26.51`, `MinecraftGame_mXuid // 1.26.51`;
- ещё есть висящие `1.21.51`, `1.21.80`.

Между 26.51 и 26.62 успели выйти 26.52 (25.09), 26.60 (27.10 — контент-дроп) и 26.62 (04.11). Хотфикс обычно сигнатуры не двигает, но контент-дроп — вполне. **Слой сигнатур/оффсетов надо перепроверить на exe 26.62 независимо от того, какой путь выберем.**

---

## 3. Сколько твоего кода отсутствует в новой базе

Из 136 модулей, зарегистрированных в твоём `ModuleManager.cpp`, **49 вообще отсутствуют** в `26.62/` (нет ни `.hpp`, ни `.cpp`):

```
AdminPanel      AfterImage      Aimbot          Ambience        AmbientCubes
ArmorHUD        ArmorSlotUnlock ArrowRecovery   ArrowTP         AutoArmor
AutoContract    AutoLoot        AutoPatrol      AutoSneak       BoneEsp
BowHitMarker    Cage            ContainerActions CustomCrosshair CustomFog
DamageLog       EnemyIndicator  EntityRadar     FakeChat        FakePlayer
Goofy           HealthBar       HealthDisplay   HitColor        HitEffects
HitParticles    InfiniteChestAura InventoryHUD  JavaInventory   JavaInventoryHotkeys
Kagune          LastTP          NearbyPlayers   NoClip          OpponentReach
OreMinerV2      PathTracer      PearlStopper    PlayerHUD       PlayerPositionOffset
PlayerTracker   PotionHUD       ProjectileTracer ReachCounter   RegionFill
SchematicBuilder SkyPack        SmartXRay       SoulSpiral      SpawnerAura
TazikHat        TeamHealthBars  Trails          VSync           WhoisCollector
WorldParticles  VoiceChat       ChatBinds       CombatantGui    ChatBindsPanel
```

87 модулей есть в обеих базах, но из них **155 файлов отличаются** (в сумме ~23 600 изменённых строк только в `Features/Modules`). 98 файлов идентичны.

---

## 4. В `26.62/` сейчас не соберётся вообще — и это до твоего кода

В 13 файлах остались **неразрешённые маркеры merge-конфликта** (причём местами 8 символов `<<<<<<<<` — конфликт сливали руками):

| Файл | Маркеров |
|---|---|
| `CMakeLists.txt` | 6 |
| `src/Hook/Hooks/RenderHooks/D3DHook.cpp` | 7 |
| `src/Dexko.cpp` | 5 |
| `src/Solstice.cpp` | 5 |
| `src/Features/Modules/Module.hpp` | 2 |
| `src/Features/Modules/Combat/Reach.cpp` | 2 |
| `src/Features/Modules/Combat/Reach.hpp` | 1 |
| `src/Hook/Hooks/MiscHooks/KeyHook.cpp` | 1 |
| `src/Hook/Hooks/RenderHooks/D3DHook.hpp` | 1 |
| `src/SDK/Minecraft/World/BlockSource.cpp` | 1 |
| `src/Utils/MemUtils.hpp` | 1 |
| `src/Utils/Utils.hpp` | 1 |
| `src/dllmain.cpp` | 1 |

Плюс логические поломки помимо маркеров:

1. **`Solstice.cpp` и `Dexko.cpp` — почти одинаковые файлы** (различаются только словом в имени). Оба определяют глобальные `std::string title`, `void setTitle(...)`, `Dexko::init` → дубли символов на линковке.
2. `src/Solstice.hpp` объявляет **`class Dexko`** (не `Solstice`) — дерево в середине переименования.
3. Смешаны обращения: 30 × `Dexko::` и 48 × `Solstice::`.
4. `D3DHook.cpp` использует `Dexko::mUnloading`, которого **нет** ни в `Dexko.hpp`, ни в `Solstice.hpp`.
5. `FeatureManager` ссылается на `mScriptManager`, который **закомментирован** в `FeatureManager.hpp`, но используется в `HelpCommand`, `TestCommand`, `CommandManager`, `ScriptingGui` — Luau-слой выпилен, а вызовы остались.
6. В `CMakeLists.txt` живут одновременно `add_library(Solstice …)` и `add_library(Dexko …)`, два набора версий (`SOLSTICE_VERSION` / `DEXKO_VERSION`), и `set(CMAKE_UNITY_BUILD …)` в трёх вариантах.
7. `src/Utils/Resources.cpp` / `.hpp` **генерируются CMake** — их нельзя править руками (это ловушка, в которую легко попасть при мерже).

Вывод: `26.62/` — это рабочая копия в состоянии «мерж наполовину сделан». Первым шагом в любом сценарии её надо довести до собираемого состояния.

---

## 5. API-разломы между твоим кодом и новой базой

### 5.1 Удалённые события (используются твоими модулями)

| Событие | Сколько файлов у тебя его упоминают |
|---|---|
| `EntityHurtEvent` | 8 |
| `ProjectileHitEvent` | 6 |
| `HurtColorEvent` | 4 |
| `ContainerSlotHoveredEvent` | 2 |
| `SetupAndRenderEvent` | 1 |

### 5.2 Удалённые хуки (есть у тебя, нет в 26.62)

`HealthHooks`, `ProjectileHooks`, `AmbienceHook`, `HurtColorHook`.

### 5.3 Удалённые утилиты/структуры, от которых зависят твои модули

| Чего нет в 26.62 | Файлов у тебя ссылается |
|---|---|
| `SDK/Minecraft/KeyboardMouseSettings` | 10 |
| `Utils/GameUtils/HealthTracker` | 6 |
| `Utils/ItemTextures` | 4 |
| `SDK/Minecraft/Rendering/ItemRenderer` | 4 |
| `SDK/Minecraft/Rendering/TexturePtr` | 2 |
| `Utils/ItemAtlas` | 1 |
| `Utils/GameUtils/CPSTracker` | 1 |
| `Utils/MiscUtils/ClientPack` | 1 |

Часть переименована: `KeyboardMouseSettings` → `KeyboardRemappingLayout` (тип возврата `ClientInstance::getKeyboardSettings()` поменялся).

### 5.4 Изменения в базовых классах

- `Actor`: `filler` 0x408 → 0x3d0, другие оффсеты полей, `getStatusFlag` больше не null-safe (твоя версия проверяла `data` на null — это была твоя правка, её надо вернуть, иначе падения при выходе из мира).
- `ClientInstance`: добавлен `vtable`, `getPlatformOnlineId()`, `getKeyboardSettings()` теперь `KeyboardRemappingLayout*`.
- `Module`: `hasCustomData()` / `serializeCustomData()` / `deserializeCustomData()` **исчезли** из `Module.hpp` (у тебя они есть — на них, скорее всего, висят сохранения состояния модулей в конфиг). Также нет `StringSetting` в `serialize()`.
- `Setting`: `mHideOnClickGUI` → `mHideInMenu`; у тебя в `Setting.hpp` добавлены поля анимаций (`knobBounce`, `clickScale`, `enumHoverCache`, `prevSliderEase`) — их использует твой `ClickGui`/`ModernDropdown`.
- `Module`: у тебя поля анимаций (`toggleBurst`, `hoverAnim`, `togglePulse`) — тоже нужны твоему GUI.

### 5.5 GUI

`26.62/Features/GUI/` содержит `ModernDropdown` от Aeternum (59 КБ) против твоего (75 КБ, 2 438 отличающихся строк) и `ScriptingGui`. Твоих `ClickGui`, `ChatBindsPanel`, `CombatantGui` — **нет** (но `ClickGui` в 26.62 есть в другом виде, в `Modules/Visual`).

То есть фактически придётся выбирать: тащить твой GUI целиком или жить на GUI новой базы.

---

## 6. Объём работы по сигнатурам и оффсетам

| | Твоя база (1.21.44) | 26.62 (Dexko) |
|---|---|---|
| Уникальных имён `DEFINE_SIG` | 116 | 94 |
| Из них активных (не закомментированы) | 93 | 67 |
| Пересечение по именам | **70** | |
| Есть только у тебя (нужно искать заново) | **46** | |
| Определений в `OffsetProvider.hpp` | 108 | 133 |

46 имён сигнатур, которые есть в твоём коде, но отсутствуют в новой базе:

```
Actor_vtable, BaseActorRenderer_renderText, BlockSource_getBiome,
BoneOrientation_computeBoneTransform, CameraYaw, CameraYaw2,
ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil,
ClientInstance_updateScreenSizeVariables, ColorTint_set,
ContainerScreenController_onContainerSlotHovered,
ContainerScreenController_tryBeginTransition,
ContainerValidation_handlePlace_jnz, ContainerValidation_handlePlace_test,
ContainerValidation_handleTransfer_jnz, Dimension_getBrightnessDependentFogColor,
Font_drawTransformed, GeneralSettingsScreenController_ctor, HitResult_getEntity,
HudCursorRenderer_render, HurtColor, ItemEntityRenderer_render,
ItemInHandRenderer_renderItem, ItemRenderer_getAtlasIconInfo,
ItemRenderer_getIconUV, ItemRenderer_iconBlit, ItemRenderer_renderGuiItem,
ItemRenderer_renderGuiItemNew, ItemStack_fromDescriptor,
ItemStack_getDamageValue, ItemStack_isEnchanted,
LevelRendererPlayer_renderOutlineSelection,
MeshHelpers_renderMeshImmediately, MeshHelpers_renderMeshImmediately2,
MinecraftGame_onResumeWaitReloadActors, Options_getSensitivity, RaknetTick,
ScreenRenderer_blit, SettingsScreenOnExit, SettingsScreenOnExit_Patch,
Tessellator_begin, ThirdPersonNametag, UIControl_getPosition,
UpdatePlayer, blockHighlightColor, mce_Color_BLACK, mce_RenderMaterialGroup_ui
```

Важно: это не значит «46 сигнатур руками». Часть нужна только твоим уникальным модулям. Если модуль не переезжает — сигнатура не нужна. В твоём `docs/UPDATE_1.21.70_RU.md` этот процесс описан правильно, методика рабочая.

---

## 7. Три возможных пути

### A. 26.62 как ствол — переносим мои модули наверх
Берём `26.62/`, доводим до сборки, потом по одному тащим 49 твоих модулей + GUI + твои правки в SDK.

- ➕ современная база, Luau, IRC, все её новые модули, меньше легаси;
- ➖ 49 модулей надо адаптировать (у многих нет зависимостей в новой базе — `ItemTextures`, `ItemRenderer`, `HealthTracker`…), твой GUI скорее всего придётся выкинуть или переписать. Потеря функционала почти гарантирована.

### B. Твой Solstice как ствол — вливаем только версионный слой
Оставляем твои 140к строк, меняем `SDK/` (структуры), `SigManager`, `OffsetProvider`, `PacketID`, `ComponentHashes`, хуки-обёртки.

- ➕ все 136 модулей и твой GUI остаются как есть, минимум переписывания логики;
- ➖ 46 сигнатур надо снять самому (у Dexko их нет), SDK-структуры новой базы тоньше — придётся вручную переносить недостающие структуры из старой базы и обновлять оффсеты. Это работа с IDA/Ghidra.

### C. Гибрид (рекомендую)
Ствол — **твой Solstice**. Из `26.62/` забираем:
- целиком `SDK/` (структуры, компоненты, пакеты) как основу, а из твоей базы доклеиваем то, чего там нет;
- `SigManager.hpp` / `OffsetProvider.hpp` как стартовые таблицы (потом перепроверить 46 недостающих);
- новые хуки, если нужны (NetherNet, SetupShaderParameter, GrabMouse, WndProc);
- `PacketID.hpp`, `ComponentHashes.hpp`, `EnttHashs.hpp`.

Твой `Features/` (модули + GUI) не трогаем вообще, кроме правок там, где поменялся API (`getKeyboardSettings`, `hasCustomData`, `mHideOnClickGUI`, `getStatusFlag`).

Это стандартный способ обновления таких клиентов: версия живёт в SDK-слое, логика модулей от версии почти не зависит.

---

## 8. Что понадобится в любом случае

1. **Собираемый `26.62/`** — снять 13 конфликтов, убрать дубль `Solstice.cpp`/`Dexko.cpp`, определиться с именем класса и брендом.
2. **Exe 26.62** (или дамп) для перепроверки сигнатур: база снята на 26.51/1.21.131, а не на 26.62.
3. Решение по GUI: твой (`ClickGui`, `ModernDropdown`, `ChatBindsPanel`, `CombatantGui`) или из новой базы.
4. Решение по Luau/IRC: нужны или вырезаем (в 26.62 они недоделаны — вызовы есть, менеджера нет).

---

## 9. Ограничение со стороны инструмента

Я работаю в Linux-песочнице: MSVC, `cmake`, `fxc.exe` и Windows SDK тут нет, поэтому **собрать `.dll` и проверить компиляцию я не могу**. Что могу делать: статический мерж деревьев, разрешение конфликтов, правка кода под новый API, генерация списков «что сломается», подготовка патчей, которые ты собираешь у себя одной командой.

---

## 10. Итог миграции (обновлено 2026-11-06)

Выбран **путь A**: ствол `26.62/`, модули и GUI из `src/` перенесены наверх.

### 10.1 Что получилось

| Показатель | 1.21.44 (`src/`) | 26.62 сейчас |
|---|---|---|
| Файлов `.cpp/.hpp` | 1083 | 814 |
| Строк | ~140 000 | ~101 000 |
| Модулей зарегистрировано | 198 | **198** |
| Сигнатур | 116 | 102 |
| Команд | — | 22 |
| Хуков | — | 30 |

Перенесены все модули старой базы, у которых есть код. Не перенесены только те,
у которых в `src/` лежат **пустые файлы** (0 строк): `RegenRecode`, `ModuleToggle`,
`NoPause`. Это не потеря — переносить было нечего.

### 10.2 Что пришлось дописать в SDK под перенесённый код

- `Item::getMaxDamage()` — vtable[35].
- `ItemStack::{getMaxDamage, getRemainingDurability, getDurabilityPercent, isArmorEnchantable, hasDurability}`.
- `ContainerScreenController::_tryBeginTransition` + сигнатура.
- `Actor::{getHunger, getOffhandContainer}`, null-safe `getStatusFlag/setStatusFlag`.
- Полный `enum AttributeId` (новая база была урезана до `Health=5, Absorption=12`;
  старые значения отличаются — `Health=7`, `Absorption=14`).
- `ItemIconQueue` + кэпчер `ItemRenderer` в `ItemRendererHook` и отрисовка через
  `renderGuiItemNew` в `SetupAndRenderHook` — без этого `InventoryHUD` не рисует иконки.
- `StringUtils::encode/decode` — нужны `IrcClient`/`WorkingVM`.
- `ClientPack`/`CubemapGenerator` — без них не собирался `SkyPack`.

### 10.3 Скрипты (Luau)

`ScriptManager` не существовал ни в одной базе — все вызовы были закомментированы.
Написан с нуля:

- один `lua_State` на скрипт, поэтому битый скрипт не роняет остальные;
- `Scripts\Modules\*.luau` возвращают таблицу с `onEnable/onDisable/onTick/onRender`;
- `Scripts\Commands\*.luau` регистрируются через `registerCommand(name, desc, fn)`;
- переключение скрипта эмитит `ModuleScriptStateChangeEvent`, поэтому `ToggleSounds`
  и `Notifications` реагируют так же, как на C++-модули;
- `.scripts list|reload [name]|enable|disable <name>`;
- окно скриптов в ClickGui по **F6**;
- примеры: `26.62/src/Features/Scripting/examples/`.

### 10.4 Известные риски (проверять в игре)

1. **`AttributeId`** — значения восстановлены по старой базе. Если Health-хуки
   ведут себя странно, смотреть сюда в первую очередь.
2. **`LoopbackPacketSender`** — новая база добавила `virtual void sendTo(void*)`
   между `send` и `sendToServer`, поэтому `sendToServer` съехал с `vtable[3]` на
   `vtable[4]`. Хук стоит на `vtable[2]` (`send`) — это безопасно, но обход
   `sendToServer`-хуков (`PacketUtils` bypass) на 26.62 может не срабатывать.
3. **Reach** — активный путь перенаправляет обе rip-константы в свой буфер, то есть
   является полной подменой. Мёртвая detour-ветка удалена. Потолок `Combat Reach`
   поднят с 7 до 20.
4. **Сигнатуры** — сняты с клиента не 26.62, а более позднего/раннего; дампа нет,
   перепроверить нечем. Пустой лог `[signatures] failed to find …` = норма,
   соответствующий модуль просто не включится.

### 10.5 Сборка

```bash
cmake -S 26.62 -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Solstice -- -m
```

Luau собирается как часть решения (`LUAU_BUILD_CLI`/`TESTS`/`WEB` = OFF).
В линковке добавлены `windowsapp.lib` и `onecore.lib` (WinRT нужен для IRC,
VoiceChat, D3DHook, D2D).
