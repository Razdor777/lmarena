# Сигнатуры и оффсеты: 1.21.44 → 1.26.62

Источник данных: `src/SDK/*` (твой Solstice) и `26.62/src/SDK/*` (база 26.62). Скрипт: `tools/sig_audit.py`.

- сигнатур в корне: **97**, в 26.62: **88**;
- оффсетов/индексов в корне: **97**, в 26.62: **74**;
- нет в 26.62, но есть у тебя: **17** (из них используются в коде: **0**, висят без дела: **17**);
- появилось нового в 26.62: **8**;

> **Важно:** отметка «да» означает только то, что запись есть в таблице
> 26.62. Подтверждена ли она именно на `1.26.62`, из кода не видно —
> см. колонку «версия в 26.62» и раздел «Что требуется от тебя».

# Чего нет в 26.62, но использует код (чинить в первую очередь)

| сигнатура | в 26.62 | версия в 26.62 | у тебя (1.21.44) | кто использует |
|---|---|---|---|---|

# Чего нет в 26.62 и никто не использует (можно удалить)

| сигнатура | в 26.62 | версия в 26.62 | у тебя (1.21.44) | кто использует |
|---|---|---|---|---|
| `Actor_getStatusFlag` | **нет** | — | да | — |
| `Actor_setPosition` | **нет** | — | да | — |
| `ClientInstance_grabMouse` | **нет** | — | да | — |
| `ClientInstance_releaseMouse` | **нет** | — | да | — |
| `ColorTint_set` | **нет** | — | да | — |
| `ContainerValidation_handlePlace_jnz` | **нет** | — | да | — |
| `ContainerValidation_handlePlace_test` | **нет** | — | да | — |
| `ContainerValidation_handleTransfer_jnz` | **нет** | — | да | — |
| `ItemEntityRenderer_render` | **нет** | — | да | — |
| `ItemInHandRenderer_renderItem` | **нет** | — | да | — |
| `ItemRenderer_renderGuiItem` | **нет** | — | да | — |
| `ItemStack_fromDescriptor` | **нет** | — | да | — |
| `JSON_toStyledString` | **нет** | — | да | — |
| `Level_getRuntimeActorList` | **нет** | — | да | — |
| `MainView_instance` | **нет** | — | да | — |
| `Mob_getJumpControlComponent` | **нет** | — | да | — |
| `isCriticalHit` | **нет** | — | да | — |

# Новое в базе 26.62

| сигнатура | в 26.62 | версия в 26.62 | у тебя (1.21.44) | кто использует |
|---|---|---|---|---|
| `ActorShaderManager_setupShaderParameters` | да | — | нет | Hook/Hooks/RenderHooks/SetupShaderParameterHook.cpp |
| `GetCollisionShapeActorProxy_getFeetAttachPos` | да | — | нет | — |
| `LevelRenderer_renderLevel` | да | — | нет | Hook/Hooks/RenderHooks/LevelRendererHook.cpp |
| `MinecraftGame_instance` | да | — | нет | — |
| `Mob_JumpFromGroundRequestComponentPatch` | да | — | нет | Features/Modules/Movement/NoJumpDelay.cpp |
| `NetherNetConnector_runEvents` | да | — | нет | Hook/Hooks/NetworkHooks/NetherNetHooks.cpp |
| `PlayerMovement_clearInputStateInlined3` | да | — | нет | Features/Modules/Movement/InventoryMove.cpp |
| `SneakMovementSystem_tickSneakMovementSystem2` | да | — | нет | Features/Modules/Movement/SafeWalk.cpp |
