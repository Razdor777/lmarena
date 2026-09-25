#pragma once
#include <Utils/SysUtils/xorstr.hpp>
//
// Created by vastrakai on 6/25/2024.
//

enum class OffsetType {
    Index,
    FieldOffset,
};

// kinda aids ik stfu
#define DEFINE_INDEX_FIELD_TYPED(type, name, str, index_offset, offset_type) \
public: \
static inline type (name) = 0; \
private: \
static void name##_initializer() { \
    static bool initialized = false; \
    if (initialized) return; \
    initialized = true; \
    auto result = scanSig(hat::compile_signature<str>(), xorstr_(#name), index_offset); \
    if (!result.has_result()) return; \
    if ((offset_type) == OffsetType::Index) name = *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(result.get()) + index_offset) / 8; \
    else (name) = *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(result.get()) + index_offset); \
} \
private: \
static inline std::function<void()> name##_function = (mSigInitializers.emplace_back(name##_initializer), std::function<void()>()); \
public:


#define DEFINE_INDEX_FIELD(name, str, index_offset, offset_type) \
public: \
static inline int (name) = 0; \
private: \
static void name##_initializer() { \
    static bool initialized = false; \
    if (initialized) return; \
    initialized = true; \
    auto result = scanSig(hat::compile_signature<str>(), xorstr_(#name), index_offset); \
    if (!result.has_result()) return; \
    if ((offset_type) == OffsetType::Index) (name) = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(result.get()) + (index_offset)) / 8; \
    else (name) = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(result.get()) + (index_offset)); \
} \
private: \
static inline std::function<void()> name##_function = (mSigInitializers.emplace_back(name##_initializer), std::function<void()>()); \
public:

// Defines a direct offset for a field (for offsets/indexes that can't be sigged)
#define DEFINE_FIELD(name, offset) \
public: \
static constexpr inline int (name) = offset;

#include <future>
#include <libhat/Scanner.hpp>

class OffsetProvider {
    static hat::scan_result scanSig(hat::signature_view sig, const std::string& name, int offset = 0);

    static inline std::vector<std::function<void()>> mSigInitializers;
    static inline int mSigScanCount;
public:
    static inline bool mIsInitialized = false;
    static inline std::unordered_map<std::string, uintptr_t> mSigs;

// [1.26] ОБНОВЛЕНО: ClientInstance_getLocalPlayer -> src-client/mc/client/game/ClientInstance.h:1472
    DEFINE_INDEX_FIELD(ClientInstance_getLocalPlayer, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 8B 88 ? ? ? ? C1 E1", 3, OffsetType::Index);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: ClientInstance_getBlockSource -> ClientInstance::getRegion()  (src-client/mc/client/game/ClientInstance.h:475, транк $getRegion:1470) — getBlockSource переименован в getRegion
    DEFINE_INDEX_FIELD(ClientInstance_getBlockSource, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 85 ? 75 ? 48 83 C4 ? 5B C3 48 8B", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: ClientInstance_getOptions -> src-client/mc/client/game/ClientInstance.h:1761
    DEFINE_INDEX_FIELD(ClientInstance_getOptions, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? 48 8B ? FF 15 ? ? ? ? 3C ? 75 ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 3C", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: ClientInstance_getScreenName -> src-client/mc/client/game/ClientInstance.h:1909 — закомментировано в проекте
    //DEFINE_INDEX_FIELD(ClientInstance_getScreenName, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 90 48 85 ? 74 ? 48 8B ? E8 ? ? ? ? 90 48 89 ? ? ? 48 8B ? ? ? 48 89 ? ? ? 48 85 ? 74 ? E8 ? ? ? ? 48 8B ? ? ? 48 85 ? 74 ? 48 8B ? E8 ? ? ? ? 48 8B ? 48 8B ? ? ? 48 8B ? ? ? 48 83 C4 ? 5F C3 E8 ? ? ? ? 90 CC CC CC CC CC 48 89 ? ? ? 48 89 ? ? ? 57 48 83 EC ? 48 8B ? ? 48 83 C1", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: ClientInstance_getMouseGrabbed -> src-client/mc/client/game/ClientInstance.h:1707
    DEFINE_INDEX_FIELD(ClientInstance_getMouseGrabbed, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 84 C0 75 ? 48 8B ? ? ? ? ? 48 85 ? 0F 84 ? ? ? ? 80 38 ? 0F 84 ? ? ? ? 48 8B ? ? ? ? ? 48 85 ? 74 ? F0 FF ? ? 0F 10 ? ? ? ? ? 0F 11 ? ? ? ? ? F2 0F", 3, OffsetType::Index); // PLEASE CHECK AFTER IDK IF THIS IS CORRECT!!!
// [1.26] ОБНОВЛЕНО: ClientInstance_setDisableInput -> src-client/mc/client/game/ClientInstance.h:2041
    DEFINE_INDEX_FIELD(ClientInstance_setDisableInput, "48 8B ? ? ? ? ? 48 8B ? FF 15 ? ? ? ? 48 85 ? 74 ? 48 8B ? 48 8B ? 48 8B", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: ClientInstance_grabMouse -> src-client/mc/client/game/ClientInstance.h:2043
    DEFINE_INDEX_FIELD(ClientInstance_grabMouse, "48 8B ? ? ? ? ? 48 8B ? FF 15 ? ? ? ? 48 85 ? 74 ? 48 8B ? 48 8B ? 48 8B", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: ClientInstance_releaseMouse -> src-client/mc/client/game/ClientInstance.h:2045
    DEFINE_INDEX_FIELD(ClientInstance_releaseMouse, "48 8B ? ? ? ? ? 48 8B ? FF 15 ? ? ? ? 48 85 ? 74 ? 48 8B ? 48 8B ? 48 8B", 3, OffsetType::Index);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): ClientInstance_mMinecraftSim -> src-client/mc/client/game/ClientInstance.h — в хидере описаны только mUITexture/mUICursorTexture — оффсет только из IDA
    DEFINE_INDEX_FIELD(ClientInstance_mMinecraftSim, "48 8B ? ? ? ? ? E8 ? ? ? ? 88 87 ? ? ? ? 48 8B ? ? ? ? ? 48 8B", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: ClientInstance_mLevelRenderer -> ClientInstance::getLevelRenderer()  (ClientInstance.h:796, $getLevelRenderer:1787) — поля класса не описаны (в 1.26 у ClientInstance всего 2 описанных поля) — бери через геттер, а не по оффсету
    DEFINE_INDEX_FIELD(ClientInstance_mLevelRenderer, "48 8B ? ? ? ? ? 48 85 ? 74 ? 48 8B ? ? ? ? ? 48 05 ? ? ? ? C3", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: ClientInstance_mPacketSender -> ClientInstance::getPacketSender()  (ClientInstance.h:994) — см. mLevelRenderer
    DEFINE_INDEX_FIELD(ClientInstance_mPacketSender, "48 8B ? ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B ? ? ? ? ? 48 8B ? 48 8B", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: ClientInstance_mGuiData -> ClientInstance::getGuiData()  (ClientInstance.h:857/859) — см. mLevelRenderer
    //DEFINE_INDEX_FIELD(ClientInstance_mGuiData, "48 8B ? ? ? ? ? 48 8B ? 48 85 ? 74 ? 33 C9 48 89 ? 48 89 ? ? 48 8B ? ? 48 85 ? 74 ? F0 FF ? ? 48 8B ? ? 48 8B ? ? 48 8B ? ? 48 89 ? 48 89 ? ? 48 85 ? 74 ? E8 ? ? ? ? 48 8B ? 48 85 ? 74 ? 48 83 38 ? 74 ? 48 8B ? 48 83 C4 ? 5B C3 E8 ? ? ? ? CC CC CC CC CC CC CC CC CC CC CC CC 48 8B ? ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 ? ? ? 57", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): MinecraftGame_playUi -> src-client/mc/client/game/MinecraftGame.h — у MinecraftGame в 1.26 не описано НИ ОДНОГО поля, метода playUi нет — реверс
    DEFINE_INDEX_FIELD(MinecraftGame_playUi, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 83 C4 ? C3 48 89 ? ? ? 48 89 ? ? ? 57 48 83 EC ? 48 8B", 3, OffsetType::Index);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): MinecraftGame_mClientInstances -> src-client/mc/client/game/MinecraftGame.h — полей не описано; геттеры primaryClientInstance ищи в MinecraftGame.h — реверс
    DEFINE_INDEX_FIELD(MinecraftGame_mClientInstances, "48 8B ? ? ? ? ? 48 8B ? 48 8B ? ? 80 78 19 ? 75 ? 48 8B ? 48 8B ? 80 78 19 ? 74 ? 80 79 19 ? 75 ? 80 79 20 ? 76 ? 48 8B ? 48 3B", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): MinecraftGame_mProfanityContext -> src-client/mc/client/game/MinecraftGame.h — полей не описано — реверс
    DEFINE_INDEX_FIELD(MinecraftGame_mProfanityContext, "49 8B ? ? ? ? ? ? 49 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 8D ? ? E8", 4, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): MinecraftGame_mMouseGrabbed -> src-client/mc/client/game/MinecraftGame.h — полей не описано; мышь — ClientInstance::grabMouse()/isMouseGrabbed()
    DEFINE_INDEX_FIELD(MinecraftGame_mMouseGrabbed, "80 B9 ? ? ? ? ? 0F 84 ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? FF 15 ? ? ? ? 48 85 C0", 2, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): MainView_bedrockPlatform -> MainView в 1.26 нет — кастомное имя — реверс
    DEFINE_INDEX_FIELD(MainView_bedrockPlatform, "48 8B ? ? ? ? ? 48 8B ? B2 ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 84 C0", 3, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО: Actor_baseTick -> src/mc/world/actor/Actor.h:1254
    DEFINE_INDEX_FIELD(Actor_baseTick, "48 8B 80 ? ? ? ? FF 15 ? ? ? ? 48 8B 8B ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? FF 15 ? ? ? ? 45 84 ED", 3, OffsetType::Index);
// [1.26] РЕВЕРС (в 1.26 нет): MinecraftSim_mGameSim -> MinecraftSim в 1.26 нет — кастомное имя из Flarial — реверс
    DEFINE_INDEX_FIELD(MinecraftSim_mGameSim, "49 8B 85 ? ? ? ? 8B 40", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): MinecraftSim_mRenderSim -> MinecraftSim в 1.26 нет — см. выше
    DEFINE_INDEX_FIELD(MinecraftSim_mRenderSim, "48 8B ? ? ? ? ? F3 0F ? ? ? F3 41 ? ? ? ? 48 8D", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): MinecraftSim_mGameSession -> MinecraftSim в 1.26 нет — см. выше
    DEFINE_INDEX_FIELD(MinecraftSim_mGameSession, "48 8B ? ? ? ? ? 48 85 ? 0F 84 ? ? ? ? 44 38 ? ? 0F 84 ? ? ? ? 48 8B", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: GameSession_mEventCallback -> GameSession::getNetEventCallback() / mLegacyClientNetworkHandler  (src/mc/world/GameSession.h:30, 60) — mEventCallback -> getNetEventCallback()
    DEFINE_INDEX_FIELD_TYPED(uint8_t, GameSession_mEventCallback, "48 8B ? ? 48 85 ? 74 ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 48 8B ? ? 48 85 ? 74 ? 48 8B ? 48 8B ? ? 48 83 C4 ? 5B 48 FF ? ? ? ? ? 48 83 C4 ? 5B C3 CC CC CC CC CC CC CC CC 48 89", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): BedrockPlatformUWP_mcGame -> BedrockPlatformUWP в 1.26 нет — кастомное имя — реверс
    DEFINE_INDEX_FIELD_TYPED(uint8_t, BedrockPlatformUWP_mcGame, "48 8B ? ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 84 C0 74 ? 48 8B ? ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 84 C0", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): bgfx_d3d12_RendererContextD3D12_m_commandQueue -> src-client/mc/external/bgfx/bgfx.h — bgfx — внешняя библиотека, layout не в хидерах БДС
    DEFINE_INDEX_FIELD(bgfx_d3d12_RendererContextD3D12_m_commandQueue, "48 8B ? ? ? ? ? 4C 8B ? ? 48 8B ? 48 8B ? ? 48 8B ? ? FF 15 ? ? ? ? 48 FF ? ? 48 8B ? ? 48 8B ? 48 3B ? 74 ? 48 8B", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): bgfx_context_m_renderCtx -> src-client/mc/external/bgfx/bgfx.h — см. выше
    DEFINE_INDEX_FIELD(bgfx_context_m_renderCtx, "48 8B ? ? ? ? ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 48 89 ? ? ? ? ? 48 63", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): BlockLegacy_mBlockId -> класса BlockLegacy в 1.26 нет — переименован/вынесен — реверс
    DEFINE_INDEX_FIELD(BlockLegacy_mBlockId, "44 0F ? ? ? ? ? ? B8 ? ? ? ? 48 8B ? 48 8B", 4, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): BlockLegacy_mayPlaceOn -> класса BlockLegacy в 1.26 нет — см. BlockLegacy_mBlockId
    DEFINE_INDEX_FIELD(BlockLegacy_mayPlaceOn, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 49 8B ? 0F B6 ? E8 ? ? ? ? 48 85 ? 74 ? 45 0F", 3, OffsetType::Index);
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mContainerManagerModel -> PlayerInventory::mHudContainerManager  (weak_ptr<HudContainerManagerModel>, src/mc/world/actor/player/PlayerInventory.h:26) — контейнер-менеджер живёт в PlayerInventory
    DEFINE_INDEX_FIELD(Actor_mContainerManagerModel, "49 8B ? ? ? ? ? 48 89 ? ? ? 48 89 ? ? ? F0 FF ? ? 66 0F ? ? ? ? EB ? 0F 57 ? 66 0F 73 D8 ? 66 48 ? ? ? 0F 57 ? 66 0F ? ? ? ? ? ? 48 85", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mGameMode -> ECS ActorGameTypeComponent  (src/mc/entity/components/ActorGameTypeComponent.h) — gamemode ушёл в ECS-компонент, бери через getEntityContext()
    DEFINE_INDEX_FIELD(Actor_mGameMode, "49 8B 8E ? ? ? ? 45 84 FF 74 ? 84 DB", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mSupplies -> ECS ActorEquipmentComponent  (src/mc/entity/components/ActorEquipmentComponent.h) — mHand/mArmor — unique_ptr<SimpleContainer>
    DEFINE_INDEX_FIELD(Actor_mSupplies, "48 8B ? ? ? ? ? 80 BA B0 00 00 00 ? 75 ? 48 8B ? ? ? ? ? 8B 52 ? 48 8B ? 48 8B ? ? 48 FF ? ? ? ? ? 48 8D ? ? ? ? ? C3 48 89", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: PlayerInventory_mContainer -> PlayerInventory::mInventory  (unique_ptr<Inventory>, src/mc/world/actor/player/PlayerInventory.h:24) — mContainer -> mInventory
    DEFINE_INDEX_FIELD(PlayerInventory_mContainer, "48 8B ? ? ? ? ? 8B 52 ? 48 8B ? 48 8B ? ? 48 FF ? ? ? ? ? 48 8D", 3, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО ($-транк): Container_getItem -> src/mc/world/Container.h:62
    DEFINE_INDEX_FIELD_TYPED(uint8_t, Container_getItem, "48 8B ? ? FF 15 ? ? ? ? 4C 8B ? 41 80 7E 23", 3, OffsetType::Index);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: LevelRenderer_mRendererPlayer -> LevelRenderer::mLevelRendererPlayer  (shared_ptr<LevelRendererPlayer>, src-client/mc/client/renderer/game/LevelRenderer.h:125) — mRendererPlayer -> mLevelRendererPlayer, тип shared_ptr
    DEFINE_INDEX_FIELD(LevelRenderer_mRendererPlayer, "48 8B ? ? ? ? ? F3 0F ? ? ? ? ? ? 0F AF", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: LevelRendererPlayer_mCameraPos -> LevelRendererCamera::mCameraPos  (Vec3, src-client/mc/client/renderer/game/LevelRendererCamera.h:263) — позиция камеры переехала в LevelRendererCamera
    DEFINE_INDEX_FIELD(LevelRendererPlayer_mCameraPos, "F3 0F ? ? ? ? ? ? F3 0F ? ? F3 0F ? ? F3 0F ? ? F3 0F ? ? F3 0F ? ? F3 0F ? ? 0F 2F ? ? ? ? ? 0F 83 ? ? ? ? 80 BB 3A 0A 00 00", 4, OffsetType::FieldOffset);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: LevelRendererPlayer_mFovX -> LevelRendererPlayer::mFov  (float, src-client/mc/client/renderer/game/LevelRendererPlayer.h:131) — в 1.26 один float mFov (есть ещё mOFov — предыдущее значение); вертикальный FOV считается из aspect ratio
    DEFINE_INDEX_FIELD(LevelRendererPlayer_mFovX, "0F 10 ? ? ? ? ? 0F 11 ? ? 0F 10 ? ? ? ? ? 0F 11 ? ? 0F 10 ? ? ? ? ? 0F 11 ? ? 0F 10 ? ? ? ? ? 0F 11 ? ? 80 B9 58 0F 00 00", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: LevelRendererPlayer_mFovY -> LevelRendererPlayer::mOFov  (float, LevelRendererPlayer.h:132) — в 1.26 отдельного «FovY» нет: mFov + mOFov (предыдущее)
    DEFINE_INDEX_FIELD(LevelRendererPlayer_mFovY, "89 87 ? ? ? ? 41 8B ? ? ? 89 87 ? ? ? ? 41 8B ? ? ? 89 87 ? ? ? ? 41 8B ? ? ? 89 87 ? ? ? ? 41 8B ? ? ? 89 87 ? ? ? ? 41 8B ? ? ? 89 87 ? ? ? ? 41 8B ? ? ? F3 0F", 2, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mSwinging -> actor data flags (ActorDataFlagComponent / getStatusFlag(ActorFlags::Swinging)) — флаги актора в 1.26 — биты в ECS-компоненте, не поле Actor
    DEFINE_INDEX_FIELD(Actor_mSwinging, "88 ? ? ? ? ? EB ? 33 ? 89", 2, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО: Actor_swing -> src/mc/world/actor/Actor.h:1426
    DEFINE_INDEX_FIELD(Actor_swing, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 8B ? ? ? ? ? 48 8B ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 48 8B", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: Actor_mLevel -> src/mc/world/actor/Actor.h:176
    DEFINE_INDEX_FIELD(Actor_mLevel, "48 8B ? ? ? ? ? 48 89 ? ? ? F3 0F ? ? ? 4C 8B", 3, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО: Level_getHitResult -> src/mc/world/level/Level.h:2443
    DEFINE_INDEX_FIELD(Level_getHitResult, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 8B ? 49 8B ? ? 48 8B ? ? ? ? ? 49 8B ? FF 15 ? ? ? ? F3 0F", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: Level_getPlayerList -> src/mc/world/level/Level.h:2411
    DEFINE_INDEX_FIELD(Level_getPlayerList, "48 8B 80 ? ? ? ? FF 15 ? ? ? ? 8B 48 ? 39 8E ? ? ? ? 0F 84", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: Level_getPlayerMovementSettings -> src/mc/world/level/Level.h:2340
    DEFINE_INDEX_FIELD(Level_getPlayerMovementSettings, "48 8B 80 ? ? ? ? FF 15 ? ? ? ? 49 8B 7E ? 80 78", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: Level_getLevelData -> src/mc/world/level/Level.h:1941
    DEFINE_INDEX_FIELD(Level_getLevelData, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 0F 10 ? ? ? ? ? 0F 11 ? ? 8B 88", 3, OffsetType::Index); // Also works on .40
// [1.26] ОБНОВЛЕНО: Level_getBlockPalette -> src/mc/world/level/Level.h:1766
    DEFINE_INDEX_FIELD(Level_getBlockPalette, "48 8B ? ? ? ? ? 48 8B ? FF 15 ? ? ? ? 4C 8B ? 48 8B ? 48 8B ? ? 48 8D ? ? 49 8B ? FF 15 ? ? ? ? 48 8B ? 44 0F", 3, OffsetType::Index);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: LevelData_mTick -> LevelData::mCurrentTick  (Tick, src/mc/world/level/storage/LevelData.h:77) — mTick -> mCurrentTick (тип Tick, 8 байт)
    DEFINE_INDEX_FIELD(LevelData_mTick, "48 8B ? ? ? ? ? 4C 8B ? ? ? ? ? E8 ? ? ? ? 44 8B", 3, OffsetType::FieldOffset);
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mDestroying -> actor data flags (ActorDataFlagComponent) — см. Actor_mSwinging
    DEFINE_INDEX_FIELD(Actor_mDestroying, "44 38 ? ? ? ? ? 74 ? 48 8B ? ? ? ? ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 44 39", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): UIProfanityContext_mEnabled -> UIProfanityContext в 1.26 нет — кастомное имя — реверс
    DEFINE_INDEX_FIELD_TYPED(uint8_t, UIProfanityContext_mEnabled, "80 79 ? ? 74 ? 80 79 ? ? 74 ? 45 ? ? 75 ? 4C 8D", 2, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): Bone_mPartModel -> Bone — собственная структура проекта — твой реверс, как и раньше
    DEFINE_INDEX_FIELD(Bone_mPartModel, "8B 81 ? ? ? ? 89 82 ? ? ? ? F3 0F 11 8A", 2, OffsetType::FieldOffset); // this isn't even correct lmao
// [1.26] ПЕРЕЕХАЛО В 1.26: Actor_mHurtTimeComponent -> ECS MobHurtTimeComponent : IntComponent  (src/mc/entity/components/MobHurtTimeComponent.h) — hurt time ушёл в ECS-компонент (mValue)
    DEFINE_INDEX_FIELD(Actor_mHurtTimeComponent, "48 8B ? ? ? ? ? 48 85 ? 74 ? 66 0F ? ? ? ? ? ? 0F 5B ? 0F 2F", 3, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО: BlockSource_getChunk -> src/mc/world/level/BlockSource.h:581
    DEFINE_INDEX_FIELD(BlockSource_getChunk, "48 8B ? ? ? ? ? 44 89 ? ? ? FF 15 ? ? ? ? 48 83 C4", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: BlockSource_setBlock -> src/mc/world/level/BlockSource.h:730
    DEFINE_INDEX_FIELD(BlockSource_setBlock, "48 8B ? ? ? ? ? FF 15 ? ? ? ? EB ? 41 83 FF", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: LevelChunk_mSubChunks -> src/mc/world/level/chunk/LevelChunk.h:188
    DEFINE_INDEX_FIELD(LevelChunk_mSubChunks, "48 2B ? ? ? ? ? 48 8B ? 48 C1 FF ? 48 0F", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): BlockSource_mBuildHeight -> src/mc/world/level/BlockSource.h — поля нет; есть getHeight()/getHeightmapPos(); высота мира — DimensionHeightRange.h
    DEFINE_INDEX_FIELD_TYPED(uint8_t, BlockSource_mBuildHeight, "0F BF ? ? 3B C8 0F 8D ? ? ? ? 8B 0F", 3, OffsetType::FieldOffset);
// [1.26] ОБНОВЛЕНО: ContainerManagerModel_getContainerType -> src/mc/world/containers/managers/models/ContainerManagerModel.h:161
    DEFINE_INDEX_FIELD_TYPED(uint8_t, ContainerManagerModel_getContainerType, "48 8B ? ? FF 15 ? ? ? ? 84 C0 75 ? 48 8B ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 84 C0 74 ? B0", 3, OffsetType::Index);
// [1.26] ПЕРЕИМЕНОВАНО В 1.26: ContainerManagerModel_getSlot -> ContainerManagerModel::getFullContainerSlot(int, FullContainerName const&)  (src/mc/world/containers/managers/models/ContainerManagerModel.h:107, $getFullContainerSlot:182) — getSlot -> getFullContainerSlot, добавился аргумент FullContainerName
    DEFINE_INDEX_FIELD_TYPED(uint8_t, ContainerManagerModel_getSlot, "48 8B ? ? FF 15 ? ? ? ? 0F B6 ? ? 44 89", 3, OffsetType::Index); // what the actual fcuk is wrong with me.
// [1.26] РЕВЕРС (в 1.26 нет): Actor_mSerializedSkin -> SerializedSkin в 1.26 не найден — ищи PlayerSkinComponent / SerializedSkinComponent — реверс
    DEFINE_INDEX_FIELD(Actor_mSerializedSkin, "4C 8B ? ? ? ? ? 48 8B ? 48 8B ? ? 89 44", 3, OffsetType::FieldOffset);
// [1.26] РЕВЕРС (в 1.26 нет): BlockLegacy_getCollisionShape -> класса BlockLegacy в 1.26 нет — см. BlockLegacy_mBlockId
    DEFINE_INDEX_FIELD_TYPED(uint8_t, BlockLegacy_getCollisionShape, "48 8B ? ? FF 15 ? ? ? ? 0F 10 ? 0F 11 ? ? ? ? ? ? F2 0F ? ? ? F2 0F", 3, OffsetType::Index);
// [1.26] ОБНОВЛЕНО: BlockSource_clip -> src/mc/world/level/BlockSource.h:686
    DEFINE_INDEX_FIELD(BlockSource_clip, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 90 48 8D ? ? ? ? ? 48 8D ? ? ? ? ? E8 ? ? ? ? 48 8B", 3, OffsetType::Index);

// [1.26] ПЕРЕИМЕНОВАНО В 1.26: ClientInstance_getInputHandler -> ClientInstance::getInput() -> ClientInputHandler*  (ClientInstance.h:1040); getMinecraftInput() (968) — getInputHandler -> getInput
    DEFINE_INDEX_FIELD(ClientInstance_getInputHandler, "48 8B ? ? ? ? ? FF 15 ? ? ? ? 8D 7E", 3, OffsetType::Index);
// [1.26] РЕВЕРС (поля класса не описаны в 1.26): ClientInputMappingFactory_mKeyboardMouseSettings -> src-client/mc/client/input/ClientInputMappingFactory.h — полей не описано — реверс
    DEFINE_INDEX_FIELD(ClientInputMappingFactory_mKeyboardMouseSettings, "49 8B 9D ? ? ? ? 48 89 5D ? 4D 8B AD ? ? ? ? 4C 89 6D ? E9", 3, OffsetType::FieldOffset); // TODO: replace with betta siggy

// [1.26] ОБНОВЛЕНО: ClientInputHandler_mMappingFactory -> src-client/mc/client/input/ClientInputHandler.h:39 — закомментировано в проекте
    //DEFINE_INDEX_FIELD_TYPED(uint8_t, ClientInputHandler_mMappingFactory, "48 8B ? ? EB ? 48 8D ? ? 48 85 ? 74 ? 48 8B ? E8 ? ? ? ? 48 8B ? 48 8B ? ? ? 48 33 ? E8 ? ? ? ? 48 8B ? ? ? 48 8B ? ? ? 48 83 C4 ? 5F C3 E8 ? ? ? ? CC CC CC CC CC CC 48 89", 3, OffsetType::FieldOffset);
    DEFINE_FIELD(ClientInputHandler_mMappingFactory, 0x28);

    // TODO: Move all CLASS_FIELD declarations and hat::member_at offsets to here
    DEFINE_FIELD(ContainerScreenController_tryExit, 12);
    DEFINE_FIELD(ItemStack_reInit, 3);
    DEFINE_FIELD(MinecraftUIRenderContext_drawImage, 7);
    DEFINE_FIELD(Actor_mEntityIdentifier, 0x240);
    DEFINE_FIELD(Actor_mLocalName, 0xCB0);
    DEFINE_FIELD(ClientInstance_mViewMatrix, 0x368);
    DEFINE_FIELD(ClientInstance_mFovX, 0x728);
    DEFINE_FIELD(ClientInstance_mFovY, 0x73C);
    DEFINE_FIELD(Block_mRuntimeId, 0xC0);
    DEFINE_FIELD(Block_mLegacy, 0x30);
    DEFINE_FIELD(ClientInstance_getScreenName, 266); // i will make sig for it later (maybe)
    DEFINE_FIELD(ClientInstance_mGuiData, 0x590); // i will make sig for it later (maybe)
    DEFINE_FIELD(BlockPalette_mLevel, 0x80);

    // ═══════════════════════════════════════════════════════════════
    // All offsets below imported from Flarial OffsetInit::init2140()
    // Version: 1.21.4X (confirmed compatible with 1.21.44)
    // ═══════════════════════════════════════════════════════════════

    // --- Level ---
    DEFINE_FIELD(Flarial_Level_hitResult, 0x248);
    DEFINE_FIELD(Flarial_Level_worldFolderName, 0x2B8);
    DEFINE_FIELD(Flarial_Level_getPlayerMap, 0xBF0);
    DEFINE_FIELD(Flarial_Level_LevelData, 0x110);

    // --- LevelData ---
    DEFINE_FIELD(Flarial_LevelData_worldName, 0x390);

    // --- Player ---
    DEFINE_FIELD(Flarial_Player_gamemode, 0xB28);
    DEFINE_FIELD(Flarial_Player_playerName, 0xCB0);

    // --- ClientInstance ---
    DEFINE_FIELD(Flarial_ClientInstance_getBlockSource, 29); // vtable index
    DEFINE_FIELD(Flarial_ClientInstance_minecraftGame, 0xD0);
    DEFINE_FIELD(Flarial_ClientInstance_levelRenderer, 0xE8);
    DEFINE_FIELD(Flarial_ClientInstance_camera, 0x2A8);
    DEFINE_FIELD(Flarial_ClientInstance_viewMatrix, 0x368);
    DEFINE_FIELD(Flarial_ClientInstance_guiData, 0x590);
    DEFINE_FIELD(Flarial_ClientInstance_getFovX, 0x728);
    DEFINE_FIELD(Flarial_ClientInstance_getFovY, 0x73C);
    DEFINE_FIELD(Flarial_ClientInstance_getPacketSender, 0xF8);

    // --- MinecraftGame ---
    DEFINE_FIELD(Flarial_MinecraftGame_mouseGrabbed, 0x1A0);
    DEFINE_FIELD(Flarial_MinecraftGame_textureGroup, 0x650);

    // --- RaknetConnector ---
    DEFINE_FIELD(Flarial_RaknetConnector_getPeer, 0x2A0);
    DEFINE_FIELD(Flarial_RaknetConnector_JoinedIp, 0x398);
    DEFINE_FIELD(Flarial_RaknetConnector_port, 0x3B8);
    DEFINE_FIELD(Flarial_RaknetConnector_rawIp, 0x378);

    // --- LevelRenderer ---
    DEFINE_FIELD(Flarial_LevelRender_getLevelRendererPlayer, 0x318);
    DEFINE_FIELD(Flarial_LevelRendererPlayer_cameraPos, 0x620);

    // --- NetworkSystem ---
    DEFINE_FIELD(Flarial_NetworkSystem_remoteConnectorComposite, 0x90);
    DEFINE_FIELD(Flarial_RemoteConnectorComposite_netherNetConnector, 0x68);
    DEFINE_FIELD(Flarial_RemoteConnectorComposite_rakNetConnector, 0x70);
    DEFINE_FIELD(Flarial_NetherNetConnector_mPeers, 0x1F0);

    // --- ScreenContext ---
    DEFINE_FIELD(Flarial_ScreenContext_tessellator, 0xC8);

    // --- Biome ---
    DEFINE_FIELD(Flarial_Biome_name, 0x10);


    static void initialize();
    static void deinitialize();
};
