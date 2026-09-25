# Чеклист: «проект полностью на 1.26»

Это не проценты, а конкретные шаги с проверкой. Пройти по порядку.

---

## §0. Сначала выбери путь — от него зависит всё остальное

| | Путь A: свой инжектор | Путь B: LeviLamina как лоадер |
|---|---|---|
| Как ищутся адреса | Signals/паттерны + оффсеты руками | symdb сама, по имени |
| Оффсеты полей | нужны, берёшь из `Offsets_1_26.hpp` | не нужны: поля уже объявлены в хидерах |
| Что с SDK | свои структуры и оффсеты | реальные классы игры (`mc/...h`) |
| ImGui/D3D | твой kiero-хук как есть | твой kiero-хук как есть (LeviLamina это не заменяет) |
| Цена | дёшево начать, дорого каждый апдейт | дорого начать, дёшево каждый апдейт |

**Рекомендую B.** Причина простая: в пути A тебе всё равно придётся снимать оффсеты в IDA,
а в пути B те же самые поля уже объявлены в хидерах и компилятор сам подставит смещения.
`Offsets_1_26.hpp` — это **инструмент пути A** (или временная подпорка на переходе).

---

## §1. Как пользоваться `Offsets_1_26.hpp` (путь A)

Там лежат пространства имён по именам классов с расчётными смещениями. Примеры на твоём коде:

### 1. Поле переименовано

```cpp
// было (1.21.44)
float LevelData::getTick() { return hat::member_at<float>(this, OffsetProvider::LevelData_mTick); }
// стало (1.26): имя mCurrentTick, тип Tick (8 байт), смещение 0x328
Tick LevelData::getCurrentTick() {
    return hat::member_at<Tick>(this, Offsets_1_26::LevelData::mCurrentTick); // 0x328
}
```

### 2. Поле переехало в другой класс

```cpp
// было: FOV лежал прямо в ClientInstance (mFovX 0x6F0 / mFovY 0x704)
float x = hat::member_at<float>(this, OffsetProvider::ClientInstance_mFovX);

// стало: FOV в LevelRendererPlayer (mFov 0x2B8, mOFov 0x2BC),
// а сам LevelRendererPlayer — через LevelRenderer::mLevelRendererPlayer (0x1B8)
auto* renderer = getLevelRenderer();
auto* player   = hat::member_at<LevelRendererPlayer*>(
                     renderer, Offsets_1_26::LevelRenderer::mLevelRendererPlayer);
float fov = hat::member_at<float>(player, Offsets_1_26::LevelRendererPlayer::mFov);
```

### 3. Поля нет вообще — бери геттером

У `ClientInstance` в 1.26 описаны только `mUITexture`/`mUICursorTexture`.
Всё остальное (`mLevelRenderer`, `mPacketSender`, `mGuiData`, `mMinecraftSim`) — **через геттеры**:

| Было (поле + оффсет) | Стало (виртуальный геттер) |
|---|---|
| `mLevelRenderer` | `getLevelRenderer()` |
| `mPacketSender` | `getPacketSender()` |
| `mGuiData` | `getGuiData()` |
| `getBlockSource()` | `getRegion()` |
| `getInputHandler()` | `getInput()` |

В пути A индекс виртуалки для геттера всё равно придётся снять в IDA
(посмотреть, какой слот в vtable занимает `ClientInstance::getRegion`),
но **имя** ты теперь знаешь точно.

### 4. Структуры с полным лейаутом — просто подставь

`MoveInputComponent`, `MoveInputState`, `StateVectorComponent`, `ActorRotationComponent`,
`FallDistanceComponent`, `CameraComponent`, `AABBShapeComponent` — у них описаны все поля,
так что смещения можно брать прямо:

```cpp
// MoveInputState: mFlagValues 0x0, mAnalogMoveVector 0x4, ...
uint32_t bits = hat::member_at<uint32_t>(input, Offsets_1_26::MoveInputState::mFlagValues);
```

> Проверка любого смещения: в IDA открыть геттер этого поля и посмотреть
> `mov rax, [rcx + 0x…]` — число должно совпасть. Если нет — пришли мне правильное,
> я поправлю генератор (учёт базового класса/выравнивания).

---

## §2. Чеклист «считается, что проект на 1.26»

### Этап 1. Подготовка (полдня)

- [ ] Достать `Minecraft.Windows.exe` нужной версии (26.51) — то, что устанавливает LeviLamina client.
- [ ] Поднять LeviLamina client (или распаковать `bedrock-runtime-data 26.51.1-client.6` — там symdb).
- [ ] Проверить, что игра стартует с загруженным лоадером.
  **Готово, когда:** в логе есть загрузка LeviLamina и игра доходит до главного меню.

### Этап 2. Сборка (1–2 дня)

- [ ] Собрать проект после порта `MoveInputComponent` (20 файлов).
- [ ] Поправить `getRawMoveInputComponent()` и `mIsMoveLocked` — там заглушки `// TODO`.
- [ ] Для пути B: перевести проект на сборку как мод LeviLamina
      (`levilamina-mod-template`, xmake, `manifest.json`), CMake оставить для пути A.
  **Готово, когда:** DLL собирается без ошибок.

### Этап 3. Хуки (2–4 дня)

Пройти по `audit.md` и переписать все 27 хуков. Порядок:

- [ ] Заменить 3 хука на события: `Keyboard::feed`, `MouseDevice::feed`, `ScreenView::setupAndRender`.
- [ ] Переписать 38 целей со статусом `FOUND`/`FOUND_THUNK` (имя есть, проверь сигнатуру).
- [ ] Переписать 27 целей `RENAMED`/`MOVED` по подсказкам из пометок в коде.
- [ ] Обновить сигнатуры там, где они изменились (`applyToPose` — обязательно,
      `fireBlockChanged`, `_hurt`, `getFullContainerSlot`).
  **Готово, когда:** все хуки ставятся (в логе нет `Failed to create detour`).

### Этап 4. Оффсеты (3–5 дней, самая длинная часть)

- [ ] Пройти по 61 записи `OffsetProvider.hpp`: где статус `FOUND`/`RENAMED`/`MOVED` —
      взять новое имя/смещение; где `NO_LAYOUT`/`GONE` — снять в IDA (23 штуки).
- [ ] Сверить `offset.txt` (832 строки) с `Offsets_1_26.hpp`.
- [ ] Обновить структуры в `src/SDK/Minecraft/**` по `Offsets_1_26.hpp`.
  **Готово, когда:** `getFov()`, `getViewMatrix()`, `getLocalPlayer()`, `getBlockSource()`,
  инвентарь и тик мира возвращают осмысленные значения (не 0, не мусор).

### Этап 5. Пакеты и сеть (1–2 дня)

- [ ] Обновить `PacketID` под 1.26 (значения добавляются/сдвигаются каждый релиз).
- [ ] Проверить `LoopbackPacketSender::send` и `RakPeer` — в 1.26 есть ещё и WebRTC-стек.
- [ ] Обновить структуры пакетов, которые используешь (`PlayerAuthInputPacket`, `MovePlayerPacket`, …)
      — их лейауты есть в `Offsets_1_26.hpp`.
  **Готово, когда:** пакеты уходят и приходят, команды работают, чат читается.

### Этап 6. Фичи (2–4 дня)

- [ ] Пройти по модулям: Combat, Misc, Movement, Player, Visual.
- [ ] Самое хрупкое: всё, что завязано на ECS-компоненты и `ComponentHashes.hpp`
      (там метка «Last updated: 1.21.44» — хеши почти наверняка другие).
  **Готово, когда:** каждый модуль включается и делает то, что должен.

### Этап 7. Приёмка

- [ ] Игра стартует, GUI открывается.
- [ ] Движение: Fly / Speed / Step / NoClip / Spider — работают.
- [ ] Бой: Reach / Criticals / Aim — работают.
- [ ] Визуал: ESP / Keystrokes / HudEditor — рисуются.
- [ ] Пакеты: Disabler / InventoryMove — работают.
- [ ] 15 минут в мире без краша, лог чистый.
  **Готово, когда:** все пункты выше — да. Вот это и есть «на 100% на 1.26».

---

## §3. Что делать с конкретными файлами

| Файл | Что править |
|---|---|
| `src/SDK/OffsetProvider.hpp` | 61 запись: взять новые имена/смещения, 23 — снять в IDA |
| `src/SDK/Minecraft/**.hpp` | структуры по `Offsets_1_26.hpp` (493 класса) |
| `src/SDK/Minecraft/Actor/Components/**` | лейауты компонентов уже посчитаны |
| `src/SDK/Minecraft/Network/Packets/**` | структуры пакетов 1.26 |
| `src/SDK/Minecraft/Actor/Components/ComponentHashes.hpp` | **хеши на 1.21.44 — обновить целиком** |
| `src/Hook/Hooks/**` (21 файл) | хуки по пометкам `// [1.26]` |
| `src/Features/Modules/**` | логика, если менялись типы |
| `offset.txt` | списано с 1.21.44, актуален только как история |

---

## §4. Что я могу закрыть по ходу, если ты будешь скидывать данные

Присылай, и я обновлю генератор/таблицы:

1. Реальные смещения из IDA, которые не совпали с `Offsets_1_26.hpp` — поправлю расчёт.
2. Хеши компонентов 1.26 — заменю `ComponentHashes.hpp`.
3. Текст краша / лог «Couldn't find symbol» — скажу, что переименовалось.
4. Новую версию LeviLamina — перезапущу `symbol_audit.py` и `ll_offsets.py`.
