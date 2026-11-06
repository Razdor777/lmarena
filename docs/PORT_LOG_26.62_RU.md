# Журнал переноса в 26.62

Хронология по шагам: что перенесено, что пришлось менять под новый SDK и что
осталось. Коммиты — в `git log --oneline`.

## 0. База и имя

- Конфликтные маркеры сняты в 11 файлах корня и 13 файлах `26.62/`,
  `resolve()` живёт в `Utils.hpp`/`MemUtils.hpp`.
- Клиент называется `Solstice`: `Solstice.cpp/.hpp`, `project(Solstice)`,
  `SolsticeShaders`, макросы `SOLSTICE_VERSION`, `DEXKO_BUILD_*` → build-info.
  Скелет `dexko/` не тронут.

## 1. SDK-слой (`26.62/src/SDK`)

Восстановлено то, что было в 1.21.44 и пропало в 1.26.x:

| API | Как сделано |
|---|---|
| `Actor::getOffhandContainer/getHunger/setGameType` | обычные методы + `OffsetProvider` |
| `Item::mArmorItemType 0x244`, `mProtection 0x264`, `getMaxDamage` (vtb 35), `isBlockItem` (vtb 15) | поля и виртуальные слоты |
| 7 методов `ItemStack` | по vtable/полям |
| `Level::getBlockPalette()` | inline `callVirtualFunc(1, this)` — сигнатуры в 26.62 нет |
| `GameMode::useItem(ItemStack*)` | алиас на `baseUseItem` |
| `ItemStack::fromDescriptor` | объявление + `ItemStack.cpp` + `SigManager::ItemStack_fromDescriptor` |
| `ContainerScreenController::_tryBeginTransition` | объявление + сигнатура |
| `UpdatePlayerGameTypePacket` | вернули (нужен `SpectatorDetector`) |
| `SettingType::String` в `Setting::parse` | строковые настройки теперь грузятся из конфига |

`StringSetting`, `hasCustomData/serializeCustomData/deserializeCustomData`
добавлены в `Module`/`ModuleManager`.

## 2. Combat (первым, по приоритету)

- 13 модулей, 26 файлов: Aura, Reach, Criticals, HitBoxes, InfiniteAura,
  InfiniteChestAura, TriggerBot и остальные.
- Правки под новый SDK: `attack(Actor&)`, `buildBlock(pos, face, handSlot, simTick)`,
  `mEntityIdentifier`, `static` → анонимный namespace, `SubChunk` с новыми полями
  (`mBlocks[2]`/`mBlocksReadPtr[2]`/`mAbsoluteIndex`).
- Подробности и что проверить в игре — `docs/COMBAT_PORT_26.62_RU.md` §9–§10.

## 3. Остальные категории

Перенесены все модули, каждый зарегистрирован в `ModuleManager` (200 штук):

- **Player** — 36 модулей + `CPSTracker`, `HealthTracker`, `PlayerTracker`, `Schematic`.
  Правки: `getmSelectedSlot()` → `mSelectedSlot`, `getmLegacy()` → `mLegacy`.
- **Movement** — 29 (включая `AutoPatrol`, `AutoSneak`, `NoClip`).
  Попутно: `SafeWalk` восстанавливал первый патч чужими байтами
  (`mOriginalData2` вместо `mOriginalData`), `AutoPath` звал несуществующий
  `getmMaterial()`.
- **Misc** — 48.
- **Visual** — 73.
- **Команды** — 12 новых (schematic-набор, pos1/pos2, spammer, fakechat, skin,
  playeroffset), всего 34.

## 4. Полнота: где база была «беднее» корня

В `26.62` часть модулей лежала заглушками или урезанными. Заменены полными
версиями из корня (17 модулей):

| Модуль | Было в 26.62 | Стало |
|---|---|---|
| `CustomChat` | 283 строки | 1005 |
| `Nametags` | 244 | 856 |
| `ClickTp` | 29 (без `customData`!) | 544 |
| `Scaffold` | 35 настроек | 64 настройки |
| `BlockESP` | 23 | 40 |
| `RobloxCamera` | 7 | 24 |
| `TargetHUD` | 20 | 36 |
| `LevelInfo` | 25 | 40 |
| `JumpCircles` | 6 | 13 |
| `ESP`, `Tracers`, `Arraylist`, `Velocity`, `DestroyProgress`, `NameProtect`, `Glint`, `OreMiner` | меньше | полные |

Плюс сервисные: `ExceptionHandler` (97 → 784 строки: отчёт о крашах с чужим
стеком, адресом обращения, последними строками лога), `ConfigManager`
(бэкап битых конфигов вместо тихого падения), `ChatUtils` (+`stripColorCodes`,
`extractNickFromTag`, `extractRankFromTag`, `canonicalizeRankKey`).

## 5. GUI и внешний вид

- Новый ClickGui: `ModernDropdown.cpp` 1516 строк (layout → input → draw,
  поиск, тултипы, пикер цвета, чат-бинды) + `CombatantGui`.
- `StringSetting` рисуется и редактируется: высота строки + невидимое
  ImGui-поле ввода поверх строки (ImGui ловит клавиатуру, рисуем сами).
- Живой градиент темы: `ColorUtils` (345 строк) + поля `Interface`
  (`mGradientFlow`, `mFlowDepth`, `mColorWave`, `mWaveSpeed`, `mWaveSpacing`,
  `mPulse*`, `mShimmer`, `mGlowBoost`). Палитры — `Aurora/Abyss/Ember/Blossom/Prism`.
- Шрифт один — `Mntsb`, `Roboto` как донор кириллицы; `FontHelper` переписан.
- `tools/gen_resources.py` пересобирает `Resources.hpp/.cpp` из
  `26.62/resources` (иначе `mntsb`/`Roboto` не попадали в атлас).

## 6. Хуки и события

Добавлены (детур ставится только если сигнатура живая — иначе в лог уходит
`signature not resolved`, игра не падает):

- `HealthHooks` → `EntityHurtEvent` (раньше адреса `base+0x23A09B0` жили один билд).
- `ProjectileHooks` → `ProjectileHitEvent` (один детур, тип попадания в `HitResult::mType`).
- `HurtColorHook` → `HurtColorEvent` (`SigManager::HurtColor`, RefSig).
- `AmbienceHook` → небо/туман/облака/солнце (пять адресов заменены заглушками `= 0`).
- `ContainerScreenControllerHook` → второй детур `_handleSlotHovered`
  (`ContainerSlotHoveredEvent`, `sHoveredInvSlot`) для `JavaInventory`.

Новые события: `ProjectileHitEvent`, `HurtColorEvent`, `ContainerSlotHoveredEvent`.

## 7. Попутные поломки, которые нашлись и исправлены

- `WndProcHook`: повторный `SetWindowLongPtr` в `WM_SIZE`/`WM_DPICHANGED`
  возвращал наш же обработчик → `CallWindowProc` уходил в рекурсию. Теперь
  `mDetour` сохраняется только если это чужой WndProc, `shutdown` не пишет
  `nullptr`.
- `SafeWalk`: см. выше.
- `ScriptingGui` удалён — он обращался к `ScriptManager`, которого в 26.62 нет
  (скриптовый GUI остался только как `Luau` в `include/`).
- `Interface::onLevelRenderEvent` был объявлен и подписан в конструкторе, но тела
  не было (его потеряли, когда .cpp перезаписывали корневым) — падала линковка на
  `&Interface::onLevelRenderEvent`. Вернули; заодно включили обработчик
  `DrawImageEvent` (ховер-детур `drawImage` в 26.62 вешается, а слушателя не было)
  и удалили мёртвый `BodyYaw` (модель считается покадрово в `onActorRenderEvent`).
- `D3DHook`: детур 5-аргументного `ResizeBuffers` писался в `oResizeBuffers1`
  (7 аргументов) — при первом ресайзе окна вызов уходил с мусором в аргументах;
  `s_shutdown()` снимал оба метода сразу (второй может быть невалиден). Теперь
  ветки разделены флагом `isd3d12`.
- `ExceptionHandler.cpp` перенесли раньше, чем его заголовок: `markThread`/
  `ThreadRole` определялись в .cpp, но отсутствовали в `hpp`. Заголовок вернули.
- `ItemRendererHook` в 26.62-версии не запоминал `ItemRenderer`/`BaseActorRenderContext`,
  а `InventoryHUD` их требует (`getItemRenderer()`/`getBarContext()`) — модуль не
  компилировался. Плюс вернули `ItemIconQueue` и сам рендер иконок движком в
  `SetupAndRenderHook` (`renderGuiItemNew`), иначе очередь заданий никто не
  опустошал и иконки в HUD не рисовались.
- `BaseTickInitEvent` пропал при переносе событий, хотя `RobloxCamera` его
  слушает. Вернули класс и одноразовый `trigger` в `BaseTickHook`.
- Вывод клиентских сообщений (`BaseTickHook`) больше не обходит `ChatUtils::sSink`:
  при включённом `CustomChat` текст уходил мимо него.
- `KeyHook` не был в списке `HookManager`, хотя `WndProcHook` зовёт
  `KeyHook::onKey()` по `WM_KEYDOWN/UP` — без него не приходили `KeyEvent`,
  глобальные клавиши модулей и чат-бинды.
- 17 имён `SigManager::*` (`ItemRenderer_getIconUV/iconBlit/renderGuiItemNew/
  getAtlasIconInfo`, `HudCursorRenderer_render`, `Actor_setPosition` и др.) и 4
  поля `OffsetProvider` вызывались из кода, но в заголовках отсутствовали —
  это ошибка компиляции, а не «сигнатура не нашлась». Объявлены нулями по
  правилу проекта (паттерны ждут скана, см. `docs/SIGS_26.62_RU.md`).
- Две мёртвые настройки (`Regen::mChecker`, `PotionHUD::mColorBars`) —
  объявлены, но не регистрировались и не читались; удалены.
- `FileUtils::getsolsticeDir()` (опечатка в регистре) → `getSolsticeDir()`:
  `OreMinerV2::debugStart()` не компилировался.
- `StringUtils` и `StackWalker` в 26.62 были урезаны (24 и 130 строк). Догнали
  корневые: `StringUtils` 645 строк (`encode/decode/encrypt/decrypt/
  containsIgnoreCase/RemoveColorCodes` — их зовут `IrcClient`, `AutoCosmetic`,
  `AutoDodge`), `StackWalker` 192 строки (`describe`/`symbolStatus` нужны
  новому `ExceptionHandler`, иначе краш-репорт не собирался).
- Из текстов убран бренд `Juzdex` (в папке, чате, дампах) — везде `Solstice`.
  Функция `FileUtils::getJuzdexDir()` оставлена алиасом на `Solstice\`.

## 7a. Первая настоящая сборка (MSVC, unity)

Сборка под Windows выявила пять дефектов, которые статика до этого не видела, —
все закрыты:

| Что было | Почему прорвалось | Как исправлено |
|---|---|---|
| `OreMinerV2.hpp`: две секции «Debug dump» (новый API + устаревший блок `DebugRow{deepslate}`, свои `mDebugCsvPath/mDebugCoordsPath/mDebugStartedAt`) | при слиянии модуля в заголовок попали оба варианта — повторное объявление (C2011), за ним каскад C2027/C2079/C2665 в чужих файлах | устаревший блок удалён (26 строк), в шапке секции — предупреждение; теперь `DebugRow/debugStart/debugFlush/debugStop/oreTypeName` объявлены по одному разу |
| `ContainerActions.cpp` (фаза Drop all): `armor->mValid_DeprecatedSeeComment = false` | поле живёт в `ItemStackBase`, а `ItemStack` начинается с `PAD(24)` → C2039 | `armor->ItemStackBase::mValid_DeprecatedSeeComment = false` |
| `ModuleManager.cpp`: `catch (const nlohmann::json::exception&)` дважды | `json::exception` наследует `std::exception`, ветка после него недостижима (C4286) | обе ветки убраны; на инициализацию и enable/disable не влияет |
| `BuildInfoCommand.cpp`: `X != ""` для строкового макроса, `STRING(...)` | сравнение массива с указателем (C5056); макрос `STRING` не виден без `StringUtils.hpp` | `std::string(X) != ""`, `std::to_string(...)` |
| `GenerateBuildInfo.cmake` публиковал только `DEXKO_*` | `BuildInfoCommand`/`ExceptionHandler` ждут `SOLSTICE_*`, `Solstice.cpp` — `DEXKO_*` | генератор пишет оба набора имён из одних значений |

Плюс шесть своих заголовков (`Authorization`, `CustomCryptor`, `SnipeCommand`,
`VclipCommand`, `WindowResizeEvent`, `CameraInstructionPacket`) включались из
нескольких единиц трансляции **без** `#pragma once` — при `UNITY_BUILD`
(в `26.62/CMakeLists.txt:242`, ~32 `.cpp` на единицу) это прямой путь к C2011.
Стражи добавлены.

Урок на будущее: класс ошибок «два объявления в одном классе» и «два
одинаковых `static` в одной unity-единице» статикой не ловился вообще — под
него сделаны отдельные анализаторы (см. §8).

## 8. Проверки (без компилятора)

```bash
bash tools/check_all.sh          # все 10 проверок, код возврата = результат
bash tools/check_all.sh --fix    # + пересобрать Resources.* и docs/SIGS_26.62_RU.md
```

По отдельности — `tools/`: `check_includes`, `check_members`, `check_undefined`,
`check_event_visibility`, `check_sig_names`, `check_method_case`, `check_enums`,
`check_duplicates`, `check_unity_collisions`, `check_header_guards`; отчёты —
`sig_checklist.py`, `sdk_compat.py`, `gen_resources.py`.

Состояние на последнем прогоне: includes 0, members 0/836, undefined 0/482,
event-visibility 0, sig-names 0, method-case 0, enums 0, duplicates 0,
unity-collisions 0/354, header-guards 0/482.

## 9. Что осталось

1. **Сборка на MSVC идёт** (первый прогон разобран в §7a). Нужен следующий лог:
   ошибки следующего слоя — линковка, шаблоны, D3D-детуры. Компилятора в этой
   среде нет, поэтому цикл такой: правки → прогон → лог сюда.
2. Доснять сигнатуры по `Minecraft.Windows.exe` 1.26.5x: шесть «догадок» и пять
   `Ambience_*` (порядок — в `docs/SIGS_26.62_RU.md`).
3. Проверить в игре: чат-бинды, hover-свопы в инвентаре, `.schematic`, `Ambience`,
   `DamageLog` (нужна `Actor_hurt`), `BowHitMarker` (нужна `Projectile_onHit`),
   иконки предметов в `InventoryHUD` (рисует движок через `renderGuiItemNew`)
   и ресайз окна (был краш в `D3DHook::resizeBuffers`).
4. `ModuleScriptStateChangeEvent` никто не шлёт: скриптового движка в 26.62 нет.
   Обработчик в `ToggleSounds` безопасен, но и не срабатывает — вернуть вместе
   со скриптами.
