#include "PearlStopper.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorType.hpp>
#include <SDK/Minecraft/Actor/Components/ActorOwnerComponent.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
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
#include <mutex>

// ─────────────────────────────────────────────────────────────────────────────
//  Enable / Disable
// ─────────────────────────────────────────────────────────────────────────────

void PearlStopper::onEnable() {
    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);

    mPearlPrevPos.clear();
    mPearlPrevTime.clear();
    mKnownPearls.clear();
    mTotalStops = 0;

    if (player) {
        if (auto* rot = player->getActorRotationComponent())
            mRots = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);
    }

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &PearlStopper::onRenderEvent>(this);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Active", 2.0f, Notification::Type::Info);
}

void PearlStopper::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &PearlStopper::onRenderEvent>(this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Disabled", 2.0f, Notification::Type::Info);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Block helpers
// ─────────────────────────────────────────────────────────────────────────────

bool PearlStopper::isSolidAt(glm::vec3 pos) {
    auto* bs = ClientInstance::get()->getBlockSource();
    if (!bs) return false;
    glm::ivec3 bp = glm::ivec3(glm::floor(pos));
    Block* block = bs->getBlock(bp);
    if (!block || !block->mLegacy) return false;
    int id = block->mLegacy->getBlockId();
    if (id == 0 || (id >= 8 && id <= 11)) return false;
    return true;
}

bool PearlStopper::isSpaceClear(glm::vec3 feetPos) {
    if (isSolidAt(feetPos))                               return false;
    if (isSolidAt(feetPos + glm::vec3(0.f, 0.9f, 0.f)))  return false;
    if (isSolidAt(feetPos + glm::vec3(0.f, 1.8f, 0.f)))  return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pearl detection
// ─────────────────────────────────────────────────────────────────────────────

bool PearlStopper::isEnderPearl(Actor* a) {
    if (!a) return false;
    auto* tc = a->getActorTypeComponent();
    return tc && (static_cast<uint32_t>(tc->mType) & 0xFFu) == 87u;
}

Actor* PearlStopper::getPearlOwner(Actor* pearl) {
    if (!pearl) return nullptr;
    try {
        auto* reg = pearl->mContext.mRegistry;
        if (!reg) return nullptr;
        auto* oc = reg->try_get<ActorOwnerComponent>(pearl->mContext.mEntityId);
        if (oc && oc->mActor) return oc->mActor;
    } catch (...) {}
    return nullptr;
}

bool PearlStopper::isOwnPearl(Actor* pearl, Actor* local) {
    if (!pearl || !local) return false;
    return getPearlOwner(pearl) == local;
}

bool PearlStopper::isPearlStillAlive(int64_t runtimeId) {
    if (runtimeId == 0) return false;
    Actor* a = ActorUtils::getActorFromRuntimeID(runtimeId);
    return a != nullptr && isEnderPearl(a);
}

std::vector<PearlStopper::PearlData> PearlStopper::findEnemyPearls(Actor* local) {
    std::vector<PearlData> result;
    if (!local) return result;

    // Собираем текущие живые перлы
    std::unordered_map<int64_t, Actor*> currentAlive;
    for (Actor* a : ActorUtils::getActorList(false, false)) {
        if (!a || !isEnderPearl(a)) continue;
        if (isOwnPearl(a, local)) continue;
        currentAlive[a->getRuntimeID()] = a;
    }

    // Удаляем из кеша перлы которых уже нет
    for (auto it = mKnownPearls.begin(); it != mKnownPearls.end(); ) {
        if (currentAlive.find(it->first) == currentAlive.end())
            it = mKnownPearls.erase(it);
        else
            ++it;
    }

    // Добавляем новые / обновляем существующие
    for (auto& [id, a] : currentAlive) {
        auto* sv = a->getStateVectorComponent();
        if (!sv) continue;

        glm::vec3 pos = sv->mPos;
        glm::vec3 vel = sv->mVelocity;

        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) continue;
        if (!std::isfinite(vel.x) || !std::isfinite(vel.y) || !std::isfinite(vel.z)) continue;

        // Если перла уже была в кеше — берём velocity из кеша если текущий нулевой
        // (сервер иногда присылает vel=0 в первый тик из-за лага)
        auto it = mKnownPearls.find(id);
        if (it != mKnownPearls.end()) {
            float curLen   = glm::length(vel);
            float cacheLen = glm::length(it->second.velocity);
            // Если текущий velocity подозрительно мал а кешированный нормальный — используем кеш
            if (curLen < 0.02f && cacheLen > 0.05f) {
                // Симулируем следующий тик из кеша
                glm::vec3 predicted = it->second.velocity * PEARL_DRAG;
                predicted.y -= PEARL_GRAVITY;
                vel = predicted;
            }
        }

        PearlData d;
        d.actor     = a;
        d.runtimeId = id;
        d.position  = pos;
        d.velocity  = vel;

        mKnownPearls[id] = d;
        result.push_back(d);
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Velocity estimation
//  Вызывается ОДИН РАЗ за тик на каждую пёрку.
//  Берёт реальное смещение между тиками и блендит с сырым velocity из StateVector.
// ─────────────────────────────────────────────────────────────────────────────

glm::vec3 PearlStopper::estimateVelocity(PearlData& pearl, uint64_t nowMs) {
    glm::vec3 vel = pearl.velocity;

    auto itPos  = mPearlPrevPos.find(pearl.runtimeId);
    auto itTime = mPearlPrevTime.find(pearl.runtimeId);

    if (itPos != mPearlPrevPos.end() && itTime != mPearlPrevTime.end()) {
        float dtSec = static_cast<float>(nowMs - itTime->second) / 1000.0f;
        if (dtSec >= 0.035f && dtSec <= 0.12f) {
            glm::vec3 empirical = (pearl.position - itPos->second) / dtSec * 0.05f;
            float lenE = glm::length(empirical);
            if (lenE > 0.05f && lenE < 2.5f)
                vel = empirical * 0.7f + vel * 0.3f;
        }
    }

    mPearlPrevPos[pearl.runtimeId]  = pearl.position;
    mPearlPrevTime[pearl.runtimeId] = nowMs;

    float len = glm::length(vel);
    if (!std::isfinite(len) || len < 0.01f || len > 3.0f)
        vel = pearl.velocity;

    return vel;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Trajectory simulation
//  Физика Bedrock EnderPearl каждый тик:
//    1. vel *= 0.99    (drag — первым)
//    2. vel.y -= 0.03  (gravity — после drag)
//    3. pos += vel
// ─────────────────────────────────────────────────────────────────────────────

std::vector<glm::vec3> PearlStopper::simulateTrajectory(glm::vec3 startPos,
                                                          glm::vec3 startVel,
                                                          int       maxTicks) {
    std::vector<glm::vec3> path;
    path.reserve(static_cast<size_t>(maxTicks) + 1);

    glm::vec3 p = startPos;
    glm::vec3 v = startVel;
    path.push_back(p);

    for (int t = 0; t < maxTicks; ++t) {
        v   *= PEARL_DRAG;
        v.y -= PEARL_GRAVITY;
        p   += v;
        path.push_back(p);

        // Прерываем только если перл врезался в блок И уже не летит вверх
        // Если velocity.y > 0 — перл ещё летит вверх (брошен снизу), не прерываем
        if (t > 3 && v.y <= 0.f && isSolidAt(p)) break;
        if (p.y < -64.f) break;
    }
    return path;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Intercept finder
//  Ищем первую точку траектории где:
//    - место свободно (нет блоков в колонне игрока)
//    - у нас есть достаточно тиков чтобы успеть телепортироваться
//
//  Позиция ног: candidateFeet.y = pearl.y - 0.9 + mYOffset
//  (пёрка на уровне центра хитбокса = 0.9м над ногами)
//
//  REACTION=3 и PING=2 — внутренние константы, пользователю не нужны.
// ─────────────────────────────────────────────────────────────────────────────

PearlStopper::InterceptResult PearlStopper::findIntercept(
    const std::vector<glm::vec3>& traj,
    glm::vec3 ourFeetPos)
{
    InterceptResult res;
    if (traj.size() < 4) return res;

    // Все пакеты TP улетают в один тик (как в ClickTP),
    // поэтому нам нужно всего: 1 тик на TP + запас на движок и пинг
    static constexpr int REACTION    = 3;
    static constexpr int PING        = 2;
    const int            ticksNeeded = 1 + REACTION + PING;

    for (size_t t = 4; t < traj.size(); ++t) {
        const glm::vec3& pearlPos = traj[t];

        if (static_cast<int>(t) <= ticksNeeded) continue;

        // Базовая позиция ног
        glm::vec3 candidateFeet;
        candidateFeet.x = pearlPos.x;
        candidateFeet.z = pearlPos.z;
        candidateFeet.y = pearlPos.y - 0.9f + mYOffset.mValue;

        // Если базовая позиция занята — пробуем найти свободное место
        // рядом по Y (перл мог прилететь снизу сквозь пол)
        bool found = false;
        for (int yTry = 0; yTry <= 3; yTry++) {
            glm::vec3 tryPos = candidateFeet + glm::vec3(0.f, (float)yTry, 0.f);
            if (isSpaceClear(tryPos)) {
                candidateFeet = tryPos;
                found = true;
                break;
            }
        }
        if (!found) continue;

        res.found     = true;
        res.feetPos   = candidateFeet;
        res.tickIndex = static_cast<int>(t);
        return res;
    }
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Teleport — 1:1 копия ClickTP
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<MovePlayerPacket> PearlStopper::createPacketForPos(glm::vec3 pos) {
    auto* p = ClientInstance::get()->getLocalPlayer();
    auto  pk = MinecraftPackets::createPacket<MovePlayerPacket>();
    pk->mPos              = pos;
    pk->mPlayerID         = p->getRuntimeID();
    pk->mRot              = { mRots.x, mRots.y };
    pk->mYHeadRot         = mRots.z;
    pk->mResetPosition    = PositionMode::Teleport;
    pk->mOnGround         = true;
    pk->mRidingID         = -1;
    pk->mCause            = TeleportationCause::Unknown;
    pk->mSourceEntityType = ActorType::Player;
    pk->mTick             = 0;
    return pk;
}

void PearlStopper::straightLineTP(glm::vec3 from, glm::vec3 to, bool save) {
    auto* snd = ClientInstance::get()->getPacketSender();
    if (!snd) return;
    float st = mStepDistance.mValue;
    if (glm::length(to - from) < 0.01f) { snd->sendToServer(createPacketForPos(to).get()); return; }
    glm::vec3 dir = glm::normalize(to - from), cur = from;
    std::vector<glm::vec3> pts;
    while (glm::distance(cur, to) > st) { cur += dir * st; pts.push_back(cur); snd->sendToServer(createPacketForPos(cur).get()); }
    pts.push_back(to); snd->sendToServer(createPacketForPos(to).get());
    if (save) { std::lock_guard<std::mutex> lk(mMutex); mPacketPositions = pts; mLastPathTime = NOW; }
}

void PearlStopper::teleportTo(glm::vec3 dest) {
    auto* p = ClientInstance::get()->getLocalPlayer();
    if (!p) return;
    straightLineTP(*p->getPos(), dest, true);
    p->setPosition(dest);
    if (auto* sv = p->getStateVectorComponent()) sv->mVelocity = glm::vec3(0.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Freeze / Unfreeze
// ─────────────────────────────────────────────────────────────────────────────

void PearlStopper::freezeAt(Actor* player, glm::vec3 feetPos) {
    if (!player) return;

    mSavedCollision = player->getStatusFlag(ActorFlags::HasCollision);
    mSavedGravity   = player->getStatusFlag(ActorFlags::HasGravity);
    mSavedPush      = player->getStatusFlag(ActorFlags::PushTowardsClosestSpace);

    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);
    if (auto* sv = player->getStateVectorComponent()) sv->mVelocity = glm::vec3(0.f);

    teleportTo(feetPos);

    player->setPosition(feetPos);
    if (auto* sv = player->getStateVectorComponent()) {
        sv->mPos      = feetPos;
        sv->mPosOld   = feetPos;
        sv->mVelocity = glm::vec3(0.f);
    }

    mInterceptFeetPos = feetPos;
    mIsFrozen         = true;
    mFrozenTicks      = 0;
    mMissedTicks      = 0;
}

void PearlStopper::maintainFreeze(Actor* player) {
    if (!player || !mIsFrozen) return;
    mFrozenTicks++;

    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);

    player->setPosition(mInterceptFeetPos);
    if (auto* sv = player->getStateVectorComponent()) {
        sv->mPos      = mInterceptFeetPos;
        sv->mPosOld   = mInterceptFeetPos;
        sv->mVelocity = glm::vec3(0.f);
    }

    auto* snd = ClientInstance::get()->getPacketSender();
    if (snd) snd->sendToServer(createPacketForPos(mInterceptFeetPos).get());
}

void PearlStopper::unfreeze(Actor* player) {
    if (!player || !mIsFrozen) return;
    player->setStatusFlag(ActorFlags::HasCollision, mSavedCollision);
    player->setStatusFlag(ActorFlags::HasGravity, mSavedGravity);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, mSavedPush);
    player->setFallDistance(0.f);
    mIsFrozen = false;
}

void PearlStopper::resetState(Actor* player) {
    if (mIsFrozen && player) unfreeze(player);
    mIsActive      = false;
    mIsFrozen      = false;
    mTargetPearlId = 0;
    mFrozenTicks   = 0;
    mMissedTicks   = 0;
    std::lock_guard<std::mutex> lk(mMutex);
    mPredictedPath.clear();
    mPacketPositions.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Main tick
// ─────────────────────────────────────────────────────────────────────────────

void PearlStopper::onBaseTickEvent(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    if (auto* rot = player->getActorRotationComponent())
        mRots = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);

    // ── Заморожены: держим позицию, ждём пёрку ───────────────────────────────
    if (mIsFrozen) {
        if (mFrozenTicks > MAX_FROZEN_TICKS) {
            unfreeze(player);
            if (mTeleportBack.mValue) teleportTo(mOriginalPos);
            resetState(player);
            if (mNotifications.mValue)
                NotifyUtils::notify("[PearlStopper] Timeout", 2.f, Notification::Type::Warning);
            return;
        }

        maintainFreeze(player);

        if (!isPearlStillAlive(mTargetPearlId)) {
            mMissedTicks++;
            if (mMissedTicks >= MISSED_TOLERANCE) {
                mTotalStops++;
                unfreeze(player);
                if (mTeleportBack.mValue) {
                    teleportTo(mOriginalPos);
                    if (mNotifications.mValue)
                        NotifyUtils::notify("[PearlStopper] Pearl stopped! Returned.", 2.5f, Notification::Type::Info);
                } else {
                    if (mNotifications.mValue)
                        NotifyUtils::notify("[PearlStopper] Pearl stopped!", 2.5f, Notification::Type::Info);
                }
                resetState(player);
            }
        } else {
            mMissedTicks = 0;
        }
        return;
    }

    // ── Сканируем: ищем вражеские пёрки ──────────────────────────────────────
    auto pearls = findEnemyPearls(player);
    if (pearls.empty()) return;

    uint64_t  nowMs  = NOW;
    glm::vec3 myFeet = *player->getPos();

    // Обновляем историю скоростей — ОДИН вызов на пёрку за тик
    for (auto& p : pearls) estimateVelocity(p, nowMs);

    // Выбираем самую угрожающую пёрку
    PearlData* best      = nullptr;
    float      bestScore = 1e9f;

    for (auto& pearl : pearls) {
        float dist = glm::distance(pearl.position, myFeet);
        if (dist > 120.f) continue; // увеличили радиус — ловим дальние броски

        float speedLen = glm::length(pearl.velocity);
        // Снижен порог: даже если velocity почти 0 в первый тик — всё равно рассматриваем
        // При лаге сервер может прислать vel=0 в первый пакет
        if (speedLen < 0.005f) continue;

        float score = dist; // базово — ближайшая

        // Учитываем направление только в горизонтальной плоскости
        // чтобы не штрафовать перлы летящие снизу вверх (вертикальные броски)
        if (speedLen > 0.02f) {
            glm::vec2 velXZ    = glm::vec2(pearl.velocity.x, pearl.velocity.z);
            glm::vec2 toUsXZ   = glm::vec2(myFeet.x - pearl.position.x,
                                            myFeet.z - pearl.position.z);
            float lenXZ = glm::length(toUsXZ);
            if (lenXZ > 0.01f) {
                float approach = glm::dot(glm::normalize(velXZ),
                                          glm::normalize(toUsXZ));
                // Горизонтальное приближение важно, но вертикаль не штрафуем
                score = dist * (1.3f - approach * 1.0f);
            }
        }

        if (score < bestScore) { bestScore = score; best = &pearl; }
    }

    if (!best) return;

    // Продвигаем velocity на один шаг — StateVector уже применил текущий тик,
    // симуляция должна начинаться со следующего
    glm::vec3 simVel = best->velocity;

    // Если velocity почти нулевой (лаг/первый тик) — пробуем восстановить из истории позиций
    if (glm::length(simVel) < 0.02f) {
        auto itPos  = mPearlPrevPos.find(best->runtimeId);
        auto itTime = mPearlPrevTime.find(best->runtimeId);
        if (itPos != mPearlPrevPos.end() && itTime != mPearlPrevTime.end()) {
            float dtSec = static_cast<float>(nowMs - itTime->second) / 1000.0f;
            if (dtSec > 0.01f && dtSec < 0.5f) {
                glm::vec3 empirical = (best->position - itPos->second) / dtSec * 0.05f;
                if (glm::length(empirical) > 0.02f)
                    simVel = empirical;
            }
        }
        // Если всё ещё нулевой — пропускаем эту перлу в этом тике
        // но НЕ удаляем из кеша — поймаем в следующем тике
        if (glm::length(simVel) < 0.005f) {
            // Сохраняем позицию для следующего тика
            mPearlPrevPos[best->runtimeId]  = best->position;
            mPearlPrevTime[best->runtimeId] = nowMs;
            return;
        }
    }

    simVel   *= PEARL_DRAG;
    simVel.y -= PEARL_GRAVITY;

    auto traj = simulateTrajectory(best->position, simVel, MAX_SIM_TICKS);

    {
        std::lock_guard<std::mutex> lk(mMutex);
        mPredictedPath = traj;
    }

    auto intercept = findIntercept(traj, myFeet);
    if (!intercept.found) return;

    mOriginalPos   = myFeet;
    mTargetPearlId = best->runtimeId;
    mIsActive      = true;

    if (mNotifications.mValue)
        NotifyUtils::notify(
            "[PearlStopper] Intercepting in ~" + std::to_string(intercept.tickIndex) + " ticks",
            1.8f, Notification::Type::Info);

    freezeAt(player, intercept.feetPos);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Packet filter — отменяем rubber-band от сервера пока заморожены
// ─────────────────────────────────────────────────────────────────────────────

void PearlStopper::onPacketInEvent(PacketInEvent& event) {
    if (!mIsFrozen) return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt->mPlayerID != player->getRuntimeID()) return;

    event.cancel();

    auto* snd = ClientInstance::get()->getPacketSender();
    if (snd) snd->sendToServer(createPacketForPos(mInterceptFeetPos).get());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Render
// ─────────────────────────────────────────────────────────────────────────────

void PearlStopper::onRenderEvent(RenderEvent& event) {
    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    uint64_t now = NOW;

    // Жёлтая линия — траектория пёрки
    if (mDrawPrediction.mValue) {
        std::lock_guard<std::mutex> lk(mMutex);

        float alpha = mIsFrozen ? 1.f : 0.f;
        if (!mIsFrozen && now < mLastPathTime + 1500)
            alpha = std::clamp(1.f - float(now - mLastPathTime) / 1500.f, 0.f, 1.f);

        if (alpha > 0.02f) {
            for (size_t i = 0; i + 1 < mPredictedPath.size(); ++i) {
                ImVec2 a, b;
                if (!RenderUtils::worldToScreen(mPredictedPath[i],     a)) continue;
                if (!RenderUtils::worldToScreen(mPredictedPath[i + 1], b)) continue;
                dl->AddLine(a, b, ImColor(1.0f, 0.85f, 0.0f, alpha * 0.75f), 2.0f);
            }
        }
    }

    // Синяя линия — путь нашего TP (как mPacketPositions в ClickTP)
    {
        std::lock_guard<std::mutex> lk(mMutex);

        float alpha = 0.f;
        if (mIsFrozen)
            alpha = 1.f;
        else if (now < mLastPathTime + 1500)
            alpha = std::clamp(1.f - float(now - mLastPathTime) / 1500.f, 0.f, 1.f);
        else
            mPacketPositions.clear();

        if (alpha > 0.02f && !mPacketPositions.empty()) {
            std::vector<ImVec2> pts;
            pts.reserve(mPacketPositions.size());
            for (auto& p : mPacketPositions) {
                ImVec2 s;
                if (RenderUtils::worldToScreen(p, s)) pts.push_back(s);
            }
            for (size_t i = 0; i + 1 < pts.size(); ++i)
                dl->AddLine(pts[i], pts[i + 1], ImColor(0.1f, 0.65f, 1.0f, alpha * 0.9f), 2.5f);
        }
    }

    // Оранжевый кружок — точка перехвата
    if (mIsFrozen) {
        ImVec2 sp;
        if (RenderUtils::worldToScreen(mInterceptFeetPos, sp)) {
            dl->AddCircleFilled(sp, 6.f,  ImColor(1.0f, 0.5f,  0.0f, 0.95f));
            dl->AddCircle      (sp, 12.f, ImColor(1.0f, 0.75f, 0.0f, 0.55f), 0, 2.0f);
        }
    }

    // Счётчик
    if (mTotalStops > 0) {
        ImVec2 scr = ImGui::GetIO().DisplaySize;
        std::string txt = "Pearls stopped: " + std::to_string(mTotalStops);
        ImVec2 ts  = ImGui::CalcTextSize(txt.c_str());
        ImVec2 pos = { scr.x * 0.5f - ts.x * 0.5f, scr.y * 0.27f };
        dl->AddText({ pos.x + 1, pos.y + 1 }, ImColor(0, 0, 0, 180),             txt.c_str());
        dl->AddText(pos,                       ImColor(0.1f, 1.0f, 0.15f, 1.0f),  txt.c_str());
    }
}
