//
// СГЕНЕРИРОВАНО tools/ll_offsets.py — НЕ ПРАВИТЬ РУКАМИ.
// Источник: заголовки LeviLamina 26.51 (TypedStorage<Align, Size, Type>).
//
// Смещения РАСЧЁТНЫЕ: поля идут в порядке объявления, выравнивание учитывается.
// Классы с базовым классом помечены BASE_UNKNOWN — смещение от начала своего блока.
// Перед использованием в бою проверяй в IDA (это экономия времени, не замена реверсу).
//
#pragma once

#include <cstddef>

namespace Offsets_1_26 {

// AABBRelativeSizeUpdateComponent  [src/mc/deps/vanilla_components/AABBRelativeSizeUpdateComponent.h]  расчётный размер 0x30
namespace AABBRelativeSizeUpdateComponent {
    constexpr ptrdiff_t mHeadScalar = 0x0;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t mEyeScalar = 0xC;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t mBreathingScalar = 0x18;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t mMouthScalar = 0x24;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace AABBRelativeSizeUpdateComponent

// AABBShapeComponent  [src/mc/deps/vanilla_components/AABBShapeComponent.h]  расчётный размер 0x20
namespace AABBShapeComponent {
    constexpr ptrdiff_t mAABB = 0x0;  // ::AABB, 24 байт
    constexpr ptrdiff_t mBBDim = 0x18;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace AABBShapeComponent

// AbilitiesComponent  [src/mc/entity/components/AbilitiesComponent.h]  расчётный размер 0x5B8
namespace AbilitiesComponent {
    constexpr ptrdiff_t mAbilities = 0x0;  // ::LayeredAbilities, 1464 байт
    constexpr ptrdiff_t Size = 0x5B8;
}  // namespace AbilitiesComponent

// AbilitiesRequestComponent  [src/mc/entity/components/AbilitiesRequestComponent.h]  расчётный размер 0x18
namespace AbilitiesRequestComponent {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::vector<::AbilitiesRequest>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace AbilitiesRequestComponent

// AbsoluteSizeUpdateComponent  [src/mc/deps/vanilla_components/AbsoluteSizeUpdateComponent.h]  расчётный размер 0x40
namespace AbsoluteSizeUpdateComponent {
    constexpr ptrdiff_t mHeadOffset = 0x0;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t mEyeOffset = 0x10;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t mBreathingOffset = 0x20;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t mMouthOffset = 0x30;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace AbsoluteSizeUpdateComponent

// Action  [src-client/mc/client/input/vanilla/remapping/Action.h]  расчётный размер 0x10
namespace Action {
    constexpr ptrdiff_t mAction = 0x0;  // ::Remapping::ActionEnum, 4 байт
    constexpr ptrdiff_t mActionName = 0x8;  // char const*, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace Action

// Actor  [src/mc/world/actor/Actor.h]  расчётный размер 0x3A8
namespace Actor {
    constexpr ptrdiff_t mEntityContext = 0x0;  // ::EntityContext, 24 байт
    constexpr ptrdiff_t mInitParams = 0x18;  // ::VariantParameterList, 72 байт
    constexpr ptrdiff_t mCustomInitEventName = 0x60;  // ::std::string, 32 байт
    constexpr ptrdiff_t mInitMethod = 0x80;  // ::ActorInitializationMethod, 1 байт
    constexpr ptrdiff_t mForceInitMethodToSpawnOnReload = 0x81;  // bool, 1 байт
    constexpr ptrdiff_t mAdded = 0x82;  // bool, 1 байт
    constexpr ptrdiff_t mDefinitions = 0x88;  // ::ActorDefinitionGroup*, 8 байт
    constexpr ptrdiff_t mCurrentDescription = 0x90;  // ::std::unique_ptr<::ActorDefinitionDescriptor>, 8 байт
    constexpr ptrdiff_t mLeashRopeSystem = 0x98;  // ::std::shared_ptr<::RopeSystem>, 16 байт
    constexpr ptrdiff_t mAlias = 0xA8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mPreviousRenderTransform = 0xC8;  // ::std::optional<::glm::mat4x4>, 68 байт
    constexpr ptrdiff_t mLastHurtByPlayerTime = 0x10C;  // int, 4 байт
    constexpr ptrdiff_t mPreviousBoneMatrices = 0x110;  // ::std::map<::HashedString, ::std::vector<::std::vector<::glm::mat4x4>>>, 16 байт
    constexpr ptrdiff_t mEntityData = 0x120;  // ::SynchedActorDataEntityWrapper, 48 байт
    constexpr ptrdiff_t mNetworkData = 0x150;  // ::std::unique_ptr<::SpatialActorNetworkData>, 8 байт
    constexpr ptrdiff_t mSentDelta = 0x158;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mScale = 0x164;  // float, 4 байт
    constexpr ptrdiff_t mScalePrev = 0x168;  // float, 4 байт
    constexpr ptrdiff_t mIgnoreLighting = 0x16C;  // bool, 1 байт
    constexpr ptrdiff_t mFilterLighting = 0x16D;  // bool, 1 байт
    constexpr ptrdiff_t mStepSoundVolume = 0x170;  // float, 4 байт
    constexpr ptrdiff_t mStepSoundPitch = 0x174;  // float, 4 байт
    constexpr ptrdiff_t mLastHitBB = 0x178;  // ::AABB*, 8 байт
    constexpr ptrdiff_t mNameTagHash = 0x180;  // uint64, 8 байт
    constexpr ptrdiff_t mShadowOffset = 0x188;  // float, 4 байт
    constexpr ptrdiff_t mPushThrough = 0x18C;  // float, 4 байт
    constexpr ptrdiff_t mTickCount = 0x190;  // int, 4 байт
    constexpr ptrdiff_t mInvulnerableTime = 0x194;  // int, 4 байт
    constexpr ptrdiff_t mLastHealth = 0x198;  // int, 4 байт
    constexpr ptrdiff_t mInvulnerable = 0x19C;  // bool, 1 байт
    constexpr ptrdiff_t mFlameTexFrameIndex = 0x1A0;  // int, 4 байт
    constexpr ptrdiff_t mFlameFrameIncrementTime = 0x1A4;  // float, 4 байт
    constexpr ptrdiff_t mAlwaysFireImmune = 0x1A8;  // bool, 1 байт
    constexpr ptrdiff_t mInheritRotationWhenRiding = 0x1A9;  // bool, 1 байт
    constexpr ptrdiff_t mForcedLoading = 0x1AA;  // bool, 1 байт
    constexpr ptrdiff_t mHighlightedThisFrame = 0x1AB;  // bool, 1 байт
    constexpr ptrdiff_t mInitialized = 0x1AC;  // bool, 1 байт
    constexpr ptrdiff_t mProcessedOnChunkDiscard = 0x1AD;  // bool, 1 байт
    constexpr ptrdiff_t mSoundVolume = 0x1B0;  // float, 4 байт
    constexpr ptrdiff_t mShakeTime = 0x1B4;  // int, 4 байт
    constexpr ptrdiff_t mLegacyUniqueID = 0x1B8;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mDimension = 0x1C0;  // ::WeakRef<::Dimension>, 16 байт
    constexpr ptrdiff_t mLevel = 0x1D0;  // ::ILevel*, 8 байт
    constexpr ptrdiff_t mActorRendererId = 0x1D8;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mCategories = 0x208;  // ::ActorCategory, 4 байт
    constexpr ptrdiff_t mVersion = 0x20C;  // ::ActorVersion, 4 байт
    constexpr ptrdiff_t mBuiltInComponents = 0x210;  // ::BuiltInActorComponents, 32 байт
    constexpr ptrdiff_t mActorRendererIdThatAnimationComponentWasInitializedWith = 0x230;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mChanged = 0x260;  // bool, 1 байт
    constexpr ptrdiff_t mRemoved = 0x261;  // bool, 1 байт
    constexpr ptrdiff_t mMovedToLimbo = 0x262;  // bool, 1 байт
    constexpr ptrdiff_t mMovedToUnloadedChunk = 0x263;  // bool, 1 байт
    constexpr ptrdiff_t mBlocksBuilding = 0x264;  // bool, 1 байт
    constexpr ptrdiff_t mAnimationComponent = 0x268;  // ::std::shared_ptr<::AnimationComponent>, 16 байт
    constexpr ptrdiff_t mUIAnimationComponent = 0x278;  // ::std::shared_ptr<::AnimationComponent>, 16 байт
    constexpr ptrdiff_t mTargetId = 0x288;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mInLovePartner = 0x290;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mPersistingTradeOffers = 0x298;  // ::std::unique_ptr<::CompoundTag>, 8 байт
    constexpr ptrdiff_t mPersistingTradeRiches = 0x2A0;  // int, 4 байт
    constexpr ptrdiff_t mPersistingTrade = 0x2A4;  // bool, 1 байт
    constexpr ptrdiff_t mEffectsDirty = 0x2A5;  // bool, 1 байт
    constexpr ptrdiff_t mLootDropped = 0x2A6;  // bool, 1 байт
    constexpr ptrdiff_t mLoadedFromNBTThisFrame = 0x2A7;  // bool, 1 байт
    constexpr ptrdiff_t mHurtColor = 0x2A8;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mDefinitionList = 0x2B8;  // ::std::unique_ptr<::ActorDefinitionDiffList>, 8 байт
    constexpr ptrdiff_t mLoadedActorPropertyTag = 0x2C0;  // ::std::unique_ptr<::CompoundTag>, 8 байт
    constexpr ptrdiff_t mActorDefinitionPtr = 0x2C8;  // ::ActorDefinitionPtr, 16 байт
    constexpr ptrdiff_t mFilteredNameTag = 0x2D8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mTerrainInterlockData = 0x2F8;  // ::ActorTerrainInterlockData, 24 байт
    constexpr ptrdiff_t mWasHurtThisTick = 0x310;  // bool, 1 байт
    constexpr ptrdiff_t mWasHurtLastTick = 0x311;  // bool, 1 байт
    constexpr ptrdiff_t mLastHurtMobId = 0x318;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mLastHurtByMobId = 0x320;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mLastHurtByPlayerId = 0x328;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mLastHurtTimestamp = 0x330;  // uint64, 8 байт
    constexpr ptrdiff_t mLastHurtCause = 0x338;  // ::SharedTypes::Legacy::ActorDamageCause, 4 байт
    constexpr ptrdiff_t mLastHurt = 0x33C;  // float, 4 байт
    constexpr ptrdiff_t mLastHurtMobTimestamp = 0x340;  // int, 4 байт
    constexpr ptrdiff_t mLastHurtByMobTime = 0x344;  // int, 4 байт
    constexpr ptrdiff_t mLastHurtByMobTimestamp = 0x348;  // int, 4 байт
    constexpr ptrdiff_t mIsPredictableProjectile = 0x34C;  // bool, 1 байт
    constexpr ptrdiff_t mIsRenderingInUI = 0x34D;  // bool, 1 байт
    constexpr ptrdiff_t mIsOnScreen = 0x34E;  // bool, 1 байт
    constexpr ptrdiff_t mUpdateBonesAndEffects = 0x34F;  // bool, 1 байт
    constexpr ptrdiff_t mUpdateEffects = 0x350;  // bool, 1 байт
    constexpr ptrdiff_t mCanPickupItems = 0x351;  // bool, 1 байт
    constexpr ptrdiff_t mHasSetCanPickupItems = 0x352;  // bool, 1 байт
    constexpr ptrdiff_t mChainedDamageEffects = 0x353;  // bool, 1 байт
    constexpr ptrdiff_t mAffectedByWaterBottleTicksToEffect = 0x354;  // int, 4 байт
    constexpr ptrdiff_t mSpawnRulesEnum = 0x358;  // ::SpawnRuleEnum, 4 байт
    constexpr ptrdiff_t mActionQueue = 0x360;  // ::std::unique_ptr<::ActionQueue>, 8 байт
    constexpr ptrdiff_t mMolangVariables = 0x368;  // ::MolangVariableMap, 56 байт
    constexpr ptrdiff_t mFishingHookID = 0x3A0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t Size = 0x3A8;
}  // namespace Actor

// ActorBlockSyncMessage  [src/mc/world/level/ActorBlockSyncMessage.h]  расчётный размер 0xC
namespace ActorBlockSyncMessage {
    constexpr ptrdiff_t mEntityUniqueID = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mMessage = 0x8;  // ::ActorBlockSyncMessage::MessageId, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace ActorBlockSyncMessage

// ActorDataDirtyFlagsComponent  [src/mc/deps/vanilla_components/ActorDataDirtyFlagsComponent.h]  расчётный размер 0x18
namespace ActorDataDirtyFlagsComponent {
    constexpr ptrdiff_t mDirtyFlags = 0x0;  // ::std::bitset<141>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ActorDataDirtyFlagsComponent

// ActorDefinitionIdentifierComponent  [src/mc/entity/components/ActorDefinitionIdentifierComponent.h]  расчётный размер 0xB0
namespace ActorDefinitionIdentifierComponent {
    constexpr ptrdiff_t mIdentifier = 0x0;  // ::ActorDefinitionIdentifier, 176 байт
    constexpr ptrdiff_t Size = 0xB0;
}  // namespace ActorDefinitionIdentifierComponent

// ActorEquipmentComponent  [src/mc/entity/components/ActorEquipmentComponent.h]  расчётный размер 0x10
namespace ActorEquipmentComponent {
    constexpr ptrdiff_t mHand = 0x0;  // ::std::unique_ptr<::SimpleContainer>, 8 байт
    constexpr ptrdiff_t mArmor = 0x8;  // ::std::unique_ptr<::SimpleContainer>, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace ActorEquipmentComponent

// ActorEventPacket  [src/mc/network/packet/ActorEventPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ActorEventPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ActorEventPacket

// ActorGameTypeComponent  [src/mc/entity/components/ActorGameTypeComponent.h]  расчётный размер 0x4
namespace ActorGameTypeComponent {
    constexpr ptrdiff_t mGameType = 0x0;  // ::GameType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ActorGameTypeComponent

// ActorHeadRotationComponent  [src/mc/entity/components/ActorHeadRotationComponent.h]  расчётный размер 0x8
namespace ActorHeadRotationComponent {
    constexpr ptrdiff_t mYHeadRot = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mYHeadRotO = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ActorHeadRotationComponent

// ActorLink  [src/mc/world/actor/ActorLink.h]  расчётный размер 0x20
namespace ActorLink {
    constexpr ptrdiff_t type = 0x0;  // ::ActorLinkType, 1 байт
    constexpr ptrdiff_t A = 0x8;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t B = 0x10;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mImmediate = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mPassengerInitiated = 0x19;  // bool, 1 байт
    constexpr ptrdiff_t mVehicleAngularVelocity = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace ActorLink

// ActorOwnerComponent  [src/mc/entity/components/ActorOwnerComponent.h]  расчётный размер 0x8
namespace ActorOwnerComponent {
    constexpr ptrdiff_t mActor = 0x0;  // ::std::unique_ptr<::Actor>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ActorOwnerComponent

// ActorRenderData  [src-client/mc/client/renderer/actor/ActorRenderData.h]  расчётный размер 0x50
namespace ActorRenderData {
    constexpr ptrdiff_t mActor = 0x0;  // ::Actor*, 8 байт
    constexpr ptrdiff_t mItemStack = 0x8;  // ::ItemStack const*, 8 байт
    constexpr ptrdiff_t position = 0x10;  // ::Vec3, 12 байт
    constexpr ptrdiff_t rotation = 0x1C;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mHeadRot = 0x24;  // ::Vec2, 8 байт
    constexpr ptrdiff_t glint = 0x2C;  // bool, 1 байт
    constexpr ptrdiff_t mIgnoreLighting = 0x2D;  // bool, 1 байт
    constexpr ptrdiff_t mIsInUI = 0x2E;  // bool, 1 байт
    constexpr ptrdiff_t mDeltaTime = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mAnimationComponent = 0x38;  // ::AnimationComponent&, 8 байт
    constexpr ptrdiff_t mVariables = 0x40;  // ::MolangVariableMap&, 8 байт
    constexpr ptrdiff_t mCSSMetadata = 0x48;  // ::mce::framebuilder::CustomSurfaceShaderMetadata, 8 байт
    constexpr ptrdiff_t Size = 0x50;
}  // namespace ActorRenderData

// ActorRotationComponent  [src/mc/entity/components/ActorRotationComponent.h]  расчётный размер 0x10
namespace ActorRotationComponent {
    constexpr ptrdiff_t mRot = 0x0;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mRotPrev = 0x8;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace ActorRotationComponent

// ActorTickNeededComponent  [src/mc/entity/components/ActorTickNeededComponent.h]  расчётный размер 0x10
namespace ActorTickNeededComponent {
    constexpr ptrdiff_t mBlockSource = 0x0;  // ::WeakRef<::BlockSource>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace ActorTickNeededComponent

// ActorTypeComponent  [src/mc/deps/vanilla_components/ActorTypeComponent.h]  расчётный размер 0x4
namespace ActorTypeComponent {
    constexpr ptrdiff_t mType = 0x0;  // ::ActorType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ActorTypeComponent

// ActorUniqueIDComponent  [src/mc/entity/components/ActorUniqueIDComponent.h]  расчётный размер 0x8
namespace ActorUniqueIDComponent {
    constexpr ptrdiff_t mActorUniqueID = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ActorUniqueIDComponent

// ActorWalkAnimationComponent  [src/mc/entity/components/ActorWalkAnimationComponent.h]  расчётный размер 0x14
namespace ActorWalkAnimationComponent {
    constexpr ptrdiff_t mWalkAnimSpeedMultiplier = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mWalkAnimSpeedO = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mWalkAnimSpeed = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mWalkAnimPos = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mMovementThisTick = 0x10;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace ActorWalkAnimationComponent

// AddActorPacket  [src/mc/network/packet/AddActorPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace AddActorPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace AddActorPacket

// AddRiderComponent  [src/mc/entity/components/AddRiderComponent.h]  расчётный размер 0x18
namespace AddRiderComponent {
    constexpr ptrdiff_t mPassengersTypes = 0x0;  // ::std::vector<::ActorDefinitionIdentifier>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace AddRiderComponent

// AdmireItemComponent  [src/mc/entity/components/AdmireItemComponent.h]  расчётный размер 0xC0
namespace AdmireItemComponent {
    constexpr ptrdiff_t mIsAdmiring = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mAdmireItem = 0x8;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t mAdmireUntil = 0xA0;  // ::Tick, 8 байт
    constexpr ptrdiff_t mItemOwnerRef = 0xA8;  // ::WeakEntityRef, 24 байт
    constexpr ptrdiff_t Size = 0xC0;
}  // namespace AdmireItemComponent

// AgeableComponent  [src/mc/entity/components/AgeableComponent.h]  расчётный размер 0x5
namespace AgeableComponent {
    constexpr ptrdiff_t mAge = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mGrowthPaused = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x5;
}  // namespace AgeableComponent

// AgentCommandComponent  [src/mc/entity/components/AgentCommandComponent.h]  расчётный размер 0x8
namespace AgentCommandComponent {
    constexpr ptrdiff_t mCurrentCommand = 0x0;  // ::std::unique_ptr<::AgentCommands::Command>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace AgentCommandComponent

// AllowOffHandItemComponent  [src/mc/deps/shared_types/beta/item/AllowOffHandItemComponent.h]  расчётный размер 0x1
namespace AllowOffHandItemComponent {
    constexpr ptrdiff_t mAllowOffHand = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace AllowOffHandItemComponent

// AmbientSoundComponent  [src/mc/entity/components/AmbientSoundComponent.h]  расчётный размер 0x10
namespace AmbientSoundComponent {
    constexpr ptrdiff_t mNeedsInitialization = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mMinimumTimeTicks = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mMaxAdditionalTimeTicks = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mRemainingTimeTicks = 0xC;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace AmbientSoundComponent

// AmbientSoundServerComponent  [src/mc/entity/components/AmbientSoundServerComponent.h]  расчётный размер 0x40
namespace AmbientSoundServerComponent {
    constexpr ptrdiff_t mDynamicSounds = 0x0;  // ::std::vector<::DynamicAmbientSound>, 24 байт
    constexpr ptrdiff_t mDefaultEventName = 0x18;  // ::std::string, 32 байт
    constexpr ptrdiff_t mReevaluationTimeTicks = 0x38;  // uint, 4 байт
    constexpr ptrdiff_t mTicksSinceReevaluation = 0x3C;  // uint, 4 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace AmbientSoundServerComponent

// AngerLevelComponent  [src/mc/entity/components_json_legacy/AngerLevelComponent.h]  расчётный размер 0xD8
namespace AngerLevelComponent {
    constexpr ptrdiff_t mSoundEvent = 0x0;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mCondition = 0x8;  // ::ExpressionNode, 16 байт
    constexpr ptrdiff_t mActorId = 0x18;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mAnger = 0x20;  // int, 4 байт
    constexpr ptrdiff_t mPriority = 0x24;  // uchar, 1 байт
    constexpr ptrdiff_t mAngerThreshold = 0x28;  // int, 4 байт
    constexpr ptrdiff_t mNextAngerDecrementTick = 0x30;  // ::Tick, 8 байт
    constexpr ptrdiff_t mDefaultAnnoyingness = 0x38;  // int, 4 байт
    constexpr ptrdiff_t mDefaultProjectileAnnoyingness = 0x3C;  // int, 4 байт
    constexpr ptrdiff_t mAngerDecrementIntervalTicks = 0x40;  // int, 4 байт
    constexpr ptrdiff_t mMaxAngerLevel = 0x44;  // int, 4 байт
    constexpr ptrdiff_t mAngryThreshold = 0x48;  // int, 4 байт
    constexpr ptrdiff_t mRemoveTargetWhenBelowAngryThreshold = 0x4C;  // bool, 1 байт
    constexpr ptrdiff_t mAngryBoost = 0x50;  // int, 4 байт
    constexpr ptrdiff_t mTopAnger = 0x54;  // int, 4 байт
    constexpr ptrdiff_t mNuisanceFilter = 0x58;  // ::std::optional<::ActorFilterGroup>, 72 байт
    constexpr ptrdiff_t mOnIncreaseSoundDefinitions = 0xA0;  // ::std::vector<::AngerLevelComponent::ConditionalSound>, 24 байт
    constexpr ptrdiff_t mNuisanceCompare = 0xB8;  // ::AngerLevelComponent::NuisanceCompare, 4 байт
    constexpr ptrdiff_t mNuisances = 0xC0;  // ::std::multiset<::AngerLevelComponent::Nuisance, ::AngerLevelComponent::NuisanceCompare>, 24 байт
    constexpr ptrdiff_t Size = 0xD8;
}  // namespace AngerLevelComponent

// AngryComponent  [src/mc/entity/components_json_legacy/AngryComponent.h]  расчётный размер 0x98
namespace AngryComponent {
    constexpr ptrdiff_t mDuration = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mHasTicked = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mBroadcastAnger = 0x5;  // bool, 1 байт
    constexpr ptrdiff_t mBroadcastOnAttack = 0x6;  // bool, 1 байт
    constexpr ptrdiff_t mBroadcastOnBeingAttacked = 0x7;  // bool, 1 байт
    constexpr ptrdiff_t mBroadcastAngerWhenDying = 0x8;  // bool, 1 байт
    constexpr ptrdiff_t mBroadcastRange = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mBroadcastFilter = 0x10;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mSubjectFilter = 0x50;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mNextSoundEventTick = 0x90;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x98;
}  // namespace AngryComponent

// AnimatePacket  [src/mc/network/packet/AnimatePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace AnimatePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace AnimatePacket

// AnimatedImageData  [src/mc/world/actor/player/AnimatedImageData.h]  расчётный размер 0x3C
namespace AnimatedImageData {
    constexpr ptrdiff_t mType = 0x0;  // ::persona::AnimatedTextureType, 4 байт
    constexpr ptrdiff_t mAnimationExpression = 0x4;  // ::persona::AnimationExpression, 4 байт
    constexpr ptrdiff_t mImage = 0x8;  // ::SkinImage, 48 байт
    constexpr ptrdiff_t mFrames = 0x38;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x3C;
}  // namespace AnimatedImageData

// AreaAttackComponent  [src/mc/entity/components_json_legacy/AreaAttackComponent.h]  расчётный размер 0x98
namespace AreaAttackComponent {
    constexpr ptrdiff_t mDamageRange = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mDamagePerTick = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mDamageCause = 0x10;  // ::SharedTypes::Legacy::ActorDamageCause, 4 байт
    constexpr ptrdiff_t mEntityFilter = 0x18;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mAttackCooldownTicks = 0x58;  // int, 4 байт
    constexpr ptrdiff_t mPlayAttackSound = 0x5C;  // bool, 1 байт
    constexpr ptrdiff_t mUseSelfAsDamageSource = 0x5D;  // bool, 1 байт
    constexpr ptrdiff_t mDeathMessageOverride = 0x60;  // ::std::string, 32 байт
    constexpr ptrdiff_t mAttackCooldownVector = 0x80;  // ::std::vector<::std::pair<::ActorUniqueID, ::Tick>>, 24 байт
    constexpr ptrdiff_t Size = 0x98;
}  // namespace AreaAttackComponent

// AttackAnimationComponent  [src/mc/entity/components/AttackAnimationComponent.h]  расчётный размер 0x4
namespace AttackAnimationComponent {
    constexpr ptrdiff_t mAttackAnimationTick = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace AttackAnimationComponent

// AttackCooldownComponent  [src/mc/entity/components/AttackCooldownComponent.h]  расчётный размер 0x9
namespace AttackCooldownComponent {
    constexpr ptrdiff_t mCompleteTick = 0x0;  // ::Tick, 8 байт
    constexpr ptrdiff_t mHasExecuted = 0x8;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x9;
}  // namespace AttackCooldownComponent

// Attribute  [src/mc/world/attribute/Attribute.h]  расчётный размер 0x38
namespace Attribute {
    constexpr ptrdiff_t mRedefinitionMode = 0x0;  // ::RedefinitionMode, 1 байт
    constexpr ptrdiff_t mSyncable = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mIDValue = 0x4;  // uint, 4 байт
    constexpr ptrdiff_t mName = 0x8;  // ::HashedString, 48 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace Attribute

// AttributeInstance  [src/mc/world/attribute/AttributeInstance.h]  расчётный размер 0x91
namespace AttributeInstance {
    constexpr ptrdiff_t mAttribute = 0x0;  // ::Attribute const*, 8 байт
    constexpr ptrdiff_t mModifierList = 0x8;  // ::std::vector<::AttributeModifier>, 24 байт
    constexpr ptrdiff_t mTemporalBuffs = 0x20;  // ::std::vector<::TemporalAttributeBuff>, 24 байт
    constexpr ptrdiff_t mListeners = 0x38;  // ::std::vector<::AttributeInstanceHandle>, 24 байт
    constexpr ptrdiff_t mDelegate = 0x50;  // ::std::shared_ptr<::AttributeInstanceDelegate>, 16 байт
    constexpr ptrdiff_t mDefaultValues = 0x60;  // float[3], 12 байт
    constexpr ptrdiff_t mDefaultMinValue = 0x6C;  // float, 4 байт
    constexpr ptrdiff_t mDefaultMaxValue = 0x70;  // float, 4 байт
    constexpr ptrdiff_t mDefaultValue = 0x74;  // float, 4 байт
    constexpr ptrdiff_t mCurrentValues = 0x78;  // float[3], 12 байт
    constexpr ptrdiff_t mCurrentMinValue = 0x84;  // float, 4 байт
    constexpr ptrdiff_t mCurrentMaxValue = 0x88;  // float, 4 байт
    constexpr ptrdiff_t mCurrentValue = 0x8C;  // float, 4 байт
    constexpr ptrdiff_t mValueChangedDuringTick = 0x90;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x91;
}  // namespace AttributeInstance

// AttributesComponent  [src/mc/entity/components/AttributesComponent.h]  расчётный размер 0x50
namespace AttributesComponent {
    constexpr ptrdiff_t mAttributes = 0x0;  // ::BaseAttributeMap, 80 байт
    constexpr ptrdiff_t Size = 0x50;
}  // namespace AttributesComponent

// AudioEmitterComponent  [src/mc/entity/components/AudioEmitterComponent.h]  расчётный размер 0x18
namespace AudioEmitterComponent {
    constexpr ptrdiff_t mNewEventsToPlay = 0x0;  // ::std::vector<::AudioEmitterComponent::SoundEvent>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace AudioEmitterComponent

// BalloonComponent  [src/mc/entity/components_json_legacy/BalloonComponent.h]  расчётный размер 0xD
namespace BalloonComponent {
    constexpr ptrdiff_t mAttachedID = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mMaxHeight = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mShouldDropAttached = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xD;
}  // namespace BalloonComponent

// BarterComponent  [src/mc/entity/components_json_legacy/BarterComponent.h]  расчётный размер 0x2F
namespace BarterComponent {
    constexpr ptrdiff_t mItemOriginationActorUniqueID = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mBarterItemsTable = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mCooldown = 0x28;  // int, 4 байт
    constexpr ptrdiff_t mBarterItemId = 0x2C;  // short, 2 байт
    constexpr ptrdiff_t mExecuteTrade = 0x2E;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2F;
}  // namespace BarterComponent

// BaseActorRenderContext  [src-client/mc/client/renderer/BaseActorRenderContext.h]  расчётный размер 0x2F6
namespace BaseActorRenderContext {
    constexpr ptrdiff_t mOffscreenCaptureDescription = 0x0;  // ::OffscreenCaptureDescription, 40 байт
    constexpr ptrdiff_t mCameraTargetPosition = 0x28;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCameraPosition = 0x34;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mWorldClipRegion = 0x40;  // ::AABB, 24 байт
    constexpr ptrdiff_t mHistoricalFrameTimes = 0x58;  // ::HistoricalFrameTimes, 488 байт
    constexpr ptrdiff_t mNumEntitiesRenderedThisFrame = 0x240;  // float, 4 байт
    constexpr ptrdiff_t mLastFrameTime = 0x244;  // float, 4 байт
    constexpr ptrdiff_t mSortedMeshDrawList = 0x248;  // ::SortedMeshDrawList*, 8 байт
    constexpr ptrdiff_t mClientInstance = 0x250;  // ::IClientInstance&, 8 байт
    constexpr ptrdiff_t mMinecraftGame = 0x258;  // ::IMinecraftGame&, 8 байт
    constexpr ptrdiff_t mScreenContext = 0x260;  // ::ScreenContext&, 8 байт
    constexpr ptrdiff_t mBlockEntityRenderDispatcher = 0x268;  // ::BlockActorRenderDispatcher&, 8 байт
    constexpr ptrdiff_t mEntityRenderDispatcher = 0x270;  // ::std::shared_ptr<::ActorRenderDispatcher>, 16 байт
    constexpr ptrdiff_t mEntityBlockRenderer = 0x280;  // ::ActorBlockRenderer&, 8 байт
    constexpr ptrdiff_t mItemInHandRenderer = 0x288;  // ::ItemInHandRenderer&, 8 байт
    constexpr ptrdiff_t mItemRenderer = 0x290;  // ::ItemRenderer&, 8 байт
    constexpr ptrdiff_t mHudIconActorRenderer = 0x298;  // ::HudIconActorRenderer&, 8 байт
    constexpr ptrdiff_t mLightVolumeManager = 0x2A0;  // ::std::weak_ptr<::LightPropagation::LightVolumeManager>, 16 байт
    constexpr ptrdiff_t mParticleSystemEngine = 0x2B0;  // ::ParticleSystemEngine*, 8 байт
    constexpr ptrdiff_t mProcessedV2DataDrivenRenderers = 0x2B8;  // ::DataDrivenRendererRegistry const*, 8 байт
    constexpr ptrdiff_t mV2DataDrivenContinuousData = 0x2C0;  // ::DataDrivenRendererContinuousData*, 8 байт
    constexpr ptrdiff_t mV2ShouldRenderActorsWithAttachables = 0x2C8;  // bool, 1 байт
    constexpr ptrdiff_t mRenderUniqueIdOverride = 0x2D0;  // ::std::optional<int64>, 16 байт
    constexpr ptrdiff_t mImpl = 0x2E0;  // ::std::unique_ptr<::BaseActorRenderContext::Impl>, 8 байт
    constexpr ptrdiff_t mFrameAlpha = 0x2E8;  // float const, 4 байт
    constexpr ptrdiff_t mActorDitheringMaxDistance = 0x2EC;  // float, 4 байт
    constexpr ptrdiff_t mIsOnScreen = 0x2F0;  // bool, 1 байт
    constexpr ptrdiff_t mUpdateBonesAndEffects = 0x2F1;  // bool, 1 байт
    constexpr ptrdiff_t mUpdateEffects = 0x2F2;  // bool, 1 байт
    constexpr ptrdiff_t mIgnoresLighting = 0x2F3;  // bool, 1 байт
    constexpr ptrdiff_t mForceLightingOn = 0x2F4;  // bool, 1 байт
    constexpr ptrdiff_t mIsRenderingOnMap = 0x2F5;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2F6;
}  // namespace BaseActorRenderContext

// BaseAttributeMap  [src/mc/world/attribute/BaseAttributeMap.h]  расчётный размер 0x20
namespace BaseAttributeMap {
    constexpr ptrdiff_t mDirtyAttributes = 0x0;  // ::std::vector<::AttributeInstanceHandle>, 24 байт
    constexpr ptrdiff_t mOnAttributeModified = 0x18;  // void (BaseAttributeMap::*)(::AttributeInstance const&), 8 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace BaseAttributeMap

// BaseGameVersionComponent  [src/mc/entity/components/BaseGameVersionComponent.h]  расчётный размер 0x20
namespace BaseGameVersionComponent {
    constexpr ptrdiff_t mBaseGameVersion = 0x0;  // ::BaseGameVersion, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace BaseGameVersionComponent

// BehaviorComponent  [src/mc/entity/components/BehaviorComponent.h]  расчётный размер 0x70
namespace BehaviorComponent {
    constexpr ptrdiff_t mTreeDefinition = 0x0;  // ::BehaviorTreeDefinitionPtr, 16 байт
    constexpr ptrdiff_t mRoot = 0x10;  // ::std::unique_ptr<::BehaviorNode>, 8 байт
    constexpr ptrdiff_t mBehaviorData = 0x18;  // ::BehaviorData, 88 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace BehaviorComponent

// Blob  [src/mc/deps/cereal/schema/blob/Blob.h]  расчётный размер 0x18
namespace Blob {
    constexpr ptrdiff_t mView = 0x0;  // ::gsl::span<uchar const>, 16 байт
    constexpr ptrdiff_t mData = 0x10;  // ::std::unique_ptr<uchar[0]>, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace Blob

// Block  [src/mc/world/level/block/Block.h]  расчётный размер 0x11B
namespace Block {
    constexpr ptrdiff_t mComponents = 0x0;  // ::BlockComponentStorage, 96 байт
    constexpr ptrdiff_t mBlockType = 0x60;  // ::gsl::not_null<::BlockType*>, 8 байт
    constexpr ptrdiff_t mCachedComponentData = 0x68;  // ::CachedComponentData, 8 байт
    constexpr ptrdiff_t mDirectData = 0x70;  // ::BlockComponentDirectData, 96 байт
    constexpr ptrdiff_t mTags = 0xD0;  // ::std::vector<::HashedString>, 24 байт
    constexpr ptrdiff_t mSerializationId = 0xE8;  // ::CompoundTag, 24 байт
    constexpr ptrdiff_t mSerializationIdHash = 0x100;  // uint64, 8 байт
    constexpr ptrdiff_t mSerializationIdHashForNetwork = 0x108;  // uint, 4 байт
    constexpr ptrdiff_t mNetworkId = 0x10C;  // uint, 4 байт
    constexpr ptrdiff_t mClientData = 0x110;  // ::std::unique_ptr<::IClientBlockData>, 8 байт
    constexpr ptrdiff_t mData = 0x118;  // ushort const, 2 байт
    constexpr ptrdiff_t mHasRuntimeId = 0x11A;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x11B;
}  // namespace Block

// BlockBreakSensorComponent  [src/mc/entity/components_json_legacy/BlockBreakSensorComponent.h]  расчётный размер 0x60
namespace BlockBreakSensorComponent {
    constexpr ptrdiff_t mSensorRadius = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mSensorPos = 0x4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mListener = 0x10;  // ::BlockEventDispatcherToken, 32 байт
    constexpr ptrdiff_t mBlockSets = 0x30;  // ::std::vector<::BlockListEventMap>, 24 байт
    constexpr ptrdiff_t mSourceFilters = 0x48;  // ::std::vector<::ActorFilterGroup>, 24 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace BlockBreakSensorComponent

// BlockCollisionEvaluationQueueComponent  [src/mc/entity/components/BlockCollisionEvaluationQueueComponent.h]  расчётный размер 0x18
namespace BlockCollisionEvaluationQueueComponent {
    constexpr ptrdiff_t mToEvaluate = 0x0;  // ::std::vector<::BlockCollisionEvaluationQueueComponent::Data>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace BlockCollisionEvaluationQueueComponent

// BlockLayer  [src/mc/world/level/levelgen/flat/BlockLayer.h]  расчётный размер 0xC
namespace BlockLayer {
    constexpr ptrdiff_t mBlock = 0x0;  // ::Block const*, 8 байт
    constexpr ptrdiff_t mNumLayers = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace BlockLayer

// BlockMovementSlowdownAppliedComponent  [src/mc/entity/components/BlockMovementSlowdownAppliedComponent.h]  расчётный размер 0x1
namespace BlockMovementSlowdownAppliedComponent {
    constexpr ptrdiff_t mWasZero = 0x0;  // bool const, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace BlockMovementSlowdownAppliedComponent

// BlockPalette  [src/mc/world/level/BlockPalette.h]  расчётный размер 0x80
namespace BlockPalette {
    constexpr ptrdiff_t mLegacyBlockStatesConversionWarningMutex = 0x0;  // ::std::mutex, 80 байт
    constexpr ptrdiff_t mLegacyBlockStatesConversionWarningSet = 0x50;  // ::std::set<::std::pair<int, int>>, 16 байт
    constexpr ptrdiff_t mBlockFromNetworkId = 0x60;  // ::std::vector<::Block const*>, 24 байт
    constexpr ptrdiff_t mLevel = 0x78;  // ::Level*, 8 байт
    constexpr ptrdiff_t Size = 0x80;
}  // namespace BlockPalette

// BlockPosTrackerComponent  [src/mc/entity/components/BlockPosTrackerComponent.h]  расчётный размер 0x25
namespace BlockPosTrackerComponent {
    constexpr ptrdiff_t mPreviousOnGround = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mPreviousBlockPos = 0x4;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mCurrentBlock = 0x10;  // ::Block const*, 8 байт
    constexpr ptrdiff_t mCurrentBlockPos = 0x18;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mShouldTriggerOnStandOn = 0x24;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x25;
}  // namespace BlockPosTrackerComponent

// BlockSource  [src/mc/world/level/BlockSource.h]  расчётный размер 0x160
namespace BlockSource {
    constexpr ptrdiff_t mOwnerThreadID = 0x0;  // ::std::thread::id const, 4 байт
    constexpr ptrdiff_t mAllowUnpopulatedChunks = 0x4;  // bool const, 1 байт
    constexpr ptrdiff_t mPublicSource = 0x5;  // bool const, 1 байт
    constexpr ptrdiff_t mMarkChangedBlocksForSaving = 0x6;  // bool const, 1 байт
    constexpr ptrdiff_t mLevel = 0x8;  // ::Level&, 8 байт
    constexpr ptrdiff_t mChunkSource = 0x10;  // ::ChunkSource&, 8 байт
    constexpr ptrdiff_t mDimension = 0x18;  // ::Dimension&, 8 байт
    constexpr ptrdiff_t mMaxHeight = 0x20;  // short const, 2 байт
    constexpr ptrdiff_t mMinHeight = 0x22;  // short const, 2 байт
    constexpr ptrdiff_t mTempBlockFetchResult = 0x28;  // ::std::vector<::BlockDataFetchResult<::Block>>, 24 байт
    constexpr ptrdiff_t mAllowTickingChanges = 0x40;  // bool, 1 байт
    constexpr ptrdiff_t mProcessingConnectionUpdates = 0x41;  // bool, 1 байт
    constexpr ptrdiff_t mPendingConnectionUpdates = 0x48;  // ::std::deque<::BlockPos>, 40 байт
    constexpr ptrdiff_t mPlaceChunkPos = 0x70;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mListeners = 0x80;  // ::std::vector<::BlockSourceListener*>, 24 байт
    constexpr ptrdiff_t mIsPersistantBlockSource = 0x98;  // bool, 1 байт
    constexpr ptrdiff_t mLastChunkPos = 0xA0;  // ::ChunkPos, 8 байт
    constexpr ptrdiff_t mLastChunkWeakPtr = 0xA8;  // ::std::weak_ptr<::LevelChunk>, 16 байт
    constexpr ptrdiff_t mLastChunkDirectPtr = 0xB8;  // ::LevelChunk*, 8 байт
    constexpr ptrdiff_t mRandomTickQueue = 0xC0;  // ::BlockTickingQueue*, 8 байт
    constexpr ptrdiff_t mTickQueue = 0xC8;  // ::BlockTickingQueue*, 8 байт
    constexpr ptrdiff_t mDefaultBrightness = 0xD0;  // ::BrightnessPair const, 2 байт
    constexpr ptrdiff_t mTempEntityList = 0xD8;  // ::std::vector<::Actor*>, 24 байт
    constexpr ptrdiff_t mTempBlockEntityList = 0xF0;  // ::std::vector<::BlockActor*>, 24 байт
    constexpr ptrdiff_t mTempCubeList = 0x108;  // ::std::vector<::AABB>, 24 байт
    constexpr ptrdiff_t mPendingPreservedBlockEntityChunks = 0x120;  // ::std::unordered_set<::ChunkPos>, 64 байт
    constexpr ptrdiff_t Size = 0x160;
}  // namespace BlockSource

// BlockSourceComponent  [src/mc/entity/components/BlockSourceComponent.h]  расчётный размер 0x10
namespace BlockSourceComponent {
    constexpr ptrdiff_t mBlockSource = 0x0;  // ::WeakRef<::BlockSource>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace BlockSourceComponent

// BoatMovementComponent  [src/mc/entity/components/BoatMovementComponent.h]  расчётный размер 0x14
namespace BoatMovementComponent {
    constexpr ptrdiff_t mIsInAir = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mOutOfControlTicks = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mYRotD = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mInvFriction = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mAirFrictionWhenLeashed = 0x10;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace BoatMovementComponent

// BoatPaddleComponent  [src/mc/entity/components/BoatPaddleComponent.h]  расчётный размер 0x30
namespace BoatPaddleComponent {
    constexpr ptrdiff_t mPaddles = 0x0;  // ::std::array<::Paddle, 2>, 40 байт
    constexpr ptrdiff_t mLocalTick = 0x28;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace BoatPaddleComponent

// BodyControlComponent  [src/mc/entity/components/BodyControlComponent.h]  расчётный размер 0x8
namespace BodyControlComponent {
    constexpr ptrdiff_t mBodyControl = 0x0;  // ::std::unique_ptr<::BodyControl>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace BodyControlComponent

// BoostableComponent  [src/mc/entity/components_json_legacy/BoostableComponent.h]  расчётный размер 0x10
namespace BoostableComponent {
    constexpr ptrdiff_t mIsBoosting = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mBoostTime = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mBoostTimeTotal = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mSpeedMultiplier = 0xC;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace BoostableComponent

// BossComponent  [src/mc/entity/components_json_legacy/BossComponent.h]  расчётный размер 0xA8
namespace BossComponent {
    constexpr ptrdiff_t mName = 0x0;  // ::Bedrock::Safety::RedactableString, 72 байт
    constexpr ptrdiff_t mHealthBarVisible = 0x48;  // bool, 1 байт
    constexpr ptrdiff_t mHealthPercent = 0x4C;  // float, 4 байт
    constexpr ptrdiff_t mShouldDarkenSky = 0x50;  // bool, 1 байт
    constexpr ptrdiff_t mCreateWorldFog = 0x51;  // bool, 1 байт
    constexpr ptrdiff_t mColor = 0x52;  // ::BossBarColor, 1 байт
    constexpr ptrdiff_t mOverlay = 0x53;  // ::BossBarOverlay, 1 байт
    constexpr ptrdiff_t mPlayersRegistered = 0x54;  // int, 4 байт
    constexpr ptrdiff_t mLastHealth = 0x58;  // int, 4 байт
    constexpr ptrdiff_t mHudRangeSquared = 0x5C;  // int, 4 байт
    constexpr ptrdiff_t mLastPlayerUpdate = 0x60;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mPlayerParty = 0x68;  // ::std::unordered_map<::mce::UUID, int>, 64 байт
    constexpr ptrdiff_t Size = 0xA8;
}  // namespace BossComponent

// BounceComponent  [src/mc/entity/components/BounceComponent.h]  расчётный размер 0x1C
namespace BounceComponent {
    constexpr ptrdiff_t mBounceStartPos = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mBounceBlock = 0x10;  // ::Block const*, 8 байт
    constexpr ptrdiff_t mBounceHeight = 0x18;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace BounceComponent

// BreakDoorAnnotationComponent  [src/mc/entity/components/BreakDoorAnnotationComponent.h]  расчётный размер 0x30
namespace BreakDoorAnnotationComponent {
    constexpr ptrdiff_t mBreakTicks = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mMinDifficulty = 0x4;  // ::SharedTypes::Legacy::Difficulty, 4 байт
    constexpr ptrdiff_t mTargetID = 0x8;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mBreakingTime = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mObstructionPos = 0x14;  // ::std::optional<::BlockPos>, 16 байт
    constexpr ptrdiff_t mLastPathIndex = 0x28;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace BreakDoorAnnotationComponent

// BreathableComponent  [src/mc/entity/components_json_legacy/BreathableComponent.h]  расчётный размер 0x4C
namespace BreathableComponent {
    constexpr ptrdiff_t mSuffocateTime = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mInhaleTime = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mAirRegenPerTick = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mBreathesAir = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mBreathesWater = 0xD;  // bool, 1 байт
    constexpr ptrdiff_t mBreathesLava = 0xE;  // bool, 1 байт
    constexpr ptrdiff_t mBreathesSolids = 0xF;  // bool, 1 байт
    constexpr ptrdiff_t mGeneratesBubbles = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t mAirSupply = 0x12;  // short, 2 байт
    constexpr ptrdiff_t mAirSupplyMax = 0x14;  // short, 2 байт
    constexpr ptrdiff_t mBreathableBlocks = 0x18;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mNonBreathableBlocks = 0x30;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mBreathableState = 0x48;  // ::BreathableComponent::BreathableState, 4 байт
    constexpr ptrdiff_t Size = 0x4C;
}  // namespace BreathableComponent

// BreedableComponent  [src/mc/entity/components_json_legacy/BreedableComponent.h]  расчётный размер 0x30
namespace BreedableComponent {
    constexpr ptrdiff_t mPrimary = 0x0;  // ::Player const*, 8 байт
    constexpr ptrdiff_t mSecondary = 0x8;  // ::Player const*, 8 байт
    constexpr ptrdiff_t mStaticData = 0x10;  // ::BreedableDefinition const*, 8 байт
    constexpr ptrdiff_t mLoveTimer = 0x18;  // int, 4 байт
    constexpr ptrdiff_t mBreedCooldown = 0x1C;  // int, 4 байт
    constexpr ptrdiff_t mBreedCooldownTime = 0x20;  // int, 4 байт
    constexpr ptrdiff_t mLoveCause = 0x28;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace BreedableComponent

// BribeableComponent  [src/mc/entity/components_json_legacy/BribeableComponent.h]  расчётный размер 0x14
namespace BribeableComponent {
    constexpr ptrdiff_t mStaticData = 0x0;  // ::BribeableDefinition const*, 8 байт
    constexpr ptrdiff_t mBribeTimer = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mBribeCooldown = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mBribeCooldownTime = 0x10;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace BribeableComponent

// BrushEffectsCooldownComponent  [src/mc/entity/components/BrushEffectsCooldownComponent.h]  расчётный размер 0x10
namespace BrushEffectsCooldownComponent {
    constexpr ptrdiff_t mCooldown = 0x0;  // ::ExpiringTick, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace BrushEffectsCooldownComponent

// BundleInteractionItemComponent  [src/mc/deps/shared_types/v1_21_30/item/BundleInteractionItemComponent.h]  расчётный размер 0x4
namespace BundleInteractionItemComponent {
    constexpr ptrdiff_t mNumViewableSlots = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace BundleInteractionItemComponent

// BuoyancyComponent  [src/mc/entity/components_json_legacy/BuoyancyComponent.h]  расчётный размер 0x38
namespace BuoyancyComponent {
    constexpr ptrdiff_t mBaseBuoyancy = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mDragDownOnRemoval = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mBigWaveProbability = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mBigWaveSpeedMultiplier = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mMovementType = 0x10;  // ::BuoyancyMovementType, 1 байт
    constexpr ptrdiff_t mApplyGravity = 0x11;  // bool, 1 байт
    constexpr ptrdiff_t mCanAutoStepFromLiquid = 0x12;  // bool, 1 байт
    constexpr ptrdiff_t mTimer = 0x18;  // double, 8 байт
    constexpr ptrdiff_t mLiquidBlocks = 0x20;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace BuoyancyComponent

// BuoyancyFloatRequestComponent  [src/mc/entity/components_json_legacy/BuoyancyFloatRequestComponent.h]  расчётный размер 0x2
namespace BuoyancyFloatRequestComponent {
    constexpr ptrdiff_t mCanFloat = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mNeedToResurface = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace BuoyancyFloatRequestComponent

// BurnsInDaylightComponent  [src/mc/entity/components/BurnsInDaylightComponent.h]  расчётный размер 0x4
namespace BurnsInDaylightComponent {
    constexpr ptrdiff_t mProtectionSlot = 0x0;  // ::SharedTypes::Legacy::EquipmentSlot, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace BurnsInDaylightComponent

// CameraAPIComponent  [src/mc/deps/minecraft_camera/CameraAPIComponent.h]  расчётный размер 0x8
namespace CameraAPIComponent {
    constexpr ptrdiff_t mApi = 0x0;  // ::std::unique_ptr<::ICameraAPI>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CameraAPIComponent

// CameraActivationRequestComponent  [src/mc/deps/minecraft_camera/components/CameraActivationRequestComponent.h]  расчётный размер 0x4
namespace CameraActivationRequestComponent {
    constexpr ptrdiff_t mActivationPriority = 0x0;  // uint, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CameraActivationRequestComponent

// CameraAdjustedPositionComponent  [src/mc/deps/minecraft_camera/components/CameraAdjustedPositionComponent.h]  расчётный размер 0x24
namespace CameraAdjustedPositionComponent {
    constexpr ptrdiff_t mAdjustedPosition = 0x0;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mAttachOffset = 0xC;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mLookAtPosition = 0x18;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace CameraAdjustedPositionComponent

// CameraAimAssistComponent  [src/mc/entity/components/camera/aimassist/CameraAimAssistComponent.h]  расчётный размер 0x41
namespace CameraAimAssistComponent {
    constexpr ptrdiff_t mViewFrustumAngle = 0x0;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mViewFrustumDistance = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mTargetMode = 0xC;  // ::CameraAimAssist::TargetMode, 1 байт
    constexpr ptrdiff_t mPriorityPresetId = 0x10;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mShowDebugRender = 0x40;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x41;
}  // namespace CameraAimAssistComponent

// CameraAimAssistRegistryComponent  [src/mc/entity/components/camera/aimassist/CameraAimAssistRegistryComponent.h]  расчётный размер 0x80
namespace CameraAimAssistRegistryComponent {
    constexpr ptrdiff_t mPriorityPresets = 0x0;  // ::std::unordered_map<::HashedString, ::CameraAimAssist::PriorityPreset>, 64 байт
    constexpr ptrdiff_t mPriorityCategoryList = 0x40;  // ::std::unordered_map<::HashedString, ::CameraAimAssist::PriorityCategory>, 64 байт
    constexpr ptrdiff_t Size = 0x80;
}  // namespace CameraAimAssistRegistryComponent

// CameraAttachComponent  [src/mc/deps/minecraft_camera/components/CameraAttachComponent.h]  расчётный размер 0x48
namespace CameraAttachComponent {
    constexpr ptrdiff_t mAttachTo = 0x0;  // ::WeakRef<::EntityContext>, 24 байт
    constexpr ptrdiff_t mLocator = 0x18;  // ::HashedString, 48 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace CameraAttachComponent

// CameraAvoidanceComponent  [src/mc/deps/minecraft_camera/components/CameraAvoidanceComponent.h]  расчётный размер 0x40
namespace CameraAvoidanceComponent {
    constexpr ptrdiff_t mAvoidanceRaysHorizontal = 0x0;  // ::std::vector<::SharedTypes::v1_21_100::CameraAvoidanceRay>, 24 байт
    constexpr ptrdiff_t mAvoidanceRaysVertical = 0x18;  // ::std::vector<::SharedTypes::v1_21_100::CameraAvoidanceRay>, 24 байт
    constexpr ptrdiff_t mRelaxDistanceSmoothingStiffness = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mDistanceConstraintMin = 0x34;  // float, 4 байт
    constexpr ptrdiff_t mRelaxDistanceVelocity = 0x38;  // float, 4 байт
    constexpr ptrdiff_t mSmoothedDistanceConstraint = 0x3C;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace CameraAvoidanceComponent

// CameraBlendStateComponent  [src/mc/deps/minecraft_camera/components/CameraBlendStateComponent.h]  расчётный размер 0xA4
namespace CameraBlendStateComponent {
    constexpr ptrdiff_t mAmount = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mFieldOfView = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mEulerAngles = 0x8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPosition = 0x14;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mTargetPos = 0x20;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mLastEaseState = 0x2C;  // ::std::optional<::MinecraftCamera::CameraBlendStateComponent::EaseState>, 36 байт
    constexpr ptrdiff_t mActiveCamera = 0x50;  // ::WeakRef<::EntityId>, 24 байт
    constexpr ptrdiff_t mPercentage = 0x68;  // float, 4 байт
    constexpr ptrdiff_t mEaseFunction = 0x70;  // float (*)(float, float, float), 8 байт
    constexpr ptrdiff_t mBlendTime = 0x78;  // float, 4 байт
    constexpr ptrdiff_t mBlendStartFieldOfView = 0x7C;  // float, 4 байт
    constexpr ptrdiff_t mBlendStartPosition = 0x80;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBlendStartEulerAngles = 0x8C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mSnapShotEaseValues = 0x98;  // ::MinecraftCamera::CameraBlendStateComponent::SnapShotEaseValues, 12 байт
    constexpr ptrdiff_t Size = 0xA4;
}  // namespace CameraBlendStateComponent

// CameraClientInstanceComponent  [src-client/mc/deps/minecraft_camera/CameraClientInstanceComponent.h]  расчётный размер 0x8
namespace CameraClientInstanceComponent {
    constexpr ptrdiff_t mClientInstance = 0x0;  // ::std::unique_ptr<::ICameraClientInstance>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CameraClientInstanceComponent

// CameraComfortMoveVRComponent  [src-client/mc/deps/vanilla_camera/components/CameraComfortMoveVRComponent.h]  расчётный размер 0x11
namespace CameraComfortMoveVRComponent {
    constexpr ptrdiff_t mLinearYRemap = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mMaxLinearYRemap = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mStartYPos = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mAccumulatedDT = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mYMotionUp = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x11;
}  // namespace CameraComfortMoveVRComponent

// CameraComponent  [src/mc/deps/minecraft_camera/components/CameraComponent.h]  расчётный размер 0x11C
namespace CameraComponent {
    constexpr ptrdiff_t mId = 0x0;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mOrientation = 0x30;  // ::glm::qua<float>, 16 байт
    constexpr ptrdiff_t mPosition = 0x40;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mAspectRatio = 0x4C;  // float, 4 байт
    constexpr ptrdiff_t mFieldOfView = 0x50;  // float, 4 байт
    constexpr ptrdiff_t mNearPlane = 0x54;  // float, 4 байт
    constexpr ptrdiff_t mFarPlane = 0x58;  // float, 4 байт
    constexpr ptrdiff_t mPostViewTransform = 0x5C;  // ::Matrix, 64 байт
    constexpr ptrdiff_t mSavedProjection = 0x9C;  // ::Matrix, 64 байт
    constexpr ptrdiff_t mSavedModelView = 0xDC;  // ::Matrix, 64 байт
    constexpr ptrdiff_t Size = 0x11C;
}  // namespace CameraComponent

// CameraDirectLookComponent  [src-client/mc/deps/minecraft_camera/components/CameraDirectLookComponent.h]  расчётный размер 0x14
namespace CameraDirectLookComponent {
    constexpr ptrdiff_t mYaw = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mPitch = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mYawDelta = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mPitchMin = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mPitchMax = 0x10;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace CameraDirectLookComponent

// CameraEntityStateComponent  [src/mc/deps/minecraft_camera/components/CameraEntityStateComponent.h]  расчётный размер 0x2
namespace CameraEntityStateComponent {
    constexpr ptrdiff_t mIsInWall = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mIsAlive = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace CameraEntityStateComponent

// CameraFadeEffectComponent  [src-client/mc/deps/minecraft_camera/components/CameraFadeEffectComponent.h]  расчётный размер 0x31
namespace CameraFadeEffectComponent {
    constexpr ptrdiff_t mColor = 0x0;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mFadeAnimation = 0x10;  // ::CameraFadeAnimation, 32 байт
    constexpr ptrdiff_t mPlayerWasSleeping = 0x30;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x31;
}  // namespace CameraFadeEffectComponent

// CameraFlyMoveComponent  [src-client/mc/deps/minecraft_camera/components/CameraFlyMoveComponent.h]  расчётный размер 0x8
namespace CameraFlyMoveComponent {
    constexpr ptrdiff_t mSpeed = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mSprintSpeed = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CameraFlyMoveComponent

// CameraGlobalInstructionComponent  [src/mc/deps/minecraft_camera/components/CameraGlobalInstructionComponent.h]  расчётный размер 0x18
namespace CameraGlobalInstructionComponent {
    constexpr ptrdiff_t mPresetIndex = 0x0;  // ::std::optional<uint>, 8 байт
    constexpr ptrdiff_t mDelayPresetIndex = 0x8;  // ::std::optional<uint>, 8 байт
    constexpr ptrdiff_t mEasingType = 0x10;  // ::EasingType, 4 байт
    constexpr ptrdiff_t mEasingTime = 0x14;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace CameraGlobalInstructionComponent

// CameraInstruction  [src/mc/deps/minecraft_camera/CameraInstruction.h]  расчётный размер 0x14E
namespace CameraInstruction {
    constexpr ptrdiff_t mAttachToEntity = 0x0;  // ::std::optional<::CameraInstructionOptions::AttachToEntityInstruction>, 24 байт
    constexpr ptrdiff_t mFade = 0x18;  // ::std::optional<::CameraInstructionOptions::FadeInstruction>, 36 байт
    constexpr ptrdiff_t mFieldOfView = 0x3C;  // ::std::optional<::CameraInstructionOptions::FovInstruction>, 20 байт
    constexpr ptrdiff_t mSet = 0x50;  // ::std::optional<::CameraInstructionOptions::SetInstruction>, 96 байт
    constexpr ptrdiff_t mSpline = 0xB0;  // ::std::optional<::CameraInstructionOptions::SplineInstruction>, 120 байт
    constexpr ptrdiff_t mTarget = 0x128;  // ::std::optional<::CameraInstructionOptions::TargetInstruction>, 32 байт
    constexpr ptrdiff_t mClear = 0x148;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mDetachFromEntity = 0x14A;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mRemoveTarget = 0x14C;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t Size = 0x14E;
}  // namespace CameraInstruction

// CameraInstructionPacket  [src/mc/network/packet/CameraInstructionPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace CameraInstructionPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CameraInstructionPacket

// CameraInstructionsComponent  [src-client/mc/deps/minecraft_camera/components/CameraInstructionsComponent.h]  расчётный размер 0x18
namespace CameraInstructionsComponent {
    constexpr ptrdiff_t mInstructions = 0x0;  // ::std::vector<::CameraInstruction>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace CameraInstructionsComponent

// CameraLiquidOffsetComponent  [src-client/mc/deps/minecraft_camera/components/CameraLiquidOffsetComponent.h]  расчётный размер 0x4
namespace CameraLiquidOffsetComponent {
    constexpr ptrdiff_t mCurrentOffset = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CameraLiquidOffsetComponent

// CameraLookAtComponent  [src-client/mc/deps/minecraft_camera/components/CameraLookAtComponent.h]  расчётный размер 0x18
namespace CameraLookAtComponent {
    constexpr ptrdiff_t mLookAt = 0x0;  // ::WeakRef<::EntityContext>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace CameraLookAtComponent

// CameraLookAtPositionComponent  [src/mc/deps/minecraft_camera/components/CameraLookAtPositionComponent.h]  расчётный размер 0x14
namespace CameraLookAtPositionComponent {
    constexpr ptrdiff_t mTargetPosition = 0x0;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mTargetRotation = 0xC;  // ::glm::vec2, 8 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace CameraLookAtPositionComponent

// CameraOffsetComponent  [src/mc/deps/minecraft_camera/components/CameraOffsetComponent.h]  расчётный размер 0x4C
namespace CameraOffsetComponent {
    constexpr ptrdiff_t mEntityOffset = 0x0;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mViewOffset = 0xC;  // ::glm::vec2, 8 байт
    constexpr ptrdiff_t mCalculatedOffset = 0x14;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mPivot = 0x20;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mCaclulatedViewOffset = 0x2C;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mOriginalEntityOffset = 0x38;  // ::glm::vec3, 12 байт
    constexpr ptrdiff_t mOriginalViewOffset = 0x44;  // ::glm::vec2, 8 байт
    constexpr ptrdiff_t Size = 0x4C;
}  // namespace CameraOffsetComponent

// CameraOrbitComponent  [src/mc/deps/minecraft_camera/components/CameraOrbitComponent.h]  расчётный размер 0x4C
namespace CameraOrbitComponent {
    constexpr ptrdiff_t mAzimuthSmoothingStiffness = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mPolarAngleSmoothingStiffness = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mDistanceSmoothingStiffness = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mPolarAngleMin = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mPolarAngleMax = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mYawAngleMin = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mYawAngleMax = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mRadius = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mInvertXInput = 0x20;  // bool, 1 байт
    constexpr ptrdiff_t mInvertYInput = 0x21;  // bool, 1 байт
    constexpr ptrdiff_t mCurrentSpherical = 0x24;  // ::Spherical, 12 байт
    constexpr ptrdiff_t mIdealSpherical = 0x30;  // ::Spherical, 12 байт
    constexpr ptrdiff_t mAzimuthVelocity = 0x3C;  // float, 4 байт
    constexpr ptrdiff_t mPolarAngleVelocity = 0x40;  // float, 4 байт
    constexpr ptrdiff_t mDistanceVelocity = 0x44;  // float, 4 байт
    constexpr ptrdiff_t mDistanceConstraint = 0x48;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4C;
}  // namespace CameraOrbitComponent

// CameraPerspectiveOptionComponent  [src/mc/deps/minecraft_camera/components/CameraPerspectiveOptionComponent.h]  расчётный размер 0x4
namespace CameraPerspectiveOptionComponent {
    constexpr ptrdiff_t mActivateForViewMode = 0x0;  // ::SharedTypes::v1_21_100::PlayerViewMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CameraPerspectiveOptionComponent

// CameraPresetComponent  [src/mc/deps/minecraft_camera/components/CameraPresetComponent.h]  расчётный размер 0x8
namespace CameraPresetComponent {
    constexpr ptrdiff_t mPresetIndex = 0x0;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CameraPresetComponent

// CameraShakeComponent  [src/mc/entity/components/CameraShakeComponent.h]  расчётный размер 0x69
namespace CameraShakeComponent {
    constexpr ptrdiff_t xAxisNoise = 0x0;  // ::std::unique_ptr<::SimplexNoise>, 8 байт
    constexpr ptrdiff_t yAxisNoise = 0x8;  // ::std::unique_ptr<::SimplexNoise>, 8 байт
    constexpr ptrdiff_t zAxisNoise = 0x10;  // ::std::unique_ptr<::SimplexNoise>, 8 байт
    constexpr ptrdiff_t mLastUpdatedTime = 0x18;  // double, 8 байт
    constexpr ptrdiff_t mNoiseTime = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mDecayRate = 0x24;  // float, 4 байт
    constexpr ptrdiff_t mShakeTypeQueues = 0x28;  // ::std::array<::CameraShakeTypeQueue, 2>, 64 байт
    constexpr ptrdiff_t mWasShaking = 0x68;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x69;
}  // namespace CameraShakeComponent

// CameraShakeSupportComponent  [src-client/mc/deps/minecraft_camera/components/CameraShakeSupportComponent.h]  расчётный размер 0xC
namespace CameraShakeSupportComponent {
    constexpr ptrdiff_t mFrequency = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mAmplitude = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mNoiseMultiplier = 0x8;  // float, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace CameraShakeSupportComponent

// CameraTargetComponent  [src-client/mc/deps/minecraft_camera/components/CameraTargetComponent.h]  расчётный размер 0x88
namespace CameraTargetComponent {
    constexpr ptrdiff_t mIsOutOfRange = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mSnappedToTarget = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mFoundActor = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mTargetActorId = 0x8;  // int64, 8 байт
    constexpr ptrdiff_t mCurrentRotation = 0x10;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mInitialRotation = 0x18;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mVerticalRotationLimits = 0x20;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mOffset2DRotation = 0x28;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mAverageRotLimitOrientation = 0x30;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCurrentCameraOrientation = 0x3C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mInitialCameraOrientation = 0x48;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mInitialTargetCenterOffset = 0x54;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mTargetCenterOffset = 0x60;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mTargetEntity = 0x70;  // ::WeakRef<::EntityContext>, 24 байт
    constexpr ptrdiff_t Size = 0x88;
}  // namespace CameraTargetComponent

// CameraTargetSettingsComponent  [src-client/mc/deps/minecraft_camera/components/CameraTargetSettingsComponent.h]  расчётный размер 0x38
namespace CameraTargetSettingsComponent {
    constexpr ptrdiff_t mSnapToTarget = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mContinueTargeting = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mRotationSpeed = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mTrackingRadius = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mHorizontalRotationLimit = 0xC;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t mVerticalRotationLimit = 0x18;  // ::std::optional<::Vec2>, 12 байт
    constexpr ptrdiff_t mCurrentEulerOrientation = 0x24;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mTargetDirectionToLook = 0x2C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace CameraTargetSettingsComponent

// CameraThirdPersonBoomComponent  [src/mc/deps/minecraft_camera/components/CameraThirdPersonBoomComponent.h]  расчётный размер 0xD
namespace CameraThirdPersonBoomComponent {
    constexpr ptrdiff_t mStartingRadius = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mStartingOrientation = 0x4;  // ::glm::vec2, 8 байт
    constexpr ptrdiff_t mRotSpace = 0xC;  // ::SharedTypes::v1_21_100::RotationSpace, 1 байт
    constexpr ptrdiff_t Size = 0xD;
}  // namespace CameraThirdPersonBoomComponent

// CameraTimeComponent  [src-client/mc/deps/minecraft_camera/components/CameraTimeComponent.h]  расчётный размер 0xC
namespace CameraTimeComponent {
    constexpr ptrdiff_t mElapsedTime = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mUpdatedAt = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mDeltaTime = 0x8;  // float, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace CameraTimeComponent

// CameraUsageComponent  [src-client/mc/deps/minecraft_camera/components/CameraUsageComponent.h]  расчётный размер 0x18
namespace CameraUsageComponent {
    constexpr ptrdiff_t mActiveCamera = 0x0;  // ::WeakRef<::EntityId const>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace CameraUsageComponent

// CameraVehicleRotationComponent  [src-client/mc/deps/vanilla_camera/components/CameraVehicleRotationComponent.h]  расчётный размер 0x4
namespace CameraVehicleRotationComponent {
    constexpr ptrdiff_t mLastVehicleRotationRadians = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CameraVehicleRotationComponent

// CanDestroyInCreativeItemComponent  [src/mc/deps/shared_types/v1_20_50/item/CanDestroyInCreativeItemComponent.h]  расчётный размер 0x1
namespace CanDestroyInCreativeItemComponent {
    constexpr ptrdiff_t mCanDestroyInCreative = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace CanDestroyInCreativeItemComponent

// CelebrateHuntComponent  [src/mc/entity/components_json_legacy/CelebrateHuntComponent.h]  расчётный размер 0x18
namespace CelebrateHuntComponent {
    constexpr ptrdiff_t mCelebrating = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mCelebrateUntil = 0x8;  // ::Tick, 8 байт
    constexpr ptrdiff_t mNextSoundEventTick = 0x10;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace CelebrateHuntComponent

// ChunkPositionComponent  [src/mc/entity/components/ChunkPositionComponent.h]  расчётный размер 0x8
namespace ChunkPositionComponent {
    constexpr ptrdiff_t mChunkPosition = 0x0;  // ::ChunkPos, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ChunkPositionComponent

// ChunkSource  [src/mc/world/level/chunk/ChunkSource.h]  расчётный размер 0x58  ⚠ BASE_UNKNOWN
namespace ChunkSource {
    constexpr ptrdiff_t mChunkSide = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mLevel = 0x8;  // ::Level*, 8 байт
    constexpr ptrdiff_t mDimension = 0x10;  // ::Dimension*, 8 байт
    constexpr ptrdiff_t mParent = 0x18;  // ::ChunkSource*, 8 байт
    constexpr ptrdiff_t mOwnedParent = 0x20;  // ::std::unique_ptr<::ChunkSource>, 8 байт
    constexpr ptrdiff_t mLevelChunkBuilderData = 0x28;  // ::LevelChunkBuilderData*, 8 байт
    constexpr ptrdiff_t mShuttingDown = 0x30;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mOnSaveSubscription = 0x38;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnLevelStorageAppSuspendSubscription = 0x48;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace ChunkSource

// ClientAcceptanceThresholdsComponent  [src/mc/entity/components/ClientAcceptanceThresholdsComponent.h]  расчётный размер 0x36
namespace ClientAcceptanceThresholdsComponent {
    constexpr ptrdiff_t mStrictThreshold = 0x0;  // ::ClientAcceptanceConfig, 24 байт
    constexpr ptrdiff_t mPermissiveThreshold = 0x18;  // ::ClientAcceptanceConfig, 24 байт
    constexpr ptrdiff_t mCurrentCooldown = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mCurrentServerInitiatedMotionCooldown = 0x34;  // uchar, 1 байт
    constexpr ptrdiff_t mMaxServerInitiatedMotionCooldown = 0x35;  // uchar, 1 байт
    constexpr ptrdiff_t Size = 0x36;
}  // namespace ClientAcceptanceThresholdsComponent

// ClientInputHandler  [src-client/mc/client/input/ClientInputHandler.h]  расчётный размер 0x88
namespace ClientInputHandler {
    constexpr ptrdiff_t mClient = 0x0;  // ::Bedrock::NotNullNonOwnerPtr<::IClientInstance>, 24 байт
    constexpr ptrdiff_t mInputHandler = 0x18;  // ::InputHandler&, 8 байт
    constexpr ptrdiff_t mDebugSplitscreenInputRedirect = 0x20;  // ::Bedrock::NotNullNonOwnerPtr<::Input::Debug::ISplitscreenRedirect>, 24 байт
    constexpr ptrdiff_t mInputBindingMode = 0x38;  // ::InputBindingMode, 4 байт
    constexpr ptrdiff_t mBindingFactory = 0x40;  // ::std::unique_ptr<::ClientBindingFactory>, 8 байт
    constexpr ptrdiff_t mMappingFactory = 0x48;  // ::std::unique_ptr<::ClientInputMappingFactory>, 8 байт
    constexpr ptrdiff_t mExpectedInGameInputMode = 0x50;  // ::std::string, 32 байт
    constexpr ptrdiff_t mIsFlying = 0x70;  // bool, 1 байт
    constexpr ptrdiff_t mIsSneaking = 0x71;  // bool, 1 байт
    constexpr ptrdiff_t mIsSprinting = 0x72;  // bool, 1 байт
    constexpr ptrdiff_t mIsSwimming = 0x73;  // bool, 1 байт
    constexpr ptrdiff_t mIsInWater = 0x74;  // bool, 1 байт
    constexpr ptrdiff_t mIsExpediateEmoteActive = 0x75;  // bool, 1 байт
    constexpr ptrdiff_t mIsInAscendableBlock = 0x76;  // bool, 1 байт
    constexpr ptrdiff_t mIsOnDescendableBlock = 0x77;  // bool, 1 байт
    constexpr ptrdiff_t mIsCreativeMode = 0x78;  // bool, 1 байт
    constexpr ptrdiff_t mIsSpectatorMode = 0x79;  // bool, 1 байт
    constexpr ptrdiff_t mInteractActive = 0x7A;  // bool, 1 байт
    constexpr ptrdiff_t mHasMobEffects = 0x7B;  // bool, 1 байт
    constexpr ptrdiff_t mProxy = 0x80;  // ::std::unique_ptr<::ClientInputHandlerProxy>, 8 байт
    constexpr ptrdiff_t Size = 0x88;
}  // namespace ClientInputHandler

// ClientInputLockComponent  [src/mc/entity/components/ClientInputLockComponent.h]  расчётный размер 0x4
namespace ClientInputLockComponent {
    constexpr ptrdiff_t mActiveCategories = 0x0;  // ::Bedrock::EnumSet<::ClientInputLockCategory, 13>, 2 байт
    constexpr ptrdiff_t mClientInputLocks = 0x2;  // ::Bedrock::EnumSet<::ClientInputLockType, 11>, 2 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ClientInputLockComponent

// ClientInputMappingFactory  [src-client/mc/client/input/ClientInputMappingFactory.h]  расчётный размер 0x118  ⚠ BASE_UNKNOWN
namespace ClientInputMappingFactory {
    constexpr ptrdiff_t mActiveInputMappings = 0x0;  // ::std::unordered_map<::std::string, ::InputMapping>, 64 байт
    constexpr ptrdiff_t mInputMappingTemplates = 0x40;  // ::std::unordered_map<::std::string, ::InputMapping>, 64 байт
    constexpr ptrdiff_t mInvertYAxis = 0x80;  // bool, 1 байт
    constexpr ptrdiff_t mSwapGamepadButtonsXY = 0x81;  // bool, 1 байт
    constexpr ptrdiff_t mSwapGamepadButtonsAB = 0x82;  // bool, 1 байт
    constexpr ptrdiff_t mSensitivity = 0x84;  // float, 4 байт
    constexpr ptrdiff_t mGameControllerRemappingLayout = 0x88;  // ::GamePadRemappingLayout, 128 байт
    constexpr ptrdiff_t mKeyboardRemappingLayout = 0x108;  // ::std::weak_ptr<::KeyboardRemappingLayout>, 16 байт
    constexpr ptrdiff_t Size = 0x118;
}  // namespace ClientInputMappingFactory

// ClientInstance  [src-client/mc/client/game/ClientInstance.h]  расчётный размер 0x28
namespace ClientInstance {
    constexpr ptrdiff_t mUITexture = 0x0;  // ::mce::Texture*, 8 байт
    constexpr ptrdiff_t mUICursorTexture = 0x8;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ClientInstance

// ClientParticleInitializationComponent  [src/mc/entity/components/ClientParticleInitializationComponent.h]  расчётный размер 0x18
namespace ClientParticleInitializationComponent {
    constexpr ptrdiff_t mParticleInitializationData = 0x0;  // ::std::vector<::ParticleInitializationData>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ClientParticleInitializationComponent

// ClientParticleTerminationComponent  [src/mc/entity/components/ClientParticleTerminationComponent.h]  расчётный размер 0x28
namespace ClientParticleTerminationComponent {
    constexpr ptrdiff_t mAnimationControllerNames = 0x0;  // ::std::queue<::HashedString, ::std::deque<::HashedString>>, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ClientParticleTerminationComponent

// ClientPushDimensionLoadingScreenComponent  [src/mc/entity/components/ClientPushDimensionLoadingScreenComponent.h]  расчётный размер 0x4
namespace ClientPushDimensionLoadingScreenComponent {
    constexpr ptrdiff_t mToDimensionType = 0x0;  // ::DimensionType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ClientPushDimensionLoadingScreenComponent

// ClientVibrationComponent  [src/mc/entity/components/ClientVibrationComponent.h]  расчётный размер 0x10
namespace ClientVibrationComponent {
    constexpr ptrdiff_t mLastVibrationTick = 0x0;  // ::std::optional<::Tick>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace ClientVibrationComponent

// ClimateBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/ClimateBiomeJsonComponent.h]  расчётный размер 0x1C  ⚠ BASE_UNKNOWN
namespace ClimateBiomeJsonComponent {
    constexpr ptrdiff_t mDownfall = 0x0;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mSnowAccumulation = 0x8;  // ::std::optional<::std::array<float, 2>>, 12 байт
    constexpr ptrdiff_t mTemperature = 0x14;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace ClimateBiomeJsonComponent

// CodebuilderComponent  [src/mc/entity/components/CodebuilderComponent.h]  расчётный размер 0x1
namespace CodebuilderComponent {
    constexpr ptrdiff_t mCodeStatus = 0x0;  // ::CodeBuilderExecutionState::CodeStatus, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace CodebuilderComponent

// CollisionBoxComponent  [src/mc/entity/components_json_legacy/CollisionBoxComponent.h]  расчётный размер 0x8
namespace CollisionBoxComponent {
    constexpr ptrdiff_t mBBDim = 0x0;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CollisionBoxComponent

// CombatRegenerationComponent  [src/mc/entity/components_json_legacy/CombatRegenerationComponent.h]  расчётный размер 0x10
namespace CombatRegenerationComponent {
    constexpr ptrdiff_t mTargetID = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mCombatRegenerationListener = 0x8;  // ::std::unique_ptr<::CombatRegenerationListener>, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace CombatRegenerationComponent

// CommandBlockComponent  [src/mc/entity/components/CommandBlockComponent.h]  расчётный размер 0xC5
namespace CommandBlockComponent {
    constexpr ptrdiff_t mBaseCommandBlock = 0x0;  // ::BaseCommandBlock, 192 байт
    constexpr ptrdiff_t mCurrentTickCount = 0xC0;  // int, 4 байт
    constexpr ptrdiff_t mTicking = 0xC4;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xC5;
}  // namespace CommandBlockComponent

// CommandOriginData  [src/mc/server/commands/CommandOriginData.h]  расчётный размер 0x40
namespace CommandOriginData {
    constexpr ptrdiff_t mType = 0x0;  // ::CommandOriginType, 1 байт
    constexpr ptrdiff_t mUUID = 0x8;  // ::mce::UUID, 16 байт
    constexpr ptrdiff_t mRequestId = 0x18;  // ::std::string, 32 байт
    constexpr ptrdiff_t mPlayerId = 0x38;  // int64, 8 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace CommandOriginData

// CommandRequestPacket  [src/mc/network/packet/CommandRequestPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace CommandRequestPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace CommandRequestPacket

// ComplexInventoryTransaction  [src/mc/world/inventory/transaction/ComplexInventoryTransaction.h]  расчётный размер 0x60
namespace ComplexInventoryTransaction {
    constexpr ptrdiff_t mType = 0x0;  // ::ComplexInventoryTransaction::Type, 4 байт
    constexpr ptrdiff_t mTransaction = 0x8;  // ::InventoryTransaction, 88 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace ComplexInventoryTransaction

// ConditionalBandwidthOptimizationComponent  [src/mc/entity/components_json_legacy/ConditionalBandwidthOptimizationComponent.h]  расчётный размер 0xD0
namespace ConditionalBandwidthOptimizationComponent {
    constexpr ptrdiff_t mDefaultValues = 0x0;  // ::ConditionalBandwidthOptimization, 88 байт
    constexpr ptrdiff_t mConditionalValues = 0x58;  // ::std::vector<::ConditionalBandwidthOptimization>, 24 байт
    constexpr ptrdiff_t mLastRequestedValuesTick = 0x70;  // ::Tick, 8 байт
    constexpr ptrdiff_t mCachedOptimizationValues = 0x78;  // ::ConditionalBandwidthOptimization, 88 байт
    constexpr ptrdiff_t Size = 0xD0;
}  // namespace ConditionalBandwidthOptimizationComponent

// ContainerClosePacket  [src/mc/network/packet/ContainerClosePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ContainerClosePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ContainerClosePacket

// ContainerComponent  [src/mc/entity/components_json_legacy/ContainerComponent.h]  расчётный размер 0x1EC  ⚠ BASE_UNKNOWN
namespace ContainerComponent {
    constexpr ptrdiff_t mContainer = 0x0;  // ::FillingContainer, 440 байт
    constexpr ptrdiff_t mListenerShim = 0x1B8;  // ::Actor*, 8 байт
    constexpr ptrdiff_t mCanBeSiphonedFrom = 0x1C0;  // bool, 1 байт
    constexpr ptrdiff_t mPrivate = 0x1C1;  // bool, 1 байт
    constexpr ptrdiff_t mRestrictToOwner = 0x1C2;  // bool, 1 байт
    constexpr ptrdiff_t mLootTable = 0x1C8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLootTableSeed = 0x1E8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x1EC;
}  // namespace ContainerComponent

// ContainerManagerModel  [src/mc/world/containers/managers/models/ContainerManagerModel.h]  расчётный размер 0x178  ⚠ BASE_UNKNOWN
namespace ContainerManagerModel {
    constexpr ptrdiff_t mContainerRefresher = 0x0;  // ::gsl::not_null<::std::unique_ptr<::PlayerContainerRefresher>>, 8 байт
    constexpr ptrdiff_t mDynamicContainerTracker = 0x8;  // ::gsl::not_null<::std::shared_ptr<::DynamicContainerTracker>>, 16 байт
    constexpr ptrdiff_t mRegistryAccess = 0x18;  // ::WeakRef<::IContainerRegistryAccess>, 16 байт
    constexpr ptrdiff_t mPlayer = 0x28;  // ::Player&, 8 байт
    constexpr ptrdiff_t mLastSlots = 0x30;  // ::std::vector<::ItemStack>, 24 байт
    constexpr ptrdiff_t mContainerId = 0x48;  // ::ContainerID, 1 байт
    constexpr ptrdiff_t mContainerType = 0x49;  // ::SharedTypes::Legacy::ContainerType, 1 байт
    constexpr ptrdiff_t mInformControllerOfDestructionCallbacks = 0x50;  // ::std::unordered_map<::ContainerManagerController const*, ::std::function<void(::ContainerManagerModel&)>>, 64 байт
    constexpr ptrdiff_t mScreenContext = 0x90;  // ::ContainerScreenContext, 40 байт
    constexpr ptrdiff_t mContainers = 0xB8;  // ::std::unordered_map<::std::string, ::std::shared_ptr<::ContainerModel>>, 64 байт
    constexpr ptrdiff_t mOnDynamicContainerChangePublisher = 0xF8;  // ::Bedrock::PubSub::Publisher<void(), ::Bedrock::PubSub::ThreadModel::MultiThreaded, 0>, 128 байт
    constexpr ptrdiff_t Size = 0x178;
}  // namespace ContainerManagerModel

// ContainerOpenPacket  [src/mc/network/packet/ContainerOpenPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ContainerOpenPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ContainerOpenPacket

// ContainerScreenContextComponent  [src/mc/world/inventory/network/ContainerScreenContextComponent.h]  расчётный размер 0x28
namespace ContainerScreenContextComponent {
    constexpr ptrdiff_t mContext = 0x0;  // ::ContainerScreenContext, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ContainerScreenContextComponent

// ContainerScreenController  [src-client/mc/client/gui/screens/controllers/ContainerScreenController.h]  расчётный размер 0x570  ⚠ BASE_UNKNOWN
namespace ContainerScreenController {
    constexpr ptrdiff_t mBlockPos = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mEntityUniqueID = 0x10;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mTypeInContainer = 0x18;  // ::TypeInContainer, 4 байт
    constexpr ptrdiff_t mShowItemCategory = 0x1C;  // bool, 1 байт
    constexpr ptrdiff_t mContainerStateMachine = 0x20;  // ::ContainerInteractionStateMachine, 336 байт
    constexpr ptrdiff_t mLastStateSlot = 0x170;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mLastPlacedAmount = 0x198;  // int, 4 байт
    constexpr ptrdiff_t mSingleSplit = 0x19C;  // bool, 1 байт
    constexpr ptrdiff_t mTouchSplitData = 0x1A0;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mSelectedSplitTarget = 0x1C8;  // ::ItemGroup, 136 байт
    constexpr ptrdiff_t mSplitDraggingToPlace = 0x250;  // bool, 1 байт
    constexpr ptrdiff_t mPreviousGestureControlEnabled = 0x251;  // bool, 1 байт
    constexpr ptrdiff_t mEnterKeyPressedForSplitting = 0x252;  // bool, 1 байт
    constexpr ptrdiff_t mArrowKeyPressed = 0x253;  // bool, 1 байт
    constexpr ptrdiff_t mProgressiveTakeButton = 0x258;  // ::ProgressiveTakeButtonData, 96 байт
    constexpr ptrdiff_t mTouchProgressiveSelectButton = 0x2B8;  // uint, 4 байт
    constexpr ptrdiff_t mPreviousState = 0x2C0;  // ::std::vector<::ItemStateData>, 24 байт
    constexpr ptrdiff_t mFadeInIconData = 0x2D8;  // ::std::map<::std::pair<::std::string, int>, ::std::pair<double, int>>, 16 байт
    constexpr ptrdiff_t mSelectedSlotData = 0x2E8;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mLastSelectedSlotData = 0x310;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mHoveredSlotData = 0x338;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mShowHoverText = 0x360;  // bool, 1 байт
    constexpr ptrdiff_t mHoverTextSuppressedUntilPointerMoves = 0x361;  // bool, 1 байт
    constexpr ptrdiff_t mHoverTextInitialPointerX = 0x364;  // int, 4 байт
    constexpr ptrdiff_t mHoverTextInitialPointerY = 0x368;  // int, 4 байт
    constexpr ptrdiff_t mHoverTextSkipNextGamepadHover = 0x36C;  // bool, 1 байт
    constexpr ptrdiff_t mLastHoverTime = 0x370;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mStartDraggingSlotData = 0x378;  // ::SlotData, 40 байт
    constexpr ptrdiff_t mPointerHasMoved = 0x3A0;  // bool, 1 байт
    constexpr ptrdiff_t mCoalesceOrderMap = 0x3A8;  // ::std::unordered_map<::std::string, ::std::vector<::std::string>>, 64 байт
    constexpr ptrdiff_t mAutoPlaceOrderMap = 0x3E8;  // ::std::unordered_map<::std::string, ::std::vector<::AutoPlaceItem>>, 64 байт
    constexpr ptrdiff_t mIsHoldingScrolling = 0x428;  // bool, 1 байт
    constexpr ptrdiff_t mReadyToVibrate = 0x429;  // bool, 1 байт
    constexpr ptrdiff_t mNeedsUpdatedBinds = 0x42A;  // bool, 1 байт
    constexpr ptrdiff_t mStartHeldTime = 0x42C;  // int, 4 байт
    constexpr ptrdiff_t mStartDeciding = 0x430;  // bool, 1 байт
    constexpr ptrdiff_t mLastPointerX = 0x432;  // short, 2 байт
    constexpr ptrdiff_t mLastPointerY = 0x434;  // short, 2 байт
    constexpr ptrdiff_t mStartDraggingPointerX = 0x436;  // short, 2 байт
    constexpr ptrdiff_t mStartDraggingPointerY = 0x438;  // short, 2 байт
    constexpr ptrdiff_t mDecidingSelectionScroll = 0x43A;  // bool, 1 байт
    constexpr ptrdiff_t mSelectionScrollStartX = 0x43C;  // short, 2 байт
    constexpr ptrdiff_t mSelectionScrollStartY = 0x43E;  // short, 2 байт
    constexpr ptrdiff_t mDraggingCollectionName = 0x440;  // ::std::string, 32 байт
    constexpr ptrdiff_t mDraggingCollectionIndex = 0x460;  // int, 4 байт
    constexpr ptrdiff_t mInteractingCollectionName = 0x468;  // ::std::string, 32 байт
    constexpr ptrdiff_t mInteractingCollectionIndex = 0x488;  // int, 4 байт
    constexpr ptrdiff_t mInteractingButtonId = 0x48C;  // uint, 4 байт
    constexpr ptrdiff_t mIsDraggingTooFast = 0x490;  // bool, 1 байт
    constexpr ptrdiff_t mProgressBarShowing = 0x491;  // bool, 1 байт
    constexpr ptrdiff_t mBundleHelper = 0x498;  // ::BundleHelper, 144 байт
    constexpr ptrdiff_t mDirty = 0x528;  // ::ui::DirtyFlag, 4 байт
    constexpr ptrdiff_t mStartOutputSlotDeciding = 0x52C;  // bool, 1 байт
    constexpr ptrdiff_t mOutputCollectionName = 0x530;  // ::std::string, 32 байт
    constexpr ptrdiff_t mOutputCollectionIndex = 0x550;  // int, 4 байт
    constexpr ptrdiff_t mLastReleaseOutputSlotTime = 0x554;  // int, 4 байт
    constexpr ptrdiff_t mScreenCanBeClosedByServer = 0x558;  // bool, 1 байт
    constexpr ptrdiff_t mDefaultItemLockAction = 0x559;  // ::ItemLockAction, 1 байт
    constexpr ptrdiff_t mInteractionModel = 0x55C;  // ::InteractionModel, 4 байт
    constexpr ptrdiff_t mContainerManagerController = 0x560;  // ::std::shared_ptr<::ContainerManagerController>, 16 байт
    constexpr ptrdiff_t Size = 0x570;
}  // namespace ContainerScreenController

// CooldownItemComponent  [src/mc/deps/shared_types/v1_20_50/item/CooldownItemComponent.h]  расчётный размер 0x28
namespace CooldownItemComponent {
    constexpr ptrdiff_t mDuration = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mType = 0x4;  // ::SharedTypes::ItemCooldownType, 1 байт
    constexpr ptrdiff_t mCategory = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace CooldownItemComponent

// CurrentTickComponent  [src/mc/entity/components/CurrentTickComponent.h]  расчётный размер 0x8
namespace CurrentTickComponent {
    constexpr ptrdiff_t mCurrentTick = 0x0;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CurrentTickComponent

// CustomSizeUpdateComponent  [src/mc/deps/vanilla_components/CustomSizeUpdateComponent.h]  расчётный размер 0x8
namespace CustomSizeUpdateComponent {
    constexpr ptrdiff_t mUpdateSize = 0x0;  // void (*)(::AABBShapeComponent const&, ::OffsetsComponent&), 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace CustomSizeUpdateComponent

// DamageItemComponent  [src/mc/deps/shared_types/v1_20_50/item/DamageItemComponent.h]  расчётный размер 0x2
namespace DamageItemComponent {
    constexpr ptrdiff_t mDamage = 0x0;  // short, 2 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace DamageItemComponent

// DamageOverTimeComponent  [src/mc/entity/components_json_legacy/DamageOverTimeComponent.h]  расчётный размер 0xC
namespace DamageOverTimeComponent {
    constexpr ptrdiff_t mHurtValue = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mDamageTimeInterval = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mDamageTime = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace DamageOverTimeComponent

// DamageSensorComponent  [src/mc/entity/components_json_legacy/DamageSensorComponent.h]  расчётный размер 0x28
namespace DamageSensorComponent {
    constexpr ptrdiff_t mDamageAmount = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mDamageIsFatal = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mDamageCause = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mTriggers = 0x10;  // ::std::vector<::DamageSensorTrigger>, 24 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace DamageSensorComponent

// DanceComponent  [src/mc/entity/components_json_legacy/DanceComponent.h]  расчётный размер 0x8
namespace DanceComponent {
    constexpr ptrdiff_t mListener = 0x0;  // ::std::unique_ptr<::DanceComponentListener>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace DanceComponent

// DashCooldownTimerComponent  [src/mc/entity/components/DashCooldownTimerComponent.h]  расчётный размер 0x4
namespace DashCooldownTimerComponent {
    constexpr ptrdiff_t mRemainingTicks = 0x0;  // uint, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace DashCooldownTimerComponent

// DebugCameraComponent  [src-client/mc/deps/minecraft_camera/components/DebugCameraComponent.h]  расчётный размер 0x125
namespace DebugCameraComponent {
    constexpr ptrdiff_t mFrozenCamera = 0x0;  // ::MinecraftCamera::CameraComponent, 288 байт
    constexpr ptrdiff_t mInputMode = 0x120;  // ::MinecraftCamera::DebugCameraComponent::InputMode, 4 байт
    constexpr ptrdiff_t mDebugDrawGameCamera = 0x124;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x125;
}  // namespace DebugCameraComponent

// DebugInfoComponent  [src/mc/entity/components/DebugInfoComponent.h]  расчётный размер 0x90  ⚠ BASE_UNKNOWN
namespace DebugInfoComponent {
    constexpr ptrdiff_t mListeners = 0x0;  // ::std::vector<::DebugInfoComponent::Listener>, 24 байт
    constexpr ptrdiff_t mPacketHashes = 0x18;  // ::std::unordered_map<::HashedString, uint64>, 64 байт
    constexpr ptrdiff_t mSentEvents = 0x58;  // ::std::deque<::std::string>, 40 байт
    constexpr ptrdiff_t mEventListenerRegistered = 0x80;  // bool, 1 байт
    constexpr ptrdiff_t mMobUniqueId = 0x88;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t Size = 0x90;
}  // namespace DebugInfoComponent

// DepenetrationComponent  [src/mc/entity/components/DepenetrationComponent.h]  расчётный размер 0x38
namespace DepenetrationComponent {
    constexpr ptrdiff_t mBits = 0x0;  // ::std::bitset<5>, 4 байт
    constexpr ptrdiff_t mMinDepenetration = 0x4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mOneWayPhysicsBlocks = 0x10;  // ::std::vector<::AABB>, 24 байт
    constexpr ptrdiff_t mTemporaryOverride = 0x28;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace DepenetrationComponent

// DespawnComponent  [src/mc/entity/components_json_legacy/DespawnComponent.h]  расчётный размер 0x8  ⚠ BASE_UNKNOWN
namespace DespawnComponent {
    constexpr ptrdiff_t mActor = 0x0;  // ::Actor&, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace DespawnComponent

// DiggerItemComponent  [src/mc/deps/shared_types/v1_20_50/item/DiggerItemComponent.h]  расчётный размер 0x130
namespace DiggerItemComponent {
    constexpr ptrdiff_t filter = 0x0;  // ::SharedTypes::Legacy::BlockDescriptor, 80 байт
    constexpr ptrdiff_t destroySpeed = 0x50;  // int, 4 байт
    constexpr ptrdiff_t onDigDeprecated = 0x58;  // ::SharedTypes::Legacy::DefinitionTrigger, 88 байт
    constexpr ptrdiff_t mDestroySpeeds = 0xB0;  // ::std::vector<::SharedTypes::v1_20_50::DiggerItemComponent::BlockInfo>, 24 байт
    constexpr ptrdiff_t mUseEfficiency = 0xC8;  // bool, 1 байт
    constexpr ptrdiff_t mOnDigDeprecated = 0xD0;  // ::std::optional<::SharedTypes::Legacy::DefinitionTrigger>, 96 байт
    constexpr ptrdiff_t Size = 0x130;
}  // namespace DiggerItemComponent

// DimensionStateComponent  [src/mc/entity/components/DimensionStateComponent.h]  расчётный размер 0x10
namespace DimensionStateComponent {
    constexpr ptrdiff_t mDimensionState = 0x0;  // ::DimensionStateComponent::DimensionState, 4 байт
    constexpr ptrdiff_t mLoadIntoDimensionTimeout = 0x8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace DimensionStateComponent

// DimensionTransitionComponent  [src/mc/entity/components/DimensionTransitionComponent.h]  расчётный размер 0x24
namespace DimensionTransitionComponent {
    constexpr ptrdiff_t mPortalEntrancePosition = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPortalEntranceDimension = 0xC;  // ::DimensionType, 4 байт
    constexpr ptrdiff_t mPortalEntranceAxis = 0x10;  // ::PortalAxis, 4 байт
    constexpr ptrdiff_t mExisitingTargetPosition = 0x14;  // ::std::optional<::Vec3>, 16 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace DimensionTransitionComponent

// DimensionTypeComponent  [src/mc/entity/components/DimensionTypeComponent.h]  расчётный размер 0x4
namespace DimensionTypeComponent {
    constexpr ptrdiff_t mType = 0x0;  // ::DimensionType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace DimensionTypeComponent

// DisplayNameItemComponent  [src/mc/deps/shared_types/v1_20_50/item/DisplayNameItemComponent.h]  расчётный размер 0x20
namespace DisplayNameItemComponent {
    constexpr ptrdiff_t mValue = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace DisplayNameItemComponent

// DryingOutTimerComponent  [src/mc/entity/components_json_legacy/DryingOutTimerComponent.h]  расчётный размер 0x288
namespace DryingOutTimerComponent {
    constexpr ptrdiff_t mTotalTime = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mWaterBottleAdditionalTime = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mOnFullyDriedOut = 0x8;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mOnStoppedDryingOut = 0x70;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mOnRecoverAfterDriedOut = 0xD8;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mDefinition = 0x140;  // ::DryingOutTimerComponent::DryingOutTimerDefinition, 320 байт
    constexpr ptrdiff_t mTicksRemainingUntilDryOut = 0x280;  // int, 4 байт
    constexpr ptrdiff_t mState = 0x284;  // ::DryingOutTimerComponent::DryingOutState, 4 байт
    constexpr ptrdiff_t Size = 0x288;
}  // namespace DryingOutTimerComponent

// DurabilityItemComponent  [src/mc/deps/shared_types/v1_20_50/item/DurabilityItemComponent.h]  расчётный размер 0xC
namespace DurabilityItemComponent {
    constexpr ptrdiff_t mDamageChance = 0x0;  // ::SharedTypes::IntRange, 8 байт
    constexpr ptrdiff_t mMaxDurability = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace DurabilityItemComponent

// DwellerComponent  [src/mc/entity/components_json_legacy/DwellerComponent.h]  расчётный размер 0x70
namespace DwellerComponent {
    constexpr ptrdiff_t mCanFindPOI = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mCanMigrate = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mHasJoinedDwelling = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mFixUpRole = 0x3;  // bool, 1 байт
    constexpr ptrdiff_t mRewardPlayersOnFirstFounding = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mPreferredProfession = 0x8;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mFirstFoundingReward = 0x38;  // int, 4 байт
    constexpr ptrdiff_t mUpdateIntervalVariant = 0x3C;  // int, 4 байт
    constexpr ptrdiff_t mDwellingUpdateInterval = 0x40;  // uint64, 8 байт
    constexpr ptrdiff_t mUpdateIntervalBase = 0x48;  // uint64, 8 байт
    constexpr ptrdiff_t mDwellingBoundsTolerance = 0x50;  // float, 4 байт
    constexpr ptrdiff_t mType = 0x54;  // ::DwellerComponent::DwellingType, 4 байт
    constexpr ptrdiff_t mRole = 0x58;  // ::DwellerRole, 4 байт
    constexpr ptrdiff_t mDwellingUniqueID = 0x60;  // ::mce::UUID, 16 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace DwellerComponent

// DyeableItemComponent  [src/mc/deps/shared_types/v1_21_30/item/DyeableItemComponent.h]  расчётный размер 0x10
namespace DyeableItemComponent {
    constexpr ptrdiff_t mDefaultColor = 0x0;  // ::SharedTypes::Color255RGB, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace DyeableItemComponent

// DynamicPropertiesComponent  [src/mc/entity/components/DynamicPropertiesComponent.h]  расчётный размер 0x40
namespace DynamicPropertiesComponent {
    constexpr ptrdiff_t mDynamicProperties = 0x0;  // ::DynamicProperties, 64 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace DynamicPropertiesComponent

// DynamicRenderOffsetComponent  [src/mc/entity/components/DynamicRenderOffsetComponent.h]  расчётный размер 0x34
namespace DynamicRenderOffsetComponent {
    constexpr ptrdiff_t mCurrentOffset = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mNormalizedOffset = 0xC;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPreviousOffset = 0x18;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mInterpolationSpeed = 0x24;  // float, 4 байт
    constexpr ptrdiff_t mCurrentPosDelta = 0x28;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x34;
}  // namespace DynamicRenderOffsetComponent

// EconomyTradeableComponent  [src/mc/entity/components_json_legacy/EconomyTradeableComponent.h]  расчётный размер 0x70
namespace EconomyTradeableComponent {
    constexpr ptrdiff_t mLastTradingPlayer = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mUpdateMerchantTimer = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mAddRecipeOnUpdate = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mRiches = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mOwner = 0x18;  // ::Actor&, 8 байт
    constexpr ptrdiff_t mOffers = 0x20;  // ::std::unique_ptr<::MerchantRecipeList>, 8 байт
    constexpr ptrdiff_t mDisplayName = 0x28;  // ::std::string, 32 байт
    constexpr ptrdiff_t mGeneratedTablePath = 0x48;  // ::std::string, 32 байт
    constexpr ptrdiff_t mConvertedFromVillagerV1 = 0x68;  // bool, 1 байт
    constexpr ptrdiff_t mDiscountDegradationTimeStamp = 0x6C;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace EconomyTradeableComponent

// EcsEventDispatcherComponent  [src/mc/deps/minecraft_camera/systems/EcsEventDispatcherComponent.h]  расчётный размер 0x10
namespace EcsEventDispatcherComponent {
    constexpr ptrdiff_t mDispatcher = 0x0;  // ::WeakRef<::EcsEventDispatcher>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace EcsEventDispatcherComponent

// ElytraFlightTimeTicksComponent  [src/mc/entity/components/ElytraFlightTimeTicksComponent.h]  расчётный размер 0x8
namespace ElytraFlightTimeTicksComponent {
    constexpr ptrdiff_t mValue = 0x0;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ElytraFlightTimeTicksComponent

// EmotePlayedTelemetryDataComponent  [src/mc/entity/components/EmotePlayedTelemetryDataComponent.h]  расчётный размер 0x24
namespace EmotePlayedTelemetryDataComponent {
    constexpr ptrdiff_t mEmoteProductId = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mEmoteSlotNumber = 0x20;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace EmotePlayedTelemetryDataComponent

// Enchant  [src/mc/world/item/enchanting/Enchant.h]  расчётный размер 0x99
namespace Enchant {
    constexpr ptrdiff_t mEnchantType = 0x0;  // ::Enchant::Type const, 1 байт
    constexpr ptrdiff_t mFrequency = 0x4;  // ::Enchant::Frequency const, 4 байт
    constexpr ptrdiff_t mIsAvailableInVillagerTrading = 0x8;  // bool const, 1 байт
    constexpr ptrdiff_t mPrimarySlots = 0xC;  // int const, 4 байт
    constexpr ptrdiff_t mSecondarySlots = 0x10;  // int const, 4 байт
    constexpr ptrdiff_t mCompatibility = 0x14;  // int const, 4 байт
    constexpr ptrdiff_t mDescription = 0x18;  // ::std::string const, 32 байт
    constexpr ptrdiff_t mStringId = 0x38;  // ::HashedString const, 48 байт
    constexpr ptrdiff_t mScriptStringId = 0x68;  // ::HashedString const, 48 байт
    constexpr ptrdiff_t mIsDisabled = 0x98;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x99;
}  // namespace Enchant

// EnchantableItemComponent  [src/mc/deps/shared_types/v1_20_50/item/EnchantableItemComponent.h]  расчётный размер 0x28
namespace EnchantableItemComponent {
    constexpr ptrdiff_t mEnchantValue = 0x0;  // short, 2 байт
    constexpr ptrdiff_t mEnchantSlot = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace EnchantableItemComponent

// EntitiesPendingEnterVolumeComponent  [src/mc/volume/components/EntitiesPendingEnterVolumeComponent.h]  расчётный размер 0x40
namespace EntitiesPendingEnterVolumeComponent {
    constexpr ptrdiff_t mEntities = 0x0;  // ::EntitySet, 64 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace EntitiesPendingEnterVolumeComponent

// EntityArmorEquipmentSlotMappingComponent  [src/mc/entity/components/EntityArmorEquipmentSlotMappingComponent.h]  расчётный размер 0x4
namespace EntityArmorEquipmentSlotMappingComponent {
    constexpr ptrdiff_t mArmorSlot = 0x0;  // ::SharedTypes::Legacy::ArmorSlot, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace EntityArmorEquipmentSlotMappingComponent

// EntityPlacerItemComponent  [src/mc/world/item/components/EntityPlacerItemComponent.h]  расчётный размер 0xF8  ⚠ BASE_UNKNOWN
namespace EntityPlacerItemComponent {
    constexpr ptrdiff_t mIdentifier = 0x0;  // ::ActorDefinitionIdentifier, 176 байт
    constexpr ptrdiff_t mAllowedUseBlocks = 0xB0;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mAllowedDispenseBlocks = 0xC8;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mCanUseOnMonsterSpawners = 0xE0;  // bool, 1 байт
    constexpr ptrdiff_t mShouldContentErrorWithoutLiquidClipped = 0xE1;  // bool, 1 байт
    constexpr ptrdiff_t mOnUseOnSubscription = 0xE8;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0xF8;
}  // namespace EntityPlacerItemComponent

// EntityRegistry  [src/mc/deps/ecs/gamerefs_entity/EntityRegistry.h]  расчётный размер 0x1D8
namespace EntityRegistry {
    constexpr ptrdiff_t mDebugName = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mRegistry = 0x20;  // ::entt::basic_registry<::EntityId>, 304 байт
    constexpr ptrdiff_t mId = 0x150;  // uint, 4 байт
    constexpr ptrdiff_t mPreEntityInvoke = 0x158;  // ::std::function<void(::EntityId)>, 64 байт
    constexpr ptrdiff_t mPostEntityInvoke = 0x198;  // ::std::function<void(::EntityId)>, 64 байт
    constexpr ptrdiff_t Size = 0x1D8;
}  // namespace EntityRegistry

// EntitySensorComponent  [src/mc/entity/components_json_legacy/EntitySensorComponent.h]  расчётный размер 0x79
namespace EntitySensorComponent {
    constexpr ptrdiff_t mRelativeRange = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mFindPlayersOnly = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mMaxBaseSearchArea = 0x4;  // ::AABB, 24 байт
    constexpr ptrdiff_t mEventCondition = 0x20;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mSubsensors = 0x60;  // ::std::vector<::EntitySubsensor>, 24 байт
    constexpr ptrdiff_t mCanFireEventWithNoEntities = 0x78;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x79;
}  // namespace EntitySensorComponent

// EntityStorageKeyComponent  [src/mc/entity/components/EntityStorageKeyComponent.h]  расчётный размер 0x41
namespace EntityStorageKeyComponent {
    constexpr ptrdiff_t mStorageKey = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLastSerializedActorState = 0x20;  // ::std::string, 32 байт
    constexpr ptrdiff_t mDirtyFromNonSerializedState = 0x40;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x41;
}  // namespace EntityStorageKeyComponent

// EquipItemComponent  [src/mc/entity/components/EquipItemComponent.h]  расчётный размер 0xA0
namespace EquipItemComponent {
    constexpr ptrdiff_t mItemStack = 0x0;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t mHasItemToEquip = 0x98;  // bool, 1 байт
    constexpr ptrdiff_t mCountToPickup = 0x9C;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xA0;
}  // namespace EquipItemComponent

// EquippableComponent  [src/mc/entity/components_json_legacy/EquippableComponent.h]  расчётный размер 0x18
namespace EquippableComponent {
    constexpr ptrdiff_t mSlots = 0x0;  // ::std::vector<::SlotDescriptor>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace EquippableComponent

// EventingDispatcherComponent  [src/mc/entity/components/EventingDispatcherComponent.h]  расчётный размер 0x8
namespace EventingDispatcherComponent {
    constexpr ptrdiff_t mEventingSource = 0x0;  // ::StrictEntityContext, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace EventingDispatcherComponent

// EventingRequestQueueComponent  [src/mc/entity/components/EventingRequestQueueComponent.h]  расчётный размер 0x28
namespace EventingRequestQueueComponent {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::queue<::EventingRequest, ::std::deque<::EventingRequest>>, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace EventingRequestQueueComponent

// ExecuteEventOnBlockRequestComponent  [src/mc/entity/components/ExecuteEventOnBlockRequestComponent.h]  расчётный размер 0x18
namespace ExecuteEventOnBlockRequestComponent {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::vector<::ExecuteEventOnBlockRequest>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ExecuteEventOnBlockRequestComponent

// ExhaustionComponent  [src/mc/entity/components/ExhaustionComponent.h]  расчётный размер 0x28
namespace ExhaustionComponent {
    constexpr ptrdiff_t mHeal = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mJump = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mSprintJump = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mMine = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mAttack = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mDamage = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mWalk = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mSprint = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mSwim = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mLunge = 0x24;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ExhaustionComponent

// ExperienceRewardComponent  [src/mc/entity/components_json_legacy/ExperienceRewardComponent.h]  расчётный размер 0x31
namespace ExperienceRewardComponent {
    constexpr ptrdiff_t mOnBred = 0x0;  // ::std::vector<::ExpressionNode>, 24 байт
    constexpr ptrdiff_t mOnDeath = 0x18;  // ::std::vector<::ExpressionNode>, 24 байт
    constexpr ptrdiff_t mIsExperienceDropEnabled = 0x30;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x31;
}  // namespace ExperienceRewardComponent

// ExplodeComponent  [src/mc/entity/components_json_legacy/ExplodeComponent.h]  расчётный размер 0x2D
namespace ExplodeComponent {
    constexpr ptrdiff_t mFuseLength = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mInitialFuseLength = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mExplosionPower = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mMaxResistance = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mIsFuseLit = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t mCausesFire = 0x11;  // bool, 1 байт
    constexpr ptrdiff_t mDamageScaling = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mBreaksBlocks = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mTogglesBlocks = 0x19;  // bool, 1 байт
    constexpr ptrdiff_t mFireAffectedByGriefing = 0x1A;  // bool, 1 байт
    constexpr ptrdiff_t mDestroyAffectedByGriefing = 0x1B;  // bool, 1 байт
    constexpr ptrdiff_t mAllowUnderwater = 0x1C;  // bool, 1 байт
    constexpr ptrdiff_t mRequiresTntExplodeGameRuleEnabled = 0x1D;  // bool, 1 байт
    constexpr ptrdiff_t mKnockbackScaling = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mParticleType = 0x24;  // ::SharedTypes::Legacy::LevelEvent, 2 байт
    constexpr ptrdiff_t mSoundEffect = 0x28;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mNegatesFallDamage = 0x2C;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2D;
}  // namespace ExplodeComponent

// ExternalDataComponent  [src/mc/entity/components/ExternalDataComponent.h]  расчётный размер 0x8
namespace ExternalDataComponent {
    constexpr ptrdiff_t mExternalDataInterface = 0x0;  // ::std::unique_ptr<::ExternalDataInterface>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ExternalDataComponent

// FadeInstruction  [src/mc/deps/minecraft_camera/camera_instruction_options/FadeInstruction.h]  расчётный размер 0x38
namespace FadeInstruction {
    constexpr ptrdiff_t mRed = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mGreen = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mBlue = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mFadeInTime = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mHoldTime = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mFadeOutTime = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mTime = 0x18;  // ::std::optional<::CameraInstructionOptions::FadeInstruction::TimeOption>, 16 байт
    constexpr ptrdiff_t mColor = 0x28;  // ::std::optional<::CameraInstructionOptions::FadeInstruction::ColorOption>, 16 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace FadeInstruction

// FallDamageResultComponent  [src/mc/entity/components/FallDamageResultComponent.h]  расчётный размер 0x10
namespace FallDamageResultComponent {
    constexpr ptrdiff_t mFallDistance = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mPos = 0x4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace FallDamageResultComponent

// FallDistanceComponent  [src/mc/entity/components/FallDistanceComponent.h]  расчётный размер 0x6
namespace FallDistanceComponent {
    constexpr ptrdiff_t mValue = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mIgnoresFallDamage = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mSpawnParticlesWhenHittingGround = 0x5;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x6;
}  // namespace FallDistanceComponent

// FireAnimationTrackerComponent  [src/mc/entity/components/FireAnimationTrackerComponent.h]  расчётный размер 0x4
namespace FireAnimationTrackerComponent {
    constexpr ptrdiff_t mFireStartTick = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace FireAnimationTrackerComponent

// FishAnimationComponent  [src/mc/entity/components/FishAnimationComponent.h]  расчётный размер 0x8
namespace FishAnimationComponent {
    constexpr ptrdiff_t mAnimationAmount = 0x0;  // ::InterpolationPair, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace FishAnimationComponent

// FloatOption  [src/mc/options/option_types/FloatOption.h]  расчётный размер 0x14  ⚠ BASE_UNKNOWN
namespace FloatOption {
    constexpr ptrdiff_t VALUE_MIN = 0x0;  // float const, 4 байт
    constexpr ptrdiff_t VALUE_MAX = 0x4;  // float const, 4 байт
    constexpr ptrdiff_t mValue = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mDefaultValue = 0xC;  // float, 4 байт
    constexpr ptrdiff_t DELTA = 0x10;  // float const, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace FloatOption

// FlockingComponent  [src/mc/entity/components_json_legacy/FlockingComponent.h]  расчётный размер 0x90
namespace FlockingComponent {
    constexpr ptrdiff_t mInWater = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mMatchVariant = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mUseCenterOfMass = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mIsLeader = 0x3;  // bool, 1 байт
    constexpr ptrdiff_t mInFlock = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mIsEnabled = 0x5;  // bool, 1 байт
    constexpr ptrdiff_t mHasTargetGoal = 0x6;  // bool, 1 байт
    constexpr ptrdiff_t mUsingDirection = 0x7;  // bool, 1 байт
    constexpr ptrdiff_t mFlockLimit = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mLonerChance = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mGoalWeight = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mInfluenceRadius = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mBreachInfluence = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mSeparationWeight = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mSeparationThreshold = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mCohesionWeight = 0x24;  // float, 4 байт
    constexpr ptrdiff_t mCohesionThreshold = 0x28;  // float, 4 байт
    constexpr ptrdiff_t mInnerCohesionThreshold = 0x2C;  // float, 4 байт
    constexpr ptrdiff_t mMinHeight = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mMaxHeight = 0x34;  // float, 4 байт
    constexpr ptrdiff_t mBlockDist = 0x38;  // float, 4 байт
    constexpr ptrdiff_t mBlockWeight = 0x3C;  // float, 4 байт
    constexpr ptrdiff_t mOverspeedRequired = 0x40;  // bool, 1 байт
    constexpr ptrdiff_t mNeighborhood = 0x48;  // ::std::vector<::ActorUniqueID>, 24 байт
    constexpr ptrdiff_t mCenterOfMass = 0x60;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mGroupVelocity = 0x6C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mGoalHeading = 0x78;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCurrentHeading = 0x84;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x90;
}  // namespace FlockingComponent

// FogAppearanceClientBiomeJsonComponent  [src/mc/deps/shared_types/v1_21_40/clientbiome/components/FogAppearanceClientBiomeJsonComponent.h]  расчётный размер 0x20  ⚠ BASE_UNKNOWN
namespace FogAppearanceClientBiomeJsonComponent {
    constexpr ptrdiff_t mFogIdentifier = 0x0;  // ::SharedTypes::Reference<9>, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace FogAppearanceClientBiomeJsonComponent

// FogCommandComponent  [src/mc/entity/components/FogCommandComponent.h]  расчётный размер 0x18
namespace FogCommandComponent {
    constexpr ptrdiff_t mFogStack = 0x0;  // ::std::vector<::FogCommandSettings>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace FogCommandComponent

// FoodItemComponent  [src/mc/world/item/components/FoodItemComponent.h]  расчётный размер 0x40  ⚠ BASE_UNKNOWN
namespace FoodItemComponent {
    constexpr ptrdiff_t mNutrition = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mSaturationModifier = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mUsingConvertsTo = 0x8;  // ::ItemDescriptor, 16 байт
    constexpr ptrdiff_t mCanAlwaysEat = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mOnUseTimeDepletedSubscription = 0x20;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x30;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace FoodItemComponent

// FreezingComponent  [src/mc/entity/components/FreezingComponent.h]  расчётный размер 0x4
namespace FreezingComponent {
    constexpr ptrdiff_t mFreezingEffectStrength = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace FreezingComponent

// FuelItemComponent  [src/mc/deps/shared_types/v1_20_50/item/FuelItemComponent.h]  расчётный размер 0x4
namespace FuelItemComponent {
    constexpr ptrdiff_t mDuration = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace FuelItemComponent

// GainedRaidOmenAtPositionComponent  [src/mc/entity/components/GainedRaidOmenAtPositionComponent.h]  расчётный размер 0xC
namespace GainedRaidOmenAtPositionComponent {
    constexpr ptrdiff_t mPosition = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace GainedRaidOmenAtPositionComponent

// GameEventListenerComponent  [src/mc/entity/components/GameEventListenerComponent.h]  расчётный размер 0x8
namespace GameEventListenerComponent {
    constexpr ptrdiff_t mListenerRegistration = 0x0;  // ::std::unique_ptr<::GameEventDynamicRegistration>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace GameEventListenerComponent

// GameEventMovementTrackingComponent  [src/mc/entity/components_json_legacy/GameEventMovementTrackingComponent.h]  расчётный размер 0x25
namespace GameEventMovementTrackingComponent {
    constexpr ptrdiff_t mNextFlapTick = 0x0;  // ::Tick, 8 байт
    constexpr ptrdiff_t mPrevPos = 0x8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mStep = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mNextStep = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mMoveDist = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mConsiderVerticalComponentForAmphibiousMovement = 0x20;  // bool, 1 байт
    constexpr ptrdiff_t mSettings = 0x21;  // ::std::optional<::GameEventMovementTrackingSettings>, 4 байт
    constexpr ptrdiff_t Size = 0x25;
}  // namespace GameEventMovementTrackingComponent

// GameMode  [src/mc/world/gamemode/GameMode.h]  расчётный размер 0x128
namespace GameMode {
    constexpr ptrdiff_t mHasBuildDirection = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mHasLastBuiltPosition = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mLastBuildBlockWasInteractive = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mLastBuildBlockAlignedPlacement = 0x3;  // bool, 1 байт
    constexpr ptrdiff_t mLastBuildBlockHadSuccessfulUse = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mLastBuiltBlockPosition = 0x8;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mBuildDirection = 0x14;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mNextBuildPos = 0x20;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mInitialClickPos = 0x2C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mContinueFacing = 0x38;  // uchar, 1 байт
    constexpr ptrdiff_t mLastBuildHandSlot = 0x39;  // ::HandSlot, 1 байт
    constexpr ptrdiff_t mHasDelayElapsed = 0x3A;  // bool, 1 байт
    constexpr ptrdiff_t mNewLastBuildTimeIfBlockIsBuilt = 0x40;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mPosDelta = 0x48;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mIsSneaking = 0x54;  // bool, 1 байт
    constexpr ptrdiff_t mHit = 0x58;  // ::BlockPos const, 12 байт
    constexpr ptrdiff_t mFacing = 0x64;  // uchar const, 1 байт
    constexpr ptrdiff_t mPlayer = 0x68;  // ::Player&, 8 байт
    constexpr ptrdiff_t mDestroyBlockPos = 0x70;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mDestroyBlockFace = 0x7C;  // uchar, 1 байт
    constexpr ptrdiff_t mOldDestroyProgress = 0x80;  // float, 4 байт
    constexpr ptrdiff_t mDestroyProgress = 0x84;  // float, 4 байт
    constexpr ptrdiff_t mLastDestroyTime = 0x88;  // double, 8 байт
    constexpr ptrdiff_t mDistanceTravelled = 0x90;  // float, 4 байт
    constexpr ptrdiff_t mPlayerLastPosition = 0x94;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBuildContext = 0xA0;  // ::GameMode::BuildContext, 60 байт
    constexpr ptrdiff_t mMinPlayerSpeed = 0xDC;  // float, 4 байт
    constexpr ptrdiff_t mContinueBreakBlockCount = 0xE0;  // int, 4 байт
    constexpr ptrdiff_t mLastBuildTime = 0xE8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mNoDestroyUntil = 0xF0;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mNoDestroySoundUntil = 0xF8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t creativeDestructionTickDelay = 0x100;  // ::std::chrono::milliseconds, 8 байт
    constexpr ptrdiff_t buildingTickDelay = 0x108;  // ::std::chrono::milliseconds, 8 байт
    constexpr ptrdiff_t destroySoundDelay = 0x110;  // ::std::chrono::milliseconds, 8 байт
    constexpr ptrdiff_t mTimer = 0x118;  // ::std::unique_ptr<::IGameModeTimer>, 8 байт
    constexpr ptrdiff_t mMessenger = 0x120;  // ::std::unique_ptr<::IGameModeMessenger>, 8 байт
    constexpr ptrdiff_t Size = 0x128;
}  // namespace GameMode

// GameSession  [src/mc/world/GameSession.h]  расчётный размер 0x61
namespace GameSession {
    constexpr ptrdiff_t mNetwork = 0x0;  // ::ClientOrServerNetworkSystemRef, 16 байт
    constexpr ptrdiff_t mLevelEntity = 0x10;  // ::OwnerPtr<::EntityContext>, 32 байт
    constexpr ptrdiff_t mLevel = 0x30;  // ::Bedrock::NonOwnerPointer<::Level>, 24 байт
    constexpr ptrdiff_t mServerNetworkHandler = 0x48;  // ::std::unique_ptr<::ServerNetworkHandler>, 8 байт
    constexpr ptrdiff_t mLegacyClientNetworkHandler = 0x50;  // ::std::unique_ptr<::NetEventCallback>, 8 байт
    constexpr ptrdiff_t mLoopbackPacketSender = 0x58;  // ::LoopbackPacketSender&, 8 байт
    constexpr ptrdiff_t mClientSubId = 0x60;  // ::SubClientId, 1 байт
    constexpr ptrdiff_t Size = 0x61;
}  // namespace GameSession

// GeneticsComponent  [src/mc/entity/components_json_legacy/GeneticsComponent.h]  расчётный размер 0x30
namespace GeneticsComponent {
    constexpr ptrdiff_t mainAllele = 0x0;  // int, 4 байт
    constexpr ptrdiff_t hiddenAllele = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mGenes = 0x8;  // ::std::vector<::GeneticsComponent::Gene>, 24 байт
    constexpr ptrdiff_t mGeneticsDescription = 0x20;  // ::GeneticsDefinition const*, 8 байт
    constexpr ptrdiff_t mRandom = 0x28;  // ::Random*, 8 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace GeneticsComponent

// GetAttachPositionViewsComponent  [src/mc/entity/components/GetAttachPositionViewsComponent.h]  расчётный размер 0x1E0
namespace GetAttachPositionViewsComponent {
    constexpr ptrdiff_t mViews = 0x0;  // ::GetAttachPositionViews, 480 байт
    constexpr ptrdiff_t Size = 0x1E0;
}  // namespace GetAttachPositionViewsComponent

// GiveableComponent  [src/mc/entity/components_json_legacy/GiveableComponent.h]  расчётный размер 0x18
namespace GiveableComponent {
    constexpr ptrdiff_t mCooldownTimeStamps = 0x0;  // ::std::vector<uint64>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace GiveableComponent

// GlidingCollisionDamageComponent  [src/mc/entity/components/GlidingCollisionDamageComponent.h]  расчётный размер 0x4
namespace GlidingCollisionDamageComponent {
    constexpr ptrdiff_t mDamage = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace GlidingCollisionDamageComponent

// GlintItemComponent  [src/mc/deps/shared_types/v1_20_50/item/GlintItemComponent.h]  расчётный размер 0x1
namespace GlintItemComponent {
    constexpr ptrdiff_t mIsGlint = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace GlintItemComponent

// GlobalTextureGroupStateComponent  [src/mc/entity/components/GlobalTextureGroupStateComponent.h]  расчётный размер 0x1
namespace GlobalTextureGroupStateComponent {
    constexpr ptrdiff_t mIsReloading = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace GlobalTextureGroupStateComponent

// GoalSelectorComponent  [src/mc/entity/components/GoalSelectorComponent.h]  расчётный размер 0x18
namespace GoalSelectorComponent {
    constexpr ptrdiff_t mGoalMap = 0x0;  // ::std::vector<::std::pair<::GoalId, ::PrioritizedGoal>>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace GoalSelectorComponent

// GroupSizeComponent  [src/mc/entity/components/GroupSizeComponent.h]  расчётный размер 0x10
namespace GroupSizeComponent {
    constexpr ptrdiff_t mCount = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mNextTick = 0x8;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace GroupSizeComponent

// GrowsCropComponent  [src/mc/entity/components_json_legacy/GrowsCropComponent.h]  расчётный размер 0x1C
namespace GrowsCropComponent {
    constexpr ptrdiff_t mCharges = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mTargetCrop = 0x4;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mLastGrownCrop = 0x10;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace GrowsCropComponent

// GuiData  [src-client/mc/client/gui/GuiData.h]  расчётный размер 0xEB0  ⚠ BASE_UNKNOWN
namespace GuiData {
    constexpr ptrdiff_t messageType = 0x0;  // ::GuiMessage::MessageType, 4 байт
    constexpr ptrdiff_t username = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t message = 0x28;  // ::std::string, 32 байт
    constexpr ptrdiff_t filteredMessage = 0x48;  // ::std::optional<::std::string>, 40 байт
    constexpr ptrdiff_t ttsMessage = 0x70;  // ::std::string, 32 байт
    constexpr ptrdiff_t lifeTime = 0x90;  // float, 4 байт
    constexpr ptrdiff_t forceVisible = 0x94;  // bool, 1 байт
    constexpr ptrdiff_t ttsRequired = 0x95;  // bool, 1 байт
    constexpr ptrdiff_t xuid = 0x98;  // ::std::string, 32 байт
    constexpr ptrdiff_t platformId = 0xB8;  // ::std::string const, 32 байт
    constexpr ptrdiff_t enableEmoticonify = 0xD8;  // bool, 1 байт
    constexpr ptrdiff_t mScreenSizeData = 0xDC;  // ::ScreenSizeData, 24 байт
    constexpr ptrdiff_t mScreenSizeDataValid = 0xF4;  // bool, 1 байт
    constexpr ptrdiff_t mGuiScale = 0xF8;  // float, 4 байт
    constexpr ptrdiff_t mInvGuiScale = 0xFC;  // float, 4 байт
    constexpr ptrdiff_t mOverlappingControlsExist = 0x100;  // bool, 1 байт
    constexpr ptrdiff_t mIsCurrentlyActive = 0x101;  // bool, 1 байт
    constexpr ptrdiff_t mPostedErrors = 0x108;  // ::std::set<int>, 16 байт
    constexpr ptrdiff_t mMenuPointer = 0x118;  // ::MenuPointer, 1 байт
    constexpr ptrdiff_t mPointerX = 0x11A;  // short, 2 байт
    constexpr ptrdiff_t mPointerY = 0x11C;  // short, 2 байт
    constexpr ptrdiff_t mHasShowPreexistingMessages = 0x11E;  // bool, 1 байт
    constexpr ptrdiff_t mToolbarWasRendered = 0x11F;  // bool, 1 байт
    constexpr ptrdiff_t mPrevSelectedSlot = 0x120;  // int, 4 байт
    constexpr ptrdiff_t mPrevSelectedInventoryContainer = 0x124;  // ::ContainerID, 1 байт
    constexpr ptrdiff_t mFlashSlotId = 0x128;  // int, 4 байт
    constexpr ptrdiff_t mFlashSlotStartTime = 0x130;  // double, 8 байт
    constexpr ptrdiff_t mClient = 0x138;  // ::IClientInstance&, 8 байт
    constexpr ptrdiff_t mToolbarArea = 0x140;  // ::RectangleArea, 16 байт
    constexpr ptrdiff_t mToolbarAreaContainer = 0x150;  // ::RectangleArea, 16 байт
    constexpr ptrdiff_t mLastPopupText = 0x160;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLastPopupSubtitleText = 0x180;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLastJukeboxPopupText = 0x1A0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLastJukeboxPopupSubtitleText = 0x1C0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mTickCount = 0x1E0;  // int, 4 байт
    constexpr ptrdiff_t mItemNameOverlayTime = 0x1E4;  // float, 4 байт
    constexpr ptrdiff_t mJukeboxNameOverlayTime = 0x1E8;  // float, 4 байт
    constexpr ptrdiff_t mPopupNoticeDirty = 0x1EC;  // bool, 1 байт
    constexpr ptrdiff_t mJukeboxPopupNoticeDirty = 0x1ED;  // bool, 1 байт
    constexpr ptrdiff_t mGuiMessages = 0x1F0;  // ::std::vector<::GuiMessage>, 24 байт
    constexpr ptrdiff_t mSubtitleMessages = 0x208;  // ::std::vector<::GuiMessage>, 24 байт
    constexpr ptrdiff_t mDevConsoleMessages = 0x220;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t mMaxDevConsoleMessages = 0x238;  // int, 4 байт
    constexpr ptrdiff_t mContentLogMessages = 0x240;  // ::std::vector<::ContentLogMessage>, 24 байт
    constexpr ptrdiff_t mContentLogErrorCount = 0x258;  // uint64, 8 байт
    constexpr ptrdiff_t mPerfTurtleMessages = 0x260;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t mModalScreenData = 0x278;  // ::std::optional<::ModalScreenData>, 376 байт
    constexpr ptrdiff_t mTitleMessage = 0x3F0;  // ::TitleMessage, 232 байт
    constexpr ptrdiff_t mHudVisibilityState = 0x4D8;  // ::std::array<::HudVisibility, 13>, 52 байт
    constexpr ptrdiff_t mServerSettingsId = 0x50C;  // uint, 4 байт
    constexpr ptrdiff_t mServerSettings = 0x510;  // ::std::string, 32 байт
    constexpr ptrdiff_t mMuteChat = 0x530;  // bool, 1 байт
    constexpr ptrdiff_t mCurrentDropTicks = 0x534;  // float, 4 байт
    constexpr ptrdiff_t mCurrentDropSlot = 0x538;  // ::PlayerInventorySlotData, 8 байт
    constexpr ptrdiff_t mLastSelectedSlot = 0x540;  // ::PlayerInventorySlotData, 8 байт
    constexpr ptrdiff_t mShowProgress = 0x548;  // bool, 1 байт
    constexpr ptrdiff_t mTipMessage = 0x550;  // ::std::string, 32 байт
    constexpr ptrdiff_t mTipMessageLength = 0x570;  // float, 4 байт
    constexpr ptrdiff_t mRcFeedbackOuter = 0x578;  // ::mce::Mesh, 632 байт
    constexpr ptrdiff_t mRcFeedbackInner = 0x7F0;  // ::mce::Mesh, 632 байт
    constexpr ptrdiff_t mVignette = 0xA68;  // ::mce::Mesh, 632 байт
    constexpr ptrdiff_t mInvFillMat = 0xCE0;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t mCursorMat = 0xCF0;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t mDevConsoleLogger = 0xD00;  // ::Bedrock::NotNullNonOwnerPtr<::DevConsoleLogger>, 24 байт
    constexpr ptrdiff_t mLastTickTime = 0xD18;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mDelayedMessages = 0xD20;  // ::std::map<::std::string, ::std::vector<::GuiMessage>>, 16 байт
    constexpr ptrdiff_t mQueuedDevConsoleMessages = 0xD30;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t mQueuedDevMessagesMutex = 0xD48;  // ::std::mutex, 80 байт
    constexpr ptrdiff_t mUseEditorGuiScale = 0xD98;  // bool, 1 байт
    constexpr ptrdiff_t mHUDHotbarRectangle = 0xD9C;  // ::RectangleArea, 16 байт
    constexpr ptrdiff_t mCoordinateCaptureType = 0xDAC;  // ::CoordinateCaptureType, 1 байт
    constexpr ptrdiff_t mOnGuiScaleUpdatedPublisher = 0xDB0;  // ::Bedrock::PubSub::Publisher<void(), ::Bedrock::PubSub::ThreadModel::MultiThreaded, 0>, 128 байт
    constexpr ptrdiff_t mServerFormDataAvailable = 0xE30;  // ::Bedrock::PubSub::Publisher<void(), ::Bedrock::PubSub::ThreadModel::MultiThreaded, 0>, 128 байт
    constexpr ptrdiff_t Size = 0xEB0;
}  // namespace GuiData

// HandEquippedItemComponent  [src/mc/deps/shared_types/v1_20_50/item/HandEquippedItemComponent.h]  расчётный размер 0x1
namespace HandEquippedItemComponent {
    constexpr ptrdiff_t mHandEquipped = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace HandEquippedItemComponent

// HeartbeatClientComponent  [src/mc/entity/components/HeartbeatClientComponent.h]  расчётный размер 0x20
namespace HeartbeatClientComponent {
    constexpr ptrdiff_t mHeartbeatSoundEvent = 0x0;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mHeartRateIntervalTicks = 0x4;  // uint, 4 байт
    constexpr ptrdiff_t mHeartbeatPhaseRatio = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mStaleData = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mLastHeartbeatTick = 0x10;  // ::std::optional<::Tick>, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace HeartbeatClientComponent

// HeartbeatServerComponent  [src/mc/entity/components/HeartbeatServerComponent.h]  расчётный размер 0x218
namespace HeartbeatServerComponent {
    constexpr ptrdiff_t mHeartRateExpression = 0x0;  // ::ExpressionNode, 16 байт
    constexpr ptrdiff_t mRenderParams = 0x10;  // ::RenderParams, 520 байт
    constexpr ptrdiff_t Size = 0x218;
}  // namespace HeartbeatServerComponent

// HideComponent  [src/mc/entity/components_json_legacy/HideComponent.h]  расчётный размер 0x2
namespace HideComponent {
    constexpr ptrdiff_t mIsInRaid = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mReactToBell = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace HideComponent

// HitResultComponent  [src/mc/entity/components/HitResultComponent.h]  расчётный размер 0x8
namespace HitResultComponent {
    constexpr ptrdiff_t mHitResults = 0x0;  // ::std::unique_ptr<::IHitResultContainer>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace HitResultComponent

// HitboxComponent  [src/mc/deps/vanilla_components/HitboxComponent.h]  расчётный размер 0x18
namespace HitboxComponent {
    constexpr ptrdiff_t mHitboxes = 0x0;  // ::std::vector<::Hitbox>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace HitboxComponent

// HomeComponent  [src/mc/entity/components_json_legacy/HomeComponent.h]  расчётный размер 0x40
namespace HomeComponent {
    constexpr ptrdiff_t mRestrictionType = 0x0;  // ::SharedTypes::v1_26_50::HomeComponentDefinition::RestrictionType, 4 байт
    constexpr ptrdiff_t mRestrictionRadius = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mRestrictionRadiusSqr = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mCooldownTicksMax = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mCooldownTicks = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mHomeBlocks = 0x18;  // ::std::vector<::BlockType const*>, 24 байт
    constexpr ptrdiff_t mHomePos = 0x30;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mDimensionId = 0x3C;  // ::DimensionType, 4 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace HomeComponent

// HopperComponent  [src/mc/entity/components_json_legacy/HopperComponent.h]  расчётный размер 0xC  ⚠ BASE_UNKNOWN
namespace HopperComponent {
    constexpr ptrdiff_t mLastPosition = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace HopperComponent

// HorizontalCollisionFlagComponent  [src/mc/deps/vanilla_components/HorizontalCollisionFlagComponent.h]  расчётный размер 0x2
namespace HorizontalCollisionFlagComponent {
    constexpr ptrdiff_t mCollidedOnXAxis = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mCollidedOnZAxis = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace HorizontalCollisionFlagComponent

// HorseAnimationComponent  [src/mc/entity/components/HorseAnimationComponent.h]  расчётный размер 0xC
namespace HorseAnimationComponent {
    constexpr ptrdiff_t mEatCounter = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mMouthCounter = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mTailCounter = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace HorseAnimationComponent

// HorseStandCounterComponent  [src/mc/entity/components/HorseStandCounterComponent.h]  расчётный размер 0x4
namespace HorseStandCounterComponent {
    constexpr ptrdiff_t mCounter = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace HorseStandCounterComponent

// HoverTextColorItemComponent  [src/mc/deps/shared_types/v1_20_50/item/HoverTextColorItemComponent.h]  расчётный размер 0x20
namespace HoverTextColorItemComponent {
    constexpr ptrdiff_t mValue = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace HoverTextColorItemComponent

// HumanoidMonsterAttackStateComponent  [src/mc/entity/components/HumanoidMonsterAttackStateComponent.h]  расчётный размер 0x4
namespace HumanoidMonsterAttackStateComponent {
    constexpr ptrdiff_t mState = 0x0;  // ::AttackState, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace HumanoidMonsterAttackStateComponent

// IconItemComponent  [src/mc/deps/shared_types/v1_20_50/item/IconItemComponent.h]  расчётный размер 0x20
namespace IconItemComponent {
    constexpr ptrdiff_t mTexture = 0x0;  // ::SharedTypes::Reference<11>, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace IconItemComponent

// InsideBlockComponent  [src/mc/entity/components/InsideBlockComponent.h]  расчётный размер 0x14
namespace InsideBlockComponent {
    constexpr ptrdiff_t mInsideBlock = 0x0;  // ::Block const*, 8 байт
    constexpr ptrdiff_t mInsideBlockPos = 0x8;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace InsideBlockComponent

// InsideBlockNotifierComponent  [src/mc/entity/components_json_legacy/InsideBlockNotifierComponent.h]  расчётный размер 0x18
namespace InsideBlockNotifierComponent {
    constexpr ptrdiff_t mBlockList = 0x0;  // ::std::vector<::InsideBlockEventMap>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace InsideBlockNotifierComponent

// InsideGenericBlockComponent  [src/mc/entity/components/InsideGenericBlockComponent.h]  расчётный размер 0x39
namespace InsideGenericBlockComponent {
    constexpr ptrdiff_t mData = 0x0;  // ::std::vector<::InsideBlockData>, 24 байт
    constexpr ptrdiff_t mSubstituteAABB = 0x18;  // ::std::optional<::AABB>, 28 байт
    constexpr ptrdiff_t mGrow = 0x34;  // float, 4 байт
    constexpr ptrdiff_t mHasChunks = 0x38;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x39;
}  // namespace InsideGenericBlockComponent

// InsideOnewayBlockComponent  [src/mc/entity/components/InsideOnewayBlockComponent.h]  расчётный размер 0x18
namespace InsideOnewayBlockComponent {
    constexpr ptrdiff_t mData = 0x0;  // ::std::vector<::AABB>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace InsideOnewayBlockComponent

// InsomniaComponent  [src/mc/entity/components_json_legacy/InsomniaComponent.h]  расчётный размер 0xC
namespace InsomniaComponent {
    constexpr ptrdiff_t mTimeSinceRest = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mDaysUntilInsomnia = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mTicksUntilInsomnia = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace InsomniaComponent

// IntOption  [src/mc/options/option_types/IntOption.h]  расчётный размер 0x70  ⚠ BASE_UNKNOWN
namespace IntOption {
    constexpr ptrdiff_t VALUE_MAX = 0x0;  // int const, 4 байт
    constexpr ptrdiff_t VALUE_MIN = 0x4;  // int const, 4 байт
    constexpr ptrdiff_t mValue = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mDefaultValue = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mClampToRange = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t mPossibleValues = 0x18;  // ::std::vector<int>, 24 байт
    constexpr ptrdiff_t mCoerceSaveValueCallback = 0x30;  // ::std::function<int(int)>, 64 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace IntOption

// InteractButtonItemComponent  [src/mc/deps/shared_types/v1_20_50/item/InteractButtonItemComponent.h]  расчётный размер 0x28
namespace InteractButtonItemComponent {
    constexpr ptrdiff_t mRequiresInteract = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mInteractText = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace InteractButtonItemComponent

// InteractComponent  [src/mc/entity/components_json_legacy/InteractComponent.h]  расчётный размер 0x20
namespace InteractComponent {
    constexpr ptrdiff_t mCooldownCounter = 0x0;  // short, 2 байт
    constexpr ptrdiff_t mInteractions = 0x8;  // ::std::vector<::InteractionDescription>, 24 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace InteractComponent

// InteractPacket  [src/mc/network/packet/InteractPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace InteractPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace InteractPacket

// InventoryAction  [src/mc/world/inventory/transaction/InventoryAction.h]  расчётный размер 0x200
namespace InventoryAction {
    constexpr ptrdiff_t mSource = 0x0;  // ::InventorySource, 12 байт
    constexpr ptrdiff_t mSlot = 0xC;  // uint, 4 байт
    constexpr ptrdiff_t mFromItemDescriptor = 0x10;  // ::NetworkItemStackDescriptor, 96 байт
    constexpr ptrdiff_t mToItemDescriptor = 0x70;  // ::NetworkItemStackDescriptor, 96 байт
    constexpr ptrdiff_t mFromItem = 0xD0;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t mToItem = 0x168;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t Size = 0x200;
}  // namespace InventoryAction

// InventoryContentPacket  [src/mc/network/packet/InventoryContentPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace InventoryContentPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace InventoryContentPacket

// InventorySource  [src/mc/world/inventory/transaction/InventorySource.h]  расчётный размер 0xC
namespace InventorySource {
    constexpr ptrdiff_t mType = 0x0;  // ::InventorySourceType, 4 байт
    constexpr ptrdiff_t mContainerId = 0x4;  // ::ContainerID, 1 байт
    constexpr ptrdiff_t mFlags = 0x8;  // ::InventorySource::InventorySourceFlags, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace InventorySource

// InventoryTransaction  [src/mc/world/inventory/transaction/InventoryTransaction.h]  расчётный размер 0x58
namespace InventoryTransaction {
    constexpr ptrdiff_t mActions = 0x0;  // ::std::unordered_map<::InventorySource, ::std::vector<::InventoryAction>>, 64 байт
    constexpr ptrdiff_t mContents = 0x40;  // ::std::vector<::InventoryTransactionItemGroup>, 24 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace InventoryTransaction

// InventoryTransactionItemGroup  [src/mc/world/inventory/transaction/InventoryTransactionItemGroup.h]  расчётный размер 0x15
namespace InventoryTransactionItemGroup {
    constexpr ptrdiff_t mItemId = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mItemAux = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mTag = 0x8;  // ::std::unique_ptr<::CompoundTag>, 8 байт
    constexpr ptrdiff_t mCount = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mOverflow = 0x14;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x15;
}  // namespace InventoryTransactionItemGroup

// InventoryTransactionPacket  [src/mc/network/packet/InventoryTransactionPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace InventoryTransactionPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace InventoryTransactionPacket

// IsSolidMobComponent  [src/mc/entity/components/IsSolidMobComponent.h]  расчётный размер 0xE
namespace IsSolidMobComponent {
    constexpr ptrdiff_t mDepenetrationMagnitude = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mIsCollidable = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mIsStackable = 0xD;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xE;
}  // namespace IsSolidMobComponent

// IsSolidMobNearbyComponent  [src/mc/entity/components/IsSolidMobNearbyComponent.h]  расчётный размер 0xC
namespace IsSolidMobNearbyComponent {
    constexpr ptrdiff_t mMaxDepenetrationMagnitude = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace IsSolidMobNearbyComponent

// Item  [src/mc/world/item/Item.h]  расчётный размер 0x208
namespace Item {
    constexpr ptrdiff_t mItemParseVersion = 0x0;  // ::ItemVersion, 4 байт
    constexpr ptrdiff_t mTextureAtlasFile = 0x8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mFrameCount = 0x28;  // int, 4 байт
    constexpr ptrdiff_t mAnimatesInToolbar = 0x2C;  // bool, 1 байт
    constexpr ptrdiff_t mIsMirroredArt = 0x2D;  // bool, 1 байт
    constexpr ptrdiff_t mUseAnim = 0x2E;  // ::SharedTypes::Legacy::UseAnimation, 1 байт
    constexpr ptrdiff_t mHoverTextColorFormat = 0x30;  // ::std::string, 32 байт
    constexpr ptrdiff_t mIconFrame = 0x50;  // int, 4 байт
    constexpr ptrdiff_t mAtlasFrame = 0x54;  // int, 4 байт
    constexpr ptrdiff_t mAtlasTotalFrames = 0x58;  // int, 4 байт
    constexpr ptrdiff_t mIconName = 0x60;  // ::std::string, 32 байт
    constexpr ptrdiff_t mAtlasName = 0x80;  // ::std::string, 32 байт
    constexpr ptrdiff_t mMaxStackSize = 0xA0;  // uchar, 1 байт
    constexpr ptrdiff_t mId = 0xA2;  // short, 2 байт
    constexpr ptrdiff_t mDescriptionId = 0xA8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mRawNameId = 0xC8;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mNamespace = 0xF8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mFullName = 0x118;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mMaxDamage = 0x148;  // short, 2 байт
    constexpr ptrdiff_t mMaxUseDuration = 0x14C;  // int, 4 байт
    constexpr ptrdiff_t mMinRequiredBaseGameVersion = 0x150;  // ::BaseGameVersion, 32 байт
    constexpr ptrdiff_t mBlockType = 0x170;  // ::BlockType const*, 8 байт
    constexpr ptrdiff_t mCreativeCategory = 0x178;  // ::SharedTypes::CreativeItemCategory, 1 байт
    constexpr ptrdiff_t mCraftingRemainingItem = 0x180;  // ::Item*, 8 байт
    constexpr ptrdiff_t mCreativeGroup = 0x188;  // ::std::string, 32 байт
    constexpr ptrdiff_t mFurnaceBurnIntervalModifier = 0x1A8;  // float, 4 байт
    constexpr ptrdiff_t mFurnaceXPmultiplier = 0x1AC;  // float, 4 байт
    constexpr ptrdiff_t mIsHiddenInCommands = 0x1B0;  // ::ItemCommandVisibility, 1 байт
    constexpr ptrdiff_t mBaseRarity = 0x1B4;  // ::Rarity, 4 байт
    constexpr ptrdiff_t mMineBlockType = 0x1B8;  // ::Interactions::Mining::MineBlockItemEffectType, 4 байт
    constexpr ptrdiff_t mFoodComponentLegacy = 0x1C0;  // ::std::unique_ptr<::FoodItemComponentLegacy>, 8 байт
    constexpr ptrdiff_t mSeedComponent = 0x1C8;  // ::std::unique_ptr<::SeedItemComponentLegacy>, 8 байт
    constexpr ptrdiff_t mCameraComponentLegacy = 0x1D0;  // ::std::unique_ptr<::CameraItemComponentLegacy>, 8 байт
    constexpr ptrdiff_t mOnResetBAICallbacks = 0x1D8;  // ::std::vector<::std::function<void()>>, 24 байт
    constexpr ptrdiff_t mTags = 0x1F0;  // ::std::vector<::ItemTag>, 24 байт
    constexpr ptrdiff_t Size = 0x208;
}  // namespace Item

// ItemActor  [src/mc/world/actor/item/ItemActor.h]  расчётный размер 0xC8  ⚠ BASE_UNKNOWN
namespace ItemActor {
    constexpr ptrdiff_t mUseAdjustments = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mFirstRenderedYaw = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mItem = 0x8;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t mAge = 0xA0;  // int, 4 байт
    constexpr ptrdiff_t mPickupDelay = 0xA4;  // int, 4 байт
    constexpr ptrdiff_t mThrowTime = 0xA8;  // int, 4 байт
    constexpr ptrdiff_t mBobOffs = 0xAC;  // float, 4 байт
    constexpr ptrdiff_t mHealth = 0xB0;  // int, 4 байт
    constexpr ptrdiff_t mLifeTime = 0xB4;  // int, 4 байт
    constexpr ptrdiff_t mIsInItemFrame = 0xB8;  // bool, 1 байт
    constexpr ptrdiff_t mIsFromFishing = 0xB9;  // bool, 1 байт
    constexpr ptrdiff_t mRenderAdjustments = 0xBC;  // ::std::optional<::ItemActor::ItemRenderAdjustments>, 12 байт
    constexpr ptrdiff_t Size = 0xC8;
}  // namespace ItemActor

// ItemComponent  [src/mc/world/item/components/ItemComponent.h]  расчётный размер 0x8  ⚠ BASE_UNKNOWN
namespace ItemComponent {
    constexpr ptrdiff_t mOwner = 0x0;  // ::ComponentItem*, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ItemComponent

// ItemDescriptor  [src/mc/world/item/ItemDescriptor.h]  расчётный размер 0x18
namespace ItemDescriptor {
    constexpr ptrdiff_t mItem = 0x0;  // ::Item const*, 8 байт
    constexpr ptrdiff_t mAuxValue = 0x8;  // short, 2 байт
    constexpr ptrdiff_t mImpl = 0x10;  // ::std::unique_ptr<::ItemDescriptor::BaseDescriptor>, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ItemDescriptor

// ItemDescriptorCount  [src/mc/world/item/ItemDescriptorCount.h]  расчётный размер 0x2  ⚠ BASE_UNKNOWN
namespace ItemDescriptorCount {
    constexpr ptrdiff_t mStackSize = 0x0;  // ushort, 2 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace ItemDescriptorCount

// ItemInUseComponent  [src/mc/entity/components/ItemInUseComponent.h]  расчётный размер 0xC
namespace ItemInUseComponent {
    constexpr ptrdiff_t mItem = 0x0;  // ::WeakPtr<::Item const>, 8 байт
    constexpr ptrdiff_t mDuration = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace ItemInUseComponent

// ItemReleaseInventoryTransaction  [src/mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h]  расчётный размер 0x74  ⚠ BASE_UNKNOWN
namespace ItemReleaseInventoryTransaction {
    constexpr ptrdiff_t mActionType = 0x0;  // ::ItemReleaseInventoryTransaction::ActionType, 4 байт
    constexpr ptrdiff_t mSlot = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mItem = 0x8;  // ::NetworkItemStackDescriptor, 96 байт
    constexpr ptrdiff_t mFromPos = 0x68;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x74;
}  // namespace ItemReleaseInventoryTransaction

// ItemStack  [src/mc/world/item/ItemStack.h]  расчётный размер 0x18  ⚠ BASE_UNKNOWN
namespace ItemStack {
    constexpr ptrdiff_t mNetIdVariant = 0x0;  // ::ItemStackNetIdVariant, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ItemStack

// ItemStackBase  [src/mc/world/item/ItemStackBase.h]  расчётный размер 0x3
namespace ItemStackBase {
    constexpr ptrdiff_t mCompareAux = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mCompareRelevantUserData = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mCompareDamage = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x3;
}  // namespace ItemStackBase

// ItemStackNetIdVariant  [src/mc/world/inventory/network/ItemStackNetIdVariant.h]  расчётный размер 0x18
namespace ItemStackNetIdVariant {
    constexpr ptrdiff_t mVariant = 0x0;  // ::std::variant<::ItemStackNetId, ::ItemStackRequestId, ::ItemStackLegacyRequestId>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ItemStackNetIdVariant

// ItemStackNetManagerEnabledComponent  [src/mc/entity/components/ItemStackNetManagerEnabledComponent.h]  расчётный размер 0x4
namespace ItemStackNetManagerEnabledComponent {
    constexpr ptrdiff_t mManagerCounter = 0x0;  // ::std::atomic<int>, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ItemStackNetManagerEnabledComponent

// ItemStackRequestAction  [src/mc/world/inventory/network/ItemStackRequestAction.h]  расчётный размер 0x1
namespace ItemStackRequestAction {
    constexpr ptrdiff_t mActionType = 0x0;  // ::ItemStackRequestActionType, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace ItemStackRequestAction

// ItemStackRequestActionDrop  [src/mc/world/inventory/network/ItemStackRequestActionDrop.h]  расчётный размер 0x1  ⚠ BASE_UNKNOWN
namespace ItemStackRequestActionDrop {
    constexpr ptrdiff_t mRandomly = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace ItemStackRequestActionDrop

// ItemStackRequestActionTransferBase  [src/mc/world/inventory/network/ItemStackRequestActionTransferBase.h]  расчётный размер 0x58  ⚠ BASE_UNKNOWN
namespace ItemStackRequestActionTransferBase {
    constexpr ptrdiff_t mIsDstSerialized = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mIsAmountSerialized = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mAmount = 0x2;  // uchar, 1 байт
    constexpr ptrdiff_t mSrc = 0x8;  // ::ItemStackRequestSlotInfo, 40 байт
    constexpr ptrdiff_t mDst = 0x30;  // ::ItemStackRequestSlotInfo, 40 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace ItemStackRequestActionTransferBase

// ItemStackRequestBatch  [src/mc/world/inventory/network/ItemStackRequestBatch.h]  расчётный размер 0x18
namespace ItemStackRequestBatch {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::vector<::std::unique_ptr<::ItemStackRequestData>>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ItemStackRequestBatch

// ItemStackRequestData  [src/mc/world/inventory/network/ItemStackRequestData.h]  расчётный размер 0x48
namespace ItemStackRequestData {
    constexpr ptrdiff_t mClientRequestId = 0x0;  // ::ItemStackRequestId, 16 байт
    constexpr ptrdiff_t mStringsToFilter = 0x10;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t mStringsToFilterOrigin = 0x28;  // ::TextProcessingEventOrigin, 4 байт
    constexpr ptrdiff_t mActions = 0x30;  // ::std::vector<::std::unique_ptr<::ItemStackRequestAction>>, 24 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace ItemStackRequestData

// ItemStackRequestPacket  [src/mc/network/packet/ItemStackRequestPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ItemStackRequestPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ItemStackRequestPacket

// ItemUseInventoryTransaction  [src/mc/world/inventory/transaction/ItemUseInventoryTransaction.h]  расчётный размер 0xA2  ⚠ BASE_UNKNOWN
namespace ItemUseInventoryTransaction {
    constexpr ptrdiff_t mActionType = 0x0;  // ::ItemUseInventoryTransaction::ActionType, 4 байт
    constexpr ptrdiff_t mTriggerType = 0x4;  // ::ItemUseInventoryTransaction::TriggerType, 1 байт
    constexpr ptrdiff_t mPos = 0x8;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mTargetBlockId = 0x14;  // uint, 4 байт
    constexpr ptrdiff_t mFace = 0x18;  // uchar, 1 байт
    constexpr ptrdiff_t mSlot = 0x1C;  // int, 4 байт
    constexpr ptrdiff_t mHand = 0x20;  // ::HandSlot, 1 байт
    constexpr ptrdiff_t mItem = 0x28;  // ::NetworkItemStackDescriptor, 96 байт
    constexpr ptrdiff_t mFromPos = 0x88;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mClickPos = 0x94;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mClientPredictedResult = 0xA0;  // ::ItemUseInventoryTransaction::PredictedResult, 1 байт
    constexpr ptrdiff_t mClientCooldownState = 0xA1;  // ::ItemUseInventoryTransaction::ClientCooldownState, 1 байт
    constexpr ptrdiff_t Size = 0xA2;
}  // namespace ItemUseInventoryTransaction

// ItemUseOnActorInventoryTransaction  [src/mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h]  расчётный размер 0x88  ⚠ BASE_UNKNOWN
namespace ItemUseOnActorInventoryTransaction {
    constexpr ptrdiff_t mRuntimeId = 0x0;  // ::ActorRuntimeID, 8 байт
    constexpr ptrdiff_t mActionType = 0x8;  // ::ItemUseOnActorInventoryTransaction::ActionType, 4 байт
    constexpr ptrdiff_t mSlot = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mItem = 0x10;  // ::NetworkItemStackDescriptor, 96 байт
    constexpr ptrdiff_t mFromPos = 0x70;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mHitPos = 0x7C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x88;
}  // namespace ItemUseOnActorInventoryTransaction

// ItemUseSlowdownModifierComponent  [src/mc/entity/components/ItemUseSlowdownModifierComponent.h]  расчётный размер 0x4
namespace ItemUseSlowdownModifierComponent {
    constexpr ptrdiff_t mModifier = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ItemUseSlowdownModifierComponent

// JumpControlComponent  [src/mc/entity/components_json_legacy/JumpControlComponent.h]  расчётный размер 0x58
namespace JumpControlComponent {
    constexpr ptrdiff_t mJumping = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mSwimming = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mJumpPower = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mJumpType = 0x8;  // ::JumpType, 4 байт
    constexpr ptrdiff_t mJumpData = 0xC;  // ::JumpData[4], 64 байт
    constexpr ptrdiff_t mJumpControl = 0x50;  // ::std::unique_ptr<::JumpControl>, 8 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace JumpControlComponent

// JumpTicksComponent  [src/mc/entity/components/JumpTicksComponent.h]  расчётный размер 0x4
namespace JumpTicksComponent {
    constexpr ptrdiff_t mValue = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace JumpTicksComponent

// LeashableComponent  [src/mc/entity/components_json_legacy/LeashableComponent.h]  расчётный размер 0xF8
namespace LeashableComponent {
    constexpr ptrdiff_t mPresets = 0x0;  // ::std::vector<::LeashablePreset>, 24 байт
    constexpr ptrdiff_t mCanBeStolen = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mOnLeash = 0x20;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mOnUnleash = 0x88;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mOnUnleashInteractOnly = 0xF0;  // bool, 1 байт
    constexpr ptrdiff_t mAngularMomentum = 0xF4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0xF8;
}  // namespace LeashableComponent

// LegacyMolangVariableComponent  [src/mc/entity/components/LegacyMolangVariableComponent.h]  расчётный размер 0x4
namespace LegacyMolangVariableComponent {
    constexpr ptrdiff_t mVariableUpdateType = 0x0;  // ::LegacyMolangVariableComponent::VariableUpdateType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LegacyMolangVariableComponent

// LegacyTradeableComponent  [src/mc/entity/components_json_legacy/LegacyTradeableComponent.h]  расчётный размер 0x58
namespace LegacyTradeableComponent {
    constexpr ptrdiff_t mAddRecipeOnUpdate = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mResetLockedOnFirstTrade = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mWillingToBreed = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mRiches = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mTradeTier = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mUpdateMerchantTimer = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mLastPlayerTradeID = 0x10;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mOffers = 0x18;  // ::std::unique_ptr<::MerchantRecipeList>, 8 байт
    constexpr ptrdiff_t mDisplayName = 0x20;  // ::std::string, 32 байт
    constexpr ptrdiff_t mTradeRecipeFirstTime = 0x40;  // ::std::vector<int>, 24 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace LegacyTradeableComponent

// Level  [src/mc/world/level/Level.h]  расчётный размер 0x728  ⚠ BASE_UNKNOWN
namespace Level {
    constexpr ptrdiff_t mPacketSender = 0x0;  // ::Bedrock::NotNullNonOwnerPtr<::PacketSender> const, 24 байт
    constexpr ptrdiff_t mActorInfoRegistry = 0x18;  // ::std::shared_ptr<::ActorInfoRegistry>, 16 байт
    constexpr ptrdiff_t mListeners = 0x28;  // ::std::vector<::LevelListener*>, 24 байт
    constexpr ptrdiff_t mLevelStorageManager = 0x40;  // ::std::unique_ptr<::LevelStorageManager>, 8 байт
    constexpr ptrdiff_t mLevelData = 0x48;  // ::Bedrock::NotNullNonOwnerPtr<::LevelData> const, 24 байт
    constexpr ptrdiff_t mRegions = 0x60;  // ::std::unordered_set<::BlockSource*>, 64 байт
    constexpr ptrdiff_t mLinkedAssetValidator = 0xA0;  // ::Bedrock::NonOwnerPointer<::LinkedAssetValidator>, 24 байт
    constexpr ptrdiff_t mEntityDefinitions = 0xB8;  // ::std::unique_ptr<::ActorDefinitionGroup>, 8 байт
    constexpr ptrdiff_t mActorAnimationGroup = 0xC0;  // ::std::unique_ptr<::ActorAnimationGroup>, 8 байт
    constexpr ptrdiff_t mActorAnimationControllerGroup = 0xC8;  // ::std::unique_ptr<::ActorAnimationControllerGroup>, 8 байт
    constexpr ptrdiff_t mBlockDefinitions = 0xD0;  // ::std::unique_ptr<::BlockDefinitionGroup>, 8 байт
    constexpr ptrdiff_t mTrimPatternRegistry = 0xD8;  // ::std::shared_ptr<::TrimPatternRegistry>, 16 байт
    constexpr ptrdiff_t mTrimMaterialRegistry = 0xE8;  // ::std::shared_ptr<::TrimMaterialRegistry>, 16 байт
    constexpr ptrdiff_t mMobSpawner = 0xF8;  // ::std::unique_ptr<::Spawner>, 8 байт
    constexpr ptrdiff_t mProjectileFactory = 0x100;  // ::std::unique_ptr<::ProjectileFactory>, 8 байт
    constexpr ptrdiff_t mActorPropertyGroups = 0x108;  // ::std::unique_ptr<::PropertyGroupManager>, 8 байт
    constexpr ptrdiff_t mCameraPresets = 0x110;  // ::std::unique_ptr<::CameraPresets>, 8 байт
    constexpr ptrdiff_t mBehaviorFactory = 0x118;  // ::std::unique_ptr<::BehaviorFactory>, 8 байт
    constexpr ptrdiff_t mAutomationBehaviorTreeDefinitions = 0x120;  // ::std::unique_ptr<::AutomationBehaviorTreeGroup>, 8 байт
    constexpr ptrdiff_t mGlobalBlockPalette = 0x128;  // ::std::unique_ptr<::BlockPalette>, 8 байт
    constexpr ptrdiff_t mRecipes = 0x130;  // ::std::unique_ptr<::Recipes>, 8 байт
    constexpr ptrdiff_t mBlockReducer = 0x138;  // ::std::unique_ptr<::BlockReducer>, 8 байт
    constexpr ptrdiff_t mNpcDialogueStorage = 0x140;  // ::std::unique_ptr<::NpcDialogueStorage>, 8 байт
    constexpr ptrdiff_t mInternalComponentRegistry = 0x148;  // ::std::unique_ptr<::InternalComponentRegistry>, 8 байт
    constexpr ptrdiff_t mPortalForcer = 0x150;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PortalForcer>>, 16 байт
    constexpr ptrdiff_t mItemRegistry = 0x160;  // ::ItemRegistryRef const, 16 байт
    constexpr ptrdiff_t mBlockTypeRegistry = 0x170;  // ::Bedrock::NotNullNonOwnerPtr<::BlockTypeRegistry>, 24 байт
    constexpr ptrdiff_t mUnknownBlockTypeRegistry = 0x188;  // ::std::unique_ptr<::IUnknownBlockTypeRegistry>, 8 байт
    constexpr ptrdiff_t mBossEventSubscriptionManager = 0x190;  // ::std::unique_ptr<::BossEventSubscriptionManager>, 8 байт
    constexpr ptrdiff_t mContainerRegistry = 0x198;  // ::gsl::not_null<::OwnerPtr<::ContainerRegistry>> const, 16 байт
    constexpr ptrdiff_t mHitResultWrapper = 0x1A8;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::HitResultWrapper>>, 16 байт
    constexpr ptrdiff_t mImmersiveReaderString = 0x1B8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mLevelRandom = 0x1D8;  // ::gsl::not_null<::OwnerPtr<::ILevelRandom>>, 16 байт
    constexpr ptrdiff_t mNetEventCallback = 0x1E8;  // ::Bedrock::NonOwnerPointer<::NetEventCallback>, 24 байт
    constexpr ptrdiff_t mIsClientSide = 0x200;  // bool const, 1 байт
    constexpr ptrdiff_t mSubClientId = 0x201;  // ::SubClientId, 1 байт
    constexpr ptrdiff_t mIsExporting = 0x202;  // bool, 1 байт
    constexpr ptrdiff_t mDisablePlayerInteractions = 0x203;  // bool, 1 байт
    constexpr ptrdiff_t mSimPaused = 0x204;  // bool, 1 байт
    constexpr ptrdiff_t mMainThreadTaskGroup = 0x208;  // ::std::unique_ptr<::TaskGroup>, 8 байт
    constexpr ptrdiff_t mIOTaskGroup = 0x210;  // ::std::unique_ptr<::TaskGroup>, 8 байт
    constexpr ptrdiff_t mScheduler = 0x218;  // ::Scheduler&, 8 байт
    constexpr ptrdiff_t mLevelId = 0x220;  // ::std::string, 32 байт
    constexpr ptrdiff_t mTickingAreasMgr = 0x240;  // ::std::unique_ptr<::TickingAreasManager>, 8 байт
    constexpr ptrdiff_t mTempEPtrManager = 0x248;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::TempEPtrManager>>, 16 байт
    constexpr ptrdiff_t mPhotoItemSavedDataCollection = 0x258;  // ::std::unique_ptr<::PhotoItemSavedDataCollection>, 8 байт
    constexpr ptrdiff_t mTearingDown = 0x260;  // bool, 1 байт
    constexpr ptrdiff_t mEventing = 0x268;  // ::IMinecraftEventing&, 8 байт
    constexpr ptrdiff_t mEventCoordinatorManager = 0x270;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::EventCoordinatorManager>>, 16 байт
    constexpr ptrdiff_t mLevelSoundManager = 0x280;  // ::std::unique_ptr<::LevelSoundManager>, 8 байт
    constexpr ptrdiff_t mStructureManager = 0x288;  // ::Bedrock::NotNullNonOwnerPtr<::StructureManager>, 24 байт
    constexpr ptrdiff_t mLevelChunkPerformanceTelemetry = 0x2A0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::LevelChunkPerformanceTelemetry>>, 16 байт
    constexpr ptrdiff_t mLootTables = 0x2B0;  // ::std::unique_ptr<::LootTables>, 8 байт
    constexpr ptrdiff_t mRegisteredBorderBlock = 0x2B8;  // ::BlockType const*, 8 байт
    constexpr ptrdiff_t mJigsawStructureRegistry = 0x2C0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::JigsawStructureRegistry>>, 16 байт
    constexpr ptrdiff_t mStructureSpawnRegistry = 0x2D0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::StructureSpawnRegistry>>, 16 байт
    constexpr ptrdiff_t mDimensionFactoryAndManager = 0x2E0;  // ::gsl::not_null<::std::unique_ptr<::DimensionFactoryAndManager>>, 8 байт
    constexpr ptrdiff_t mLevelEntity = 0x2E8;  // ::WeakRef<::EntityContext>, 24 байт
    constexpr ptrdiff_t mEntityRegistryOwner = 0x300;  // ::Bedrock::NotNullNonOwnerPtr<::IEntityRegistryOwner>, 24 байт
    constexpr ptrdiff_t mPauseManager = 0x318;  // ::OwnerPtr<::PauseManager>, 16 байт
    constexpr ptrdiff_t mEntitySystemsManager = 0x328;  // ::gsl::not_null<::OwnerPtr<::EntitySystemsManager>>, 16 байт
    constexpr ptrdiff_t mFeatureRegistry = 0x338;  // ::std::unique_ptr<::FeatureRegistry>, 8 байт
    constexpr ptrdiff_t mFeatureTypeFactory = 0x340;  // ::std::unique_ptr<::FeatureTypeFactory>, 8 байт
    constexpr ptrdiff_t mLightTextureImageBuilderFactory = 0x348;  // ::Factory<::BaseLightTextureImageBuilder, ::Level&, ::Scheduler&>, 64 байт
    constexpr ptrdiff_t mEducationSettingsManager = 0x388;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::EducationSettingsManager>>, 16 байт
    constexpr ptrdiff_t mCapabilities = 0x398;  // ::std::unique_ptr<::PlayerCapabilities::ISharedController>, 8 байт
    constexpr ptrdiff_t mBlockActorLevelListener = 0x3A0;  // ::std::unique_ptr<::BlockActorLevelListener>, 8 байт
    constexpr ptrdiff_t mVanillaActorEventListenerManager = 0x3A8;  // ::std::unique_ptr<::VanillaActorEventListenerManager>, 8 байт
    constexpr ptrdiff_t mPositionTrackerDBClient = 0x3B0;  // ::std::unique_ptr<::PositionTrackingDB::PositionTrackingDBClient>, 8 байт
    constexpr ptrdiff_t mClientSideChunkGenEnabled = 0x3B8;  // bool, 1 байт
    constexpr ptrdiff_t mBlockNetworkIdsAreHashes = 0x3B9;  // bool, 1 байт
    constexpr ptrdiff_t mBiomeManager = 0x3C0;  // ::std::unique_ptr<::BiomeManager>, 8 байт
    constexpr ptrdiff_t mActorGarbageCollector = 0x3C8;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorGarbageCollector>>, 16 байт
    constexpr ptrdiff_t mActorManager = 0x3D8;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorManager>>, 16 байт
    constexpr ptrdiff_t mOnRemoveActorEntityReferences = 0x3E8;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mAutonomousActorManager = 0x3F8;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::AutonomousActorManager>>, 16 байт
    constexpr ptrdiff_t mActorFetcher = 0x408;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorFetcher>>, 16 байт
    constexpr ptrdiff_t mGameplayUserManager = 0x418;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::GameplayUserManager>>, 16 байт
    constexpr ptrdiff_t mOnGameplayUserAddedSubscription = 0x428;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnGameplayUserRemovedSubscription = 0x438;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnAnyGameplayUsersRemovedSubscription = 0x448;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mScoreboard = 0x458;  // ::std::unique_ptr<::Scoreboard>, 8 байт
    constexpr ptrdiff_t mMolangPackSettingsCache = 0x460;  // ::std::shared_ptr<::MolangPackSettingsCache>, 16 байт
    constexpr ptrdiff_t mActorRuntimeIDManager = 0x470;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorRuntimeIDManager>>, 16 байт
    constexpr ptrdiff_t mPlayerLocationReceiver = 0x480;  // ::Bedrock::UniqueOwnerPointer<::PlayerLocationReceiver>, 16 байт
    constexpr ptrdiff_t mPlayerLocationSender = 0x490;  // ::Bedrock::UniqueOwnerPointer<::PlayerLocationSender>, 16 байт
    constexpr ptrdiff_t mPlayerListManager = 0x4A0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerListManager>>, 16 байт
    constexpr ptrdiff_t mPlayerTickManager = 0x4B0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerTickManager>>, 16 байт
    constexpr ptrdiff_t mServerParticleManager = 0x4C0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ServerParticleManager>>, 16 байт
    constexpr ptrdiff_t mParticleProvider = 0x4D0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ParticleProvider>>, 16 байт
    constexpr ptrdiff_t mOnSendServerLegacyParticleSubscription = 0x4E0;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnAddTerrainParticleEffectSubscription = 0x4F0;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnAddTerrainSlideEffectSubscription = 0x500;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnAddBreakingItemParticleEffectSubscription = 0x510;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnAddBiomeTintedParticleEffectSubscription = 0x520;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mChunkTickRangeManager = 0x530;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ChunkTickRangeManager>>, 16 байт
    constexpr ptrdiff_t mChunkViewTrackerManager = 0x540;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ChunkViewTrackerManager>>, 16 байт
    constexpr ptrdiff_t mActorEventBroadcaster = 0x550;  // ::Bedrock::UniqueOwnerPointer<::ActorEventBroadcaster> const, 16 байт
    constexpr ptrdiff_t mCerealContext = 0x560;  // ::gsl::not_null<::OwnerPtr<::cereal::ReflectionCtx>>, 16 байт
    constexpr ptrdiff_t mActorFactory = 0x570;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorFactory>>, 16 байт
    constexpr ptrdiff_t mLoadingScreenIdManager = 0x580;  // ::Bedrock::UniqueOwnerPointer<::LoadingScreenIdManager>, 16 байт
    constexpr ptrdiff_t mPlayerDimensionTransferManager = 0x590;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerDimensionTransferManager>>, 16 байт
    constexpr ptrdiff_t mLevelCrashDumpManager = 0x5A0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::LevelCrashDumpManager>> const, 16 байт
    constexpr ptrdiff_t mUniqueIDManager = 0x5B0;  // ::gsl::not_null<::OwnerPtr<::UniqueIDManager>>, 16 байт
    constexpr ptrdiff_t mOnPictureTakenSubscription = 0x5C0;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mPhotoManager = 0x5D0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PhotoManager>>, 16 байт
    constexpr ptrdiff_t mLevelEventManager = 0x5E0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::LevelEventManager>>, 16 байт
    constexpr ptrdiff_t mLevelEventDataSubscription = 0x5F0;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mLevelEventCompoundTagSubscription = 0x600;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mChunkTickOffsetManager = 0x610;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ChunkTickOffsetManager>>, 16 байт
    constexpr ptrdiff_t mPlayerMovementSettingsManager = 0x620;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerMovementSettingsManager>>, 16 байт
    constexpr ptrdiff_t mPlayerAbilitiesManager = 0x630;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerAbilitiesManager>>, 16 байт
    constexpr ptrdiff_t mPlayerPermissionsManager = 0x640;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerPermissionsManager>>, 16 байт
    constexpr ptrdiff_t mPlayerPermissionsSynchroniser = 0x650;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::PlayerPermissionsSynchroniser>>, 16 байт
    constexpr ptrdiff_t mLevelChunkEventManager = 0x660;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::LevelChunkEventManager>>, 16 байт
    constexpr ptrdiff_t mOnChunkLoadedSubscription = 0x670;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnChunkReloadedSubscription = 0x680;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnChunkDiscardedSubscription = 0x690;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mActorDimensionTransferManager = 0x6A0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::ActorDimensionTransferManager>>, 16 байт
    constexpr ptrdiff_t mLevelBlockDestroyer = 0x6B0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::LevelBlockDestroyer>>, 16 байт
    constexpr ptrdiff_t mTickDeltaTimeManager = 0x6C0;  // ::gsl::not_null<::Bedrock::UniqueOwnerPointer<::TickDeltaTimeManager>>, 16 байт
    constexpr ptrdiff_t mLevelChunkSaveManager = 0x6D0;  // ::OwnerPtr<::LevelChunkSaveManager>, 16 байт
    constexpr ptrdiff_t mWeatherManager = 0x6E0;  // ::gsl::not_null<::OwnerPtr<::WeatherManager>>, 16 байт
    constexpr ptrdiff_t mHandleLowMemoryOnTick = 0x6F0;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mLowMemorySubscription = 0x6F8;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mShapeRegistry = 0x708;  // ::std::shared_ptr<::VoxelShapes::VoxelShapeRegistry>, 16 байт
    constexpr ptrdiff_t mDensityFunctionRegistry = 0x718;  // ::gsl::not_null<::OwnerPtr<::DensityFunctionRegistry>>, 16 байт
    constexpr ptrdiff_t Size = 0x728;
}  // namespace Level

// LevelChunk  [src/mc/world/level/chunk/LevelChunk.h]  расчётный размер 0x18D2
namespace LevelChunk {
    constexpr ptrdiff_t mNeighbors = 0x0;  // uchar, 1 байт
    constexpr ptrdiff_t aabb = 0x4;  // ::BoundingBox, 24 байт
    constexpr ptrdiff_t type = 0x1C;  // ::HardcodedSpawnAreaType, 1 байт
    constexpr ptrdiff_t mWasStored = 0x1D;  // bool, 1 байт
    constexpr ptrdiff_t mWasGenerated = 0x1E;  // bool, 1 байт
    constexpr ptrdiff_t mWasRequestedInsideTickRange = 0x1F;  // bool, 1 байт
    constexpr ptrdiff_t mWasLoadedInsideTickRange = 0x20;  // bool, 1 байт
    constexpr ptrdiff_t mBlockEntityAccessLock = 0x28;  // ::std::mutex, 80 байт
    constexpr ptrdiff_t mLevel = 0x78;  // ::ILevel&, 8 байт
    constexpr ptrdiff_t mDimension = 0x80;  // ::Dimension&, 8 байт
    constexpr ptrdiff_t mMin = 0x88;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mMax = 0x94;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mPosition = 0xA0;  // ::ChunkPos, 8 байт
    constexpr ptrdiff_t mLightingFixupDone = 0xA8;  // bool, 1 байт
    constexpr ptrdiff_t mLightingTaskActive = 0xA9;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mReadOnly = 0xAA;  // bool, 1 байт
    constexpr ptrdiff_t mGenerator = 0xB0;  // ::ChunkSource*, 8 байт
    constexpr ptrdiff_t mTmpTickingEntity = 0xB8;  // ::LevelChunkTicking::Entity, 4 байт
    constexpr ptrdiff_t mLoadedFormat = 0xBC;  // ::std::optional<::LevelChunkFormat>, 2 байт
    constexpr ptrdiff_t mHadSerializedEntities = 0xBE;  // bool, 1 байт
    constexpr ptrdiff_t mSerializedEntitiesBuffer = 0xC0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mFailedSerializedEntitiesBuffer = 0xE0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mUnresolvedActorLinks = 0x100;  // ::std::vector<::ActorLink>, 24 байт
    constexpr ptrdiff_t mLoadState = 0x118;  // ::std::atomic<::ChunkState>, 1 байт
    constexpr ptrdiff_t mIsCurrentLoadStateIndeterminate = 0x119;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mTerrainDataState = 0x11A;  // ::ChunkTerrainDataState, 1 байт
    constexpr ptrdiff_t mDebugDisplaySavedState = 0x11B;  // ::ChunkDebugDisplaySavedState, 1 байт
    constexpr ptrdiff_t mCachedDataState = 0x11C;  // ::ChunkCachedDataState, 1 байт
    constexpr ptrdiff_t mCachedDataStateSpinLock = 0x120;  // ::SpinLockImpl, 24 байт
    constexpr ptrdiff_t mClientRequestHeightmapAdjustSpinLock = 0x138;  // ::SpinLockImpl, 24 байт
    constexpr ptrdiff_t mLastTick = 0x150;  // ::Tick, 8 байт
    constexpr ptrdiff_t mTickQueue = 0x158;  // ::std::unique_ptr<::BlockTickingQueue>, 8 байт
    constexpr ptrdiff_t mRandomTickQueue = 0x160;  // ::std::unique_ptr<::BlockTickingQueue>, 8 байт
    constexpr ptrdiff_t mSubChunks = 0x168;  // ::std::vector<::SubChunk>, 24 байт
    constexpr ptrdiff_t mBiomes = 0x180;  // ::LevelChunkBiomes, 552 байт
    constexpr ptrdiff_t mCachedData = 0x3A8;  // ::std::array<::ColumnCachedData, 256>, 2048 байт
    constexpr ptrdiff_t mHeightmap = 0xBA8;  // ::std::array<::ChunkLocalHeight, 256>, 512 байт
    constexpr ptrdiff_t mRenderHeightmap = 0xDA8;  // ::std::array<::ChunkLocalHeight, 256>, 512 байт
    constexpr ptrdiff_t mPreWorldGenHeightmap = 0xFA8;  // ::std::unique_ptr<::std::vector<short>>, 8 байт
    constexpr ptrdiff_t mNonAirMaxHeight = 0xFB0;  // ::ChunkLocalHeight, 2 байт
    constexpr ptrdiff_t mBiomeStates = 0xFB8;  // ::std::unordered_map<::BiomeIdType, ::BiomeChunkState>, 64 байт
    constexpr ptrdiff_t mHasCachedTemperatureNoise = 0xFF8;  // bool, 1 байт
    constexpr ptrdiff_t mBorderBlockMap = 0xFF9;  // ::std::array<bool, 256>, 256 байт
    constexpr ptrdiff_t mIsRedstoneLoaded = 0x10F9;  // bool, 1 байт
    constexpr ptrdiff_t mOwnedByTickingThread = 0x10FA;  // bool, 1 байт
    constexpr ptrdiff_t mUse3DBiomeMaps = 0x10FB;  // bool, 1 байт
    constexpr ptrdiff_t mLevelChunkHas3dBiomeTag = 0x10FC;  // bool, 1 байт
    constexpr ptrdiff_t mCurrentInstatick = 0x1100;  // int, 4 байт
    constexpr ptrdiff_t mFinalized = 0x1104;  // ::LevelChunk::Finalization, 4 байт
    constexpr ptrdiff_t mTelemetry = 0x1108;  // ::LevelChunk::Telemetry, 4 байт
    constexpr ptrdiff_t mChunkGenContext = 0x1110;  // ::ChunkGenContext, 64 байт
    constexpr ptrdiff_t mFullChunkDirtyTicksCounters = 0x1150;  // ::DirtyTicksCounter[6], 48 байт
    constexpr ptrdiff_t mChunkActorsDirtyTicksCounter = 0x1180;  // ::DirtyTicksCounter, 8 байт
    constexpr ptrdiff_t mRainHeights = 0x1188;  // ::std::array<::ChunkLocalHeight, 256>, 512 байт
    constexpr ptrdiff_t mEntities = 0x1388;  // ::std::vector<::WeakEntityRef>, 24 байт
    constexpr ptrdiff_t mRemovedActorStorageKeys = 0x13A0;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t mBlockEntities = 0x13B8;  // ::LevelChunkBlockActorStorage, 184 байт
    constexpr ptrdiff_t mPreservedBlockEntities = 0x1470;  // ::std::vector<::std::shared_ptr<::BlockActor>>, 24 байт
    constexpr ptrdiff_t mPreservedBlockEntitiesTick = 0x1488;  // ::Tick, 8 байт
    constexpr ptrdiff_t mVolumes = 0x1490;  // ::std::vector<::WeakRef<::EntityContext>>, 24 байт
    constexpr ptrdiff_t mDefaultBrightness = 0x14A8;  // ::BrightnessPair, 2 байт
    constexpr ptrdiff_t mSpawningAreas = 0x14B0;  // ::std::vector<::LevelChunk::SpawningArea>, 24 байт
    constexpr ptrdiff_t mLastNetworkLightingTime = 0x14C8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t mLoadedMetaDataHash = 0x14D0;  // uint64, 8 байт
    constexpr ptrdiff_t mMetaData = 0x14D8;  // ::std::shared_ptr<::LevelChunkMetaData>, 16 байт
    constexpr ptrdiff_t mChunkInterpolants = 0x14E8;  // float[2][2], 16 байт
    constexpr ptrdiff_t mGenerationSeed = 0x14F8;  // ::std::optional<uint>, 8 байт
    constexpr ptrdiff_t mbChunkHasConverterTag = 0x1500;  // bool, 1 байт
    constexpr ptrdiff_t mSavedNeighbors = 0x1501;  // ::LevelChunk::Neighbors, 1 байт
    constexpr ptrdiff_t mClientNeedsToRequestSubChunks = 0x1502;  // bool, 1 байт
    constexpr ptrdiff_t mLevelChunkHasHadOriginalLighting = 0x1503;  // bool, 1 байт
    constexpr ptrdiff_t mGeneratedPreCavesAndCliffsBlending = 0x1504;  // bool, 1 байт
    constexpr ptrdiff_t mOnChunkLoadedCalled = 0x1505;  // bool, 1 байт
    constexpr ptrdiff_t mAllSerializedEntitiesLoaded = 0x1506;  // bool, 1 байт
    constexpr ptrdiff_t mOutOfBoundsBlockEntitiesBuffer = 0x1508;  // ::std::string, 32 байт
    constexpr ptrdiff_t mSaveNonActorDataIfDirty = 0x1528;  // bool, 1 байт
    constexpr ptrdiff_t mSaveIfNeverSaved = 0x1529;  // bool, 1 байт
    constexpr ptrdiff_t mHasSubChunksToPrune = 0x152A;  // bool, 1 байт
    constexpr ptrdiff_t mHaveSubChunksBeenPruned = 0x152B;  // bool, 1 байт
    constexpr ptrdiff_t mShouldShiftUpFlatWorldSubChunks = 0x152C;  // bool, 1 байт
    constexpr ptrdiff_t mSubChunksToPruneOutsideRange = 0x1530;  // ::std::unique_ptr<::std::vector<::SubChunk>>, 8 байт
    constexpr ptrdiff_t mChunkTimings = 0x1538;  // ::gsl::not_null<::std::unique_ptr<::Bedrock::LevelChunkTimings>>, 8 байт
    constexpr ptrdiff_t mGameEventListenerRegistry = 0x1540;  // ::std::shared_ptr<::GameEventListenerRegistry>, 16 байт
    constexpr ptrdiff_t mSnowRandomValue = 0x1550;  // ::std::atomic<int>, 4 байт
    constexpr ptrdiff_t mIsClientGenerated = 0x1554;  // bool, 1 байт
    constexpr ptrdiff_t mWasClientGenerated = 0x1555;  // bool, 1 байт
    constexpr ptrdiff_t mIsEmptyClientChunk = 0x1556;  // bool, 1 байт
    constexpr ptrdiff_t mActorStorageUpdatedAfterCheckingForReplacementData = 0x1557;  // bool, 1 байт
    constexpr ptrdiff_t mIsTransient = 0x1558;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mDidSaveJigsawStructureBlueprints = 0x1559;  // bool, 1 байт
    constexpr ptrdiff_t mLevelChunkVolumeData = 0x1560;  // ::LevelChunkVolumeData, 856 байт
    constexpr ptrdiff_t mMoveLockMutex = 0x18B8;  // ::SpinLockImpl, 24 байт
    constexpr ptrdiff_t mIsBeingMoved = 0x18D0;  // ::std::atomic<bool>, 1 байт
    constexpr ptrdiff_t mChunkCreationMode = 0x18D1;  // ::std::atomic<::LevelChunkCreationMode>, 1 байт
    constexpr ptrdiff_t Size = 0x18D2;
}  // namespace LevelChunk

// LevelComponent  [src/mc/entity/components/LevelComponent.h]  расчётный размер 0x8
namespace LevelComponent {
    constexpr ptrdiff_t mLevel = 0x0;  // ::std::unique_ptr<::ILevel>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace LevelComponent

// LevelData  [src/mc/world/level/storage/LevelData.h]  расчётный размер 0x654
namespace LevelData {
    constexpr ptrdiff_t mAdventureSettings = 0x0;  // ::AdventureSettings, 5 байт
    constexpr ptrdiff_t mWorldTemplateLevelData = 0x8;  // ::WorldTemplateLevelData, 144 байт
    constexpr ptrdiff_t mGameRules = 0x98;  // ::GameRules, 192 байт
    constexpr ptrdiff_t mExperiments = 0x158;  // ::ExperimentStorage, 72 байт
    constexpr ptrdiff_t mDefaultAbilities = 0x1A0;  // ::Abilities, 240 байт
    constexpr ptrdiff_t mDefaultPermissions = 0x290;  // ::PermissionsHandler, 24 байт
    constexpr ptrdiff_t mLevelName = 0x2A8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mStorageVersion = 0x2C8;  // ::StorageVersion, 4 байт
    constexpr ptrdiff_t mMinCompatibleClientVersion = 0x2D0;  // ::GameVersion, 56 байт
    constexpr ptrdiff_t mNetworkVersion = 0x308;  // int, 4 байт
    constexpr ptrdiff_t mInventoryVersion = 0x310;  // ::SemVersion, 24 байт
    constexpr ptrdiff_t mCurrentTick = 0x328;  // ::Tick, 8 байт
    constexpr ptrdiff_t mHasSpawnPos = 0x330;  // bool, 1 байт
    constexpr ptrdiff_t mLimitedWorldOrigin = 0x334;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mOverworldTimeSetting = 0x340;  // int, 4 байт
    constexpr ptrdiff_t mWorldClockRegistry = 0x348;  // ::Bedrock::NonOwnerPointer<::WorldClockRegistry const>, 24 байт
    constexpr ptrdiff_t mLastSaved = 0x360;  // int64, 8 байт
    constexpr ptrdiff_t mServerTickRange = 0x368;  // uint, 4 байт
    constexpr ptrdiff_t mRainLevel = 0x36C;  // float, 4 байт
    constexpr ptrdiff_t mRainTime = 0x370;  // int, 4 байт
    constexpr ptrdiff_t mLightningLevel = 0x374;  // float, 4 байт
    constexpr ptrdiff_t mLightningTime = 0x378;  // int, 4 байт
    constexpr ptrdiff_t mNetherScale = 0x37C;  // int, 4 байт
    constexpr ptrdiff_t mLastOpenedWithVersion = 0x380;  // ::GameVersion, 56 байт
    constexpr ptrdiff_t mGameDifficulty = 0x3B8;  // ::SharedTypes::Legacy::Difficulty, 4 байт
    constexpr ptrdiff_t mForceGameType = 0x3BC;  // bool, 1 байт
    constexpr ptrdiff_t mIsHardcore = 0x3BD;  // bool, 1 байт
    constexpr ptrdiff_t mPlayerHasDied = 0x3BE;  // bool, 1 байт
    constexpr ptrdiff_t mSpawnMobs = 0x3BF;  // bool, 1 байт
    constexpr ptrdiff_t mAdventureModeOverridesEnabled = 0x3C0;  // bool, 1 байт
    constexpr ptrdiff_t mFlatWorldOptions = 0x3C8;  // ::Json::Value, 16 байт
    constexpr ptrdiff_t mWorldStartCount = 0x3D8;  // uint, 4 байт
    constexpr ptrdiff_t mAchievementsDisabled = 0x3DC;  // bool, 1 байт
    constexpr ptrdiff_t mEditorWorldType = 0x3E0;  // ::Editor::WorldType, 4 байт
    constexpr ptrdiff_t mAllowAnonymousBlockDropsInEditorWorlds = 0x3E4;  // bool, 1 байт
    constexpr ptrdiff_t mServerEditorConnectionPolicy = 0x3E8;  // ::ServerEditorConnectionPolicy, 4 байт
    constexpr ptrdiff_t mIsCreatedInEditor = 0x3EC;  // bool, 1 байт
    constexpr ptrdiff_t mIsExportedFromEditor = 0x3ED;  // bool, 1 байт
    constexpr ptrdiff_t mEducationEditionOffer = 0x3F0;  // ::EducationEditionOfferValue, 4 байт
    constexpr ptrdiff_t mCloudSaveInfo = 0x3F8;  // ::std::optional<::CloudSaveLevelInfo>, 176 байт
    constexpr ptrdiff_t mEducationFeaturesEnabled = 0x4A8;  // bool, 1 байт
    constexpr ptrdiff_t mIsSingleUseWorld = 0x4A9;  // bool, 1 байт
    constexpr ptrdiff_t mConfirmedPlatformLockedContent = 0x4AA;  // bool, 1 байт
    constexpr ptrdiff_t mMultiplayerGameIntent = 0x4AB;  // bool, 1 байт
    constexpr ptrdiff_t mMultiplayerGame = 0x4AC;  // bool, 1 байт
    constexpr ptrdiff_t mLANBroadcastIntent = 0x4AD;  // bool, 1 байт
    constexpr ptrdiff_t mLANBroadcast = 0x4AE;  // bool, 1 байт
    constexpr ptrdiff_t mXBLBroadcastIntent = 0x4B0;  // ::Social::GamePublishSetting, 4 байт
    constexpr ptrdiff_t mXBLBroadcastMode = 0x4B4;  // ::Social::GamePublishSetting, 4 байт
    constexpr ptrdiff_t mPlatformBroadcastIntent = 0x4B8;  // ::Social::GamePublishSetting, 4 байт
    constexpr ptrdiff_t mPlatformBroadcastMode = 0x4BC;  // ::Social::GamePublishSetting, 4 байт
    constexpr ptrdiff_t mCheatsEnabled = 0x4C0;  // bool, 1 байт
    constexpr ptrdiff_t mCommandsEnabled = 0x4C1;  // bool, 1 байт
    constexpr ptrdiff_t mTexturePacksRequired = 0x4C2;  // bool, 1 байт
    constexpr ptrdiff_t mHasLockedBehaviorPack = 0x4C3;  // bool, 1 байт
    constexpr ptrdiff_t mHasLockedResourcePack = 0x4C4;  // bool, 1 байт
    constexpr ptrdiff_t mIsFromLockedTemplate = 0x4C5;  // bool, 1 байт
    constexpr ptrdiff_t mIsRandomSeedAllowed = 0x4C6;  // bool, 1 байт
    constexpr ptrdiff_t mEducationProductId = 0x4C8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mEducationCreatorId = 0x4E8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mEducationCreatorWorldId = 0x508;  // ::std::string, 32 байт
    constexpr ptrdiff_t mEducationReferrerId = 0x528;  // ::std::string, 32 байт
    constexpr ptrdiff_t mUseMsaGamertagsOnly = 0x548;  // bool, 1 байт
    constexpr ptrdiff_t mBonusChestEnabled = 0x549;  // bool, 1 байт
    constexpr ptrdiff_t mBonusChestSpawned = 0x54A;  // bool, 1 байт
    constexpr ptrdiff_t mStartWithMapEnabled = 0x54B;  // bool, 1 байт
    constexpr ptrdiff_t mMapsCenteredToOrigin = 0x54C;  // bool, 1 байт
    constexpr ptrdiff_t mRequiresCopiedPackRemovalCheck = 0x54D;  // bool, 1 байт
    constexpr ptrdiff_t mSpawnV1Villagers = 0x54E;  // bool, 1 байт
    constexpr ptrdiff_t mPersonaDisabled = 0x54F;  // bool, 1 байт
    constexpr ptrdiff_t mCustomSkinsDisabled = 0x550;  // bool, 1 байт
    constexpr ptrdiff_t mEmoteChatMuted = 0x551;  // bool, 1 байт
    constexpr ptrdiff_t mHasUncompleteWorldFileOnDisk = 0x552;  // bool, 1 байт
    constexpr ptrdiff_t mNetherType = 0x553;  // ::NetherWorldType, 1 байт
    constexpr ptrdiff_t mSpawnSettings = 0x558;  // ::SpawnSettings, 48 байт
    constexpr ptrdiff_t mValues = 0x588;  // ::std::unordered_map<::HashedString, ::LevelDataValue>, 64 байт
    constexpr ptrdiff_t mOverrides = 0x5C8;  // ::std::unordered_map<::HashedString, ::LevelDataValue>, 64 байт
    constexpr ptrdiff_t mBiomeOverride = 0x608;  // ::std::string, 32 байт
    constexpr ptrdiff_t mDataDrivenGeneratorType = 0x628;  // ::std::optional<::GeneratorType>, 8 байт
    constexpr ptrdiff_t mChatRestrictionLevel = 0x630;  // ::ChatRestrictionLevel, 1 байт
    constexpr ptrdiff_t mIsHardcoreSubscribers = 0x638;  // ::std::unique_ptr<::Bedrock::PubSub::Publisher<void(bool), ::Bedrock::PubSub::ThreadModel::SingleThreaded, 0>>, 8 байт
    constexpr ptrdiff_t mOnSaveLevelData = 0x640;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mLevelDataType = 0x650;  // ::LevelDataType, 4 байт
    constexpr ptrdiff_t Size = 0x654;
}  // namespace LevelData

// LevelEventPacket  [src/mc/network/packet/LevelEventPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace LevelEventPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LevelEventPacket

// LevelRenderer  [src-client/mc/client/renderer/game/LevelRenderer.h]  расчётный размер 0xA60  ⚠ BASE_UNKNOWN
namespace LevelRenderer {
    constexpr ptrdiff_t mRenderChunkCoordinators = 0x0;  // ::std::unordered_map<::DimensionType, ::OwnerPtr<::RenderChunkCoordinator>>, 64 байт
    constexpr ptrdiff_t mPointLightCoordinator = 0x40;  // ::std::unique_ptr<::PointLighting::PointLightCoordinator>, 8 байт
    constexpr ptrdiff_t mPointLightShadowProbeManager = 0x48;  // ::std::shared_ptr<::PointLighting::PointLightShadowProbeManager>, 16 байт
    constexpr ptrdiff_t mLightVolumeManager = 0x58;  // ::std::shared_ptr<::LightPropagation::LightVolumeManager>, 16 байт
    constexpr ptrdiff_t mLightPropagationCoordinator = 0x68;  // ::std::shared_ptr<::LightPropagation::LightPropagationCoordinator>, 16 байт
    constexpr ptrdiff_t mLightVolumeOptionSubscription = 0x78;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mTextureGroup = 0x88;  // ::std::shared_ptr<::mce::TextureGroup>, 16 байт
    constexpr ptrdiff_t mShadowCylinder = 0x98;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mShadowOverlayCube = 0xA8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mShadowDisc = 0xB8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mSkyMesh = 0xC8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mStarsMesh = 0xD8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mEndSkyMesh = 0xE8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mSunMesh = 0xF8;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mMoonMesh = 0x108;  // ::std::shared_ptr<::mce::Mesh>[8], 128 байт
    constexpr ptrdiff_t mNameplateBackgroundMesh = 0x188;  // ::std::shared_ptr<::mce::Mesh>, 16 байт
    constexpr ptrdiff_t mAtlasTexture = 0x198;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mBrightnessTex = 0x1B8;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mSunTex = 0x1D8;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mSunTexVibrantVisuals = 0x1F8;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mMoonTex = 0x218;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mEndSkyTex = 0x238;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mEndFlashTex = 0x258;  // ::mce::TexturePtr, 32 байт
    constexpr ptrdiff_t mCrackFrames = 0x278;  // ::std::vector<::mce::TexturePtr>, 24 байт
    constexpr ptrdiff_t mDebugRenderer = 0x290;  // ::DebugRenderer, 1 байт
    constexpr ptrdiff_t mScriptDebugRenderer = 0x298;  // ::Scripting::RenderHelper::Renderer, 192 байт
    constexpr ptrdiff_t mTicks = 0x358;  // ::Tick, 8 байт
    constexpr ptrdiff_t mBlockEntityRenderDispatcher = 0x360;  // ::BlockActorRenderDispatcher&, 8 байт
    constexpr ptrdiff_t mLocalRenderer = 0x368;  // ::std::unique_ptr<::BlockTessellator>, 8 байт
    constexpr ptrdiff_t mParticleEngine = 0x370;  // ::Bedrock::NonOwnerPointer<::ParticleEngine>, 24 байт
    constexpr ptrdiff_t mParticleSystemEngine = 0x388;  // ::Bedrock::NonOwnerPointer<::ParticleSystemEngine>, 24 байт
    constexpr ptrdiff_t mGeometryEditorGui = 0x3A0;  // ::std::shared_ptr<::GeometryEditorGui>, 16 байт
    constexpr ptrdiff_t mTaskGroup = 0x3B0;  // ::std::unique_ptr<::TaskGroup>, 8 байт
    constexpr ptrdiff_t mSyncTaskGroup = 0x3B8;  // ::std::unique_ptr<::TaskGroup>, 8 байт
    constexpr ptrdiff_t mGameRenderer = 0x3C0;  // ::GameRenderer&, 8 байт
    constexpr ptrdiff_t mLevel = 0x3C8;  // ::ClientLevel&, 8 байт
    constexpr ptrdiff_t mOptions = 0x3D0;  // ::std::shared_ptr<::OptionRegistry>, 16 байт
    constexpr ptrdiff_t mClientInstance = 0x3E0;  // ::IClientInstance&, 8 байт
    constexpr ptrdiff_t mLocalUser = 0x3E8;  // ::WeakEntityRef, 24 байт
    constexpr ptrdiff_t mReleaseParticleSystemInterfaceProxyCallback = 0x400;  // ::std::function<void()>, 64 байт
    constexpr ptrdiff_t mLevelRendererPlayer = 0x440;  // ::std::shared_ptr<::LevelRendererPlayer>, 16 байт
    constexpr ptrdiff_t mLevelRendererShadowCamera = 0x450;  // ::std::shared_ptr<::LevelRendererShadowCamera>, 16 байт
    constexpr ptrdiff_t mLevelRendererCloudShadowCamera = 0x460;  // ::std::shared_ptr<::LevelRendererShadowCamera>, 16 байт
    constexpr ptrdiff_t mShadowCascadeState = 0x470;  // ::ShadowCascadeState, 4 байт
    constexpr ptrdiff_t mTextureShiftsEnabled = 0x474;  // bool, 1 байт
    constexpr ptrdiff_t mLevelBuilder = 0x478;  // ::LevelBuilder, 1456 байт
    constexpr ptrdiff_t mProxy = 0xA28;  // ::std::unique_ptr<::LevelRendererProxy>, 8 байт
    constexpr ptrdiff_t mClearBufferColor = 0xA30;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mPlayerView = 0xA40;  // ::std::weak_ptr<::PlayerRenderView>, 16 байт
    constexpr ptrdiff_t mDataDrivenRendererV2RequiredData = 0xA50;  // ::std::unique_ptr<::DataDrivenRendererV2RequiredData>, 8 байт
    constexpr ptrdiff_t mUniversalEntityRenderer = 0xA58;  // ::std::unique_ptr<::UniversalEntityRenderer>, 8 байт
    constexpr ptrdiff_t Size = 0xA60;
}  // namespace LevelRenderer

// LevelRendererPlayer  [src-client/mc/client/renderer/game/LevelRendererPlayer.h]  расчётный размер 0x13B0  ⚠ BASE_UNKNOWN
namespace LevelRendererPlayer {
    constexpr ptrdiff_t mEffect = 0x0;  // ::HashedString, 48 байт
    constexpr ptrdiff_t mColor = 0x30;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mStartColor = 0x40;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mTargetColor = 0x50;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mCurrentColor = 0x60;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mTime = 0x70;  // float, 4 байт
    constexpr ptrdiff_t WATER_VISION_QUICK_TIME = 0x74;  // float const, 4 байт
    constexpr ptrdiff_t WATER_VISION_MAX_TIME = 0x78;  // float const, 4 байт
    constexpr ptrdiff_t WATER_VISION_QUICK_PERCENT = 0x7C;  // float const, 4 байт
    constexpr ptrdiff_t mCloudsImage = 0x80;  // ::std::weak_ptr<::cg::ImageBuffer>, 16 байт
    constexpr ptrdiff_t mCloudMeshes = 0x90;  // ::std::shared_ptr<::std::vector<::mce::Mesh>>, 16 байт
    constexpr ptrdiff_t mCloudMeshesCurrentlyTessellating = 0xA0;  // ::std::shared_ptr<::std::vector<::mce::Mesh>>, 16 байт
    constexpr ptrdiff_t mCloudTessellator = 0xB0;  // ::std::unique_ptr<::Tessellator>, 8 байт
    constexpr ptrdiff_t mCloudTextureTessellator = 0xB8;  // ::std::unique_ptr<::TextureTessellator>, 8 байт
    constexpr ptrdiff_t mUpdatingClouds = 0xC0;  // bool, 1 байт
    constexpr ptrdiff_t mLastCloudUpdatePosition = 0xC4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mLastCloudSide = 0xD0;  // uchar, 1 байт
    constexpr ptrdiff_t mLastCloudMeshSize = 0xD4;  // int, 4 байт
    constexpr ptrdiff_t mLastCloudGridSize = 0xD8;  // int, 4 байт
    constexpr ptrdiff_t mDirectionalLightRenderData = 0xDC;  // ::std::array<::DirectionalLightRenderData, 3>, 336 байт
    constexpr ptrdiff_t mRecordServerSoundMap = 0x230;  // ::std::unordered_map<::BlockPos, ::ServerSoundHandle>, 64 байт
    constexpr ptrdiff_t mDestroyingBlockList = 0x270;  // ::std::unordered_map<::BlockPos, ::BlockDestructInfo>, 64 байт
    constexpr ptrdiff_t mLastDestroyRenderTime = 0x2B0;  // double, 8 байт
    constexpr ptrdiff_t mFov = 0x2B8;  // float, 4 байт
    constexpr ptrdiff_t mOFov = 0x2BC;  // float, 4 байт
    constexpr ptrdiff_t mRenderingParameters = 0x2C0;  // ::PlayerRenderingParameters, 280 байт
    constexpr ptrdiff_t thirdDistance = 0x3D8;  // float, 4 байт
    constexpr ptrdiff_t thirdDistanceO = 0x3DC;  // float, 4 байт
    constexpr ptrdiff_t thirdRotation = 0x3E0;  // float, 4 байт
    constexpr ptrdiff_t thirdRotationO = 0x3E4;  // float, 4 байт
    constexpr ptrdiff_t thirdTilt = 0x3E8;  // float, 4 байт
    constexpr ptrdiff_t thirdTiltO = 0x3EC;  // float, 4 байт
    constexpr ptrdiff_t cameraRoll = 0x3F0;  // float, 4 байт
    constexpr ptrdiff_t cameraRollO = 0x3F4;  // float, 4 байт
    constexpr ptrdiff_t currentCameraDist = 0x3F8;  // float, 4 байт
    constexpr ptrdiff_t prevHeightOffset = 0x3FC;  // float, 4 байт
    constexpr ptrdiff_t mHeightOffsetAdjustment = 0x400;  // float, 4 байт
    constexpr ptrdiff_t mUnderwaterVisionTime = 0x404;  // float, 4 байт
    constexpr ptrdiff_t mUnderwaterVisionTimePrevious = 0x408;  // float, 4 байт
    constexpr ptrdiff_t mUnderwaterVisionClarity = 0x40C;  // float, 4 байт
    constexpr ptrdiff_t mUnderwaterVisionScale = 0x410;  // float, 4 байт
    constexpr ptrdiff_t mUnderwaterLightLevel = 0x414;  // float, 4 байт
    constexpr ptrdiff_t selectionBlockEntityOverlayMaterial = 0x418;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t selectionBlockEntityOverlayColorMaterial = 0x428;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t selectionOverlayMaterial = 0x438;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t selectionOpaqueMaterial = 0x448;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t selectionTwoSidedMaterial = 0x458;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t cracksOverlayMaterial = 0x468;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t cracksOverlayAlphaTestMaterial = 0x478;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t cracksOverlayBlockEntityMaterial = 0x488;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t mOutlineSelectionMaterial = 0x498;  // ::mce::MaterialPtr, 16 байт
    constexpr ptrdiff_t mSounds = 0x4A8;  // ::SoundMapping const&, 8 байт
    constexpr ptrdiff_t mDeferredSounds = 0x4B0;  // ::std::vector<::DeferredSound>, 24 байт
    constexpr ptrdiff_t uvBreakBlockItem = 0x4C8;  // ::TextureAtlasItem, 72 байт
    constexpr ptrdiff_t mClientInstance = 0x510;  // ::IClientInstance&, 8 байт
    constexpr ptrdiff_t mLocalUser = 0x518;  // ::WeakEntityRef, 24 байт
    constexpr ptrdiff_t mLevelRenderer = 0x530;  // ::LevelRenderer&, 8 байт
    constexpr ptrdiff_t mSoundPlayer = 0x538;  // ::Bedrock::NonOwnerPointer<::SoundPlayerInterface>, 24 байт
    constexpr ptrdiff_t mLevel = 0x550;  // ::Level&, 8 байт
    constexpr ptrdiff_t mAmbientSoundController = 0x558;  // ::std::unique_ptr<::AmbientSoundController>, 8 байт
    constexpr ptrdiff_t mListenerState = 0x560;  // ::ListenerState, 48 байт
    constexpr ptrdiff_t mCameraForward = 0x590;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCameraUp = 0x59C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mLastListenerPos = 0x5A8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mLastListenerUpdateTime = 0x5B8;  // double, 8 байт
    constexpr ptrdiff_t mSkyColorLerp = 0x5C0;  // ::std::optional<::LevelRendererPlayer::SkyColorLerp>, 56 байт
    constexpr ptrdiff_t mRainSoundTime = 0x5F8;  // int, 4 байт
    constexpr ptrdiff_t mOnLevelSoundEvent = 0x600;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnLevelSoundEventWithVolumeAndPitch = 0x610;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnStopLevelSoundEvent = 0x620;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnStopAllLevelSoundsEvent = 0x630;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnStopMusicEvent = 0x640;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnRecordStartedEvent = 0x650;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnRecordStoppedEvent = 0x660;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mSubtitlesOptionSubscription = 0x670;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mSubtitlesEnabled = 0x680;  // bool, 1 байт
    constexpr ptrdiff_t mLevelAnimateTickHelper = 0x688;  // ::std::unique_ptr<::LevelAnimateTickHelper>, 8 байт
    constexpr ptrdiff_t mScriptGraphicsOverrideManager = 0x690;  // ::ServerGraphicsOverrideManager, 3360 байт
    constexpr ptrdiff_t Size = 0x13B0;
}  // namespace LevelRendererPlayer

// LieDownAnimationComponent  [src/mc/entity/components/LieDownAnimationComponent.h]  расчётный размер 0x10
namespace LieDownAnimationComponent {
    constexpr ptrdiff_t mLieDownAmount = 0x0;  // ::InterpolationPair, 8 байт
    constexpr ptrdiff_t mLieDownAmountTail = 0x8;  // ::InterpolationPair, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace LieDownAnimationComponent

// LiquidClippedItemComponent  [src/mc/deps/shared_types/v1_20_50/item/LiquidClippedItemComponent.h]  расчётный размер 0x1
namespace LiquidClippedItemComponent {
    constexpr ptrdiff_t mIsLiquidClipped = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace LiquidClippedItemComponent

// LoadedChunksComponent  [src/mc/entity/components/LoadedChunksComponent.h]  расчётный размер 0x4
namespace LoadedChunksComponent {
    constexpr ptrdiff_t mLoadedChunksCount = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LoadedChunksComponent

// LoadingScreenPacketSenderComponent  [src/mc/entity/components/LoadingScreenPacketSenderComponent.h]  расчётный размер 0x18
namespace LoadingScreenPacketSenderComponent {
    constexpr ptrdiff_t mPackets = 0x0;  // ::std::vector<::std::shared_ptr<::Packet>>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace LoadingScreenPacketSenderComponent

// LoadingScreenStateChangeComponent  [src/mc/entity/components/LoadingScreenStateChangeComponent.h]  расчётный размер 0x4
namespace LoadingScreenStateChangeComponent {
    constexpr ptrdiff_t mLoadingScreenStateChangeType = 0x0;  // ::LoadingScreenStateChangeType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LoadingScreenStateChangeComponent

// LoadingStateComponent  [src/mc/entity/components/LoadingStateComponent.h]  расчётный размер 0x8
namespace LoadingStateComponent {
    constexpr ptrdiff_t mLoadingState = 0x0;  // ::LoadingState, 4 байт
    constexpr ptrdiff_t mLoadingProgress = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace LoadingStateComponent

// LocalPlayerDimensionWaitComponent  [src/mc/entity/components/LocalPlayerDimensionWaitComponent.h]  расчётный размер 0x10
namespace LocalPlayerDimensionWaitComponent {
    constexpr ptrdiff_t mWaitingForServerDimensionChangeAcknowledgment = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mServerDimensionChangeTimer = 0x8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace LocalPlayerDimensionWaitComponent

// LocalPlayerPrePlayerTravelComponent  [src/mc/entity/components/LocalPlayerPrePlayerTravelComponent.h]  расчётный размер 0x4
namespace LocalPlayerPrePlayerTravelComponent {
    constexpr ptrdiff_t mFlySpeed = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LocalPlayerPrePlayerTravelComponent

// LodestoneCompassComponent  [src/mc/entity/components/LodestoneCompassComponent.h]  расчётный размер 0x4
namespace LodestoneCompassComponent {
    constexpr ptrdiff_t mTrackingHandle = 0x0;  // ::PositionTrackingId, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LodestoneCompassComponent

// LoginPacket  [src/mc/network/packet/LoginPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace LoginPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace LoginPacket

// LookControlComponent  [src/mc/entity/components/LookControlComponent.h]  расчётный размер 0x30
namespace LookControlComponent {
    constexpr ptrdiff_t mLookControl = 0x0;  // ::std::unique_ptr<::LookControl>, 8 байт
    constexpr ptrdiff_t mWantedPosition = 0x8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mWantedRotation = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mYMax = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mXMax = 0x24;  // float, 4 байт
    constexpr ptrdiff_t mHasWantedPosition = 0x28;  // bool, 1 байт
    constexpr ptrdiff_t mHasWantedRotation = 0x29;  // bool, 1 байт
    constexpr ptrdiff_t mLookAtPositionLockedTicks = 0x2C;  // uint, 4 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace LookControlComponent

// LookedAtComponent  [src/mc/entity/components/LookedAtComponent.h]  расчётный размер 0x10
namespace LookedAtComponent {
    constexpr ptrdiff_t mDefinition = 0x0;  // ::LookedAtDefinition const*, 8 байт
    constexpr ptrdiff_t mCooldown = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mLookedAtDuration = 0xC;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace LookedAtComponent

// LoopbackPacketSender  [src/mc/network/LoopbackPacketSender.h]  расчётный размер 0x48  ⚠ BASE_UNKNOWN
namespace LoopbackPacketSender {
    constexpr ptrdiff_t mNetwork = 0x0;  // ::ClientOrServerNetworkSystemRef, 16 байт
    constexpr ptrdiff_t mLoopbackCallbacks = 0x10;  // ::std::vector<::NetEventCallback*>, 24 байт
    constexpr ptrdiff_t mUserList = 0x28;  // ::std::vector<::OwnerPtr<::EntityContext>> const*, 8 байт
    constexpr ptrdiff_t mTempUserIds = 0x30;  // ::std::vector<::NetworkIdentifierWithSubId>, 24 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace LoopbackPacketSender

// Material  [src-client/mc/client/renderer/block/tessellation_pipeline/Material.h]  расчётный размер 0x30  ⚠ BASE_UNKNOWN
namespace Material {
    constexpr ptrdiff_t mUvSets = 0x0;  // ::std::vector<::ClientBlockPipeline::TextureUVs>, 24 байт
    constexpr ptrdiff_t mAmbientOcclusion = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mBlockRenderLayer = 0x1C;  // ::BlockRenderLayer, 1 байт
    constexpr ptrdiff_t mTintMethod = 0x1D;  // ::TintMethod, 1 байт
    constexpr ptrdiff_t mPackedBools = 0x1E;  // ::ClientBlockPipeline::Material::PackedBools, 1 байт
    constexpr ptrdiff_t mTextureShiftBufferIndex = 0x20;  // ::std::optional<uint64>, 16 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace Material

// MaxStackSizeItemComponent  [src/mc/deps/shared_types/v1_20_50/item/MaxStackSizeItemComponent.h]  расчётный размер 0x2
namespace MaxStackSizeItemComponent {
    constexpr ptrdiff_t mValue = 0x0;  // short, 2 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace MaxStackSizeItemComponent

// MinecartPreNormalTickBlockPosComponent  [src/mc/entity/components/MinecartPreNormalTickBlockPosComponent.h]  расчётный размер 0xC
namespace MinecartPreNormalTickBlockPosComponent {
    constexpr ptrdiff_t mBlockPos = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace MinecartPreNormalTickBlockPosComponent

// MinecraftGameShimComponent  [src-client/mc/client/entity/components/events/MinecraftGameShimComponent.h]  расчётный размер 0x8
namespace MinecraftGameShimComponent {
    constexpr ptrdiff_t mGame = 0x0;  // ::gsl::not_null<::IMinecraftGame*>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace MinecraftGameShimComponent

// MingleComponent  [src/mc/entity/components/MingleComponent.h]  расчётный размер 0x18
namespace MingleComponent {
    constexpr ptrdiff_t mMingleState = 0x0;  // ::MingleComponent::MingleState, 4 байт
    constexpr ptrdiff_t mPartnerId = 0x8;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mPreviousPartnerId = 0x10;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace MingleComponent

// MobBodyRotationComponent  [src/mc/entity/components/MobBodyRotationComponent.h]  расчётный размер 0x8
namespace MobBodyRotationComponent {
    constexpr ptrdiff_t mYBodyRot = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mYBodyRotO = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace MobBodyRotationComponent

// MobEffectComponent  [src/mc/entity/components_json_legacy/MobEffectComponent.h]  расчётный размер 0x70
namespace MobEffectComponent {
    constexpr ptrdiff_t mEffectRange = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mEffectId = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mEffectTime = 0x8;  // ::EffectDuration, 4 байт
    constexpr ptrdiff_t mCooldownTicks = 0xC;  // ushort, 2 байт
    constexpr ptrdiff_t mEntityFilter = 0x10;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mIsAmbient = 0x50;  // bool, 1 байт
    constexpr ptrdiff_t mCooldown = 0x58;  // ::std::optional<::ExpiringTick>, 24 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace MobEffectComponent

// MobEffectImmunityComponent  [src/mc/entity/components/MobEffectImmunityComponent.h]  расчётный размер 0x18
namespace MobEffectImmunityComponent {
    constexpr ptrdiff_t mMobEffects = 0x0;  // ::std::vector<uint>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace MobEffectImmunityComponent

// MobEffectPacket  [src/mc/network/packet/MobEffectPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace MobEffectPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MobEffectPacket

// MobEffectsComponent  [src/mc/entity/components/MobEffectsComponent.h]  расчётный размер 0x18
namespace MobEffectsComponent {
    constexpr ptrdiff_t mMobEffects = 0x0;  // ::std::vector<::MobEffectInstance>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace MobEffectsComponent

// MobEquipmentPacket  [src/mc/network/packet/MobEquipmentPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace MobEquipmentPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MobEquipmentPacket

// MobJumpComponent  [src/mc/entity/components/MobJumpComponent.h]  расчётный размер 0x14
namespace MobJumpComponent {
    constexpr ptrdiff_t mJumpStartPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mJumpVelRedux = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mNoJumpDelay = 0x10;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace MobJumpComponent

// MobOnPlayerJumpRequestComponent  [src/mc/entity/components/MobOnPlayerJumpRequestComponent.h]  расчётный размер 0x4
namespace MobOnPlayerJumpRequestComponent {
    constexpr ptrdiff_t mJumpAmount = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MobOnPlayerJumpRequestComponent

// MobTravelComponent  [src/mc/entity/components/MobTravelComponent.h]  расчётный размер 0x26
namespace MobTravelComponent {
    constexpr ptrdiff_t mLocalMovementVelocity = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mMovementSpeed = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mPosBeforeMove = 0x10;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mFriction = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mSlowdown = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mDoesVerticalFriction = 0x24;  // bool, 1 байт
    constexpr ptrdiff_t mDoesGroundFriction = 0x25;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x26;
}  // namespace MobTravelComponent

// ModalFormRequestPacket  [src/mc/network/packet/ModalFormRequestPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ModalFormRequestPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ModalFormRequestPacket

// ModalFormResponsePacket  [src/mc/network/packet/ModalFormResponsePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace ModalFormResponsePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace ModalFormResponsePacket

// MountTamingComponent  [src/mc/entity/components_json_legacy/MountTamingComponent.h]  расчётный размер 0xA8
namespace MountTamingComponent {
    constexpr ptrdiff_t mTemper = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mCounter = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mTemperMod = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mWaitCount = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mOnTame = 0x10;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mFeedItems = 0x78;  // ::std::vector<::FeedItem>, 24 байт
    constexpr ptrdiff_t mAutoRejectItems = 0x90;  // ::std::vector<::ItemDescriptor>, 24 байт
    constexpr ptrdiff_t Size = 0xA8;
}  // namespace MountTamingComponent

// MountainParametersBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/MountainParametersBiomeJsonComponent.h]  расчётный размер 0xF2  ⚠ BASE_UNKNOWN
namespace MountainParametersBiomeJsonComponent {
    constexpr ptrdiff_t mMaterial = 0x0;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mNorthSlopes = 0x68;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mSouthSlopes = 0x6A;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mWestSlopes = 0x6C;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mEastSlopes = 0x6E;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mEnabled = 0x70;  // bool, 1 байт
    constexpr ptrdiff_t mSteepMaterialAdjustment = 0x78;  // ::std::optional<::SharedTypes::v1_20_60::MountainParametersBiomeJsonComponent::SteepMaterial>, 120 байт
    constexpr ptrdiff_t mTopSlide = 0xF0;  // ::std::optional<::SharedTypes::v1_20_60::MountainParametersBiomeJsonComponent::TopSlideSettings>, 2 байт
    constexpr ptrdiff_t Size = 0xF2;
}  // namespace MountainParametersBiomeJsonComponent

// MoveActorAbsoluteData  [src/mc/network/packet/MoveActorAbsoluteData.h]  расчётный размер 0x30
namespace MoveActorAbsoluteData {
    constexpr ptrdiff_t mRaw = 0x0;  // uchar, 1 байт
    constexpr ptrdiff_t mRuntimeId = 0x8;  // ::ActorRuntimeID, 8 байт
    constexpr ptrdiff_t mHeader = 0x10;  // ::MoveActorAbsoluteData::Header, 1 байт
    constexpr ptrdiff_t mPos = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRotX = 0x20;  // schar, 1 байт
    constexpr ptrdiff_t mRotY = 0x21;  // schar, 1 байт
    constexpr ptrdiff_t mRotYHead = 0x22;  // schar, 1 байт
    constexpr ptrdiff_t mRotYBody = 0x23;  // schar, 1 байт
    constexpr ptrdiff_t mTicks = 0x28;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace MoveActorAbsoluteData

// MoveActorAbsolutePacket  [src/mc/network/packet/MoveActorAbsolutePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace MoveActorAbsolutePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MoveActorAbsolutePacket

// MoveActorDeltaData  [src/mc/network/packet/MoveActorDeltaData.h]  расчётный размер 0x58
namespace MoveActorDeltaData {
    constexpr ptrdiff_t mRaw = 0x0;  // ushort, 2 байт
    constexpr ptrdiff_t mRuntimeId = 0x8;  // ::ActorRuntimeID, 8 байт
    constexpr ptrdiff_t mHeader = 0x10;  // ::MoveActorDeltaData::Header, 2 байт
    constexpr ptrdiff_t mNewPositionX = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mNewPositionY = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mNewPositionZ = 0x1C;  // float, 4 байт
    constexpr ptrdiff_t mRotX = 0x20;  // schar, 1 байт
    constexpr ptrdiff_t mRotY = 0x21;  // schar, 1 байт
    constexpr ptrdiff_t mRotYHead = 0x22;  // schar, 1 байт
    constexpr ptrdiff_t mTicks = 0x28;  // uint64, 8 байт
    constexpr ptrdiff_t mPreviousData = 0x30;  // ::MoveActorAbsoluteData, 40 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace MoveActorDeltaData

// MoveActorDeltaPacket  [src/mc/network/packet/MoveActorDeltaPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace MoveActorDeltaPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MoveActorDeltaPacket

// MoveControlComponent  [src/mc/entity/components_json_legacy/MoveControlComponent.h]  расчётный размер 0x30
namespace MoveControlComponent {
    constexpr ptrdiff_t mHasWanted = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mWantedPosition = 0x4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mShouldBreach = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t mMaxTurn = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mSpeedModifier = 0x18;  // float, 4 байт
    constexpr ptrdiff_t mMoveControl = 0x20;  // ::std::shared_ptr<::MoveControl>, 16 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace MoveControlComponent

// MoveInputComponent  [src/mc/entity/components/MoveInputComponent.h]  расчётный размер 0x64
namespace MoveInputComponent {
    constexpr ptrdiff_t mInputState = 0x0;  // ::MoveInputState, 16 байт
    constexpr ptrdiff_t mRawInputState = 0x10;  // ::MoveInputState, 16 байт
    constexpr ptrdiff_t mHoldAutoJumpInWaterTicks = 0x20;  // uchar, 1 байт
    constexpr ptrdiff_t mMove = 0x24;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mLookDelta = 0x2C;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mInteractDir = 0x34;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mDisplacement = 0x3C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mDisplacementDelta = 0x48;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCameraOrientation = 0x54;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mFlagValues = 0x60;  // ::brstd::bitset<11, ushort>, 2 байт
    constexpr ptrdiff_t mIsPaddling = 0x62;  // ::std::array<bool, 2>, 2 байт
    constexpr ptrdiff_t Size = 0x64;
}  // namespace MoveInputComponent

// MoveInputState  [src/mc/input/MoveInputState.h]  расчётный размер 0xF
namespace MoveInputState {
    constexpr ptrdiff_t mFlagValues = 0x0;  // ::brstd::bitset<27, uint>, 4 байт
    constexpr ptrdiff_t mAnalogMoveVector = 0x4;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mLookSlightDirField = 0xC;  // uchar, 1 байт
    constexpr ptrdiff_t mLookNormalDirField = 0xD;  // uchar, 1 байт
    constexpr ptrdiff_t mLookSmoothDirField = 0xE;  // uchar, 1 байт
    constexpr ptrdiff_t Size = 0xF;
}  // namespace MoveInputState

// MovePlayerPacket  [src/mc/network/packet/MovePlayerPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace MovePlayerPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace MovePlayerPacket

// MoveRequestComponent  [src/mc/deps/vanilla_components/MoveRequestComponent.h]  расчётный размер 0x98
namespace MoveRequestComponent {
    constexpr ptrdiff_t mOriginalAABBShape = 0x0;  // ::AABB, 24 байт
    constexpr ptrdiff_t mMoveCollisionLastFetchedBox = 0x18;  // ::AABB, 24 байт
    constexpr ptrdiff_t mPreCollisionSpeed = 0x30;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mSpeed = 0x3C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mMaxDepenetration = 0x48;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mIsPenetrating = 0x54;  // bool, 1 байт
    constexpr ptrdiff_t mSneaking = 0x55;  // bool, 1 байт
    constexpr ptrdiff_t mCollisionShapes = 0x58;  // ::CollisionShapes, 56 байт
    constexpr ptrdiff_t mPreviousHorizontalDelta = 0x90;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x98;
}  // namespace MoveRequestComponent

// MovementAbilitiesComponent  [src/mc/deps/vanilla_components/MovementAbilitiesComponent.h]  расчётный размер 0x10
namespace MovementAbilitiesComponent {
    constexpr ptrdiff_t mOverrides = 0x0;  // ::std::bitset<8>, 4 байт
    constexpr ptrdiff_t mFlagValues = 0x4;  // ::std::bitset<8>, 4 байт
    constexpr ptrdiff_t mFloatValues = 0x8;  // ::std::array<float, 2>, 8 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace MovementAbilitiesComponent

// MovementAttributesComponent  [src/mc/deps/vanilla_components/MovementAttributesComponent.h]  расчётный размер 0x48
namespace MovementAttributesComponent {
    constexpr ptrdiff_t mMovementSpeed = 0x0;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mUnderwaterMovementSpeed = 0x8;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mLavaMovementSpeed = 0x10;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mJumpStrength = 0x18;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mHealth = 0x20;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mHunger = 0x28;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mFrictionModifier = 0x30;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mBounciness = 0x38;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mAirDragModifier = 0x40;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace MovementAttributesComponent

// MovementCorrectionTelemetryComponent  [src/mc/entity/components/MovementCorrectionTelemetryComponent.h]  расчётный размер 0x24
namespace MovementCorrectionTelemetryComponent {
    constexpr ptrdiff_t mActorType = 0x0;  // ::ActorType, 4 байт
    constexpr ptrdiff_t mDivergences = 0x8;  // ::std::vector<float>, 24 байт
    constexpr ptrdiff_t mTickCount = 0x20;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace MovementCorrectionTelemetryComponent

// MovementEffectsComponent  [src/mc/entity/components/MovementEffectsComponent.h]  расчётный размер 0x18
namespace MovementEffectsComponent {
    constexpr ptrdiff_t mMovementEffects = 0x0;  // ::std::vector<::MovementEffect>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace MovementEffectsComponent

// MovementInterpolatorComponent  [src/mc/entity/components/MovementInterpolatorComponent.h]  расчётный размер 0x26
namespace MovementInterpolatorComponent {
    constexpr ptrdiff_t mPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRot = 0xC;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mHeadYaw = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mPositionSteps = 0x18;  // int, 4 байт
    constexpr ptrdiff_t mRotationSteps = 0x1C;  // int, 4 байт
    constexpr ptrdiff_t mHeadYawSteps = 0x20;  // int, 4 байт
    constexpr ptrdiff_t mMustBeCompleted = 0x24;  // bool, 1 байт
    constexpr ptrdiff_t mPlayerControlServerVehicle = 0x25;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x26;
}  // namespace MovementInterpolatorComponent

// MovementSoundComponent  [src/mc/entity/components/MovementSoundComponent.h]  расчётный размер 0x10
namespace MovementSoundComponent {
    constexpr ptrdiff_t mMakesFlySound = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mMakesStepSound = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mCrystalSoundIntensity = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mLastCrystalSoundPlayTick = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mTravelType = 0xC;  // ::MovementSoundTravelType, 4 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace MovementSoundComponent

// MovementWasCorrectedComponent  [src/mc/entity/components/MovementWasCorrectedComponent.h]  расчётный размер 0xD
namespace MovementWasCorrectedComponent {
    constexpr ptrdiff_t mDivergence = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mShouldReportTelemetry = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xD;
}  // namespace MovementWasCorrectedComponent

// MultinoiseGenerationRulesBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/MultinoiseGenerationRulesBiomeJsonComponent.h]  расчётный размер 0x28  ⚠ BASE_UNKNOWN
namespace MultinoiseGenerationRulesBiomeJsonComponent {
    constexpr ptrdiff_t mTargetTemperature = 0x0;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mTargetHumidity = 0x8;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mTargetAltitude = 0x10;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mTargetWeirdness = 0x18;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mWeight = 0x20;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace MultinoiseGenerationRulesBiomeJsonComponent

// NameableComponent  [src/mc/entity/components_json_legacy/NameableComponent.h]  расчётный размер 0x2
namespace NameableComponent {
    constexpr ptrdiff_t mAllowNameTagRenaming = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mAlwaysShow = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace NameableComponent

// NavigationComponent  [src/mc/entity/components_json_legacy/NavigationComponent.h]  расчётный размер 0x58
namespace NavigationComponent {
    constexpr ptrdiff_t mTick = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mTickTimeout = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mLastStuckCheck = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mEndPathRadiusSqr = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mSpeed = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mTerminationThreshold = 0x14;  // float, 4 байт
    constexpr ptrdiff_t mLastStuckCheckPosition = 0x18;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mTargetOffset = 0x24;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBlocksToAvoid = 0x30;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mNavigation = 0x48;  // ::std::unique_ptr<::PathNavigation>, 8 байт
    constexpr ptrdiff_t mPath = 0x50;  // ::std::unique_ptr<::Path>, 8 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace NavigationComponent

// NetEventCallbackComponent  [src/mc/network/NetEventCallbackComponent.h]  расчётный размер 0x18
namespace NetEventCallbackComponent {
    constexpr ptrdiff_t mCallback = 0x0;  // ::Bedrock::NonOwnerPointer<::NetEventCallback>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace NetEventCallbackComponent

// NetworkItemStackDescriptor  [src/mc/world/item/NetworkItemStackDescriptor.h]  расчётный размер 0x48  ⚠ BASE_UNKNOWN
namespace NetworkItemStackDescriptor {
    constexpr ptrdiff_t mIncludeNetIds = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mNetIdVariant = 0x8;  // ::ItemStackNetIdVariant, 24 байт
    constexpr ptrdiff_t mBlockRuntimeId = 0x20;  // uint, 4 байт
    constexpr ptrdiff_t mUserDataBuffer = 0x28;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace NetworkItemStackDescriptor

// NetworkStackLatencyPacket  [src/mc/network/packet/NetworkStackLatencyPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace NetworkStackLatencyPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace NetworkStackLatencyPacket

// NoActionTimeComponent  [src/mc/entity/components/NoActionTimeComponent.h]  расчётный размер 0x4
namespace NoActionTimeComponent {
    constexpr ptrdiff_t mValue = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace NoActionTimeComponent

// NpcComponent  [src/mc/entity/components_json_legacy/NpcComponent.h]  расчётный размер 0x169
namespace NpcComponent {
    constexpr ptrdiff_t mCurrentSkin = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mPortraitOffsets = 0x4;  // ::NpcGUIOffset, 36 байт
    constexpr ptrdiff_t mPickerOffsets = 0x28;  // ::NpcGUIOffset, 36 байт
    constexpr ptrdiff_t mNPCSkins = 0x50;  // ::std::vector<::SkinData>, 24 байт
    constexpr ptrdiff_t mActionsContainer = 0x68;  // ::npc::ActionContainer, 48 байт
    constexpr ptrdiff_t mPlayerSceneState = 0x98;  // ::std::unordered_map<::ActorUniqueID, ::std::string>, 64 байт
    constexpr ptrdiff_t mDefaultSceneId = 0xD8;  // ::std::string, 32 байт
    constexpr ptrdiff_t mInteractText = 0xF8;  // ::std::optional<::std::string>, 40 байт
    constexpr ptrdiff_t mInteractTextFilter = 0x120;  // ::std::function<::std::string(::std::string const&)>, 64 байт
    constexpr ptrdiff_t mNpcI18nObserver = 0x160;  // ::std::unique_ptr<::NpcI18nObserver>, 8 байт
    constexpr ptrdiff_t mIsGloballyAccessed = 0x168;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x169;
}  // namespace NpcComponent

// OfferFlowerTickComponent  [src/mc/entity/components/OfferFlowerTickComponent.h]  расчётный размер 0x4
namespace OfferFlowerTickComponent {
    constexpr ptrdiff_t mOfferFlowerTick = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace OfferFlowerTickComponent

// OffsetsComponent  [src/mc/deps/vanilla_components/OffsetsComponent.h]  расчётный размер 0x44
namespace OffsetsComponent {
    constexpr ptrdiff_t mHeightOffset = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mExplosionOffset = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mHeadOffset = 0x8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mDropOffset = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mEyeOffset = 0x20;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mMouthOffset = 0x2C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBreathingOffset = 0x38;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x44;
}  // namespace OffsetsComponent

// OnFireComponent  [src/mc/entity/components/OnFireComponent.h]  расчётный размер 0x5
namespace OnFireComponent {
    constexpr ptrdiff_t mOnFireTicks = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mIgnitedThisTick = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x5;
}  // namespace OnFireComponent

// OnUseItemComponent  [src/mc/world/item/components/OnUseItemComponent.h]  расчётный размер 0x38  ⚠ BASE_UNKNOWN
namespace OnUseItemComponent {
    constexpr ptrdiff_t mOnUse = 0x0;  // ::DefinitionTrigger, 56 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace OnUseItemComponent

// OpenDoorAnnotationComponent  [src/mc/entity/components_json_legacy/OpenDoorAnnotationComponent.h]  расчётный размер 0x28
namespace OpenDoorAnnotationComponent {
    constexpr ptrdiff_t mPassedDoorPositions = 0x0;  // ::std::queue<::BlockPos, ::std::deque<::BlockPos>>, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace OpenDoorAnnotationComponent

// OverworldGenerationRulesBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/OverworldGenerationRulesBiomeJsonComponent.h]  расчётный размер 0xE8  ⚠ BASE_UNKNOWN
namespace OverworldGenerationRulesBiomeJsonComponent {
    constexpr ptrdiff_t mValues = 0x0;  // ::std::array<::std::variant<::SharedTypes::Reference<3>, uint>, 2>, 80 байт
    constexpr ptrdiff_t mWeightedBiomeNames = 0x50;  // ::std::vector<::SharedTypes::v1_20_60::OverworldGenerationRulesBiomeJsonComponent::WeightedBiomeName>, 24 байт
    constexpr ptrdiff_t mHillsTransformation = 0x68;  // ::std::optional<::SharedTypes::v1_20_60::OverworldGenerationRulesBiomeJsonComponent::WeightedBiomeNameVector>, 32 байт
    constexpr ptrdiff_t mMutateTransformation = 0x88;  // ::std::optional<::SharedTypes::v1_20_60::OverworldGenerationRulesBiomeJsonComponent::WeightedBiomeNameVector>, 32 байт
    constexpr ptrdiff_t mRiverTransformation = 0xA8;  // ::std::optional<::SharedTypes::v1_20_60::OverworldGenerationRulesBiomeJsonComponent::WeightedBiomeNameVector>, 32 байт
    constexpr ptrdiff_t mShoreTransformation = 0xC8;  // ::std::optional<::SharedTypes::v1_20_60::OverworldGenerationRulesBiomeJsonComponent::WeightedBiomeNameVector>, 32 байт
    constexpr ptrdiff_t Size = 0xE8;
}  // namespace OverworldGenerationRulesBiomeJsonComponent

// OverworldHeightBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/OverworldHeightBiomeJsonComponent.h]  расчётный размер 0x14  ⚠ BASE_UNKNOWN
namespace OverworldHeightBiomeJsonComponent {
    constexpr ptrdiff_t mNoiseType = 0x0;  // ::std::optional<::SharedTypes::v1_20_60::OverworldHeightBiomeJsonComponent::NoiseType>, 8 байт
    constexpr ptrdiff_t mNoiseParams = 0x8;  // ::std::optional<::std::array<float, 2>>, 12 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace OverworldHeightBiomeJsonComponent

// ParticleEventDispatcherComponent  [src/mc/entity/components/ParticleEventDispatcherComponent.h]  расчётный размер 0x8
namespace ParticleEventDispatcherComponent {
    constexpr ptrdiff_t mEventQueueOwner = 0x0;  // ::StrictEntityContext, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ParticleEventDispatcherComponent

// ParticleEventRequestQueueComponent  [src/mc/entity/components/ParticleEventRequestQueueComponent.h]  расчётный размер 0x28
namespace ParticleEventRequestQueueComponent {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::queue<::ParticleEventRequest, ::std::deque<::ParticleEventRequest>>, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ParticleEventRequestQueueComponent

// PassengerComponent  [src/mc/entity/components/PassengerComponent.h]  расчётный размер 0x10
namespace PassengerComponent {
    constexpr ptrdiff_t mVehicle = 0x0;  // ::StrictActorIDEntityContextPair, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace PassengerComponent

// PassengerRenderingRidingOffsetComponent  [src/mc/entity/components/PassengerRenderingRidingOffsetComponent.h]  расчётный размер 0x8
namespace PassengerRenderingRidingOffsetComponent {
    constexpr ptrdiff_t mInterpolationPair = 0x0;  // ::RidingOffsetInterpolationPair, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PassengerRenderingRidingOffsetComponent

// PassengerYRotLimitComponent  [src/mc/entity/components/PassengerYRotLimitComponent.h]  расчётный размер 0x8
namespace PassengerYRotLimitComponent {
    constexpr ptrdiff_t mPassengerYRotLimit = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mLockedBodyYRot = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PassengerYRotLimitComponent

// PassengersToPositionComponent  [src/mc/entity/components/PassengersToPositionComponent.h]  расчётный размер 0x30
namespace PassengersToPositionComponent {
    constexpr ptrdiff_t mData = 0x0;  // ::std::vector<::PassengersToPositionComponent::Data>, 24 байт
    constexpr ptrdiff_t mStack = 0x18;  // ::std::vector<::StrictEntityContext>, 24 байт
    constexpr ptrdiff_t Size = 0x30;
}  // namespace PassengersToPositionComponent

// PeekComponent  [src/mc/entity/components/PeekComponent.h]  расчётный размер 0x8
namespace PeekComponent {
    constexpr ptrdiff_t mHadTarget = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mDuration = 0x4;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PeekComponent

// PendingRemovePassengersComponent  [src/mc/entity/components/PendingRemovePassengersComponent.h]  расчётный размер 0x18
namespace PendingRemovePassengersComponent {
    constexpr ptrdiff_t mPassengersToRemove = 0x0;  // ::std::vector<::StrictActorIDEntityContextPair>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace PendingRemovePassengersComponent

// PickComponent  [src/mc/entity/components/PickComponent.h]  расчётный размер 0x4
namespace PickComponent {
    constexpr ptrdiff_t mRayPickRadius = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PickComponent

// PlanterItemComponent  [src/mc/world/item/components/PlanterItemComponent.h]  расчётный размер 0x3A  ⚠ BASE_UNKNOWN
namespace PlanterItemComponent {
    constexpr ptrdiff_t mBlock = 0x0;  // ::BlockType const*, 8 байт
    constexpr ptrdiff_t mAlignedPlacement = 0x8;  // bool const, 1 байт
    constexpr ptrdiff_t mAllowedUseBlocks = 0x10;  // ::std::vector<::BlockDescriptor>, 24 байт
    constexpr ptrdiff_t mOnUseOnSubscription = 0x28;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mCanUseBlockAsIcon = 0x38;  // bool, 1 байт
    constexpr ptrdiff_t mReplaceBlockItem = 0x39;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x3A;
}  // namespace PlanterItemComponent

// PlaySoundPacket  [src/mc/network/packet/PlaySoundPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace PlaySoundPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlaySoundPacket

// PlayerActionComponent  [src/mc/entity/components/PlayerActionComponent.h]  расчётный размер 0x170
namespace PlayerActionComponent {
    constexpr ptrdiff_t mPlayerActions = 0x0;  // ::std::bitset<39>, 8 байт
    constexpr ptrdiff_t mItemUseTransaction = 0x8;  // ::std::optional<::PackedItemUseLegacyInventoryTransaction>, 320 байт
    constexpr ptrdiff_t mItemStackRequest = 0x148;  // ::std::unique_ptr<::ItemStackRequestData>, 8 байт
    constexpr ptrdiff_t mPlayerBlockActions = 0x150;  // ::PlayerBlockActions, 24 байт
    constexpr ptrdiff_t mPreSimulationRotation = 0x168;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x170;
}  // namespace PlayerActionComponent

// PlayerActionPacket  [src/mc/network/packet/PlayerActionPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace PlayerActionPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlayerActionPacket

// PlayerAuthInputPacket  [src/mc/network/packet/PlayerAuthInputPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace PlayerAuthInputPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlayerAuthInputPacket

// PlayerBlockActionData  [src/mc/entity/components/PlayerBlockActionData.h]  расчётный размер 0x14
namespace PlayerBlockActionData {
    constexpr ptrdiff_t mPlayerActionType = 0x0;  // ::PlayerActionType, 4 байт
    constexpr ptrdiff_t mPos = 0x4;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mFacing = 0x10;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace PlayerBlockActionData

// PlayerBlockActions  [src/mc/entity/components/PlayerBlockActions.h]  расчётный размер 0x18
namespace PlayerBlockActions {
    constexpr ptrdiff_t mActions = 0x0;  // ::std::vector<::PlayerBlockActionData>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace PlayerBlockActions

// PlayerChangeDimensionRequestComponent  [src/mc/entity/components/PlayerChangeDimensionRequestComponent.h]  расчётный размер 0x38
namespace PlayerChangeDimensionRequestComponent {
    constexpr ptrdiff_t mState = 0x0;  // ::PlayerChangeDimensionRequestComponent::State, 1 байт
    constexpr ptrdiff_t mRequest = 0x8;  // ::ChangeDimensionRequest, 48 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace PlayerChangeDimensionRequestComponent

// PlayerDestroyProgressCacheComponent  [src/mc/entity/components/PlayerDestroyProgressCacheComponent.h]  расчётный размер 0x28
namespace PlayerDestroyProgressCacheComponent {
    constexpr ptrdiff_t mCurrentToolIsOptimalToolForMining = 0x0;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mCurrentToolBlockDestroySpeed = 0x4;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mLastBlockBeingDestroyed = 0x10;  // uint64, 8 байт
    constexpr ptrdiff_t mLastItemUsedToDestroy = 0x18;  // ::ItemDescriptor, 16 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace PlayerDestroyProgressCacheComponent

// PlayerDimensionTransferSaveSuspensionComponent  [src/mc/entity/components/PlayerDimensionTransferSaveSuspensionComponent.h]  расчётный размер 0x28
namespace PlayerDimensionTransferSaveSuspensionComponent {
    constexpr ptrdiff_t mPlayerSuspendLevelStorageSaveToken = 0x0;  // ::PlayerSuspendLevelStorageSaveToken const, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace PlayerDimensionTransferSaveSuspensionComponent

// PlayerFlyingTravelComponent  [src/mc/entity/components/PlayerFlyingTravelComponent.h]  расчётный размер 0x4
namespace PlayerFlyingTravelComponent {
    constexpr ptrdiff_t mPreMoveYVel = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlayerFlyingTravelComponent

// PlayerInputModeComponent  [src/mc/entity/components/PlayerInputModeComponent.h]  расчётный размер 0x8
namespace PlayerInputModeComponent {
    constexpr ptrdiff_t mInputMode = 0x0;  // ::InputMode, 4 байт
    constexpr ptrdiff_t mPlayMode = 0x4;  // ::ClientPlayMode, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PlayerInputModeComponent

// PlayerInputRequestComponent  [src/mc/entity/components/PlayerInputRequestComponent.h]  расчётный размер 0x26
namespace PlayerInputRequestComponent {
    constexpr ptrdiff_t mMove = 0x0;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mIsSprinting = 0x8;  // bool, 1 байт
    constexpr ptrdiff_t mSprintCanceled = 0x9;  // bool, 1 байт
    constexpr ptrdiff_t mStopSprinting = 0xA;  // bool, 1 байт
    constexpr ptrdiff_t mIsPlayerRiding = 0xB;  // bool, 1 байт
    constexpr ptrdiff_t mUnblockedToStand = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t mUnblockedToSneak = 0xD;  // bool, 1 байт
    constexpr ptrdiff_t mUnblockedToCrawl = 0xE;  // bool, 1 байт
    constexpr ptrdiff_t mCanSprint = 0xF;  // ::CanSprintResult, 2 байт
    constexpr ptrdiff_t mBreathingPoint = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBreathingInAir = 0x20;  // bool, 1 байт
    constexpr ptrdiff_t mBreathingInLiquid = 0x21;  // bool, 1 байт
    constexpr ptrdiff_t mHasFlyIntent = 0x22;  // bool, 1 байт
    constexpr ptrdiff_t mHasGlideIntent = 0x23;  // bool, 1 байт
    constexpr ptrdiff_t mInstabuild = 0x24;  // bool, 1 байт
    constexpr ptrdiff_t mMayFly = 0x25;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x26;
}  // namespace PlayerInputRequestComponent

// PlayerInteractionModelComponent  [src/mc/entity/components/PlayerInteractionModelComponent.h]  расчётный размер 0x5
namespace PlayerInteractionModelComponent {
    constexpr ptrdiff_t mModel = 0x0;  // ::NewInteractionModel, 4 байт
    constexpr ptrdiff_t mShouldApplyBlockBreakingDelay = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x5;
}  // namespace PlayerInteractionModelComponent

// PlayerInventory  [src/mc/world/actor/player/PlayerInventory.h]  расчётный размер 0xD8  ⚠ BASE_UNKNOWN
namespace PlayerInventory {
    constexpr ptrdiff_t mSelected = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mInfiniteItem = 0x8;  // ::ItemStack, 152 байт
    constexpr ptrdiff_t mSelectedContainerId = 0xA0;  // ::ContainerID, 1 байт
    constexpr ptrdiff_t mInventory = 0xA8;  // ::std::unique_ptr<::Inventory>, 8 байт
    constexpr ptrdiff_t mComplexItems = 0xB0;  // ::std::vector<::ItemStack>, 24 байт
    constexpr ptrdiff_t mHudContainerManager = 0xC8;  // ::std::weak_ptr<::HudContainerManagerModel>, 16 байт
    constexpr ptrdiff_t Size = 0xD8;
}  // namespace PlayerInventory

// PlayerListEntry  [src/mc/world/actor/player/PlayerListEntry.h]  расчётный размер 0xA3
namespace PlayerListEntry {
    constexpr ptrdiff_t mId = 0x0;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mUUID = 0x8;  // ::mce::UUID, 16 байт
    constexpr ptrdiff_t mName = 0x18;  // ::std::string, 32 байт
    constexpr ptrdiff_t mXUID = 0x38;  // ::std::string, 32 байт
    constexpr ptrdiff_t mPlatformOnlineId = 0x58;  // ::std::string, 32 байт
    constexpr ptrdiff_t mBuildPlatform = 0x78;  // ::BuildPlatform, 4 байт
    constexpr ptrdiff_t mSkin = 0x80;  // ::SerializedSkinRef, 16 байт
    constexpr ptrdiff_t mColor = 0x90;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mIsTeacher = 0xA0;  // bool, 1 байт
    constexpr ptrdiff_t mIsHost = 0xA1;  // bool, 1 байт
    constexpr ptrdiff_t mIsSubClient = 0xA2;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xA3;
}  // namespace PlayerListEntry

// PlayerListPacket  [src/mc/network/packet/PlayerListPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace PlayerListPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlayerListPacket

// PlayerLoadingScreenComponent  [src/mc/entity/components/PlayerLoadingScreenComponent.h]  расчётный размер 0x8
namespace PlayerLoadingScreenComponent {
    constexpr ptrdiff_t mLoadingScreenId = 0x0;  // ::LoadingScreenId, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PlayerLoadingScreenComponent

// PlayerMovementSettingsComponent  [src/mc/world/actor/player/PlayerMovementSettingsComponent.h]  расчётный размер 0x40
namespace PlayerMovementSettingsComponent {
    constexpr ptrdiff_t mSettings = 0x0;  // ::PlayerMovementSettings, 64 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace PlayerMovementSettingsComponent

// PlayerPositionModeComponent  [src/mc/entity/components/PlayerPositionModeComponent.h]  расчётный размер 0x1
namespace PlayerPositionModeComponent {
    constexpr ptrdiff_t mPositionMode = 0x0;  // ::PlayerPositionModeComponent::PositionMode, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace PlayerPositionModeComponent

// PlayerPreMobTravelComponent  [src/mc/entity/components/PlayerPreMobTravelComponent.h]  расчётный размер 0xC
namespace PlayerPreMobTravelComponent {
    constexpr ptrdiff_t mPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace PlayerPreMobTravelComponent

// PlayerSaveSuspensionComponent  [src/mc/world/level/storage/PlayerSaveSuspensionComponent.h]  расчётный размер 0x10
namespace PlayerSaveSuspensionComponent {
    constexpr ptrdiff_t mWeakSuspendToken = 0x0;  // ::std::weak_ptr<::SuspendPlayerSave>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace PlayerSaveSuspensionComponent

// PlayerSkinPacket  [src/mc/network/packet/PlayerSkinPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace PlayerSkinPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace PlayerSkinPacket

// PortalCooldownDurationComponent  [src/mc/entity/components/PortalCooldownDurationComponent.h]  расчётный размер 0x1C
namespace PortalCooldownDurationComponent {
    constexpr ptrdiff_t mPortalBlockPos = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mMaxPortalCooldownTicks = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mCurrentPortalCooldownTicks = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mInsidePortalTimeTicks = 0x14;  // int, 4 байт
    constexpr ptrdiff_t mMaxPortalWaitTime = 0x18;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace PortalCooldownDurationComponent

// PositionPassengerRequestComponent  [src/mc/entity/components/PositionPassengerRequestComponent.h]  расчётный размер 0x8
namespace PositionPassengerRequestComponent {
    constexpr ptrdiff_t mVehicleRelativeSeatRotation = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mPassengerRelativeSeatRotation = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PositionPassengerRequestComponent

// PostGameEventRequestComponent  [src/mc/entity/components/PostGameEventRequestComponent.h]  расчётный размер 0x18
namespace PostGameEventRequestComponent {
    constexpr ptrdiff_t mGameEvents = 0x0;  // ::std::vector<::std::reference_wrapper<::GameEvent const>>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace PostGameEventRequestComponent

// PredictedMovementComponent  [src/mc/entity/components/PredictedMovementComponent.h]  расчётный размер 0x90  ⚠ BASE_UNKNOWN
namespace PredictedMovementComponent {
    constexpr ptrdiff_t mDisableConditions = 0x0;  // ::brstd::bitset<2, uchar>, 1 байт
    constexpr ptrdiff_t mRuntimePredictionData = 0x8;  // ::std::unique_ptr<::PredictedMovementComponent::RuntimePredictionData>, 8 байт
    constexpr ptrdiff_t mLastStartItem = 0x10;  // ::std::shared_ptr<::PredictedMovementComponent::HistoryItem const>, 16 байт
    constexpr ptrdiff_t mLastEndItem = 0x20;  // ::std::shared_ptr<::PredictedMovementComponent::HistoryItem const>, 16 байт
    constexpr ptrdiff_t mHistoryCache = 0x30;  // ::std::unique_ptr<::PredictedMovementComponent::HistoryCache>, 8 байт
    constexpr ptrdiff_t mForceTeleportAfterResuming = 0x38;  // bool, 1 байт
    constexpr ptrdiff_t mAllowTeleportingWithoutRegion = 0x39;  // bool, 1 байт
    constexpr ptrdiff_t mLastCompletedInterpolationSequenceID = 0x3C;  // uint, 4 байт
    constexpr ptrdiff_t mPredictionEventsListenerFunc = 0x40;  // ::std::function<void(::MovePredictionType)>, 64 байт
    constexpr ptrdiff_t mPredictedMovementValues = 0x80;  // ::PredictedMovementValues, 16 байт
    constexpr ptrdiff_t Size = 0x90;
}  // namespace PredictedMovementComponent

// PreferredPathComponent  [src/mc/entity/components/PreferredPathComponent.h]  расчётный размер 0x28
namespace PreferredPathComponent {
    constexpr ptrdiff_t mDefaultBlockCost = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mJumpCost = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mMaxFallBlocks = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mPreferredPathGroup = 0x10;  // ::std::vector<::BlockSet>, 24 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace PreferredPathComponent

// PreviousDefinitionsComponent  [src/mc/entity/components/PreviousDefinitionsComponent.h]  расчётный размер 0x8
namespace PreviousDefinitionsComponent {
    constexpr ptrdiff_t mPreviousDefinitions = 0x0;  // ::std::vector<::std::string> const*, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace PreviousDefinitionsComponent

// ProfanityFilterComponent  [src/mc/entity/components/ProfanityFilterComponent.h]  расчётный размер 0x2
namespace ProfanityFilterComponent {
    constexpr ptrdiff_t mIsRemoteProfanityFilterEnabled = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mIsLocalProfanityFilterEnabled = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace ProfanityFilterComponent

// ProjectileComponent  [src/mc/entity/components_json_legacy/ProjectileComponent.h]  расчётный размер 0x2FB
namespace ProjectileComponent {
    constexpr ptrdiff_t mWasOnGround = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mNoPhysics = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mOwnerId = 0x8;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mThrownPos = 0x10;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mApexPos = 0x1C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mFoundApex = 0x28;  // bool, 1 байт
    constexpr ptrdiff_t mDespawnTimer = 0x2C;  // int, 4 байт
    constexpr ptrdiff_t mLifetime = 0x30;  // int, 4 байт
    constexpr ptrdiff_t mFlightTime = 0x34;  // int, 4 байт
    constexpr ptrdiff_t mOnGroundTime = 0x38;  // int, 4 байт
    constexpr ptrdiff_t mShakeTime = 0x3C;  // int, 4 байт
    constexpr ptrdiff_t mBlock = 0x40;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mStuckToBlockPos = 0x4C;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mCollisionPos = 0x58;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mEnchantPower = 0x64;  // int, 4 байт
    constexpr ptrdiff_t mEnchantImpaler = 0x68;  // int, 4 байт
    constexpr ptrdiff_t mHitResult = 0x70;  // ::HitResult, 136 байт
    constexpr ptrdiff_t mHitFacing = 0xF8;  // uchar, 1 байт
    constexpr ptrdiff_t mReflect = 0xF9;  // bool, 1 байт
    constexpr ptrdiff_t mReflectImmunityTicks = 0xFC;  // int, 4 байт
    constexpr ptrdiff_t mPower = 0x100;  // float, 4 байт
    constexpr ptrdiff_t mDamage = 0x104;  // float, 4 байт
    constexpr ptrdiff_t mOffset = 0x108;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mHitParticle = 0x114;  // ::ParticleType, 4 байт
    constexpr ptrdiff_t mGravity = 0x118;  // float, 4 байт
    constexpr ptrdiff_t mUpwardsAngleOffset = 0x11C;  // float, 4 байт
    constexpr ptrdiff_t mOnHitCommands = 0x120;  // ::std::vector<::OnHitSubcomponent*>, 24 байт
    constexpr ptrdiff_t mOnHitEvent = 0x138;  // ::ActorDefinitionTrigger, 104 байт
    constexpr ptrdiff_t mUncertaintyBase = 0x1A0;  // float, 4 байт
    constexpr ptrdiff_t mUncertaintyMultiplier = 0x1A4;  // float, 4 байт
    constexpr ptrdiff_t mOnFireTime = 0x1A8;  // float, 4 байт
    constexpr ptrdiff_t mPotionEffect = 0x1AC;  // int, 4 байт
    constexpr ptrdiff_t mSplashRange = 0x1B0;  // float, 4 байт
    constexpr ptrdiff_t mKnockbackForce = 0x1B4;  // float, 4 байт
    constexpr ptrdiff_t mCatchFire = 0x1B8;  // bool, 1 байт
    constexpr ptrdiff_t mChanneling = 0x1B9;  // bool, 1 байт
    constexpr ptrdiff_t mInertiaMod = 0x1BC;  // float, 4 байт
    constexpr ptrdiff_t mLiquidInertia = 0x1C0;  // float, 4 байт
    constexpr ptrdiff_t mSpawnPosAnchor = 0x1C4;  // ::SharedTypes::v1_26_50::ProjectileComponentDefinition::ProjectileAnchor, 1 байт
    constexpr ptrdiff_t mHitEntitySound = 0x1C8;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mHitGroundSound = 0x1CC;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mShootSound = 0x1D0;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mIsDangerous = 0x1D4;  // bool, 1 байт
    constexpr ptrdiff_t mShootTarget = 0x1D5;  // bool, 1 байт
    constexpr ptrdiff_t mDamageOwner = 0x1D6;  // bool, 1 байт
    constexpr ptrdiff_t mReflectOnHurt = 0x1D7;  // bool, 1 байт
    constexpr ptrdiff_t mDestroyOnHurt = 0x1D8;  // bool, 1 байт
    constexpr ptrdiff_t mStopOnHurt = 0x1D9;  // bool, 1 байт
    constexpr ptrdiff_t mCritParticleOnHurt = 0x1DA;  // bool, 1 байт
    constexpr ptrdiff_t mHitWater = 0x1DB;  // bool, 1 байт
    constexpr ptrdiff_t mHitActor = 0x1DC;  // bool, 1 байт
    constexpr ptrdiff_t mMultipleHits = 0x1DD;  // bool, 1 байт
    constexpr ptrdiff_t mPiercingEnchantLevel = 0x1E0;  // int, 4 байт
    constexpr ptrdiff_t mActorHitCount = 0x1E4;  // int, 4 байт
    constexpr ptrdiff_t mIgnoredEntities = 0x1E8;  // ::std::vector<::ActorType>, 24 байт
    constexpr ptrdiff_t mHitNearestPassenger = 0x200;  // bool, 1 байт
    constexpr ptrdiff_t mIsHoming = 0x201;  // bool, 1 байт
    constexpr ptrdiff_t mPendingTargetID = 0x208;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mTarget = 0x210;  // ::TempEPtr<::Actor>, 40 байт
    constexpr ptrdiff_t mTargetID = 0x238;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mTargetDelta = 0x240;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mFlightSteps = 0x24C;  // int, 4 байт
    constexpr ptrdiff_t mCurrentMoveDirection = 0x250;  // ::ProjectileComponent::EAxis, 4 байт
    constexpr ptrdiff_t mLastReflectActor = 0x258;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mHitState = 0x260;  // ::ProjectileHitState, 5 байт
    constexpr ptrdiff_t mShouldBounce = 0x265;  // ::SharedTypes::v1_26_50::ProjectileComponentDefinition::ShouldBounce, 1 байт
    constexpr ptrdiff_t mHasLeftOwner = 0x266;  // bool, 1 байт
    constexpr ptrdiff_t mCurrentDelay = 0x268;  // uint, 4 байт
    constexpr ptrdiff_t mWaitingForServer = 0x26C;  // bool, 1 байт
    constexpr ptrdiff_t mWaitingForServerHitGround = 0x26D;  // bool, 1 байт
    constexpr ptrdiff_t mCachedHitResult = 0x270;  // ::HitResult, 136 байт
    constexpr ptrdiff_t mDelayOneFrame = 0x2F8;  // bool, 1 байт
    constexpr ptrdiff_t mShouldRestoreUsesMobTravelComponent = 0x2F9;  // bool, 1 байт
    constexpr ptrdiff_t mIsolatedPhysics = 0x2FA;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2FB;
}  // namespace ProjectileComponent

// ProjectileItemComponent  [src/mc/deps/shared_types/v1_20_50/item/ProjectileItemComponent.h]  расчётный размер 0x28
namespace ProjectileItemComponent {
    constexpr ptrdiff_t mMinimumCriticalPower = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mProjectileEntity = 0x8;  // ::SharedTypes::Reference<1>, 32 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ProjectileItemComponent

// PropertyComponent  [src/mc/world/actor/state/PropertyComponent.h]  расчётный размер 0xA0
namespace PropertyComponent {
    constexpr ptrdiff_t mPropertyGroup = 0x0;  // ::gsl::not_null<::std::shared_ptr<::PropertyGroup const>>, 16 байт
    constexpr ptrdiff_t mDirtyIntProperties = 0x10;  // ::std::bitset<32>, 4 байт
    constexpr ptrdiff_t mDirtyFloatProperties = 0x14;  // ::std::bitset<32>, 4 байт
    constexpr ptrdiff_t mDirtyBoolProperties = 0x18;  // ::std::bitset<32>, 4 байт
    constexpr ptrdiff_t mDirtyEnumIndexProperties = 0x1C;  // ::std::bitset<32>, 4 байт
    constexpr ptrdiff_t mEverythingDirty = 0x20;  // bool, 1 байт
    constexpr ptrdiff_t mPropertyContainer = 0x28;  // ::PropertyContainer, 120 байт
    constexpr ptrdiff_t Size = 0xA0;
}  // namespace PropertyComponent

// PropertySyncData  [src/mc/world/actor/state/PropertySyncData.h]  расчётный размер 0x38
namespace PropertySyncData {
    constexpr ptrdiff_t mPropertyIndex = 0x0;  // uint, 4 байт
    constexpr ptrdiff_t mData = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mIntEntries = 0x8;  // ::std::vector<::PropertySyncData::PropertySyncIntEntry>, 24 байт
    constexpr ptrdiff_t mFloatEntries = 0x20;  // ::std::vector<::PropertySyncData::PropertySyncFloatEntry>, 24 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace PropertySyncData

// PushedByComponent  [src/mc/entity/components/PushedByComponent.h]  расчётный размер 0x10
namespace PushedByComponent {
    constexpr ptrdiff_t mPushedBy = 0x0;  // ::StrictActorIDEntityContextPair, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace PushedByComponent

// RaidBossComponent  [src/mc/entity/components/RaidBossComponent.h]  расчётный размер 0xB0
namespace RaidBossComponent {
    constexpr ptrdiff_t mVillage = 0x0;  // ::std::weak_ptr<::Village>, 16 байт
    constexpr ptrdiff_t mOwnerID = 0x10;  // ::ActorUniqueID, 8 байт
    constexpr ptrdiff_t mName = 0x18;  // ::Bedrock::Safety::RedactableString, 72 байт
    constexpr ptrdiff_t mProgress = 0x60;  // ::std::string, 32 байт
    constexpr ptrdiff_t mPlayersRegistered = 0x80;  // int, 4 байт
    constexpr ptrdiff_t mWaveStarted = 0x84;  // bool, 1 байт
    constexpr ptrdiff_t mRaidInProgress = 0x85;  // bool, 1 байт
    constexpr ptrdiff_t mHealthBarVisible = 0x86;  // bool, 1 байт
    constexpr ptrdiff_t mHealthPercent = 0x88;  // float, 4 байт
    constexpr ptrdiff_t mBossBarVisibleBounds = 0x8C;  // ::AABB, 24 байт
    constexpr ptrdiff_t mLastPlayerUpdate = 0xA8;  // ::std::chrono::steady_clock::time_point, 8 байт
    constexpr ptrdiff_t Size = 0xB0;
}  // namespace RaidBossComponent

// RailMovementComponent  [src/mc/entity/components/RailMovementComponent.h]  расчётный размер 0x4
namespace RailMovementComponent {
    constexpr ptrdiff_t mMaxSpeed = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace RailMovementComponent

// RaiseArmAnimationComponent  [src/mc/entity/components/RaiseArmAnimationComponent.h]  расчётный размер 0x4
namespace RaiseArmAnimationComponent {
    constexpr ptrdiff_t mRaiseArmAmount = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace RaiseArmAnimationComponent

// RandomComponent  [src/mc/entity/components/RandomComponent.h]  расчётный размер 0x8
namespace RandomComponent {
    constexpr ptrdiff_t mRandom = 0x0;  // ::gsl::not_null<::IRandom*>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace RandomComponent

// RandomReferenceComponent  [src/mc/entity/components/RandomReferenceComponent.h]  расчётный размер 0x8
namespace RandomReferenceComponent {
    constexpr ptrdiff_t mRandomOwner = 0x0;  // ::StrictEntityContext, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace RandomReferenceComponent

// RarityItemComponent  [src/mc/deps/shared_types/v1_21_30/item/RarityItemComponent.h]  расчётный размер 0x20
namespace RarityItemComponent {
    constexpr ptrdiff_t mValue = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace RarityItemComponent

// RawMoveInputComponent  [src/mc/entity/components/RawMoveInputComponent.h]  расчётный размер 0x18
namespace RawMoveInputComponent {
    constexpr ptrdiff_t mRawInput = 0x0;  // ::MoveInputState, 16 байт
    constexpr ptrdiff_t mRawMove = 0x10;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace RawMoveInputComponent

// Reader  [src/mc/deps/json/Reader.h]  расчётный размер 0x11B
namespace Reader {
    constexpr ptrdiff_t type_ = 0x0;  // ::Json::Reader::TokenType, 4 байт
    constexpr ptrdiff_t start_ = 0x8;  // char const*, 8 байт
    constexpr ptrdiff_t end_ = 0x10;  // char const*, 8 байт
    constexpr ptrdiff_t stringHasEscape_ = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t token_ = 0x20;  // ::Json::Reader::Token, 32 байт
    constexpr ptrdiff_t message_ = 0x40;  // ::std::string, 32 байт
    constexpr ptrdiff_t extra_ = 0x60;  // char const*, 8 байт
    constexpr ptrdiff_t nodes_ = 0x68;  // ::std::stack<::Json::Value*, ::std::deque<::Json::Value*>>, 40 байт
    constexpr ptrdiff_t errors_ = 0x90;  // ::std::deque<::Json::Reader::ErrorInfo>, 40 байт
    constexpr ptrdiff_t document_ = 0xB8;  // ::std::string, 32 байт
    constexpr ptrdiff_t begin_ = 0xD8;  // char const*, 8 байт
    constexpr ptrdiff_t current_ = 0xE0;  // char const*, 8 байт
    constexpr ptrdiff_t lastValueEnd_ = 0xE8;  // char const*, 8 байт
    constexpr ptrdiff_t lastValue_ = 0xF0;  // ::Json::Value*, 8 байт
    constexpr ptrdiff_t commentsBefore_ = 0xF8;  // ::std::string, 32 байт
    constexpr ptrdiff_t features_ = 0x118;  // ::Json::Features, 2 байт
    constexpr ptrdiff_t collectComments_ = 0x11A;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x11B;
}  // namespace Reader

// RecordItemComponent  [src/mc/deps/shared_types/beta/item/RecordItemComponent.h]  расчётный размер 0x28
namespace RecordItemComponent {
    constexpr ptrdiff_t mSoundEvent = 0x0;  // ::SharedTypes::Versionless::SoundEventName, 32 байт
    constexpr ptrdiff_t mDuration = 0x20;  // float, 4 байт
    constexpr ptrdiff_t mComparatorSignal = 0x24;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace RecordItemComponent

// RemoveActorPacket  [src/mc/network/packet/RemoveActorPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace RemoveActorPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace RemoveActorPacket

// RemovePassengersComponent  [src/mc/entity/components/RemovePassengersComponent.h]  расчётный размер 0x18
namespace RemovePassengersComponent {
    constexpr ptrdiff_t mPassengersToRemove = 0x0;  // ::std::vector<::StrictEntityContext>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace RemovePassengersComponent

// RenderOffsetsItemComponent  [src/mc/deps/shared_types/legacy/item/RenderOffsetsItemComponent.h]  расчётный размер 0xFC
namespace RenderOffsetsItemComponent {
    constexpr ptrdiff_t mTranslation = 0x0;  // ::std::array<float, 3>, 12 байт
    constexpr ptrdiff_t mRotation = 0xC;  // ::std::array<float, 3>, 12 байт
    constexpr ptrdiff_t mScale = 0x18;  // ::std::array<float, 3>, 12 байт
    constexpr ptrdiff_t mFirstPerson = 0x24;  // ::SharedTypes::Legacy::RenderOffsetsItemComponent::TRS, 36 байт
    constexpr ptrdiff_t mThirdPerson = 0x48;  // ::SharedTypes::Legacy::RenderOffsetsItemComponent::TRS, 36 байт
    constexpr ptrdiff_t mMainHand = 0x6C;  // ::SharedTypes::Legacy::RenderOffsetsItemComponent::ItemTransforms, 72 байт
    constexpr ptrdiff_t mOffhand = 0xB4;  // ::SharedTypes::Legacy::RenderOffsetsItemComponent::ItemTransforms, 72 байт
    constexpr ptrdiff_t Size = 0xFC;
}  // namespace RenderOffsetsItemComponent

// RenderRotationComponent  [src/mc/entity/components/RenderRotationComponent.h]  расчётный размер 0x8
namespace RenderRotationComponent {
    constexpr ptrdiff_t mRot = 0x0;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace RenderRotationComponent

// RepairableItemComponent  [src/mc/deps/shared_types/v1_20_50/item/RepairableItemComponent.h]  расчётный размер 0x60
namespace RepairableItemComponent {
    constexpr ptrdiff_t mItems = 0x0;  // ::std::vector<::SharedTypes::Legacy::ItemDescriptor>, 24 байт
    constexpr ptrdiff_t mRepairAmount = 0x18;  // ::SharedTypes::Legacy::ExpressionNode, 48 байт
    constexpr ptrdiff_t mRepairItems = 0x48;  // ::std::vector<::SharedTypes::v1_20_50::RepairableItemComponent::RepairItemEntry>, 24 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace RepairableItemComponent

// ReplayStateComponent  [src/mc/entity/components/ReplayStateComponent.h]  расчётный размер 0x20
namespace ReplayStateComponent {
    constexpr ptrdiff_t mForceCorrection = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mShouldReportNextCorrectionTelemetry = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mIsCorrectionTelemetryEnabled = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mDivergenceCounter = 0x3;  // uchar, 1 байт
    constexpr ptrdiff_t mHistory = 0x8;  // ::std::unique_ptr<::ActorHistory>, 8 байт
    constexpr ptrdiff_t mPolicy = 0x10;  // ::std::unique_ptr<::IReplayStatePolicy>, 8 байт
    constexpr ptrdiff_t mCurrentTick = 0x18;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace ReplayStateComponent

// ReplayStateTrackerComponent  [src/mc/entity/components/ReplayStateTrackerComponent.h]  расчётный размер 0x68
namespace ReplayStateTrackerComponent {
    constexpr ptrdiff_t mDiff = 0x0;  // ::ReplayStateTrackerDiff, 104 байт
    constexpr ptrdiff_t Size = 0x68;
}  // namespace ReplayStateTrackerComponent

// ResourceLocation  [src/mc/deps/core/resource/ResourceLocation.h]  расчётный размер 0x38
namespace ResourceLocation {
    constexpr ptrdiff_t mFileSystem = 0x0;  // ::ResourceFileSystem, 4 байт
    constexpr ptrdiff_t mPath = 0x8;  // ::Core::PathBuffer<::std::string>, 32 байт
    constexpr ptrdiff_t mPathHash = 0x28;  // uint64, 8 байт
    constexpr ptrdiff_t mFullHash = 0x30;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace ResourceLocation

// RewindCollisionShapesComponent  [src/mc/entity/components/RewindCollisionShapesComponent.h]  расчётный размер 0x60
namespace RewindCollisionShapesComponent {
    constexpr ptrdiff_t mFetchedPosition = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mCollisionShapes = 0x10;  // ::CollisionShapes, 56 байт
    constexpr ptrdiff_t mMoveCollisionLastFetchedBox = 0x48;  // ::AABB, 24 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace RewindCollisionShapesComponent

// RideableComponent  [src/mc/entity/components_json_legacy/RideableComponent.h]  расчётный размер 0xA8
namespace RideableComponent {
    constexpr ptrdiff_t mData = 0x0;  // ::RideableComponentData, 168 байт
    constexpr ptrdiff_t Size = 0xA8;
}  // namespace RideableComponent

// RidingPrevIDComponent  [src/mc/entity/components/RidingPrevIDComponent.h]  расчётный размер 0x10
namespace RidingPrevIDComponent {
    constexpr ptrdiff_t mVehicle = 0x0;  // ::StrictActorIDEntityContextPair, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace RidingPrevIDComponent

// RuntimeIDComponent  [src/mc/entity/components/RuntimeIDComponent.h]  расчётный размер 0x8
namespace RuntimeIDComponent {
    constexpr ptrdiff_t mRuntimeID = 0x0;  // ::ActorRuntimeID, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace RuntimeIDComponent

// ScaleByAgeComponent  [src/mc/entity/components_json_legacy/ScaleByAgeComponent.h]  расчётный размер 0x8
namespace ScaleByAgeComponent {
    constexpr ptrdiff_t mStartScale = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mEndScale = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ScaleByAgeComponent

// SchedulePlayerLoadingScreenComponent  [src/mc/entity/components/SchedulePlayerLoadingScreenComponent.h]  расчётный размер 0x18
namespace SchedulePlayerLoadingScreenComponent {
    constexpr ptrdiff_t mSchedule = 0x0;  // ::std::vector<::LoadingScreenScheduleData>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace SchedulePlayerLoadingScreenComponent

// SchedulerComponent  [src/mc/entity/components_json_legacy/SchedulerComponent.h]  расчётный размер 0x4
namespace SchedulerComponent {
    constexpr ptrdiff_t mCurrentEventIndex = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace SchedulerComponent

// SendPacketsComponent  [src/mc/entity/components/SendPacketsComponent.h]  расчётный размер 0x18
namespace SendPacketsComponent {
    constexpr ptrdiff_t mPacketsToSend = 0x0;  // ::std::vector<::std::variant<::SendPacketInfo, ::SendPacketToOwnerInfo>>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace SendPacketsComponent

// SerializedPersonaPieceHandle  [src/mc/world/actor/player/SerializedPersonaPieceHandle.h]  расчётный размер 0x60
namespace SerializedPersonaPieceHandle {
    constexpr ptrdiff_t mPieceId = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mPieceType = 0x20;  // ::SharedTypes::persona::PieceType, 4 байт
    constexpr ptrdiff_t mPackId = 0x28;  // ::mce::UUID, 16 байт
    constexpr ptrdiff_t mIsDefaultPiece = 0x38;  // bool, 1 байт
    constexpr ptrdiff_t mProductId = 0x40;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace SerializedPersonaPieceHandle

// ServerActiveCameraComponent  [src/mc/entity/components/ServerActiveCameraComponent.h]  расчётный размер 0xC
namespace ServerActiveCameraComponent {
    constexpr ptrdiff_t mPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace ServerActiveCameraComponent

// ServerAnticipateClientLoadingScreenComponent  [src/mc/entity/components/ServerAnticipateClientLoadingScreenComponent.h]  расчётный размер 0x18
namespace ServerAnticipateClientLoadingScreenComponent {
    constexpr ptrdiff_t mLoadingScreenIds = 0x0;  // ::std::vector<::LoadingScreenId>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ServerAnticipateClientLoadingScreenComponent

// ServerCameraInstructionComponent  [src/mc/entity/components/ServerCameraInstructionComponent.h]  расчётный размер 0x20
namespace ServerCameraInstructionComponent {
    constexpr ptrdiff_t mPresetIndex = 0x0;  // uint, 4 байт
    constexpr ptrdiff_t mEase = 0x4;  // ::std::optional<::ServerCameraInstructionComponent::EaseOption>, 28 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace ServerCameraInstructionComponent

// ServerCameraStatesComponent  [src/mc/entity/components/ServerCameraStatesComponent.h]  расчётный размер 0x38
namespace ServerCameraStatesComponent {
    constexpr ptrdiff_t mActiveCameraPreset = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mStates = 0x20;  // ::std::vector<::ServerCameraStatesComponent::State>, 24 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace ServerCameraStatesComponent

// ServerCatchupMovementTrackerComponent  [src/mc/entity/components/ServerCatchupMovementTrackerComponent.h]  расчётный размер 0xC
namespace ServerCatchupMovementTrackerComponent {
    constexpr ptrdiff_t mPosAtLastSimulationTick = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace ServerCatchupMovementTrackerComponent

// ServerPlayerCurrentMovementComponent  [src/mc/entity/components/ServerPlayerCurrentMovementComponent.h]  расчётный размер 0x105
namespace ServerPlayerCurrentMovementComponent {
    constexpr ptrdiff_t mCurrentUpdate = 0x0;  // ::PlayerAuthInputPacket, 232 байт
    constexpr ptrdiff_t mOldPosition = 0xE8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPrevRotation = 0xF4;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mNewRotation = 0xFC;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mShouldCorrectForSneaking = 0x104;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x105;
}  // namespace ServerPlayerCurrentMovementComponent

// ServerPlayerInteractComponent  [src/mc/entity/components/ServerPlayerInteractComponent.h]  расчётный размер 0x58
namespace ServerPlayerInteractComponent {
    constexpr ptrdiff_t mCurrentInteraction = 0x0;  // ::InteractPacket, 88 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace ServerPlayerInteractComponent

// ServerPlayerInventoryTransactionComponent  [src/mc/entity/components/ServerPlayerInventoryTransactionComponent.h]  расчётный размер 0x18
namespace ServerPlayerInventoryTransactionComponent {
    constexpr ptrdiff_t mCurrentTransactions = 0x0;  // ::std::vector<::InventoryPacketHandler>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace ServerPlayerInventoryTransactionComponent

// ServerPlayerMovementComponent  [src/mc/entity/components/ServerPlayerMovementComponent.h]  расчётный размер 0x48
namespace ServerPlayerMovementComponent {
    constexpr ptrdiff_t mServerHasMovementAuthority = 0x0;  // ::std::bitset<4>, 4 байт
    constexpr ptrdiff_t mServerHasInventoryAuthority = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mAcceptClientPosIfWithinDistanceSq = 0x8;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mPlayerTickCredits = 0x10;  // uint64, 8 байт
    constexpr ptrdiff_t mPolicy = 0x18;  // ::std::unique_ptr<::IPlayerTickPolicy>, 8 байт
    constexpr ptrdiff_t mQueuedUpdates = 0x20;  // ::std::deque<::MovementPackets>, 40 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace ServerPlayerMovementComponent

// SetActorMotionPacket  [src/mc/network/packet/SetActorMotionPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace SetActorMotionPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace SetActorMotionPacket

// SetInstruction  [src/mc/deps/minecraft_camera/camera_instruction_options/SetInstruction.h]  расчётный размер 0x87
namespace SetInstruction {
    constexpr ptrdiff_t mEasingType = 0x0;  // ::EasingType, 4 байт
    constexpr ptrdiff_t mEasingTime = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mEntityOffsetX = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mEntityOffsetY = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mEntityOffsetZ = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mFacingPos = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPos = 0x20;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRotX = 0x2C;  // float, 4 байт
    constexpr ptrdiff_t mRotY = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mViewOffsetX = 0x34;  // float, 4 байт
    constexpr ptrdiff_t mViewOffsetY = 0x38;  // float, 4 байт
    constexpr ptrdiff_t mPresetIndex = 0x3C;  // uint, 4 байт
    constexpr ptrdiff_t mEase = 0x40;  // ::std::optional<::CameraInstructionOptions::SetInstruction::EaseOption>, 12 байт
    constexpr ptrdiff_t mRot = 0x4C;  // ::std::optional<::CameraInstructionOptions::SetInstruction::RotOption>, 12 байт
    constexpr ptrdiff_t mFacing = 0x58;  // ::std::optional<::CameraInstructionOptions::SetInstruction::FacingOption>, 16 байт
    constexpr ptrdiff_t mViewOffset = 0x68;  // ::std::optional<::CameraInstructionOptions::SetInstruction::ViewOffsetOption>, 12 байт
    constexpr ptrdiff_t mEntityOffset = 0x74;  // ::std::optional<::CameraInstructionOptions::SetInstruction::EntityOffsetOption>, 16 байт
    constexpr ptrdiff_t mDefault = 0x84;  // ::std::optional<bool>, 2 байт
    constexpr ptrdiff_t mRemoveIgnoreStartingValuesComponent = 0x86;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x87;
}  // namespace SetInstruction

// SetPlayerGameTypePacket  [src/mc/network/packet/SetPlayerGameTypePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace SetPlayerGameTypePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace SetPlayerGameTypePacket

// ShooterComponent  [src/mc/entity/components_json_legacy/ShooterComponent.h]  расчётный размер 0xD9
namespace ShooterComponent {
    constexpr ptrdiff_t mThrowPower = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mShooterProjectileData = 0x8;  // ::std::vector<::ShooterProjectileData>, 24 байт
    constexpr ptrdiff_t mDefaultActorDef = 0x20;  // ::ActorDefinitionIdentifier, 176 байт
    constexpr ptrdiff_t mDefaultAuxValue = 0xD0;  // int, 4 байт
    constexpr ptrdiff_t mSoundEvent = 0xD4;  // ::SharedTypes::Legacy::LevelSoundEvent, 4 байт
    constexpr ptrdiff_t mMagicAttacks = 0xD8;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xD9;
}  // namespace ShooterComponent

// ShooterItemComponent  [src/mc/world/item/components/ShooterItemComponent.h]  расчётный размер 0x60  ⚠ BASE_UNKNOWN
namespace ShooterItemComponent {
    constexpr ptrdiff_t mDuration = 0x0;  // float, 4 байт
    constexpr ptrdiff_t itemDesc = 0x8;  // ::ItemDescriptor, 16 байт
    constexpr ptrdiff_t useOffhand = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t searchInventory = 0x19;  // bool, 1 байт
    constexpr ptrdiff_t useInCreative = 0x1A;  // bool, 1 байт
    constexpr ptrdiff_t mAmmunition = 0x20;  // ::std::vector<::ShooterItemComponent::ShooterAmmunitionEntry>, 24 байт
    constexpr ptrdiff_t mDrawDuration = 0x38;  // ::ShooterItemComponent::DrawDuration, 4 байт
    constexpr ptrdiff_t mScalePowerByDrawDuration = 0x3C;  // bool, 1 байт
    constexpr ptrdiff_t mChargeOnDraw = 0x3D;  // bool, 1 байт
    constexpr ptrdiff_t mOnUseTimeDepletedSubscription = 0x40;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x50;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x60;
}  // namespace ShooterItemComponent

// ShouldDespawnItemComponent  [src/mc/deps/shared_types/v1_20_50/item/ShouldDespawnItemComponent.h]  расчётный размер 0x1
namespace ShouldDespawnItemComponent {
    constexpr ptrdiff_t mShouldDespawn = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace ShouldDespawnItemComponent

// ShulkerPeekAmountComponent  [src/mc/entity/components/ShulkerPeekAmountComponent.h]  расчётный размер 0x8
namespace ShulkerPeekAmountComponent {
    constexpr ptrdiff_t mPeekAmount = 0x0;  // ::InterpolationPair, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace ShulkerPeekAmountComponent

// SimpleContainer  [src/mc/world/SimpleContainer.h]  расчётный размер 0x20  ⚠ BASE_UNKNOWN
namespace SimpleContainer {
    constexpr ptrdiff_t mSize = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mItems = 0x8;  // ::std::vector<::ItemStack>, 24 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace SimpleContainer

// SkyColorClientBiomeJsonComponent  [src/mc/deps/shared_types/v1_21_40/clientbiome/components/SkyColorClientBiomeJsonComponent.h]  расчётный размер 0x10  ⚠ BASE_UNKNOWN
namespace SkyColorClientBiomeJsonComponent {
    constexpr ptrdiff_t mSkyColor = 0x0;  // ::SharedTypes::Color255RGB, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace SkyColorClientBiomeJsonComponent

// SlotDropChancesComponent  [src/mc/entity/components/SlotDropChancesComponent.h]  расчётный размер 0x38
namespace SlotDropChancesComponent {
    constexpr ptrdiff_t mDropChances = 0x0;  // ::std::array<::EquipmentTableDropChance, 6>, 48 байт
    constexpr ptrdiff_t mDefaultChance = 0x30;  // float, 4 байт
    constexpr ptrdiff_t mDefaultChanceForInvalidSlots = 0x34;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x38;
}  // namespace SlotDropChancesComponent

// SnapOnRailComponent  [src/mc/entity/components/SnapOnRailComponent.h]  расчётный размер 0x26
namespace SnapOnRailComponent {
    constexpr ptrdiff_t mSnapToPosition = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mOriginalEntityPosition = 0xC;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mBlock = 0x18;  // ::Block const*, 8 байт
    constexpr ptrdiff_t mRailDirection = 0x20;  // int, 4 байт
    constexpr ptrdiff_t mShouldHaltTrack = 0x24;  // bool, 1 байт
    constexpr ptrdiff_t mIsPowerTrack = 0x25;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x26;
}  // namespace SnapOnRailComponent

// SneakingComponent  [src/mc/entity/components/SneakingComponent.h]  расчётный размер 0x4
namespace SneakingComponent {
    constexpr ptrdiff_t mSneakingMovementFactor = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace SneakingComponent

// SneezeComponent  [src/mc/entity/components/SneezeComponent.h]  расчётный размер 0x4
namespace SneezeComponent {
    constexpr ptrdiff_t mSneezeTicks = 0x0;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace SneezeComponent

// SoundEventPlayerComponent  [src/mc/entity/components/SoundEventPlayerComponent.h]  расчётный размер 0x8
namespace SoundEventPlayerComponent {
    constexpr ptrdiff_t mSoundPlayer = 0x0;  // ::StrictEntityContext, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace SoundEventPlayerComponent

// SoundEventRequestQueueComponent  [src/mc/entity/components/SoundEventRequestQueueComponent.h]  расчётный размер 0x28
namespace SoundEventRequestQueueComponent {
    constexpr ptrdiff_t mRequests = 0x0;  // ::std::queue<::SoundEventRequest, ::std::deque<::SoundEventRequest>>, 40 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace SoundEventRequestQueueComponent

// SpawnActorComponent  [src/mc/entity/components_json_legacy/SpawnActorComponent.h]  расчётный размер 0x18
namespace SpawnActorComponent {
    constexpr ptrdiff_t mSpawnEntries = 0x0;  // ::std::vector<::SpawnActorEntry>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace SpawnActorComponent

// SpecialCameraModeComponent  [src-client/mc/deps/minecraft_camera/components/SpecialCameraModeComponent.h]  расчётный размер 0x1
namespace SpecialCameraModeComponent {
    constexpr ptrdiff_t mTripodCameraModeEnabled = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace SpecialCameraModeComponent

// SpinAttackResultsComponent  [src/mc/entity/components/SpinAttackResultsComponent.h]  расчётный размер 0x18
namespace SpinAttackResultsComponent {
    constexpr ptrdiff_t mEntities = 0x0;  // ::std::vector<::StrictEntityContext>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace SpinAttackResultsComponent

// StackedByDataItemComponent  [src/mc/deps/shared_types/v1_20_50/item/StackedByDataItemComponent.h]  расчётный размер 0x1
namespace StackedByDataItemComponent {
    constexpr ptrdiff_t mIsStackedByData = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace StackedByDataItemComponent

// StandAnimationComponent  [src/mc/entity/components/StandAnimationComponent.h]  расчётный размер 0x8
namespace StandAnimationComponent {
    constexpr ptrdiff_t mStandAnim = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mStandAnimO = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace StandAnimationComponent

// StateVectorComponent  [src/mc/deps/vanilla_components/StateVectorComponent.h]  расчётный размер 0x24
namespace StateVectorComponent {
    constexpr ptrdiff_t mPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPosPrev = 0xC;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPosDelta = 0x18;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace StateVectorComponent

// StationaryCameraComponent  [src-client/mc/deps/minecraft_camera/components/StationaryCameraComponent.h]  расчётный размер 0x14
namespace StationaryCameraComponent {
    constexpr ptrdiff_t mPos = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRot = 0xC;  // ::Vec2, 8 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace StationaryCameraComponent

// StepSoundFrequencyComponent  [src/mc/entity/components/StepSoundFrequencyComponent.h]  расчётный размер 0x8
namespace StepSoundFrequencyComponent {
    constexpr ptrdiff_t mMoveDist = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mNextStep = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace StepSoundFrequencyComponent

// StorageItemComponent  [src/mc/world/item/components/StorageItemComponent.h]  расчётный размер 0x78  ⚠ BASE_UNKNOWN
namespace StorageItemComponent {
    constexpr ptrdiff_t mNumSlots = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mAllowNestedStorageItem = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mBannedItems = 0x8;  // ::std::vector<::ItemDescriptor>, 24 байт
    constexpr ptrdiff_t mAllowedItems = 0x20;  // ::std::vector<::ItemDescriptor>, 24 байт
    constexpr ptrdiff_t mContainerSerialization = 0x38;  // ::WeakRef<::IDynamicContainerSerialization>, 16 байт
    constexpr ptrdiff_t mContainerRegistryAccess = 0x48;  // ::WeakRef<::IContainerRegistryAccess>, 16 байт
    constexpr ptrdiff_t mContainerRegistryTracker = 0x58;  // ::WeakRef<::IContainerRegistryTracker>, 16 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x68;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x78;
}  // namespace StorageItemComponent

// SubBBsComponent  [src/mc/deps/vanilla_components/SubBBsComponent.h]  расчётный размер 0x18
namespace SubBBsComponent {
    constexpr ptrdiff_t mSubBBs = 0x0;  // ::std::vector<::AABB>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace SubBBsComponent

// SubChunk  [src/mc/world/level/chunk/SubChunk.h]  расчётный размер 0x65
namespace SubChunk {
    constexpr ptrdiff_t mDirtyTicksCounter = 0x0;  // ::DirtyTicksCounter, 8 байт
    constexpr ptrdiff_t mSkyLight = 0x8;  // ::std::unique_ptr<::SubChunkBrightnessStorage>, 8 байт
    constexpr ptrdiff_t mBlockLight = 0x10;  // ::std::unique_ptr<::SubChunkBrightnessStorage>, 8 байт
    constexpr ptrdiff_t mSubChunkState = 0x18;  // ::SubChunk::SubChunkState, 4 байт
    constexpr ptrdiff_t mHasMaxSkyLight = 0x1C;  // bool, 1 байт
    constexpr ptrdiff_t mNeedsInitLighting = 0x1D;  // bool, 1 байт
    constexpr ptrdiff_t mNeedsClientLighting = 0x1E;  // bool, 1 байт
    constexpr ptrdiff_t mBlocks = 0x20;  // ::std::unique_ptr<::SubChunkStorage<::Block>>[2], 16 байт
    constexpr ptrdiff_t mBlocksReadPtr = 0x30;  // ::SubChunkStorage<::Block>* [2], 16 байт
    constexpr ptrdiff_t mWriteLock = 0x40;  // ::SpinLockImpl, 24 байт
    constexpr ptrdiff_t mHash = 0x58;  // uint64, 8 байт
    constexpr ptrdiff_t mHashDirty = 0x60;  // bool, 1 байт
    constexpr ptrdiff_t mAbsoluteIndex = 0x61;  // schar, 1 байт
    constexpr ptrdiff_t mIsReplacementSubChunk = 0x62;  // bool, 1 байт
    constexpr ptrdiff_t mRenderChunkTrackingVersionNumber = 0x63;  // uchar, 1 байт
    constexpr ptrdiff_t mIsInitialized = 0x64;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x65;
}  // namespace SubChunk

// SurfaceMaterialAdjustmentsBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/SurfaceMaterialAdjustmentsBiomeJsonComponent.h]  расчётный размер 0x480  ⚠ BASE_UNKNOWN
namespace SurfaceMaterialAdjustmentsBiomeJsonComponent {
    constexpr ptrdiff_t mTopMaterial = 0x0;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mMidMaterial = 0x68;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mSeaFloorMaterial = 0xD0;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mFoundationMaterial = 0x138;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mSeaMaterial = 0x1A0;  // ::std::optional<::SharedTypes::v1_20_60::BlockSpecifier>, 104 байт
    constexpr ptrdiff_t mMaterials = 0x208;  // ::SharedTypes::v1_20_60::SurfaceMaterialAdjustmentsBiomeJsonComponent::SurfaceMaterialAdjustmentMaterials, 520 байт
    constexpr ptrdiff_t mNoiseRange = 0x410;  // ::std::optional<::std::array<float, 2>>, 12 байт
    constexpr ptrdiff_t mHeightRange = 0x420;  // ::std::optional<::std::array<::SharedTypes::Util::MolangJsonContainer, 2>>, 88 байт
    constexpr ptrdiff_t mNoiseFrequencyScale = 0x478;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t Size = 0x480;
}  // namespace SurfaceMaterialAdjustmentsBiomeJsonComponent

// SuspectTrackingComponent  [src/mc/entity/components/SuspectTrackingComponent.h]  расчётный размер 0x20
namespace SuspectTrackingComponent {
    constexpr ptrdiff_t mSuspiciousPos = 0x0;  // ::std::optional<::BlockPos>, 16 байт
    constexpr ptrdiff_t mLastSuspicionTick = 0x10;  // ::std::optional<::Tick>, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace SuspectTrackingComponent

// SwimAmountComponent  [src/mc/entity/components/SwimAmountComponent.h]  расчётный размер 0x8
namespace SwimAmountComponent {
    constexpr ptrdiff_t mSwimAmount = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mSwimAmountPrev = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace SwimAmountComponent

// SyncedPlayerMovementSettings  [src/mc/world/actor/player/SyncedPlayerMovementSettings.h]  расчётный размер 0x5
namespace SyncedPlayerMovementSettings {
    constexpr ptrdiff_t mRewindHistorySize = 0x0;  // int, 4 байт
    constexpr ptrdiff_t ServerAuthBlockBreaking = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x5;
}  // namespace SyncedPlayerMovementSettings

// SynchedActorDataComponent  [src/mc/entity/components/SynchedActorDataComponent.h]  расчётный размер 0x48
namespace SynchedActorDataComponent {
    constexpr ptrdiff_t mData = 0x0;  // ::SynchedActorData, 72 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace SynchedActorDataComponent

// TagsBiomeJsonComponent  [src/mc/deps/shared_types/v1_20_60/biome/components/TagsBiomeJsonComponent.h]  расчётный размер 0x18  ⚠ BASE_UNKNOWN
namespace TagsBiomeJsonComponent {
    constexpr ptrdiff_t mTags = 0x0;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace TagsBiomeJsonComponent

// TagsItemComponent  [src/mc/deps/shared_types/v1_20_50/item/TagsItemComponent.h]  расчётный размер 0x18
namespace TagsItemComponent {
    constexpr ptrdiff_t mTags = 0x0;  // ::std::vector<::std::string>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace TagsItemComponent

// TameableComponent  [src/mc/entity/components_json_legacy/TameableComponent.h]  расчётный размер 0x20
namespace TameableComponent {
    constexpr ptrdiff_t mChance = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mTameItems = 0x8;  // ::std::vector<::ActorDefinitionTameItem>, 24 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace TameableComponent

// TargetCameraOrientationComponent  [src/mc/deps/minecraft_camera/components/TargetCameraOrientationComponent.h]  расчётный размер 0x10
namespace TargetCameraOrientationComponent {
    constexpr ptrdiff_t mTargetCameraOrientation = 0x0;  // ::glm::qua<float>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace TargetCameraOrientationComponent

// TargetCameraRotationLimitComponent  [src-client/mc/deps/minecraft_camera/components/TargetCameraRotationLimitComponent.h]  расчётный размер 0x8
namespace TargetCameraRotationLimitComponent {
    constexpr ptrdiff_t mAverageHorizontalRotLimit = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mAverageVerticalRotLimit = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace TargetCameraRotationLimitComponent

// TargetCameraRotationSpeedComponent  [src-client/mc/deps/minecraft_camera/components/TargetCameraRotationSpeedComponent.h]  расчётный размер 0x4
namespace TargetCameraRotationSpeedComponent {
    constexpr ptrdiff_t mCameraRotationSpeed = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace TargetCameraRotationSpeedComponent

// TargetNearbyComponent  [src/mc/entity/components/TargetNearbyComponent.h]  расчётный размер 0x8
namespace TargetNearbyComponent {
    constexpr ptrdiff_t mWasSeenLastTick = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mWasInsideRange = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mWasOutsideRange = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mPreviousDistance = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace TargetNearbyComponent

// TeleportComponent  [src/mc/entity/components_json_legacy/TeleportComponent.h]  расчётный размер 0x70
namespace TeleportComponent {
    constexpr ptrdiff_t mRandomTeleports = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mTeleportsOnProjectileHit = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mProjectileFilter = 0x8;  // ::ActorFilterGroup, 64 байт
    constexpr ptrdiff_t mMinTeleportTime = 0x48;  // int, 4 байт
    constexpr ptrdiff_t mMaxTeleportTime = 0x4C;  // int, 4 байт
    constexpr ptrdiff_t mRandomTeleportCube = 0x50;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mTargetDistance = 0x5C;  // float, 4 байт
    constexpr ptrdiff_t mTargetTeleportChance = 0x60;  // float, 4 байт
    constexpr ptrdiff_t mLightTeleportChance = 0x64;  // float, 4 байт
    constexpr ptrdiff_t mDarkTeleportChance = 0x68;  // float, 4 байт
    constexpr ptrdiff_t mTeleportTime = 0x6C;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x70;
}  // namespace TeleportComponent

// TextPacket  [src/mc/network/packet/TextPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace TextPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace TextPacket

// TexturePtr  [src-client/mc/deps/minecraft_renderer/renderer/TexturePtr.h]  расчётный размер 0x20
namespace TexturePtr {
    constexpr ptrdiff_t mClientTexture = 0x0;  // ::std::shared_ptr<::BedrockTextureData const>, 16 байт
    constexpr ptrdiff_t mResourceLocationPtr = 0x10;  // ::std::shared_ptr<::ResourceLocation>, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace TexturePtr

// ThrowableItemComponent  [src/mc/world/item/components/ThrowableItemComponent.h]  расчётный размер 0x28  ⚠ BASE_UNKNOWN
namespace ThrowableItemComponent {
    constexpr ptrdiff_t mDoSwing = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mMinDrawDuration = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mDrawDuration = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mLaunchPowerScale = 0xC;  // float, 4 байт
    constexpr ptrdiff_t mMaxLaunchPower = 0x10;  // float, 4 байт
    constexpr ptrdiff_t mScalePowerByDrawDuration = 0x14;  // bool, 1 байт
    constexpr ptrdiff_t mTriggerActorUseItemEvent = 0x15;  // bool, 1 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x18;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x28;
}  // namespace ThrowableItemComponent

// TickWorldComponent  [src/mc/entity/components_json_legacy/TickWorldComponent.h]  расчётный размер 0x20
namespace TickWorldComponent {
    constexpr ptrdiff_t mChunkRadius = 0x0;  // uint, 4 байт
    constexpr ptrdiff_t mMaxDistToPlayers = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mAlwaysActive = 0x8;  // bool, 1 байт
    constexpr ptrdiff_t mChanged = 0x9;  // bool, 1 байт
    constexpr ptrdiff_t mTickingArea = 0x10;  // ::std::weak_ptr<::ITickingArea>, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace TickWorldComponent

// TimerComponent  [src/mc/entity/components/TimerComponent.h]  расчётный размер 0x40
namespace TimerComponent {
    constexpr ptrdiff_t mTime = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mTimeStamp = 0x8;  // uint64, 8 байт
    constexpr ptrdiff_t mHasExecuted = 0x10;  // bool, 1 байт
    constexpr ptrdiff_t mLooping = 0x11;  // bool, 1 байт
    constexpr ptrdiff_t mStartTime = 0x14;  // int, 4 байт
    constexpr ptrdiff_t mRandomInterval = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mMinTime = 0x1C;  // int, 4 байт
    constexpr ptrdiff_t mMaxTime = 0x20;  // int, 4 байт
    constexpr ptrdiff_t mTimeChoices = 0x28;  // ::WeightedChoices<float>, 24 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace TimerComponent

// TintColorComponent  [src/mc/entity/components/TintColorComponent.h]  расчётный размер 0x20
namespace TintColorComponent {
    constexpr ptrdiff_t mTintColor = 0x0;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t mTintColor2 = 0x10;  // ::mce::Color, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace TintColorComponent

// TintMapColor  [src/mc/deps/core_graphics/helpers/TintMapColor.h]  расчётный размер 0x40
namespace TintMapColor {
    constexpr ptrdiff_t colors = 0x0;  // ::std::array<::mce::Color, 4>, 64 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace TintMapColor

// TradeResupplyComponent  [src/mc/entity/components/TradeResupplyComponent.h]  расчётный размер 0x1
namespace TradeResupplyComponent {
    constexpr ptrdiff_t mHasResupplied = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace TradeResupplyComponent

// TrailComponent  [src/mc/entity/components/TrailComponent.h]  расчётный размер 0x14
namespace TrailComponent {
    constexpr ptrdiff_t mBlockType = 0x0;  // ::BlockType const*, 8 байт
    constexpr ptrdiff_t mSpawnOffset = 0x8;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x14;
}  // namespace TrailComponent

// TransformationComponent  [src/mc/deps/shared_types/v1_26_20/block/components/TransformationComponent.h]  расчётный размер 0x3C
namespace TransformationComponent {
    constexpr ptrdiff_t mTranslation = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mScale = 0xC;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mScalePivot = 0x18;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRotation = 0x24;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mRotationPivot = 0x30;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x3C;
}  // namespace TransformationComponent

// TriggerJumpRequestComponent  [src/mc/entity/components/TriggerJumpRequestComponent.h]  расчётный размер 0x10
namespace TriggerJumpRequestComponent {
    constexpr ptrdiff_t mJumpPreventionResult = 0x0;  // ::JumpPreventionResult, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace TriggerJumpRequestComponent

// TripodCameraActivatedComponent  [src/mc/entity/components/TripodCameraActivatedComponent.h]  расчётный размер 0x1C
namespace TripodCameraActivatedComponent {
    constexpr ptrdiff_t mSubject = 0x0;  // ::WeakRef<::EntityContext>, 24 байт
    constexpr ptrdiff_t mCountdown = 0x18;  // int, 4 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace TripodCameraActivatedComponent

// TrustComponent  [src/mc/entity/components/TrustComponent.h]  расчётный размер 0x40
namespace TrustComponent {
    constexpr ptrdiff_t mTrustedPlayerIDs = 0x0;  // ::std::unordered_set<::ActorUniqueID>, 64 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace TrustComponent

// TrustingComponent  [src/mc/entity/components/TrustingComponent.h]  расчётный размер 0x4
namespace TrustingComponent {
    constexpr ptrdiff_t mChance = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace TrustingComponent

// UnlockedRecipesClientComponent  [src/mc/entity/components/UnlockedRecipesClientComponent.h]  расчётный размер 0xC8
namespace UnlockedRecipesClientComponent {
    constexpr ptrdiff_t mUnlockedCategories = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mNewlyUnlockedCategories = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mUnlockedItems = 0x8;  // ::std::unordered_set<int>, 64 байт
    constexpr ptrdiff_t mUnlockedRecipes = 0x48;  // ::std::unordered_set<::std::string>, 64 байт
    constexpr ptrdiff_t mNewlyUnlockedRecipes = 0x88;  // ::std::unordered_set<::std::string>, 64 байт
    constexpr ptrdiff_t Size = 0xC8;
}  // namespace UnlockedRecipesClientComponent

// UnlockedRecipesServerComponent  [src/mc/entity/components/UnlockedRecipesServerComponent.h]  расчётный размер 0xA0
namespace UnlockedRecipesServerComponent {
    constexpr ptrdiff_t mInventoryHasChanged = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mInitialDataSent = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mUsedContexts = 0x4;  // uint, 4 байт
    constexpr ptrdiff_t mUnlockedRecipes = 0x8;  // ::std::unordered_set<::std::string>, 64 байт
    constexpr ptrdiff_t mChangedInventorySlots = 0x48;  // ::std::unordered_set<int>, 64 байт
    constexpr ptrdiff_t mUnlockingInstructions = 0x88;  // ::std::vector<::UnlockedRecipesServerComponent::UnlockingInstruction>, 24 байт
    constexpr ptrdiff_t Size = 0xA0;
}  // namespace UnlockedRecipesServerComponent

// UpdateAndRenderThrottleComponent  [src/mc/entity/components/UpdateAndRenderThrottleComponent.h]  расчётный размер 0x2
namespace UpdateAndRenderThrottleComponent {
    constexpr ptrdiff_t mCanUpdateAndRender = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mWasVisibleLastRender = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0x2;
}  // namespace UpdateAndRenderThrottleComponent

// UpdateBlockPacket  [src/mc/network/packet/UpdateBlockPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace UpdateBlockPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace UpdateBlockPacket

// UpdateBlockSyncedPacket  [src/mc/network/packet/UpdateBlockSyncedPacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace UpdateBlockSyncedPacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace UpdateBlockSyncedPacket

// UpdatePlayerFromCameraComponent  [src-client/mc/deps/vanilla_camera/components/UpdatePlayerFromCameraComponent.h]  расчётный размер 0x4
namespace UpdatePlayerFromCameraComponent {
    constexpr ptrdiff_t mLookMode = 0x0;  // ::SharedTypes::v1_21_100::LookMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace UpdatePlayerFromCameraComponent

// UpdatePlayerGameTypePacket  [src/mc/network/packet/UpdatePlayerGameTypePacket.h]  расчётный размер 0x4  ⚠ BASE_UNKNOWN
namespace UpdatePlayerGameTypePacket {
    constexpr ptrdiff_t mSerializationMode = 0x0;  // ::SerializationMode, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace UpdatePlayerGameTypePacket

// UpdateWaterStateRequestComponent  [src/mc/entity/components/UpdateWaterStateRequestComponent.h]  расчётный размер 0x10
namespace UpdateWaterStateRequestComponent {
    constexpr ptrdiff_t mMaterialType = 0x0;  // ::SharedTypes::v1_26_20::MaterialType, 1 байт
    constexpr ptrdiff_t mShouldApplyFlow = 0x1;  // ::UpdateWaterStateFlowRequest, 1 байт
    constexpr ptrdiff_t mHasAdjacentFlowingBlocks = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mIsAnyWaterInBB = 0x3;  // bool, 1 байт
    constexpr ptrdiff_t mAccumulatedFlow = 0x4;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace UpdateWaterStateRequestComponent

// UseAnimationItemComponent  [src/mc/deps/shared_types/v1_20_50/item/UseAnimationItemComponent.h]  расчётный размер 0x1
namespace UseAnimationItemComponent {
    constexpr ptrdiff_t mAnimation = 0x0;  // ::SharedTypes::Legacy::UseAnimation, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace UseAnimationItemComponent

// UseModifiersItemComponent  [src/mc/world/item/components/UseModifiersItemComponent.h]  расчётный размер 0x58  ⚠ BASE_UNKNOWN
namespace UseModifiersItemComponent {
    constexpr ptrdiff_t mUseDuration = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mEmitVibrations = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mStartSound = 0x8;  // ::std::optional<::SoundEventIdentifier>, 48 байт
    constexpr ptrdiff_t mMovementModifier = 0x38;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t mStartUsing = 0x40;  // ::UseModifiersItemComponent::StartUsing, 1 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x48;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x58;
}  // namespace UseModifiersItemComponent

// UserEntityIdentifierComponent  [src/mc/entity/components/UserEntityIdentifierComponent.h]  расчётный размер 0x250
namespace UserEntityIdentifierComponent {
    constexpr ptrdiff_t mNetworkId = 0x0;  // ::NetworkIdentifier, 176 байт
    constexpr ptrdiff_t mClientSubId = 0xB0;  // ::SubClientId, 1 байт
    constexpr ptrdiff_t mClientUUID = 0xB8;  // ::mce::UUID, 16 байт
    constexpr ptrdiff_t mAuthenticationType = 0xC8;  // ::PlayerAuthenticationType, 4 байт
    constexpr ptrdiff_t mTrustedPlayerInfo = 0xD0;  // ::PlayerAuthenticationInfo, 384 байт
    constexpr ptrdiff_t Size = 0x250;
}  // namespace UserEntityIdentifierComponent

// Value  [src/mc/deps/json/Value.h]  расчётный размер 0x40
namespace Value {
    constexpr ptrdiff_t cstr_ = 0x0;  // char*, 8 байт
    constexpr ptrdiff_t array_ = 0x8;  // ::std::vector<::Json::Value*>*, 8 байт
    constexpr ptrdiff_t bool_ = 0x10;  // bool, 8 байт
    constexpr ptrdiff_t int_ = 0x18;  // int64, 8 байт
    constexpr ptrdiff_t map_ = 0x20;  // ::std::map<::Json::Value::CZString, ::Json::Value, ::Json::Value::CZStringCompare>*, 8 байт
    constexpr ptrdiff_t real_ = 0x28;  // double, 8 байт
    constexpr ptrdiff_t string_ = 0x30;  // ::Json::Value::CZString*, 8 байт
    constexpr ptrdiff_t uint_ = 0x38;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace Value

// VanillaCameraAPIComponent  [src-client/mc/deps/vanilla_camera/VanillaCameraAPIComponent.h]  расчётный размер 0x8
namespace VanillaCameraAPIComponent {
    constexpr ptrdiff_t mApi = 0x0;  // ::std::unique_ptr<::IVanillaCameraAPI>, 8 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace VanillaCameraAPIComponent

// VanillaClientGameplayComponent  [src/mc/deps/vanilla_components/VanillaClientGameplayComponent.h]  расчётный размер 0x50
namespace VanillaClientGameplayComponent {
    constexpr ptrdiff_t mSprintTriggerTime = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mJumpRidingTicks = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mJumpTriggerTime = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mFlyTriggerSource = 0xC;  // int, 4 байт
    constexpr ptrdiff_t mScaffoldingDropHeldTime = 0x10;  // int, 4 байт
    constexpr ptrdiff_t mWasChangeHeight = 0x14;  // bool, 1 байт
    constexpr ptrdiff_t mWasJumping = 0x15;  // bool, 1 байт
    constexpr ptrdiff_t mWasSneaking = 0x16;  // bool, 1 байт
    constexpr ptrdiff_t mWasRunning = 0x17;  // bool, 1 байт
    constexpr ptrdiff_t mEmotingLastTick = 0x18;  // bool, 1 байт
    constexpr ptrdiff_t mSprintingOnInput = 0x19;  // bool, 1 байт
    constexpr ptrdiff_t mBoatingInputSwitchRequested = 0x1A;  // bool, 1 байт
    constexpr ptrdiff_t mMinecartInputSwitchRequested = 0x1B;  // bool, 1 байт
    constexpr ptrdiff_t mLastDelta = 0x1C;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mLastPos = 0x28;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPreviousRidingPosition = 0x34;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPreviousRidingDistance = 0x40;  // float, 4 байт
    constexpr ptrdiff_t mStartRidingTick = 0x48;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x50;
}  // namespace VanillaClientGameplayComponent

// VanillaOffsetComponent  [src/mc/entity/components/VanillaOffsetComponent.h]  расчётный размер 0x24
namespace VanillaOffsetComponent {
    constexpr ptrdiff_t mCurrentOffset = 0x0;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPreviousOffset = 0xC;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mClimbingOffset = 0x18;  // ::Vec3, 12 байт
    constexpr ptrdiff_t Size = 0x24;
}  // namespace VanillaOffsetComponent

// VariableMaxAutoStepComponent  [src/mc/entity/components/VariableMaxAutoStepComponent.h]  расчётный размер 0xD
namespace VariableMaxAutoStepComponent {
    constexpr ptrdiff_t mBaseValue = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mControlledValue = 0x4;  // float, 4 байт
    constexpr ptrdiff_t mJumpPreventedValue = 0x8;  // float, 4 байт
    constexpr ptrdiff_t mHardcodedClientSide = 0xC;  // bool, 1 байт
    constexpr ptrdiff_t Size = 0xD;
}  // namespace VariableMaxAutoStepComponent

// VehicleComponent  [src/mc/entity/components/VehicleComponent.h]  расчётный размер 0x18
namespace VehicleComponent {
    constexpr ptrdiff_t mPassengers = 0x0;  // ::std::vector<::StrictActorIDEntityContextPair>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace VehicleComponent

// VehicleInputIntentComponent  [src/mc/deps/vanilla_components/VehicleInputIntentComponent.h]  расчётный размер 0x48
namespace VehicleInputIntentComponent {
    constexpr ptrdiff_t mWASDGroundControlled = 0x0;  // bool, 1 байт
    constexpr ptrdiff_t mWASDFreeCameraControlled = 0x1;  // bool, 1 байт
    constexpr ptrdiff_t mControlledByPlayer = 0x2;  // bool, 1 байт
    constexpr ptrdiff_t mIsClientPredicted = 0x3;  // bool, 1 байт
    constexpr ptrdiff_t mUsePaddleForce = 0x4;  // bool, 1 байт
    constexpr ptrdiff_t mJumpAmount = 0x8;  // int, 4 байт
    constexpr ptrdiff_t mRotation = 0xC;  // ::Vec2, 8 байт
    constexpr ptrdiff_t mLocalMovementVelocity = 0x14;  // ::Vec3, 12 байт
    constexpr ptrdiff_t mPaddleForces = 0x20;  // ::std::array<float, 2>, 8 байт
    constexpr ptrdiff_t mIsPaddling = 0x28;  // ::std::array<bool, 2>, 2 байт
    constexpr ptrdiff_t mDoControllers = 0x2A;  // ::brstd::bitset<3, uchar>, 1 байт
    constexpr ptrdiff_t mControllingEntity = 0x30;  // ::StrictActorIDEntityContextPair, 16 байт
    constexpr ptrdiff_t mCurrentTick = 0x40;  // uint64, 8 байт
    constexpr ptrdiff_t Size = 0x48;
}  // namespace VehicleInputIntentComponent

// VehicleRenderingRidingOffsetComponent  [src/mc/entity/components/VehicleRenderingRidingOffsetComponent.h]  расчётный размер 0x18
namespace VehicleRenderingRidingOffsetComponent {
    constexpr ptrdiff_t mOffsetsInfo = 0x0;  // ::std::vector<::RenderingRidingOffsetInfo>, 24 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace VehicleRenderingRidingOffsetComponent

// VibrationDataComponent  [src/mc/entity/components/VibrationDataComponent.h]  расчётный размер 0x18
namespace VibrationDataComponent {
    constexpr ptrdiff_t mLastVibrationPos = 0x0;  // ::std::optional<::BlockPos>, 16 байт
    constexpr ptrdiff_t mLastVibrationTick = 0x10;  // ::Tick, 8 байт
    constexpr ptrdiff_t Size = 0x18;
}  // namespace VibrationDataComponent

// VibrationListenerComponent  [src/mc/entity/components/VibrationListenerComponent.h]  расчётный размер 0x10
namespace VibrationListenerComponent {
    constexpr ptrdiff_t mVibrationListener = 0x0;  // ::std::weak_ptr<::VibrationListener>, 16 байт
    constexpr ptrdiff_t Size = 0x10;
}  // namespace VibrationListenerComponent

// VolumeBoundsComponent  [src/mc/volume/components/VolumeBoundsComponent.h]  расчётный размер 0x1C
namespace VolumeBoundsComponent {
    constexpr ptrdiff_t mMin = 0x0;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mMax = 0xC;  // ::BlockPos, 12 байт
    constexpr ptrdiff_t mDimension = 0x18;  // ::DimensionType, 4 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace VolumeBoundsComponent

// VolumeCreationDataComponent  [src/mc/volume/components/VolumeCreationDataComponent.h]  расчётный размер 0x40
namespace VolumeCreationDataComponent {
    constexpr ptrdiff_t mJsonIdentifier = 0x0;  // ::std::string, 32 байт
    constexpr ptrdiff_t mInstanceName = 0x20;  // ::std::string, 32 байт
    constexpr ptrdiff_t Size = 0x40;
}  // namespace VolumeCreationDataComponent

// WalkDistComponent  [src/mc/entity/components/WalkDistComponent.h]  расчётный размер 0x8
namespace WalkDistComponent {
    constexpr ptrdiff_t mCurrent = 0x0;  // float, 4 байт
    constexpr ptrdiff_t mPrevious = 0x4;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x8;
}  // namespace WalkDistComponent

// WardenSpawnTrackerComponent  [src/mc/entity/components/WardenSpawnTrackerComponent.h]  расчётный размер 0xC
namespace WardenSpawnTrackerComponent {
    constexpr ptrdiff_t mThreatLevel = 0x0;  // int, 4 байт
    constexpr ptrdiff_t mThreatLevelIncreaseCooldown = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mThreatLevelDecreaseTimer = 0x8;  // int, 4 байт
    constexpr ptrdiff_t Size = 0xC;
}  // namespace WardenSpawnTrackerComponent

// WaterAppearanceClientBiomeJsonComponent  [src/mc/deps/shared_types/v1_21_40/clientbiome/components/WaterAppearanceClientBiomeJsonComponent.h]  расчётный размер 0x1C  ⚠ BASE_UNKNOWN
namespace WaterAppearanceClientBiomeJsonComponent {
    constexpr ptrdiff_t mSurfaceColor = 0x0;  // ::std::optional<::SharedTypes::Color255RGB>, 20 байт
    constexpr ptrdiff_t mSurfaceOpacity = 0x14;  // ::std::optional<float>, 8 байт
    constexpr ptrdiff_t Size = 0x1C;
}  // namespace WaterAppearanceClientBiomeJsonComponent

// WaterMovementComponent  [src/mc/entity/components/WaterMovementComponent.h]  расчётный размер 0x4
namespace WaterMovementComponent {
    constexpr ptrdiff_t mDragFactor = 0x0;  // float, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace WaterMovementComponent

// WaterSplashEffectRequestComponent  [src/mc/entity/components/WaterSplashEffectRequestComponent.h]  расчётный размер 0x4
namespace WaterSplashEffectRequestComponent {
    constexpr ptrdiff_t mSplashType = 0x0;  // ::LiquidSplashRequest::SplashType, 4 байт
    constexpr ptrdiff_t Size = 0x4;
}  // namespace WaterSplashEffectRequestComponent

// WeaponItemComponent  [src/mc/world/item/components/WeaponItemComponent.h]  расчётный размер 0xA8  ⚠ BASE_UNKNOWN
namespace WeaponItemComponent {
    constexpr ptrdiff_t mOnHurtActor = 0x0;  // ::DefinitionTrigger, 56 байт
    constexpr ptrdiff_t mOnHitActor = 0x38;  // ::DefinitionTrigger, 56 байт
    constexpr ptrdiff_t mOnHitBlock = 0x70;  // ::DefinitionTrigger, 56 байт
    constexpr ptrdiff_t Size = 0xA8;
}  // namespace WeaponItemComponent

// WearableItemComponent  [src/mc/world/item/components/WearableItemComponent.h]  расчётный размер 0x20  ⚠ BASE_UNKNOWN
namespace WearableItemComponent {
    constexpr ptrdiff_t mEquipmentSlot = 0x0;  // ::SharedTypes::Legacy::EquipmentSlot, 4 байт
    constexpr ptrdiff_t mProtection = 0x4;  // int, 4 байт
    constexpr ptrdiff_t mHidesPlayerLocation = 0x8;  // bool, 1 байт
    constexpr ptrdiff_t mOnUseSubscription = 0x10;  // ::Bedrock::PubSub::Subscription, 16 байт
    constexpr ptrdiff_t Size = 0x20;
}  // namespace WearableItemComponent

// WitherBossPreAIStepResultComponent  [src/mc/entity/components/WitherBossPreAIStepResultComponent.h]  расчётный размер 0x1
namespace WitherBossPreAIStepResultComponent {
    constexpr ptrdiff_t mWitherBossPreAIStepResult = 0x0;  // ::WitherBossPreAIStepResult, 1 байт
    constexpr ptrdiff_t Size = 0x1;
}  // namespace WitherBossPreAIStepResultComponent

}  // namespace Offsets_1_26
