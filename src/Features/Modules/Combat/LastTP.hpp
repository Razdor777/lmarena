#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/EntityHurtEvent.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// LastTP — remembers the LAST combat action involving the local player and
// teleports beside whoever it was:
//
//   1. a player I hit (and who really lost HP), or
//   2. a player who hit me (and made me really lose HP)
//
// Whichever of the two happened LAST wins — see recordCombatEvent(), where the
// events are compared by the moment the attack/swing happened, not by when the
// confirmation arrived.
//
// The combat tracker is passive and lives for the whole session (started in
// onInit), so the very first time you toggle the module there is already data.
// Toggling is a one-shot: enable -> teleport -> instantly disable itself.
class LastTP : public ModuleBase<LastTP>
{
public:
    enum class Direction { Behind, Front, Left, Right, Top };

    EnumSettingT<Direction> mDirection = EnumSettingT(
        "Direction", "Where to land, relative to the target's head rotation",
        Direction::Behind, "Behind", "Front", "Left", "Right", "Top");

    NumberSetting mDistance = NumberSetting(
        "Distance", "Blocks away from the target",
        3.f, 0.1f, 10.f, 0.1f);

    // AutoLock: после телепорта камера (и серверный yaw/pitch) наводится ровно
    // на ту цель, к которой мы телепортировались, и держится так несколько
    // тиков. Наводка считается ТОЛЬКО из хитбокса этой цели, а её личность
    // подтверждается unique id — поэтому прицел не может уехать на другого
    // игрока, даже если цель окружили со всех сторон.
    BoolSetting mAutoLock = BoolSetting(
        "AutoLock", "Aim the crosshair straight at the target after teleporting", false);

    LastTP() : ModuleBase("LastTP",
        "Teleports to the last player you hit or got hit by",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(&mDirection, &mDistance, &mAutoLock);

        mNames = {
            {Lowercase,       "lasttp"},
            {LowercaseSpaced, "last tp"},
            {Normal,          "LastTP"},
            {NormalSpaced,    "Last TP"}
        };
    }

    // ==================== LIFECYCLE ====================
    void onInit() override;    // passive tracker, never removed
    void onEnable() override;  // one-shot: teleport, then disable
    void onDisable() override {}

    // ==================== TRACKED DATA ====================
    // One of my outgoing attacks, waiting for the victim to actually lose HP
    // (this is what filters out gamemode / invulnerable players).
    struct PendingAttack {
        int64_t  uniqueID  = -1;
        int64_t  runtimeID = -1;
        uint64_t time      = 0;
    };

    // A swing packet from another player close to me, waiting for me to lose HP.
    struct Swinger {
        int64_t  runtimeID = -1;
        uint64_t time      = 0;
    };

    std::vector<PendingAttack> mPendingAttacks;
    std::vector<Swinger>       mRecentSwings;

    // Last confirmed interaction. Both directions write here; the newest action
    // (by mTargetEventTime) is the one the module uses. Persists between toggles
    // on purpose, so you can re-enable the module and teleport again.
    int64_t     mTargetRuntimeID = -1;
    int64_t     mTargetUniqueID  = -1;
    uint64_t    mTargetEventTime = 0;     // when the attack/swing happened, not when it was confirmed
    std::string mTargetName      = "None";
    bool        mTargetWasMe     = false; // true when the last action was him hitting me

    // runtimeID -> last seen HP, used to detect damage that was really applied
    std::unordered_map<int64_t, float> mHealthCache;
    float mLocalHealth       = -1.f;
    int   mPrevLocalHurtTime = 0;

    // ==================== AUTOLOCK STATE ====================
    // Идентичность цели фиксируется ДВУМЯ id одновременно: runtime id может
    // переиспользоваться движком под другого игрока, unique id — нет.
    int64_t mLockRuntimeID = -1;
    int64_t mLockUniqueID  = -1;
    int     mLockTicks     = 0;

    static constexpr int kAutoLockTicks = 6; // ~300 мс удержания прицела

    // ==================== TUNABLES ====================
    static constexpr float    kStepDistance       = 8.f;  // ClickTP packet step, fixed
    static constexpr uint64_t kConfirmWindowMs    = 1000; // attack -> damage window
    static constexpr uint64_t kSwingWindowMs      = 800;  // their swing -> my damage window
    static constexpr uint64_t kLooseSwingWindowMs = 3000; // fallback candidate window
    static constexpr uint64_t kKeepPendingMs      = 2500; // how long an entry stays useful
    static constexpr float    kSwingMaxDistance   = 8.f;
    static constexpr float    kFallbackDistance   = 4.f;  // nearest-player fallback
    static constexpr size_t   kMaxEntries         = 32;

    // ==================== EVENTS ====================
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onEntityHurtEvent(struct EntityHurtEvent& event);

    // ==================== INTERNALS ====================
    // Confirmation paths
    void confirmMyAttack(class Actor* victim, uint64_t now);
    void confirmTheirAttack(uint64_t now);
    void recordCombatEvent(class Actor* target, bool iWasHit, uint64_t actionTime);
    void pruneEntries(uint64_t now);

    // Teleport
    class Actor* resolveTarget();
    void attemptTeleport();

    // AutoLock
    class Actor* resolveLockTarget();
    void updateAutoLock();

    std::string getSettingDisplay() override
    {
        if (mTargetRuntimeID == -1 && mTargetUniqueID == -1) return "No target";
        // [hit] = I hit him last, [hurt] = he hit me last.
        return mTargetName + (mTargetWasMe ? " [hurt]" : " [hit]");
    }
};
