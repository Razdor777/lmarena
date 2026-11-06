# Combat: что именно менять при переносе в `26.62/`

> **Статус: перенесено.** Все 13 модулей Combat лежат в `26.62/src/Features/Modules/Combat`,
> зарегистрированы в `ModuleManager` (кроме `GhostMode` — он в старой базе тоже не
> регистрировался, см. §9), правки ниже применены. Ниже — что именно сделано и
> что осталось проверить в игре.

Приоритет — Combat. Ниже только то, что реально сломается, с точными строками
и готовыми заменами. Всё остальное в этих файлах переносится без правок.

## 0. Итог по файлам

| Модуль | Статус | Что править |
|---|---|---|
| `Aura` | ✅ почти как есть | 1 вызов `attack`, см. §1 |
| `TriggerBot` | ✅ почти как есть | 1 вызов `attack`, см. §1 |
| `AutoClicker` | ✅ как есть | только `KeyboardMouseSettings` (транзитивно, §4) |
| `Criticals` | ✅ как есть | пакеты `PlayerAuthInputPacket` / `InventoryTransactionPacket` в новой базе есть |
| `HitBoxes` | ✅ как есть | чистая логика через `ActorUtils::getActorList` + `AABBShapeComponent` |
| `Reach` | ⚠️ проверить | патчит живую функцию через `SigManager::Reach`/`BlockReach` — оба сигнала есть и в `26.62`, но адреса новые, патч-смещения (`func + 11`, `NopBytes(func + 5, 6)`) сняты с .44 → **перепроверить на новом exe** |
| `InfiniteAura` | ⚠️ 1 API | `getOffhandContainer` (§2) |
| `InfiniteChestAura` | ⚠️ 3 API | `buildBlock` (§1), `getHunger` (§3), `baseUseItem` (ок) |

Файлов, которых нет в новой базе, у Combat **нет** — все `#include` резолвятся,
кроме перечисленных в §4.

---

## 1. `GameMode` — новый интерфейс (главная правка)

Старый (корневой) `SDK/Minecraft/Actor/GameMode.hpp` — сплошные чистые виртуалки,
вызов через vtable. Новый (`26.62/src/SDK/Minecraft/Actor/GameMode.hpp`) — тонкие
обёртки над `MemUtils::callVirtualFunc`, и **сигнатуры другие**:

| Было (.44) | Стало (26.62) | Разница |
|---|---|---|
| `attack(Actor* target)` | `attack(Actor& Entity, glm::vec3 AttackPos = {})` | **по ссылке**, не по указателю! |
| `buildBlock(pos, side, bool)` | `buildBlock(pos, face, **unsigned char slot**, bool isSimTick)` | добавился слот руки |
| `interact(Actor*, vec3)` | `interact(Actor*, vec3, **unsigned char slot**)` | добавился слот |
| `startDestroyBlock(ivec3*, int, bool&)` | `startDestroyBlock(ivec3 pos, unsigned char face, bool)` | по значению |
| `destroyBlock(ivec3*, int)` | `destroyBlock(ivec3 pos, unsigned char face)` | по значению |
| `stopDestroyBlock(ivec3 const&)` | `stopDestroyBlock(ivec3 pos)` | по значению |
| `useItem`, `useItemOn`, `continueBuildBlock`, `stopBuildBlock`, `getPickRange`, `tick` | **удалены** | ищите замену в §5 |

### Правки по строкам

`src/Features/Modules/Combat/Aura.cpp:691`
```cpp
if (gameMode) gameMode->attack(attackTarget);          // было
if (gameMode) gameMode->attack(*attackTarget);         // стало
```

`src/Features/Modules/Combat/TriggerBot.cpp:73`
```cpp
player->getGameMode()->attack(actor);                  // было
player->getGameMode()->attack(*actor);                 // стало
```

`src/Features/Modules/Misc/AutoLootbox.cpp:81` — то же самое (`attack(*closestActor)`).

`src/Features/Modules/Combat/InfiniteChestAura.cpp:417`
```cpp
player->getGameMode()->buildBlock(pos, bestFace, false);           // было (bool в 3-м арг.)
player->getGameMode()->buildBlock(pos, bestFace, 0 /*hand*/, false); // стало
```

`src/Features/Modules/Player/ChestAura.cpp:78` и
`src/Utils/MiscUtils/BlockUtils.cpp:244` — тот же `buildBlock`, 3 аргумента → 4.

> Проверить у себя `attack(Actor&)`: старый вариант с указателем компилироваться
> перестанет на `Actor*` (нет неявного разыменования), а вот `interact` и
> `baseUseItem` остались с теми же типами.

## 2. `Actor::getOffhandContainer()` — удалён

`src/Features/Modules/Combat/InfiniteAura.cpp:539`
```cpp
auto* offhand = target->getOffhandContainer();   // было
```
В новой базе метод убрали, но `ActorEquipmentComponent` остался и **поле на месте**
(`26.62/src/SDK/Minecraft/Actor/Components/ActorEquipmentComponent.hpp`):
```cpp
struct ActorEquipmentComponent {
    class SimpleContainer* mOffhandContainer;
    class SimpleContainer* mArmorContainer;
};
```
Два варианта, оба рабочие:

**A. локальный хелпер в модуле** (ничего не трогаем в SDK):
```cpp
auto* equip = target->mContext.getComponent<ActorEquipmentComponent>();
if (!equip || !equip->mOffhandContainer) return false;
auto* stack = equip->mOffhandContainer->getItem(0);
```

**B. вернуть метод в SDK** (`26.62/src/SDK/Minecraft/Actor/Actor.{hpp,cpp}`) —
как было в .44, он безвредный:
```cpp
// Actor.hpp
SimpleContainer* getOffhandContainer();
// Actor.cpp
SimpleContainer* Actor::getOffhandContainer() {
    auto* e = mContext.getComponent<ActorEquipmentComponent>();
    return e ? e->mOffhandContainer : nullptr;
}
```
Рекомендую **B**: тот же метод нужен ещё в 9 местах (ArmorSlotUnlock,
JavaInventory, JavaInventoryHotkeys, AutoArmor и др.) — вернув его один раз,
не будете править каждый модуль.

## 3. `Actor::getHunger()` — удалён

`src/Features/Modules/Combat/InfiniteChestAura.cpp:516`
```cpp
const bool hungry = player->getHunger() < 20.f;   // было
```
В новой базе есть `Actor::getAttribute(AttributeId)`, а `AttributeId` теперь
`enum` с `Health = 5`, `Absorption = 12` — **голода (2) в нём нет**. Рабочий
вариант — через хеш атрибута (в базе есть `AttributeHashes::HUNGER`):
```cpp
static float getHunger(Actor* a) {
    auto* inst = a->getAttribute(2 /* AttributeId::PlayerHunger */);
    return inst ? inst->mCurrentValue : 20.f;
}
```
и добавить `PlayerHunger = 2` в `enum AttributeId` новой базы. Строку заменить на
`const bool hungry = getHunger(player) < 20.f;`

## 4. Заголовки (единственные 5 несуществующих include)

| Модуль | Include | Решение |
|---|---|---|
| `GhostMode.cpp:19` | `SDK/Minecraft/KeyboardMouseSettings.hpp` | файл переименован → `SDK/Minecraft/KeyboardRemappingLayout.hpp`, класс `KeyboardMouseSettings` → `KeyboardRemappingLayout`, метод `getKeyboardMouseSettings()` → `getKeyboardRemappingLayout()`. В `GhostMode.cpp` этот include нигде не используется (§ниже) — можно просто удалить |
| `GhostMode.cpp` | `SDK/Minecraft/MinecraftSim.hpp` | есть в базе, ок |
| `LastTP.hpp` | `Features/Events/EntityHurtEvent.hpp` | события нет в `26.62` → перенести файл `src/Features/Events/EntityHurtEvent.hpp` (он самодостаточный) |
| `Reach.cpp` | `Utils/Buffer.hpp` | есть в базе, ок |
| все | `spdlog/…`, `nlohmann/…`, `glm/…` | внешние зависимости, ок |

## 5. Чего в новой базе вообще нет (`GameMode::tick`, `useItem`, `useItemOn`, `getPickRange`)

- `tick()` — реальных вызовов в моих модулях **0** (встречается только как имя
  поля/локальной переменной) → правок не нужно.
- `useItem` — 4 места, ни одного в Combat:
  `Movement/Speed.cpp:92`, `Player/MidclickAction.cpp:64`,
  `Utils/GameUtils/ItemUtils.cpp:357`, `ItemUtils.hpp:25`.
  В новой базе аналог — `GameMode::baseUseItem(ItemStack*)` (есть) либо
  `PlayerInventory`/`InventoryTransactionPacket` вручную.
- `useItemOn` / `getPickRange` — в моём коде не используются.

## 6. Что починить ДО первой сборки Combat

`26.62/src/Features/Modules/Combat/Reach.{cpp,hpp}` до сих пор содержат маркеры
конфликта (`<<<<<<< HEAD`). Пока их нет — категория Combat не собирается вся,
даже если все правки выше сделаны.

## 7. Сигнатуры, которые нужны именно Combat

Сверено с `26.62/src/SDK/SigManager.hpp`:

| Нужно | Есть в новой базе |
|---|---|
| `Reach`, `BlockReach` (патч дистанции в `Reach.cpp`) | ✅ есть |
| `ItemStack_vTable` | ✅ есть |
| `Actor_vtable` | ❌ нет, и в моём коде не используется — можно не переносить |
| `ContainerScreenController_onContainerSlotHovered`, `ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil`, `HurtColor`, `HudCursorRenderer_render` | ❌ нет — используются не в Combat (ContainerActions, ItemUseDelayFix, HurtColorHook, CustomCrosshair), доснимать по новому exe |

## 8. Порядок действий по Combat

1. Разобрать конфликты в `26.62/src/Features/Modules/Combat/Reach.*`.
2. Скопировать 13 файлов Combat из `src/` в `26.62/src/` (перезаписать Aura,
   AutoClicker, Criticals, HitBoxes, InfiniteAura, Reach, TriggerBot).
3. Применить §1 (attack ×3, buildBlock ×3), §2 (вариант B), §3 (голод).
4. Удалить `#include <SDK/Minecraft/KeyboardMouseSettings.hpp>` в `GhostMode.cpp`,
   перенести `Events/EntityHurtEvent.hpp`.
5. Собрать Debug (в `26.62` `__DEBUG__` теперь только для Debug-конфигурации —
   удобно), смотреть `[signatures] failed to find …`.
6. В игре проверить: `Reach` не крашит (патч-смещения!), `Aura` бьёт,
   `InfiniteAura` видит щит в офф-хенде, `InfiniteChestAura` открывает сундук и ест.

---

## 9. Что фактически сделано при переносе (журнал)

| Шаг | Файл | Изменение |
|---|---|---|
| 1 | `Combat/*` (13 модулей) | перенесены из корня в `26.62/src`, 6 новых (`Aimbot`, `ArrowTP`, `GhostMode`, `InfiniteChestAura`, `LastTP`, `PearlStopper`) зарегистрированы в `ModuleManager` |
| 2 | `Aura.cpp:691`, `TriggerBot.cpp:73` | `attack(Actor*)` → `attack(Actor&)` |
| 3 | `InfiniteChestAura.cpp:417` | `buildBlock(pos, face, bool)` → `buildBlock(pos, face, 0, false)` |
| 4 | `TriggerBot.cpp:38` | `getmEntityIdentifier()` → поле `mEntityIdentifier` |
| 5 | `InfiniteChestAura.cpp:217,236` | `blockReadPtr` → `mBlocksReadPtr[0]`, `subchunkIndex` → `mAbsoluteIndex` |
| 6 | `GhostMode.cpp:19` | убран несуществующий `KeyboardMouseSettings.hpp` (не использовался) |
| 7 | `Features/Events/EntityHurtEvent.hpp` | перенесён (нужен `LastTP`) |
| 8 | `Reach.cpp`, `InfiniteChestAura.cpp` | file-local `static` → анонимный namespace (unity-сборка) |
| 9 | `SDK`: `Actor::getOffhandContainer/getHunger/setGameType`, `ItemStack` дурабилити + `valid()`, `Item::getMaxDamage/isBlockItem` | восстановлены, чтобы не править 46 обращений в модулях |
| 10 | `Module`/`ModuleManager`/`Setting` | вернулись `hasCustomData/serializeCustomData/deserializeCustomData` и `StringSetting` — конфиги `InfiniteAura` снова сохраняют `lockTargetKey` |
| 11 | `CommandManager` | `.aimbot` / `.ab` |

### Осталось проверить в игре (без бинарника не проверяется)

1. `Reach` — патч идёт по `SigManager::Reach` со смещениями `func+4`, `func+10`
   (rip-relative disp32). Сигнатуры в `26.62` есть, но смещения сняты с .44:
   если клиент крашится при включении — первым делом смотреть сюда.
2. `Item::getMaxDamage` — vtable-индекс 35 (унаследован с .44). Проверка: `ArmorHUD`
   показывает прочность брони, а не мусор.
3. `Actor::getHunger` — через `AttributeId(2)`. Проверка: `InfiniteChestAura` ест,
   когда голоден, и не ест на полном.
4. `GameMode::attack(*actor)` — проверка: `Aura` и `TriggerBot` реально бьют.

## 10. Следующая категория

По плану — `Player` (62 модуля), но перед этим стоит добить ядро: 10 «пропавших»
идентификаторов в `docs/SDK_COMPAT_26.62_RU.md` §3 показывают, что мешает
остальным модулям (`blockReadPtr` в `OreMiner`/`BlockESP`/`SmartXRay`,
`useItem` в `Speed`/`MidclickAction`, `Options::mMinimum` и т.д.).
