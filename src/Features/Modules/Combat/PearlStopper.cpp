#include "PearlStopper.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorType.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Actor/Components/ActorOwnerComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorTypeComponent.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorRotationComponent.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

#include <algorithm>
#include <cmath>

// ══════════════════════════════════════════════════════════════════
//  Enable / Disable
// ══════════════════════════════════════════════════════════════════

void PearlStopper::onEnable()
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);
    mTotalStops = 0;
    mPearlLastPos.clear();
    mPearlLastTime.clear();

    if (player) {
        if (auto* rot = player->getActorRotationComponent())
            mCurrentRotation = { rot->mPitch, rot->mYaw, rot->mYaw };
    }

    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mPredictedPath.clear();
        mTeleportPath.clear();
    }

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &PearlStopper::onRenderEvent>  (this);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Enabled", 2.f, Notification::Type::Info);
}

void PearlStopper::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &PearlStopper::onRenderEvent>  (this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);
    mPearlLastPos.clear();
    mPearlLastTime.clear();

    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mPredictedPath.clear();
        mTeleportPath.clear();
    }

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Disabled", 2.f, Notification::Type::Info);
}

// ══════════════════════════════════════════════════════════════════
//  Block collision helpers
// ══════════════════════════════════════════════════════════════════

bool PearlStopper::isSolidBlock(glm::vec3 pos)
{
    auto* bs = ClientInstance::get()->getBlockSource();
    if (!bs) return false;
    glm::ivec3 bp = glm::ivec3(glm::floor(pos));
    Block* b = bs->getBlock(bp);
    if (!b || !b->mLegacy) return false;
    int id = b->mLegacy->getBlockId();
    if (id == 0) return false;            // air
    if (id >= 8 && id <= 11) return false; // water / lava
    return true;
}

glm::vec3 PearlStopper::ensureNotInBlock(glm::vec3 pos)
{
    int safety = 0;
    while (isSolidBlock(pos) && safety++ < 10)
        pos.y += 1.0f;

    safety = 0;
    while (isSolidBlock(pos + glm::vec3(0.f, 1.8f, 0.f)) && safety++ < 10)
        pos.y += 1.0f;

    return pos;
}

// ══════════════════════════════════════════════════════════════════
//  Pearl detection
// ══════════════════════════════════════════════════════════════════

bool PearlStopper::isEnderPearl(Actor* actor)
{
    if (!actor) return false;
    auto* tc = actor->getActorTypeComponent();
    if (!tc) return false;
    return (static_cast<uint32_t>(tc->mType) & 0xFFu) == 87u;
}

Actor* PearlStopper::getPearlOwner(Actor* pearl)
{
    if (!pearl) return nullptr;
    try {
        auto* reg = pearl->mContext.mRegistry;
        if (!reg) return nullptr;
        auto* oc = reg->try_get<ActorOwnerComponent>(pearl->mContext.mEntityId);
        if (oc && oc->mActor) return oc->mActor;
    } catch (...) {}
    return nullptr;
}

bool PearlStopper::isOwnPearl(Actor* pearl, Actor* localPlayer)
{
    if (!pearl || !localPlayer) return false;
    return getPearlOwner(pearl) == localPlayer;
}

std::vector<PearlStopper::PearlData> PearlStopper::findEnemyPearls(Actor* localPlayer)
{
    std::vector<PearlData> result;
    if (!localPlayer) return result;

    // Используем getActorList без фильтра по игрокам чтобы видеть снаряды
    auto allActors = ActorUtils::getActorList(false, false);
    
    for (Actor* actor : allActors)
    {
        if (!actor) continue;

        // Проверяем что это эндер перла
        if (!isEnderPearl(actor)) continue;

        // Пропускаем свою перлу
        if (isOwnPearl(actor, localPlayer)) continue;

        glm::vec3 pos{}, vel{};
        if (auto* sv = actor->getStateVectorComponent()) {
            pos = sv->mPos;
            vel = sv->mVelocity;
        } else if (auto* p = actor->getPos()) {
            pos = *p;
        } else {
            continue; // нет позиции — пропускаем
        }

        if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z)) continue;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) continue;

        glm::vec3 throwerPos = pos;
        Actor* owner = getPearlOwner(actor);
        if (owner) {
            try {
                auto* ownerPos = owner->getPos();
                if (ownerPos) throwerPos = *ownerPos;
            } catch (...) {}
        }

        PearlData d;
        d.actor      = actor;
        d.runtimeId  = actor->getRuntimeID();
        d.position   = pos;
        d.velocity   = vel;
        d.throwerPos = throwerPos;
        result.push_back(d);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════
//  Reliable velocity — FIXED
// ══════════════════════════════════════════════════════════════════

glm::vec3 PearlStopper::getReliableVelocity(const PearlData& pearl)
{
    uint64_t now = static_cast<uint64_t>(NOW);

    auto posIt  = mPearlLastPos.find(pearl.runtimeId);
    auto timeIt = mPearlLastTime.find(pearl.runtimeId);

    glm::vec3 vel = pearl.velocity;

    if (posIt != mPearlLastPos.end() && timeIt != mPearlLastTime.end())
    {
        float dt = static_cast<float>(now - timeIt->second) / 1000.f;

        // Ждём минимум 1 тик
        if (dt >= 0.04f && dt <= 0.3f)
        {
            glm::vec3 delta    = pearl.position - posIt->second;
            float ticksElapsed = dt / TICK_TIME;

            if (ticksElapsed > 0.1f)
            {
                // Эмпирическая скорость в blocks/tick
                glm::vec3 empVel = delta / ticksElapsed;
                float empLen     = glm::length(empVel);
                float svLen      = glm::length(vel);

                // Разумный диапазон для перлы: 0.5 - 3.0 blocks/tick
                if (empLen > 0.1f && empLen < 4.f)
                {
                    if (svLen > 0.1f && svLen < 4.f)
                        vel = empVel * 0.6f + vel * 0.4f;
                    else
                        vel = empVel;
                }
            }
        }
    }

    mPearlLastPos[pearl.runtimeId]  = pearl.position;
    mPearlLastTime[pearl.runtimeId] = now;

    // Fallback
    float velLen = glm::length(vel);
    if (velLen < 0.1f || velLen > 4.f)
    {
        glm::vec3 dir = pearl.position - pearl.throwerPos;
        float dist = glm::length(dir);
        if (dist > 0.5f)
        {
            vel = glm::normalize(dir) * 1.5f;
        }
        else
        {
            // Перла только что брошена — стандартная скорость вперёд
            vel = glm::vec3(0.f, 0.5f, 1.5f);
        }
    }

    return vel;
}

std::vector<glm::vec3> PearlStopper::simulateTrajectory(
    glm::vec3 startPos, glm::vec3 startVel, int ticks)
{
    std::vector<glm::vec3> path;
    path.reserve(ticks + 1);

    glm::vec3 pos = startPos;
    glm::vec3 vel = startVel;
    path.push_back(pos);

    for (int i = 0; i < ticks; ++i)
    {
        vel.y -= PEARL_GRAVITY;
        vel   *= PEARL_DRAG;
        pos   += vel;

        path.push_back(pos);

        // ФИКС: Игнорируем проверку коллизии первые 3 тика. 
        // Если враг кидает из-за стены или перка спавнится в его модельке, 
        // симуляция не должна моментально обрываться.
        if (i > 3 && isSolidBlock(pos))
            break; // Перка врезалась в стену/пол

        if (pos.y < -64.f) break;
    }
    return path;
}

// ══════════════════════════════════════════════════════════════════
//  Teleport time
// ══════════════════════════════════════════════════════════════════

float PearlStopper::calculateTeleportTime(float distance, float stepDist)
{
    if (distance < 0.01f || std::isnan(distance)) return 0.f;
    float packets = std::ceil(distance / stepDist);
    return (packets + REACTION_TICKS + PING_TICKS) * TICK_TIME;
}

// ══════════════════════════════════════════════════════════════════
//  Find intercept — EARLY as possible, with air check
// ══════════════════════════════════════════════════════════════════

PearlStopper::InterceptResult PearlStopper::findInterceptPoint(
    const std::vector<glm::vec3>& trajectory,
    glm::vec3 playerPos,
    glm::vec3 throwerPos,
    float stepDist)
{
    InterceptResult result;
    if (trajectory.size() < 5) return result; // Траектория слишком короткая

    // Ищем подходящую точку, начиная с 4-го тика полета перлы
    for (size_t tick = 4; tick < trajectory.size(); ++tick)
    {
        const glm::vec3& pearlPos = trajectory[tick];

        // Не тепаемся прямо в лицо врагу (чтобы не было бага с ногами)
        if (glm::distance(pearlPos, throwerPos) < MIN_DIST) continue;

        // Рассчитываем позицию НОГ нашего персонажа. 
        // mYOffset (1.4) значит, что перка пролетит на уровне нашей груди.
        glm::vec3 feetPos = pearlPos - glm::vec3(0.f, mYOffset.mValue, 0.f);

        // Проверяем, свободен ли ВОЗДУХ для нашего хитбокса (ноги, туловище, голова).
        // Если хотя бы одна из этих точек в стене - стоять там нельзя.
        if (isSolidBlock(feetPos) || 
            isSolidBlock(feetPos + glm::vec3(0.f, 1.0f, 0.f)) || 
            isSolidBlock(feetPos + glm::vec3(0.f, 1.8f, 0.f))) 
        {
            continue; // Мы застрянем в стене, ищем точку дальше по траектории
        }

        // Расчет времени: успеем ли мы телепортироваться туда до прилета перлы?
        float distToPoint = glm::distance(playerPos, feetPos);
        int packetsNeeded = static_cast<int>(std::ceil(distToPoint / stepDist));
        
        // Сколько тиков займет наш ТП + пинг + реакция
        int ourTicks = packetsNeeded + REACTION_TICKS + PING_TICKS;

        // ФИКС ТАЙМИНГОВ: Мы должны быть на месте как минимум за 3 тика ДО прилета перлы.
        // Если перла прилетит раньше нас (tick < ourTicks + 3), пропускаем точку.
        if (static_cast<int>(tick) < ourTicks + 3) continue; 

        // Идеальная точка найдена! Возвращаем позицию НОГ.
        result.found     = true;
        result.position  = feetPos; 
        result.tickIndex = static_cast<int>(tick);
        result.distance  = distToPoint;
        return result; 
    }

    return result;
}

// ══════════════════════════════════════════════════════════════════
//  ClickTp-style teleport
// ══════════════════════════════════════════════════════════════════

std::shared_ptr<MovePlayerPacket> PearlStopper::createPacketForPos(glm::vec3 pos)
{
    auto p = ClientInstance::get()->getLocalPlayer();
    auto pk = MinecraftPackets::createPacket<MovePlayerPacket>();
    pk->mPos              = pos;
    pk->mPlayerID         = p->getRuntimeID();
    pk->mRot              = { mCurrentRotation.x, mCurrentRotation.y };
    pk->mYHeadRot         = mCurrentRotation.z;
    pk->mResetPosition    = PositionMode::Teleport;
    pk->mOnGround         = true;
    pk->mRidingID         = -1;
    pk->mCause            = TeleportationCause::Unknown;
    pk->mSourceEntityType = ActorType::Player;
    pk->mTick             = 0;
    return pk;
}

void PearlStopper::straightLineTP(glm::vec3 from, glm::vec3 to, bool save)
{
    if (std::isnan(from.x) || std::isnan(to.x)) return;

    auto* snd = ClientInstance::get()->getPacketSender();
    if (!snd) return;

    float st = mStepDistance.mValue;

    if (glm::length(to - from) < 0.01f)
    {
        snd->sendToServer(createPacketForPos(to).get());
        return;
    }

    glm::vec3 dir = glm::normalize(to - from);
    glm::vec3 cur = from;
    std::vector<glm::vec3> pts;

    while (glm::distance(cur, to) > st)
    {
        cur += dir * st;
        pts.push_back(cur);
        snd->sendToServer(createPacketForPos(cur).get());
    }

    pts.push_back(to);
    snd->sendToServer(createPacketForPos(to).get());

    if (save)
    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mTeleportPath   = std::move(pts);
        mLastRenderTime = static_cast<uint64_t>(NOW);
    }
}

void PearlStopper::teleportTo(glm::vec3 dest)
{
    auto* p = ClientInstance::get()->getLocalPlayer();
    if (!p) return;
    if (std::isnan(dest.x)) return;

    straightLineTP(*p->getPos(), dest, true);
    p->setPosition(dest);

    if (auto* sv = p->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);
}

// ══════════════════════════════════════════════════════════════════
//  Freeze
// ══════════════════════════════════════════════════════════════════

void PearlStopper::freezePlayer(Actor* player)
{
    if (!player) return;

    mSavedCollision = player->getStatusFlag(ActorFlags::HasCollision);
    mSavedGravity   = player->getStatusFlag(ActorFlags::HasGravity);
    mSavedPush      = player->getStatusFlag(ActorFlags::PushTowardsClosestSpace);

    player->setStatusFlag(ActorFlags::HasCollision,            false);
    player->setStatusFlag(ActorFlags::HasGravity,              false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);

    if (auto* sv = player->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);

    mIsFrozen    = true;
    mFrozenTicks = 0;
    mMissedTicks = 0;
}

void PearlStopper::maintainFreeze(Actor* player)
{
    if (!player || !mIsFrozen) return;
    mFrozenTicks++;

    player->setStatusFlag(ActorFlags::HasCollision,            false);
    player->setStatusFlag(ActorFlags::HasGravity,              false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);
    player->setPosition(mInterceptPosition);

    if (auto* sv = player->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);

    auto* sender = ClientInstance::get()->getPacketSender();
    if (sender)
        sender->sendToServer(createPacketForPos(mInterceptPosition).get());
}

void PearlStopper::unfreezePlayer(Actor* player)
{
    if (!player || !mIsFrozen) return;

    player->setStatusFlag(ActorFlags::HasCollision, mSavedCollision);
    player->setStatusFlag(ActorFlags::HasGravity,   mSavedGravity);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, mSavedPush);
    player->setFallDistance(0.f);

    if (auto* sv = player->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);

    mIsFrozen = false;
}

// ══════════════════════════════════════════════════════════════════
//  State helpers
// ══════════════════════════════════════════════════════════════════

bool PearlStopper::isPearlAlive(uint64_t pearlId, Actor* localPlayer)
{
    auto pearls = findEnemyPearls(localPlayer);
    for (const auto& p : pearls)
        if (static_cast<uint64_t>(p.runtimeId) == pearlId)
            return true;
    return false;
}

void PearlStopper::resetState(Actor* player)
{
    if (mIsFrozen && player)
        unfreezePlayer(player);

    mIsActive      = false;
    mIsFrozen      = false;
    mTargetPearlId = 0;
    mFrozenTicks   = 0;
    mMissedTicks   = 0;
}

// ══════════════════════════════════════════════════════════════════
//  PacketInEvent
// ══════════════════════════════════════════════════════════════════

void PearlStopper::onPacketInEvent(PacketInEvent& event)
{
    if (!mSilentMode.mValue) return;
    if (!mIsFrozen)          return;
    if (!event.mPacket)      return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MovePlayerPacket>();
    if (!pkt || pkt->mPlayerID != player->getRuntimeID()) return;

    event.cancel();

    auto* sender = ClientInstance::get()->getPacketSender();
    if (sender)
        sender->sendToServer(createPacketForPos(mInterceptPosition).get());
}

// ══════════════════════════════════════════════════════════════════
//  BaseTickEvent — MAIN LOGIC (velocity computed ONCE per tick)
// ══════════════════════════════════════════════════════════════════

void PearlStopper::onBaseTickEvent(BaseTickEvent& event)
{
    auto* player = event.mActor;
    if (!player) return;

    if (auto* rot = player->getActorRotationComponent())
        mCurrentRotation = { rot->mPitch, rot->mYaw, rot->mYaw };

    // ── FROZEN ──────────────────────────────────────────────────
    if (mIsFrozen)
    {
        if (mFrozenTicks > MAX_FREEZE_TICKS)
        {
            if (mNotifications.mValue)
                NotifyUtils::notify("[PearlStopper] Timeout!", 2.f, Notification::Type::Warning);

            unfreezePlayer(player);
            if (mTeleportBack.mValue)
                teleportTo(mOriginalPosition);
            resetState(player);
            return;
        }

        maintainFreeze(player);

        if (!isPearlAlive(mTargetPearlId, player))
        {
            mMissedTicks++;
            if (mMissedTicks > MISSED_TOLERANCE)
            {
                mTotalStops++;
                unfreezePlayer(player);

                if (mTeleportBack.mValue)
                {
                    teleportTo(mOriginalPosition);
                    if (mNotifications.mValue)
                        NotifyUtils::notify("[PearlStopper] Blocked! Returning...", 2.f, Notification::Type::Info);
                }
                else
                {
                    if (mNotifications.mValue)
                        NotifyUtils::notify("[PearlStopper] Blocked!", 2.f, Notification::Type::Info);
                }

                resetState(player);
            }
        }
        else
        {
            mMissedTicks = 0;
        }
        return;
    }

    // ── SCAN ────────────────────────────────────────────────────
    glm::vec3 playerPos = *player->getPos();
    auto enemyPearls = findEnemyPearls(player);

    // Clean dead trackers
    for (auto it = mPearlLastPos.begin(); it != mPearlLastPos.end(); )
    {
        bool found = false;
        for (const auto& p : enemyPearls)
            if (p.runtimeId == it->first) { found = true; break; }
        it = found ? std::next(it) : mPearlLastPos.erase(it);
    }
    for (auto it = mPearlLastTime.begin(); it != mPearlLastTime.end(); )
    {
        bool found = false;
        for (const auto& p : enemyPearls)
            if (p.runtimeId == it->first) { found = true; break; }
        it = found ? std::next(it) : mPearlLastTime.erase(it);
    }

    // Compute velocity ONCE for every pearl this tick
    std::unordered_map<int64_t, glm::vec3> pearlVels;
    for (auto& pearl : enemyPearls)
        pearlVels[pearl.runtimeId] = getReliableVelocity(pearl);

    // Pick best pearl
    PearlData* best    = nullptr;
    float      bestScore = FLT_MAX;

    for (auto& pearl : enemyPearls)
    {
        glm::vec3 vel = pearlVels[pearl.runtimeId];
        if (glm::length(vel) < 0.05f) continue;

        float dist = glm::distance(pearl.position, playerPos);
        if (dist > MAX_DIST) continue;

        glm::vec3 toPlayer = glm::normalize(playerPos - pearl.position);
        float approach = glm::dot(glm::normalize(vel), toPlayer);
        float score = dist * (3.0f - approach * 2.0f);

        if (score < bestScore)
        {
            bestScore = score;
            best = &pearl;
        }
    }

    if (!best) return;

    // Simulate using pre-computed velocity
    glm::vec3 vel = pearlVels[best->runtimeId];
    auto trajectory = simulateTrajectory(best->position, vel, PREDICT_TICKS);

    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mPredictedPath = trajectory;
    }

    auto intercept = findInterceptPoint(trajectory, playerPos, best->throwerPos, mStepDistance.mValue);
    if (!intercept.found) return;

    // ВАЖНО: Мы больше не прибавляем mYOffset здесь и НЕ ИСПОЛЬЗУЕМ ensureNotInBlock.
    // Функция findInterceptPoint уже вернула идеальные координаты для ног,
    // где нет блоков, а перка пролетит ровно в грудь.
    glm::vec3 interceptPos = intercept.position;

    mOriginalPosition  = playerPos;

    mOriginalPosition  = playerPos;
    mInterceptPosition = interceptPos;
    mTargetPearlId     = static_cast<uint64_t>(best->runtimeId);
    mIsActive          = true;
    mMissedTicks       = 0;

    if (mNotifications.mValue)
    {
        int dist = static_cast<int>(intercept.distance);
        NotifyUtils::notify(
            "[PearlStopper] Intercepting! " + std::to_string(dist) + "b",
            1.5f, Notification::Type::Info
        );
    }

    freezePlayer(player);
    teleportTo(interceptPos);
}

// ══════════════════════════════════════════════════════════════════
//  Render
// ══════════════════════════════════════════════════════════════════

void PearlStopper::onRenderEvent(RenderEvent& event)
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    float alpha = 0.f;
    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        constexpr uint64_t FADE_MS = 2000;
        if (mIsFrozen) {
            alpha = 1.f;
        } else if (static_cast<uint64_t>(NOW) <= mLastRenderTime + FADE_MS) {
            float e = static_cast<float>(static_cast<uint64_t>(NOW) - mLastRenderTime);
            alpha = std::clamp(1.f - e / static_cast<float>(FADE_MS), 0.f, 1.f);
        }
    }

    // Pearl trajectory (yellow)
    if (mDrawPrediction.mValue)
    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        for (size_t i = 0; i + 1 < mPredictedPath.size(); ++i)
        {
            ImVec2 a, b;
            if (!RenderUtils::worldToScreen(mPredictedPath[i],     a)) continue;
            if (!RenderUtils::worldToScreen(mPredictedPath[i + 1], b)) continue;
            dl->AddLine(a, b, ImColor(1.f, 1.f, 0.f, mIsFrozen ? 0.7f : 0.4f), 2.f);
        }
    }

    // TP path (themed)
    if (alpha > 0.01f)
    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        for (size_t i = 0; i + 1 < mTeleportPath.size(); ++i)
        {
            ImVec2 a, b;
            if (!RenderUtils::worldToScreen(mTeleportPath[i],     a)) continue;
            if (!RenderUtils::worldToScreen(mTeleportPath[i + 1], b)) continue;
            ImColor c = ColorUtils::getThemedColor(static_cast<float>(i) * 0.05f);
            c.Value.w *= alpha;
            dl->AddLine(a, b, c, 2.5f);
        }
    }

    // Intercept point (orange)
    if (mIsFrozen)
    {
        ImVec2 sp;
        if (RenderUtils::worldToScreen(mInterceptPosition, sp))
        {
            dl->AddCircleFilled(sp, 6.f,  ImColor(1.f, 0.5f, 0.f, 0.95f));
            dl->AddCircle      (sp, 11.f, ImColor(1.f, 0.5f, 0.f, 0.6f), 0, 2.5f);
        }
    }

    // HUD
    ImVec2 scr = ImGui::GetIO().DisplaySize;

    if (mIsFrozen)
    {
        const char* txt = "BLOCKING PEARL!";
        ImVec2 ts = ImGui::CalcTextSize(txt);
        ImVec2 p  = { scr.x * 0.5f - ts.x * 0.5f, scr.y * 0.35f };
        dl->AddText({ p.x + 2, p.y + 2 }, ImColor(0.f, 0.f, 0.f, 0.8f), txt);
        dl->AddText(p, ImColor(1.f, 0.3f, 0.3f, 1.f), txt);

        float timeLeft = static_cast<float>(MAX_FREEZE_TICKS - mFrozenTicks) * TICK_TIME;
        std::string timer = "Time: " + std::to_string(static_cast<int>(timeLeft)) + "s";
        ImVec2 ts2 = ImGui::CalcTextSize(timer.c_str());
        ImVec2 p2  = { scr.x * 0.5f - ts2.x * 0.5f, p.y + ts.y + 4.f };
        dl->AddText(p2, ImColor(1.f, 1.f, 1.f, 0.7f), timer.c_str());
    }

    if (mTotalStops > 0)
    {
        std::string s = "Stopped: " + std::to_string(mTotalStops);
        ImVec2 ts = ImGui::CalcTextSize(s.c_str());
        ImVec2 p  = { scr.x * 0.5f - ts.x * 0.5f, scr.y * 0.35f + 40.f };
        dl->AddText(p, ImColor(0.3f, 1.f, 0.3f, 0.9f), s.c_str());
    }
}