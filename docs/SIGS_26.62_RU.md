# Сигнатуры 26.62: состояние и чек-лист

Сгенерировано `tools/sig_checklist.py`. Бинарника 1.26.5x нет, поэтому
числа ниже — это не «найдено/не найдено», а «что ждёт скана».

- живых `DEFINE_SIG`: **74**
- закомментированных: **29**
- объявленных заглушек (`= 0`, детур не встанет): **17**
- имён, на которые реально ссылается код: **102**

## Что сломается без сигнатуры

| Группа | Имя | Состояние | Где используется |
|---|---|---|---|
| Урон и снаряды | `Actor_hurt` | 🟡 паттерн-догадка (проверить) | Hook/Hooks/ActorHooks/HealthHooks.cpp |
| Урон и снаряды | `Projectile_onHit` | 🟡 паттерн-догадка (проверить) | Hook/Hooks/ActorHooks/ProjectileHooks.cpp |
| Урон и снаряды | `HurtColor` | 🟡 паттерн-догадка (проверить) | Hook/Hooks/RenderHooks/HurtColorHook.cpp |
| Ambience (небо/туман/облака) | `Ambience_SkyRender` | ⛔ заглушка `= 0` | Hook/Hooks/RenderHooks/AmbienceHook.cpp |
| Ambience (небо/туман/облака) | `Ambience_EndSkyRender` | ⛔ заглушка `= 0` | Hook/Hooks/RenderHooks/AmbienceHook.cpp |
| Ambience (небо/туман/облака) | `Ambience_ChunkRender` | ⛔ заглушка `= 0` | Hook/Hooks/RenderHooks/AmbienceHook.cpp |
| Ambience (небо/туман/облака) | `Ambience_SunMoonRender` | ⛔ заглушка `= 0` | Hook/Hooks/RenderHooks/AmbienceHook.cpp |
| Ambience (небо/туман/облака) | `Ambience_CloudRender` | ⛔ заглушка `= 0` | Hook/Hooks/RenderHooks/AmbienceHook.cpp |
| Инвентарь | `ContainerScreenController_tick` | ✅ есть паттерн | Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.cpp |
| Инвентарь | `ContainerScreenController_onContainerSlotHovered` | 🟡 паттерн-догадка (проверить) | Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.cpp |
| Инвентарь | `ContainerScreenController_handleAutoPlace` | ✅ есть паттерн | SDK/Minecraft/Inventory/ContainerScreenController.cpp |
| Инвентарь | `ContainerScreenController_tryBeginTransition` | 🟡 паттерн-догадка (проверить) | SDK/Minecraft/Inventory/ContainerScreenController.cpp |
| Предметы | `ItemStack_fromDescriptor` | 🟡 паттерн-догадка (проверить) | SDK/Minecraft/Inventory/ItemStack.cpp |
| Предметы | `EnchantUtils_getEnchantLevel` | ✅ есть паттерн | — |
| Предметы | `GameMode_getDestroyRate` | ✅ есть паттерн | SDK/Minecraft/Actor/GameMode.cpp |
| Предметы | `GameMode_baseUseItem` | ✅ есть паттерн | SDK/Minecraft/Actor/GameMode.cpp |
| Сеть | `ComplexInventoryTransaction_vtable` | ✅ есть паттерн | SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp |
| Сеть | `ConnectionRequest_create_DefaultInputMode` | ✅ есть паттерн | Features/Modules/Misc/EditionFaker.cpp |
| Анимации/рендер | `BobHurt` | ✅ есть паттерн | Hook/Hooks/ActorHooks/AnimationHooks.cpp |
| Анимации/рендер | `TapSwingAnim` | ✅ есть паттерн | Features/Modules/Visual/Animations.cpp |
| Анимации/рендер | `FluxSwing` | ✅ есть паттерн | Features/Modules/Visual/Animations.cpp |
| Анимации/рендер | `LevelRenderer_renderLevel` | ✅ есть паттерн | Hook/Hooks/RenderHooks/LevelRendererHook.cpp |

## Код зовёт, а живого паттерна нет

| Имя | Почему нет | Где |
|---|---|---|
| `ActorRenderDispatcher_render` | паттерн закомментирован | Hook/Hooks/RenderHooks/ActorRenderDispatcherHook.cpp |
| `FastEat` | паттерн закомментирован | Features/Modules/Player/FastEat.cpp |
| `InputModeBypass` | паттерн закомментирован | Features/Modules/Misc/EditionFaker.cpp |
| `InputModeBypassFix` | паттерн закомментирован | Features/Modules/Misc/EditionFaker.cpp |
| `Keyboard_feed` | паттерн закомментирован | Hook/Hooks/MiscHooks/KeyHook.cpp |
| `Level_getRuntimeActorList` | паттерн закомментирован | SDK/Minecraft/World/Level.cpp |

## Порядок скана, когда появится exe

0. Сначала перепроверить шесть «догадок» — они уже вписаны, но не сняты:
   `Actor_hurt`, `Projectile_onHit`, `HurtColor`,
   `ContainerScreenController_onContainerSlotHovered`,
   `ItemStack_fromDescriptor`, `ContainerScreenController_tryBeginTransition`.
   Если пролог не совпал — сканер вернёт 0, и детур не встанет (падения нет).

1. Снять `Actor_hurt` — без него молчат `DamageLog` и `LastTP`.
2. Снять `Projectile_onHit` — без него молчат `BowHitMarker` и `ArrowRecovery`.
3. Снять `HurtColor` — без него `HitColor` не перекрашивает вспышку урона.
4. Снять `ContainerScreenController_onContainerSlotHovered` — без него
   `JavaInventory`/`JavaInventoryHotkeys` не знают слот под курсором.
5. Снять пять `Ambience_*` — без них `Ambience` молчит (небо/туман/облака).
6. Проверить «унаследованные» `ItemStack_fromDescriptor` и
   `ContainerScreenController_tryBeginTransition` — они восстановлены по
   догадке, а не сняты с бинарника.

