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

    // Мосты. Меняют только геометрию и ритм постановки: где появляется блок,
    // сколько клеток за тик имеет смысл поставить и когда прыгать. Клавиши
    // движения не трогаются — идёшь так, как идёшь. Голова во всех режимах
    // смотрит В блок, а не в фиксированный угол.
    enum class BridgeMode {
        Normal,     // всё строго по настройкам ниже
        GodBridge,  // прыгает на каждом краю, держит спринт, ставит вплотную
        Breezily,   // без прыжков, по диагонали ставит две клетки за тик
        Moonwalk,   // ставит блоки ЗА собой: идёшь вперёд — мост растёт за спиной
        Telly       // прыгает и ставит только в воздухе, дальше впереди
    };

    // Пресет моста. Разбирается на лету, ничего не хранит.
    // Угол головы больше не задаётся числом: голова всегда смотрит В блок,
    // который ставится (фиксированные 84° у GodBridge смотрели мимо грани,
    // сервер отклонял постановку — и игрок падал).
    struct BridgeProfile {
        float lookAhead = 0.f;   // за сколько блоков до края ставить (0 → 0.9)
        float extendDir = 1.f;   // +1 вперёд, -1 за спину
        int places = 0;          // 0 → берём mPlaces
        bool airOnly = false;    // ставить только в воздухе
        bool autoJump = false;
        bool sprint = false;
    };

    NumberSetting mPlaces = NumberSetting("Places", "The amount of blocks to place per tick", 1, 0, 20, 0.01);
    NumberSetting mRange = NumberSetting("Range", "The range at which to place blocks", 5, 0, 10, 0.01);
    NumberSetting mExtend = NumberSetting("Extend", "The distance to extend the placement", 3, 0, 10, 1);
    // Как живой игрок: блок ставится, только когда ты вот-вот на него
    // наступишь (под точкой, где ноги будут через ~2 тика), и не дальше
    // одного блока вперёд — никаких блоков «за 1-3 до того, как дошёл».
    BoolSetting mLegitPlace = BoolSetting("Legit Place", "Place a block only right before you step onto it (never 1-3 blocks ahead)", true);
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
    BoolSetting mSwing = BoolSetting("Swing", "Whether or not to swing the arm", false);
    BoolSetting mTest = BoolSetting("Diagonal bypass", "Test", false);

    EnumSettingT<BridgeMode> mBridgeMode = EnumSettingT<BridgeMode>("Bridge Mode", "Bridging style: where the block goes and the rhythm", BridgeMode::Normal, "Normal", "GodBridge", "Breezily", "Moonwalk", "Telly");
    NumberSetting mRotateSpeed = NumberSetting("Rotate Speed", "How fast the head turns to the block (x18 degrees per tick)", 1.6, 0.2, 5, 0.05);
    NumberSetting mRotateHold = NumberSetting("Rotate Hold", "How long the head keeps looking at the bridge after the last block (ms, min 250)", 300, 0, 1500, 10);
    // Новое имя (было "Rotate First", по умолчанию выключено): старые конфиги
    // не должны выключать то, на чём теперь держатся ротации.
    BoolSetting mRotateFirst = BoolSetting("Look First", "Place a block only when the server already sees you looking at it (rotation first, then the block)", true);
    BoolSetting mAutoJump = BoolSetting("Auto Jump", "Jump automatically while bridging (Telly / GodBridge)", false);
    NumberSetting mJumpDelay = NumberSetting("Jump Delay", "Delay between automatic jumps (ms)", 150, 0, 1000, 5);
    BoolSetting mKeepSprint = BoolSetting("Keep Sprint", "Force sprint while bridging", false);
    BoolSetting mHumanize = BoolSetting("Humanize", "Aim slightly off-centre and let the head overshoot a touch — looks like a real player", true);

    Scaffold() : ModuleBase("Scaffold", "Automatically places blocks below you", ModuleCategory::Player, 0, false) {
        addSettings(
            &mPlaces,
            &mRange,
            &mExtend,
            &mLegitPlace,
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
            &mSwing,
            &mTest,
            &mBridgeMode,
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
        VISIBILITY_CONDITION(mExtend, !mLegitPlace.mValue);
        VISIBILITY_CONDITION(mRotateSpeed, mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotateHold, mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mRotateFirst, mRotateMode.mValue != RotateMode::None);
        VISIBILITY_CONDITION(mJumpDelay, mAutoJump.mValue || mBridgeMode.mValue != BridgeMode::Normal);

        mNames = {
            {Lowercase, "scaffold"},
            {LowercaseSpaced, "scaffold"},
            {Normal, "Scaffold"},
            {NormalSpaced, "Scaffold"}
        };

        gFeatureManager->mDispatcher->listen<RenderEvent, &Scaffold::onRenderEvent>(this);
    }

    glm::vec3 mLastBlock = { 0, 0, 0 };
    int mLastFace = -1;
    bool mShouldRotate = false;
    uint64_t mLastSwitchTime = 0;
    int mLastSlot = -1;
    bool mShouldClip = false;
    bool mIsTowering = false;

    // ── уровень моста ────────────────────────────────────────────────────
    // Клетка, на которой стоит мост (та, на которую ты наступаешь). В воздухе
    // она НЕ поднимается вместе с прыжком: иначе блок уходит в клетку, где
    // стоят ноги, игрока выдавливает из блока — и он падает. Раньше уровень
    // брался из текущей высоты каждый тик, поэтому любой прыжок (в том числе
    // от Auto Jump) строил ступеньку вверх, а проседание на миллиметр —
    // ступеньку вниз. Это и было «падаю каждый раз, когда включён Scaffold».
    int        mBridgeY = INT_MAX;
    bool       mBridgeYValid = false;
    glm::ivec3 mUnderCell = glm::ivec3(INT_MAX);   // клетка моста под ногами
    glm::ivec3 mAheadCell = glm::ivec3(INT_MAX);   // клетка моста по ходу движения
    glm::vec2  mDir = { 0.f, 1.f };                // направление моста (x, z)
    bool       mHasDir = false;
    bool       mNeedJump = false;                  // впереди край → прыжок оправдан
    bool       mPlacedThisTick = false;

    // ── голова: настоящая ротация (только пакет и своя модель) ───────────
    // Камеру не трогаем ни на градус: наклон видят сервер и другие игроки,
    // а не ты. Голова едет к блоку раз в тик и возвращается обратно.
    bool      mHeadActive = false;            // ротация сейчас наша
    glm::vec2 mHeadRot    = { 0.f, 0.f };     // {pitch, yaw} в MC-градусах
    glm::vec2 mHeadTarget = { 0.f, 0.f };     // куда смотреть (блок)
    uint64_t  mWantUntil  = 0;                // до этого момента — смотрим на мост
    glm::vec2 mSentRot    = { 0.f, 0.f };     // что ушло в последнем пакете
    bool      mSentValid  = false;
    uint64_t  mNeedSince  = 0;                // блок ждёт поворота головы с ...
    glm::vec2 mUserRot    = { 0.f, 0.f };     // «настоящая» ротация игрока
    glm::vec2 mLastUserRot = { 0.f, 0.f };    // прошлый шаг (ловим рывок мыши)
    bool      mUserRotValid = false;

    // ── человеческая постановка ───────────────────────────────────────────
    uint64_t  mLastPlaceTime = 0;

    // ── авто-прыжки ──────────────────────────────────────────────────────
    bool      mDidForceJump = false;
    uint64_t  mLastJump = 0;

    // ── «человечность» ───────────────────────────────────────────────────
    // Разброс прицела пересчитывается ТОЛЬКО при смене блока, иначе голова
    // дрожала бы на каждом тике.
    glm::vec3 mAimJitter    = glm::vec3(0.f);
    glm::vec3 mAimCell      = glm::vec3(-99999.f);
    float     mAimPitchBias = 0.f;
    float     mAimYawBias   = 0.f;
    float     mAimDownPitch = 89.9f;  // Down

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    bool tickPlace(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void applyLocalModelRotation(class Actor* player); // своя голова = то, что ушло на сервер

    BridgeProfile getBridgeProfile() const;
    void updatePlacementFrame(class Actor* player);      // уровень моста и клетки этого тика
    bool cellBlocksPlayer(const glm::ivec3& cell) const; // клетка пересекает коробку игрока?
    void collectPlaceCells(class Actor* player, std::vector<glm::ivec3>& out, int maxCells);
    glm::vec2 getTargetRots();                 // {pitch, yaw} в MC-градусах
    bool rotationAllowed() const;               // RotateMode != None и Aura не мешает
    glm::vec2 stepHead(const glm::vec2& user, uint64_t now, bool& override); // шаг головы (тик)
    bool headAligned(const glm::vec2& target) const; // сервер уже смотрит туда
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
