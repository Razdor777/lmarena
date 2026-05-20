#pragma once
// ============================================
// Данные из анализа IDA Pro — Minecraft Bedrock
// Обновлено после анализа ServerPlayer функций
// ============================================

// Container Vtable индексы
namespace ContainerVtable {
    constexpr int getType             = 8;
    constexpr int getItem             = 56;
    constexpr int setItemForceBalance = 64;
    constexpr int getContainerSize    = 160;
    constexpr int setArmorItem        = 600;
}

// ServerPlayer offsets (подтверждены деструктором)
// sizeof(ServerPlayer) = 4496
namespace ServerPlayerOffsets {
    // Базовые (от Player)
    constexpr size_t Registry         = 16;
    constexpr size_t EntityId         = 24;
    constexpr size_t Position         = 624;   // float[3]
    constexpr size_t OpenContainer    = 544;   // shared_ptr obj
    constexpr size_t OpenContainerCtrl = 552;  // shared_ptr ctrl
    constexpr size_t Level            = 560;
    
    // Swing animation
    constexpr size_t SwingPercent     = 1124;  // float
    constexpr size_t SwingProgress    = 1128;  // int
    constexpr size_t IsSwinging       = 1156;  // bool
    
    // Container sync
    constexpr size_t ContainerOpen    = 1552;  // shared_ptr
    constexpr size_t ContainerCtrl    = 1560;
    constexpr size_t ContainerInfo    = 1576;
    
    // Pathfinding
    constexpr size_t HasPathfinding   = 1969;  // bool
    constexpr size_t PathfindingFlag2 = 1970;  // bool (from sub_140C66EA0)
    constexpr size_t PathfindingSystem = 2184; // ptr
    
    // Packet sender
    constexpr size_t PacketSender     = 2200;  // ptr to queue
    
    // Chunk tracking
    constexpr size_t NeedsChunkLoad   = 2954;  // bool
    constexpr size_t ChunkRadius      = 3076;  // int
    
    // Chunk manager
    constexpr size_t ChunkManager     = 2880;  // ptr (136 bytes obj)
    
    // Spawn position (from sub_140C66EA0)
    constexpr size_t SpawnX           = 2932;  // int
    constexpr size_t SpawnY           = 2936;  // int
    constexpr size_t SpawnZ           = 2940;  // int
    constexpr size_t SpawnDimension   = 2944;  // int
    
    // Container manager
    constexpr size_t ContainerManager = 3376;  // vtable ptr
    
    // Portal
    constexpr size_t PortalIntensity  = 3488;  // float
    constexpr size_t PrevPortalInt    = 3492;  // float
    constexpr size_t LastReportTime   = 3496;  // int64
    constexpr size_t ServerPlayerInit = 3512;  // state data
    
    // Network handler
    constexpr size_t NetworkHandler   = 3544;  // ptr
    constexpr size_t DisconnectTimer  = 3552;  // int
    
    // Movement delta (from sub_140C653F0)
    constexpr size_t MoveDeltaX       = 3556;  // float
    constexpr size_t MoveDeltaY       = 3560;  // float
    constexpr size_t MoveDeltaZ       = 3564;  // float
    
    // Rotation tracking (from sub_140C65940)
    constexpr size_t LastYawRotation  = 3572;  // float
    constexpr size_t AccumulatedYaw   = 3580;  // float
    constexpr size_t RotationInited   = 3584;  // bool
    
    constexpr size_t InPortal         = 3585;  // bool
    
    // ItemStack Net Managers (152 bytes each)
    constexpr size_t ItemStackNet1    = 3592;
    constexpr size_t ItemStackNet2    = 3744;
    constexpr size_t ContainerData    = 3872;
    constexpr size_t SelectedSlotCache = 3896; // int
    constexpr size_t ContainerSyncByte = 3900; // byte (default=247)
    constexpr size_t ForceResync      = 3901;  // bool
    constexpr size_t TickCounter      = 3904;  // uint64 (mod 1200)
    constexpr size_t DisconnectPending = 3936; // bool
    constexpr size_t PortalForced     = 3937;  // bool
    constexpr size_t Initialized      = 3939;  // bool
    constexpr size_t IsRespawning     = 3940;  // bool
    
    // Hash table / network
    constexpr size_t NetworkName      = 3944;  // SSO string
    constexpr size_t HashTable        = 3976;  // struct
    constexpr size_t ExtraString      = 4048;  // SSO string
    
    // ItemStack Net Manager 3
    constexpr size_t ItemStackNet3    = 4088;

    // Timers
    constexpr size_t CountdownTimer1  = 4240;  // int
    constexpr size_t PackedChunkRadius = 4296; // int
    
    // Skin/Renderer
    constexpr size_t SkinRenderer     = 4256;
    constexpr size_t ChunkLoadManager = 4264;
    
    // Spawn state machine
    constexpr size_t AlwaysSendUpdates = 4312; // bool
    constexpr size_t CountdownTimer2  = 4328;  // int
    constexpr size_t DebugName        = 4336;  // SSO string
    
    // Abilities
    constexpr size_t Abilities        = 4368;
    
    // Entity init
    constexpr size_t EntityInitialized = 4488; // bool
    
    // Total size
    constexpr size_t TotalSize        = 4496;
}

// Player base offsets (from analyzed functions)
namespace PlayerOffsets {
    constexpr size_t Health           = 916;   // float (from sub_140C66BC0)
    constexpr size_t Score            = 1264;  // int (from sub_140C66A60/sub_140C66AC0)
    constexpr size_t HurtTime         = 500;   // int
    constexpr size_t HurtTimeCopy     = 504;   // int
    constexpr size_t MaxHurtTime      = 1032;  // int
    constexpr size_t HurtTimePtr      = 1144;  // int* ptr
    constexpr size_t AttackTime       = 1132;  // int (from sub_140C66EA0)
    constexpr size_t PortalBool       = 1968;  // bool (from sub_140C66EA0)
    
    // Riding/Vehicle (from sub_140C670A0)
    constexpr size_t RidingRotation   = 640;   // float* ptr
    
    // Use item system (from sub_140C66BC0)
    constexpr size_t UseItemStartTick = 1992;  // int (from sub_140C665F0)
    constexpr size_t UseItemData      = 2000;  // UseItem struct
    constexpr size_t UseItemDataPtr   = 2008;  // ptr
}

// ECS Component Hashes
namespace ECSHashes {
    constexpr uint32_t ServerPlayer     = 3562053035;
    constexpr uint32_t PlayerFlag       = 3046315770;
    constexpr uint32_t ServerPosition   = 2390486057;
    constexpr uint32_t ServerRotation   = 1691951361;
    constexpr uint32_t ChunkTracker     = 4226230442u;
    constexpr uint32_t PlayerState      = 555318230;
    constexpr uint32_t AbilityComponent = 3286268323u; // 0xC3E299A3
    constexpr uint32_t MovementComponent = 3631693529u; // 0xD8773ED9 (from sub_140C653F0)
}

// Vtable offsets (from analyzed functions)
namespace VtableOffsets {
    // NetworkHandler vtable
    namespace NetworkHandler {
        constexpr int GetActorData      = 2312;  // getActorData()
        constexpr int GetChunkRadius    = 1552;  // getChunkRadius related
        constexpr int GetMovementData   = 3160;  // movement component
        constexpr int IsFlying          = 1360;  // isFlying check
        constexpr int HasRider          = 2648;  // hasRider
        constexpr int SendPacket        = 16;    // sendPacket
        constexpr int IsSneaking        = 480;   // isSneaking
        constexpr int GetPlayerCamera   = 1504;  // camera related
        constexpr int GetInterpolation  = 1688;  // interpolation data
    }
    
    // Actor/Entity vtable
    namespace Actor {
        constexpr int GetLevel          = 1776;  // getLevel
        constexpr int IsPlayer          = 384;   // isPlayer check
        constexpr int GetRotation       = 360;   // setRotation
        constexpr int CanRide           = 2304;  // riding check
    }
    
    // Item vtable (from sub_140C66B30)
    namespace Item {
        constexpr int IsFood            = 8;     // isFood check
    }
}

// ActorData offsets (from bone/rotation calculations)
namespace ActorDataOffsets {
    constexpr size_t VelocityX        = 108;   // float
    constexpr size_t VelocityY        = 112;   // float
    constexpr size_t VelocityZ        = 116;   // float
    constexpr size_t StoredVelocity   = 512;   // Vec3 (from sub_140C653F0)
    constexpr size_t Pitch            = 588;   // float (from sub_140C65940)
    constexpr size_t Yaw              = 592;   // float
    constexpr size_t HeadYaw          = 596;   // float
    constexpr size_t BodyYaw          = 608;   // float
    constexpr size_t RenderFlags      = 626;   // short
    constexpr size_t InterpolationMode = 784;  // int
}

// Movement component offsets (from sub_140C665F0)
namespace MovementComponentOffsets {
    constexpr size_t SpeedModifier    = 132;   // float
}

// Riding entity offsets (from sub_140C653F0)
namespace RidingOffsets {
    constexpr size_t RidingFlags      = 616;   // int (bitmask 0x20100 = 131328)
}

// Skin geometry строки для SlimSkins модуля
// SerializedSkin::mDefaultGeometryName (+0x80)
// "geometry.humanoid.custom"     = Steve
// "geometry.humanoid.customSlim" = Alex

// Bone indices (from sub_140C653F0)
namespace BoneIndices {
    constexpr int Root            = 2;
    constexpr int Body            = 10;
    constexpr int Head            = 14;
    constexpr int Neck            = 15;
    constexpr int RightArm        = 20;
}

// Packet types (from sub_140C65250)
namespace PacketTypes {
    constexpr short DefaultSync   = 247;     // v14 default value
}

// ============================================
// Данные из claude-sonnet-4-6.ini (IDA Pro дамп)
// ActorPhysicsUpdate — 35+ capability flags
// ============================================
namespace ActorPhysicsCapabilities {
    // Capability bit flags from ActorPhysicsUpdate reverse
    constexpr uint32_t HasGravity          = 0x01;
    constexpr uint32_t HasCollision        = 0x02;
    constexpr uint32_t HasFriction         = 0x04;
    constexpr uint32_t CanSwim             = 0x08;
    constexpr uint32_t CanClimb            = 0x10;
    constexpr uint32_t CanFly              = 0x20;
    constexpr uint32_t HasBuoyancy         = 0x40;
    constexpr uint32_t NoClipMode          = 0x80;
    constexpr uint32_t HasKnockback        = 0x100;
    constexpr uint32_t WaterDrag           = 0x200;
    constexpr uint32_t LavaDrag            = 0x400;
    constexpr uint32_t IceSlide            = 0x800;
    constexpr uint32_t BlockSlowdown       = 0x1000;
    constexpr uint32_t StepUp              = 0x2000;
    constexpr uint32_t GroundCheck         = 0x4000;
    constexpr uint32_t ServerMovement      = 0x8000;
    constexpr uint32_t PistonPush          = 0x10000;
    constexpr uint32_t SneakMovement       = 0x20000;
    constexpr uint32_t SprintMovement      = 0x40000;
    constexpr uint32_t FallDamage          = 0x80000;
    constexpr uint32_t GlideFlight         = 0x100000;
}

// Movement ECS System vtable offsets
namespace MovementSystemVtables {
    constexpr uintptr_t MoveSpeedCap       = 0;  // needs runtime resolve
    constexpr uintptr_t NoClipSystem       = 0;  // needs runtime resolve
    constexpr uintptr_t BlockSlowdownSys   = 0;  // needs runtime resolve
    constexpr uintptr_t GravitySystem      = 0;  // needs runtime resolve
    constexpr uintptr_t FrictionSystem     = 0;  // needs runtime resolve
    constexpr uintptr_t BuoyancySystem     = 0;  // needs runtime resolve
    // Above zeroes need to be filled with actual vtable addresses from runtime scanning
    // Pattern: "48 89 5C 24 ? 57 48 83 EC 20 48 8B DA 48 8B F9 E8 ? ? ? ? 48"
}

// Hitbox/AABB structure offsets
namespace HitboxOffsets {
    constexpr size_t MinX                  = 0;   // float
    constexpr size_t MinY                  = 4;   // float
    constexpr size_t MinZ                  = 8;   // float
    constexpr size_t MaxX                  = 12;  // float
    constexpr size_t MaxY                  = 16;  // float
    constexpr size_t MaxZ                  = 20;  // float
    constexpr size_t Width                 = 24;  // float (computed)
    constexpr size_t Height                = 28;  // float (computed)
}

// Fog System offsets (from rendering analysis)
namespace FogSystemOffsets {
    // FogDefinition structure
    constexpr size_t FogStart              = 0;   // float — distance where fog starts
    constexpr size_t FogEnd                = 4;   // float — distance where fog is fully opaque
    constexpr size_t FogColorR             = 8;   // float — red channel
    constexpr size_t FogColorG             = 12;  // float — green channel
    constexpr size_t FogColorB             = 16;  // float — blue channel
    constexpr size_t FogColorA             = 20;  // float — alpha channel
    constexpr size_t FogDensity            = 24;  // float — fog density multiplier
    constexpr size_t FogType               = 28;  // int — 0=linear, 1=exponential, 2=exp2
}

// Render Pipeline Stage IDs (22 stages)
namespace RenderStages {
    constexpr int WorldGeometry            = 0;
    constexpr int Entities                 = 1;
    constexpr int Sky                      = 2;
    constexpr int Clouds                   = 3;
    constexpr int Weather                  = 4;
    constexpr int Water                    = 5;
    constexpr int Particles                = 6;
    constexpr int UI                       = 7;
    constexpr int PostProcess              = 8;
    constexpr int ShadowMap                = 9;
    constexpr int ScreenEffect             = 10;
    // Additional stages exist but are not commonly hooked
}

// Entity Component Registry — confirmed ECS component hashes
// Duplicated from ECSHashes for clarity, with additional entries
namespace EntityComponents {
    constexpr uint32_t PositionComponent   = 2390486057u; // confirmed (ServerPosition)
    constexpr uint32_t RotationComponent   = 1691951361u; // confirmed (ServerRotation)
    constexpr uint32_t MovementComp        = 3631693529u; // confirmed (0xD8773ED9)
    constexpr uint32_t AbilityComp         = 3286268323u; // confirmed (0xC3E299A3)
    constexpr uint32_t PlayerFlag          = 3046315770u; // confirmed
    constexpr uint32_t ServerPlayerComp    = 3562053035u; // confirmed
    // Full list of 100+ components available in claude-sonnet-4-6.ini
}

// Skin Geometry names (for custom skin modules)
namespace SkinGeometry {
    constexpr const char* Steve            = "geometry.humanoid.custom";
    constexpr const char* Alex             = "geometry.humanoid.customSlim";
    constexpr const char* AmongUs          = "geometry.Among_Us_Skinpack.Amogus";
}