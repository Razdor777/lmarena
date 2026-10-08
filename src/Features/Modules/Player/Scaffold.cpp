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
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>

// ═══════════════════════════════════════════════════════════════
// Rotation helpers (file-local)
// ═══════════════════════════════════════════════════════════════
namespace
{
    // Сколько времени голова едет к блоку. 90° за (90 / speed) мс.
    float rotateInMs(float speed)
    {
        return 90.f / std::max(speed, 0.2f);
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
    mFlickActive = false;
    mOverrodeCamera = false;
    mLastApplied = { 0.f, 0.f };
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
    mFlickActive = false;
    mOverrodeCamera = false;

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

    if (!BlockUtils::isAirBlock(blockPos) && !mIsTowering)
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
    // Rotate First держит постановку, пока флик не довёл взгляд до блока
    // (с жёстким таймаутом, чтобы модуль не встал колом).
    const bool canRotate = mVisibleRotate.mValue && mRotateMode.mValue != RotateMode::None;
    if (canRotate)
    {
        if (!mFlickActive)
        {
            if (auto* rot = player->getActorRotationComponent())
                mUserRot = { rot->mPitch, rot->mYaw };
            startFlick();
        }
        else
        {
            mFlickTarget = getTargetRots();
        }

        if (mRotateFirst.mValue
            && !flickAligned(8.f)
            && static_cast<uint64_t>(NOW) - mFlickStart < 220ull)
        {
            return false; // в этом тике только поворачиваемся
        }
    }

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
    // Живой игрок не бьёт в одну и ту же точку блока каждый раз.
    // Разброс пересчитывается только когда меняется блок — иначе
    // прицел дрожал бы на каждом тике.
    if (mHumanize.mValue)
    {
        if (mLastBlock != mAimCell)
        {
            mAimCell = mLastBlock;
            const float j = 0.34f; // ±треть блока: луч всё ещё попадает в него
            mAimJitter    = { MathUtils::randomFloat(-j, j),
                              MathUtils::randomFloat(-j, j),
                              MathUtils::randomFloat(-j, j) };
            mAimPitchBias = MathUtils::randomFloat(-2.2f, 2.2f);
            mAimYawBias   = MathUtils::randomFloat(-2.2f, 2.2f);
        }
    }
    else
    {
        mAimJitter    = glm::vec3(0.f);
        mAimPitchBias = 0.f;
        mAimYawBias   = 0.f;
    }

    // Целимся в блок, по которому «кликаем» — это сосед целевой клетки,
    // плюс разброс внутри его грани
    glm::vec3 target = mLastBlock + glm::vec3(0.5f);
    if (mLastFace >= 0) target += BlockUtils::blockFaceOffsets[mLastFace];
    target += mAimJitter;

    glm::vec3 eyePos = *player->getPos();

    glm::vec2 rotations = MathUtils::getRots(eyePos, target); // {pitch, yaw}

    if (prof.pitchDeg >= 0.f)
    {
        // Стиль моста сам решает, под каким углом смотрит голова.
        rotations.x = prof.pitchDeg;
    }
    else
    {
        if (mRotateMode.mValue == RotateMode::Normal)
        {
            // Живой игрок не держит ровно один и тот же угол
            float minPitch = mHumanize.mValue ? MathUtils::randomFloat(80.f, 87.f) : 82.f;
            rotations.x = fmax(rotations.x, minPitch);
        }
        if (mRotateMode.mValue == RotateMode::Down)
            rotations.x = mHumanize.mValue ? MathUtils::randomFloat(87.f, 89.9f) : 89.9f;
        if (mRotateMode.mValue == RotateMode::Backwards)
            rotations.y += 180.f;
    }

    if (mHumanize.mValue)
    {
        rotations.x += mAimPitchBias;
        rotations.y += mAimYawBias;
    }

    rotations.x = MathUtils::clamp(rotations.x, -90.f, 90.f);
    rotations.y = normYawDeg(rotations.y);
    return rotations;
}

void Scaffold::startFlick()
{
    const uint64_t now = static_cast<uint64_t>(NOW);

    // Уже летим вниз — не перезапускаем фазу, только подтягиваем цель
    // к новому блоку (иначе при непрерывном мосте голова не успевала бы опуститься).
    if (mFlickActive && static_cast<float>(now - mFlickStart) < rotateInMs(mRotateSpeed.mValue))
    {
        mFlickTarget = getTargetRots();
        return;
    }

    mFlickActive = true;
    mFlickStart = now;
    mFlickTarget = getTargetRots();
    mFlickBase = mUserRot;
}

glm::vec2 Scaffold::sampleFlick(uint64_t now, bool& active)
{
    const float inMs = rotateInMs(mRotateSpeed.mValue);
    const float holdMs = std::max(mRotateHold.mValue, 0.f);
    const float outMs = inMs * 1.35f;
    const float t = static_cast<float>(now - mFlickStart);

    active = mFlickActive && t < inMs + holdMs + outMs;
    if (!active) return mUserRot;

    if (t < inMs)
    {
        float u = std::clamp(t / inMs, 0.f, 1.f);
        if (mHumanize.mValue)
        {
            // Живой игрок чуть перелетает цель и дорабатывает (overshoot)
            constexpr float c1 = 1.70158f;
            constexpr float c3 = 2.70158f;
            const float f = u - 1.f;
            u = std::clamp(1.f + c3 * f * f * f + c1 * f * f, 0.f, 1.16f);
        }
        else
        {
            u = smoothStep(u);
        }
        return lerpRots(mFlickBase, mFlickTarget, u);
    }
    if (t < inMs + holdMs)
        return mFlickTarget;
    return lerpRots(mFlickTarget, mFlickBase,
                    smoothStep((t - inMs - holdMs) / std::max(outMs, 1.f)));
}

bool Scaffold::flickAligned(float tolerance)
{
    if (!mFlickActive) return true;

    bool active = false;
    glm::vec2 current = sampleFlick(static_cast<uint64_t>(NOW), active);
    if (!active) return true;

    const float dPitch = std::fabs(current.x - mFlickTarget.x);
    const float dYaw = std::fabs(normYawDeg(current.y - mFlickTarget.y));
    return dPitch + dYaw <= tolerance;
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
    // Вычитаем свой прошлый вклад — получаем «настоящую» ротацию игрока,
    // даже пока идёт флик (иначе камера «прилипала» бы к блоку).
    if (mFlickActive && mOverrodeCamera)
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

    if (!mVisibleRotate.mValue || mRotateMode.mValue == RotateMode::None)
    {
        mOverrodeCamera = false;
        return;
    }

    // Аура ведёт цель и мы её не перебиваем — камеру не трогаем.
    auto auraMod = gFeatureManager->mModuleManager->getModule<Aura>();
    if (auraMod && auraMod->sHasTarget && mFlickMode.mValue == FlickMode::None)
    {
        mFlickActive = false;
        mOverrodeCamera = false;
        return;
    }

    bool active = false;
    glm::vec2 rots = sampleFlick(static_cast<uint64_t>(NOW), active);
    if (!active)
    {
        if (mFlickActive) mFlickActive = false;
        mOverrodeCamera = false;
        return;
    }

    direct->mRotRads = mcToRad(rots);
    mLastApplied = rots;
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

    if (event.mPacket->getId() == PacketID::PlayerAuthInput)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();

        if (mTest.mValue)
        {
            paip->mPos.y = paip->mPos.y - 0.01f;
        }

        if (mRotateMode.mValue == RotateMode::None || mLastFace < 0) return;

        auto auraMod = gFeatureManager->mModuleManager->getModule<Aura>();
        const bool auraBusy = auraMod && auraMod->sHasTarget;

        // Аура ведёт цель и мы её не перебиваем — пакет не трогаем.
        if (auraBusy && mFlickMode.mValue == FlickMode::None) return;

        const uint64_t now = static_cast<uint64_t>(NOW);

        // Ротация = то, что сейчас реально показывает голова (флик),
        // иначе — цель постановки, пока не истёк таймаут после блока.
        bool active = false;
        glm::vec2 rotations = sampleFlick(now, active);
        if (!active)
        {
            if (now - mLastSwitchTime > 500ull) return;
            rotations = getTargetRots();
        }

        paip->mRot = rotations;      // {pitch, yaw} в MC-градусах
        paip->mYHeadRot = rotations.y;
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
