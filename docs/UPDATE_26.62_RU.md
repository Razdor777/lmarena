# Апдейт Solstice: 1.21.44 → 26.62

Что сделано в этом заходе, что осталось и как это проверять.
Читать вместе с `docs/UPDATE_1.21.70_RU.md` — общая методика там, здесь только
то, что специфично для этого перехода.

---

## 1. Что вообще произошло (контекст)

В репозитории лежали два независимых проекта:

- **корень `src/`** — твой клиент на базе Solstice, 1.21.44: 1094 файла,
  ~140k строк, 203 модуля;
- **`26.62/`** — это **не обновлённый твой проект**, а другой клиент (Dexko),
  обрезанный до 130 модулей, зато с SDK, уже перепиненным под 26.51/1.26.x.
  Плюс он лежал в состоянии неразрешённого merge (маркеры конфликтов в 14
  файлах, дубль класса `Dexko`/`Solstice`, два таргета на один `src/` —
  в таком виде он не собирался).

Стратегия, которую выбрали: **база — твой Solstice**, из `26.62` забираем
только то, что привязано к версии (сигнатуры, оффсеты, изменившиеся структуры
SDK). Ничего из твоих 203 модулей не выброшено.

---

## 2. Сигнатуры: двухпроходный скан вместо перезаписи

Было бы неправильно просто заменить паттерны на те, что в `26.62`: часть твоих
записей была сделана в «широком» виде (с большим числом wildcard'ов) и работала
на 1.21.44, а часть в 26.62 записана точными байтами под новую сборку.
Перезапись «в лоб» сломала бы то, что сейчас работает.

Поэтому в `src/SDK/SigManager.hpp`:

- `DEFINE_SIG(...)` — основной паттерн, **проход 1**;
- `DEFINE_SIG_FALLBACK(...)` — второй паттерн для того же имени (из 26.62),
  **проход 2**, сканируется **только если проход 1 не нашёл**.

Уже найденная сигнатура никогда не перезаписывается. Это даёт:

| Сборка | Поведение |
|---|---|
| 1.21.44 | работает как раньше, fallback'и просто не нужны |
| 26.62 | где старый паттерн не нашёл — подхватывает новый |

Итог по числам:

- **56** сигнатур получили fallback-паттерн из 26.62
  (в 15 из них поменялся ещё и тип/смещение: было `RefSig/1` — стало `Sig/0`,
  потому что раньше ловился вызов, а теперь пролог функции);
- **8** сигнатур добавлено полностью новых:
  `MinecraftGame_instance`, `Mob_JumpFromGroundRequestComponentPatch`,
  `PlayerMovement_clearInputStateInlined3`, `NetherNetConnector_runEvents`,
  `SneakMovementSystem_tickSneakMovementSystem2`,
  `ActorShaderManager_setupShaderParameters`,
  `GetCollisionShapeActorProxy_getFeetAttachPos`, `LevelRenderer_renderLevel`.

В логе теперь видно, что именно починилось через новый паттерн:

```
[signatures] <Имя> resolved via 26.62 fallback only
[signatures] pass 2 (26.62 patterns) recovered N of M in Xms
```

---

## 3. Оффсеты

`src/SDK/OffsetProvider.hpp`:

- **7 значений перепинено** на 26.62:

  | поле | 1.21.44 | 26.62 |
  |---|---|---|
  | `ClientInstance_mGuiData` | 0x590 | **0x668** |
  | `ClientInstance_mViewMatrix` | 0x368 | **0x438** |
  | `ClientInstance_getScreenName` | 266 | **243** |
  | `Actor_mEntityIdentifier` | 0x240 | **0x260** |
  | `Actor_mLocalName` | 0xCB0 | **0xbe8** |
  | `Block_mLegacy` | 0x30 | **0x68** |
  | `ClientInputHandler_mMappingFactory` | 0x28 | **0x48** |

- **57 полей добавлено** — тех, которых в старой таблице не было вовсе
  (`MinecraftGame_mXuid`, `BlockLegacy_mBlockId = 0x17e`, `Actor_mSwingProgress`,
  `Level_mHitResult`, `GameMode_buildBlock` и т.д.);
- **32 записи помечены `STALE`** — в 26.62 для них значения нет (там они
  закомментированы: `Flarial_*`, `ClientInstance_mFovX/mFovY`, …).
  Они оставлены со старыми числами, но их **надо пересчитать с нового
  бинарника**, иначе будет тихо неверное поведение. Ищи по слову `STALE`.

### 3.1 Важное: 44 конфликта, которые были и до апдейта

В файле один и тот же член был объявлен **дважды** — и сканом
(`DEFINE_INDEX_FIELD`), и константой (`DEFINE_FIELD`). Например
`ClientInstance_getLocalPlayer`, `Actor_baseTick`, `MinecraftGame_mProfanityContext`
и ещё 41 имя. Две `static inline`-переменные с одним именем в одном классе — это
ошибка компиляции, то есть **проект не собирался и на 1.21.44**.

В 26.62 автор эти сканы закомментировал и оставил константы. Сделано так же:
скан отключён с комментарием, авторитетной считается константа. Активных
`DEFINE_INDEX_FIELD` осталось 14, `DEFINE_FIELD` — 101, пересечений нет.

---

## 3.2 Второе «было и до апдейта»: 82 блока конфликтов в `src/`

Проверка `git show HEAD:...` показала, что маркеры конфликтов были закоммичены
в `src/` ещё в исходном коммите, то есть **проект не собирался в принципе**:

| файл | блоков |
|---|---|
| `Modules/Visual/Trails.cpp` | 17 |
| `GUI/CombatantGui.cpp` | 15 |
| `Modules/Visual/WorldParticles.cpp` | 14 |
| `Modules/Player/OreMinerV2.cpp` | 13 |
| `Modules/Visual/TazikHat.cpp` | 10 |
| `Modules/Visual/Tracers.cpp` | 3 |
| `Modules/Player/OreMinerV2.hpp` | 3 |
| `GUI/CombatantGui.hpp` | 2 |
| `Modules/Combat/InfiniteAura.hpp` | 2 |
| `Modules/Visual/JumpCircles.cpp` | 2 |
| `Modules/Visual/Kagune.cpp` | 1 |

Всего 1458 строк внутри маркеров: **1037** в варианте `HEAD`, **421** в варианте
`theirs`.

Как разрешено:

- **21 блок** — вариант `HEAD` полностью содержит `theirs`, взят `HEAD`;
- **8 блоков** — у `HEAD` пусто, а у `theirs` есть код (в основном
  `using namespace XImpl;` и хвосты функций, обрывавшихся на конфликте).
  Здесь взят `theirs`, иначе оставались незакрытые `}` и неразрешённые имена;
- **53 блока** — версии реально расходятся (у `OreMinerV2` — две разные системы
  Debug-дампа, у `CombatantGui` — две версии обработки чат-биндов).
  Везде выбран вариант **`HEAD`** как более новый и полный
  (`HEAD` ссылается на `mChatBinds`, `mDebug` и т.п. — то есть уже
  интегрирован с остальным кодом, которого в `theirs` нет).

Второй вариант **не потерян**: он целиком лежит в истории в
`git show 0e98be7:src/...`, плюс исходные файлы с маркерами остаются в коммите
до этого изменения. Если по фиче нужна именно `theirs`-реализация — скажи по
какому файлу, переключу.

После разрешения: маркеров в `src/` нет, фигурные скобки сбалансированы во всех
11 файлах.

---

## 4. Изменившиеся структуры SDK

| Файл | Что поменялось | Что пришлось править в коде |
|---|---|---|
| `Inventory/ItemStack.hpp` | `Item** mItem` → `WeakPtr<Item> mItem` (ref-counted), `int8_t mCount` → `uint8_t`, `PAD(0x60)` → `PAD(0x58)` | убрать все `*mItem` / `&mItem` / `mItem.mItem`; `getItem()` теперь через `WeakPtr` |
| `Network/Packets/TextPacket.hpp` | плоские `mType/mAuthor/mMessage/mFilteredMessage` → `std::variant<MessageOnly, AuthorAndMessage, MessageAndParams>` | ~69 обращений `packet->mMessage`; см. ниже |
| `Options.hpp` | появился базовый `Option`, `IntOption::value` → `mValue`, `FloatOption::mMinimum` → `mMinValue`, все оффсеты `Options` сдвинулись | 11 файлов |
| `Rendering/GuiData.hpp` | весь блок сдвинут на +0x10 (`mResolution` 0x30→0x40, `mGuiScale` 0x4C→0x5C) | правок не нужно, всё через поля |
| `KeyboardMouseSettings` | переименован в `KeyboardRemappingLayout`, фабрика отдаёт `getKeyboardRemappingLayout()` | `using KeyboardMouseSettings = KeyboardRemappingLayout` + тонкая обёртка, 12 модулей не тронуты |

### 4.1 Про `TextPacket` — почему аксессоры, а не переименование

В твоём коде ~69 мест вида `textPacket->mMessage` и `textPacket->mAuthor`.
Переписывать их все ради переименования — лишний риск. Вместо этого в новой
структуре оставлены **методы с теми же именами**:

```cpp
std::string mMessage() const;   // читается из variant
std::string mAuthor()  const;   // пусто, если тело не AuthorAndMessage
```

`packet->mMessage` по-прежнему компилируется и возвращает ровно то же значение.
Новый код может пользоваться `getMessage()` / `getSource()` / `hasAuthor()`.

Единственное отличие: это уже **значение**, а не поле. Взять `&packet->mMessage`
или присвоить `packet->mMessage = ...` больше нельзя — для записи есть
`setMessage()` / `setChat()`.

### 4.2 `BlockSource::getBlock` — намеренно НЕ трогали

В 26.62 методы переименованы в `reretard/reretard2/reretard3`. Порядок
виртуальных функций при этом **тот же**, поэтому переименование дало бы только
порчу ~50 мест вызова. Оставлены нормальные имена.

---

## 5. Сборка

`CMakeLists.txt`:

- убрано безусловное `add_compile_definitions(__DEBUG__)`. Раньше из-за него
  **все** `#ifndef __DEBUG__` были мёртвым кодом, и ненайденная сигнатура не
  приводила к падению на инициализации — клиент падал позже и в случайном месте;
- починена причина: конструкция
  `set(IS_DEBUG_BUILD CMAKE_BUILD_TYPE STREQUAL "Debug")` + `if (${IS_DEBUG_BUILD})`
  делает `IS_DEBUG_BUILD` *списком* из трёх элементов, а такой `if` всегда
  истинен. Теперь одна корректная проверка:
  **Debug → печатает все ненайденные сигнатуры и живёт дальше,
  Release → падает сразу со списком.**

Также `mSigScanCount` в `SigManager`/`OffsetProvider` стал `std::atomic<int>`:
он инкрементировался из `#pragma omp parallel for`, а это гонка данных.

---

## 6. Что осталось (по приоритету)

1. **Пересчитать 32 `STALE`-оффсета** по новому `Minecraft.Windows.exe`
   (иначе тихо неверные адреса; искать `grep -rn STALE src/SDK`).
2. **Сигнатуры, которых нет в 26.62 дампе** — их надо снимать заново.
   Твой код на них опирается, и новый паттерн для них неизвестен:
   `HudCursorRenderer_render`, `ItemRenderer_iconBlit`,
   `ItemRenderer_getAtlasIconInfo`, `ItemRenderer_getIconUV`,
   `ItemRenderer_renderGuiItemNew`, `ItemStack_fromDescriptor`,
   `ContainerScreenController_tryBeginTransition`,
   `ContainerScreenController_onContainerSlotHovered`,
   `ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil`,
   `HurtColor`, `FireRender`, `ContainerValidation_handlePlace_*`,
   `ContainerValidation_handleTransfer_jnz`, `isCriticalHit`,
   `ItemInHandRenderer_renderItem`, `ItemEntityRenderer_render`,
   `ItemRenderer_renderGuiItem`, `ColorTint_set`.
3. **Проверить `Option`-раскладку в игре**: `static_assert(sizeof(IntOption) == 0x80)`
   взят из 26.62, но `mThirdPerson`/`mGfxFieldOfView`/`mGfxGamma` я не мог
   подтвердить без бинарника.
4. **Пакеты**: `PacketID` и `InventoryTransactionPacket` в 26.62 переписаны
   заметно (563 строки разницы). Твои модули, которые строят транзакции
   (`ArmorSlotUnlock`, `ItemDupe`, `ContainerActions`, `JavaInventory`), надо
   прогнать в игре отдельно.

---

## 7. Как проверять

1. Собрать **Debug** (в нём fail-fast выключен по построению).
2. Запустить игру, открыть лог. Искать:
   - `[signatures] failed to find <Имя>` — сигнатура не нашлась даже вторым проходом;
   - `[signatures] <Имя> resolved via 26.62 fallback only` — починилось новым паттерном (это нормально);
   - `[offsets] failed to find <Имя>` — проблема с оффсетом.
3. Идти по списку, начиная с сигнатур из п. 6.2 — они гарантированно
   потребуют бинарника.
4. После сигнатур проверить поведенческие вещи: `ItemPhysics` (патчит glm),
   `Zoom`/`FullBright` (Options), чат-модули (TextPacket), `ItemESP`/`ArmorHUD`
   (ItemStack).

Сборка в этом окружении невозможна (нет MSVC и Windows SDK), поэтому всё
выше — статические правки; первый реальный прогон компилятора покажет
остаточные расхождения.
