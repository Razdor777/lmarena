#pragma once

#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <unordered_map>
#include <mutex>

struct ImDrawList;

class InfiniteAura : public ModuleBase<InfiniteAura>
{
public:
    enum class Mode     { Single, Switch };
    enum class Priority { Closest, Armor, HurtBy };
    enum class SwapMode { None, Full, Spoof };
    enum class RenderMode { Lines, Boxes };
    enum class KBDirection { Default, PushBack, PushForward, PushLeft, PushRight, Custom };

    // --- Core ---
    EnumSettingT<Mode>     mMode          = EnumSettingT("Mode", "Target selection mode", Mode::Single, "Single", "Switch");
    EnumSettingT<Priority> mPriority      = EnumSettingT("Priority", "Target priority", Priority::Closest, "Closest", "Armor", "HurtBy");
    NumberSetting          mRange         = NumberSetting("Range", "The range of the aura.", 70.f, 3.f, 200.f, 0.01f);
    BoolSetting            mInfiniteRange = BoolSetting("Infinite Range", "No range limit", false);
    NumberSetting          mAPS           = NumberSetting("APS", "Attacks per second.", 20.f, 1.f, 20.f, 0.01f);
    NumberSetting          mStepDistance  = NumberSetting("Step Distance", "Blocks per packet.", 8.f, 0.1f, 12.f, 0.01f);
    NumberSetting          mSwitchDelay   = NumberSetting("Switch Delay", "Delay between switches (ms)", 0, 0, 2000, 50);

    // --- Lock ---
    BoolSetting   mAutoUnlock      = BoolSetting("Auto Unlock", "Auto unlock if target invalid/far", false);
    NumberSetting mMaxLockDistance = NumberSetting("Max Lock Distance", "Auto unlock if farther", 50.f, 10.f, 200.f, 5.f);
    NumberSetting mLockFOV        = NumberSetting("Lock FOV", "Max angle for lock", 15.f, 5.f, 90.f, 5.f);
    BoolSetting   mShowBindPanel  = BoolSetting("Show Bind Panel", "Show lock key bind button", true);

    // --- Criticals ---
    BoolSetting mCriticals = BoolSetting("Criticals", "Every hit is critical", true);

    // --- Anti-KB ---
    BoolSetting mFullVelocity = BoolSetting("Full Velocity", "Cancel all knockback (0 KB)", false);

    // --- KB ---
    EnumSettingT<KBDirection> mKBDirection  = EnumSettingT("KB Direction", "Knockback direction",
        KBDirection::Default, "Default", "Push Back", "Push Forward", "Push Left", "Push Right", "Custom");
    NumberSetting mKBOffset      = NumberSetting("KB Offset", "Distance from target", 2.5f, 0.5f, 5.f, 0.1f);
    NumberSetting mKBCustomAngle = NumberSetting("KB Angle", "Custom angle", 0.f, 0.f, 360.f, 1.f);

    // --- Filtering ---
    BoolSetting   mOnlySameY    = BoolSetting("Only Same Y", "Only target similar Y", false);
    NumberSetting mYOffset      = NumberSetting("Y Offset", "Max Y difference", 3.f, 0.5f, 10.f, 0.5f);
    BoolSetting   mOnlyOnGround = BoolSetting("Only On Ground", "Only attack on ground", false);

    // --- Behavior ---
    BoolSetting mFollow        = BoolSetting("Follow", "Teleport to target", false);
    BoolSetting mSilentAccept  = BoolSetting("Silent Accept", "Prevent rubber banding", true);
    BoolSetting mIgnoreFriends = BoolSetting("Ignore Friends", "Don't attack friends", true);

    // --- Weapon ---
    EnumSettingT<SwapMode> mSwapMode   = EnumSettingT("Swap", "Weapon swap mode", SwapMode::None, "None", "Full", "Spoof");
    BoolSetting            mHotbarOnly = BoolSetting("Hotbar Only", "Only hotbar items", true);

    // --- Auto Shield ---
    BoolSetting mAutoShield = BoolSetting("Auto Shield", "Use axe or attack behind when target shields", false);

    // --- Visuals ---
    EnumSettingT<RenderMode> mRenderMode      = EnumSettingT("Render Mode", "Path render mode.", RenderMode::Lines, "Lines", "Boxes");
    BoolSetting              mDrawPath        = BoolSetting("Draw Path", "Draw movement path", true);
    BoolSetting              mDrawGhost       = BoolSetting("Draw Ghost", "Draw ghost at attack pos", true);
    BoolSetting              mHighlightLocked = BoolSetting("Highlight Locked", "Highlight locked target", true);
    BoolSetting              mTargetInfo      = BoolSetting("Target Info", "HP bar, armor and distance above the locked target", true);
    BoolSetting              mHitTrail        = BoolSetting("Hit Trail", "Fading afterimages of the previous hits", true);
    NumberSetting            mTrailPoints     = NumberSetting("Trail Points", "How many previous hits to keep", 4.f, 1.f, 12.f, 1.f);
    BoolSetting              mHitSparks      = BoolSetting("Hit Sparks", "Spark burst flying out of the hit point", true);
    BoolSetting              mPathFlow       = BoolSetting("Path Flow", "Animated dots running along the teleport path", true);
    NumberSetting            mPathWidth      = NumberSetting("Path Width", "Teleport path line thickness", 2.f, 1.f, 6.f, 0.5f);

    // --- Debug ---
    // Одна настройка: показывает, что реально происходит с ударами. Нужна,
    // чтобы измерить настоящую задержку сервера, а не гадать по CPS.
    BoolSetting              mDebug          = BoolSetting("Debug", "Sent vs confirmed hits: real delay, real APS, interval history", false);

    InfiniteAura() : ModuleBase("InfiniteAura", "Attack players at any distance via chain TP",
        ModuleCategory::Combat, 0, false)
    {
        addSettings(
            &mMode, &mPriority, &mRange, &mInfiniteRange, &mAPS, &mStepDistance, &mSwitchDelay,
            &mAutoUnlock, &mMaxLockDistance, &mLockFOV, &mShowBindPanel,
            &mCriticals,
            &mFullVelocity,
            &mKBDirection, &mKBOffset, &mKBCustomAngle,
            &mOnlySameY, &mYOffset, &mOnlyOnGround,
            &mFollow, &mSilentAccept, &mIgnoreFriends,
            &mSwapMode, &mHotbarOnly,
            &mAutoShield,
            &mRenderMode, &mDrawPath, &mDrawGhost, &mHighlightLocked,
            &mTargetInfo, &mHitTrail, &mTrailPoints, &mHitSparks,
            &mPathFlow, &mPathWidth,
            &mDebug
        );

        VISIBILITY_CONDITION(mTrailPoints, mHitTrail.mValue);
        VISIBILITY_CONDITION(mPathFlow,    mDrawPath.mValue);
        VISIBILITY_CONDITION(mPathWidth,   mDrawPath.mValue && mRenderMode.mValue == RenderMode::Lines);

        VISIBILITY_CONDITION(mYOffset,         mOnlySameY.mValue);
        VISIBILITY_CONDITION(mSwitchDelay,     mMode.mValue == Mode::Switch);
        VISIBILITY_CONDITION(mHotbarOnly,      mSwapMode.mValue != SwapMode::None);
        VISIBILITY_CONDITION(mRange,           !mInfiniteRange.mValue);
        VISIBILITY_CONDITION(mKBOffset,        mKBDirection.mValue != KBDirection::Default);
        VISIBILITY_CONDITION(mKBCustomAngle,   mKBDirection.mValue == KBDirection::Custom);
        VISIBILITY_CONDITION(mMaxLockDistance, mAutoUnlock.mValue && !mInfiniteRange.mValue);

        mNames = {
            {Lowercase,       "infiniteaura"},
            {LowercaseSpaced, "infinite aura"},
            {Normal,          "InfiniteAura"},
            {NormalSpaced,    "Infinite Aura"}
        };
    }

    // ==================== STATE ====================
    glm::vec3 mRots         = {0, 0, 0};
    bool      mHasTarget    = false;
    uint64_t  mLastAttack   = 0;
    uint64_t  mLastTargetSwitch    = 0;
    int64_t   mLastTargetRuntimeID = -1;
    glm::vec3 mGhostPos     = {0, 0, 0};
    bool      mGhostVisible = false;

    std::vector<glm::vec3> mPacketPositions;
    uint64_t               mLastPathTime = 0;
    std::mutex             mMutex;

    // Визуал: точки прошлых ударов (новые — в конце) и «болит ли он» для вспышки.
    // Массив фиксированного размера, а не vector: шлейф читает рендер-поток, и
    // push_back под ним мог бы ловить реаллокацию в самый неподходящий момент.
    static constexpr int kMaxTrailPoints = 12;
    glm::vec3            mGhostHistory[kMaxTrailPoints] = {};
    int                  mGhostHistoryCount = 0;
    float                mLastLockedHealth  = -1.f;
    uint64_t             mLastHpDropTime    = 0;

    std::unordered_map<int64_t, uint64_t> mLastAttacksByID;
    std::unordered_map<int64_t, uint64_t> mRecentAttackers;

    struct PositionEntry {
        glm::vec3 position;
        uint64_t timestamp;
    };
    std::unordered_map<int64_t, PositionEntry> mLastPositions;

    float       mLastHealth        = 0.f;
    int64_t     mLockedTargetRuntimeID = -1;
    std::string mLockedTargetName;
    int         mLockTargetKey     = VK_CAPITAL;

    // ── Debug: отправлено vs подтверждено ────────────────────────────────
    // Каждое нажатие кладём в очередь; подтверждением считаем ActorEvent HURT
    // по этой же цели. Разница = настоящая задержка сервера. Очередь маленькая
    // и фиксированная: рендер-поток читает её под своим мьютексом.
    struct DebugAttack { uint64_t at; int64_t target; };
    static constexpr int kDebugQueueSize = 64;
    static constexpr int kDebugConfirmWindow = 32;
    std::mutex  mDebugMutex;
    DebugAttack mDebugQueue[kDebugQueueSize]{};
    int         mDebugQueueCount = 0;
    uint64_t    mDbgSent = 0, mDbgConfirmed = 0, mDbgLost = 0, mDbgForeign = 0;
    uint64_t    mDbgLatSum = 0;
    float       mDbgLastMs = 0.f, mDbgAvgMs = 0.f, mDbgMinMs = 0.f, mDbgMaxMs = 0.f;
    int         mDbgHist[5] = { 0, 0, 0, 0, 0 };       // 0-50 / 50-100 / 100-200 / 200-500 / 500+
    uint64_t    mDbgConfirmTimes[kDebugConfirmWindow]{};
    int         mDbgConfirmCount = 0;
    uint64_t    mDbgLastConfirmAt = 0;
    float       mDbgLastIntervalMs = 0.f, mDbgAvgIntervalMs = 0.f;
    uint64_t    mDbgIntervalSum = 0;
    float       mDbgRealAps = 0.f;

    std::unordered_map<int, bool> mLockKeyStates;
    bool mIsBindingLockKey = false;
    bool mBindWaitRelease  = false;

    // ==================== METHODS ====================
    void onEnable()  override;
    void onDisable() override;

    bool           hasCustomData() const override { return true; }
    nlohmann::json serializeCustomData() override;
    void           deserializeCustomData(const nlohmann::json& j) override;

    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void      straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
    glm::vec3 getAttackPosition(class Actor* target, class Actor* player);
    int       getBestWeapon();
    int       getArmorPieces(class Actor* actor);

    bool      isActorSafeToUse(class Actor* actor);
    bool      isValidTarget(class Actor* actor, class Actor* player);
    bool      hasShieldInOffhand(class Actor* target);

    bool         isLockKeyJustPressed(int vk);
    class Actor* raycastToActor(float maxAngle);
    class Actor* getLockedTarget();
    void         clearLockedTarget();
    bool         isLockedTargetValid();
    void         updateLockTarget();
    class Actor* findActorByLockedName();
    const char*  getKeyName(int vk);

    static bool sIsAnyKeyHeld();
    static int  sFindHeldKey();

    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    // Debug (Task 16): измеряем реальную задержку ударов
    void dbgRecordSent(int64_t targetId);
    void dbgRecordHit(int64_t targetId);
    void dbgReset();
    void dbgRender(ImDrawList* drawList);

    std::string getSettingDisplay() override
    {
        if (mIsBindingLockKey) return "Binding...";
        std::string r = (mLockedTargetRuntimeID != -1)
            ? "Lock[" + std::string(getKeyName(mLockTargetKey)) + "]"
            : (mInfiniteRange.mValue ? "Inf" : std::to_string((int)mRange.mValue) + "b");
        if (mKBDirection.mValue != KBDirection::Default) r += " KB";
        if (mCriticals.mValue)                           r += " Crit";
        return r;
    }
};