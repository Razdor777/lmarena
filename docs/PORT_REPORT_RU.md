# Порт Solstice 1.21.44 → база 26.62 (1.26.62): что сделано

Ветка: `arena/b26c371c-lmarena`. Корень `src/` остаётся эталоном 1.21.44,
целевое дерево — `26.62/`. Всё, что можно было перенести без запуска игры,
перенесено; всё, что требует бинарника, помечено `UNVERIFIED`.

## 1. Итог в цифрах

| | корень (1.21.44) | 26.62 (после порта) |
|---|---|---|
| модули (`Features/Modules`) | 198 | 200 |
| зарегистрировано в `ModuleManager` | 137 | **178** |
| команды | 30 | **29** (+`LuaCommand` закомментирована) |
| хуки (`ADD_HOOK`) | 23 | **30** |
| утилиты (`Utils/`) | 49 | 50 |
| события (`Features/Events`) | 32 | 35 |
| активные сигнатуры (`SigManager`) | 97 | 88 |
| оффсеты (`OffsetProvider`) | 104 | 74 |
| строк `.cpp`+`.hpp` | 139 995 | 101 868 |

Сверка по именам файлов: **модули 0 расхождений, команды 0, утилиты 0,
события 0, хуки 0.** Всё содержимое корня присутствует в 26.62.

Только в 26.62 (не переносить обратно): `Misc/IRC`, `Movement/AutoWalk`,
`Player/AutoEat`, события `ClientTickEvent` / `LevelRenderEvent` /
`SetupShaderParameterEvent`, Luau-скриптинг.

## 2. Что именно перенесено

- **75 модулей** (147 файлов) — все, что были только в корне.
- **12 команд**: `FakeChat`, `PlayerOffset`, `SCopy`, `SPaste`, `SBuild`,
  `SCancel`, `Spammer`, `Skin` (header-only, раньше не регистрировалась нигде).
- **13 утилит**: `CPSTracker`, `HealthTracker`, `PlatformTracker`, `ItemAtlas`,
  `ItemTextures`, `ClientPack`, `CubemapGenerator`, `PngWriter`, `NurikIcons`.
- **5 событий**: `ContainerSlotHoveredEvent`, `EntityHurtEvent`,
  `HurtColorEvent`, `ProjectileHitEvent`, `SetupAndRenderEvent`.
- **4 хука**: `HealthHooks`, `ProjectileHooks`, `AmbienceHook`, `HurtColorHook`.
- **`Features/ChatBinds/*`** + `GUI/ChatBindsPanel` + `GUI/CombatantGui`.
- **`SDK/Minecraft/Rendering/ItemRenderer.hpp`**, **`TexturePtr.hpp`**,
  **`UpdatePlayerGameTypePacket.hpp`**.
- **`Features/Modules/ModuleToggle.hpp`** — переключение модулей в потоке игры
  (без гонок с фоновым потоком).

### Overlays, замененные на корневые (корень — надмножество)

| файл | было в 26.62 | стало |
|---|---|---|
| `Utils/StringUtils.*` | 645 строк, 12 методов | 949 строк, 28 методов |
| `Utils/GameUtils/ChatUtils.*` | 45 строк | 172 строки (+`stripColorCodes`, `extractNickFromTag`, `sSink`) |
| `Utils/MiscUtils/ColorUtils.*` | 55 строк | 345 строк (+`getStaticAccentColor`, HSL-хелперы) |

Без этого портированные модули не компилировались: они вызывали
`StringUtils::trim/startsWith/containsIgnoreCase/replace/...`,
`ColorUtils::getStaticAccentColor`, `ChatUtils::stripColorCodes`.

### ClickGui: стиль Combatant

В 26.62 был только `Modern`. Добавлено:
`ClickGuiStyle::Combatant`, настройка `UI Scale` (`mUiScale`),
`static CombatantGui combatantGui`, вызовы `onOpen/onClose/onWindowResize/onKey`
и ветка рендера. `CombatantGui::onKey` получает клавиши раньше `Modern`,
как в корне.

### KeyHook: интеграция CustomChat / ChatBinds

В 26.62 хук был **выключен** (`//ADD_HOOK(KeyHook)`), потому что
`SigManager::Keyboard_feed` закомментирована. Теперь:

- хук включён;
- перенесён корневой порядок «ImGui получает клавиши первым» (ToUnicode,
  а не ToAscii — иначе кириллица ломается);
- добавлены `ChatBinds::handleKey()` и `ChatBinds::flushPending()`;
- `oFunc` берётся только если `mDetour` создан: **пока сигнатура
  `Keyboard_feed` не подтверждена для 1.26.62, хук работает в режиме
  «ImGui + модули без прокидывания в игру»**, клиент не падает.

## 3. Починенный SDK

Добавлено в 26.62 (перенесено из корня, не зависит от версии игры):

- `ItemStack`: `getDamage`, `getMaxDamage`, `getRemainingDurability`,
  `getDurabilityPercent`, `hasDurability`, `isArmorEnchantable`.
- `Item`: `getMaxDamage` (vtable 35), `isBlockItem` (vtable 15) — **UNVERIFIED**.
- `Actor`: `getOffhandContainer()`, `getHunger()`; `AttributeId::PlayerHunger`.
- `ContainerScreenController::_tryBeginTransition`.
- `OffsetProvider`: `ClientInstance_mFovX` = `0x6F0`,
  `ClientInstance_mFovY` = `0x704` (взято из комментариев 26.62;
  в 1.21.44 было `0x728` / `0x73C`).
- `ContainerScreenControllerHook` — полная корневая версия: второй детур
  `_handleSlotHovered`, `sHoveredInvSlot`, `sScreenActive`, `isScreenActive()`,
  триггер `ContainerSlotHoveredEvent`. Это то, что нужно `JavaInventory`.

## 4. Починенная сборка

- `SOLSTICE_BUILD_VERSION/SHORT/BRANCH/COMMIT_MESSAGE` и
  `SOLSTICE_FILES_CHANGED_COUNT` → **`DEXKO_*`** (генератор эмитит только
  `DEXKO_*`; 11 мест в 4 файлах).
- `ModuleManager.cpp`: 178 регистраций, каждая с `#include`; `NoPause`
  исключён (файлы-заглушки по 0 байт), `Schematic` — вспомогательный заголовок.
- `OreMinerV2.hpp`: удалён дублирующий блок «Debug dump» (повторные
  `struct DebugRow`, `mDebugRows`, `mDebugWritten`, `mDebugWasOn`,
  `debugStart/debugStop/debugFlush` — ошибка компиляции).
- Маркеров конфликтов: **0** во всём `26.62/`.

## 5. Что нельзя проверить без игры

Все паттерны ниже сняты с **1.21.44** и помечены `// UNVERIFIED` в
`26.62/src/SDK/SigManager.hpp`. Их надо перепроверить на 1.26.62:

| сигнатура | кто использует | если не найдётся |
|---|---|---|
| `HudCursorRenderer_render` | `Visual/CustomCrosshair` | прицел не рисуется |
| `ClientInputCallbacks_..._setNoBlockBreakUntil` | `Misc/ItemUseDelayFix` | задержка использования предметов |
| `ContainerScreenController_tryBeginTransition` | `Player/ContainerActions` | действия с контейнером |
| `ContainerScreenController_onContainerSlotHovered` | `JavaInventory`, `CombatantGui` | ховер-свап отключается сам |
| `HurtColor` | `Visual/HitColor` | цвет вспышки урона не меняется |
| `ItemRenderer_getAtlasIconInfo` / `getIconUV` / `renderGuiItemNew` / `iconBlit` | `ItemAtlas`, `ItemTextures`, `ArmorHUD`, `InventoryHUD`, `PotionHUD` | иконки предметов в HUD не рисуются |
| `Item::getMaxDamage` (vtable 35), `isBlockItem` (vtable 15) | `ArmorHUD`, `AutoArmor`, `RegionFill` | прочность/броня показываются неверно |
| `ClientInstance_mFovX/Y` = 0x6F0/0x704 | `ClientInstance::getFov` | FOV считается неверно |

Дополнительно перенесены 10 сигнатур, которые база 26.62 держала
закомментированными, а код на них ссылался (без них сборка падала):

`ActorRenderDispatcher_render`, `ConnectionRequest_create_CurrentInputMode`,
`FastEat`, `FireRender`, `InputModeBypass`, `InputModeBypassFix`,
`ItemInHandRenderer_renderItem_bytepatch`, `ItemPositionConst`,
`Keyboard_feed`, `mce_framebuilder_RenderItemInHandDescription_ctor`.

Итого помечено `// UNVERIFIED (1.21.44)` — **19 сигнатур**.
Ключевое: **все эти места деградируют, а не падают** — при `addr == 0`
хук/фича сами себя выключают с `spdlog::warn`.

## 6. Как проверять на своей машине

1. Собрать **Debug** (`CMakePresets.json`, Ninja + cl.exe).
2. В логе искать:
   - `[signatures] failed to find` — каких паттернов нет;
   - `[offsets] failed to find` — каких оффсетов нет;
   - `[KeyHook] Keyboard_feed не найдена` — хук клавиатуры в безопасном режиме.
3. Запустить в игре и пройти чек-лист:
   - `.help` — команды живы (29 штук);
   - ClickGui: `Style` → `Modern` и `Combatant` — оба должны открываться;
   - `.skin`, `.sbuild`, `.spos1/.spos2` — новые команды;
   - ESP/Tracers/Nametags — рендер и матрицы;
   - HitColor/HurtColor — вспышка урона;
   - InventoryHUD/ArmorHUD/PotionHUD — иконки предметов;
   - CustomCrosshair — прицел;
   - JavaInventory — ховер-свап (нужен успешный `ContainerScreenController_onContainerSlotHovered`).
4. Найденный правильный паттерн вписать в `SigManager.hpp` и убрать
   `// UNVERIFIED`.

## 7. Инструменты

- `tools/sig_audit.py` — сверка `SigManager`/`OffsetProvider` двух деревьев
  и индекса использования. Результат: `docs/SIGNATURE_TODO_RU.md`,
  `docs/signatures.json`.
- `tools/resolve_conflicts.py` — отчёт/разрешение маркеров конфликтов
  (`report`, `resolve <tree> <head|other|union> [--write]`).
