# Сигнатуры проекта → символы 1.26

Таблица нужна, чтобы заменить поиск по байтам (`DEFINE_SIG`) на поиск по имени в symdb.

Источник: `src/SDK/SigManager.hpp` (123 записей).

Статусы: **FOUND** 39, **NO_CLASS** 34, **NOT_IN_CLASS** 21, **GONE** 17, **MOVED** 9, **RENAMED** 3

| Сигнатура | Класс | Член | Символ 1.26 | Статус | Где |
|---|---|---|---|---|---|
| `Actor_setPosition` | `Actor` | `setPosition` | `Actor::_setPos` | RENAMED | Actor.h:661 — в 1.26 setPosition переименован в _setPos (или teleportTo, Actor.h:380) |
| `HudCursorRenderer_render` | `HudCursorRenderer` | `render` | `HudCursorRenderer::render` | FOUND | src-client/mc/client/gui/controls/renderers/HudCursorRenderer.h:34 |
| `Actor_getNameTag` | `Actor` | `getNameTag` | `Actor::getNameTag` | FOUND | src/mc/world/actor/Actor.h:827 |
| `Actor_setNameTag` | `Actor` | `setNameTag` | `Actor::setNameTag` | FOUND | src/mc/world/actor/Actor.h:1083 |
| `ActorRenderDispatcher_render` | `ActorRenderDispatcher` | `render` | `ActorRenderDispatcher::render` | FOUND | src-client/mc/client/renderer/actor/ActorRenderDispatcher.h:93 |
| `ClientInstance_mBgfx` | `ClientInstance` | `mBgfx` | — | GONE | bgfx в 1.26 внешняя библиотека (src-client/mc/external/bgfx), поля в ClientInstance нет |
| `ClientInstance_grabMouse` | `ClientInstance` | `grabMouse` | `ClientInstance::grabMouse` | FOUND | src-client/mc/client/game/ClientInstance.h:1054 |
| `ClientInstance_releaseMouse` | `ClientInstance` | `releaseMouse` | `ClientInstance::releaseMouse` | FOUND | src-client/mc/client/game/ClientInstance.h:1056 |
| `ContainerScreenController_tick` | `ContainerScreenController` | `tick` | `ContainerScreenController::tick` | FOUND | src-client/mc/client/gui/screens/controllers/ContainerScreenController.h:119 |
| `ContainerScreenController_tryBeginTransition` | `ContainerScreenController` | `tryBeginTransition` | — | GONE | метода нет ни в ContainerScreenController, ни в базовых классах |
| `ContainerScreenController_handleAutoPlace` | `ContainerScreenController` | `handleAutoPlace` | `ContainerScreenController::_handleAutoPlace` | FOUND | src-client/mc/client/gui/screens/controllers/ContainerScreenController.h:231 |
| `ContainerValidation_handleTransfer_jnz` | `ContainerValidation` | `handleTransfer_jnz` | `ScreenData::handleTransfer_jnz` | MOVED | переехало: src/mc/world/inventory/network/ItemStackRequestActionHandler.h:175 (метод) |
| `ComplexInventoryTransaction_vtable` | `ComplexInventoryTransaction` | `vtable` | `ComplexInventoryTransaction::ComplexInventoryTransaction` | FOUND | vtаблица класса; адрес бери от конструктора |
| `isCriticalHit` | `isCriticalHit` | `` | — | NO_CLASS | хидер isCriticalHit.h не найден |
| `EnchantUtils_getEnchantLevel` | `EnchantUtils` | `getEnchantLevel` | `EnchantUtils::getEnchantLevel` | FOUND | src/mc/world/item/enchanting/EnchantUtils.h:67 |
| `GameMode_getDestroyRate` | `GameMode` | `getDestroyRate` | `GameMode::getDestroyRate` | FOUND | src/mc/world/gamemode/GameMode.h:198 |
| `ContainerValidation_handlePlace_jnz` | `ContainerValidation` | `handlePlace_jnz` | — | NOT_IN_CLASS | ContainerValidation есть, метода handlePlace_jnz в нём нет |
| `ContainerValidation_handlePlace_test` | `ContainerValidation` | `handlePlace_test` | — | NOT_IN_CLASS | ContainerValidation есть, метода handlePlace_test в нём нет |
| `HoverTextRenderer_render` | `HoverTextRenderer` | `render` | `HoverTextRenderer::render` | FOUND | src-client/mc/client/gui/controls/renderers/HoverTextRenderer.h:38 |
| `GameMode_baseUseItem` | `GameMode` | `baseUseItem` | `GameMode::baseUseItem` | FOUND | src/mc/world/gamemode/GameMode.h:192 |
| `GuiData_displayClientMessage` | `GuiData` | `displayClientMessage` | `GuiData::displayClientMessage` | FOUND | src-client/mc/client/gui/GuiData.h:246 |
| `InventoryTransaction_addAction` | `InventoryTransaction` | `addAction` | `InventoryTransaction::addAction` | FOUND | src/mc/world/inventory/transaction/InventoryTransaction.h:29 |
| `ItemStack_vTable` | `ItemStack` | `vTable` | — | NOT_IN_CLASS | ItemStack есть, метода vTable в нём нет |
| `ItemStack_getCustomName` | `ItemStack` | `getCustomName` | `ComparisonOptions::getCustomName` | MOVED | переехало: src/mc/world/item/ItemStackBase.h:183 (метод) |
| `ItemStack_fromDescriptor` | `ItemStack` | `fromDescriptor` | `ItemStack::fromDescriptor` | FOUND | src/mc/world/item/ItemStack.h:94 |
| `ItemUseInventoryTransaction_vtable` | `ItemUseInventoryTransaction` | `vtable` | — | NOT_IN_CLASS | ItemUseInventoryTransaction есть, метода vtable в нём нет |
| `ItemUseOnActorInventoryTransaction_vtable` | `ItemUseOnActorInventoryTransaction` | `vtable` | — | NOT_IN_CLASS | ItemUseOnActorInventoryTransaction есть, метода vtable в нём нет |
| `ItemReleaseInventoryTransaction_vtable` | `ItemReleaseInventoryTransaction` | `vtable` | — | NOT_IN_CLASS | ItemReleaseInventoryTransaction есть, метода vtable в нём нет |
| `Keyboard_feed` | `Keyboard` | `feed` | — | GONE | в 1.26 вход — событие ll::event::input::KeyInputEvent |
| `MainView_instance` | `MainView` | `instance` | — | GONE | класса MainView в 1.26 нет |
| `MinecraftPackets_createPacket` | `MinecraftPackets` | `createPacket` | `MinecraftPackets::createPacket` | FOUND | src/mc/network/MinecraftPackets.h:18 |
| `Mob_getJumpControlComponent` | `Mob` | `getJumpControlComponent` | `Mob::getJumpControlComponent` | MOVED | компонент переехал в ECS: MobJumpControlComponent |
| `Mob_getCurrentSwingDuration` | `Mob` | `getCurrentSwingDuration` | `Mob::getModifiedSwingDuration` | RENAMED | Item::getSwingDuration — это про предмет, не про моба |
| `MouseDevice_feed` | `MouseDevice` | `feed` | — | GONE | в 1.26 вход — событие ll::event::input::MouseInputEvent |
| `NetworkStackItemDescriptor_ctor` | `NetworkStackItemDescriptor` | `ctor` | — | NO_CLASS | хидер NetworkStackItemDescriptor.h не найден |
| `PlayerMovement_clearInputStateInlined` | `PlayerMovement` | `clearInputStateInlined` | — | NOT_IN_CLASS | PlayerMovement есть, метода clearInputStateInlined в нём нет |
| `PlayerMovement_clearInputStateInlined2` | `PlayerMovement` | `clearInputStateInlined2` | — | NOT_IN_CLASS | PlayerMovement есть, метода clearInputStateInlined2 в нём нет |
| `RakNet_RakPeer_runUpdateCycle` | `RakNet` | `RakPeer_runUpdateCycle` | — | NOT_IN_CLASS | RakNet есть, метода RakPeer_runUpdateCycle в нём нет |
| `RakNet_RakPeer_sendImmediate` | `RakNet` | `RakPeer_sendImmediate` | — | GONE | RakNet вынесен во внешний модуль; в 1.26 основной транспорт — WebRTC |
| `ScreenView_setupAndRender` | `ScreenView` | `setupAndRender` | — | GONE | в 1.26 рендер UI — событие RenderEvent |
| `SimulatedPlayer_simulateJump` | `SimulatedPlayer` | `simulateJump` | `SimulatedPlayer::simulateJump` | FOUND | src/mc/server/SimulatedPlayer.h:188 |
| `ItemInHandRenderer_render_bytepatch` | `ItemInHandRenderer` | `render_bytepatch` | `Page::render_bytepatch` | MOVED | переехало: src-client/mc/client/gui/Font.h:80 (метод) |
| `SneakMovementSystem_tickSneakMovementSystem` | `SneakMovementSystem` | `tickSneakMovementSystem` | — | NOT_IN_CLASS | SneakMovementSystem есть, метода tickSneakMovementSystem в нём нет |
| `ConnectionRequest_create` | `ConnectionRequest` | `create` | — | GONE | сетевой стек переехал (WebRTC) |
| `CameraDirectLookSystemUtil_handleLookInput` | `CameraDirectLookSystemUtil` | `handleLookInput` | — | GONE | класса CameraDirectLookSystemUtil в 1.26 нет |
| `ItemRenderer_render` | `ItemRenderer` | `render` | `ItemRenderer::render` | FOUND | src-client/mc/client/renderer/actor/ItemRenderer.h:92 |
| `ItemInHandRenderer_renderItem` | `ItemInHandRenderer` | `renderItem` | `ItemInHandRenderer::renderItem` | FOUND | src-client/mc/client/renderer/game/ItemInHandRenderer.h:283 |
| `ItemEntityRenderer_render` | `ItemEntityRenderer` | `render` | — | NO_CLASS | хидер ItemEntityRenderer.h не найден |
| `ColorTint_set` | `ColorTint` | `set` | — | NO_CLASS | хидер ColorTint.h не найден |
| `ItemRenderer_renderGuiItem` | `ItemRenderer` | `renderGuiItem` | — | NOT_IN_CLASS | ItemRenderer есть, метода renderGuiItem в нём нет |
| `ItemPositionConst` | `ItemPositionConst` | `` | — | NO_CLASS | хидер ItemPositionConst.h не найден |
| `glm_rotate` | `glm` | `rotate` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `glm_rotateRef` | `glm` | `rotateRef` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `glm_translateRef` | `glm` | `translateRef` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `glm_translateRef2` | `glm` | `translateRef2` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `BlockSource_fireBlockChanged` | `BlockSource` | `fireBlockChanged` | `BlockSource::fireBlockChanged` | FOUND | метод есть, виртуальный; транк $fireBlockChanged |
| `ActorAnimationControllerPlayer_applyToPose` | `ActorAnimationControllerPlayer` | `applyToPose` | `ActorAnimationControllerPlayer::applyToPose` | FOUND | src/mc/world/actor/animation/ActorAnimationControllerPlayer.h:47 |
| `JSON_parse` | `JSON` | `parse` | — | GONE | в 1.26 Json::Value парсится иначе |
| `Actor_getStatusFlag` | `Actor` | `getStatusFlag` | `Actor::getStatusFlag` | FOUND | src/mc/world/actor/Actor.h:204 |
| `Level_getRuntimeActorList` | `Level` | `getRuntimeActorList` | `Level::getRuntimeActorList` | FOUND | src/mc/world/level/Level.h:1326 |
| `ConcreteBlockLegacy_getCollisionShapeForCamera` | `ConcreteBlockLegacy` | `getCollisionShapeForCamera` | — | GONE | класса ConcreteBlockLegacy в 1.26 нет |
| `ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil` | `ClientInputCallbacks` | `handleBuildAction_onAttack_setNoBlockBreakUntil` | — | NOT_IN_CLASS | ClientInputCallbacks есть, метода handleBuildAction_onAttack_setNoBlockBreakUntil в нём нет |
| `WaterBlockLegacy_getCollisionShapeForCamera` | `WaterBlockLegacy` | `getCollisionShapeForCamera` | — | NO_CLASS | хидер WaterBlockLegacy.h не найден |
| `mce_framebuilder_RenderItemInHandDescription_ctor` | `mce` | `framebuilder_RenderItemInHandDescription_ctor` | — | NO_CLASS | хидер mce.h не найден |
| `ResourcePackManager_composeFullStackBp` | `ResourcePackManager` | `composeFullStackBp` | `ResourcePackManager::composeFullStack` | RENAMED | ResourcePackManager.h:142 — суффикс Bp в 1.26 отпал |
| `ClientInstance_isPreGame` | `ClientInstance` | `isPreGame` | `ClientInstance::isPreGame` | FOUND | src-client/mc/client/game/ClientInstance.h:764 |
| `tickEntity_ItemUseSlowdownModifierComponent` | `tickEntity` | `ItemUseSlowdownModifierComponent` | — | NO_CLASS | хидер tickEntity.h не найден |
| `checkBlocks` | `checkBlocks` | `` | — | NO_CLASS | хидер checkBlocks.h не найден |
| `JSON_toStyledString` | `JSON` | `toStyledString` | `T::toStyledString` | MOVED | переехало: src/mc/deps/json/Value.h:415 (метод) |
| `Unknown_renderBlockOverlay` | `Unknown` | `renderBlockOverlay` | — | NO_CLASS | хидер Unknown.h не найден |
| `FastEat` | `FastEat` | `` | — | NO_CLASS | хидер FastEat.h не найден |
| `Unknown_renderNametag` | `Unknown` | `renderNametag` | — | NO_CLASS | хидер Unknown.h не найден |
| `Reach` | `Reach` | `` | — | NO_CLASS | хидер Reach.h не найден |
| `BlockReach` | `BlockReach` | `` | — | NO_CLASS | хидер BlockReach.h не найден |
| `GetSpeedInAirWithSprint` | `GetSpeedInAirWithSprint` | `` | — | NO_CLASS | хидер GetSpeedInAirWithSprint.h не найден |
| `ConnectionRequest_create_DeviceModel` | `ConnectionRequest` | `create_DeviceModel` | — | NOT_IN_CLASS | ConnectionRequest есть, метода create_DeviceModel в нём нет |
| `ConnectionRequest_create_DeviceOS` | `ConnectionRequest` | `create_DeviceOS` | — | NOT_IN_CLASS | ConnectionRequest есть, метода create_DeviceOS в нём нет |
| `ConnectionRequest_create_DefaultInputMode` | `ConnectionRequest` | `create_DefaultInputMode` | — | NOT_IN_CLASS | ConnectionRequest есть, метода create_DefaultInputMode в нём нет |
| `ConnectionRequest_create_CurrentInputMode` | `ConnectionRequest` | `create_CurrentInputMode` | — | NOT_IN_CLASS | ConnectionRequest есть, метода create_CurrentInputMode в нём нет |
| `InputModeBypass` | `InputModeBypass` | `` | — | NO_CLASS | хидер InputModeBypass.h не найден |
| `InputModeBypassFix` | `InputModeBypassFix` | `` | — | NO_CLASS | хидер InputModeBypassFix.h не найден |
| `TapSwingAnim` | `TapSwingAnim` | `` | — | NO_CLASS | хидер TapSwingAnim.h не найден |
| `Unknown_updatePlayerFromCamera` | `Unknown` | `updatePlayerFromCamera` | — | NO_CLASS | хидер Unknown.h не найден |
| `FluxSwing` | `FluxSwing` | `` | — | NO_CLASS | хидер FluxSwing.h не найден |
| `BobHurt` | `BobHurt` | `` | — | NO_CLASS | хидер BobHurt.h не найден |
| `CameraComponent_applyRotation` | `CameraComponent` | `applyRotation` | — | NOT_IN_CLASS | CameraComponent есть, метода applyRotation в нём нет |
| `FireRender` | `FireRender` | `` | — | NO_CLASS | хидер FireRender.h не найден |
| `Actor_canSee` | `Actor` | `canSee` | `Actor::canSee` | FOUND | src/mc/world/actor/Actor.h:687 |
| `ItemInHandRenderer_renderItem_bytepatch` | `ItemInHandRenderer` | `renderItem_bytepatch` | `ItemInHandRenderer::renderItem` | FOUND | src-client/mc/client/renderer/game/ItemInHandRenderer.h:283 |
| `ItemInHandRenderer_renderItem_bytepatch2` | `ItemInHandRenderer` | `renderItem_bytepatch2` | — | NOT_IN_CLASS | ItemInHandRenderer есть, метода renderItem_bytepatch2 в нём нет |
| `HurtColor` | `HurtColor` | `` | — | NO_CLASS | хидер HurtColor.h не найден |
| `Font_drawTransformed` | `Font` | `drawTransformed` | `Font::drawTransformed` | FOUND | src-client/mc/client/gui/Font.h:331 |
| `BaseActorRenderer_renderText` | `BaseActorRenderer` | `renderText` | `SignRenderer::renderText` | MOVED | переехало: src-client/mc/client/renderer/blockactor/SignRenderer.h:81 (метод) |
| `ThirdPersonNametag` | `ThirdPersonNametag` | `` | — | NO_CLASS | хидер ThirdPersonNametag.h не найден |
| `ScreenRenderer_blit` | `ScreenRenderer` | `blit` | `ScreenRenderer::blit` | FOUND | src-client/mc/client/gui/ScreenRenderer.h:66 |
| `mce_RenderMaterialGroup_ui` | `mce` | `RenderMaterialGroup_ui` | — | NO_CLASS | хидер mce.h не найден |
| `blockHighlightColor` | `blockHighlightColor` | `` | — | NO_CLASS | хидер blockHighlightColor.h не найден |
| `mce_Color_BLACK` | `mce` | `Color_BLACK` | — | NO_CLASS | хидер mce.h не найден |
| `LevelRendererPlayer_renderOutlineSelection` | `LevelRendererPlayer` | `renderOutlineSelection` | `LevelRendererPlayer::_renderOutlineSelection` | FOUND | src-client/mc/client/renderer/game/LevelRendererPlayer.h:352 |
| `Tessellator_begin` | `Tessellator` | `begin` | `Tessellator::begin` | FOUND | src-client/mc/client/renderer/Tessellator.h:93 |
| `MeshHelpers_renderMeshImmediately` | `MeshHelpers` | `renderMeshImmediately` | `MeshHelpers::renderMeshImmediately` | FOUND | src-client/mc/common/client/renderer/helpers/MeshHelpers.h:23 |
| `MeshHelpers_renderMeshImmediately2` | `MeshHelpers` | `renderMeshImmediately2` | — | NOT_IN_CLASS | MeshHelpers есть, метода renderMeshImmediately2 в нём нет |
| `glm_rotateRef` | `glm` | `rotateRef` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `glm_translateRef` | `glm` | `translateRef` | — | GONE | статическая библиотека, символа в exe нет — считай сам |
| `CameraYaw` | `CameraYaw` | `` | — | NO_CLASS | хидер CameraYaw.h не найден |
| `CameraYaw2` | `CameraYaw2` | `` | — | NO_CLASS | хидер CameraYaw2.h не найден |
| `UpdatePlayer` | `UpdatePlayer` | `` | — | NO_CLASS | хидер UpdatePlayer.h не найден |
| `Dimension_getBrightnessDependentFogColor` | `Dimension` | `getBrightnessDependentFogColor` | `Dimension::getBrightnessDependentFogColor` | FOUND | src/mc/world/level/dimension/Dimension.h:231 |
| `BlockSource_getBiome` | `BlockSource` | `getBiome` | `BlockSource::getBiome` | FOUND | src/mc/world/level/BlockSource.h:468 |
| `ItemStack_getDamageValue` | `ItemStack` | `getDamageValue` | `OffhandAllowed::getDamageValue` | MOVED | переехало: src/mc/world/item/Item.h:475 (метод) |
| `ItemStack_isEnchanted` | `ItemStack` | `isEnchanted` | `Vec3::isEnchanted` | MOVED | переехало: src/mc/world/actor/Actor.h:458 (метод) |
| `Actor_vtable` | `Actor` | `vtable` | — | NOT_IN_CLASS | Actor есть, метода vtable в нём нет |
| `HitResult_getEntity` | `HitResult` | `getEntity` | `HitResult::getEntity` | FOUND | src/mc/world/phys/HitResult.h:80 |
| `RaknetTick` | `RaknetTick` | `` | — | NO_CLASS | хидер RaknetTick.h не найден |
| `Options_getSensitivity` | `Options` | `getSensitivity` | — | NO_CLASS | хидер Options.h не найден |
| `UIControl_getPosition` | `UIControl` | `getPosition` | `WriteBufferInfo::getPosition` | MOVED | переехало: src/mc/deps/core/file/file_system/FileImpl.h:77 (метод) |
| `ClientInstance_updateScreenSizeVariables` | `ClientInstance` | `updateScreenSizeVariables` | `ClientInstance::_updateScreenSizeVariables` | FOUND | src-client/mc/client/game/ClientInstance.h:1374 |
| `GeneralSettingsScreenController_ctor` | `GeneralSettingsScreenController` | `ctor` | `GeneralSettingsScreenController::ctor` | FOUND | src-client/mc/client/gui/screens/controllers/GeneralSettingsScreenController.h:144 |
| `SettingsScreenOnExit` | `SettingsScreenOnExit` | `` | — | NO_CLASS | хидер SettingsScreenOnExit.h не найден |
| `SettingsScreenOnExit_Patch` | `SettingsScreenOnExit` | `Patch` | — | NO_CLASS | хидер SettingsScreenOnExit.h не найден |
| `ContainerScreenController_onContainerSlotHovered` | `ContainerScreenController` | `onContainerSlotHovered` | `ContainerScreenController::_onContainerSlotHovered` | FOUND | src-client/mc/client/gui/screens/controllers/ContainerScreenController.h:155 |
| `MinecraftGame_onResumeWaitReloadActors` | `MinecraftGame` | `onResumeWaitReloadActors` | `MinecraftGame::_onResumeWaitReloadActors` | FOUND | src-client/mc/client/game/MinecraftGame.h:1671 |
| `BoneOrientation_computeBoneTransform` | `BoneOrientation` | `computeBoneTransform` | — | NOT_IN_CLASS | BoneOrientation есть, метода computeBoneTransform в нём нет |

## Как этим пользоваться

1. Скачать `bedrock-runtime-data` версии `26.51.1-client.6` (релиз на github.com/LiteLDev/bedrock-runtime-data).
2. Распаковать в `tools/data/bedrock_runtime_data/`.
3. `SymDB::instance().load(...)` прочитает файлы, а `SigManager` при неудачном
   поиске по байтам попробует найти адрес по имени из этой таблицы.

Там, где статус GONE/NO_CLASS, имени нет — такие места надо переписывать
на события или искать вручную.
