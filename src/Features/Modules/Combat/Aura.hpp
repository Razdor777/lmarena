#pragma once

#include <Features/Modules/Module.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

class Aura : public ModuleBase<Aura> {
public:

    enum class AttackMode  { Earliest, Synched };
    enum class RotateMode  { None, Normal, Flick };
    enum class SwitchMode  { None, Full, Spoof };
    enum class BypassMode  { None, Raycast };
    
    enum class ESPStyle    { 
        None,
        Wireframe3D,
        Chams,
        Trail
    };

    NumberSetting mAttackRange   = NumberSetting("Attack Range",  "Distance to attack target",        3.5f, 0.f, 10.f, 0.01f);
    NumberSetting mAimRange      = NumberSetting("Aim Range",     "Distance to start aiming",         5.f,  0.f, 15.f, 0.01f);
    NumberSetting mAPS           = NumberSetting("APS",           "Attacks per second",               10.f, 1.f, 20.f, 0.01f);
    BoolSetting   mRandomizeAPS  = BoolSetting  ("Randomize APS", "Randomize attacks per second",     false);
    NumberSetting mAPSMin        = NumberSetting("APS Min",       "Minimum APS",                      8.f,  1.f, 20.f, 0.01f);
    NumberSetting mAPSMax        = NumberSetting("APS Max",       "Maximum APS",                      12.f, 1.f, 20.f, 0.01f);
    EnumSettingT<AttackMode> mAttackMode = EnumSettingT("Attack Mode", "How to send attack",
        AttackMode::Earliest, "Earliest", "Synched");
    BoolSetting mThroughWalls    = BoolSetting("Through Walls",   "Attack through walls",             true);

    EnumSettingT<SwitchMode> mSwitchMode = EnumSettingT("Switch Mode", "Weapon switching mode",
        SwitchMode::None, "None", "Full", "Spoof");
    BoolSetting mHotbarOnly      = BoolSetting("Hotbar Only",     "Only use hotbar items",            true);
    BoolSetting mFistFriends     = BoolSetting("Fist Friends",    "Use fists on friends",             false);
    BoolSetting mSwing           = BoolSetting("Swing",           "Animate arm swing",                true);

    EnumSettingT<RotateMode> mRotateMode = EnumSettingT("Rotate Mode", "Rotation mode",
        RotateMode::Normal, "None", "Normal", "Flick");
    NumberSetting mAimY          = NumberSetting("Aim Y", "Vertical aim point (0=feet, 1=head)",      0.85f, 0.f, 1.f, 0.01f);
    NumberSetting mAimRandom     = NumberSetting("Aim Random",    "Randomize aim point in hitbox",    0.25f, 0.f, 1.f, 0.01f);
    NumberSetting mRotSpeed      = NumberSetting("Rot Speed",     "Max deg/sec (0=instant)",          55.f,  0.f, 180.f, 1.f);
    NumberSetting mReactionDelay = NumberSetting("Reaction",      "Delay before aim ms",              80.f,  0.f, 500.f, 10.f);

    EnumSettingT<BypassMode> mBypassMode = EnumSettingT("Bypass Mode", "Attack bypass mode",
        BypassMode::Raycast, "None", "Raycast");

    BoolSetting mShowESP         = BoolSetting("Show ESP",        "Draw visuals around target",       true);
    EnumSettingT<ESPStyle> mESPStyle = EnumSettingT("ESP Style", "Visual style around target",
        ESPStyle::Wireframe3D, "None", "Wireframe 3D", "Chams", "Trail");
    NumberSetting mESPAlpha      = NumberSetting("ESP Alpha",     "Transparency of ESP effects",      0.8f, 0.1f, 1.f, 0.01f);
    NumberSetting mTrailLength   = NumberSetting("Trail Length",  "How long trail persists (sec)",    2.f, 0.5f, 5.f, 0.1f);

    BoolSetting mTargetStrafe    = BoolSetting("TargetStrafe",    "Enable TargetStrafe module when fighting", false);
    BoolSetting mAutoDisable     = BoolSetting("Auto Disable",    "Disable on dimension change",      true);

    Aura() : ModuleBase("Aura", "Automatically attacks nearby enemies", ModuleCategory::Combat, 0, false) {
        addSettings(
            &mAttackRange,
            &mAimRange,
            &mAPS,
            &mRandomizeAPS,
            &mAPSMin,
            &mAPSMax,
            &mAttackMode,
            &mThroughWalls,
            &mSwitchMode,
            &mHotbarOnly,
            &mFistFriends,
            &mSwing,
            &mRotateMode,
            &mAimY,
            &mAimRandom,
            &mRotSpeed,
            &mReactionDelay,
            &mBypassMode,
            &mShowESP,
            &mESPStyle,
            &mESPAlpha,
            &mTrailLength,
            &mTargetStrafe,
            &mAutoDisable
        );

        VISIBILITY_CONDITION(mAPSMin,         mRandomizeAPS.mValue);
        VISIBILITY_CONDITION(mAPSMax,         mRandomizeAPS.mValue);
        VISIBILITY_CONDITION(mAPS,            !mRandomizeAPS.mValue);
        VISIBILITY_CONDITION(mAimY,           mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mAimRandom,      mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotSpeed,       mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mReactionDelay,  mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mESPStyle,       mShowESP.mValue);
        VISIBILITY_CONDITION(mESPAlpha,       mShowESP.mValue && mESPStyle.mValue != ESPStyle::None);
        VISIBILITY_CONDITION(mTrailLength,    mShowESP.mValue && mESPStyle.mValue == ESPStyle::Trail);

        mNames = {
            {Lowercase,       "aura"},
            {LowercaseSpaced, "aura"},
            {Normal,          "Aura"},
            {NormalSpaced,    "Aura"}
        };
    }

    int64_t  mTargetRuntimeID = 0;
    bool     mRotating        = false;
    bool     mLmbHeld         = false;
    bool     mRmbClicked      = false;
    bool     mAutoTargeted    = false;
    int64_t  mLastAttack      = 0;
    int      mLastSlot        = 0;

    static inline int64_t  sTargetRuntimeID = 0;
    static inline bool     sHasTarget       = false;
    static inline Actor*   sTarget          = nullptr;

    glm::vec3 mCurrentAimOffset = glm::vec3(0.f);
    glm::vec2 mCurrentRotations = glm::vec2(0.f);
    int64_t   mTargetLockTime   = 0;

    struct TrailPoint {
        glm::vec3 pos;
        int64_t   timestamp;
        float     alpha;
    };
    static inline std::vector<TrailPoint> sTrailHistory;
    static inline int64_t sLastTrailUpdate = 0;

    void resetState();
    void resetTarget();

    bool isValidTarget(Actor* actor, Actor* player) const;
    bool isInAimRange(Actor* actor, Actor* player) const;
    bool isInAttackRange(Actor* actor, Actor* player) const;

    Actor* resolveTarget(Actor* player);
    Actor* findBestTarget(Actor* player);
    Actor* findObstructor(Actor* player, Actor* target);

    int       getBestWeapon(Actor* target);
    glm::vec2 calcRotations(Actor* player);

    void drawWireframe3D(Actor* target, Actor* player, ImColor color, float alpha);
    void drawTrail(Actor* target, ImColor color);
    void drawChams(Actor* target, ImColor color, float alpha);

    void onEnable()  override;
    void onDisable() override;

    void onMouseEvent      (class MouseEvent&      event);
    void onBaseTickEvent   (class BaseTickEvent&   event);
    void onPacketOutEvent  (class PacketOutEvent&  event);
    void onPacketInEvent   (class PacketInEvent&   event);
    void onRenderEvent     (class RenderEvent&     event);
    void onBobHurtEvent    (class BobHurtEvent&    event);
    void onBoneRenderEvent (class BoneRenderEvent& event);

    std::string getSettingDisplay() override {
        return mRotateMode.mValues[mRotateMode.as<int>()];
    }
};