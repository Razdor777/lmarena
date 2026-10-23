//
// Created by vastrakai on 7/10/2024.
//

#include "Scaffold.hpp"

#include <algorithm>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/LookInputEvent.hpp>
#include <Features/Modules/Combat/Aura.hpp>
#include <Features/Modules/Misc/TestModule.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/PacketID.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Actor/Components/ActorHeadRotationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MobBodyRotationComponent.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>

// ═══════════════════════════════════════════════════════════════
// Rotation helpers (file-local)
// ═══════════════════════════════════════════════════════════════
namespace
{
    // Сколько градусов голова проходит за тик (Rotate Speed 1.6 -> ~29°).
    float rotateStepDeg(float speed)
    {
        return 18.f * std::clamp(speed, 0.2f, 5.f);
    }

    float normYawDeg(float yaw)
    {
        while (yaw > 180.f)  yaw -= 360.f;
        while (yaw < -180.f) yaw += 360.f;
        return yaw;
    }

    float normRad(float rad)
    {
        while (rad > IM_PI)  rad -= 2.f * IM_PI;
        while (rad < -IM_PI) rad += 2.f * IM_PI;
        return rad;
    }

    float smoothStep(float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    // MC-градусы {pitch (вниз > 0), yaw} → CameraDirectLookComponent::mRotRads.
    // Формула снята с рабочего Aimbot::calcTargetRotRads:
    //   mRotRads.x = PI - yawMC,   mRotRads.y = -pitchMC
    glm::vec2 mcToRad(const glm::vec2& mc)
    {
        return { normRad(glm::radians(180.f - mc.y)),
                 std::clamp(glm::radians(-mc.x), -1.55f, 1.55f) };
    }

    // Обратная конверсия — чтобы понимать, куда игрок смотрит на самом деле.
    glm::vec2 radToMc(const glm::vec2& rad)
    {
        return { -glm::degrees(rad.y), normYawDeg(180.f - glm::degrees(rad.x)) };
    }

    glm::vec2 lerpRots(const glm::vec2& from, const glm::vec2& to, float t)
    {
        return { from.x + (to.x - from.x) * t,
                 from.y + normYawDeg(to.y - from.y) * t };
    }

    // Наибольшее расхождение по осям, градусы.
    float angDiff(const glm::vec2& a, const glm::vec2& b)
    {
        return std::max(std::fabs(a.x - b.x), std::fabs(normYawDeg(a.y - b.y)));
    }

    // Направление взгляда по MC-ротации {pitch, yaw}.
    glm::vec3 lookDir(const glm::vec2& r)
    {
        const float p = glm::radians(r.x), y = glm::radians(r.y);
        return { -std::sin(y) * std::cos(p), -std::sin(p), std::cos(y) * std::cos(p) };
    }

    // Угол между двумя взглядами, градусы. Глядя почти вертикально вниз,
    // yaw почти ничего не меняет — и этот угол тоже (в отличие от разницы
    // самих yaw).
    float viewAngle(const glm::vec2& a, const glm::vec2& b)
    {
        const float d = std::clamp(glm::dot(lookDir(a), lookDir(b)), -1.f, 1.f);
        return glm::degrees(std::acos(d));
    }

    constexpr float kAlignTolerance = 10.f;    // «смотрит на блок», градусы
    constexpr uint64_t kRotateTimeoutMs = 300; // дольше блок голову не ждёт
    constexpr uint64_t kMinPlaceGapMs = 90;    // чаще живой игрок не ставит
    // Рывок мыши: столько градусов за один пакет (50 мс) = ~500°/с и быстрее.
    // Такой поворот голова не «догоняет» шагами — она встаёт на него сразу.
    constexpr float kFlickDeg = 25.f;
}

Scaffold::BridgeProfile Scaffold::getBridgeProfile() const
{
    BridgeProfile p{};
    switch (mBridgeMode.mValue)
    {
    case BridgeMode::GodBridge:
        p.pitchDeg = 84.f;
        p.places = 2;
        p.autoJump = true;
        p.sprint = true;
        break;
    case BridgeMode::Breezily:
        p.pitchDeg = 58.f;
        p.places = 2;
        p.sprint = true;
        p.extendBias = 0.5f;
        break;
    case BridgeMode::Moonwalk:
        // Блоки уходят ЗА спину: идёшь вперёд — мост растёт за тобой.
        p.pitchDeg = 74.f;
        p.extendDir = -1.f;
        p.places = 1;
        break;
    case BridgeMode::Telly:
        // Прыжок → блоки ставятся только в воздухе, на блок впереди,
        // чтобы приземлиться уже на мост.
        p.pitchDeg = 66.f;
        p.places = 3;
        p.airOnly = true;
        p.autoJump = true;
        p.sprint = true;
        p.extendBias = 1.f;
        break;
    default:
        break;
    }
    return p;
}

void Scaffold::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Scaffold::onBaseTickEvent, nes::event_priority::LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Scaffold::onPacketOutEvent, nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<LookInputEvent, &Scaffold::onLookInputEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    mHeadActive = false;
    mWantUntil = 0;
    mSentValid = false;
    mNeedSince = 0;
    mOverrodeCamera = false;
    mLastApplied = { 0.f, 0.f };
    mUserRotValid = false;
    mLastPlaceTime = 0;
    mAheadCell = glm::ivec3(INT_MAX);
    mDidForceJump = false;
    if (!player) return;

    mStartY = player->getPos()->y - PLAYER_HEIGHT - 1.f;
    mLastSlot = player->getSupplies()->mSelectedSlot;

    if (auto* rot = player->getActorRotationComponent())
        mUserRot = { rot->mPitch, rot->mYaw };
}

void Scaffold::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Scaffold::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Scaffold::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &Scaffold::onLookInputEvent>(this);

    mStartY = 0.f;
    mLastBlock = {0, 0, 0};
    mLastFace = 0;
    mLastSwitchTime = 0;
    mShouldRotate = false;
    mHeadActive = false;
    mWantUntil = 0;
    mSentValid = false;
    mOverrodeCamera = false;
    mUserRotValid = false;

    // Пока мы включены, могли держать прыжок — снимаем, иначе игрок будет
    // прыгать сам по себе уже после выключения модуля.
    releaseForcedJump();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->isValid()) return;

    if (mLastSlot != -1)
    {
        player->getSupplies()->mSelectedSlot = mLastSlot;
    }

    if (mIsTowering)
    {
        mIsTowering = false;
        if (mTowerMode.as<TowerMode>() != TowerMode::Vanilla)
        {
            player->getStateVectorComponent()->mVelocity.y = -5.0f;
        }
    }
}

void Scaffold::releaseForcedJump()
{
    if (!mDidForceJump) return;
    mDidForceJump = false;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->isValid()) return;
    if (auto* moveInput = player->getMoveInputComponent())
        moveInput->setJumping(false);
}

void Scaffold::updateAutoJump(BaseTickEvent& event)
{
    auto player = event.mActor;
    auto moveInput = player->getMoveInputComponent();
    if (!moveInput) return;

    const BridgeProfile prof = getBridgeProfile();
    const bool wantJump = mAutoJump.mValue || prof.autoJump;

    // Только что взлетели — отпускаем прыжок, иначе он будет «залипать».
    if (mDidForceJump && !player->isOnGround())
    {
        moveInput->setJumping(false);
        mDidForceJump = false;
    }

    if (!wantJump || !Keyboard::isUsingMoveKeys())
    {
        releaseForcedJump();
        return;
    }

    if (!player->isOnGround()) return;
    if (static_cast<uint64_t>(NOW) - mLastJump < static_cast<uint64_t>(mJumpDelay.mValue)) return;

    moveInput->setJumping(true);
    mDidForceJump = true;
    mLastJump = static_cast<uint64_t>(NOW);
}

void Scaffold::onBaseTickEvent(BaseTickEvent& event)
{
    mShouldClip = false;
    auto player = event.mActor;
    if (!player || !player->isValid()) return;

    // Своя модель (F5, тени, записи): голова и тело — ровно то, что сейчас
    // уходит на сервер. Без этого Interface рисовал голову по сглаженным
    // пакетам, и в третьем лице она отставала от камеры.
    applyLocalModelRotation(player);

    const BridgeProfile prof = getBridgeProfile();

    updateAutoJump(event);

    // Telly работает только в воздухе: на земле ждём прыжка.
    if (prof.airOnly && player->isOnGround()) return;

    if ((prof.sprint || mKeepSprint.mValue) && Keyboard::isUsingMoveKeys()
        && !ClientInstance::get()->getMouseGrabbed())
    {
        if (auto* moveInput = player->getMoveInputComponent())
            moveInput->mIsSprinting = true;
    }

    int places = prof.places > 0 ? prof.places : mPlaces.as<int>();

    if (mFastClutch.mValue && player->getFallDistance() > mClutchFallDistance.mValue)
    {
        places = mCluchPlaces.as<int>();
    }

    for (int i = 0; i < places; i++)
    {
        if (!tickPlace(event)) break;
        if (mShouldClip) break;
    }
}

bool Scaffold::tickPlace(BaseTickEvent& event)
{
    auto player = event.mActor;

    auto moveInput = player->getMoveInputComponent();
    auto actorRot = player->getActorRotationComponent();
    auto stateVec = player->getStateVectorComponent();

    const BridgeProfile prof = getBridgeProfile();

    auto currentY = player->getPos()->y - 2.62f;
    if (!mLockY.mValue) mStartY = currentY;
    if (player->getPos()->y - 2.62f < mStartY) mStartY = player->getPos()->y - 2.62f;
    if (moveInput->mIsJumping && !Keyboard::isUsingMoveKeys()) mStartY = currentY;

    glm::vec3 velocity = stateVec->mVelocity;

    bool isMoving = Keyboard::isUsingMoveKeys();

    float maxExtend = mExtend.mValue;

    if (ItemUtils::getAllPlaceables(mHotbarOnly.mValue) == 0)
    {
        if (mIsTowering)
        {
            mIsTowering = false;
            stateVec->mVelocity.y = -5.0f;
        }
        return false;
    }

    if (mSwitchMode.mValue == SwitchMode::Fake && mLastSlot != -1) player->getSupplies()->mInHandSlot = mLastSlot;

    glm::vec3 blockPos = getPlacePos(0.f);

    if (!Keyboard::isUsingMoveKeys())
    {
        maxExtend = 0.f;
    }

    auto& keyboard = *ClientInstance::get()->getKeyboardSettings();
    bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
    bool wasTowering = mIsTowering;

    float fallDistance = player->getFallDistance();
    if (!mFallDistanceCheck.mValue) fallDistance = 0.f;
    switch (mTowerMode.mValue)
    {
    default:
        break;
    case TowerMode::Velocity:
        {
            if (ClientInstance::get()->getMouseGrabbed()) break;
            if ((space && mAllowMovement.mValue || space && !isMoving) && fallDistance < 3.f)
            {
                if (!mAllowMovement.mValue)
                {
                    stateVec->mVelocity.x = 0;
                    stateVec->mVelocity.z = 0;
                } else if (!player->isOnGround())
                {
                    glm::vec2 currentMotion = {stateVec->mVelocity.x, stateVec->mVelocity.z};
                    float movementSpeed = sqrt(currentMotion.x * currentMotion.x + currentMotion.y * currentMotion.y);
                    float movementYaw = atan2(currentMotion.y, currentMotion.x);
                    float moveYawDeg = movementYaw * (180 / IM_PI) - 90.f;
                    float playerYawDeg = actorRot->mYaw + MathUtils::getRotationKeyOffset();
                    float yawDiff = playerYawDeg - moveYawDeg;
                    float yawDiffRad = yawDiff * (IM_PI / 180);
                    float newMoveYaw = movementYaw + yawDiffRad;
                    stateVec->mVelocity.x = cos(newMoveYaw) * movementSpeed;
                    stateVec->mVelocity.z = sin(newMoveYaw) * movementSpeed;
                }
                mStartY = player->getPos()->y;
                mIsTowering = true;
                stateVec->mVelocity.y = mTowerSpeed.mValue / 10;
                maxExtend = 0.f;
            }
            else if (wasTowering)
            {
                mIsTowering = false;
                stateVec->mVelocity.y = -5.0f;
            }
            break;
        }
    case TowerMode::Clip:
        {
            if (ClientInstance::get()->getMouseGrabbed()) break;
            if ((space && mAllowMovement.mValue || space && !isMoving) && fallDistance < 3.f)
            {
                if (!mAllowMovement.mValue)
                {
                    stateVec->mVelocity.x = 0;
                    stateVec->mVelocity.z = 0;
                } else if (!player->isOnGround())
                {
                    glm::vec2 currentMotion = {stateVec->mVelocity.x, stateVec->mVelocity.z};
                    float movementSpeed = sqrt(currentMotion.x * currentMotion.x + currentMotion.y * currentMotion.y);
                    float movementYaw = atan2(currentMotion.y, currentMotion.x);
                    float moveYawDeg = movementYaw * (180 / IM_PI) - 90.f;
                    float playerYawDeg = actorRot->mYaw + MathUtils::getRotationKeyOffset();
                    float yawDiff = playerYawDeg - moveYawDeg;
                    float yawDiffRad = yawDiff * (IM_PI / 180);
                    float newMoveYaw = movementYaw + yawDiffRad;
                    stateVec->mVelocity.x = cos(newMoveYaw) * movementSpeed;
                    stateVec->mVelocity.z = sin(newMoveYaw) * movementSpeed;
                }
                mStartY = player->getPos()->y;
                mIsTowering = true;
                maxExtend = 0.f;
                mShouldClip = true;
            }
            else if (wasTowering)
            {
                mIsTowering = false;
                stateVec->mVelocity.y = -5.0f;
            }
        }
    }

    const uint64_t now = static_cast<uint64_t>(NOW);
    const bool falling = player->getFallDistance() > 2.f;

    if (mLegitPlace.mValue && !mIsTowering)
    {
        // Только то, что нужно прямо сейчас: блок под ногами или под точкой,
        // куда ноги придут через ~2 тика. «Скоро понадобится» — голова уже
        // поворачивается, блок поставим, когда дойдём.
        bool soon = false;
        blockPos = getLegitPlacePos(soon);
        if (blockPos.x == FLT_MAX)
        {
            if (soon && rotationAllowed())
            {
                mHeadTarget = getTargetRots();
                mWantUntil = std::max(mWantUntil, now + 250);
            }
            return false;
        }
        // живой игрок не ставит чаще ~10 раз в секунду (падение — исключение)
        if (!falling && now - mLastPlaceTime < kMinPlaceGapMs) return false;
    }
    else if (!BlockUtils::isAirBlock(blockPos) && !mIsTowering)
    {
        for (float i = 0.f; i < maxExtend; i += 1.f)
        {
            blockPos = getPlacePos(i);
            if (BlockUtils::isAirBlock(blockPos)) break;
        }
    }

    if (!BlockUtils::isValidPlacePos(blockPos)) return false;
    if (!BlockUtils::isAirBlock(blockPos)) return false;
    int side = BlockUtils::getBlockPlaceFace(blockPos);

    if (mAvoidUnderplace.mValue && side == 0) return false;

    mLastBlock = blockPos;
    mLastFace = side;

    // ── Сначала голова, потом блок ───────────────────────────────────────
    // Пакет постановки уходит раньше, чем PlayerAuthInput этого тика, — то
    // есть сервер видит блок, поставленный с ПРОШЛОЙ ротацией. Поэтому голова
    // поворачивается к блоку, а ставим его, только когда сервер уже получил
    // этот взгляд (Rotate First). Не дольше kRotateTimeoutMs; при падении и
    // в башне — сразу.
    if (rotationAllowed())
    {
        mHeadTarget = getTargetRots();
        mWantUntil = now + static_cast<uint64_t>(std::max(mRotateHold.mValue, 250.f));
        if (mRotateFirst.mValue && !falling && !mIsTowering && !headAligned(mHeadTarget))
        {
            if (mNeedSince == 0) mNeedSince = now;
            if (now - mNeedSince < kRotateTimeoutMs)
                return false; // в этом тике только поворачиваемся
        }
    }
    mNeedSince = 0;

    mLastSwitchTime = NOW;

    if (mLastSlot == -1) mLastSlot = player->getSupplies()->mSelectedSlot;
    int lastSlot = player->getSupplies()->mSelectedSlot;

    if (mSwitchMode.mValue != SwitchMode::None)
    {
        int slot = ItemUtils::getPlaceableItemOnBlock(blockPos, mHotbarOnly.mValue, mSwitchPriority.mValue == SwitchPriority::Highest);
        if (slot == -1) return false;
        if (mSwitchMode.mValue != SwitchMode::Spoof) player->getSupplies()->mSelectedSlot = slot;
        else
        {
            player->getSupplies()->mSelectedSlot = slot;
            PacketUtils::spoofSlot(slot);
        }
    }
    mShouldRotate = true;

    if (mSwing.mValue) player->swing();

    if (mShouldClip)
        player->setPosition(*player->getPos() + glm::vec3(0, 1.f * (mTowerSpeed.mValue / 10), 0));

    BlockUtils::placeBlock(blockPos, side);
    mLastPlaceTime = now;
    mAheadCell = glm::ivec3(INT_MAX); // следующий блок — свой разброс «когда ставить»
    if (mSwitchMode.mValue == SwitchMode::Spoof) {
        player->getSupplies()->mSelectedSlot = lastSlot;
        PacketUtils::spoofSlot(lastSlot);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Rotation helpers
// ═══════════════════════════════════════════════════════════════

// MC-конвенция ({pitch, yaw} в градусах) — та же, что использует Aura.
// ВАЖНО: позиции в этом SDK хранятся на уровне глаз (ступни + 1.62),
// поэтому PLAYER_HEIGHT НЕ добавляем — getPos() уже глаза.
glm::vec2 Scaffold::getTargetRots()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return mUserRot;

    const BridgeProfile prof = getBridgeProfile();

    // ── Человеческий разброс ──────────────────────────────────────────
    // Живой игрок не бьёт в одну и ту же точку и не держит один угол. Всё
    // случайное выбирается один раз на блок — иначе цель дрожала бы каждый
    // тик и голова никогда бы на неё не «садилась».
    if (mLastBlock != mAimCell)
    {
        mAimCell = mLastBlock;
        if (mHumanize.mValue)
        {
            const float j = 0.3f; // по грани, луч всё ещё попадает в неё
            mAimJitter    = { MathUtils::randomFloat(-j, j),
                              MathUtils::randomFloat(-j, j),
                              MathUtils::randomFloat(-j, j) };
            mAimPitchBias = MathUtils::randomFloat(-2.2f, 2.2f);
            mAimYawBias   = MathUtils::randomFloat(-2.2f, 2.2f);
            mAimMinPitch  = MathUtils::randomFloat(80.f, 87.f);
            mAimDownPitch = MathUtils::randomFloat(87.f, 89.9f);
        }
        else
        {
            mAimJitter = glm::vec3(0.f);
            mAimPitchBias = mAimYawBias = 0.f;
            mAimMinPitch = 82.f;
            mAimDownPitch = 89.9f;
        }
    }

    // Смотрим на грань опоры, к которой «кликаем» (между опорой и новым
    // блоком), с разбросом по самой грани.
    const glm::vec3 eyePos = *player->getPos();
    const glm::vec3 cell = mLastBlock + glm::vec3(0.5f);
    glm::vec3 face = cell;
    if (mLastFace >= 0 && mLastFace < 6)
    {
        const glm::vec3 n = BlockUtils::blockFaceOffsets[mLastFace];
        face += n * 0.5f;
        face += mAimJitter * (glm::vec3(1.f) - glm::abs(n)); // только вдоль грани
    }
    glm::vec2 rotations = MathUtils::getRots(eyePos, face); // {pitch, yaw}

    // Yaw. Взгляд почти вертикальный — atan2 на опору под ногами скачет на
    // ±100° от тика к тику. Берём направление на сам новый блок, а если и он
    // прямо под нами — направление движения (стоим — прежний yaw).
    const float horiz = glm::length(glm::vec2(cell.x - eyePos.x, cell.z - eyePos.z));
    if (horiz > 0.35f)
    {
        rotations.y = MathUtils::getRots(eyePos, cell).y;
    }
    else
    {
        const glm::vec3 v = player->getStateVectorComponent()->mVelocity;
        if (v.x * v.x + v.z * v.z > 0.0004f)
            rotations.y = glm::degrees(std::atan2(v.z, v.x)) - 90.f;
        else
            rotations.y = mHeadActive ? mHeadRot.y : mUserRot.y;
    }

    if (prof.pitchDeg >= 0.f)
    {
        // Стиль моста сам решает, под каким углом смотрит голова.
        rotations.x = prof.pitchDeg;
    }
    else
    {
        if (mRotateMode.mValue == RotateMode::Normal)
            rotations.x = std::max(rotations.x, mAimMinPitch);
        if (mRotateMode.mValue == RotateMode::Down)
            rotations.x = mAimDownPitch;
        if (mRotateMode.mValue == RotateMode::Backwards)
            rotations.y += 180.f;
    }

    rotations.x += mAimPitchBias;
    rotations.y += mAimYawBias;

    rotations.x = MathUtils::clamp(rotations.x, -90.f, 90.f);
    rotations.y = normYawDeg(rotations.y);
    return rotations;
}

bool Scaffold::rotationAllowed() const
{
    if (mRotateMode.mValue == RotateMode::None) return false;
    // Аура ведёт цель и Flick Mode = None — её ротацию не перебиваем.
    auto auraMod = gFeatureManager->mModuleManager->getModule<Aura>();
    return !(auraMod && auraMod->sHasTarget && mFlickMode.mValue == FlickMode::None);
}

// Один шаг головы — раз в тик, из PlayerAuthInput. Пока строим мост
// (mWantUntil), голова едет к блоку, потом обратно к тому, куда смотрит игрок;
// дошла обратно — ротацию отпускаем. Большие повороты — на полной скорости,
// последние градусы медленнее, как рука с мышкой.
glm::vec2 Scaffold::stepHead(const glm::vec2& user, uint64_t now, bool& override)
{
    const bool want = now < mWantUntil;
    override = false;

    // Насколько игрок повернул голову САМ с прошлого шага. Резкий рывок мыши
    // нельзя фильтровать шагом в 29°/тик: именно из-за него поворот за мышкой
    // выглядел с задержкой.
    const float userTurn = mUserRotValid ? angDiff(user, mLastUserRot) : 0.f;
    mLastUserRot = user;
    mUserRotValid = true;

    if (!want && !mHeadActive) return user;
    if (!mHeadActive)
    {
        mHeadActive = true;
        mHeadRot = user;
        mHeadPrev = user;
    }

    if (userTurn > kFlickDeg)
    {
        mHeadPrev = user;
        mHeadRot = user;
        mHeadStepAt = now;
        if (want)
        {
            // Мост ещё строим — ротацию держим, но она уже на голове игрока;
            // к блоку она поедет со следующего пакета.
            override = true;
            return user;
        }
        mHeadActive = false;
        mCamSettleUntil = now + 60;
        return user;
    }

    const glm::vec2 target = want ? mHeadTarget : user;
    const float maxStep = rotateStepDeg(mRotateSpeed.mValue);
    auto step = [&](float diff)
    {
        const float a = std::fabs(diff);
        const float s = std::min(a, std::max(maxStep * std::clamp(a / 30.f + 0.25f, 0.25f, 1.f), 1.f));
        return diff < 0.f ? -s : s;
    };
    mHeadPrev = mHeadRot;
    mHeadStepAt = now;
    mHeadRot.x = std::clamp(mHeadRot.x + step(target.x - mHeadRot.x), -90.f, 90.f);
    mHeadRot.y = normYawDeg(mHeadRot.y + step(normYawDeg(target.y - mHeadRot.y)));

    if (!want && angDiff(mHeadRot, user) < 1.5f)
    {
        // Вернулись к игроку — ротация снова его. Камера отстаёт на тик
        // (плавность между тиками): даём ей доехать ровно до игрока.
        mHeadActive = false;
        mHeadRot = user;
        mCamSettleUntil = now + 60;
        return user;
    }
    override = true;
    return mHeadRot;
}

bool Scaffold::headAligned(const glm::vec2& target) const
{
    return mSentValid && viewAngle(mSentRot, target) <= kAlignTolerance;
}

// Legit Place. Что ставить прямо сейчас:
//   - под ногами воздух (сошёл с края, падаешь) — этот блок;
//   - иначе блок под точкой, где ноги будут через mPlaceAhead тиков (по
//     реальной скорости), но не дальше одного блока; по диагонали без опоры
//     сначала ставится боковой блок.
// Ничего — FLT_MAX; soon = край близко (голова уже поворачивается).
glm::vec3 Scaffold::getLegitPlacePos(bool& soon)
{
    soon = false;
    constexpr glm::vec3 none = { FLT_MAX, FLT_MAX, FLT_MAX };
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return none;
    const glm::vec3 pos = *player->getPos();
    const glm::vec3 vel = player->getStateVectorComponent()->mVelocity;
    const int y = static_cast<int>(std::floor(mStartY));
    const glm::ivec3 cur = { static_cast<int>(std::floor(pos.x)), y, static_cast<int>(std::floor(pos.z)) };

    auto placeable = [](const glm::ivec3& c)
    {
        return BlockUtils::isAirBlock(c) && BlockUtils::isValidPlacePos(c) && BlockUtils::getBlockPlaceFace(c) != -1;
    };

    if (BlockUtils::isAirBlock(cur))
    {
        if (placeable(cur)) return glm::vec3(cur);
        // под ногами не к чему прилепить — ближайшая опора рядом (как раньше)
        glm::ivec3 c = BlockUtils::getClosestPlacePos(cur, 2.f);
        if (c.x != INT_MAX && BlockUtils::getBlockPlaceFace(c) != -1) return glm::vec3(c);
        return none;
    }

    glm::vec2 v = { vel.x, vel.z };
    const float speed = glm::length(v);
    if (speed < 0.02f || !Keyboard::isUsingMoveKeys()) return none;

    // Когда ставить — у каждого блока свой момент (живые руки не метроном).
    auto aheadCell = [&](float ticks, float maxDist)
    {
        glm::vec2 d = v * ticks;
        const float l = glm::length(d);
        if (l > maxDist) d *= maxDist / l;
        return glm::ivec3(static_cast<int>(std::floor(pos.x + d.x)), y, static_cast<int>(std::floor(pos.z + d.y)));
    };

    const glm::ivec3 next = aheadCell(mHumanize.mValue ? mPlaceAhead : 1.8f, 0.95f);
    if (next != cur && BlockUtils::isAirBlock(next))
    {
        if (next != mAheadCell)
        {
            mAheadCell = next;
            mPlaceAhead = mHumanize.mValue ? MathUtils::randomFloat(1.3f, 2.3f) : 1.8f;
        }
        if (placeable(next)) return glm::vec3(next);
        // по диагонали: сначала тот боковой, что ближе к направлению
        const glm::ivec3 sideA = { next.x, y, cur.z }, sideB = { cur.x, y, next.z };
        const bool aFirst = std::fabs(v.x) >= std::fabs(v.y);
        for (const glm::ivec3& c : { aFirst ? sideA : sideB, aFirst ? sideB : sideA })
            if (c != cur && placeable(c)) return glm::vec3(c);
        return none;
    }

    // Край ещё не близко, но через ~7 тиков будет — пусть голова поворачивается
    // заранее (иначе первый блок моста ждал бы её и опаздывал).
    const glm::ivec3 later = aheadCell(7.f, 2.0f);
    if (later != cur && BlockUtils::isAirBlock(later))
    {
        soon = true;
        mLastBlock = glm::vec3(later);
        mLastFace = BlockUtils::getBlockPlaceFace(later);
        if (mLastFace < 0) mLastFace = 1;
    }
    return none;
}

// ═══════════════════════════════════════════════════════════════
// Visual rotation — поворот камеры клиента
// ═══════════════════════════════════════════════════════════════

void Scaffold::onLookInputEvent(LookInputEvent& event)
{
    auto* direct = event.mCameraDirectLookComponent;
    if (!direct) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    const glm::vec2 frameRot = radToMc(direct->mRotRads);

    // Игра применила мышь поверх того, что мы вписали в прошлом кадре.
    // Вычитаем свой прошлый вклад — получаем «настоящую» ротацию игрока.
    if (mOverrodeCamera)
    {
        glm::vec2 delta = { frameRot.x - mLastApplied.x,
                            normYawDeg(frameRot.y - mLastApplied.y) };
        mUserRot.x = MathUtils::clamp(mUserRot.x + delta.x, -90.f, 90.f);
        mUserRot.y = normYawDeg(mUserRot.y + delta.y);
    }
    else
    {
        mUserRot = frameRot;
    }

    const bool camActive = mHeadActive || static_cast<uint64_t>(NOW) < mCamSettleUntil;
    if (!mVisibleRotate.mValue || !camActive || !rotationAllowed())
    {
        mOverrodeCamera = false;
        return;
    }

    // Между тиками — плавно, без ступенек. Камере отдаётся только наклон:
    // повернуть её ещё и по yaw — W/A/S/D пошли бы в другую сторону (а в
    // Backwards и вовсе назад). Полный поворот головы видят сервер и F5.
    const float t = std::clamp(static_cast<float>(static_cast<uint64_t>(NOW) - mHeadStepAt) / 50.f, 0.f, 1.f);
    const glm::vec2 head = lerpRots(mHeadPrev, mHeadRot, t);
    const glm::vec2 cam = { head.x, mUserRot.y };

    // Запоминаем ровно то, что камера сможет показать (наклон у неё
    // обрезан до ±88.8°), иначе на следующем кадре разница выглядела бы как
    // движение мыши и «настоящая» ротация игрока уползала бы.
    direct->mRotRads = mcToRad(cam);
    mLastApplied = radToMc(direct->mRotRads);
    mOverrodeCamera = true;
}

// ═══════════════════════════════════════════════════════════════
// Render
// ═══════════════════════════════════════════════════════════════

void Scaffold::onRenderEvent(RenderEvent& event)
{
    if (mBlockHUDStyle.mValue == BlockHUDStyle::None) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    float delta = ImGui::GetIO().DeltaTime;

    static EasingUtil inEase = EasingUtil();
    static float anim = 0.f;
    constexpr float easeSpeed = 10.f;
    this->mEnabled ? inEase.incrementPercentage(delta * easeSpeed / 10)
    : inEase.decrementPercentage(delta * 2 * easeSpeed / 10);
    float inScale = inEase.easeOutExpo();
    if (inEase.isPercentageMax()) inScale = 0.996;
    inScale = MathUtils::clamp(inScale, 0.0f, 0.996);
    anim = MathUtils::lerp(0, 1, inEase.easeOutExpo());

    anim = MathUtils::lerp(anim, mEnabled ? 1.f : 0.f, delta * 10.f);

    if (anim < 0.0001f) return;

    ImVec2 pos = ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y * 0.75f - 40);

    int totalBlocks = ItemUtils::getAllPlaceables(mHotbarOnly.mValue);

    std::string displayText = "Blocks: ";
    Interface* daInterface = gFeatureManager->mModuleManager->getModule<Interface>();
    if (daInterface->mNamingStyle.mValue == NamingStyle::Lowercase || daInterface->mNamingStyle.mValue == NamingStyle::LowercaseSpaced)
    {
        displayText = "blocks: ";
    }
    std::string text = displayText + std::to_string(totalBlocks);
    std::string numberText = std::to_string(totalBlocks);

    // Mntsb — единственный шрифт клиента, и он уже жирный.
    FontHelper::pushPrefFont(true);

    float fontSize = 25.f * anim;

    ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, text.c_str());
    pos.x -= textSize.x / 2;
    pos.y -= textSize.y / 2;

    auto drawList = ImGui::GetBackgroundDrawList();

    drawList->AddShadowRect(ImVec2(pos.x - 5, pos.y - 5), ImVec2(pos.x + textSize.x + 5, pos.y + textSize.y + 5), ImColor(0.f, 0.f, 0.f, 0.45f * anim), 500, ImVec2(0, 0));
    drawList->AddRectFilled(ImVec2(pos.x - 5, pos.y - 5), ImVec2(pos.x + textSize.x + 5, pos.y + textSize.y + 5), ImColor(0.f, 0.f, 0.f, (0.45f * anim)), 5.f, 0);

    ImColor color = ImColor(255, 255, 255, 255);

    for (int i = 0; i < displayText.size(); i++) {
        color = ColorUtils::getThemedColor(i * 100);
        color.Value.w = color.Value.w * anim;
        ImVec2 ptextSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, std::string(1, displayText[i]).c_str());

        ImVec2 shadowPos = ImVec2(pos.x + 2, pos.y + 2);
        drawList->AddText(ImGui::GetFont(), fontSize, shadowPos, ImColor(color.Value.x * 0.25f, color.Value.y * 0.25f, color.Value.z * 0.25f, 0.9f), std::string(1, displayText[i]).c_str());
        drawList->AddText(ImGui::GetFont(), fontSize, pos, color, std::string(1, displayText[i]).c_str());
        pos.x += ptextSize.x;
    }

    int colorStartingIndex = displayText.size() * 100;

    static std::vector<std::string> numbers = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    static std::string joinedNumbers = StringUtils::join(numbers, "\n");

    int num = totalBlocks;
    std::string numStr = std::to_string(num);

    ImVec2 numTextSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, numStr.c_str());

    ImVec2 clipRectMin = ImVec2(pos.x, pos.y);
    ImVec2 clipRectMax = ImVec2(pos.x + numTextSize.x + 5, pos.y + numTextSize.y);

    drawList->PushClipRect(clipRectMin, clipRectMax, true);

    for (int i = 0; i < numStr.size(); i++) {
        std::string num = std::string(1, numStr[i]);
        int realNum = std::stoi(num);
        ImVec2 ptextSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, num.c_str());

        float offset = ptextSize.y * (realNum);
        static std::map<int, float> indexOffsetMap;

        if (!indexOffsetMap.contains(i)) {
            indexOffsetMap[i] = offset;
        }

        ImVec2 daPos = ImVec2((float)i * (float)ptextSize.x + pos.x, (float)-indexOffsetMap[i] + pos.y);

        color = ColorUtils::getThemedColor(colorStartingIndex + i * 100);
        color.Value.w = color.Value.w * anim;

        ImVec2 shadowPos = ImVec2(daPos.x + 2, daPos.y + 2);
        drawList->AddText(ImGui::GetFont(), fontSize, shadowPos, ImColor(color.Value.x * 0.25f, color.Value.y * 0.25f, color.Value.z * 0.25f, 0.9f * anim), joinedNumbers.c_str());
        drawList->AddText(ImGui::GetFont(), fontSize, daPos, color, joinedNumbers.c_str());

        indexOffsetMap[i] = MathUtils::lerp(indexOffsetMap[i], offset, ImGui::GetIO().DeltaTime * 10.f);
    }

    drawList->PopClipRect();

    ImGui::PopFont();
}

// ═══════════════════════════════════════════════════════════════
// Packet Out — серверная ротация + clickPos фикс
// ═══════════════════════════════════════════════════════════════

void Scaffold::onPacketOutEvent(PacketOutEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    if (!event.mPacket) return;   // пакет мог быть уже отменён другим модулем

    if (event.mPacket->getId() == PacketID::InventoryTransaction)
    {
        if (const auto it = event.getPacket<InventoryTransactionPacket>(); it->mTransaction->type ==
            ComplexInventoryTransaction::Type::ItemUseTransaction)
        {
            const auto transac = reinterpret_cast<ItemUseInventoryTransaction*>(it->mTransaction.get());
            if (transac->mActionType == ItemUseInventoryTransaction::ActionType::Place)
            {
                transac->mClickPos = BlockUtils::clickPosOffsets[transac->mFace];
                for (int i = 0; i < 3; i++)
                {
                    if (transac->mClickPos[i] == 0.5)
                    {
                        transac->mClickPos[i] = MathUtils::randomFloat(-0.49f, 0.49f);
                    }
                }
            }
        }
    }

    const PacketID id = event.mPacket->getId();

    // Ротация несёт не только PlayerAuthInput. На серверах, где движение
    // клиент-авторитетное (AuthorityMode = ClientAuthoritative), поворот уходит
    // в MovePlayer — раньше мы туда не писали вообще, и скэффолд «не поворачивал
    // голову». Пишем ровно в тот пакет, который реально несёт ротацию (как Aura).
    if (id != PacketID::PlayerAuthInput && id != PacketID::MovePlayer) return;

    if (!rotationAllowed())
    {
        mHeadActive = false;
        mSentValid = false;
        return;
    }

    PlayerAuthInputPacket* paip = nullptr;
    MovePlayerPacket* mpp = nullptr;
    glm::vec2 packetRot = { 0.f, 0.f };

    if (id == PacketID::PlayerAuthInput)
    {
        paip = event.getPacket<PlayerAuthInputPacket>();
        if (!paip) return;
        if (mTest.mValue) paip->mPos.y = paip->mPos.y - 0.01f;
        packetRot = { paip->mRot.x, paip->mRot.y };
    }
    else
    {
        mpp = event.getPacket<MovePlayerPacket>();
        if (!mpp) return;
        // Телепорт/респавн несёт служебную ротацию — её не перебиваем.
        if (mpp->mResetPosition == PositionMode::Teleport ||
            mpp->mResetPosition == PositionMode::Respawn)
        {
            mSentValid = false;
            return;
        }
        packetRot = { mpp->mRot.x, mpp->mRot.y };
    }

    // Ротация игрока: с Visible Rotate камера наша, берём отслеженную.
    const glm::vec2 user = (mVisibleRotate.mValue && mOverrodeCamera) ? mUserRot : packetRot;
    bool ovr = false;
    const glm::vec2 rot = stepHead(user, static_cast<uint64_t>(NOW), ovr);
    if (ovr)
    {
        if (paip)
        {
            paip->mRot = rot;            // {pitch, yaw} в MC-градусах
            paip->mYHeadRot = rot.y;
        }
        else if (mpp)
        {
            mpp->mRot = rot;
            mpp->mYHeadRot = rot.y;
        }
    }
    mSentRot = ovr ? rot : user;     // это и видит сервер на следующем блоке
    mSentValid = true;
}

// Своя голова и тело = то, что получил сервер. Пишется в тике (как в Aura),
// после того как игра разложила свои компоненты, — иначе следующий тик их
// перезапишет. Пока голова наша, только мы и решаем, куда она смотрит.
void Scaffold::applyLocalModelRotation(Actor* player)
{
    if (!player) return;
    if (!mHeadActive || !mSentValid || !rotationAllowed()) return;

    if (auto* head = player->getActorHeadRotationComponent())
    {
        head->mHeadRot = mSentRot.y;
        head->mOldHeadRot = mSentRot.y;
    }
    if (auto* body = player->getMobBodyRotationComponent())
    {
        body->yBodyRot = mSentRot.y;
        body->yOldBodyRot = mSentRot.y;
    }
}

glm::vec3 Scaffold::getRotBasedPos(float extend, float yPos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    glm::vec2 playerRots = glm::vec2(player->getActorRotationComponent()->mPitch,
                                 player->getActorRotationComponent()->mYaw + MathUtils::getRotationKeyOffset());

    if (mPlacementMode.mValue == PlacementMode::Flareon)
    {
        playerRots.y = MathUtils::snapYaw(playerRots.y);
    }

    float correctedYaw = (playerRots.y + 90) * ((float)IM_PI / 180);
    float inFrontX = cos(correctedYaw) * extend;
    float inFrontZ = sin(correctedYaw) * extend;
    float placeX = player->getPos()->x + inFrontX;
    float placeY = yPos;
    float placeZ = player->getPos()->z + inFrontZ;

    return {floor(placeX), floor(placeY), floor(placeZ)};
}

glm::vec3 Scaffold::getPlacePos(float extend)
{
    // Стиль моста сдвигает точку постановки: вперёд/назад и на bias блоков.
    const BridgeProfile prof = getBridgeProfile();
    const float e = (extend + prof.extendBias) * prof.extendDir;

    float yPos = mStartY;
    glm::ivec3 blockSel = {INT_MAX, INT_MAX, INT_MAX};

    blockSel = getRotBasedPos(e, yPos);

    int side = BlockUtils::getBlockPlaceFace(blockSel);

    if (side == -1)
    {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return {FLT_MAX, FLT_MAX, FLT_MAX};

        if (player->getFallDistance() > 3.f)
        {
            blockSel.y = player->getPos()->y - 3.62f;
        }

        blockSel = BlockUtils::getClosestPlacePos(blockSel, mRange.as<float>());
        if (blockSel.x == INT_MAX) return {FLT_MAX, FLT_MAX, FLT_MAX};
        side = BlockUtils::getBlockPlaceFace(blockSel);

        if (side == -1) return {FLT_MAX, FLT_MAX, FLT_MAX};
    }

    if (blockSel.x == INT_MAX) return {FLT_MAX, FLT_MAX, FLT_MAX};

    return blockSel;
}
