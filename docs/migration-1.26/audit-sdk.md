# Аудит SDK: классы проекта против заголовков LeviLamina 26.51

- Проверено классов из `src/SDK/Minecraft/**`: **935**
- С описанным layout'ом (можно взять смещения): **493**
- Класс есть, но поля не описаны: **57**
- Класс в 1.26 не найден: **385**

Смещения посчитаны `tools/ll_offsets.py` по `TypedStorage<Align, Size, Type>`
и лежат в `src/SDK/Generated/Offsets_1_26.hpp`.

## Классы с известным layout'ом 1.26

| Класс | Хидер LeviLamina | Размер | Полей | Упоминаний в проекте |
|---|---|---|---|---|
| `ClientInstance` | `src-client/mc/client/game/ClientInstance.h` | 0x28 | 2 | 1044 |
| `Actor` | `src/mc/world/actor/Actor.h` | 0x3A8 | 93 | 935 |
| `Value` | `src/mc/deps/json/Value.h` | 0x40 | 8 | 723 |
| `ItemStack` | `src/mc/world/item/ItemStack.h` | 0x18 | 1 | 205 |
| `Block` | `src/mc/world/level/block/Block.h` | 0x11B | 12 | 176 |
| `PlayerAuthInputPacket` | `src/mc/network/packet/PlayerAuthInputPacket.h` | 0x4 | 1 | 105 |
| `MovePlayerPacket` | `src/mc/network/packet/MovePlayerPacket.h` | 0x4 | 1 | 103 |
| `Item` | `src/mc/world/item/Item.h` | 0x208 | 35 | 97 |
| `Level` | `src/mc/world/level/Level.h` | 0x728 | 127 | 80 |
| `InventoryTransactionPacket` | `src/mc/network/packet/InventoryTransactionPacket.h` | 0x4 | 1 | 77 |
| `BlockSource` | `src/mc/world/level/BlockSource.h` | 0x160 | 26 | 71 |
| `PlayerInventory` | `src/mc/world/actor/player/PlayerInventory.h` | 0xD8 | 6 | 70 |
| `ItemUseInventoryTransaction` | `src/mc/world/inventory/transaction/ItemUseInventoryTransaction.h` | 0xA2 | 12 | 59 |
| `LoopbackPacketSender` | `src/mc/network/LoopbackPacketSender.h` | 0x48 | 4 | 55 |
| `Action` | `src-client/mc/client/input/vanilla/remapping/Action.h` | 0x10 | 2 | 54 |
| `NetworkItemStackDescriptor` | `src/mc/world/item/NetworkItemStackDescriptor.h` | 0x48 | 4 | 50 |
| `TextPacket` | `src/mc/network/packet/TextPacket.h` | 0x4 | 1 | 47 |
| `CameraComponent` | `src/mc/deps/minecraft_camera/components/CameraComponent.h` | 0x11C | 10 | 46 |
| `InventoryAction` | `src/mc/world/inventory/transaction/InventoryAction.h` | 0x200 | 6 | 45 |
| `ComplexInventoryTransaction` | `src/mc/world/inventory/transaction/ComplexInventoryTransaction.h` | 0x60 | 2 | 42 |
| `GameMode` | `src/mc/world/gamemode/GameMode.h` | 0x128 | 36 | 42 |
| `InventoryTransaction` | `src/mc/world/inventory/transaction/InventoryTransaction.h` | 0x58 | 2 | 37 |
| `GuiData` | `src-client/mc/client/gui/GuiData.h` | 0xEB0 | 73 | 35 |
| `LevelChunk` | `src/mc/world/level/chunk/LevelChunk.h` | 0x18D2 | 94 | 33 |
| `ModalFormResponsePacket` | `src/mc/network/packet/ModalFormResponsePacket.h` | 0x4 | 1 | 31 |
| `PlayerActionPacket` | `src/mc/network/packet/PlayerActionPacket.h` | 0x4 | 1 | 30 |
| `AABBShapeComponent` | `src/mc/deps/vanilla_components/AABBShapeComponent.h` | 0x20 | 2 | 28 |
| `Enchant` | `src/mc/world/item/enchanting/Enchant.h` | 0x99 | 10 | 28 |
| `MoveInputComponent` | `src/mc/entity/components/MoveInputComponent.h` | 0x64 | 11 | 28 |
| `MobEquipmentPacket` | `src/mc/network/packet/MobEquipmentPacket.h` | 0x4 | 1 | 27 |
| `StateVectorComponent` | `src/mc/deps/vanilla_components/StateVectorComponent.h` | 0x24 | 3 | 27 |
| `ContainerScreenController` | `src-client/mc/client/gui/screens/controllers/ContainerScreenController.h` | 0x570 | 60 | 26 |
| `ItemUseOnActorInventoryTransaction` | `src/mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h` | 0x88 | 6 | 26 |
| `SimpleContainer` | `src/mc/world/SimpleContainer.h` | 0x20 | 2 | 25 |
| `Blob` | `src/mc/deps/cereal/schema/blob/Blob.h` | 0x18 | 2 | 23 |
| `ContainerManagerModel` | `src/mc/world/containers/managers/models/ContainerManagerModel.h` | 0x178 | 11 | 23 |
| `ItemDescriptor` | `src/mc/world/item/ItemDescriptor.h` | 0x18 | 3 | 23 |
| `LevelRendererPlayer` | `src-client/mc/client/renderer/game/LevelRendererPlayer.h` | 0x13B0 | 78 | 23 |
| `CameraDirectLookComponent` | `src-client/mc/deps/minecraft_camera/components/CameraDirectLookComponent.h` | 0x14 | 5 | 22 |
| `InventorySource` | `src/mc/world/inventory/transaction/InventorySource.h` | 0xC | 3 | 22 |
| `ActorOwnerComponent` | `src/mc/entity/components/ActorOwnerComponent.h` | 0x8 | 1 | 21 |
| `BaseActorRenderContext` | `src-client/mc/client/renderer/BaseActorRenderContext.h` | 0x2F6 | 32 | 21 |
| `ActorTypeComponent` | `src/mc/deps/vanilla_components/ActorTypeComponent.h` | 0x4 | 1 | 20 |
| `SetActorMotionPacket` | `src/mc/network/packet/SetActorMotionPacket.h` | 0x4 | 1 | 20 |
| `TexturePtr` | `src-client/mc/deps/minecraft_renderer/renderer/TexturePtr.h` | 0x20 | 2 | 19 |
| `Attribute` | `src/mc/world/attribute/Attribute.h` | 0x38 | 4 | 18 |
| `LevelRenderer` | `src-client/mc/client/renderer/game/LevelRenderer.h` | 0xA60 | 51 | 18 |
| `MobEffectPacket` | `src/mc/network/packet/MobEffectPacket.h` | 0x4 | 1 | 18 |
| `RuntimeIDComponent` | `src/mc/entity/components/RuntimeIDComponent.h` | 0x8 | 1 | 18 |
| `UpdateBlockPacket` | `src/mc/network/packet/UpdateBlockPacket.h` | 0x4 | 1 | 18 |
| `MoveInputState` | `src/mc/input/MoveInputState.h` | 0xF | 5 | 17 |
| `ActorBlockSyncMessage` | `src/mc/world/level/ActorBlockSyncMessage.h` | 0xC | 2 | 16 |
| `InteractPacket` | `src/mc/network/packet/InteractPacket.h` | 0x4 | 1 | 16 |
| `ActorRotationComponent` | `src/mc/entity/components/ActorRotationComponent.h` | 0x10 | 2 | 15 |
| `DebugCameraComponent` | `src-client/mc/deps/minecraft_camera/components/DebugCameraComponent.h` | 0x125 | 3 | 15 |
| `ItemActor` | `src/mc/world/actor/item/ItemActor.h` | 0xC8 | 12 | 15 |
| `ItemStackNetIdVariant` | `src/mc/world/inventory/network/ItemStackNetIdVariant.h` | 0x18 | 1 | 15 |
| `PlayerListEntry` | `src/mc/world/actor/player/PlayerListEntry.h` | 0xA3 | 11 | 15 |
| `PropertySyncData` | `src/mc/world/actor/state/PropertySyncData.h` | 0x38 | 4 | 15 |
| `SetInstruction` | `src/mc/deps/minecraft_camera/camera_instruction_options/SetInstruction.h` | 0x87 | 19 | 15 |
| `ActorRenderData` | `src-client/mc/client/renderer/actor/ActorRenderData.h` | 0x50 | 12 | 14 |
| `AttributeInstance` | `src/mc/world/attribute/AttributeInstance.h` | 0x91 | 14 | 14 |
| `Material` | `src-client/mc/client/renderer/block/tessellation_pipeline/Material.h` | 0x30 | 6 | 14 |
| `UpdatePlayerFromCameraComponent` | `src-client/mc/deps/vanilla_camera/components/UpdatePlayerFromCameraComponent.h` | 0x4 | 1 | 14 |
| `BlockPalette` | `src/mc/world/level/BlockPalette.h` | 0x80 | 4 | 13 |
| `ChunkSource` | `src/mc/world/level/chunk/ChunkSource.h` | 0x58 | 9 | 13 |
| `GameSession` | `src/mc/world/GameSession.h` | 0x61 | 7 | 13 |
| `RemoveActorPacket` | `src/mc/network/packet/RemoveActorPacket.h` | 0x4 | 1 | 13 |
| `SubChunk` | `src/mc/world/level/chunk/SubChunk.h` | 0x65 | 16 | 13 |
| `SyncedPlayerMovementSettings` | `src/mc/world/actor/player/SyncedPlayerMovementSettings.h` | 0x5 | 2 | 13 |
| `ContainerClosePacket` | `src/mc/network/packet/ContainerClosePacket.h` | 0x4 | 1 | 12 |
| `FadeInstruction` | `src/mc/deps/minecraft_camera/camera_instruction_options/FadeInstruction.h` | 0x38 | 8 | 12 |
| `InventoryTransactionItemGroup` | `src/mc/world/inventory/transaction/InventoryTransactionItemGroup.h` | 0x15 | 5 | 12 |
| `ItemStackRequestData` | `src/mc/world/inventory/network/ItemStackRequestData.h` | 0x48 | 4 | 12 |
| `LevelData` | `src/mc/world/level/storage/LevelData.h` | 0x654 | 83 | 12 |
| `MobBodyRotationComponent` | `src/mc/entity/components/MobBodyRotationComponent.h` | 0x8 | 2 | 12 |
| `ActorEquipmentComponent` | `src/mc/entity/components/ActorEquipmentComponent.h` | 0x10 | 2 | 11 |
| `ActorEventPacket` | `src/mc/network/packet/ActorEventPacket.h` | 0x4 | 1 | 11 |
| `ActorHeadRotationComponent` | `src/mc/entity/components/ActorHeadRotationComponent.h` | 0x8 | 2 | 11 |
| `ActorUniqueIDComponent` | `src/mc/entity/components/ActorUniqueIDComponent.h` | 0x8 | 1 | 11 |
| `ActorWalkAnimationComponent` | `src/mc/entity/components/ActorWalkAnimationComponent.h` | 0x14 | 5 | 11 |
| `AttributesComponent` | `src/mc/entity/components/AttributesComponent.h` | 0x50 | 1 | 11 |
| `CameraInstruction` | `src/mc/deps/minecraft_camera/CameraInstruction.h` | 0x14E | 9 | 11 |
| `ClientInputHandler` | `src-client/mc/client/input/ClientInputHandler.h` | 0x88 | 20 | 11 |
| `ClientInputMappingFactory` | `src-client/mc/client/input/ClientInputMappingFactory.h` | 0x118 | 8 | 11 |
| `JumpControlComponent` | `src/mc/entity/components_json_legacy/JumpControlComponent.h` | 0x58 | 6 | 11 |
| `LevelEventPacket` | `src/mc/network/packet/LevelEventPacket.h` | 0x4 | 1 | 11 |
| `PlayerSkinPacket` | `src/mc/network/packet/PlayerSkinPacket.h` | 0x4 | 1 | 11 |
| `ActorGameTypeComponent` | `src/mc/entity/components/ActorGameTypeComponent.h` | 0x4 | 1 | 10 |
| `CameraOrbitComponent` | `src/mc/deps/minecraft_camera/components/CameraOrbitComponent.h` | 0x4C | 16 | 10 |
| `CameraPresetComponent` | `src/mc/deps/minecraft_camera/components/CameraPresetComponent.h` | 0x8 | 1 | 10 |
| `FallDistanceComponent` | `src/mc/entity/components/FallDistanceComponent.h` | 0x6 | 3 | 10 |
| `ModalFormRequestPacket` | `src/mc/network/packet/ModalFormRequestPacket.h` | 0x4 | 1 | 10 |
| `MoveActorAbsoluteData` | `src/mc/network/packet/MoveActorAbsoluteData.h` | 0x30 | 9 | 10 |
| `OverworldGenerationRulesBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/OverworldGenerationRulesBiomeJsonComponent.h` | 0xE8 | 6 | 10 |
| `ProjectileComponent` | `src/mc/entity/components_json_legacy/ProjectileComponent.h` | 0x2FB | 74 | 10 |
| `RawMoveInputComponent` | `src/mc/entity/components/RawMoveInputComponent.h` | 0x18 | 2 | 10 |
| `SetPlayerGameTypePacket` | `src/mc/network/packet/SetPlayerGameTypePacket.h` | 0x4 | 1 | 10 |
| `AngerLevelComponent` | `src/mc/entity/components_json_legacy/AngerLevelComponent.h` | 0xD8 | 19 | 9 |
| `BaseAttributeMap` | `src/mc/world/attribute/BaseAttributeMap.h` | 0x20 | 2 | 9 |
| `CameraAvoidanceComponent` | `src/mc/deps/minecraft_camera/components/CameraAvoidanceComponent.h` | 0x40 | 6 | 9 |
| `ItemStackRequestAction` | `src/mc/world/inventory/network/ItemStackRequestAction.h` | 0x1 | 1 | 9 |
| `ItemUseSlowdownModifierComponent` | `src/mc/entity/components/ItemUseSlowdownModifierComponent.h` | 0x4 | 1 | 9 |
| `OnFireComponent` | `src/mc/entity/components/OnFireComponent.h` | 0x5 | 2 | 9 |
| `PlaySoundPacket` | `src/mc/network/packet/PlaySoundPacket.h` | 0x4 | 1 | 9 |
| `PredictedMovementComponent` | `src/mc/entity/components/PredictedMovementComponent.h` | 0x90 | 10 | 9 |
| `Reader` | `src/mc/deps/json/Reader.h` | 0x11B | 17 | 9 |
| `RenderOffsetsItemComponent` | `src/mc/deps/shared_types/legacy/item/RenderOffsetsItemComponent.h` | 0xFC | 7 | 9 |
| `ResourceLocation` | `src/mc/deps/core/resource/ResourceLocation.h` | 0x38 | 4 | 9 |
| `BlockLayer` | `src/mc/world/level/levelgen/flat/BlockLayer.h` | 0xC | 2 | 8 |
| `CommandRequestPacket` | `src/mc/network/packet/CommandRequestPacket.h` | 0x4 | 1 | 8 |
| `ContainerOpenPacket` | `src/mc/network/packet/ContainerOpenPacket.h` | 0x4 | 1 | 8 |
| `HorizontalCollisionFlagComponent` | `src/mc/deps/vanilla_components/HorizontalCollisionFlagComponent.h` | 0x2 | 2 | 8 |
| `IntOption` | `src/mc/options/option_types/IntOption.h` | 0x70 | 7 | 8 |
| `InventoryContentPacket` | `src/mc/network/packet/InventoryContentPacket.h` | 0x4 | 1 | 8 |
| `ItemReleaseInventoryTransaction` | `src/mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h` | 0x74 | 4 | 8 |
| `ItemStackRequestActionDrop` | `src/mc/world/inventory/network/ItemStackRequestActionDrop.h` | 0x1 | 1 | 8 |
| `MoveActorDeltaData` | `src/mc/network/packet/MoveActorDeltaData.h` | 0x58 | 11 | 8 |
| `MoveRequestComponent` | `src/mc/deps/vanilla_components/MoveRequestComponent.h` | 0x98 | 9 | 8 |
| `NameableComponent` | `src/mc/entity/components_json_legacy/NameableComponent.h` | 0x2 | 2 | 8 |
| `NetworkStackLatencyPacket` | `src/mc/network/packet/NetworkStackLatencyPacket.h` | 0x4 | 1 | 8 |
| `PlayerListPacket` | `src/mc/network/packet/PlayerListPacket.h` | 0x4 | 1 | 8 |
| `StationaryCameraComponent` | `src-client/mc/deps/minecraft_camera/components/StationaryCameraComponent.h` | 0x14 | 2 | 8 |
| `SubBBsComponent` | `src/mc/deps/vanilla_components/SubBBsComponent.h` | 0x18 | 1 | 8 |
| `ActorLink` | `src/mc/world/actor/ActorLink.h` | 0x20 | 6 | 7 |
| `AnimatedImageData` | `src/mc/world/actor/player/AnimatedImageData.h` | 0x3C | 4 | 7 |
| `CameraBlendStateComponent` | `src/mc/deps/minecraft_camera/components/CameraBlendStateComponent.h` | 0xA4 | 14 | 7 |
| `DryingOutTimerComponent` | `src/mc/entity/components_json_legacy/DryingOutTimerComponent.h` | 0x288 | 8 | 7 |
| `FloatOption` | `src/mc/options/option_types/FloatOption.h` | 0x14 | 5 | 7 |
| `ItemDescriptorCount` | `src/mc/world/item/ItemDescriptorCount.h` | 0x2 | 1 | 7 |
| `ItemStackBase` | `src/mc/world/item/ItemStackBase.h` | 0x3 | 3 | 7 |
| `ItemStackRequestActionTransferBase` | `src/mc/world/inventory/network/ItemStackRequestActionTransferBase.h` | 0x58 | 5 | 7 |
| `ItemStackRequestPacket` | `src/mc/network/packet/ItemStackRequestPacket.h` | 0x4 | 1 | 7 |
| `LoginPacket` | `src/mc/network/packet/LoginPacket.h` | 0x4 | 1 | 7 |
| `MountainParametersBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/MountainParametersBiomeJsonComponent.h` | 0xF2 | 8 | 7 |
| `PlayerBlockActionData` | `src/mc/entity/components/PlayerBlockActionData.h` | 0x14 | 3 | 7 |
| `PlayerBlockActions` | `src/mc/entity/components/PlayerBlockActions.h` | 0x18 | 1 | 7 |
| `ShooterItemComponent` | `src/mc/world/item/components/ShooterItemComponent.h` | 0x60 | 11 | 7 |
| `UpdatePlayerGameTypePacket` | `src/mc/network/packet/UpdatePlayerGameTypePacket.h` | 0x4 | 1 | 7 |
| `AudioEmitterComponent` | `src/mc/entity/components/AudioEmitterComponent.h` | 0x18 | 1 | 6 |
| `BlockCollisionEvaluationQueueComponent` | `src/mc/entity/components/BlockCollisionEvaluationQueueComponent.h` | 0x18 | 1 | 6 |
| `BreathableComponent` | `src/mc/entity/components_json_legacy/BreathableComponent.h` | 0x4C | 13 | 6 |
| `CameraFlyMoveComponent` | `src-client/mc/deps/minecraft_camera/components/CameraFlyMoveComponent.h` | 0x8 | 2 | 6 |
| `CameraInstructionPacket` | `src/mc/network/packet/CameraInstructionPacket.h` | 0x4 | 1 | 6 |
| `CameraLookAtPositionComponent` | `src/mc/deps/minecraft_camera/components/CameraLookAtPositionComponent.h` | 0x14 | 2 | 6 |
| `CameraOffsetComponent` | `src/mc/deps/minecraft_camera/components/CameraOffsetComponent.h` | 0x4C | 7 | 6 |
| `ClientInputLockComponent` | `src/mc/entity/components/ClientInputLockComponent.h` | 0x4 | 2 | 6 |
| `CommandOriginData` | `src/mc/server/commands/CommandOriginData.h` | 0x40 | 4 | 6 |
| `DebugInfoComponent` | `src/mc/entity/components/DebugInfoComponent.h` | 0x90 | 5 | 6 |
| `DiggerItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/DiggerItemComponent.h` | 0x130 | 6 | 6 |
| `DimensionStateComponent` | `src/mc/entity/components/DimensionStateComponent.h` | 0x10 | 2 | 6 |
| `DwellerComponent` | `src/mc/entity/components_json_legacy/DwellerComponent.h` | 0x70 | 14 | 6 |
| `EntityRegistry` | `src/mc/deps/ecs/gamerefs_entity/EntityRegistry.h` | 0x1D8 | 5 | 6 |
| `GeneticsComponent` | `src/mc/entity/components_json_legacy/GeneticsComponent.h` | 0x30 | 5 | 6 |
| `ItemComponent` | `src/mc/world/item/components/ItemComponent.h` | 0x8 | 1 | 6 |
| `ItemStackRequestBatch` | `src/mc/world/inventory/network/ItemStackRequestBatch.h` | 0x18 | 1 | 6 |
| `LegacyMolangVariableComponent` | `src/mc/entity/components/LegacyMolangVariableComponent.h` | 0x4 | 1 | 6 |
| `MingleComponent` | `src/mc/entity/components/MingleComponent.h` | 0x18 | 3 | 6 |
| `OffsetsComponent` | `src/mc/deps/vanilla_components/OffsetsComponent.h` | 0x44 | 7 | 6 |
| `OverworldHeightBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/OverworldHeightBiomeJsonComponent.h` | 0x14 | 2 | 6 |
| `PassengersToPositionComponent` | `src/mc/entity/components/PassengersToPositionComponent.h` | 0x30 | 2 | 6 |
| `PlayerChangeDimensionRequestComponent` | `src/mc/entity/components/PlayerChangeDimensionRequestComponent.h` | 0x38 | 2 | 6 |
| `PlayerPositionModeComponent` | `src/mc/entity/components/PlayerPositionModeComponent.h` | 0x1 | 1 | 6 |
| `RepairableItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/RepairableItemComponent.h` | 0x60 | 3 | 6 |
| `SerializedPersonaPieceHandle` | `src/mc/world/actor/player/SerializedPersonaPieceHandle.h` | 0x60 | 5 | 6 |
| `ServerCameraInstructionComponent` | `src/mc/entity/components/ServerCameraInstructionComponent.h` | 0x20 | 2 | 6 |
| `ServerCameraStatesComponent` | `src/mc/entity/components/ServerCameraStatesComponent.h` | 0x38 | 2 | 6 |
| `SurfaceMaterialAdjustmentsBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/SurfaceMaterialAdjustmentsBiomeJsonComponent.h` | 0x480 | 9 | 6 |
| `TintMapColor` | `src/mc/deps/core_graphics/helpers/TintMapColor.h` | 0x40 | 1 | 6 |
| `UnlockedRecipesServerComponent` | `src/mc/entity/components/UnlockedRecipesServerComponent.h` | 0xA0 | 6 | 6 |
| `UseModifiersItemComponent` | `src/mc/world/item/components/UseModifiersItemComponent.h` | 0x58 | 6 | 6 |
| `AABBRelativeSizeUpdateComponent` | `src/mc/deps/vanilla_components/AABBRelativeSizeUpdateComponent.h` | 0x30 | 4 | 5 |
| `AbilitiesComponent` | `src/mc/entity/components/AbilitiesComponent.h` | 0x5B8 | 1 | 5 |
| `AbilitiesRequestComponent` | `src/mc/entity/components/AbilitiesRequestComponent.h` | 0x18 | 1 | 5 |
| `AbsoluteSizeUpdateComponent` | `src/mc/deps/vanilla_components/AbsoluteSizeUpdateComponent.h` | 0x40 | 4 | 5 |
| `ActorDataDirtyFlagsComponent` | `src/mc/deps/vanilla_components/ActorDataDirtyFlagsComponent.h` | 0x18 | 1 | 5 |
| `ActorDefinitionIdentifierComponent` | `src/mc/entity/components/ActorDefinitionIdentifierComponent.h` | 0xB0 | 1 | 5 |
| `ActorTickNeededComponent` | `src/mc/entity/components/ActorTickNeededComponent.h` | 0x10 | 1 | 5 |
| `AddActorPacket` | `src/mc/network/packet/AddActorPacket.h` | 0x4 | 1 | 5 |
| `AddRiderComponent` | `src/mc/entity/components/AddRiderComponent.h` | 0x18 | 1 | 5 |
| `AdmireItemComponent` | `src/mc/entity/components/AdmireItemComponent.h` | 0xC0 | 4 | 5 |
| `AgeableComponent` | `src/mc/entity/components/AgeableComponent.h` | 0x5 | 2 | 5 |
| `AgentCommandComponent` | `src/mc/entity/components/AgentCommandComponent.h` | 0x8 | 1 | 5 |
| `AllowOffHandItemComponent` | `src/mc/deps/shared_types/beta/item/AllowOffHandItemComponent.h` | 0x1 | 1 | 5 |
| `AmbientSoundComponent` | `src/mc/entity/components/AmbientSoundComponent.h` | 0x10 | 4 | 5 |
| `AmbientSoundServerComponent` | `src/mc/entity/components/AmbientSoundServerComponent.h` | 0x40 | 4 | 5 |
| `AngryComponent` | `src/mc/entity/components_json_legacy/AngryComponent.h` | 0x98 | 10 | 5 |
| `AnimatePacket` | `src/mc/network/packet/AnimatePacket.h` | 0x4 | 1 | 5 |
| `AreaAttackComponent` | `src/mc/entity/components_json_legacy/AreaAttackComponent.h` | 0x98 | 9 | 5 |
| `AttackAnimationComponent` | `src/mc/entity/components/AttackAnimationComponent.h` | 0x4 | 1 | 5 |
| `AttackCooldownComponent` | `src/mc/entity/components/AttackCooldownComponent.h` | 0x9 | 2 | 5 |
| `BalloonComponent` | `src/mc/entity/components_json_legacy/BalloonComponent.h` | 0xD | 3 | 5 |
| `BarterComponent` | `src/mc/entity/components_json_legacy/BarterComponent.h` | 0x2F | 5 | 5 |
| `BaseGameVersionComponent` | `src/mc/entity/components/BaseGameVersionComponent.h` | 0x20 | 1 | 5 |
| `BehaviorComponent` | `src/mc/entity/components/BehaviorComponent.h` | 0x70 | 3 | 5 |
| `BlockBreakSensorComponent` | `src/mc/entity/components_json_legacy/BlockBreakSensorComponent.h` | 0x60 | 5 | 5 |
| `BlockMovementSlowdownAppliedComponent` | `src/mc/entity/components/BlockMovementSlowdownAppliedComponent.h` | 0x1 | 1 | 5 |
| `BlockPosTrackerComponent` | `src/mc/entity/components/BlockPosTrackerComponent.h` | 0x25 | 5 | 5 |
| `BlockSourceComponent` | `src/mc/entity/components/BlockSourceComponent.h` | 0x10 | 1 | 5 |
| `BoatMovementComponent` | `src/mc/entity/components/BoatMovementComponent.h` | 0x14 | 5 | 5 |
| `BoatPaddleComponent` | `src/mc/entity/components/BoatPaddleComponent.h` | 0x30 | 2 | 5 |
| `BodyControlComponent` | `src/mc/entity/components/BodyControlComponent.h` | 0x8 | 1 | 5 |
| `BoostableComponent` | `src/mc/entity/components_json_legacy/BoostableComponent.h` | 0x10 | 4 | 5 |
| `BossComponent` | `src/mc/entity/components_json_legacy/BossComponent.h` | 0xA8 | 12 | 5 |
| `BounceComponent` | `src/mc/entity/components/BounceComponent.h` | 0x1C | 3 | 5 |
| `BreakDoorAnnotationComponent` | `src/mc/entity/components/BreakDoorAnnotationComponent.h` | 0x30 | 6 | 5 |
| `BreedableComponent` | `src/mc/entity/components_json_legacy/BreedableComponent.h` | 0x30 | 7 | 5 |
| `BribeableComponent` | `src/mc/entity/components_json_legacy/BribeableComponent.h` | 0x14 | 4 | 5 |
| `BrushEffectsCooldownComponent` | `src/mc/entity/components/BrushEffectsCooldownComponent.h` | 0x10 | 1 | 5 |
| `BundleInteractionItemComponent` | `src/mc/deps/shared_types/v1_21_30/item/BundleInteractionItemComponent.h` | 0x4 | 1 | 5 |
| `BuoyancyComponent` | `src/mc/entity/components_json_legacy/BuoyancyComponent.h` | 0x38 | 9 | 5 |
| `BuoyancyFloatRequestComponent` | `src/mc/entity/components_json_legacy/BuoyancyFloatRequestComponent.h` | 0x2 | 2 | 5 |
| `BurnsInDaylightComponent` | `src/mc/entity/components/BurnsInDaylightComponent.h` | 0x4 | 1 | 5 |
| `CameraAPIComponent` | `src/mc/deps/minecraft_camera/CameraAPIComponent.h` | 0x8 | 1 | 5 |
| `CameraActivationRequestComponent` | `src/mc/deps/minecraft_camera/components/CameraActivationRequestComponent.h` | 0x4 | 1 | 5 |
| `CameraAdjustedPositionComponent` | `src/mc/deps/minecraft_camera/components/CameraAdjustedPositionComponent.h` | 0x24 | 3 | 5 |
| `CameraAimAssistComponent` | `src/mc/entity/components/camera/aimassist/CameraAimAssistComponent.h` | 0x41 | 5 | 5 |
| `CameraAimAssistRegistryComponent` | `src/mc/entity/components/camera/aimassist/CameraAimAssistRegistryComponent.h` | 0x80 | 2 | 5 |
| `CameraAttachComponent` | `src/mc/deps/minecraft_camera/components/CameraAttachComponent.h` | 0x48 | 2 | 5 |
| `CameraClientInstanceComponent` | `src-client/mc/deps/minecraft_camera/CameraClientInstanceComponent.h` | 0x8 | 1 | 5 |
| `CameraComfortMoveVRComponent` | `src-client/mc/deps/vanilla_camera/components/CameraComfortMoveVRComponent.h` | 0x11 | 5 | 5 |
| `CameraEntityStateComponent` | `src/mc/deps/minecraft_camera/components/CameraEntityStateComponent.h` | 0x2 | 2 | 5 |
| `CameraFadeEffectComponent` | `src-client/mc/deps/minecraft_camera/components/CameraFadeEffectComponent.h` | 0x31 | 3 | 5 |
| `CameraGlobalInstructionComponent` | `src/mc/deps/minecraft_camera/components/CameraGlobalInstructionComponent.h` | 0x18 | 4 | 5 |
| `CameraInstructionsComponent` | `src-client/mc/deps/minecraft_camera/components/CameraInstructionsComponent.h` | 0x18 | 1 | 5 |
| `CameraLiquidOffsetComponent` | `src-client/mc/deps/minecraft_camera/components/CameraLiquidOffsetComponent.h` | 0x4 | 1 | 5 |
| `CameraLookAtComponent` | `src-client/mc/deps/minecraft_camera/components/CameraLookAtComponent.h` | 0x18 | 1 | 5 |
| `CameraPerspectiveOptionComponent` | `src/mc/deps/minecraft_camera/components/CameraPerspectiveOptionComponent.h` | 0x4 | 1 | 5 |
| `CameraShakeComponent` | `src/mc/entity/components/CameraShakeComponent.h` | 0x69 | 8 | 5 |
| `CameraShakeSupportComponent` | `src-client/mc/deps/minecraft_camera/components/CameraShakeSupportComponent.h` | 0xC | 3 | 5 |
| `CameraTargetComponent` | `src-client/mc/deps/minecraft_camera/components/CameraTargetComponent.h` | 0x88 | 14 | 5 |
| `CameraTargetSettingsComponent` | `src-client/mc/deps/minecraft_camera/components/CameraTargetSettingsComponent.h` | 0x38 | 8 | 5 |
| `CameraThirdPersonBoomComponent` | `src/mc/deps/minecraft_camera/components/CameraThirdPersonBoomComponent.h` | 0xD | 3 | 5 |
| `CameraTimeComponent` | `src-client/mc/deps/minecraft_camera/components/CameraTimeComponent.h` | 0xC | 3 | 5 |
| `CameraUsageComponent` | `src-client/mc/deps/minecraft_camera/components/CameraUsageComponent.h` | 0x18 | 1 | 5 |
| `CameraVehicleRotationComponent` | `src-client/mc/deps/vanilla_camera/components/CameraVehicleRotationComponent.h` | 0x4 | 1 | 5 |
| `CanDestroyInCreativeItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/CanDestroyInCreativeItemComponent.h` | 0x1 | 1 | 5 |
| `CelebrateHuntComponent` | `src/mc/entity/components_json_legacy/CelebrateHuntComponent.h` | 0x18 | 3 | 5 |
| `ChunkPositionComponent` | `src/mc/entity/components/ChunkPositionComponent.h` | 0x8 | 1 | 5 |
| `ClientAcceptanceThresholdsComponent` | `src/mc/entity/components/ClientAcceptanceThresholdsComponent.h` | 0x36 | 5 | 5 |
| `ClientParticleInitializationComponent` | `src/mc/entity/components/ClientParticleInitializationComponent.h` | 0x18 | 1 | 5 |
| `ClientParticleTerminationComponent` | `src/mc/entity/components/ClientParticleTerminationComponent.h` | 0x28 | 1 | 5 |
| `ClientPushDimensionLoadingScreenComponent` | `src/mc/entity/components/ClientPushDimensionLoadingScreenComponent.h` | 0x4 | 1 | 5 |
| `ClientVibrationComponent` | `src/mc/entity/components/ClientVibrationComponent.h` | 0x10 | 1 | 5 |
| `ClimateBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/ClimateBiomeJsonComponent.h` | 0x1C | 3 | 5 |
| `CodebuilderComponent` | `src/mc/entity/components/CodebuilderComponent.h` | 0x1 | 1 | 5 |
| `CollisionBoxComponent` | `src/mc/entity/components_json_legacy/CollisionBoxComponent.h` | 0x8 | 1 | 5 |
| `CombatRegenerationComponent` | `src/mc/entity/components_json_legacy/CombatRegenerationComponent.h` | 0x10 | 2 | 5 |
| `CommandBlockComponent` | `src/mc/entity/components/CommandBlockComponent.h` | 0xC5 | 3 | 5 |
| `ConditionalBandwidthOptimizationComponent` | `src/mc/entity/components_json_legacy/ConditionalBandwidthOptimizationComponent.h` | 0xD0 | 4 | 5 |
| `ContainerComponent` | `src/mc/entity/components_json_legacy/ContainerComponent.h` | 0x1EC | 7 | 5 |
| `ContainerScreenContextComponent` | `src/mc/world/inventory/network/ContainerScreenContextComponent.h` | 0x28 | 1 | 5 |
| `CooldownItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/CooldownItemComponent.h` | 0x28 | 3 | 5 |
| `CurrentTickComponent` | `src/mc/entity/components/CurrentTickComponent.h` | 0x8 | 1 | 5 |
| `CustomSizeUpdateComponent` | `src/mc/deps/vanilla_components/CustomSizeUpdateComponent.h` | 0x8 | 1 | 5 |
| `DamageItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/DamageItemComponent.h` | 0x2 | 1 | 5 |
| `DamageOverTimeComponent` | `src/mc/entity/components_json_legacy/DamageOverTimeComponent.h` | 0xC | 3 | 5 |
| `DamageSensorComponent` | `src/mc/entity/components_json_legacy/DamageSensorComponent.h` | 0x28 | 4 | 5 |
| `DanceComponent` | `src/mc/entity/components_json_legacy/DanceComponent.h` | 0x8 | 1 | 5 |
| `DashCooldownTimerComponent` | `src/mc/entity/components/DashCooldownTimerComponent.h` | 0x4 | 1 | 5 |
| `DepenetrationComponent` | `src/mc/entity/components/DepenetrationComponent.h` | 0x38 | 4 | 5 |
| `DespawnComponent` | `src/mc/entity/components_json_legacy/DespawnComponent.h` | 0x8 | 1 | 5 |
| `DimensionTransitionComponent` | `src/mc/entity/components/DimensionTransitionComponent.h` | 0x24 | 4 | 5 |
| `DimensionTypeComponent` | `src/mc/entity/components/DimensionTypeComponent.h` | 0x4 | 1 | 5 |
| `DisplayNameItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/DisplayNameItemComponent.h` | 0x20 | 1 | 5 |
| `DurabilityItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/DurabilityItemComponent.h` | 0xC | 2 | 5 |
| `DyeableItemComponent` | `src/mc/deps/shared_types/v1_21_30/item/DyeableItemComponent.h` | 0x10 | 1 | 5 |
| `DynamicPropertiesComponent` | `src/mc/entity/components/DynamicPropertiesComponent.h` | 0x40 | 1 | 5 |
| `DynamicRenderOffsetComponent` | `src/mc/entity/components/DynamicRenderOffsetComponent.h` | 0x34 | 5 | 5 |
| `EconomyTradeableComponent` | `src/mc/entity/components_json_legacy/EconomyTradeableComponent.h` | 0x70 | 10 | 5 |
| `EcsEventDispatcherComponent` | `src/mc/deps/minecraft_camera/systems/EcsEventDispatcherComponent.h` | 0x10 | 1 | 5 |
| `ElytraFlightTimeTicksComponent` | `src/mc/entity/components/ElytraFlightTimeTicksComponent.h` | 0x8 | 1 | 5 |
| `EmotePlayedTelemetryDataComponent` | `src/mc/entity/components/EmotePlayedTelemetryDataComponent.h` | 0x24 | 2 | 5 |
| `EnchantableItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/EnchantableItemComponent.h` | 0x28 | 2 | 5 |
| `EntitiesPendingEnterVolumeComponent` | `src/mc/volume/components/EntitiesPendingEnterVolumeComponent.h` | 0x40 | 1 | 5 |
| `EntityArmorEquipmentSlotMappingComponent` | `src/mc/entity/components/EntityArmorEquipmentSlotMappingComponent.h` | 0x4 | 1 | 5 |
| `EntityPlacerItemComponent` | `src/mc/world/item/components/EntityPlacerItemComponent.h` | 0xF8 | 6 | 5 |
| `EntitySensorComponent` | `src/mc/entity/components_json_legacy/EntitySensorComponent.h` | 0x79 | 6 | 5 |
| `EntityStorageKeyComponent` | `src/mc/entity/components/EntityStorageKeyComponent.h` | 0x41 | 3 | 5 |
| `EquipItemComponent` | `src/mc/entity/components/EquipItemComponent.h` | 0xA0 | 3 | 5 |
| `EquippableComponent` | `src/mc/entity/components_json_legacy/EquippableComponent.h` | 0x18 | 1 | 5 |
| `EventingDispatcherComponent` | `src/mc/entity/components/EventingDispatcherComponent.h` | 0x8 | 1 | 5 |
| `EventingRequestQueueComponent` | `src/mc/entity/components/EventingRequestQueueComponent.h` | 0x28 | 1 | 5 |
| `ExecuteEventOnBlockRequestComponent` | `src/mc/entity/components/ExecuteEventOnBlockRequestComponent.h` | 0x18 | 1 | 5 |
| `ExhaustionComponent` | `src/mc/entity/components/ExhaustionComponent.h` | 0x28 | 10 | 5 |
| `ExperienceRewardComponent` | `src/mc/entity/components_json_legacy/ExperienceRewardComponent.h` | 0x31 | 3 | 5 |
| `ExplodeComponent` | `src/mc/entity/components_json_legacy/ExplodeComponent.h` | 0x2D | 17 | 5 |
| `ExternalDataComponent` | `src/mc/entity/components/ExternalDataComponent.h` | 0x8 | 1 | 5 |
| `FallDamageResultComponent` | `src/mc/entity/components/FallDamageResultComponent.h` | 0x10 | 2 | 5 |
| `FireAnimationTrackerComponent` | `src/mc/entity/components/FireAnimationTrackerComponent.h` | 0x4 | 1 | 5 |
| `FishAnimationComponent` | `src/mc/entity/components/FishAnimationComponent.h` | 0x8 | 1 | 5 |
| `FlockingComponent` | `src/mc/entity/components_json_legacy/FlockingComponent.h` | 0x90 | 28 | 5 |
| `FogAppearanceClientBiomeJsonComponent` | `src/mc/deps/shared_types/v1_21_40/clientbiome/components/FogAppearanceClientBiomeJsonComponent.h` | 0x20 | 1 | 5 |
| `FogCommandComponent` | `src/mc/entity/components/FogCommandComponent.h` | 0x18 | 1 | 5 |
| `FoodItemComponent` | `src/mc/world/item/components/FoodItemComponent.h` | 0x40 | 6 | 5 |
| `FreezingComponent` | `src/mc/entity/components/FreezingComponent.h` | 0x4 | 1 | 5 |
| `FuelItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/FuelItemComponent.h` | 0x4 | 1 | 5 |
| `GainedRaidOmenAtPositionComponent` | `src/mc/entity/components/GainedRaidOmenAtPositionComponent.h` | 0xC | 1 | 5 |
| `GameEventListenerComponent` | `src/mc/entity/components/GameEventListenerComponent.h` | 0x8 | 1 | 5 |
| `GameEventMovementTrackingComponent` | `src/mc/entity/components_json_legacy/GameEventMovementTrackingComponent.h` | 0x25 | 7 | 5 |
| `GetAttachPositionViewsComponent` | `src/mc/entity/components/GetAttachPositionViewsComponent.h` | 0x1E0 | 1 | 5 |
| `GiveableComponent` | `src/mc/entity/components_json_legacy/GiveableComponent.h` | 0x18 | 1 | 5 |
| `GlidingCollisionDamageComponent` | `src/mc/entity/components/GlidingCollisionDamageComponent.h` | 0x4 | 1 | 5 |
| `GlintItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/GlintItemComponent.h` | 0x1 | 1 | 5 |
| `GlobalTextureGroupStateComponent` | `src/mc/entity/components/GlobalTextureGroupStateComponent.h` | 0x1 | 1 | 5 |
| `GoalSelectorComponent` | `src/mc/entity/components/GoalSelectorComponent.h` | 0x18 | 1 | 5 |
| `GroupSizeComponent` | `src/mc/entity/components/GroupSizeComponent.h` | 0x10 | 2 | 5 |
| `GrowsCropComponent` | `src/mc/entity/components_json_legacy/GrowsCropComponent.h` | 0x1C | 3 | 5 |
| `HandEquippedItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/HandEquippedItemComponent.h` | 0x1 | 1 | 5 |
| `HeartbeatClientComponent` | `src/mc/entity/components/HeartbeatClientComponent.h` | 0x20 | 5 | 5 |
| `HeartbeatServerComponent` | `src/mc/entity/components/HeartbeatServerComponent.h` | 0x218 | 2 | 5 |
| `HideComponent` | `src/mc/entity/components_json_legacy/HideComponent.h` | 0x2 | 2 | 5 |
| `HitResultComponent` | `src/mc/entity/components/HitResultComponent.h` | 0x8 | 1 | 5 |
| `HitboxComponent` | `src/mc/deps/vanilla_components/HitboxComponent.h` | 0x18 | 1 | 5 |
| `HomeComponent` | `src/mc/entity/components_json_legacy/HomeComponent.h` | 0x40 | 8 | 5 |
| `HopperComponent` | `src/mc/entity/components_json_legacy/HopperComponent.h` | 0xC | 1 | 5 |
| `HorseAnimationComponent` | `src/mc/entity/components/HorseAnimationComponent.h` | 0xC | 3 | 5 |
| `HorseStandCounterComponent` | `src/mc/entity/components/HorseStandCounterComponent.h` | 0x4 | 1 | 5 |
| `HoverTextColorItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/HoverTextColorItemComponent.h` | 0x20 | 1 | 5 |
| `HumanoidMonsterAttackStateComponent` | `src/mc/entity/components/HumanoidMonsterAttackStateComponent.h` | 0x4 | 1 | 5 |
| `IconItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/IconItemComponent.h` | 0x20 | 1 | 5 |
| `InsideBlockComponent` | `src/mc/entity/components/InsideBlockComponent.h` | 0x14 | 2 | 5 |
| `InsideBlockNotifierComponent` | `src/mc/entity/components_json_legacy/InsideBlockNotifierComponent.h` | 0x18 | 1 | 5 |
| `InsideGenericBlockComponent` | `src/mc/entity/components/InsideGenericBlockComponent.h` | 0x39 | 4 | 5 |
| `InsideOnewayBlockComponent` | `src/mc/entity/components/InsideOnewayBlockComponent.h` | 0x18 | 1 | 5 |
| `InsomniaComponent` | `src/mc/entity/components_json_legacy/InsomniaComponent.h` | 0xC | 3 | 5 |
| `InteractButtonItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/InteractButtonItemComponent.h` | 0x28 | 2 | 5 |
| `InteractComponent` | `src/mc/entity/components_json_legacy/InteractComponent.h` | 0x20 | 2 | 5 |
| `IsSolidMobComponent` | `src/mc/entity/components/IsSolidMobComponent.h` | 0xE | 3 | 5 |
| `IsSolidMobNearbyComponent` | `src/mc/entity/components/IsSolidMobNearbyComponent.h` | 0xC | 1 | 5 |
| `ItemInUseComponent` | `src/mc/entity/components/ItemInUseComponent.h` | 0xC | 2 | 5 |
| `ItemStackNetManagerEnabledComponent` | `src/mc/entity/components/ItemStackNetManagerEnabledComponent.h` | 0x4 | 1 | 5 |
| `JumpTicksComponent` | `src/mc/entity/components/JumpTicksComponent.h` | 0x4 | 1 | 5 |
| `LeashableComponent` | `src/mc/entity/components_json_legacy/LeashableComponent.h` | 0xF8 | 6 | 5 |
| `LegacyTradeableComponent` | `src/mc/entity/components_json_legacy/LegacyTradeableComponent.h` | 0x58 | 10 | 5 |
| `LevelComponent` | `src/mc/entity/components/LevelComponent.h` | 0x8 | 1 | 5 |
| `LieDownAnimationComponent` | `src/mc/entity/components/LieDownAnimationComponent.h` | 0x10 | 2 | 5 |
| `LiquidClippedItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/LiquidClippedItemComponent.h` | 0x1 | 1 | 5 |
| `LoadedChunksComponent` | `src/mc/entity/components/LoadedChunksComponent.h` | 0x4 | 1 | 5 |
| `LoadingScreenPacketSenderComponent` | `src/mc/entity/components/LoadingScreenPacketSenderComponent.h` | 0x18 | 1 | 5 |
| `LoadingScreenStateChangeComponent` | `src/mc/entity/components/LoadingScreenStateChangeComponent.h` | 0x4 | 1 | 5 |
| `LoadingStateComponent` | `src/mc/entity/components/LoadingStateComponent.h` | 0x8 | 2 | 5 |
| `LocalPlayerDimensionWaitComponent` | `src/mc/entity/components/LocalPlayerDimensionWaitComponent.h` | 0x10 | 2 | 5 |
| `LocalPlayerPrePlayerTravelComponent` | `src/mc/entity/components/LocalPlayerPrePlayerTravelComponent.h` | 0x4 | 1 | 5 |
| `LodestoneCompassComponent` | `src/mc/entity/components/LodestoneCompassComponent.h` | 0x4 | 1 | 5 |
| `LookControlComponent` | `src/mc/entity/components/LookControlComponent.h` | 0x30 | 8 | 5 |
| `LookedAtComponent` | `src/mc/entity/components/LookedAtComponent.h` | 0x10 | 3 | 5 |
| `MaxStackSizeItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/MaxStackSizeItemComponent.h` | 0x2 | 1 | 5 |
| `MinecartPreNormalTickBlockPosComponent` | `src/mc/entity/components/MinecartPreNormalTickBlockPosComponent.h` | 0xC | 1 | 5 |
| `MinecraftGameShimComponent` | `src-client/mc/client/entity/components/events/MinecraftGameShimComponent.h` | 0x8 | 1 | 5 |
| `MobEffectComponent` | `src/mc/entity/components_json_legacy/MobEffectComponent.h` | 0x70 | 7 | 5 |
| `MobEffectImmunityComponent` | `src/mc/entity/components/MobEffectImmunityComponent.h` | 0x18 | 1 | 5 |
| `MobEffectsComponent` | `src/mc/entity/components/MobEffectsComponent.h` | 0x18 | 1 | 5 |
| `MobJumpComponent` | `src/mc/entity/components/MobJumpComponent.h` | 0x14 | 3 | 5 |
| `MobOnPlayerJumpRequestComponent` | `src/mc/entity/components/MobOnPlayerJumpRequestComponent.h` | 0x4 | 1 | 5 |
| `MobTravelComponent` | `src/mc/entity/components/MobTravelComponent.h` | 0x26 | 7 | 5 |
| `MountTamingComponent` | `src/mc/entity/components_json_legacy/MountTamingComponent.h` | 0xA8 | 7 | 5 |
| `MoveActorAbsolutePacket` | `src/mc/network/packet/MoveActorAbsolutePacket.h` | 0x4 | 1 | 5 |
| `MoveActorDeltaPacket` | `src/mc/network/packet/MoveActorDeltaPacket.h` | 0x4 | 1 | 5 |
| `MoveControlComponent` | `src/mc/entity/components_json_legacy/MoveControlComponent.h` | 0x30 | 6 | 5 |
| `MovementAbilitiesComponent` | `src/mc/deps/vanilla_components/MovementAbilitiesComponent.h` | 0x10 | 3 | 5 |
| `MovementAttributesComponent` | `src/mc/deps/vanilla_components/MovementAttributesComponent.h` | 0x48 | 9 | 5 |
| `MovementCorrectionTelemetryComponent` | `src/mc/entity/components/MovementCorrectionTelemetryComponent.h` | 0x24 | 3 | 5 |
| `MovementEffectsComponent` | `src/mc/entity/components/MovementEffectsComponent.h` | 0x18 | 1 | 5 |
| `MovementInterpolatorComponent` | `src/mc/entity/components/MovementInterpolatorComponent.h` | 0x26 | 8 | 5 |
| `MovementSoundComponent` | `src/mc/entity/components/MovementSoundComponent.h` | 0x10 | 5 | 5 |
| `MovementWasCorrectedComponent` | `src/mc/entity/components/MovementWasCorrectedComponent.h` | 0xD | 2 | 5 |
| `MultinoiseGenerationRulesBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/MultinoiseGenerationRulesBiomeJsonComponent.h` | 0x28 | 5 | 5 |
| `NavigationComponent` | `src/mc/entity/components_json_legacy/NavigationComponent.h` | 0x58 | 11 | 5 |
| `NetEventCallbackComponent` | `src/mc/network/NetEventCallbackComponent.h` | 0x18 | 1 | 5 |
| `NoActionTimeComponent` | `src/mc/entity/components/NoActionTimeComponent.h` | 0x4 | 1 | 5 |
| `NpcComponent` | `src/mc/entity/components_json_legacy/NpcComponent.h` | 0x169 | 11 | 5 |
| `OfferFlowerTickComponent` | `src/mc/entity/components/OfferFlowerTickComponent.h` | 0x4 | 1 | 5 |
| `OnUseItemComponent` | `src/mc/world/item/components/OnUseItemComponent.h` | 0x38 | 1 | 5 |
| `OpenDoorAnnotationComponent` | `src/mc/entity/components_json_legacy/OpenDoorAnnotationComponent.h` | 0x28 | 1 | 5 |
| `ParticleEventDispatcherComponent` | `src/mc/entity/components/ParticleEventDispatcherComponent.h` | 0x8 | 1 | 5 |
| `ParticleEventRequestQueueComponent` | `src/mc/entity/components/ParticleEventRequestQueueComponent.h` | 0x28 | 1 | 5 |
| `PassengerComponent` | `src/mc/entity/components/PassengerComponent.h` | 0x10 | 1 | 5 |
| `PassengerRenderingRidingOffsetComponent` | `src/mc/entity/components/PassengerRenderingRidingOffsetComponent.h` | 0x8 | 1 | 5 |
| `PassengerYRotLimitComponent` | `src/mc/entity/components/PassengerYRotLimitComponent.h` | 0x8 | 2 | 5 |
| `PeekComponent` | `src/mc/entity/components/PeekComponent.h` | 0x8 | 2 | 5 |
| `PendingRemovePassengersComponent` | `src/mc/entity/components/PendingRemovePassengersComponent.h` | 0x18 | 1 | 5 |
| `PickComponent` | `src/mc/entity/components/PickComponent.h` | 0x4 | 1 | 5 |
| `PlanterItemComponent` | `src/mc/world/item/components/PlanterItemComponent.h` | 0x3A | 6 | 5 |
| `PlayerActionComponent` | `src/mc/entity/components/PlayerActionComponent.h` | 0x170 | 5 | 5 |
| `PlayerDestroyProgressCacheComponent` | `src/mc/entity/components/PlayerDestroyProgressCacheComponent.h` | 0x28 | 4 | 5 |
| `PlayerDimensionTransferSaveSuspensionComponent` | `src/mc/entity/components/PlayerDimensionTransferSaveSuspensionComponent.h` | 0x28 | 1 | 5 |
| `PlayerFlyingTravelComponent` | `src/mc/entity/components/PlayerFlyingTravelComponent.h` | 0x4 | 1 | 5 |
| `PlayerInputModeComponent` | `src/mc/entity/components/PlayerInputModeComponent.h` | 0x8 | 2 | 5 |
| `PlayerInputRequestComponent` | `src/mc/entity/components/PlayerInputRequestComponent.h` | 0x26 | 16 | 5 |
| `PlayerInteractionModelComponent` | `src/mc/entity/components/PlayerInteractionModelComponent.h` | 0x5 | 2 | 5 |
| `PlayerLoadingScreenComponent` | `src/mc/entity/components/PlayerLoadingScreenComponent.h` | 0x8 | 1 | 5 |
| `PlayerMovementSettingsComponent` | `src/mc/world/actor/player/PlayerMovementSettingsComponent.h` | 0x40 | 1 | 5 |
| `PlayerPreMobTravelComponent` | `src/mc/entity/components/PlayerPreMobTravelComponent.h` | 0xC | 1 | 5 |
| `PlayerSaveSuspensionComponent` | `src/mc/world/level/storage/PlayerSaveSuspensionComponent.h` | 0x10 | 1 | 5 |
| `PortalCooldownDurationComponent` | `src/mc/entity/components/PortalCooldownDurationComponent.h` | 0x1C | 5 | 5 |
| `PositionPassengerRequestComponent` | `src/mc/entity/components/PositionPassengerRequestComponent.h` | 0x8 | 2 | 5 |
| `PostGameEventRequestComponent` | `src/mc/entity/components/PostGameEventRequestComponent.h` | 0x18 | 1 | 5 |
| `PreferredPathComponent` | `src/mc/entity/components/PreferredPathComponent.h` | 0x28 | 4 | 5 |
| `PreviousDefinitionsComponent` | `src/mc/entity/components/PreviousDefinitionsComponent.h` | 0x8 | 1 | 5 |
| `ProfanityFilterComponent` | `src/mc/entity/components/ProfanityFilterComponent.h` | 0x2 | 2 | 5 |
| `ProjectileItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/ProjectileItemComponent.h` | 0x28 | 2 | 5 |
| `PropertyComponent` | `src/mc/world/actor/state/PropertyComponent.h` | 0xA0 | 7 | 5 |
| `PushedByComponent` | `src/mc/entity/components/PushedByComponent.h` | 0x10 | 1 | 5 |
| `RaidBossComponent` | `src/mc/entity/components/RaidBossComponent.h` | 0xB0 | 11 | 5 |
| `RailMovementComponent` | `src/mc/entity/components/RailMovementComponent.h` | 0x4 | 1 | 5 |
| `RaiseArmAnimationComponent` | `src/mc/entity/components/RaiseArmAnimationComponent.h` | 0x4 | 1 | 5 |
| `RandomComponent` | `src/mc/entity/components/RandomComponent.h` | 0x8 | 1 | 5 |
| `RandomReferenceComponent` | `src/mc/entity/components/RandomReferenceComponent.h` | 0x8 | 1 | 5 |
| `RarityItemComponent` | `src/mc/deps/shared_types/v1_21_30/item/RarityItemComponent.h` | 0x20 | 1 | 5 |
| `RecordItemComponent` | `src/mc/deps/shared_types/beta/item/RecordItemComponent.h` | 0x28 | 3 | 5 |
| `RemovePassengersComponent` | `src/mc/entity/components/RemovePassengersComponent.h` | 0x18 | 1 | 5 |
| `RenderRotationComponent` | `src/mc/entity/components/RenderRotationComponent.h` | 0x8 | 1 | 5 |
| `ReplayStateComponent` | `src/mc/entity/components/ReplayStateComponent.h` | 0x20 | 7 | 5 |
| `ReplayStateTrackerComponent` | `src/mc/entity/components/ReplayStateTrackerComponent.h` | 0x68 | 1 | 5 |
| `RewindCollisionShapesComponent` | `src/mc/entity/components/RewindCollisionShapesComponent.h` | 0x60 | 3 | 5 |
| `RideableComponent` | `src/mc/entity/components_json_legacy/RideableComponent.h` | 0xA8 | 1 | 5 |
| `RidingPrevIDComponent` | `src/mc/entity/components/RidingPrevIDComponent.h` | 0x10 | 1 | 5 |
| `ScaleByAgeComponent` | `src/mc/entity/components_json_legacy/ScaleByAgeComponent.h` | 0x8 | 2 | 5 |
| `SchedulePlayerLoadingScreenComponent` | `src/mc/entity/components/SchedulePlayerLoadingScreenComponent.h` | 0x18 | 1 | 5 |
| `SchedulerComponent` | `src/mc/entity/components_json_legacy/SchedulerComponent.h` | 0x4 | 1 | 5 |
| `SendPacketsComponent` | `src/mc/entity/components/SendPacketsComponent.h` | 0x18 | 1 | 5 |
| `ServerActiveCameraComponent` | `src/mc/entity/components/ServerActiveCameraComponent.h` | 0xC | 1 | 5 |
| `ServerAnticipateClientLoadingScreenComponent` | `src/mc/entity/components/ServerAnticipateClientLoadingScreenComponent.h` | 0x18 | 1 | 5 |
| `ServerCatchupMovementTrackerComponent` | `src/mc/entity/components/ServerCatchupMovementTrackerComponent.h` | 0xC | 1 | 5 |
| `ServerPlayerCurrentMovementComponent` | `src/mc/entity/components/ServerPlayerCurrentMovementComponent.h` | 0x105 | 5 | 5 |
| `ServerPlayerInteractComponent` | `src/mc/entity/components/ServerPlayerInteractComponent.h` | 0x58 | 1 | 5 |
| `ServerPlayerInventoryTransactionComponent` | `src/mc/entity/components/ServerPlayerInventoryTransactionComponent.h` | 0x18 | 1 | 5 |
| `ServerPlayerMovementComponent` | `src/mc/entity/components/ServerPlayerMovementComponent.h` | 0x48 | 6 | 5 |
| `ShooterComponent` | `src/mc/entity/components_json_legacy/ShooterComponent.h` | 0xD9 | 6 | 5 |
| `ShouldDespawnItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/ShouldDespawnItemComponent.h` | 0x1 | 1 | 5 |
| `ShulkerPeekAmountComponent` | `src/mc/entity/components/ShulkerPeekAmountComponent.h` | 0x8 | 1 | 5 |
| `SkyColorClientBiomeJsonComponent` | `src/mc/deps/shared_types/v1_21_40/clientbiome/components/SkyColorClientBiomeJsonComponent.h` | 0x10 | 1 | 5 |
| `SlotDropChancesComponent` | `src/mc/entity/components/SlotDropChancesComponent.h` | 0x38 | 3 | 5 |
| `SnapOnRailComponent` | `src/mc/entity/components/SnapOnRailComponent.h` | 0x26 | 6 | 5 |
| `SneakingComponent` | `src/mc/entity/components/SneakingComponent.h` | 0x4 | 1 | 5 |
| `SneezeComponent` | `src/mc/entity/components/SneezeComponent.h` | 0x4 | 1 | 5 |
| `SoundEventPlayerComponent` | `src/mc/entity/components/SoundEventPlayerComponent.h` | 0x8 | 1 | 5 |
| `SoundEventRequestQueueComponent` | `src/mc/entity/components/SoundEventRequestQueueComponent.h` | 0x28 | 1 | 5 |
| `SpawnActorComponent` | `src/mc/entity/components_json_legacy/SpawnActorComponent.h` | 0x18 | 1 | 5 |
| `SpecialCameraModeComponent` | `src-client/mc/deps/minecraft_camera/components/SpecialCameraModeComponent.h` | 0x1 | 1 | 5 |
| `SpinAttackResultsComponent` | `src/mc/entity/components/SpinAttackResultsComponent.h` | 0x18 | 1 | 5 |
| `StackedByDataItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/StackedByDataItemComponent.h` | 0x1 | 1 | 5 |
| `StandAnimationComponent` | `src/mc/entity/components/StandAnimationComponent.h` | 0x8 | 2 | 5 |
| `StepSoundFrequencyComponent` | `src/mc/entity/components/StepSoundFrequencyComponent.h` | 0x8 | 2 | 5 |
| `StorageItemComponent` | `src/mc/world/item/components/StorageItemComponent.h` | 0x78 | 8 | 5 |
| `SuspectTrackingComponent` | `src/mc/entity/components/SuspectTrackingComponent.h` | 0x20 | 2 | 5 |
| `SwimAmountComponent` | `src/mc/entity/components/SwimAmountComponent.h` | 0x8 | 2 | 5 |
| `SynchedActorDataComponent` | `src/mc/entity/components/SynchedActorDataComponent.h` | 0x48 | 1 | 5 |
| `TagsBiomeJsonComponent` | `src/mc/deps/shared_types/v1_20_60/biome/components/TagsBiomeJsonComponent.h` | 0x18 | 1 | 5 |
| `TagsItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/TagsItemComponent.h` | 0x18 | 1 | 5 |
| `TameableComponent` | `src/mc/entity/components_json_legacy/TameableComponent.h` | 0x20 | 2 | 5 |
| `TargetCameraOrientationComponent` | `src/mc/deps/minecraft_camera/components/TargetCameraOrientationComponent.h` | 0x10 | 1 | 5 |
| `TargetCameraRotationLimitComponent` | `src-client/mc/deps/minecraft_camera/components/TargetCameraRotationLimitComponent.h` | 0x8 | 2 | 5 |
| `TargetCameraRotationSpeedComponent` | `src-client/mc/deps/minecraft_camera/components/TargetCameraRotationSpeedComponent.h` | 0x4 | 1 | 5 |
| `TargetNearbyComponent` | `src/mc/entity/components/TargetNearbyComponent.h` | 0x8 | 4 | 5 |
| `TeleportComponent` | `src/mc/entity/components_json_legacy/TeleportComponent.h` | 0x70 | 11 | 5 |
| `ThrowableItemComponent` | `src/mc/world/item/components/ThrowableItemComponent.h` | 0x28 | 8 | 5 |
| `TickWorldComponent` | `src/mc/entity/components_json_legacy/TickWorldComponent.h` | 0x20 | 5 | 5 |
| `TimerComponent` | `src/mc/entity/components/TimerComponent.h` | 0x40 | 9 | 5 |
| `TintColorComponent` | `src/mc/entity/components/TintColorComponent.h` | 0x20 | 2 | 5 |
| `TradeResupplyComponent` | `src/mc/entity/components/TradeResupplyComponent.h` | 0x1 | 1 | 5 |
| `TrailComponent` | `src/mc/entity/components/TrailComponent.h` | 0x14 | 2 | 5 |
| `TransformationComponent` | `src/mc/deps/shared_types/v1_26_20/block/components/TransformationComponent.h` | 0x3C | 5 | 5 |
| `TriggerJumpRequestComponent` | `src/mc/entity/components/TriggerJumpRequestComponent.h` | 0x10 | 1 | 5 |
| `TripodCameraActivatedComponent` | `src/mc/entity/components/TripodCameraActivatedComponent.h` | 0x1C | 2 | 5 |
| `TrustComponent` | `src/mc/entity/components/TrustComponent.h` | 0x40 | 1 | 5 |
| `TrustingComponent` | `src/mc/entity/components/TrustingComponent.h` | 0x4 | 1 | 5 |
| `UnlockedRecipesClientComponent` | `src/mc/entity/components/UnlockedRecipesClientComponent.h` | 0xC8 | 5 | 5 |
| `UpdateAndRenderThrottleComponent` | `src/mc/entity/components/UpdateAndRenderThrottleComponent.h` | 0x2 | 2 | 5 |
| `UpdateBlockSyncedPacket` | `src/mc/network/packet/UpdateBlockSyncedPacket.h` | 0x4 | 1 | 5 |
| `UpdateWaterStateRequestComponent` | `src/mc/entity/components/UpdateWaterStateRequestComponent.h` | 0x10 | 5 | 5 |
| `UseAnimationItemComponent` | `src/mc/deps/shared_types/v1_20_50/item/UseAnimationItemComponent.h` | 0x1 | 1 | 5 |
| `UserEntityIdentifierComponent` | `src/mc/entity/components/UserEntityIdentifierComponent.h` | 0x250 | 5 | 5 |
| `VanillaCameraAPIComponent` | `src-client/mc/deps/vanilla_camera/VanillaCameraAPIComponent.h` | 0x8 | 1 | 5 |
| `VanillaClientGameplayComponent` | `src/mc/deps/vanilla_components/VanillaClientGameplayComponent.h` | 0x50 | 18 | 5 |
| `VanillaOffsetComponent` | `src/mc/entity/components/VanillaOffsetComponent.h` | 0x24 | 3 | 5 |
| `VariableMaxAutoStepComponent` | `src/mc/entity/components/VariableMaxAutoStepComponent.h` | 0xD | 4 | 5 |
| `VehicleComponent` | `src/mc/entity/components/VehicleComponent.h` | 0x18 | 1 | 5 |
| `VehicleInputIntentComponent` | `src/mc/deps/vanilla_components/VehicleInputIntentComponent.h` | 0x48 | 13 | 5 |
| `VehicleRenderingRidingOffsetComponent` | `src/mc/entity/components/VehicleRenderingRidingOffsetComponent.h` | 0x18 | 1 | 5 |
| `VibrationDataComponent` | `src/mc/entity/components/VibrationDataComponent.h` | 0x18 | 2 | 5 |
| `VibrationListenerComponent` | `src/mc/entity/components/VibrationListenerComponent.h` | 0x10 | 1 | 5 |
| `VolumeBoundsComponent` | `src/mc/volume/components/VolumeBoundsComponent.h` | 0x1C | 3 | 5 |
| `VolumeCreationDataComponent` | `src/mc/volume/components/VolumeCreationDataComponent.h` | 0x40 | 2 | 5 |
| `WalkDistComponent` | `src/mc/entity/components/WalkDistComponent.h` | 0x8 | 2 | 5 |
| `WardenSpawnTrackerComponent` | `src/mc/entity/components/WardenSpawnTrackerComponent.h` | 0xC | 3 | 5 |
| `WaterAppearanceClientBiomeJsonComponent` | `src/mc/deps/shared_types/v1_21_40/clientbiome/components/WaterAppearanceClientBiomeJsonComponent.h` | 0x1C | 2 | 5 |
| `WaterMovementComponent` | `src/mc/entity/components/WaterMovementComponent.h` | 0x4 | 1 | 5 |
| `WaterSplashEffectRequestComponent` | `src/mc/entity/components/WaterSplashEffectRequestComponent.h` | 0x4 | 1 | 5 |
| `WeaponItemComponent` | `src/mc/world/item/components/WeaponItemComponent.h` | 0xA8 | 3 | 5 |
| `WearableItemComponent` | `src/mc/world/item/components/WearableItemComponent.h` | 0x20 | 4 | 5 |
| `WitherBossPreAIStepResultComponent` | `src/mc/entity/components/WitherBossPreAIStepResultComponent.h` | 0x1 | 1 | 5 |

## Класс есть, но поля не описаны (layout неизвестен)

| Класс | Хидер | Упоминаний |
|---|---|---|
| `Inventory` | `src/mc/world/actor/player/Inventory.h` | 174 |
| `Type` | `src/mc/world/level/poi/Type.h` | 164 |
| `Packet` | `src/mc/deps/raknet/Packet.h` | 129 |
| `MinecraftPackets` | `src/mc/network/MinecraftPackets.h` | 123 |
| `Color` | `src/mc/deps/core/math/Color.h` | 106 |
| `Event` | `src/mc/codebuilder/utils/Event.h` | 66 |
| `HitResult` | `src/mc/world/phys/HitResult.h` | 59 |
| `EntityId` | `src/mc/deps/ecs/EntityId.h` | 51 |
| `UUID` | `src/mc/platform/UUID.h` | 48 |
| `Tag` | `src/mc/deps/nbt/Tag.h` | 36 |
| `MinecraftGame` | `src-client/mc/client/game/MinecraftGame.h` | 35 |
| `EntityContext` | `src/mc/deps/ecs/gamerefs_entity/EntityContext.h` | 20 |
| `Container` | `src/mc/editor/datastore/container/Container.h` | 19 |
| `CompoundTag` | `src/mc/deps/nbt/CompoundTag.h` | 18 |
| `ConnectionRequest` | `src/mc/network/ConnectionRequest.h` | 16 |
| `Image` | `src/mc/deps/core/image/Image.h` | 15 |
| `ItemStackRequestSlotInfo` | `src/mc/world/inventory/network/ItemStackRequestSlotInfo.h` | 13 |
| `WeakEntityRef` | `src/mc/deps/ecs/WeakEntityRef.h` | 6 |
| `ArmSize` | `src/mc/world/actor/player/persona/ArmSize.h` | 5 |
| `SubChunkStorage` | `src/mc/world/level/chunk/SubChunkStorage.h` | 5 |
| `ByteArrayTag` | `src/mc/deps/nbt/ByteArrayTag.h` | 4 |
| `ByteTag` | `src/mc/deps/nbt/ByteTag.h` | 4 |
| `CompoundTagVariant` | `src/mc/deps/nbt/CompoundTagVariant.h` | 4 |
| `DoubleTag` | `src/mc/deps/nbt/DoubleTag.h` | 4 |
| `EndTag` | `src/mc/deps/nbt/EndTag.h` | 4 |
| `FloatTag` | `src/mc/deps/nbt/FloatTag.h` | 4 |
| `HistoricalFrameTimes` | `src-client/mc/client/renderer/HistoricalFrameTimes.h` | 4 |
| `Int64Tag` | `src/mc/deps/nbt/Int64Tag.h` | 4 |
| `IntArrayTag` | `src/mc/deps/nbt/IntArrayTag.h` | 4 |
| `IntTag` | `src/mc/deps/nbt/IntTag.h` | 4 |
| `ListTag` | `src/mc/deps/nbt/ListTag.h` | 4 |
| `SemVersion` | `src/mc/deps/core/sem_ver/SemVersion.h` | 4 |
| `ShortTag` | `src/mc/deps/nbt/ShortTag.h` | 4 |
| `StringTag` | `src/mc/deps/nbt/StringTag.h` | 4 |
| `BinaryStream` | `src/mc/deps/core/utility/BinaryStream.h` | 3 |
| `DataItem` | `src/mc/world/actor/DataItem.h` | 3 |
| `ItemStackRequestActionSwap` | `src/mc/world/inventory/network/ItemStackRequestActionSwap.h` | 3 |
| `WebToken` | `src/mc/deps/certificates/WebToken.h` | 3 |
| `ChunkBlockPos` | `src/mc/world/level/ChunkBlockPos.h` | 2 |
| `ActorDataBoundingBoxComponent` | `src/mc/deps/vanilla_components/ActorDataBoundingBoxComponent.h` | 1 |
| `AdultRidingHeightOffsetComponent` | `src/mc/entity/components/AdultRidingHeightOffsetComponent.h` | 1 |
| `AttributeRequestComponent` | `src/mc/entity/components/AttributeRequestComponent.h` | 1 |
| `BalloonableComponent` | `src/mc/entity/components_json_legacy/BalloonableComponent.h` | 1 |
| `BreakBlocksComponent` | `src/mc/entity/components_json_legacy/BreakBlocksComponent.h` | 1 |
| `BucketableComponent` | `src/mc/entity/components_json_legacy/BucketableComponent.h` | 1 |
| `CameraAimAssistRequestComponent` | `src-client/mc/entity/components/camera/aimassist/CameraAimAssistRequestComponent.h` | 1 |
| `HealableComponent` | `src/mc/entity/components_json_legacy/HealableComponent.h` | 1 |
| `IllagerBeastBlockedComponent` | `src/mc/entity/components_json_legacy/IllagerBeastBlockedComponent.h` | 1 |
| `ImitateMobSoundsComponent` | `src/mc/entity/components/ImitateMobSoundsComponent.h` | 1 |
| `InsideBubbleColumnBlockComponent` | `src/mc/entity/components/InsideBubbleColumnBlockComponent.h` | 1 |
| `InstantDespawnComponent` | `src/mc/entity/components_json_legacy/InstantDespawnComponent.h` | 1 |
| `ManagedWanderingTraderComponent` | `src/mc/entity/components_json_legacy/ManagedWanderingTraderComponent.h` | 1 |
| `RailActivatorComponent` | `src/mc/entity/components_json_legacy/RailActivatorComponent.h` | 1 |
| `ShareableComponent` | `src/mc/entity/components_json_legacy/ShareableComponent.h` | 1 |
| `ShouldUpdateBoundingBoxRequestComponent` | `src/mc/entity/components/ShouldUpdateBoundingBoxRequestComponent.h` | 1 |
| `SitComponent` | `src/mc/entity/components_json_legacy/SitComponent.h` | 1 |
| `VillageManagerComponent` | `src/mc/entity/components/VillageManagerComponent.h` | 1 |

## Класс не найден в 1.26 (переименован/удалён/кастомный)

| Класс | Упоминаний |
|---|---|
| `std` | 5867 |
| `name` | 779 |
| `PacketID` | 307 |
| `AuthInputAction` | 153 |
| `entt` | 145 |
| `hash` | 93 |
| `SItemType` | 86 |
| `EffectType` | 83 |
| `ActorFlags` | 75 |
| `ChunkPos` | 63 |
| `BuildPlatform` | 60 |
| `ActorType` | 58 |
| `BlockLegacy` | 50 |
| `ContainerID` | 48 |
| `ActionType` | 41 |
| `MinecraftSim` | 33 |
| `MoveInputFlag` | 30 |
| `InventorySourceType` | 29 |
| `CameraMode` | 27 |
| `MinecraftJson` | 26 |
| `RenderCameraComponent` | 25 |
| `Component` | 23 |
| `ContainerType` | 23 |
| `SerializedSkin` | 23 |
| `Options` | 21 |
| `PositionMode` | 20 |
| `KeyboardMouseSettings` | 19 |
| `ValueType` | 19 |
| `GameType` | 17 |
| `PlayerActionType` | 17 |
| `CameraRenderPlayerModelComponent` | 16 |
| `MainView` | 16 |
| `TeleportationCause` | 16 |
| `InputMode` | 14 |
| `SubChunkBlockStorage` | 14 |
| `Bone` | 13 |
| `TextPacketType` | 13 |
| `bgfx_context` | 12 |
| `ActorPartModel` | 11 |
| `MobHurtTimeComponent` | 11 |
| `ModalFormCancelReason` | 11 |
| `BaseDescriptor` | 10 |
| `CameraRenderFirstPersonObjectsComponent` | 10 |
| `RedirectCameraInputComponent` | 10 |
| `CZString` | 9 |
| `Certificate` | 9 |
| `UIProfanityContext` | 9 |
| `BedrockPlatformUWP` | 8 |
| `ActorDataFlagComponent` | 7 |
| `ActorDataPacket` | 7 |
| `CameraAlignWithTargetForwardComponent` | 7 |
| `OnGroundFlagComponent` | 7 |
| `Simulation` | 7 |
| `TriggerType` | 7 |
| `Deleter` | 6 |
| `InputData` | 6 |
| `MaxAutoStepComponent` | 6 |
| `RenderPositionComponent` | 6 |
| `WasOnGroundFlagComponent` | 6 |
| `BlockMovementSlowdownMultiplierComponent` | 5 |
| `EaseOption` | 5 |
| `IsDeadFlagComponent` | 5 |
| `SetMovingFlagRequestComponent` | 5 |
| `ShapeType` | 5 |
| `bgfx_rendercontexti` | 5 |
| `AnimationExpression` | 4 |
| `ColorOption` | 4 |
| `ContainerEnumName` | 4 |
| `CurrentInputCameraComponent` | 4 |
| `DebugCameraIsActiveComponent` | 4 |
| `DuplicationPolicy` | 4 |
| `FacingOption` | 4 |
| `GameCameraComponent` | 4 |
| `InventorySourceFlags` | 4 |
| `ItemEntry` | 4 |
| `ItemTag` | 4 |
| `MessageId` | 4 |
| `MobIsJumpingFlagComponent` | 4 |
| `PieceType` | 4 |
| `PlayerStateAffectsRenderingComponent` | 4 |
| `PredictedResult` | 4 |
| `PropertySyncFloatEntry` | 4 |
| `PropertySyncIntEntry` | 4 |
| `RotOption` | 4 |
| `ShadowOffsetComponent` | 4 |
| `TimeOption` | 4 |
| `TrustedSkinFlag` | 4 |
| `UnverifiedCertificate` | 4 |
| `ActiveCameraComponent` | 3 |
| `ActorAddedFlagComponent` | 3 |
| `ActorChunkMoveFlagComponent` | 3 |
| `ActorDataHorseFlagComponent` | 3 |
| `ActorHeadInWaterFlagComponent` | 3 |
| `ActorHeadWasInWaterFlagComponent` | 3 |
| `ActorIsBeingDestroyedFlagComponent` | 3 |
| `ActorIsFirstTickFlagComponent` | 3 |
| `ActorIsImmobileFlagComponent` | 3 |
| `ActorIsKnockedBackOnDeathFlagComponent` | 3 |
| `ActorLinkType` | 3 |
| `ActorLocalPlayerEntityMovedFlagComponent` | 3 |
| `ActorRemovedFlagComponent` | 3 |
| `AgentFlagComponent` | 3 |
| `AirTravelFlagComponent` | 3 |
| `AllowInsideBlockRenderComponent` | 3 |
| `AnimatedTextureType` | 3 |
| `AntiCheatRewindFlagComponent` | 3 |
| `ArmorFlyEnabledFlagComponent` | 3 |
| `AutoClimbTravelFlagComponent` | 3 |
| `AutoStepRequestFlagComponent` | 3 |
| `BatFlagComponent` | 3 |
| `BeeFlagComponent` | 3 |
| `BlazeFlagComponent` | 3 |
| `BlockUpdateFlag` | 3 |
| `BoatFlagComponent` | 3 |
| `BreaksFallingBlocksFlagComponent` | 3 |
| `CamelFlagComponent` | 3 |
| `CanSeeInvisibleFlagComponent` | 3 |
| `CanStandOnSnowFlagComponent` | 3 |
| `CanVehicleSprintFlagComponent` | 3 |
| `ChickenFlagComponent` | 3 |
| `ClientPlayMode` | 3 |
| `CollidableMobFlagComponent` | 3 |
| `CollidableMobNearFlagComponent` | 3 |
| `CollisionFlagComponent` | 3 |
| `CommandOriginType` | 3 |
| `Compressibility` | 3 |
| `DashJumpFlagComponent` | 3 |
| `DiscardFrictionFlagComponent` | 3 |
| `DolphinFlagComponent` | 3 |
| `EjectedByActivatorRailFlagComponent` | 3 |
| `EnderDragonFlagComponent` | 3 |
| `EnderManFlagComponent` | 3 |
| `EntityNeedsInitializeFlagComponent` | 3 |
| `ExitFromPassengerFlagComponent` | 3 |
| `ExperienceOrbFlagComponent` | 3 |
| `ExtendPlayerRenderingComponent` | 3 |
| `EyeOfEnderFlagComponent` | 3 |
| `FallingBlockFlagComponent` | 3 |
| `FireworksRocketFlagComponent` | 3 |
| `FishFlagComponent` | 3 |
| `FishingHookFlagComponent` | 3 |
| `FreezeImmuneFlagComponent` | 3 |
| `GlidingTravelFlagComponent` | 3 |
| `GroundTravelFlagComponent` | 3 |
| `GuardianFlagComponent` | 3 |
| `HangingActorFlagComponent` | 3 |
| `HasLightweightFamilyFlagComponent` | 3 |
| `HasTeleportedFlagComponent` | 3 |
| `HorseFlagComponent` | 3 |
| `HorseLandedOnGroundFlagComponent` | 3 |
| `IgnoresEntityInsideFlagComponent` | 3 |
| `IllagerBeastFlagComponent` | 3 |
| `InteractPreventDefaultFlagComponent` | 3 |
| `InternalType` | 3 |
| `IsChasingDuringPlayFlagComponent` | 3 |
| `IsFishableFlagComponent` | 3 |
| `IsHorizontalPoseFlagComponent` | 3 |
| `IsNearDolphinsFlagComponent` | 3 |
| `IsPanickingFlagComponent` | 3 |
| `ItemActorFlagComponent` | 3 |
| `JoinRaidQueuedFlagComponent` | 3 |
| `KeepRidingEvenIfTooLargeForVehicleFlagComponent` | 3 |
| `LavaSlimeFlagComponent` | 3 |
| `LavaTravelFlagComponent` | 3 |
| `LevitateTravelFlagComponent` | 3 |
| `LiquidTravelFlagComponent` | 3 |
| `MinecartFlagComponent` | 3 |
| `MinecraftBind` | 3 |
| `MobAllowStandSlidingFlagComponent` | 3 |
| `MobFlagComponent` | 3 |
| `MobIsImmobileFlagComponent` | 3 |
| `MonsterFlagComponent` | 3 |
| `MoveTowardsClosestSpaceFlagComponent` | 3 |
| `NeedSetPreviousPositionFlagComponent` | 3 |
| `NeedsUpgradeToBodySlotFlagComponent` | 3 |
| `NeverChangesSizeFlagComponent` | 3 |
| `NewInteractionModel` | 3 |
| `OldAssureFinder` | 3 |
| `PacketHandlerDispatcher` | 3 |
| `PacketPriority` | 3 |
| `PaintingFlagComponent` | 3 |
| `PandaFlagComponent` | 3 |
| `ParrotFlagComponent` | 3 |
| `PassengersChangedFlagComponent` | 3 |
| `PermissionFlyFlagComponent` | 3 |
| `PlayerIsSleepingFlagComponent` | 3 |
| `PosOption` | 3 |
| `PowerJumpFlagComponent` | 3 |
| `PrevPosRotSetThisTickFlagComponent` | 3 |
| `PrimedTntFlagComponent` | 3 |
| `ProjectileFlagComponent` | 3 |
| `Reliability` | 3 |
| `RemoveInPeacefulFlagComponent` | 3 |
| `ReplayStateLenderFlagComponent` | 3 |
| `ScanForDolphinFlagComponent` | 3 |
| `SheepFlagComponent` | 3 |
| `ShouldAwardWhoNeedsRocketsAchievementFlagComponent` | 3 |
| `ShulkerBulletFlagComponent` | 3 |
| `ShulkerFlagComponent` | 3 |
| `SimulatedPlayerFlagComponent` | 3 |
| `SkeletonFlagComponent` | 3 |
| `SkipBodySlotUpgradeFlagComponent` | 3 |
| `SkipPlayerTickSystemFlagComponent` | 3 |
| `SlimeFlagComponent` | 3 |
| `SoulSpeedEnchantFlagComponent` | 3 |
| `SpiderFlagComponent` | 3 |
| `SquidFlagComponent` | 3 |
| `StandOnHoneyOrSlimeBlockFlagComponent` | 3 |
| `StandOnOtherBlockFlagComponent` | 3 |
| `SwitchingVehiclesFlagComponent` | 3 |
| `TagMemoryChunk` | 3 |
| `ThrownTridentFlagComponent` | 3 |
| `TropicalFishFlagComponent` | 3 |
| `VerticalCollisionFlagComponent` | 3 |
| `VexFlagComponent` | 3 |
| `VillagerV2FlagComponent` | 3 |
| `WasHandledBySculkCatalystFlagComponent` | 3 |
| `WasInLavaFlagComponent` | 3 |
| `WasInWaterFlagComponent` | 3 |
| `WaterAnimalFlagComponent` | 3 |
| `WaterTravelFlagComponent` | 3 |
| `WitchFlagComponent` | 3 |
| `WitherBossFlagComponent` | 3 |
| `WitherSkullFlagComponent` | 3 |
| `WolfFlagComponent` | 3 |
| `bgfx_d3d12_RendererContextD3D12` | 3 |
| `AbsorptionAttribute` | 2 |
| `ActorDataIDs` | 2 |
| `ActorModel` | 2 |
| `CameraBobComponent` | 2 |
| `CameraFirstPersonComponent` | 2 |
| `CameraThirdPersonComponent` | 2 |
| `CameraWorldSpaceRotationComponent` | 2 |
| `DataItemType` | 2 |
| `FollowRangeAttribute` | 2 |
| `HealthAttribute` | 2 |
| `ImageFormat` | 2 |
| `ImageUsage` | 2 |
| `MovementAttribute` | 2 |
| `PlayerAuthActionType` | 2 |
| `PlayerExperienceAttribute` | 2 |
| `PlayerHungerAttribute` | 2 |
| `PlayerLevelAttribute` | 2 |
| `PlayerListPacketType` | 2 |
| `PlayerSaturationAttribute` | 2 |
| `ServerAuthMovementMode` | 2 |
| `SmartAssureFinder` | 2 |
| `StringStorage` | 2 |
| `TargetCameraSetInitialOrientationComponent` | 2 |
| `AbilitiesDirtyComponent` | 1 |
| `ActionOrStopComponent` | 1 |
| `ActorComponent` | 1 |
| `ActorDataControllingSeatIndexComponent` | 1 |
| `ActorDataHorseTypeComponent` | 1 |
| `ActorDataJumpDurationComponent` | 1 |
| `ActorDataSeatOffsetComponent` | 1 |
| `ActorDiedComponent` | 1 |
| `ActorInBubbleColumnComponent` | 1 |
| `ActorLimitedLifetimeComponent` | 1 |
| `ActorMovementTickNeededComponent` | 1 |
| `ActorSetPositionRequestComponent` | 1 |
| `ActorTickedComponent` | 1 |
| `AirSpeedComponent` | 1 |
| `ArmorItemComponent` | 1 |
| `ArmorStandPoseIndexComponent` | 1 |
| `AutonomousActorComponent` | 1 |
| `BlockClimberComponent` | 1 |
| `CameraAimAssistBlockTargetsComponent` | 1 |
| `CameraAimAssistCachedDataComponent` | 1 |
| `CameraAimAssistCachedPositionDataComponent` | 1 |
| `CameraAimAssistCategoryUpdaterComponent` | 1 |
| `CameraAimAssistResultComponent` | 1 |
| `CameraAimAssistTickComponent` | 1 |
| `CameraIgnoreInstructionValuesComponent` | 1 |
| `CameraIgnoreStartingValuesComponent` | 1 |
| `CameraLocalSpaceRotationComponent` | 1 |
| `CameraOutOfRangeWarningSentComponent` | 1 |
| `CameraPortalDistortionComponent` | 1 |
| `CameraSleepVignetteComponent` | 1 |
| `CameraStartingValuesComponent` | 1 |
| `CanJoinRaidComponent` | 1 |
| `CanMakeAudibleSoundsComponent` | 1 |
| `CappedSurfaceBiomeJsonComponent` | 1 |
| `ClientParticleTrackingComponent` | 1 |
| `ComparisonEventingCapComponent` | 1 |
| `ControlledByLocalInstanceComponent` | 1 |
| `CurrentLocalMoveVelocityComponent` | 1 |
| `CurrentlyImmuneToFallDamageComponent` | 1 |
| `DamageNearbyMobsComponent` | 1 |
| `DashComponent` | 1 |
| `DeathCameraComponent` | 1 |
| `DefaultInputCameraComponent` | 1 |
| `DimensionBoundComponent` | 1 |
| `DisplayEntityComponent` | 1 |
| `EditorActorPauseTickNeededComponent` | 1 |
| `EditorActorPausedComponent` | 1 |
| `EnvironmentSensorComponent` | 1 |
| `FallFlyTicksComponent` | 1 |
| `FrictionModifierComponent` | 1 |
| `FrozenOceanSurfaceBiomeJsonComponent` | 1 |
| `GallopSoundCounterComponent` | 1 |
| `GameplayAffectsFovComponent` | 1 |
| `GlobalActorComponent` | 1 |
| `GlobalActorRenderComponent` | 1 |
| `HorseWasOnGroundPreTravelComponent` | 1 |
| `HurtOnConditionComponent` | 1 |
| `ImmuneToLavaDragComponent` | 1 |
| `InsideSlowingSweetBerryBushBlockComponent` | 1 |
| `InsideWebBlockComponent` | 1 |
| `InteractAction` | 1 |
| `ItemStackLegacyRequestIdTag` | 1 |
| `ItemStackNetIdTag` | 1 |
| `ItemStackRequestIdTag` | 1 |
| `JumpFromGroundRequestComponent` | 1 |
| `JumpPendingScaleComponent` | 1 |
| `JumpRidingScaleComponent` | 1 |
| `LavaSlimeJumpRequestComponent` | 1 |
| `LocalConstBlockSourceFactoryComponent` | 1 |
| `LocalMoveVelocityComponent` | 1 |
| `LocalPlayerComponent` | 1 |
| `LocalPlayerJumpRequestComponent` | 1 |
| `LocalSpatialEntityFetcherFactoryComponent` | 1 |
| `MakesLavaStepSoundComponent` | 1 |
| `MatchType` | 1 |
| `MesaSurfaceBiomeJsonComponent` | 1 |
| `MobAnimationComponent` | 1 |
| `MobRotationComponent` | 1 |
| `MovementSpeedComponent` | 1 |
| `NetworkComponent` | 1 |
| `OtherJumpRequestComponent` | 1 |
| `OutOfControlComponent` | 1 |
| `OverrideCameraComponent` | 1 |
| `ParseOption` | 1 |
| `PermanentSkipMobAiStepComponent` | 1 |
| `PermanentSkipMobTravelComponent` | 1 |
| `PermanentSkipNormalTickComponent` | 1 |
| `PersistSitComponent` | 1 |
| `PersistentComponent` | 1 |
| `PhysicsComponent` | 1 |
| `PlayerActionAcceptanceComponent` | 1 |
| `PlayerAudioListenerComponent` | 1 |
| `PlayerComponent` | 1 |
| `PlayerCurrentTickComponent` | 1 |
| `PlayerLastPosComponent` | 1 |
| `PlayerTickComponent` | 1 |
| `PostSplashGameEventRequestComponent` | 1 |
| `PostTickPositionDeltaComponent` | 1 |
| `PushActorsRequestComponent` | 1 |
| `PushableComponent` | 1 |
| `RaidTriggerComponent` | 1 |
| `RecalculateControlledByLocalInstanceRequestComponent` | 1 |
| `RemotePlayerComponent` | 1 |
| `RemoveAllPassengersRequestComponent` | 1 |
| `ResetTargetRequestComponent` | 1 |
| `RidingHeightComponent` | 1 |
| `RollCounterComponent` | 1 |
| `SaveSurroundingChunksComponent` | 1 |
| `ScanForDolphinTimerComponent` | 1 |
| `SendMotionToServerComponent` | 1 |
| `SensingComponent` | 1 |
| `ServerPlayerComponent` | 1 |
| `ServerPlayerInitialLoadingComponent` | 1 |
| `ShouldBeSimulatedComponent` | 1 |
| `ShouldStopEmotingRequestComponent` | 1 |
| `SkipAiStepComponent` | 1 |
| `SkipMobTravelComponent` | 1 |
| `SkipNormalTickComponent` | 1 |
| `SlimeWasOnGroundPreNormalTickComponent` | 1 |
| `SquidJumpRequestComponent` | 1 |
| `StopRidingRequestComponent` | 1 |
| `SurfaceParametersBiomeJsonComponent` | 1 |
| `SwampSurfaceBiomeJsonComponent` | 1 |
| `SwimSpeedMultiplierComponent` | 1 |
| `TargetCameraOutOfRangeComponent` | 1 |
| `TheEndSurfaceBiomeJsonComponent` | 1 |
| `TransientComponent` | 1 |
| `TripodCameraActivateComponent` | 1 |
| `TripodCameraComponent` | 1 |
| `UsesDefaultStepSoundComponent` | 1 |
| `VRMoveAdjustAngleComponent` | 1 |
| `VibrationDamperComponent` | 1 |
| `WasControlledByLocalInstanceComponent` | 1 |
| `WaterWalkSpeedEnchantComponent` | 1 |
| `WindChargeKnockbackComponent` | 1 |
| `WingFlapComponent` | 1 |
