//
// Created by vastrakai on 7/10/2024.
//

#include "Scaffold.hpp"

#include <algorithm>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
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
    // Мост задаёт только геометрию и ритм: за сколько блоков до края ставить,
    // вперёд или за спину, сколько клеток за тик имеет смысл и надо ли прыгать.
    // Угол головы больше не задаётся числом — она смотрит В блок.
    // Ключевое отличие режимов — НА СКОЛЬКО КЛЕТОК вперёд ставится блок и
    // каким ритмом. Раньше разница была только в lookAhead (0.85…1.10): пол-
    // блока в ту или другую сторону почти всегда попадает в ТУ ЖЕ клетку,
    // поэтому все режимы и выглядели одинаково.
    BridgeProfile p{};
    switch (mBridgeMode.mValue)
    {
    case BridgeMode::GodBridge:
        p.aheadCells = 1;      // блок встаёт прямо перед ногой
        p.places     = 1;
        p.gapMs      = 90;     // самый плотный ритм
        p.autoJump   = true;   // прыжок только на краю (mNeedJump)
        p.sprint     = true;
        break;
    case BridgeMode::Breezily:
        p.aheadCells = 2;      // мост уходит на две клетки вперёд
        p.places     = 2;      // и за тик можно поставить обе
        p.gapMs      = 150;    // размереннее GodBridge
        p.sprint     = true;
        break;
    case BridgeMode::Moonwalk:
        // Блоки уходят ЗА спину: идёшь вперёд — мост растёт за тобой.
        p.aheadCells = 1;
        p.extendDir  = -1.f;
        p.places     = 1;
        p.gapMs      = 120;
        break;
    case BridgeMode::Telly:
        // Только в воздухе, блоки на три клетки вперёд: приземляешься уже на мост.
        p.aheadCells = 3;
        p.places     = 2;
        p.gapMs      = 90;
        p.airOnly    = true;
        p.autoJump   = true;
        p.sprint     = true;
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

    auto player = ClientInstance::get()->getLocalPlayer();

    mHeadActive = false;
    mWantUntil = 0;
    mSentValid = false;
    mNeedSince = 0;
    mUserRotValid = false;
    mLastPlaceTime = 0;
    mDidForceJump = false;
    mLastJump = 0;
    mBridgeY = INT_MAX;
    mBridgeYValid = false;
    mUnderCell = glm::ivec3(INT_MAX);
    mAheadCell = glm::ivec3(INT_MAX);
    mHasDir = false;
    mNeedJump = false;
    mPlacedThisTick = false;
    mIsTowering = false;

    if (!player) return;

    mLastSlot = player->getSupplies()->mSelectedSlot;

    if (auto* rot = player->getActorRotationComponent())
        mUserRot = { rot->mPitch, rot->mYaw };
}

void Scaffold::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Scaffold::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Scaffold::onPacketOutEvent>(this);

    mLastBlock = {0, 0, 0};
    mLastFace = 0;
    mLastSwitchTime = 0;
    mShouldRotate = false;
    mHeadActive = false;
    mWantUntil = 0;
    mSentValid = false;
    mUserRotValid = false;
    mBridgeY = INT_MAX;
    mBridgeYValid = false;
    mIsTowering = false;

    // Пока мы были включены, могли держать прыжок — снимаем, иначе игрок будет
    // прыгать сам по себе уже после выключения модуля.
    releaseForcedJump();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->isValid()) return;

    if (mLastSlot != -1 && player->getSupplies())
        player->getSupplies()->mSelectedSlot = mLastSlot;
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

// Уровень моста и клетки этого тика.
// Главное правило: пока игрок в воздухе, уровень НЕ поднимается вместе с
// прыжком. Иначе блок ставится в клетку, где стоят ноги, игрока выдавливает из
// блока — и он падает. Раньше уровень брался из текущей высоты каждый тик (и с
// floor без запаса), поэтому любой прыжок строил ступеньку вверх, а проседание
// на миллиметр — ступеньку вниз.
void Scaffold::updatePlacementFrame(Actor* player)
{
    if (!player) return;

    const glm::vec3 pos = *player->getPos();
    const auto* stateVec = player->getStateVectorComponent();

    // +0.001 — чтобы «стою ровно на 11.0» не превращалось в 10.999 → floor 9.
    const int support = static_cast<int>(std::floor(pos.y - 2.62f + 0.001f));

    if (mIsTowering)
    {
        // Башня: уровень едет вместе с игроком вверх.
        mBridgeY = support;
    }
    else if (!mBridgeYValid)
    {
        mBridgeY = support;
    }
    else if (player->isOnGround())
    {
        // На земле уровень — это блок под ногами (может и подняться, и опуститься).
        mBridgeY = support;
    }
    else if (!mSameY.mValue)
    {
        // Same Y выключен: уровень идёт за игроком и в воздухе — мост
        // получается ступенькой (вверх на прыжке, вниз на просадке).
        mBridgeY = support;
    }
    else if (support < mBridgeY)
    {
        // Падаем ниже моста — уровень едет вниз, чтобы успеть поставить под себя.
        mBridgeY = support;
    }
    // В воздухе ВЫШЕ моста (прыжок) уровень не трогаем — это и есть ступенька,
    // на которой раньше всё ломалось.
    mBridgeYValid = true;

    mUnderCell = { static_cast<int>(std::floor(pos.x)), mBridgeY,
                   static_cast<int>(std::floor(pos.z)) };

    // Направление: по движению, а если стоим — по взгляду. Moonwalk
    // разворачивает его назад.
    glm::vec2 dir = { 0.f, 1.f };
    if (stateVec)
    {
        const glm::vec2 v = { stateVec->mVelocity.x, stateVec->mVelocity.z };
        const float speed = glm::length(v);
        if (speed > 0.02f)
            dir = v / speed;
        else if (auto* rot = player->getActorRotationComponent())
        {
            const float yaw = glm::radians(rot->mYaw + 90.f);
            dir = { std::cos(yaw), std::sin(yaw) };
        }
    }

    const BridgeProfile prof = getBridgeProfile();
    if (prof.extendDir < 0.f) dir = -dir;
    mDir = dir;
    mHasDir = true;

    // Клетка по ходу: шаг целыми клетками (1 — GodBridge/Moonwalk, 2 — Breezily,
    // 3 — Telly). Раньше здесь был дробный lookAhead, и любой режим попадал
    // в соседнюю клетку — визуально они были неотличимы.
    const int ahead = std::clamp(prof.aheadCells, 1, 4);
    glm::ivec2 step = { static_cast<int>(std::lround(dir.x)), static_cast<int>(std::lround(dir.y)) };
    if (step.x == 0 && step.y == 0) step = { 0, 1 };
    mAheadCell = { static_cast<int>(std::floor(pos.x)) + step.x * ahead, mBridgeY,
                   static_cast<int>(std::floor(pos.z)) + step.y * ahead };
}

void Scaffold::updateAutoJump(BaseTickEvent& event)
{
    auto player = event.mActor;
    auto moveInput = player->getMoveInputComponent();
    if (!moveInput) return;

    // Прыжок держим ровно один тик и отпускаем в СЛЕДУЮЩЕМ тике, до всех
    // проверок. Раньше он отпускался по «уже не на земле»: если игра
    // обрабатывала тик раньше нас, прыжок оставался зажатым — и получался
    // бесконечный бхоп.
    if (mDidForceJump)
    {
        moveInput->setJumping(false);
        mDidForceJump = false;
    }

    const BridgeProfile prof = getBridgeProfile();
    if (!(mAutoJump.mValue || prof.autoJump)) return;
    if (!Keyboard::isUsingMoveKeys()) return;
    if (!player->isOnGround()) return;

    const uint64_t now = static_cast<uint64_t>(NOW);
    const uint64_t delay = static_cast<uint64_t>(std::max(mJumpDelay.mValue, 60.f));
    if (mLastJump != 0 && now - mLastJump < delay) return;

    // Прыгаем только когда это действительно нужно — на краю моста.
    // На ровной земле Auto Jump больше не прыгает вообще.
    if (!mNeedJump) return;

    moveInput->setJumping(true);
    mDidForceJump = true;
    mLastJump = now;
}

void Scaffold::onBaseTickEvent(BaseTickEvent& event)
{
    mShouldClip = false;
    mPlacedThisTick = false;

    auto player = event.mActor;
    if (!player || !player->isValid()) return;

    // Своя модель (F5, тени, записи): голова — ровно то, что сейчас уходит
    // на сервер. Камера при этом остаётся твоей.
    applyLocalModelRotation(player);

    const BridgeProfile prof = getBridgeProfile();

    updatePlacementFrame(player);

    // Прыжок оправдан только на краю: впереди на уровне моста пусто, а под
    // ногами — блок. На ровной земле и в воздухе — нет.
    mNeedJump = mHasDir && Keyboard::isUsingMoveKeys()
        && mAheadCell != mUnderCell
        && !BlockUtils::isAirBlock(mUnderCell)
        && BlockUtils::isAirBlock(mAheadCell);

    // Голова начинает ехать к краю ЗАРАНЕЕ: к моменту, когда блок реально
    // нужен, сервер уже видит нас смотрящими на него. Без этого после каждой
    // паузы (>Rotate Hold) голова возвращалась к камере и каждый блок стоил
    // 2-3 тика ожидания поворота.
    if (mNeedJump && rotationAllowed())
    {
        const int face = BlockUtils::getBlockPlaceFace(mAheadCell);
        if (face != -1)
        {
            mLastBlock = glm::vec3(mAheadCell);
            mLastFace = face;
            mHeadTarget = getTargetRots();
            mWantUntil = std::max(mWantUntil, static_cast<uint64_t>(NOW) + 250);
        }
    }

    updateAutoJump(event);

    // Telly работает только в воздухе: на земле ждём прыжка.
    if (prof.airOnly && player->isOnGround()) return;

    if ((prof.sprint || mKeepSprint.mValue) && Keyboard::isUsingMoveKeys()
        && !ClientInstance::get()->getMouseGrabbed())
    {
        if (auto* moveInput = player->getMoveInputComponent())
            moveInput->mIsSprinting = true;
    }

    int places = prof.places > 0 ? prof.places : static_cast<int>(mPlaces.mValue);

    if (mFastClutch.mValue && player->getFallDistance() > mClutchFallDistance.mValue)
        places = static_cast<int>(mCluchPlaces.mValue);

    places = std::clamp(places, 1, 4);

    // Ритм: не чаще одного «залпа» в kMinPlaceGapMs. Внутри залпа можно
    // поставить несколько клеток (диагональ), но подряд два залпа — нет.
    // Под ногами пусто — это не «залп», а спасение: ставим сразу.
    const uint64_t now = static_cast<uint64_t>(NOW);
    const bool underIsAir = BlockUtils::isAirBlock(mUnderCell);
    const uint64_t gap = prof.gapMs > 0 ? static_cast<uint64_t>(prof.gapMs) : kMinPlaceGapMs;
    if (!underIsAir && now - mLastPlaceTime < gap) return;

    for (int i = 0; i < places; i++)
    {
        if (!tickPlace(event)) break;
        if (mShouldClip) break;
    }
}

bool Scaffold::tickPlace(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return false;

    auto moveInput = player->getMoveInputComponent();
    auto actorRot = player->getActorRotationComponent();
    auto stateVec = player->getStateVectorComponent();
    if (!moveInput || !actorRot || !stateVec) return false;

    if (ItemUtils::getAllPlaceables(mHotbarOnly.mValue) == 0)
    {
        mIsTowering = false;
        return false;
    }

    if (mSwitchMode.mValue == SwitchMode::Fake && mLastSlot != -1)
        player->getSupplies()->mInHandSlot = mLastSlot;

    const BridgeProfile prof = getBridgeProfile();
    const uint64_t now = static_cast<uint64_t>(NOW);

    // ── Tower ────────────────────────────────────────────────────────────
    {
        auto& keyboard = *ClientInstance::get()->getKeyboardSettings();
        const bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
        const bool wasTowering = mIsTowering;
        const bool isMoving = Keyboard::isUsingMoveKeys();

        float fallDistance = player->getFallDistance();
        if (!mFallDistanceCheck.mValue) fallDistance = 0.f;

        bool toweringNow = false;
        if (mTowerMode.mValue != TowerMode::Vanilla)
        {
            const bool towerKey = (space && mAllowMovement.mValue) || (space && !isMoving);
            if (towerKey && fallDistance < 3.f && !ClientInstance::get()->getMouseGrabbed())
            {
                if (!mAllowMovement.mValue)
                {
                    stateVec->mVelocity.x = 0;
                    stateVec->mVelocity.z = 0;
                }
                else if (!player->isOnGround())
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
                mIsTowering = true;
                toweringNow = true;
                if (mTowerMode.mValue == TowerMode::Velocity)
                    stateVec->mVelocity.y = mTowerSpeed.mValue / 10;
                else
                    mShouldClip = true;
            }
        }

        // Больше не бьём velocity в −5 при выходе из башни: это был не
        // «сброс», а телепорт на пять блоков вниз — ровно то, что выглядело
        // как падение после отпускания пробела.
        if (!toweringNow && wasTowering)
            mIsTowering = false;
    }

    // ── Что ставим в этом тике ───────────────────────────────────────────
    std::vector<glm::ivec3> cells;
    if (mLegitPlace.mValue && !mIsTowering)
    {
        int maxCells = prof.places > 0 ? prof.places : static_cast<int>(mPlaces.mValue);
        if (mFastClutch.mValue && player->getFallDistance() > mClutchFallDistance.mValue)
            maxCells = std::max(1, static_cast<int>(mCluchPlaces.mValue));
        maxCells = std::clamp(maxCells, 1, 4);
        collectPlaceCells(player, cells, maxCells);
    }
    else
    {
        // Старый режим (Legit Place выключен): блок под ногами и до mExtend
        // блоков вперёд. Уровень всё равно берётся из моста, а не из высоты
        // глаз, и блок в себя не ставится.
        const float maxExtend = (Keyboard::isUsingMoveKeys() && !mIsTowering) ? mExtend.mValue : 0.f;
        glm::vec3 first = getPlacePos(0.f);
        if (first.x == FLT_MAX) return false;
        if (!BlockUtils::isAirBlock(glm::ivec3(first)))
        {
            for (float i = 1.f; i <= maxExtend; i += 1.f)
            {
                const glm::vec3 c = getPlacePos(i);
                if (c.x == FLT_MAX) continue;
                if (BlockUtils::isAirBlock(glm::ivec3(c)))
                {
                    first = c;
                    break;
                }
            }
        }
        if (!BlockUtils::isAirBlock(glm::ivec3(first)))
        {
            // Ставить нечего, но голову всё равно наводим на край, чтобы
            // следующий блок ушёл уже с правильной ротацией.
            if (rotationAllowed() && Keyboard::isUsingMoveKeys())
            {
                const glm::vec3 edge = getPlacePos(maxExtend);
                if (edge.x != FLT_MAX)
                {
                    mLastBlock = edge;
                    mLastFace = BlockUtils::getBlockPlaceFace(glm::ivec3(edge));
                    if (mLastFace < 0) mLastFace = 1;
                    mHeadTarget = getTargetRots();
                    mWantUntil = std::max(mWantUntil, now + 250);
                }
            }
            return false;
        }
        cells.push_back(glm::ivec3(first));
    }

    if (cells.empty()) return false;

    const glm::ivec3 cell = cells.front();
    const int side = BlockUtils::getBlockPlaceFace(cell);
    if (side == -1) return false;
    if (mAvoidUnderplace.mValue && side == 0) return false;

    // Когда ждать поворот головы нельзя: блок под ногами (мы уже падаем),
    // падение, башня или вторая клетка того же залпа.
    const bool urgent = cell == mUnderCell || mPlacedThisTick
        || player->getFallDistance() > 1.f || mIsTowering;

    mLastBlock = glm::vec3(cell);
    mLastFace = side;

    // ── Сначала голова, потом блок ───────────────────────────────────────
    // Пакет постановки уходит раньше, чем PlayerAuthInput этого тика, — то
    // есть сервер видит блок, поставленный с ПРОШЛОЙ ротацией. Поэтому голова
    // поворачивается к блоку, а ставим его, только когда сервер уже получил
    // этот взгляд. Не дольше kRotateTimeoutMs; при падении и в башне — сразу.
    if (rotationAllowed())
    {
        mHeadTarget = getTargetRots();
        mWantUntil = now + static_cast<uint64_t>(std::max(mRotateHold.mValue, 250.f));
        if (mRotateFirst.mValue && !urgent && !headAligned(mHeadTarget))
        {
            if (mNeedSince == 0) mNeedSince = now;
            if (now - mNeedSince < kRotateTimeoutMs)
                return false; // в этом тике только поворачиваемся
        }
    }
    mNeedSince = 0;

    // ── Слот и постановка ────────────────────────────────────────────────
    mLastSwitchTime = NOW;

    if (mLastSlot == -1) mLastSlot = player->getSupplies()->mSelectedSlot;
    const int lastSlot = player->getSupplies()->mSelectedSlot;

    if (mSwitchMode.mValue != SwitchMode::None)
    {
        const int slot = ItemUtils::getPlaceableItemOnBlock(glm::vec3(cell), mHotbarOnly.mValue,
                                                            mSwitchPriority.mValue == SwitchPriority::Highest);
        if (slot == -1) return false;
        player->getSupplies()->mSelectedSlot = slot;
        if (mSwitchMode.mValue == SwitchMode::Spoof) PacketUtils::spoofSlot(slot);
    }
    mShouldRotate = true;

    if (mSwing.mValue) player->swing();

    if (mShouldClip)
        player->setPosition(*player->getPos() + glm::vec3(0, 1.f * (mTowerSpeed.mValue / 10), 0));

    BlockUtils::placeBlock(glm::vec3(cell), side);
    mLastPlaceTime = now;
    mPlacedThisTick = true;

    if (mSwitchMode.mValue == SwitchMode::Spoof)
    {
        player->getSupplies()->mSelectedSlot = lastSlot;
        PacketUtils::spoofSlot(lastSlot);
    }

    return true;
}

glm::vec2 Scaffold::getTargetRots()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return mUserRot;

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
        }
        else
        {
            mAimJitter = glm::vec3(0.f);
            mAimPitchBias = mAimYawBias = 0.f;
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

    // Pitch — ровно на грань блока. Раньше здесь стоял фиксированный угол
    // (GodBridge 84°, Breezily 58° и т. д.): взгляд уходил МИМО блока, сервер
    // отклонял постановку, и игрок падал.
    if (mRotateMode.mValue == RotateMode::Down)
        rotations.x = mAimDownPitch;            // «Down» — намеренно вниз
    if (mRotateMode.mValue == RotateMode::Backwards)
        rotations.y += 180.f;

    rotations.x += mAimPitchBias;
    rotations.y += mAimYawBias;

    rotations.x = MathUtils::clamp(rotations.x, -89.9f, 89.9f);
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

    // Насколько игрок повернул голову САМ с прошлого шага.
    const float userTurn = mUserRotValid ? angDiff(user, mLastUserRot) : 0.f;
    mLastUserRot = user;
    mUserRotValid = true;

    auto step = [&](const glm::vec2& target)
    {
        const float maxStep = rotateStepDeg(mRotateSpeed.mValue);
        auto one = [&](float diff)
        {
            const float a = std::fabs(diff);
            const float s = std::min(a, std::max(maxStep * std::clamp(a / 30.f + 0.25f, 0.25f, 1.f), 1.f));
            return diff < 0.f ? -s : s;
        };
        mHeadRot.x = std::clamp(mHeadRot.x + one(target.x - mHeadRot.x), -90.f, 90.f);
        mHeadRot.y = normYawDeg(mHeadRot.y + one(normYawDeg(target.y - mHeadRot.y)));
    };

    // ── Строим мост ────────────────────────────────────────────────────
    // Пока идёт постановка, голова работает ТОЛЬКО по блоку. Раньше резкий
    // поворот мыши на один тик отдавал голову игроку, а следующий тик она
    // снова прыгала к блоку — на быстрой камере это и было «Scaffold дёргает
    // модель сильнее всего»: голова ходила туда-сюда 20 раз в секунду.
    if (want)
    {
        if (!mHeadActive)
        {
            mHeadActive = true;
            mHeadRot = user;
        }
        step(mHeadTarget);
        override = true;
        return mHeadRot;
    }

    // ── Мост не строим ─────────────────────────────────────────────────
    if (!mHeadActive) return user;

    if (userTurn > kFlickDeg)
    {
        // Игрок сам резко повернул — не «догоняем» его шагами, отдаём сразу.
        mHeadRot = user;
        mHeadActive = false;
        return user;
    }

    step(user);
    if (angDiff(mHeadRot, user) < 1.5f)
    {
        // Вернулись к игроку — ротация снова его.
        mHeadActive = false;
        mHeadRot = user;
        return user;
    }
    override = true;
    return mHeadRot;
}

bool Scaffold::headAligned(const glm::vec2& target) const
{
    return mSentValid && viewAngle(mSentRot, target) <= kAlignTolerance;
}

// Клетка пересекается с коробкой игрока? Блок в себя не ставим никогда:
// игра выдавливает игрока из такого блока, и он падает.
bool Scaffold::cellBlocksPlayer(const glm::ivec3& cell) const
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;

    const glm::vec3 eye = *player->getPos();
    const float feetY = eye.y - PLAYER_HEIGHT;

    // Коробка игрока: 0.6 x 1.8, поджатая на миллиметр, чтобы блок вплотную
    // к ногам не считался «внутри».
    const float minX = eye.x - 0.30f + 0.001f, maxX = eye.x + 0.30f - 0.001f;
    const float minY = feetY + 0.001f,          maxY = feetY + 1.80f - 0.001f;
    const float minZ = eye.z - 0.30f + 0.001f, maxZ = eye.z + 0.30f - 0.001f;

    return !(cell.x + 1 <= minX || cell.x >= maxX ||
             cell.y + 1 <= minY || cell.y >= maxY ||
             cell.z + 1 <= minZ || cell.z >= maxZ);
}

// Legit Place. Что ставить прямо сейчас, в порядке важности:
//   1. Клетка моста под ногами, если она пустая — единственная причина
//      «я падаю»: ставим сразу и без ожидания поворота.
//   2. Клетка моста впереди (или позади, если Moonwalk), но не дальше
//      lookAhead — то есть вплотную, когда до края осталось полшага.
//   3. По диагонали — та из двух боковых клеток, что ближе к движению:
//      без неё диагональную просто не к чему прилепить.
// Ничего «за 1-3 блока до того, как дошёл», ничего выше моста, ничего в себя.
void Scaffold::collectPlaceCells(Actor* player, std::vector<glm::ivec3>& out, int maxCells)
{
    out.clear();
    if (!player || !mBridgeYValid || maxCells <= 0) return;

    auto usable = [&](const glm::ivec3& c)
    {
        if (c.y > mBridgeY) return false;                          // выше моста не строим
        if (!BlockUtils::isAirBlock(c)) return false;              // там уже блок
        if (BlockUtils::getBlockPlaceFace(c) == -1) return false;  // не к чему лепить
        if (cellBlocksPlayer(c)) return false;                     // в себя не ставим
        return true;
    };

    if (usable(mUnderCell)) out.push_back(mUnderCell);
    if (static_cast<int>(out.size()) >= maxCells) return;

    if (!Keyboard::isUsingMoveKeys() || !mHasDir) return;

    // Мост растёт по линии: клетки идут от ближней к дальней, иначе дальняя
    // не к чему лепить. Сколько клеток — задаёт режим (1/2/3).
    const BridgeProfile prof = getBridgeProfile();
    const int aheadCells = std::clamp(prof.aheadCells, 1, 4);

    glm::ivec2 step = { static_cast<int>(std::lround(mDir.x)), static_cast<int>(std::lround(mDir.y)) };
    if (step.x == 0 && step.y == 0) step = { 0, 1 };

    for (int k = 1; k <= aheadCells && static_cast<int>(out.size()) < maxCells; ++k)
    {
        const glm::ivec3 c = { mUnderCell.x + step.x * k, mBridgeY, mUnderCell.z + step.y * k };
        if (c == mUnderCell) continue;
        if (!BlockUtils::isAirBlock(c)) continue;                   // уже стоит — идём дальше
        if (BlockUtils::getBlockPlaceFace(c) == -1) break;          // не к чему лепить — дальше некуда
        if (cellBlocksPlayer(c)) continue;
        if (c.y > mBridgeY) break;
        out.push_back(c);
    }
    if (static_cast<int>(out.size()) >= maxCells) return;

    // Диагональ: только для ближней клетки — иначе «боковые» улетят на
    // несколько клеток в сторону.
    const glm::ivec3 ahead = mAheadCell;
    if (aheadCells == 1 && ahead != mUnderCell &&
        ahead.x != mUnderCell.x && ahead.z != mUnderCell.z)
    {
        const glm::ivec3 sideA = { ahead.x, mBridgeY, mUnderCell.z };
        const glm::ivec3 sideB = { mUnderCell.x, mBridgeY, ahead.z };
        const bool aFirst = std::fabs(mDir.x) >= std::fabs(mDir.y);
        for (const glm::ivec3& c : { aFirst ? sideA : sideB, aFirst ? sideB : sideA })
        {
            if (c == mUnderCell) continue;
            if (usable(c))
            {
                out.push_back(c);
                break;
            }
        }
    }
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

    // Камеру мы больше не трогаем: в пакете лежит ровно то, куда смотрит
    // игрок. Это и есть «настоящая» ротация для шага головы.
    const glm::vec2 user = packetRot;
    mUserRot = user;
    mUserRotValid = true;
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

// Своя голова = то, что получил сервер. Камеру не трогаем вообще: наклон
// видят сервер и другие игроки, а не ты.
//
// Тело здесь больше НЕ трогаем. Раньше, когда голова уходила от корпуса
// больше чем на 75°, тело одним прыжком ставилось на прицел — на быстрой
// камере это был рывок корпуса. Корпус теперь считает Interface: покадрово,
// по направлению хода, и без клампов.
void Scaffold::applyLocalModelRotation(Actor* player)
{
    if (!player) return;
    if (!mHeadActive || !mSentValid || !rotationAllowed()) return;

    if (auto* head = player->getActorHeadRotationComponent())
    {
        head->mHeadRot = mSentRot.y;
        head->mOldHeadRot = mSentRot.y;
    }
}

glm::vec3 Scaffold::getRotBasedPos(float extend, float yPos)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return { FLT_MAX, FLT_MAX, FLT_MAX };
    auto* rot = player->getActorRotationComponent();
    if (!rot) return { FLT_MAX, FLT_MAX, FLT_MAX };

    glm::vec2 playerRots = glm::vec2(rot->mPitch, rot->mYaw + MathUtils::getRotationKeyOffset());

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
    // Стиль моста сдвигает точку постановки вперёд/назад.
    const BridgeProfile prof = getBridgeProfile();
    const float e = extend * prof.extendDir;

    if (!mBridgeYValid) return {FLT_MAX, FLT_MAX, FLT_MAX};

    glm::ivec3 blockSel = getRotBasedPos(e, static_cast<float>(mBridgeY));

    int side = BlockUtils::getBlockPlaceFace(blockSel);

    if (side == -1)
    {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return {FLT_MAX, FLT_MAX, FLT_MAX};

        // Падаем — цель под ногами, ниже моста.
        if (player->getFallDistance() > 3.f)
            blockSel.y = static_cast<int>(std::floor(player->getPos()->y - 2.62f));

        blockSel = BlockUtils::getClosestPlacePos(blockSel, mRange.as<float>());
        if (blockSel.x == INT_MAX) return {FLT_MAX, FLT_MAX, FLT_MAX};
        side = BlockUtils::getBlockPlaceFace(blockSel);

        if (side == -1) return {FLT_MAX, FLT_MAX, FLT_MAX};
    }

    if (blockSel.x == INT_MAX) return {FLT_MAX, FLT_MAX, FLT_MAX};

    return blockSel;
}
