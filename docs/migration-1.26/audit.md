# Аудит миграции Solstice 1.21.44 → 1.26 (LeviLamina)

- Проект: `lmarena` (Solstice, гейм-версия 1.21.44)
- Заголовки LeviLamina: `/home/user/LeviLamina`
- Проиндексировано классов в хидерах: **20521**
- Проверено целей: **88** (хуков: 27, сигнатур/полей: 61)
- Найдено по имени в 1.26: **49** (56%) — из них точно в том же классе: **34** (транков `$`: 6), в другом классе (на проверку): **15**

Статусы:

- `FOUND` — метод/поле с таким именем есть в заголовке 1.26 (адрес резолвит symdb)
- `FOUND_THUNK` — найден `$`-транк: виртуальную функцию хукают через `&Class::$method`
- `FOUND_ELSEWHERE` — в «своём» классе имени нет, но оно найдено в другом классе (переехало)
- `EMPTY_CLASS` — класс/структура в хидерах пустая (`struct X {};`) — layout неизвестен, только реверс
- `NOT_IN_CLASS` — класс есть, но такого члена нет (переименовано/удалено/переехало)
- `CLASS_MISSING` — класса с таким именем в хидерах 1.26 нет вообще

## Хуки

| Цель в проекте (1.21.44) | Статус | LeviLamina 26.51 | Объявление | Примечание |
|---|---|---|---|---|
| `Actor::baseTick` | FOUND | `src/mc/world/actor/Actor.h:1254` | `MCAPI void $baseTick();` | готовый символ |
| `ActorAnimationControllerPlayer::applyToPose` | FOUND | `src/mc/world/actor/animation/ActorAnimationControllerPlayer.h:122` | `MCAPI void $applyToPose(` | символ есть, но сигнатура изменилась: (ApplyAnimationContext const&, RenderParams&, unordered_map<SkeletalHierarchyIndex, vector<BoneOrientation>>&, float) — тело хука переписывать |
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
| `bobHurt` | FOUND_ELSEWHERE | `src-client/mc/client/renderer/game/LevelRendererPlayer.h:365` | `MCAPI void bobHurt(::Matrix& mv, float a);` | найдено как LevelRendererPlayer::bobHurt(Matrix&, float) — класс сменился \| найдено в другом классе — проверь, тот ли это метод |
| `entityHealthChanged` | FOUND_ELSEWHERE | `src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:127` | `virtual void onActorHealthChanged(` | в 1.26: onActorHealthChanged (IScriptWorldAfterEvents.h:127) \| найдено в другом классе — проверь, тот ли это метод |
| `entityHurt` | FOUND_ELSEWHERE | `src/mc/world/actor/Actor.h:612` | `_hurt(::ActorDamageSource const& source, float damage, ::HurtParameters const& hurtPara…` | найдено в другом классе — проверь, тот ли это метод |
| `projectileHitBlock` | FOUND_ELSEWHERE | `src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:341` | `onProjectileHitBlock(::std::shared_ptr<::ScriptModuleMinecraft::ScriptProjectileHitInte…` | в 1.26 есть scripting-событие onProjectileHitBlock + ProjectileHitEvent \| найдено в другом классе — проверь, тот ли это метод |
| `projectileHitEntity` | FOUND_ELSEWHERE | `src/mc/scripting/modules/minecraft/events/IScriptWorldAfterEvents.h:344` | `onProjectileHitEntity(::std::shared_ptr<::ScriptModuleMinecraft::ScriptProjectileHitInt…` | в 1.26 есть scripting-событие onProjectileHitEntity + ProjectileHitEvent \| найдено в другом классе — проверь, тот ли это метод |
| `BlockSource::fireBlockChanged` | FOUND_ELSEWHERE | `src/mc/world/level/BlockSource.h:282` | `virtual void fireBlockChanged(` | найдено в другом классе — проверь, тот ли это метод |
| `Mob::getCurrentSwingDuration` | FOUND_ELSEWHERE | `src/mc/world/item/Item.h:231` | `virtual uint getSwingDuration() const;` | в 1.26: uint Item::getSwingDuration() (Item.h:231) \| найдено в другом классе — проверь, тот ли это метод |
| `mce::framebuilder::RenderItemInHandDescription::RenderItemInHandDescription` | EMPTY_CLASS | `src/mc/deps/minecraft_renderer/framebuilder/RenderItemInHandDescription.h:7` | `struct RenderItemInHandDescription {};` | структура в хидерах пустая (layout неизвестен) — только ручной реверс |
| `Keyboard::feed` | NOT_IN_CLASS | — | `—` | в 1.26 хук клавиатуры не нужен: ll::event::input::KeyInputEvent + ll::input::KeyRegistry::getOrCreateKey (src-client/ll/api/event/input) |
| `PacketHandlerDispatcherInstance<` | CLASS_MISSING | — | `—` | класс есть: src/mc/network/PacketHandlerDispatcherInstance.h (шаблон, имя в Detour обрезано) |
| `CameraDirectLookSystemUtil::_handleLookInput` | CLASS_MISSING | — | `—` | класс переехал: теперь CameraDirectLookComponent / CameraDirectLookDefinition |
| `Unknown::renderNametag` | CLASS_MISSING | — | `—` | в 1.26 неймтеги — это NameTagRenderObject / NameTagRenderer (объекты рендера), не отдельная функция |

## Сигнатуры и поля (OffsetProvider.hpp)

| Цель в проекте (1.21.44) | Статус | LeviLamina 26.51 | Объявление | Примечание |
|---|---|---|---|---|
| `Actor_baseTick` | FOUND | `src/mc/world/actor/Actor.h:1254` | `MCAPI void $baseTick();` |  |
| `Actor_mLevel` | FOUND | `src/mc/world/actor/Actor.h:176` | `[[nodiscard]] Level& getLevel() const { return *reinterpret_cast<Level*>(mLevel); }` |  |
| `Actor_swing` | FOUND | `src/mc/world/actor/Actor.h:1426` | `MCFOLD bool $swing(::ActorSwingSource swingSource, ::HandSlot handSlot);` |  |
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
| `BlockLegacy_getCollisionShape` | FOUND_ELSEWHERE | `src/mc/world/level/block/AirBlock.h:54` | `virtual ::AABB getCollisionShape(` | найдено в другом классе — проверь, тот ли это метод |
| `BlockLegacy_mayPlaceOn` | FOUND_ELSEWHERE | `src/mc/world/level/block/AzaleaBlock.h:47` | `virtual bool mayPlaceOn(::BlockSource& region, ::BlockPos const& pos) const /*override*/;` | найдено в другом классе — проверь, тот ли это метод |
| `BlockSource_clip` | FOUND_ELSEWHERE | `src/mc/util/ClipUtils.h:16` | `MCNAPI ::HitResult clip(::BlockPos const& pos, ::Vec3 const& A, ::Vec3 const& B, ::AABB…` | найдено в другом классе — проверь, тот ли это метод |
| `BlockSource_getChunk` | FOUND_ELSEWHERE | `src/mc/world/level/BlockSource.h:156` | `virtual ::LevelChunk* getChunk(int x, int z) const /*override*/;` | найдено в другом классе — проверь, тот ли это метод |
| `BlockSource_setBlock` | FOUND_ELSEWHERE | `src/mc/editor/EditorStructureBlockSource.h:63` | `virtual bool setBlock(` | найдено в другом классе — проверь, тот ли это метод |
| `ClientInstance_getBlockSource` | FOUND_ELSEWHERE | `src/mc/gametest/MinecraftGameTestHelper.h:264` | `virtual ::std::optional<::gametest::GameTestError> getBlockSource(::BlockSource*& block…` | найдено в другом классе — проверь, тот ли это метод |
| `ContainerManagerModel_getSlot` | FOUND_ELSEWHERE | `src/mc/scripting/modules/minecraft/ScriptContainer.h:74` | `getSlot(` | найдено в другом классе — проверь, тот ли это метод |
| `PlayerInventory_mContainer` | FOUND_ELSEWHERE | `src/mc/world/ContainerIterator.h:16` | `[[nodiscard]] constexpr ContainerIterator(T* container, int position) : mContainer(cont…` | найдено в другом классе — проверь, тот ли это метод |
| `Actor_mContainerManagerModel` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mDestroying` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mGameMode` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mHurtTimeComponent` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mSerializedSkin` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mSupplies` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `Actor_mSwinging` | NOT_IN_CLASS | `src/mc/world/actor/Actor.h:` | `—` |  |
| `BlockSource_mBuildHeight` | NOT_IN_CLASS | `src-client/mc/client/renderer/block/tessellation_pipeline/world_view/BlockSource.h:` | `—` |  |
| `ClientInputMappingFactory_mKeyboardMouseSettings` | NOT_IN_CLASS | `src-client/mc/client/input/ClientInputMappingFactory.h:` | `—` |  |
| `ClientInstance_getInputHandler` | NOT_IN_CLASS | `src-client/mc/client/game/ClientInstance.h:` | `—` |  |
| `ClientInstance_mGuiData` | NOT_IN_CLASS | `src-client/mc/client/game/ClientInstance.h:` | `—` | закомментировано в проекте |
| `ClientInstance_mLevelRenderer` | NOT_IN_CLASS | `src-client/mc/client/game/ClientInstance.h:` | `—` |  |
| `ClientInstance_mMinecraftSim` | NOT_IN_CLASS | `src-client/mc/client/game/ClientInstance.h:` | `—` |  |
| `ClientInstance_mPacketSender` | NOT_IN_CLASS | `src-client/mc/client/game/ClientInstance.h:` | `—` |  |
| `GameSession_mEventCallback` | NOT_IN_CLASS | `src/mc/world/GameSession.h:` | `—` |  |
| `LevelData_mTick` | NOT_IN_CLASS | `src/mc/world/level/storage/LevelData.h:` | `—` |  |
| `LevelRenderer_mRendererPlayer` | NOT_IN_CLASS | `src-client/mc/client/renderer/game/LevelRenderer.h:` | `—` |  |
| `LevelRendererPlayer_mCameraPos` | NOT_IN_CLASS | `src-client/mc/client/renderer/game/LevelRendererPlayer.h:` | `—` |  |
| `LevelRendererPlayer_mFovX` | NOT_IN_CLASS | `src-client/mc/client/renderer/game/LevelRendererPlayer.h:` | `—` |  |
| `LevelRendererPlayer_mFovY` | NOT_IN_CLASS | `src-client/mc/client/renderer/game/LevelRendererPlayer.h:` | `—` |  |
| `MinecraftGame_mClientInstances` | NOT_IN_CLASS | `src-client/mc/client/game/MinecraftGame.h:` | `—` |  |
| `MinecraftGame_mMouseGrabbed` | NOT_IN_CLASS | `src-client/mc/client/game/MinecraftGame.h:` | `—` |  |
| `MinecraftGame_mProfanityContext` | NOT_IN_CLASS | `src-client/mc/client/game/MinecraftGame.h:` | `—` |  |
| `MinecraftGame_playUi` | NOT_IN_CLASS | `src-client/mc/client/game/MinecraftGame.h:` | `—` |  |
| `bgfx_context_m_renderCtx` | NOT_IN_CLASS | `src-client/mc/external/bgfx/bgfx.h:` | `—` |  |
| `bgfx_d3d12_RendererContextD3D12_m_commandQueue` | NOT_IN_CLASS | `src-client/mc/external/bgfx/bgfx.h:` | `—` |  |
| `BedrockPlatformUWP_mcGame` | CLASS_MISSING | — | `—` |  |
| `BlockLegacy_mBlockId` | CLASS_MISSING | — | `—` |  |
| `Bone_mPartModel` | CLASS_MISSING | — | `—` |  |
| `MainView_bedrockPlatform` | CLASS_MISSING | — | `—` |  |
| `MinecraftSim_mGameSession` | CLASS_MISSING | — | `—` |  |
| `MinecraftSim_mGameSim` | CLASS_MISSING | — | `—` |  |
| `MinecraftSim_mRenderSim` | CLASS_MISSING | — | `—` |  |
| `UIProfanityContext_mEnabled` | CLASS_MISSING | — | `—` |  |

## Что это значит

- Всё, что попало в `FOUND*` — можно перестать искать сигнатурой:
  адрес даст symdb, имя и типы даст хидер.
- `NOT_IN_CLASS` / `CLASS_MISSING` — это и есть объём ручного реверса.
- Отчёт сгенерирован `tools/symbol_audit.py`, перезапускайте после обновления хидеров.
