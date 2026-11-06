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

// ═════════════════════════════════════════════════════════════════════════════
//  Enable / Disable
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onEnable() {
    auto* player = ClientInstance::get()->getLocalPlayer();
    fullReset(player);

    mTrackedPearls.clear();
    mTotalStops = 0;

    if (player) {
        if (auto* rot = player->getActorRotationComponent())
            mRots = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);
    }

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PearlStopper::onBaseTick>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &PearlStopper::onPacketIn>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &PearlStopper::onRender>(this);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Active", 2.0f, Notification::Type::Info);
}

void PearlStopper::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PearlStopper::onBaseTick>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &PearlStopper::onPacketIn>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &PearlStopper::onRender>(this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    fullReset(player);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Disabled", 2.0f, Notification::Type::Info);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Block helpers
// ═════════════════════════════════════════════════════════════════════════════

bool PearlStopper::isSolidAt(glm::vec3 pos) {
    auto* bs = ClientInstance::get()->getBlockSource();
    if (!bs) return false;
    glm::ivec3 bp = glm::ivec3(glm::floor(pos));
    Block* block = bs->getBlock(bp);
    if (!block || !block->mLegacy) return false;
    int id = block->mLegacy->getBlockId();
    // Air=0, Water=8-9, Lava=10-11 — все считаются проходимыми
    return id != 0 && !(id >= 8 && id <= 11);
}

bool PearlStopper::isSpaceClear(glm::vec3 feetPos) {
    // Проверяем три точки по высоте хитбокса: ноги, середина, голова
    if (isSolidAt(feetPos))                                return false;
    if (isSolidAt(feetPos + glm::vec3(0.f, 0.9f,  0.f)))  return false;
    if (isSolidAt(feetPos + glm::vec3(0.f, 1.62f, 0.f)))  return false;
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Pearl detection
// ═════════════════════════════════════════════════════════════════════════════

bool PearlStopper::isEnderPearl(Actor* a) {
    if (!a) return false;
    auto* tc = a->getActorTypeComponent();
    return tc && (static_cast<uint32_t>(tc->mType) & 0xFFu) == 87u;
}

bool PearlStopper::isOwnPearl(Actor* pearl, Actor* local) {
    if (!pearl || !local) return false;
    try {
        auto* reg = pearl->mContext.mRegistry;
        if (!reg) return false;
        auto* oc = reg->try_get<ActorOwnerComponent>(pearl->mContext.mEntityId);
        if (oc && oc->mActor) return oc->mActor == local;
    } catch (...) {}
    return false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Pearl tracking
//  Каждый тик обновляем кеш TrackedPearl для всех вражеских пёрл.
//  Сохраняем prevPos для вычисления velocity из дельты позиций.
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::updateTrackedPearls(Actor* local) {
    if (!local) return;

    // Собираем живые вражеские пёрлы
    std::unordered_map<int64_t, Actor*> alive;
    for (Actor* a : ActorUtils::getActorList(false, false)) {
        if (!a || !isEnderPearl(a)) continue;
        if (isOwnPearl(a, local))   continue;
        alive[a->getRuntimeID()] = a;
    }

    // Удаляем из кеша мёртвые пёрлы
    for (auto it = mTrackedPearls.begin(); it != mTrackedPearls.end(); ) {
        if (alive.find(it->first) == alive.end())
            it = mTrackedPearls.erase(it);
        else
            ++it;
    }

    // Обновляем / добавляем
    for (auto& [id, actor] : alive) {
        auto* sv = actor->getStateVectorComponent();
        if (!sv) continue;

        glm::vec3 pos = sv->mPos;
        glm::vec3 vel = sv->mVelocity;

        // Валидация NaN/Inf
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z))
            continue;
        if (!std::isfinite(vel.x)) vel.x = 0.f;
        if (!std::isfinite(vel.y)) vel.y = 0.f;
        if (!std::isfinite(vel.z)) vel.z = 0.f;

        auto it = mTrackedPearls.find(id);
        if (it != mTrackedPearls.end()) {
            // Существующая пёрла — сохраняем предыдущую позицию для delta-velocity
            it->second.prevPos = it->second.pos;
            it->second.hasPrev = true;
            it->second.pos     = pos;
            it->second.vel     = vel;
            it->second.age++;
        } else {
            // Новая пёрла
            TrackedPearl tp;
            tp.runtimeId = id;
            tp.pos       = pos;
            tp.vel       = vel;
            tp.hasPrev   = false;
            tp.age       = 0;
            mTrackedPearls[id] = tp;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Velocity estimation — КЛЮЧЕВОЙ ФИКС
//
//  Старый код:
//    empirical = (pos - prev) / dtSec * 0.05f  ← МУСОР
//    vel = empirical * 0.7 + stateVel * 0.3    ← ещё хуже
//
//  Новый код:
//    1. Берём velocity из StateVectorComponent (уже в blocks/tick)
//    2. Если нулевой — используем (pos - prevPos) напрямую
//       pos - prevPos = перемещение за 1 тик = velocity (blocks/tick)
//       Без деления, без коэффициентов, без блендинга
// ═════════════════════════════════════════════════════════════════════════════

glm::vec3 PearlStopper::getReliableVelocity(TrackedPearl& pearl) {
    // Источник 1 (ПРИОРИТЕТ): дельта позиций между тиками
    // ВСЕГДА точная — вычисляется из реальных позиций синхронизированных сервером
    // pos - prevPos = реальное перемещение за 1 тик = реальная velocity
    if (pearl.hasPrev) {
        glm::vec3 delta = pearl.pos - pearl.prevPos;
        float deltaLen = glm::length(delta);
        if (deltaLen > 0.01f && deltaLen < 5.0f) {
            return delta;
        }
    }

    // Источник 2 (FALLBACK): StateVector velocity
    // Для ЧУЖИХ энтити может отставать на 1-3 тика из-за сетевой задержки!
    // Используем ТОЛЬКО если дельты позиций ещё нет (первый тик)
    float svLen = glm::length(pearl.vel);
    if (svLen > 0.01f && svLen < 5.0f) {
        return pearl.vel;
    }

    // Нет данных — пропускаем эту пёрлу в этом тике
    return glm::vec3(0.f);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Trajectory simulation
//
//  Bedrock EnderPearl physics (правильный порядок каждый тик):
//    1. vel *= 0.99      (drag — ПЕРВЫМ)
//    2. vel.y -= 0.03    (gravity — ВТОРЫМ)
//    3. pos += vel       (движение — ТРЕТЬИМ)
//
//  КЛЮЧЕВОЙ ФИКС: НЕ делаем предварительный шаг velocity перед симуляцией.
//  Старый код делал simVel *= DRAG; simVel.y -= GRAVITY; ПЕРЕД вызовом
//  simulateTrajectory, что сдвигало всю траекторию на 1 тик и давало промах.
// ═════════════════════════════════════════════════════════════════════════════

std::vector<glm::vec3> PearlStopper::simulateTrajectory(glm::vec3 pos, glm::vec3 vel) {
    std::vector<glm::vec3> path;
    path.reserve(static_cast<size_t>(MAX_SIM_TICKS) + 1);

    glm::vec3 p = pos;
    glm::vec3 v = vel;
    path.push_back(p); // traj[0] = текущая позиция пёрлы

    for (int t = 0; t < MAX_SIM_TICKS; ++t) {
        // Improved simulation order (fixes inconsistent tracer when throwing high / looking up)
        // Move first with current velocity, then apply drag+gravity.
        // This gives more accurate upward arcs for high vertical throws.
        p += v;
        v *= DRAG;
        v.y -= GRAVITY;

        path.push_back(p);

        // Коллизия с блоком (пропускаем t=0 чтобы не зацепить блок под бросающим)
        if (t >= 1 && isSolidAt(p)) break;

        // Под воидом
        if (p.y < -64.f) break;

        // Пёрла остановилась (застряла)
        if (t > 10 && glm::length(v) < 0.001f) break;
    }

    return path;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Intercept finder — КЛЮЧЕВОЙ ФИКС
//
//  Старый код:
//    - Начинал с тика 4, пропускал всё до тика 6 (ticksNeeded = 1+3+2 = 6)
//    - Позиция ног: pearlPos.y - 0.9 + offset (без clamping)
//    - Пробовал Y-корректировку только ВВЕРХ (yTry = 0,1,2,3)
//    - Если не находил — пропускал (не пробовал другие оффсеты)
//
//  Новый код:
//    - Начинает с тика 2 (все пакеты TP улетают за 1 тик, + 1 на сервер)
//    - Позиция ног: pearlPos.y - 0.9 + offset, CLAMPED чтобы перла
//      гарантированно попадала в хитбокс [feetY, feetY+1.8]
//    - Пробует Y-корректировки И ВВЕРХ И ВНИЗ (±0.5, ±1.0)
//    - Берёт ПЕРВЫЙ валидный тик = самый ранний перехват (быстрее ловим)
// ═════════════════════════════════════════════════════════════════════════════

PearlStopper::InterceptResult PearlStopper::findBestIntercept(
    const std::vector<glm::vec3>& traj,
    glm::vec3 myPos)
{
    InterceptResult best;

    if (traj.size() < 3) return best;

    const size_t minTick = static_cast<size_t>(mInterceptTicks.mValue);

    for (size_t t = minTick; t < traj.size(); ++t) {
        const glm::vec3& pearlPos = traj[t];

        // ── Вычисляем позицию ног ────────────────────────────────────────
        // Хитбокс игрока: от feetPos.y до feetPos.y + 1.8
        // Перла должна быть ВНУТРИ: feetPos.y <= pearlPos.y <= feetPos.y + 1.8
        // Оптимум: feetPos.y = pearlPos.y - 0.9 (перла в центре хитбокса)
        glm::vec3 feetPos;
        feetPos.x = pearlPos.x;
        feetPos.z = pearlPos.z;
        feetPos.y = pearlPos.y - 0.9f + mYOffset.mValue;

        // Clamping: гарантируем что перла попадает в хитбокс
        // pearlPos.y - PEARL_PLAYER_H <= feetPos.y <= pearlPos.y
        feetPos.y = glm::clamp(feetPos.y, pearlPos.y - PEARL_PLAYER_H, pearlPos.y);

        // ── Проверяем свободное пространство ─────────────────────────────
        if (!isSpaceClear(feetPos)) {
            // Пробуем сдвиги по Y (вверх и вниз)
            bool found = false;
            for (float dy : {0.5f, -0.5f, 1.0f, -1.0f, 1.5f, -1.5f}) {
                glm::vec3 tryPos = feetPos + glm::vec3(0.f, dy, 0.f);
                // Проверяем что перла всё ещё попадёт в хитбокс
                float pearlRelY = pearlPos.y - tryPos.y;
                if (pearlRelY < 0.f || pearlRelY > PEARL_PLAYER_H) continue;
                if (isSpaceClear(tryPos)) {
                    feetPos = tryPos;
                    found = true;
                    break;
                }
            }
            if (!found) continue;
        }

        // ── Нашли валидную точку! ────────────────────────────────────────
        // Берём ПЕРВУЮ (самую раннюю) — ловим пёрлу максимально быстро
        best.valid    = true;
        best.feetPos  = feetPos;
        best.tick     = static_cast<int>(t);
        best.distance = glm::distance(myPos, pearlPos);
        return best;
    }

    return best;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Teleport — ClickTP-style step teleportation
// ═════════════════════════════════════════════════════════════════════════════

std::shared_ptr<MovePlayerPacket> PearlStopper::makePacket(glm::vec3 pos) {
    auto* p  = ClientInstance::get()->getLocalPlayer();
    auto  pk = MinecraftPackets::createPacket<MovePlayerPacket>();

    pk->mPos              = pos;
    pk->mPlayerID         = p->getRuntimeID();
    pk->mRot              = { mRots.x, mRots.y };
    pk->mYHeadRot         = mRots.z;
    pk->mResetPosition    = PositionMode::Teleport;
    pk->mOnGround         = false;  // Мы в воздухе (гравитация отключена)
    pk->mRidingID         = -1;
    pk->mCause            = TeleportationCause::Unknown;
    pk->mSourceEntityType = ActorType::Player;
    pk->mTick             = 0;

    return pk;
}

void PearlStopper::doStepTP(glm::vec3 from, glm::vec3 to, bool saveForRender) {
    auto* snd = ClientInstance::get()->getPacketSender();
    if (!snd) return;

    float step = mStepDistance.mValue;
    float totalDist = glm::distance(from, to);

    // Если расстояние меньше шага — один пакет
    if (totalDist < 0.01f) {
        snd->sendToServer(makePacket(to).get());
        return;
    }

    glm::vec3 dir = glm::normalize(to - from);
    glm::vec3 cur = from;
    std::vector<glm::vec3> pts;

    // Шагаем к цели
    while (glm::distance(cur, to) > step) {
        cur += dir * step;
        pts.push_back(cur);
        snd->sendToServer(makePacket(cur).get());
    }

    // Финальный пакет на точную позицию
    pts.push_back(to);
    snd->sendToServer(makePacket(to).get());

    if (saveForRender) {
        std::lock_guard<std::mutex> lk(mRenderMutex);
        mTpPath        = pts;
        mLastRenderTime = NOW;
    }
}

void PearlStopper::teleportTo(glm::vec3 dest) {
    auto* p = ClientInstance::get()->getLocalPlayer();
    if (!p) return;

    // Берём актуальную позицию из StateVector (надёжнее чем getPos)
    auto* sv = p->getStateVectorComponent();
    glm::vec3 from = sv ? sv->mPos : *p->getPos();

    doStepTP(from, dest, true);

    // Синхронизируем клиентскую позицию
    p->setPosition(dest);
    if (sv) {
        sv->mPos      = dest;
        sv->mPosOld   = dest;
        sv->mVelocity = glm::vec3(0.f);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Freeze / Unfreeze
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::freezeAt(Actor* player, glm::vec3 feetPos) {
    if (!player) return;

    // Сохраняем оригинальные флаги
    mSavedCollision = player->getStatusFlag(ActorFlags::HasCollision);
    mSavedGravity   = player->getStatusFlag(ActorFlags::HasGravity);
    mSavedPush      = player->getStatusFlag(ActorFlags::PushTowardsClosestSpace);

    // Отключаем физику
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);

    // Телепортируемся на точку перехвата
    teleportTo(feetPos);

    // Фиксируем позицию
    mInterceptPos = feetPos;
    mIsFrozen     = true;
    mFrozenTicks  = 0;
    mGoneTicks    = 0;
}

void PearlStopper::holdFreeze(Actor* player) {
    if (!player || !mIsFrozen) return;
    mFrozenTicks++;

    // Поддерживаем отключение физики каждый тик
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);

    // Держим позицию
    player->setPosition(mInterceptPos);
    if (auto* sv = player->getStateVectorComponent()) {
        sv->mPos      = mInterceptPos;
        sv->mPosOld   = mInterceptPos;
        sv->mVelocity = glm::vec3(0.f);
    }

    // Отправляем пакет позиции каждый тик для борьбы с rubberband
    auto* snd = ClientInstance::get()->getPacketSender();
    if (snd) snd->sendToServer(makePacket(mInterceptPos).get());
}

void PearlStopper::unfreeze(Actor* player) {
    if (!player || !mIsFrozen) return;
    player->setStatusFlag(ActorFlags::HasCollision, mSavedCollision);
    player->setStatusFlag(ActorFlags::HasGravity,   mSavedGravity);
    player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, mSavedPush);
    player->setFallDistance(0.f);
    mIsFrozen = false;
}

void PearlStopper::fullReset(Actor* player) {
    if (mIsFrozen && player) unfreeze(player);
    mIsFrozen      = false;
    mLockedPearlId = 0;
    mFrozenTicks   = 0;
    mGoneTicks     = 0;
    std::lock_guard<std::mutex> lk(mRenderMutex);
    mPredictedPath.clear();
    mTpPath.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main tick — ядро модуля
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onBaseTick(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    // Обновляем ротацию для пакетов
    if (auto* rot = player->getActorRotationComponent())
        mRots = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);

    // ═════════════════════════════════════════════════════════════════════
    //  FROZEN: держим позицию, ждём пока пёрла попадёт в нас
    // ═════════════════════════════════════════════════════════════════════
    if (mIsFrozen) {
        // Таймаут — слишком долго ждём
        if (mFrozenTicks > MAX_FROZEN_TICKS) {
            unfreeze(player);
            if (mTeleportBack.mValue) teleportTo(mOriginalPos);
            fullReset(player);
            if (mNotifications.mValue)
                NotifyUtils::notify("[PearlStopper] Timeout", 2.f, Notification::Type::Warning);
            return;
        }

        // Держим позицию
        holdFreeze(player);

        // Проверяем жива ли целевая пёрла
        Actor* pearlActor = ActorUtils::getActorFromRuntimeID(mLockedPearlId);
        bool pearlAlive = pearlActor != nullptr && isEnderPearl(pearlActor);

        if (!pearlAlive) {
            mGoneTicks++;
            if (mGoneTicks >= GONE_GRACE_TICKS) {
                // Пёрла исчезла = мы её поймали!
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
                fullReset(player);
            }
        } else {
            mGoneTicks = 0;
        }
        return;
    }

    // ═════════════════════════════════════════════════════════════════════
    //  SCANNING: ищем вражеские пёрлы для перехвата
    // ═════════════════════════════════════════════════════════════════════
    updateTrackedPearls(player);

    if (mTrackedPearls.empty()) return;

    glm::vec3 myPos = *player->getPos();

    // Ищем лучшую пёрлу для перехвата
    TrackedPearl*          bestPearl      = nullptr;
    InterceptResult        bestIntercept;
    std::vector<glm::vec3> bestTrajectory;

    for (auto& [id, pearl] : mTrackedPearls) {
        // Получаем надёжную velocity
        glm::vec3 vel = getReliableVelocity(pearl);
        if (glm::length(vel) < 0.005f) continue; // Нет данных — ждём следующий тик

        // Симулируем траекторию (без предварительного шага velocity!)
        auto traj = simulateTrajectory(pearl.pos, vel);

        // Ищем точку перехвата
        auto intercept = findBestIntercept(traj, myPos);
        if (!intercept.valid) continue;

        // Выбираем пёрлу с самым ранним перехватом (ловим БЫСТРЕЕ)
        if (!bestPearl || intercept.tick < bestIntercept.tick) {
            bestPearl      = &pearl;
            bestIntercept  = intercept;
            bestTrajectory = traj;
        }
    }

    if (!bestPearl) return;

    // Сохраняем траекторию для рендера
    {
        std::lock_guard<std::mutex> lk(mRenderMutex);
        mPredictedPath  = bestTrajectory;
        mLastRenderTime = NOW;
    }

    // ── ПЕРЕХВАТ! ────────────────────────────────────────────────────────
    mOriginalPos   = myPos;
    mLockedPearlId = bestPearl->runtimeId;

    if (mNotifications.mValue)
        NotifyUtils::notify(
            "[PearlStopper] Intercepting in ~" + std::to_string(bestIntercept.tick) + " ticks",
            1.8f, Notification::Type::Info);

    freezeAt(player, bestIntercept.feetPos);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Packet filter — отменяем rubber-band от сервера пока заморожены
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onPacketIn(PacketInEvent& event) {
    if (!mIsFrozen) return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt->mPlayerID != player->getRuntimeID()) return;

    // Отменяем серверный rubber-band
    event.cancel();

    // Переподтверждаем нашу позицию
    auto* snd = ClientInstance::get()->getPacketSender();
    if (snd) snd->sendToServer(makePacket(mInterceptPos).get());
}

// ═════════════════════════════════════════════════════════════════════════════
//  Render
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onRender(RenderEvent& event) {
    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    uint64_t now = NOW;

    // ── Жёлтая линия — предсказанная траектория пёрлы ────────────────────
    if (mDrawPrediction.mValue) {
        std::lock_guard<std::mutex> lk(mRenderMutex);

        float alpha = mIsFrozen ? 1.f : 0.f;
        if (!mIsFrozen && now < mLastRenderTime + 1500)
            alpha = std::clamp(1.f - float(now - mLastRenderTime) / 1500.f, 0.f, 1.f);

        if (alpha > 0.02f) {
            for (size_t i = 0; i + 1 < mPredictedPath.size(); ++i) {
                ImVec2 a, b;
                if (!RenderUtils::worldToScreen(mPredictedPath[i],     a)) continue;
                if (!RenderUtils::worldToScreen(mPredictedPath[i + 1], b)) continue;
                dl->AddLine(a, b, ImColor(1.0f, 0.85f, 0.0f, alpha * 0.75f), 2.0f);
            }
        }
    }

    // ── Синяя линия — наш путь TP ────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lk(mRenderMutex);

        float alpha = 0.f;
        if (mIsFrozen)
            alpha = 1.f;
        else if (now < mLastRenderTime + 1500)
            alpha = std::clamp(1.f - float(now - mLastRenderTime) / 1500.f, 0.f, 1.f);
        else
            mTpPath.clear();

        if (alpha > 0.02f && !mTpPath.empty()) {
            std::vector<ImVec2> pts;
            pts.reserve(mTpPath.size());
            for (auto& p : mTpPath) {
                ImVec2 s;
                if (RenderUtils::worldToScreen(p, s)) pts.push_back(s);
            }
            for (size_t i = 0; i + 1 < pts.size(); ++i)
                dl->AddLine(pts[i], pts[i + 1], ImColor(0.1f, 0.65f, 1.0f, alpha * 0.9f), 2.5f);
        }
    }

    // ── Оранжевый маркер — точка перехвата ────────────────────────────────
    if (mIsFrozen) {
        ImVec2 sp;
        if (RenderUtils::worldToScreen(mInterceptPos, sp)) {
            dl->AddCircleFilled(sp, 6.f,  ImColor(1.0f, 0.5f,  0.0f, 0.95f));
            dl->AddCircle      (sp, 12.f, ImColor(1.0f, 0.75f, 0.0f, 0.55f), 0, 2.0f);
        }
    }

    // ── Счётчик остановленных пёрл ───────────────────────────────────────
    if (mTotalStops > 0) {
        ImVec2 scr = ImGui::GetIO().DisplaySize;
        std::string txt = "Pearls stopped: " + std::to_string(mTotalStops);
        ImVec2 ts  = ImGui::CalcTextSize(txt.c_str());
        ImVec2 pos = { scr.x * 0.5f - ts.x * 0.5f, scr.y * 0.27f };
        dl->AddText({ pos.x + 1, pos.y + 1 }, ImColor(0, 0, 0, 180),             txt.c_str());
        dl->AddText(pos,                       ImColor(0.1f, 1.0f, 0.15f, 1.0f),  txt.c_str());
    }
}