# Миграция Solstice 1.21.44 → 1.26: что сделано, что обновлено, что осталось

Дата сверки: заголовки LeviLamina `main` (коммит `b3ff77f2`, версия 26.51, `bedrockdata 26.51.1`).
Источник истины по символам — клон LeviLamina (`/home/user/LeviLamina`), а не догадки.

## Что сделано

1. Написан `tools/symbol_audit.py` — он вытаскивает из проекта все цели хуков
   (`Detour>("Class::method"`) и все записи `src/SDK/OffsetProvider.hpp`, затем ищет каждое имя
   в заголовках LeviLamina и раскладывает по статусам.
2. Сгенерированы отчёты:
   - `audit.md` — таблицы со статусами и примечаниями;
   - `symbol-map.csv` — то же самое в машиночитаемом виде.
3. **В исходниках расставлены пометки `// [1.26]`** (это только комментарии — сборку не ломают):
   - `src/SDK/OffsetProvider.hpp` — 61 пометка, каждая над своей записью;
   - 21 файл хуков в `src/Hook/Hooks/**` — блок пометок после инклюдов.

Пометки бывают трёх видов:

| Пометка | Значение |
|---|---|
| `// [1.26] ОБНОВЛЕНО: X -> файл:строка` | имя найдено в хидерах 1.26 — адрес/имя/типы можно брать оттуда |
| `// [1.26] ПРОВЕРИТЬ: X -> файл:строка` | имя найдено, но в другом классе — надо подтвердить, тот ли метод |
| `// [1.26] ОСТАЛОСЬ (...)`: X -> … | в хидерах нет — ручной реверс |

## Итог по цифрам

| Группа | Всего | Найдено в 1.26 | Осталось |
|---|---|---|---|
| Хуки (`src/Hook/**`) | 27 | **22** (81 %) | 5 |
| Сигнатуры/поля (`OffsetProvider.hpp`) | 61 | **27** (44 %) | 34 |
| **Итого** | **88** | **49** (56 %) | **39** |

Хуки закрыты хорошо, потому что имена функций Mojang стабильнее, чем layout классов.
Поля — хуже, и это ожидаемо: поля классов в хидерах LeviLamina описаны далеко не везде
(`TypedStorage` встречается в 7 540 файлах из ~25 000, из них клиентских — 2 071;
например, `ClientInstance.h` — всего 2 описанных поля, а `Level.h` — 128).

## Обновлено (можно брать из хидеров 1.26)

Хуки, по которым имя найдено (полный список с объявлениями — в `audit.md`):

| Было в 1.21.44 | Стало в 1.26 |
|---|---|
| `Actor::baseTick` | `src/mc/world/actor/Actor.h:1254` |
| `ActorAnimationControllerPlayer::applyToPose` | `.../ActorAnimationControllerPlayer.h:122` — **сигнатура изменилась** |
| `ActorRenderDispatcher::render` | `src-client/mc/client/renderer/actor/ActorRenderDispatcher.h:93` |
| `BlockSource::fireBlockChanged` | `src/mc/world/level/BlockSource.h:282` (+ `$fireBlockChanged:705`) |
| `ClientInstance::isPreGame` | `src-client/mc/client/game/ClientInstance.h:1755` (`$isPreGame`) |
| `ConnectionRequest::create` | `src/mc/network/ConnectionRequest.h:68` |
| `ContainerScreenController::tick` | `src-client/mc/client/gui/screens/controllers/ContainerScreenController.h:320` |
| `HoverTextRenderer::render` | `src-client/mc/client/gui/controls/renderers/HoverTextRenderer.h:60` |
| `ItemRenderer::render` | `src-client/mc/client/renderer/actor/ItemRenderer.h:245` |
| `LoopbackPacketSender::send` | `src/mc/network/LoopbackPacketSender.h:64` |
| `MinecraftUIRenderContext::drawImage` | `src-client/mc/client/renderer/screen/MinecraftUIRenderContext.h:299` |
| `MouseDevice::feed` | `src-client/mc/deps/input/MouseDevice.h:30` (альтернатива — `MouseInputEvent`) |
| `RakNet::RakPeer::GetLastPing / RunUpdateCycle / SendImmediate` | `src/mc/deps/raknet/RakPeer.h:788 / 887 / 615` |
| `ScreenView::setupAndRender` | `src-client/mc/client/gui/screens/ScreenView.h:319` — в 1.26 это `ScreenView::render` |
| `Mob::getCurrentSwingDuration` | `src/mc/world/item/Item.h:231` — `Item::getSwingDuration` |
| `bobHurt` | `src-client/mc/client/renderer/game/LevelRendererPlayer.h:365` |
| `entityHurt` | `Actor::_hurt` — `src/mc/world/actor/Actor.h:612` |
| `entityHealthChanged` | `onActorHealthChanged` — `IScriptWorldAfterEvents.h:127` |
| `projectileHitBlock` / `projectileHitEntity` | `onProjectileHitBlock` / `onProjectileHitEntity` — `IScriptWorldAfterEvents.h:341/344` |

Три хука вообще можно **выбросить**, потому что LeviLamina даёт событие:

- `KeyHook` (`Keyboard::feed`) → `ll::event::input::KeyInputEvent` + `ll::input::KeyRegistry::getOrCreateKey(...)`
- `MouseHook` (`MouseDevice::feed`) → `ll::event::input::MouseInputEvent`
- `SetupAndRenderHook` → `BeforeUIRenderEvent` / `AfterUIRenderEvent` (`ScreenView&`, `MinecraftUIRenderContext&`)

## Осталось (ручной реверс)

**Хуки (5):**

| Цель | Почему осталось |
|---|---|
| `CameraDirectLookSystemUtil::_handleLookInput` | класс переехал: теперь `CameraDirectLookComponent` / `CameraDirectLookDefinition` |
| `Unknown::renderNametag` | в 1.26 неймтеги — это `NameTagRenderObject` / `NameTagRenderer` (объекты рендера), а не функция |
| `mce::framebuilder::RenderItemInHandDescription::…` | структура в хидерах **пустая** (`struct … {};`) — layout неизвестен |
| `PacketHandlerDispatcherInstance<…>` | имя в `Detour(…)` обрезано; класс есть (`src/mc/network/PacketHandlerDispatcherInstance.h`) — уточнить шаблон |
| `Keyboard::feed` | как функции нет; заменяется событием LeviLamina (см. выше) |

**Поля/сигнатуры (34)** — например `ClientInstance_mLevelRenderer`, `ClientInstance_mPacketSender`,
`MinecraftGame_mClientInstances`, `LevelRendererPlayer_mFovX/mFovY/mCameraPos`,
`Actor_mSwinging`, `Actor_mHurtTimeComponent`, `LevelData_mTick`, `Bone_mPartModel`, `bgfx_*`.
Причины ровно две: либо класс в хидерах без описанных полей, либо поле переименовано/удалено.

## Как пользоваться

```bash
# просто отчёт
python3 tools/symbol_audit.py

# перерасставить пометки в исходниках (идемпотентно: старые [1.26] стираются)
python3 tools/symbol_audit.py --annotate

# если клон LeviLamina лежит в другом месте
python3 tools/symbol_audit.py --ll /path/to/LeviLamina --annotate
```

После обновления LeviLamina (новая версия игры) достаточно обновить клон и перезапустить скрипт —
таблица и пометки пересчитаются. Переименования, которые скрипт не знает, правятся
в словарях `SYNONYMS` и `NOTES` в начале `tools/symbol_audit.py`.

## Порядок дальнейших работ

1. Поднять клиентскую сборку LeviLamina 26.51 (или просто использовать её хидеры как дамп).
2. Заменить три хука (Key/Mouse/SetupAndRender) на события — минус три файла сразу.
3. Пройти по 22 хукам из таблицы «обновлено»: взять имя и **новую сигнатуру** из хидера,
   переписать тело там, где аргументы изменились (минимум — `applyToPose`).
4. Оставшиеся 5 хуков и 34 поля — реверс в IDA: это и есть реальный объём работы.
5. Логика модулей (`Features/Modules/**`), GUI и конфиги правится последней — она зависит от типов.

> Напоминание не по технике: LeviLamina просит не использовать её для программ,
> «compromising Minecraft's security» (`usage_guidelines.en.md`), а клиентская установка патчит
> `Minecraft.Windows.exe` на месте. Для серверных модов всё вышеописанное работает «в чистую».
