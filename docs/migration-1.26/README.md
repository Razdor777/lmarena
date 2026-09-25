# Миграция Solstice 1.21.44 → 1.26: что сделано, что обновлено, что осталось

Дата сверки: заголовки LeviLamina `main` (26.51, `bedrockdata 26.51.1`).
Источник истины по символам — клон LeviLamina, а не догадки.

## Коротко: что применено, а что только размечено

| Слой | Что сделано | Статус |
|---|---|---|
| Хуки (27 целей) | **аудит + пометки `// [1.26]`** в 21 файле | размечено |
| `OffsetProvider.hpp` (61 запись) | **пометки `// [1.26]`** над каждой записью | размечено |
| `MoveInputComponent` | **переписан код**: структура под 1.26 + методы вместо полей-флагов, **98 мест вызова переправлены** | **применено** |
| Все классы SDK | **сгенерирован `src/SDK/Generated/Offsets_1_26.hpp`** — реальные смещения для 493 классов | **применено (новый файл)** |
| Остальные SDK-структуры | аудит `audit-sdk.md`, смещения посчитаны | размечено, можно брать |

## Что применено по-настоящему

### 1. `MoveInputComponent` — переписан под 1.26

`src/SDK/Minecraft/Actor/Components/MoveInputComponent.hpp`

В 1.21.44 это были отдельные байты по фиксированным смещениям
(`mIsSneakDown` 0x20, `mIsJumping` 0x26, `mIsSprinting` 0x27, WASD 0x2C–0x2F, `mMoveVector` 0x48).
В 1.26 это **биты** в `brstd::bitset<27, uint>` внутри двух структур `MoveInputState`,
а вектор движения лежит в `mMove` (Vec2) по смещению `0x24`. Размер структуры `0x64` вместо 136.

Индексы битов взяты из `MoveInputState::Flag` (`src/mc/input/MoveInputState.h`):
`SneakDown=0, JumpDown=7, SprintDown=8, Up=13, Down=14, Left=15, Right=16,
JumpInputCurrentlyDown=26`.

Структура переписана, старый API сохранён в виде методов, и
`tools/port_moveinput.py` **переправил 98 мест вызова** в 20 файлах
(`Fly`, `InventoryMove`, `TargetStrafe`, `Keystrokes`, `Step`, `MathUtils`, `Keyboard`, …):

```cpp
// было
moveInput->mIsSneakDown = false;
bool jumping = player->getMoveInputComponent()->mIsJumping;
// стало
moveInput->setSneakDown(false);
bool jumping = player->getMoveInputComponent()->isJumping();
```

Правка обратима: `git checkout <файлы>`.

### 2. `src/SDK/Generated/Offsets_1_26.hpp` — смещения для 493 классов

Сгенерировано `tools/ll_offsets.py` из `TypedStorage<Align, Size, Type>` хидеров LeviLamina:
поля идут в порядке объявления, выравнивание учитывается.

```cpp
namespace Offsets_1_26 {
namespace Actor {
    constexpr ptrdiff_t mEntityContext = 0x0;    // ::EntityContext, 24 байт
    constexpr ptrdiff_t mSentDelta     = 0x158;  // ::Vec3, 12 байт
    ...
    constexpr ptrdiff_t Size = 0x3A8;
}
namespace MoveInputComponent {
    constexpr ptrdiff_t mInputState    = 0x0;
    constexpr ptrdiff_t mRawInputState = 0x10;
    constexpr ptrdiff_t mMove          = 0x24;
    constexpr ptrdiff_t Size = 0x64;
}
}
```

**Это расчётные смещения, а не снятые в IDA.** Использовать так:
быстро получить кандидата — проверить в IDA — заменить хардкод.
Классы с базовым классом помечены `⚠ BASE_UNKNOWN` (смещение от начала своего блока полей).

## Аудит (пометки и отчёты)

| Группа | Всего | Найдено в 1.26 | Осталось |
|---|---|---|---|
| Хуки | 27 | **22** (81 %) | 5 |
| Сигнатуры/поля `OffsetProvider` | 61 | **27** (44 %) | 34 |
| Классы SDK | 935 | **493** с layout (53 %) | 385 не найдено + 61 без полей |

Отчёты: `audit.md` (хуки и сигнатуры), `audit-sdk.md` (классы SDK), `symbol-map.csv`.

Пометки в коде: `// [1.26] ОБНОВЛЕНО` / `// [1.26] ПРОВЕРИТЬ` / `// [1.26] ОСТАЛОСЬ`.

## Кто применяет остальное — честный ответ

**Никто, кроме тебя (или человека с IDA и бинарником 1.26).** Причины жёсткие, а не «я не захотел»:

1. **Здесь нет `Minecraft.Windows.exe` 1.26.** Смещения полей, которых нет в хидерах
   (`ClientInstance_mLevelRenderer`, `LevelRendererPlayer_mFovX`, `Actor_mSwinging`, …),
   добываются только из бинарника. Заголовки LeviLamina — это дамп символов, а не памяти.
2. **Здесь нет MSVC и Windows.** Проект собирается CMake+MSVC, я не могу даже проверить,
   что правки компилируются. Всё, что я менял вслепую, — это механика, проверяемая grep'ом.
3. **Часть вещей принципиально требует прогона в игре**: бит `SneakDown` против
   `SneakInputCurrentlyDown`, `mIsMoveLocked` (в 1.26 переехал в другой компонент),
   индексы компонентов ECS. Это 10–15 минут в отладчике и ноль минут в рассуждениях.

Что я мог сделать без бинарника — я сделал: там, где имя/лейаут есть в символах,
получился **реальный код и реальные числа** (493 класса, `MoveInputComponent` целиком).
Там, где символов нет, — получилась **точная опись объёма** с файлами и строками,
чтобы не тратить время на «а что вообще осталось».

## Порядок дальнейших работ

1. Собрать проект — поправить то, что сломалось после порта `MoveInputComponent`.
2. Заменить три хука (Key/Mouse/SetupAndRender) на события LeviLamina (см. `audit.md`).
3. Пройти по 22 хукам из `audit.md`: взять имя и **новую сигнатуру** (минимум — `applyToPose`).
4. Для каждого хардкод-оффсета из `OffsetProvider.hpp` свериться с
   `Offsets_1_26.hpp`; что совпало — заменить, что нет — снять в IDA.
5. Оставшиеся 5 хуков и 34 поля — реверс.
6. Логика модулей, GUI и конфиги — последними (зависит от типов).

## Инструменты

```bash
python3 tools/symbol_audit.py --annotate              # хуки + OffsetProvider -> отчёт и пометки
python3 tools/ll_offsets.py --all-components --emit-header src/SDK/Generated/Offsets_1_26.hpp \
                                            --emit-md docs/migration-1.26/audit-sdk.md
python3 tools/ll_offsets.py MoveInputComponent        # смещения одного класса
python3 tools/symbol_audit.py --ll /path/to/LeviLamina
```

Переименования, которые скрипт не знает, правятся в `SYNONYMS` / `NOTES`
в начале `tools/symbol_audit.py`.

> Не по технике: LeviLamina просит не использовать её для программ,
> «compromising Minecraft's security» (`usage_guidelines.en.md`), а клиентская установка
> патчит `Minecraft.Windows.exe` на месте. Для серверных модов всё это работает «в чистую».
