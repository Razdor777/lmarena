#pragma once
//
// Created by vastrakai on 7/10/2024.
//

#include <Features/Modules/Module.hpp>

class Scaffold : public ModuleBase<Scaffold> {
public:
    enum class RotateMode {
        None,
        Normal,
        Down,
        Backwards
    };

    enum class FlickMode {
        None,
        Combat,
        Always
    };

    enum class PlacementMode {
        Normal,
        Flareon
    };

    enum class SwitchMode {
        None,
        Full,
        Fake,
        Spoof
    };

    enum class SwitchPriority {
        First,
        Highest
    };

    enum class TowerMode {
        Vanilla,
        Velocity,
        Clip
    };

    enum class BlockHUDStyle {
        None,
        Solstice,
    };

    // Мосты. Меняют только геометрию и ритм постановки: куда уходит блок,
    // под каким углом наклонена голова и с какой частотой всё ставится.
    // Клавиши движения не трогаются — идёшь так, как идёшь.
    enum class BridgeMode {
        Normal,     // всё строго по настройкам ниже
        GodBridge,  // смотрит почти вертикально вниз, прыгает, держит спринт
        Breezily,   // ~58° вниз, без авто-прыжков, ставит по два блока за тик
        Moonwalk,   // ставит блоки ЗА собой: идёшь вперёд — мост растёт за спиной
        Telly       // прыгает и ставит только в воздухе, по три блока за тик
    };

    // Пресет моста. Разбирается на лету, ничего не хранит.
    struct BridgeProfile {
        float pitchDeg = -1.f;   // >= 0 → переопределяет наклон головы
        float extendDir = 1.f;   // +1 вперёд, -1 за спину
        float extendBias = 0.f;  // смещение точки постановки, в блоках
        int places = 0;          // 0 → берём mPlaces
        bool airOnly = false;    // ставить только в воздухе
        bool autoJump = false;
        bool sprint = false;
    };

    NumberSetting mPlaces = NumberSetting("Places", "The amount of blocks to place per tick", 1, 0, 20, 0.01);
    NumberSetting mRange = NumberSetting("Range", "The range at which to place blocks", 5, 0, 10, 0.01);
    NumberSetting mExtend = NumberSetting("Extend", "The distance to extend the placement", 3, 0, 10, 1);
    EnumSettingT<RotateMode> mRotateMode = EnumSettingT<RotateMode>("Rotate Mode", "The mode of rotation", RotateMode::Normal, "None", "Normal", "Down", "Backwards");
    EnumSettingT<FlickMode> mFlickMode = EnumSettingT<FlickMode>("Flick Mode", "The mode for block flicking", FlickMode::Combat, "None", "Combat", "Always");
    EnumSettingT<PlacementMode> mPlacementMode = EnumSettingT<PlacementMode>("Placement", "The mode for block placement", PlacementMode::Normal, "Normal", "Flareon");
    EnumSettingT<SwitchMode> mSwitchMode = EnumSettingT<SwitchMode>("Switch Mode", "The mode for block switching", SwitchMode::Full, "None", "Full", "Fake", "Spoof");
    EnumSettingT<SwitchPriority> mSwitchPriority = EnumSettingT<SwitchPriority>("Switch Prio", "The priority for block switching", SwitchPriority::First, "First", "Highest");
    BoolSetting mHotbarOnly = BoolSetting("Hotbar Only", "Whether or not to only place blocks from the hotbar", false);
    EnumSettingT<TowerMode> mTowerMode = EnumSettingT<TowerMode>("Tower Mode", "The mode for tower placement", TowerMode::Vanilla, "Vanilla", "Velocity", "Clip");
    NumberSetting mTowerSpeed = NumberSetting("Tower Speed", "The speed for tower placement", 8.5, 0, 20, 0.01);
    BoolSetting mFallDistanceCheck = BoolSetting("Fall Distance Check", "Whether or not to check fall distance before towering", false);
    BoolSetting mAllowMovement = BoolSetting("Allow Movement", "Whether or not to allow movement while towering", false);
    EnumSettingT<BlockHUDStyle> mBlockHUDStyle = EnumSettingT<BlockHUDStyle>("HUD Style", "The style for the block HUD", BlockHUDStyle::Solstice, "None", "Solstice");
    BoolSetting mAvoidUnderplace = BoolSetting("Avoid Underplace", "Whether or not to avoid underplacing", false);
    BoolSetting mFastClutch = BoolSetting("Fast Clutch", "Whether or not to use fast clutch", false);
    NumberSetting mClutchFallDistance = NumberSetting("Clutch Fall Dist", "The fall distance to clutch at", 3, 0, 20, 0.01);
    NumberSetting mCluchPlaces = NumberSetting("Clutch Places", "The amount of blocks to place per tick", 1, 0, 20, 0.01);
    BoolSetting mLockY = BoolSetting("Lock Y", "Whether or not to lock the Y position", false);
    BoolSetting mSwing = BoolSetting("Swing", "Whether or not to swing the arm", false);
    BoolSetting mTest = BoolSetting("Diagonal bypass", "Test", false);

    EnumSettingT<BridgeMode> mBridgeMode = EnumSettingT<BridgeMode>("Bridge Mode", "Bridging style: where the block goes, where the head looks and the rhythm", BridgeMode::Normal, "Normal", "GodBridge", "Breezily", "Moonwalk", "Telly");
    BoolSetting mVisibleRotate = BoolSetting("Visible Rotate", "Actually turn the head to the block instead of only spoofing the packet rotation", true);
    NumberSetting mRotateSpeed = NumberSetting("Rotate Speed", "How fast the head turns down to the block", 1.6, 0.2, 5, 0.05);
    NumberSetting mRotateHold = NumberSetting("Rotate Hold", "How long the head stays on the block (ms)", 70, 0, 500, 5);
    BoolSetting mRotateFirst = BoolSetting("Rotate First", "Place only after the head reached the block (looks legit, a bit slower)", false);
    BoolSetting mAutoJump = BoolSetting("Auto Jump", "Jump automatically while bridging (Telly / GodBridge)", false);
    NumberSetting mJumpDelay = NumberSetting("Jump Delay", "Delay between automatic jumps (ms)", 150, 0, 1000, 5);
    BoolSetting mKeepSprint = BoolSetting("Keep Sprint", "Force sprint while bridging", false);
    BoolSetting mHumanize = BoolSetting("Humanize", "Aim slightly off-centre and let the head overshoot a touch — looks like a real player", true);

    Scaffold() : ModuleBase("Scaffold", "Automatically places blocks below you", ModuleCategory::Player, 0, false) {
        addSettings(
            &mPlaces,
            &mRange,
            &mExtend,
            &mRotateMode,
            &mFlickMode,
            &mPlacementMode,
            &mSwitchMode,
            &mSwitchPriority,
            &mHotbarOnly,
            &mTowerMode,
            &mTowerSpeed,
            &mFallDistanceCheck,
            &mAllowMovement,
            &mBlockHUDStyle,
            &mAvoidUnderplace,
            &mFastClutch,
            &mClutchFallDistance,
            &mCluchPlaces,
            &mLockY,
            &mSwing,
            &mTest,
            &mBridgeMode,
            &mVisibleRotate,
            &mRotateSpeed,
            &mRotateHold,
            &mRotateFirst,
            &mAutoJump,
            &mJumpDelay,
            &mKeepSprint,
            &mHumanize);

        VISIBILITY_CONDITION(mFlickMode, mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mSwitchPriority, mSwitchMode.mValue != SwitchMode::None);
        VISIBILITY_CONDITION(mHotbarOnly, mSwitchMode.mValue != SwitchMode::None);
        VISIBILITY_CONDITION(mTowerSpeed, mTowerMode.mValue != TowerMode::Vanilla);
        VISIBILITY_CONDITION(mClutchFallDistance, mFastClutch.mValue);
        VISIBILITY_CONDITION(mCluchPlaces, mFastClutch.mValue);
        VISIBILITY_CONDITION(mVisibleRotate, mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotateSpeed, mVisibleRotate.mValue && mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotateHold, mVisibleRotate.mValue && mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotateFirst, mVisibleRotate.mValue && mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mJumpDelay, mAutoJump.mValue);

        mNames = {
            {Lowercase, "scaffold"},
            {LowercaseSpaced, "scaffold"},
            {Normal, "Scaffold"},
            {NormalSpaced, "Scaffold"}
        };

        gFeatureManager->mDispatcher->listen<RenderEvent, &Scaffold::onRenderEvent>(this);
    }

    float mStartY = 0;
    glm::vec3 mLastBlock = { 0, 0, 0 };
    int mLastFace = -1;
    bool mShouldRotate = false;
    uint64_t mLastSwitchTime = 0;
    int mLastSlot = -1;
    bool mShouldClip = false;
    bool mIsTowering = false;

    // ── видимая ротация (флик головой) ────────────────────────────────────
    bool mFlickActive = false;
    uint64_t mFlickStart = 0;
    glm::vec2 mFlickTarget = { 0.f, 0.f };   // {pitch, yaw} в MC-градусах
    glm::vec2 mFlickBase = { 0.f, 0.f };     // ротация игрока на момент старта флика
    glm::vec2 mUserRot = { 0.f, 0.f };       // текущая «настоящая» ротация игрока (MC-градусы)
    glm::vec2 mLastApplied = { 0.f, 0.f };   // что мы вписали в камеру в прошлом кадре
    bool mOverrodeCamera = false;

    // ── авто-прыжки ──────────────────────────────────────────────────────
    bool mDidForceJump = false;
    uint64_t mLastJump = 0;

    // ── «человечность» ───────────────────────────────────────────────────
    // Разброс прицела пересчитывается ТОЛЬКО при смене блока, иначе
    // голова дрожала бы на каждом тике
    glm::vec3 mAimJitter    = glm::vec3(0.f);
    glm::vec3 mAimCell      = glm::vec3(-99999.f);
    float     mAimPitchBias = 0.f;
    float     mAimYawBias   = 0.f;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    bool tickPlace(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onLookInputEvent(class LookInputEvent& event);

    BridgeProfile getBridgeProfile() const;
    glm::vec2 getTargetRots();                 // {pitch, yaw} в MC-градусах
    void startFlick();                          // начать «кивок» головой к блоку
    glm::vec2 sampleFlick(uint64_t now, bool& active); // ротация флика на этот кадр
    bool flickAligned(float tolerance);
    void updateAutoJump(class BaseTickEvent& event);
    void releaseForcedJump();
    glm::vec3 getRotBasedPos(float extend, float yPos);
    glm::vec3 getPlacePos(float extend);

    std::string getSettingDisplay() override {
        if (mBridgeMode.mValue != BridgeMode::Normal)
            return mBridgeMode.mValues[mBridgeMode.as<int>()];
        return mRotateMode.mValues[mRotateMode.as<int>()];
    }
};
