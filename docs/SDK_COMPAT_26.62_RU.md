# SDK-совместимость: 1.21.44 (корень) → 1.26.5x (`26.62/`)

Сгенерировано `tools/sdk_compat.py` (только чтение, ничего не меняет).
Сравниваются наборы объявлений в `src/SDK/**` и `26.62/src/SDK/**`; для каждого
пропавшего имени ниже показано, где оно реально используется вне SDK — это и есть
список правок при переносе модулей.

- SDK-заголовков: старых 110, новых 112, общих 104
- Только в старой базе: **6** (наш код в SDK, надо переносить руками)
- Только в новой базе: **8** (новые фичи базы, идут «бесплатно»)
- Полностью пропавших идентификаторов: **31**
- Из них реально используются в наших модулях/хуках: **6**
- Переехавших в другой класс/заголовок (имя живо, хозяин сменился): **4**

## 1. Заголовки SDK только в старой базе (перенести как есть)

| Заголовок |
|---|
| `SDK/IDA_Analysis.hpp` |
| `SDK/Minecraft/KeyboardMouseSettings.hpp` |
| `SDK/Minecraft/Network/Packets/TransferPacket.hpp` |
| `SDK/Minecraft/Network/Packets/UpdatePlayerGameTypePacket.hpp` |
| `SDK/Minecraft/Rendering/ItemRenderer.hpp` |
| `SDK/Minecraft/Rendering/TexturePtr.hpp` |

## 2. Заголовки SDK только в новой базе (появились в 26.62)

| Заголовок |
|---|
| `SDK/EnttHashs.hpp` |
| `SDK/Minecraft/Actor/Components/MobJumpComponent.hpp` |
| `SDK/Minecraft/KeyboardRemappingLayout.hpp` |
| `SDK/Minecraft/ptr/SharedCounter.hpp` |
| `SDK/Minecraft/ptr/SharedPtr.hpp` |
| `SDK/Minecraft/ptr/WeakPtr.hpp` |
| `SDK/heap_rd.hpp` |
| `SDK/heap_rd_impl.hpp` |

## 3. Пропавшие идентификаторы, которые реально используются

Колонка «статус»: файл уже перенесён в `26.62/` и обращение починено (✅),
перенесён но ещё использует старое имя (⚠️) или ещё не перенесён (—).

| Идентификатор | Был в | Использований | Статус | Где |
|---|---|---|---|---|
| `read` | `Minecraft/Network/Packets/Packet.hpp` | 11 | ⚠️ 10 из 11 | `Features/Modules/Movement/DebugFly.hpp:33`<br>`Features/Modules/Movement/DebugFly.hpp:34`<br>`Features/Modules/Movement/DebugFly.hpp:35`<br>`Features/Modules/Movement/DebugFly.hpp:36`<br>`Features/Modules/Movement/DebugFly.hpp:37`<br>`Features/Modules/Movement/DebugFly.hpp:39`<br>…ещё 5 |
| `blockReadPtr` | `Minecraft/World/Chunk/SubChunk.hpp` | 6 | ⚠️ 3 из 6 | `Features/Modules/Combat/InfiniteChestAura.cpp:210`<br>`Features/Modules/Player/OreMiner.cpp:378`<br>`Features/Modules/Player/OreMinerV2.cpp:485`<br>`Features/Modules/Player/OreMinerV2.cpp:2250`<br>`Features/Modules/Visual/BlockESP.cpp:208`<br>`Features/Modules/Visual/SmartXRay.hpp:229` |
| `subchunkIndex` | `Minecraft/World/Chunk/SubChunk.hpp` | 6 | ⚠️ 3 из 6 | `Features/Modules/Combat/InfiniteChestAura.cpp:229`<br>`Features/Modules/Player/OreMiner.cpp:384`<br>`Features/Modules/Player/OreMinerV2.cpp:503`<br>`Features/Modules/Player/OreMinerV2.cpp:550`<br>`Features/Modules/Visual/BlockESP.cpp:223`<br>`Features/Modules/Visual/SmartXRay.hpp:243` |
| `mUnknown` | `Minecraft/Network/Packets/TextPacket.hpp` | 2 | ✅ 2 | `Features/Events/NametagRenderEvent.hpp:13`<br>`Features/Events/NametagRenderEvent.hpp:19` |
| `AuthorityMode` | `Minecraft/Actor/SyncedPlayerMovementSettings.hpp` | 2 | ✅ 2 | `Features/Modules/Misc/TestModule.cpp:683`<br>`Features/Modules/Misc/TestModule.cpp:683` |
| `mMinimum` | `Minecraft/Options.hpp` | 2 | ✅ 2 | `Features/Modules/Visual/Zoom.cpp:14`<br>`Features/Modules/Visual/Zoom.cpp:26` |

## 3b. Имена, которые остались в новой базе, но у другого класса

Компилятор поругается только если обращаться к ним через старый тип. Проверять при правке.

| Идентификатор | Был в (старый) | Есть в новой (заголовок) | Использований |
|---|---|---|---|
| `tick` | `Minecraft/Actor/GameMode.hpp` | `Minecraft/Actor/Components/AttributesComponent.hpp` | 57 |
| `write` | `Minecraft/Network/Packets/Packet.hpp` | `Minecraft/Inventory/CompoundTag.hpp` | 21 |
| `toString` | `Minecraft/Actor/SerializedSkin.hpp` | `Minecraft/JSON.hpp`<br>`Minecraft/mce.hpp` | 7 |
| `mPlayFabId` | `Minecraft/Actor/SerializedSkin.hpp` | — | 7 |

## 4. Пропавшие идентификаторы без использования (можно игнорировать)

—

## 5. Риск по файлам (сколько обращений к пропавшему API)

| Файл | Обращений |
|---|---|
| `Features/Modules/Movement/Speed.cpp` | 21 |
| `Utils/AudioUtils.cpp` | 13 |
| `Features/Modules/Movement/DebugFly.hpp` | 8 |
| `Features/Modules/Misc/SkinBlinker.cpp` | 5 |
| `Features/Modules/Player/Derp.cpp` | 5 |
| `Features/Modules/Combat/Criticals.cpp` | 4 |
| `Features/Modules/Combat/PearlStopper.cpp` | 4 |
| `Features/Modules/Misc/SkinStealer.cpp` | 4 |
| `Features/Modules/Player/OreMinerV2.cpp` | 4 |
| `Features/Modules/Combat/PearlStopper.hpp` | 3 |
| `Features/Modules/Misc/PacketLogger.cpp` | 3 |
| `Features/Modules/Player/Scaffold.hpp` | 3 |
| `Features/Events/NametagRenderEvent.hpp` | 2 |
| `Features/Modules/Combat/InfiniteChestAura.cpp` | 2 |
| `Features/Modules/Misc/TestModule.cpp` | 2 |
| `Features/Modules/Movement/Speed.hpp` | 2 |
| `Features/Modules/Player/ChainTP.hpp` | 2 |
| `Features/Modules/Player/OreMiner.cpp` | 2 |
| `Features/Modules/Visual/BlockESP.cpp` | 2 |
| `Features/Modules/Visual/SmartXRay.hpp` | 2 |
| `Features/Modules/Visual/Zoom.cpp` | 2 |
| `Utils/FileUtils.cpp` | 2 |
| `Utils/SysUtils/ExceptionHandler.cpp` | 2 |
| `Features/Command/Commands/FakeChatCommand.cpp` | 1 |
| `Features/Modules/Combat/Criticals.hpp` | 1 |
| `Features/Modules/Misc/CostumeSpammer.cpp` | 1 |
| `Features/Modules/Misc/DeviceSpoof.cpp` | 1 |
| `Features/Modules/Misc/PartySpammer.cpp` | 1 |
| `Features/Modules/Misc/SkinBlinker.hpp` | 1 |
| `Features/Modules/Player/AutoArmor.hpp` | 1 |
| `Features/Modules/Player/Cage.hpp` | 1 |
| `Features/Modules/Player/Derp.hpp` | 1 |
| `Features/Modules/Player/Nuker.hpp` | 1 |
| `Features/Modules/Player/OreMiner.hpp` | 1 |
| `Features/Modules/Player/PlayerTracker.hpp` | 1 |
| `Features/Modules/Player/SpawnerAura.hpp` | 1 |
| `Features/Modules/Player/Timer.hpp` | 1 |
| `Features/Modules/Player/ZipLine.hpp` | 1 |
| `Features/Modules/Visual/BlockESP.hpp` | 1 |
| `Features/Modules/Visual/WorldParticles.hpp` | 1 |
| `Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.cpp` | 1 |
| `Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.hpp` | 1 |
| `Utils/MiscUtils/ClientPack.cpp` | 1 |
| `Utils/MiscUtils/PngWriter.hpp` | 1 |
| `Utils/MiscUtils/SoundUtils.cpp` | 1 |

## 6. Что добавилось в общих заголовках (куда переезжать)

### `SDK/Minecraft/Actor/Actor.hpp`

`getMobJumpComponent`, `setPos`

### `SDK/Minecraft/Actor/ActorPartModel.hpp`

`PAD`

### `SDK/Minecraft/Actor/Components/AttributesComponent.hpp`

`getInstance`, `mAttribute`, `mCurrentMaxValue`, `mCurrentMinValue`, `mDefaultMaxValue`, `mDefaultMinValue`, `mDefaultValue`, `mValueChangedDuringTick`

### `SDK/Minecraft/Actor/Components/MoveInputComponent.hpp`

`AnalogMoveVector`, `FlagValues`, `LookNormalDirField`, `LookSlightDirField`, `LookSmoothDirField`, `isDown`, `isJumping`, `isLeft`, `isRight`, `isSneaking`, `isSprinting`, `isUp`, `mCameraOrientation`, `mDisplacement`, `mDisplacementDelta`, `mFlagValues`, `mHoldAutoJumpInWaterTicks`, `mInputState`, `mInteractDir`, `mIsPaddling`, `mLookDelta`, `mMove`, `mRawInputState`, `mRawMove`, `set`, `setDown`, `setLeft`, `setRight`, `setSneaking`, `setSprinting`, `setUp`

### `SDK/Minecraft/Actor/GameMode.hpp`

`AttackPos`, `callVirtualFunc`

### `SDK/Minecraft/Actor/SerializedSkin.hpp`

`ThreadId`, `ThreadOwner`

### `SDK/Minecraft/ClientInstance.hpp`

`CLASS_FIELD`, `getPlatformOnlineId`

### `SDK/Minecraft/Inventory/Item.hpp`

`getArmorTier`

### `SDK/Minecraft/Inventory/ItemStack.hpp`

`GetProcessHeap`, `HeapFree`, `WeakPtr`, `mValid_DeprecatedSeeComment`

### `SDK/Minecraft/MinecraftGame.hpp`

`getxuid`

### `SDK/Minecraft/Network/ConnectionRequest.hpp`

`mIsSelfSigned`

### `SDK/Minecraft/Network/LoopbackPacketSender.hpp`

`isInitialized`, `sendTo`

### `SDK/Minecraft/Network/Packets/AnimatePacket.hpp`

`mSwingSource`

### `SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp`

`mClientCooldownState`, `mItem`, `mItemDescriptor`, `mVariantTransaction`

### `SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp`

`mRawMoveVector`

### `SDK/Minecraft/Network/Packets/TextPacket.hpp`

`getMessage`, `getSource`, `gettype`, `mBody`, `mPlatform`

### `SDK/Minecraft/Options.hpp`

`mClampToRange`, `mDelta`, `mImpl`, `mMaxValue`, `mMinValue`

### `SDK/Minecraft/Rendering/LevelRenderer.hpp`

`setFovX`, `setFovY`

### `SDK/Minecraft/World/BlockLegacy.hpp`

`mType`

### `SDK/Minecraft/World/Chunk/LevelChunk.hpp`

`PAD`, `mFinalized`, `mLoadState`, `mPositionX`, `mPositionY`

### `SDK/Minecraft/World/Chunk/SubChunk.hpp`

`PAD`, `mAbsoluteIndex`, `mHasMaxSkyLight`, `mHash`, `mHashDirty`, `mIsInitialized`, `mIsReplacementSubChunk`, `mNeedsClientLighting`, `mNeedsInitLighting`, `mRenderChunkTrackingVersionNumber`

### `SDK/Minecraft/World/HitResult.hpp`

`getType`, `getTypeString`, `mEntityAABB`

### `SDK/Minecraft/World/Level.hpp`

`mBlockFromRuntimeId`, `mColor`, `mIsHost`, `mIsSubClient`, `mIsTeacher`, `mLegacyBlockStatesWarningMutex`, `mLegacyBlockStatesWarningSet`, `mLevel`, `mNameLookup`, `mSkin`, `set`

