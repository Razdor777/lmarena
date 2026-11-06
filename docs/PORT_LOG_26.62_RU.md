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
- Из текстов убран бренд `Juzdex` (в папке, чате, дампах) — везде `Solstice`.
  Функция `FileUtils::getJuzdexDir()` оставлена алиасом на `Solstice\`.

## 8. Проверки (без компилятора)

```bash
python3 tools/check_includes.py 26.62/src        # 0 unresolved
python3 tools/check_members.py --vocab-exclude Features -- 26.62/src   # 0 unresolved
python3 tools/sdk_compat.py --out docs/SDK_COMPAT_26.62_RU.md
python3 tools/sig_checklist.py --out docs/SIGS_26.62_RU.md
python3 tools/gen_resources.py --check
```

## 9. Что осталось

1. **Собрать на MSVC/Windows SDK** — это единственная проверка, которой не хватает.
2. Доснять сигнатуры по `Minecraft.Windows.exe` 1.26.5x: шесть «догадок» и пять
   `Ambience_*` (порядок — в `docs/SIGS_26.62_RU.md`).
3. Проверить в игре: чат-бинды, hover-свопы в инвентаре, `.schematic`, `Ambience`,
   `DamageLog` (нужна `Actor_hurt`), `BowHitMarker` (нужна `Projectile_onHit`).
