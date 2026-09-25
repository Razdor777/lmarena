# Аудит миграции Solstice 1.21.44 → 1.26 (LeviLamina)

- Проект: `lmarena` (Solstice, гейм-версия 1.21.44)
- Заголовки LeviLamina: `/home/user/LeviLamina`
- Проиндексировано классов в хидерах: **20521**
- Проверено целей: **88** (хуков: 27, сигнатур/полей: 61)
- Найдено по имени в 1.26: **38** (43%) — из них точно в том же классе: **38** (транков `$`: 6), в другом классе (на проверку): **0**

Статусы:

- `FOUND` — метод/поле с таким именем есть в заголовке 1.26 (адрес резолвит symdb)
- `FOUND_THUNK` — найден `$`-транк: виртуальную функцию хукают через `&Class::$method`
- `FOUND_ELSEWHERE` — в «своём» классе имени нет, но оно найдено в другом классе (переехало)
- `RENAMED` — переименовано в 1.26, новый символ найден и подтверждён вручную
- `MOVED` — переехало в другой класс/компонент (или заменяется событием), найдено вручную
- `NO_LAYOUT` — класс есть, но поля не описаны: оффсет только из IDA
- `GONE` — в 1.26 нет (удалено или переименовано неизвестно куда): реверс
- `EMPTY_CLASS` — класс/структура в хидерах пустая (`struct X {};`) — layout неизвестен, только реверс
- `NOT_IN_CLASS` — класс есть, но такого члена нет (переименовано/удалено/переехало)
- `CLASS_MISSING` — класса с таким именем в хидерах 1.26 нет вообще

## Хуки

| Цель в проекте (1.21.44) | Статус | LeviLamina 26.51 | Объявление | Примечание |
|---|---|---|---|---|
| `Actor::baseTick` | FOUND | `src/mc/world/actor/Actor.h:1254` | `MCAPI void $baseTick();` | готовый символ |
| `ActorAnimationControllerPlayer::applyToPose` | FOUND | `src/mc/world/actor/animation/ActorAnimationControllerPlayer.h:122` | `MCAPI void $applyToPose(` | символ есть, но сигнатура изменилась: (ApplyAnimationContext const&, RenderParams&, unordered_map<SkeletalHierarchyIndex, vector<BoneOrientation>>&, float) — тело хука переписывать |
| `BlockSource::fireBlockChanged` | FOUND | `src/mc/world/level/BlockSource.h:705` | `MCAPI void $fireBlockChanged(` |  |
| `ClientInstance::isPreGame` | FOUND | `src-client/mc/client/game/ClientInstance.h:1755` | `MCAPI bool $isPreGame() const;` | готовый символ + транк $isPreGame |
| `ContainerScreenController::tick` | FOUND | `src-client/mc/client/gui/screens/controllers/ContainerScreenController.h:320` | `MCAPI ::ui::DirtyFlag $tick();` | готовый символ |
| `HoverTextRenderer::render` | FOUND | `src-client/mc/client/gui/controls/renderers/HoverTextRenderer.h:60` | `MCAPI void $render(::MinecraftUIRenderContext& renderContext, ::IClientInstance& client…` |  |
| `ItemRenderer::render` | FOUND | `src-client/mc/client/renderer/actor/ItemRenderer.h:245` | `MCAPI void $render(::BaseActorRenderContext& renderContext, ::ActorRenderData& entityRe…` |  |
| `LoopbackPacketSender::send` | FOUND | `src/mc/network/LoopbackPacketSender.h:64` | `MCAPI void $send(::Packet& packet);` | готовый символ (send/sendTo/sendToServer) |
| `MinecraftUIRenderContext::drawImage` | FOUND | `src-client/mc/client/renderer/screen/MinecraftUIRenderContext.h:299` | `MCAPI void $drawImage(` |  |
| `RakNet::RakPeer::GetLastPing` | FOUND | `src/mc/deps/raknet/RakPeer.h:788` | `MCAPI int $GetLastPing(::RakNet::AddressOrGUID const systemIdentifier) const;` | RakNet в хидерах ещё есть (src/mc/deps/raknet/RakPeer.h), но появился src/mc/external/webrtc — транспорт менялся, проверять вживую |
| `RakNet::RakPeer::RunUpdateCycle` | FOUND | `src/mc/deps/raknet/RakPeer.h:887` | `MCAPI bool $RunUpdateCycle(::RakNet::BitStream& updateBitStream);` | см. RakNet::RakPeer::GetLastPing |
| `ActorRenderDispatcher::render` | FOUND_THUNK | `src-client/mc/client/renderer/actor/ActorRenderDispatcher.h:93` | `MCAPI void render(::BaseActorRenderContext& entityRenderContext, ::Actor& entity, bool …` |  |
| `ConnectionRequest::create` | FOUND_THUNK | `src/mc/network/ConnectionRequest.h:68` | `MCAPI static ::ConnectionRequest create(` |  |
| `MouseDevice::feed` | FOUND_THUNK | `src-client/mc/deps/input/MouseDevice.h:30` | `feed(char actionButtonId, schar buttonData, short x, short y, short dx, short dy, bool …` | альтернатива без хука: ll::event::input::MouseInputEvent |
| `RakNet::RakPeer::SendImmediate` | FOUND_THUNK | `src/mc/deps/raknet/RakPeer.h:615` | `MCAPI bool SendImmediate(` | в RakPeer.h есть Send + транк $Send; SendImmediate ищите рядом |
| `ScreenView::setupAndRender` | FOUND_THUNK | `src-client/mc/client/gui/screens/ScreenView.h:319` | `MCAPI void render(::UIRenderContext& uiRenderContext);` | в 1.26 это ScreenView::render; LeviLamina уже хукает его и даёт Before/AfterUIRenderEvent (src-client/ll/api/event/render) \| переименовано: render |
| `PacketHandlerDispatcherInstance<` | MOVED | `src/mc/network/PacketHandlerDispatcherInstance.h:` | `—` |  имя в Detour() обрезано, уточни шаблонные параметры |
| `bobHurt` | MOVED | `LevelRendererPlayer::bobHurt(Matrix&, float)  (src-client/mc/client/renderer/game/LevelRendererPlayer.h:365):` | `—` | метод переехал из анонимного класса в LevelRendererPlayer |
| `entityHealthChanged` | RENAMED | `onActorHealthChanged  (src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:127):` | `—` | это scripting-событие; для нативного хука ищи Actor::_hurt / heal |
| `entityHurt` | RENAMED | `Actor::_hurt(ActorDamageSource const&, float, HurtParameters const&)  (src/mc/world/actor/Actor.h:612, транк $_hurt:1478):` | `—` | сигнатура изменилась: появился третий аргумент HurtParameters |
| `projectileHitBlock` | RENAMED | `onProjectileHitBlock  (IScriptWorldAfterEvents.h:341) + ProjectileHitEvent:` | `—` | scripting-событие; нативный вариант — хук ProjectileComponent / Actor::_onHit |
| `projectileHitEntity` | RENAMED | `onProjectileHitEntity  (IScriptWorldAfterEvents.h:344) + ProjectileHitEvent:` | `—` | см. projectileHitBlock |
| `Keyboard::feed` | MOVED | `ll::event::input::KeyInputEvent + ll::input::KeyRegistry  (src-client/ll/api/event/input/):` | `—` | в 1.26 вместо хука клавиатуры лучше взять готовое событие LeviLamina |
| `Mob::getCurrentSwingDuration` | RENAMED | `Mob::getModifiedSwingDuration  (src/mc/world/actor/Mob.h:338):` | `—` | в 1.26 это int getModifiedSwingDuration(); Item::getSwingDuration (Item.h:231) — длительность взмаха предмета, НЕ моба: для хука бери Mob::getModifiedSwingDuration |
| `mce::framebuilder::RenderItemInHandDescription::RenderItemInHandDescription` | EMPTY_CLASS | `src/mc/deps/minecraft_renderer/framebuilder/RenderItemInHandDescription.h:7` | `struct RenderItemInHandDescription {};` | структура в хидерах пустая (layout неизвестен) — только ручной реверс |
| `CameraDirectLookSystemUtil::_handleLookInput` | GONE | `CameraDirectLookComponent / CameraDirectLookDefinition  (src-client/mc/deps/minecraft_camera/components/, src/mc/deps/shared_types/v1_21_100/camera/):` | `—` | класс переехал на компонентную систему камеры — реверс |
| `Unknown::renderNametag` | GONE | `NameTagRenderObject / NameTagRenderer  (src-client/mc/deps/minecraft_renderer/objects/NameTagRenderObject.h, src-client/mc/client/gui/controls/renderers/NameTagRenderer.h):` | `—` | в 1.26 неймтеги — объекты рендера, отдельной функции нет |

## Сигнатуры и поля (OffsetProvider.hpp)

| Цель в проекте (1.21.44) | Статус | LeviLamina 26.51 | Объявление | Примечание |
|---|---|---|---|---|
| `Actor_baseTick` | FOUND | `src/mc/world/actor/Actor.h:1254` | `MCAPI void $baseTick();` |  |
| `Actor_mLevel` | FOUND | `src/mc/world/actor/Actor.h:176` | `[[nodiscard]] Level& getLevel() const { return *reinterpret_cast<Level*>(mLevel); }` |  |
| `Actor_swing` | FOUND | `src/mc/world/actor/Actor.h:1426` | `MCFOLD bool $swing(::ActorSwingSource swingSource, ::HandSlot handSlot);` |  |
| `BlockSource_clip` | FOUND | `src/mc/world/level/BlockSource.h:686` | `MCAPI ::HitResult $clip(` |  |
| `BlockSource_getChunk` | FOUND | `src/mc/world/level/BlockSource.h:581` | `MCAPI ::LevelChunk* $getChunk(int x, int z) const;` |  |
| `BlockSource_setBlock` | FOUND | `src/mc/world/level/BlockSource.h:730` | `MCAPI bool $setBlock(` |  |
| `ClientInputHandler_mMappingFactory` | FOUND | `src-client/mc/client/input/ClientInputHandler.h:39` | `::ll::TypedStorage<8, 8, ::std::unique_ptr<::ClientInputMappingFactory>> mMappingFactory;` | закомментировано в проекте |
| `ClientInstance_getLocalPlayer` | FOUND | `src-client/mc/client/game/ClientInstance.h:1472` | `MCAPI ::LocalPlayer* $getLocalPlayer() const;` |  |
| `ClientInstance_getMouseGrabbed` | FOUND | `src-client/mc/client/game/ClientInstance.h:1707` | `MCAPI bool $getMouseGrabbed() const;` |  |
| `ClientInstance_getOptions` | FOUND | `src-client/mc/client/game/ClientInstance.h:1761` | `MCAPI ::IOptionRegistry& $getOptions();` |  |
| `ClientInstance_getScreenName` | FOUND | `src-client/mc/client/game/ClientInstance.h:1909` | `MCAPI ::std::string $getScreenName() const;` | закомментировано в проекте |
| `ClientInstance_grabMouse` | FOUND | `src-client/mc/client/game/ClientInstance.h:2043` | `MCAPI void $grabMouse();` |  |
| `ClientInstance_releaseMouse` | FOUND | `src-client/mc/client/game/ClientInstance.h:2045` | `MCAPI void $releaseMouse();` |  |
| `ClientInstance_setDisableInput` | FOUND | `src-client/mc/client/game/ClientInstance.h:2041` | `MCAPI void $setDisableInput(bool disableInput);` |  |
| `ContainerManagerModel_getContainerType` | FOUND | `src/mc/world/containers/managers/models/ContainerManagerModel.h:161` | `MCFOLD ::SharedTypes::Legacy::ContainerType $getContainerType() const;` |  |
| `Level_getBlockPalette` | FOUND | `src/mc/world/level/Level.h:1766` | `MCFOLD ::BlockPalette const& $getBlockPalette() const;` |  |
| `Level_getHitResult` | FOUND | `src/mc/world/level/Level.h:2443` | `MCAPI ::HitResult& $getHitResult();` |  |
| `Level_getLevelData` | FOUND | `src/mc/world/level/Level.h:1941` | `MCFOLD ::LevelData& $getLevelData();` |  |
| `Level_getPlayerList` | FOUND | `src/mc/world/level/Level.h:2411` | `MCAPI ::std::unordered_map<::mce::UUID, ::PlayerListEntry> const& $getPlayerList() const;` |  |
| `Level_getPlayerMovementSettings` | FOUND | `src/mc/world/level/Level.h:2340` | `MCAPI ::PlayerMovementSettings const& $getPlayerMovementSettings() const;` |  |
| `LevelChunk_mSubChunks` | FOUND | `src/mc/world/level/chunk/LevelChunk.h:188` | `::ll::TypedStorage<8, 24, ::std::vector<::SubChunk>>               mSubChunks;` |  |
| `Container_getItem` | FOUND_THUNK | `src/mc/world/Container.h:62` | `[[nodiscard]] ItemStack const& operator[](int index) const { return this->getItem(index…` |  |
| `Actor_mContainerManagerModel` | MOVED | `PlayerInventory::mHudContainerManager  (weak_ptr<HudContainerManagerModel>, src/mc/world/actor/player/PlayerInventory.h:26):` | `—` | контейнер-менеджер живёт в PlayerInventory |
| `Actor_mDestroying` | MOVED | `actor data flags (ActorDataFlagComponent):` | `—` | см. Actor_mSwinging |
| `Actor_mGameMode` | MOVED | `ECS ActorGameTypeComponent  (src/mc/entity/components/ActorGameTypeComponent.h):` | `—` | gamemode ушёл в ECS-компонент, бери через getEntityContext() |
| `Actor_mHurtTimeComponent` | MOVED | `ECS MobHurtTimeComponent : IntComponent  (src/mc/entity/components/MobHurtTimeComponent.h):` | `—` | hurt time ушёл в ECS-компонент (mValue) |
| `Actor_mSupplies` | MOVED | `ECS ActorEquipmentComponent  (src/mc/entity/components/ActorEquipmentComponent.h):` | `—` | mHand/mArmor — unique_ptr<SimpleContainer> |
| `Actor_mSwinging` | MOVED | `actor data flags (ActorDataFlagComponent / getStatusFlag(ActorFlags::Swinging)):` | `—` | флаги актора в 1.26 — биты в ECS-компоненте, не поле Actor |
| `ClientInstance_getBlockSource` | RENAMED | `ClientInstance::getRegion()  (src-client/mc/client/game/ClientInstance.h:475, транк $getRegion:1470):` | `—` | getBlockSource переименован в getRegion |
| `ClientInstance_getInputHandler` | RENAMED | `ClientInstance::getInput() -> ClientInputHandler*  (ClientInstance.h:1040); getMinecraftInput() (968):` | `—` | getInputHandler -> getInput |
| `ClientInstance_mGuiData` | MOVED | `ClientInstance::getGuiData()  (ClientInstance.h:857/859):` | `—` | см. mLevelRenderer |
| `ClientInstance_mLevelRenderer` | MOVED | `ClientInstance::getLevelRenderer()  (ClientInstance.h:796, $getLevelRenderer:1787):` | `—` | поля класса не описаны (в 1.26 у ClientInstance всего 2 описанных поля) — бери через геттер, а не по оффсету |
| `ClientInstance_mPacketSender` | MOVED | `ClientInstance::getPacketSender()  (ClientInstance.h:994):` | `—` | см. mLevelRenderer |
| `ContainerManagerModel_getSlot` | RENAMED | `ContainerManagerModel::getFullContainerSlot(int, FullContainerName const&)  (src/mc/world/containers/managers/models/ContainerManagerModel.h:107, $getFullContainerSlot:182):` | `—` | getSlot -> getFullContainerSlot, добавился аргумент FullContainerName |
| `GameSession_mEventCallback` | RENAMED | `GameSession::getNetEventCallback() / mLegacyClientNetworkHandler  (src/mc/world/GameSession.h:30, 60):` | `—` | mEventCallback -> getNetEventCallback() |
| `LevelData_mTick` | RENAMED | `LevelData::mCurrentTick  (Tick, src/mc/world/level/storage/LevelData.h:77):` | `—` | mTick -> mCurrentTick (тип Tick, 8 байт) |
| `LevelRenderer_mRendererPlayer` | RENAMED | `LevelRenderer::mLevelRendererPlayer  (shared_ptr<LevelRendererPlayer>, src-client/mc/client/renderer/game/LevelRenderer.h:125):` | `—` | mRendererPlayer -> mLevelRendererPlayer, тип shared_ptr |
| `LevelRendererPlayer_mCameraPos` | MOVED | `LevelRendererCamera::mCameraPos  (Vec3, src-client/mc/client/renderer/game/LevelRendererCamera.h:263):` | `—` | позиция камеры переехала в LevelRendererCamera |
| `LevelRendererPlayer_mFovX` | RENAMED | `LevelRendererPlayer::mFov  (float, src-client/mc/client/renderer/game/LevelRendererPlayer.h:131):` | `—` | в 1.26 один float mFov (есть ещё mOFov — предыдущее значение); вертикальный FOV считается из aspect ratio |
| `LevelRendererPlayer_mFovY` | RENAMED | `LevelRendererPlayer::mOFov  (float, LevelRendererPlayer.h:132):` | `—` | в 1.26 отдельного «FovY» нет: mFov + mOFov (предыдущее) |
| `PlayerInventory_mContainer` | RENAMED | `PlayerInventory::mInventory  (unique_ptr<Inventory>, src/mc/world/actor/player/PlayerInventory.h:24):` | `—` | mContainer -> mInventory |
| `BlockSource_mBuildHeight` | NO_LAYOUT | `src/mc/world/level/BlockSource.h:` | `—` | поля нет; есть getHeight()/getHeightmapPos(); высота мира — DimensionHeightRange.h |
| `ClientInputMappingFactory_mKeyboardMouseSettings` | NO_LAYOUT | `src-client/mc/client/input/ClientInputMappingFactory.h:` | `—` | полей не описано — реверс |
| `ClientInstance_mMinecraftSim` | NO_LAYOUT | `src-client/mc/client/game/ClientInstance.h:` | `—` | в хидере описаны только mUITexture/mUICursorTexture — оффсет только из IDA |
| `MinecraftGame_mClientInstances` | NO_LAYOUT | `src-client/mc/client/game/MinecraftGame.h:` | `—` | полей не описано; геттеры primaryClientInstance ищи в MinecraftGame.h — реверс |
| `MinecraftGame_mMouseGrabbed` | NO_LAYOUT | `src-client/mc/client/game/MinecraftGame.h:` | `—` | полей не описано; мышь — ClientInstance::grabMouse()/isMouseGrabbed() |
| `MinecraftGame_mProfanityContext` | NO_LAYOUT | `src-client/mc/client/game/MinecraftGame.h:` | `—` | полей не описано — реверс |
| `MinecraftGame_playUi` | NO_LAYOUT | `src-client/mc/client/game/MinecraftGame.h:` | `—` | у MinecraftGame в 1.26 не описано НИ ОДНОГО поля, метода playUi нет — реверс |
| `bgfx_context_m_renderCtx` | NO_LAYOUT | `src-client/mc/external/bgfx/bgfx.h:` | `—` | см. выше |
| `bgfx_d3d12_RendererContextD3D12_m_commandQueue` | NO_LAYOUT | `src-client/mc/external/bgfx/bgfx.h:` | `—` | bgfx — внешняя библиотека, layout не в хидерах БДС |
| `Actor_mSerializedSkin` | GONE | `SerializedSkin в 1.26 не найден:` | `—` | ищи PlayerSkinComponent / SerializedSkinComponent — реверс |
| `BedrockPlatformUWP_mcGame` | GONE | `BedrockPlatformUWP в 1.26 нет:` | `—` | кастомное имя — реверс |
| `BlockLegacy_getCollisionShape` | GONE | `класса BlockLegacy в 1.26 нет:` | `—` | см. BlockLegacy_mBlockId |
| `BlockLegacy_mBlockId` | GONE | `класса BlockLegacy в 1.26 нет:` | `—` | переименован/вынесен — реверс |
| `BlockLegacy_mayPlaceOn` | GONE | `класса BlockLegacy в 1.26 нет:` | `—` | см. BlockLegacy_mBlockId |
| `Bone_mPartModel` | GONE | `Bone — собственная структура проекта:` | `—` | твой реверс, как и раньше |
| `MainView_bedrockPlatform` | GONE | `MainView в 1.26 нет:` | `—` | кастомное имя — реверс |
| `MinecraftSim_mGameSession` | GONE | `MinecraftSim в 1.26 нет:` | `—` | см. выше |
| `MinecraftSim_mGameSim` | GONE | `MinecraftSim в 1.26 нет:` | `—` | кастомное имя из Flarial — реверс |
| `MinecraftSim_mRenderSim` | GONE | `MinecraftSim в 1.26 нет:` | `—` | см. выше |
| `UIProfanityContext_mEnabled` | GONE | `UIProfanityContext в 1.26 нет:` | `—` | кастомное имя — реверс |

## Что это значит

- Всё, что попало в `FOUND*` — можно перестать искать сигнатурой:
  адрес даст symdb, имя и типы даст хидер.
- `NOT_IN_CLASS` / `CLASS_MISSING` — это и есть объём ручного реверса.
- Отчёт сгенерирован `tools/symbol_audit.py`, перезапускайте после обновления хидеров.
