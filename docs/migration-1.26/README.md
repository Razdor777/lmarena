# Миграция Solstice 1.21.44 → 1.26: что сделано, что обновлено, что осталось

Дата сверки: заголовки LeviLamina `main` (26.51, `bedrockdata 26.51.1`).

## Коротко: что применено, а что размечено

| Слой | Что сделано | Статус |
|---|---|---|
| Хуки (27) + `OffsetProvider` (61) | **аудит с вердиктами по каждой цели**, пометки `// [1.26]` | размечено + разобрано |
| `MoveInputComponent` | **переписан под 1.26**, 98 мест вызова переправлены | **применено** |
| Все классы SDK | **`src/SDK/Generated/Offsets_1_26.hpp`** — смещения для 493 классов | **применено** |

## Итог по хукам и сигнатурам (88 целей)

| Вердикт | Сколько | Что значит |
|---|---|---|
| `FOUND` / `FOUND_THUNK` | **38** | имя есть в 1.26, адрес даст symdb |
| `RENAMED` | **14** | переименовано, новый символ найден и подтверждён |
| `MOVED` | **13** | переехало в другой класс/компонент или заменяется событием |
| `NO_LAYOUT` | **9** | класс есть, но поля не описаны → оффсет только из IDA |
| `GONE` | **13** | в 1.26 нет (удалено либо твои/чужие кастомные имена) → реверс |
| `EMPTY_CLASS` | **1** | структура в хидерах пустая |

**Разобрано и закрыто: 65 из 88 (74 %).** Осталось 23, и это не «я не посмотрел»,
а то, чего физически нет в симвлах (`MinecraftSim`, `Bone`, `UIProfanityContext`, поля `MinecraftGame`…).

## Как проверять — три уровня

### Уровень 1. По заголовкам (делаю я, бинарник не нужен)

Открыть хидер класса в `LeviLamina/src-client/mc/**` и посмотреть, есть ли член.
Так найдены и подтверждены, например:

| Было (1.21.44) | Стало в 1.26 (проверено по хидеру) |
|---|---|
| `ClientInstance::getBlockSource` | `ClientInstance::getRegion()` (`ClientInstance.h:475`, `$getRegion:1470`) |
| `ClientInstance::mLevelRenderer` | `getLevelRenderer()` (`:796`, `$getLevelRenderer:1787`) |
| `ClientInstance::mPacketSender` / `mGuiData` | `getPacketSender()` (`:994`) / `getGuiData()` (`:857`) |
| `ClientInstance::getInputHandler` | `getInput()` → `ClientInputHandler*` (`:1040`) |
| `Mob::getCurrentSwingDuration` | `Mob::getModifiedSwingDuration()` (`Mob.h:338`) — **не** `Item::getSwingDuration` |
| `LevelRenderer::mRendererPlayer` | `mLevelRendererPlayer` (`shared_ptr`, `LevelRenderer.h:125`) |
| `LevelRendererPlayer::mFovX/mFovY` | `mFov` + `mOFov` (`:131-132`) — один float, вертикальный FOV считается |
| `LevelRendererPlayer::mCameraPos` | `LevelRendererCamera::mCameraPos` (`LevelRendererCamera.h:263`) |
| `LevelData::mTick` | `mCurrentTick` (тип `Tick`, `LevelData.h:77`) |
| `GameSession::mEventCallback` | `getNetEventCallback()` / `mLegacyClientNetworkHandler` (`GameSession.h:30`) |
| `PlayerInventory::mContainer` | `mInventory` (`unique_ptr<Inventory>`, `PlayerInventory.h:24`) |
| `ContainerManagerModel::getSlot` | `getFullContainerSlot(int, FullContainerName const&)` (`:107`) |
| `BlockSource::fireBlockChanged` | есть, виртуалка + `$fireBlockChanged` (`BlockSource.h:282/705`) |
| `Actor::_hurt` | `_hurt(ActorDamageSource const&, float, HurtParameters const&)` (`Actor.h:612`) |
| `bobHurt` | `LevelRendererPlayer::bobHurt(Matrix&, float)` (`:365`) |
| `Actor::mGameMode` / `mHurtTimeComponent` / `mSupplies` | ECS-компоненты `ActorGameTypeComponent` / `MobHurtTimeComponent` / `ActorEquipmentComponent` |

### Уровень 2. По бинарнику (только ты)

Нужно, когда символа нет вообще: `ClientInstance_mMinecraftSim`,
все поля `MinecraftGame` (в хидере не описано **ни одного** поля),
`LevelRendererPlayer_mFovY` как отдельная величина, `BlockSource_mBuildHeight`,
`bgfx_*`, `BlockLegacy_*` (класса в 1.26 нет),
и твои кастомные `Bone` / `MinecraftSim` / `UIProfanityContext`.

Метод тот же, что у тебя сейчас, но теперь объём известен: 23 цели вместо 88.
Сигнатуру снимать не надо — LeviLamina даст адрес по имени, если имя есть;
снимать надо только то, что в хидер не попало.

### Уровень 3. В рантайме (только ты)

Подтвердить то, что в символах выглядит однозначно, но проверяется только игрой:
- `SneakDown` (бит 0) против `SneakInputCurrentlyDown` (бит 21) — что именно ты хочешь перехватывать;
- `MoveInputComponent::mIsMoveLocked` — в 1.26 переехал, заглушка в коде помечена `// TODO`;
- хеши ECS-компонентов в `ComponentHashes.hpp` (там прямо написано «Last updated: 1.21.44»);
- соответствует ли `mFov` (один float) тому, что ты раньше называл `mFovX`.

## Что применено как код

### 1. `MoveInputComponent` — переписан под 1.26

`src/SDK/Minecraft/Actor/Components/MoveInputComponent.hpp`

В 1.21.44: отдельные байты (`mIsSneakDown` 0x20, `mIsJumping` 0x26, `mIsSprinting` 0x27,
WASD 0x2C–0x2F, `mMoveVector` 0x48), размер 136.
В 1.26: **биты** в `bitset<27,uint>` внутри двух `MoveInputState`, `mMove` (Vec2) по `0x24`, размер `0x64`.

Индексы битов — из `MoveInputState::Flag` (`src/mc/input/MoveInputState.h`):
`SneakDown=0, JumpDown=7, SprintDown=8, Up=13, Down=14, Left=15, Right=16, JumpInputCurrentlyDown=26`.

`tools/port_moveinput.py` переправил **98 мест вызова в 20 файлах**:

```cpp
// было                                  стало
moveInput->mIsSneakDown = false;   →    moveInput->setSneakDown(false);
moveInput->mIsJumping              →    moveInput->isJumping()
input->mMoveVector = glm::vec2(0)  →    input->setMoveVector(glm::vec2(0.f))
```

### 2. `src/SDK/Generated/Offsets_1_26.hpp` — смещения для 493 классов

Сгенерировано `tools/ll_offsets.py` из `TypedStorage<Align, Size, Type>`:

```cpp
namespace Offsets_1_26::Actor { mEntityContext = 0x0; mSentDelta = 0x158; ... Size = 0x3A8; }
namespace Offsets_1_26::MoveInputComponent { mInputState = 0x0; mRawInputState = 0x10; mMove = 0x24; Size = 0x64; }
```

Смещения **расчётные** (поля по порядку + выравнивание), а не снятые в IDA.
Рабочий процесс: взять кандидата → проверить в IDA → заменить хардкод.
`⚠ BASE_UNKNOWN` = есть базовый класс, смещение от начала своего блока полей.

## Инструменты

```bash
python3 tools/symbol_audit.py --annotate      # хуки + OffsetProvider -> audit.md, csv, пометки
python3 tools/ll_offsets.py --all-components --emit-header src/SDK/Generated/Offsets_1_26.hpp \
                                             --emit-md docs/migration-1.26/audit-sdk.md
python3 tools/ll_offsets.py MoveInputComponent
python3 tools/port_moveinput.py --dry-run
python3 tools/symbol_audit.py --ll /path/to/LeviLamina
```

Ручные вердикты лежат в словаре `MANUAL` в `tools/symbol_audit.py` — туда же добавляй
новые находки, чтобы пометки в коде перегенерировались с ними.

## Порядок работ

1. Собрать проект и поправить, что сломал порт `MoveInputComponent` (20 файлов).
2. Заменить три хука на события LeviLamina: `Keyboard::feed`, `MouseDevice::feed`, `ScreenView::setupAndRender`.
3. Пройти 38 `FOUND` + 27 `RENAMED/MOVED` по таблице в `audit.md` — это механика.
4. Оставшиеся 23 — IDA, объём теперь известен точечно.
5. Модули, GUI и конфиги — последними.

> Не по технике: LeviLamina просит не использовать её для программ,
> «compromising Minecraft's security» (`usage_guidelines.en.md`), а клиентская установка
> патчит `Minecraft.Windows.exe` на месте. Для серверных модов всё это работает «в чистую».
