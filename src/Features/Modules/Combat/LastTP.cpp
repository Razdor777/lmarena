#include "LastTP.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorType.hpp>
#include <SDK/Minecraft/Actor/Components/ActorHeadRotationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorRotationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorUniqueIDComponent.hpp>
#include <SDK/Minecraft/Actor/Components/CameraComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MobHurtTimeComponent.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/AnimatePacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp> // NOW

namespace
{
    constexpr float kPi = 3.14159265f;

    Actor* localPlayer()
    {
        auto* client = ClientInstance::get();
        return client ? client->getLocalPlayer() : nullptr;
    }

    int64_t uniqueIdOf(Actor* actor)
    {
        if (!actor) return -1;
        try
        {
            auto* component = actor->getActorUniqueIDComponent();
            return component ? static_cast<int64_t>(component->mUniqueID) : -1;
        }
        catch (...) { return -1; }
    }

    int64_t runtimeIdOf(Actor* actor)
    {
        if (!actor) return -1;
        try { return actor->getRuntimeID(); }
        catch (...) { return -1; }
    }

    // The attack transaction carries the actor's unique id (same convention as
    // ActorUtils::createAttackTransaction), but resolve both ways so a server or
    // client build that fills in a runtime id still works.
    Actor* actorFromPacketId(int64_t id)
    {
        if (id <= 0) return nullptr;

        Actor* actor = nullptr;
        try { actor = ActorUtils::getActorFromUniqueId(id); } catch (...) { actor = nullptr; }
        if (!actor)
        {
            try { actor = ActorUtils::getActorFromRuntimeID(id); } catch (...) { actor = nullptr; }
        }
        return actor;
    }

    // Only players that are alive, loaded and have health count as combat targets.
    bool isUsablePlayer(Actor* actor)
    {
        if (!actor) return false;
        try
        {
            if (!actor->isPlayer()) return false;
            if (!actor->isValid()) return false;
            if (actor->isDead()) return false;
            if (actor->getHealth() <= 0.f) return false;
        }
        catch (...) { return false; }
        return true;
    }

    float distanceBetween(Actor* a, Actor* b)
    {
        if (!a || !b) return FLT_MAX;
        try { return a->distanceTo(b); }
        catch (...) { return FLT_MAX; }
    }

    std::string displayNameOf(Actor* actor)
    {
        if (!actor) return "?";
        try { std::string name = actor->getRawName(); if (!name.empty()) return name; } catch (...) {}
        try { std::string name = actor->getNameTag(); if (!name.empty()) return name; } catch (...) {}
        return "?";
    }

    bool freshEnough(uint64_t now, uint64_t then)
    {
        return now >= then && (now - then) <= LastTP::kConfirmWindowMs;
    }

    // ==================== AUTOLOCK AIM MATH ====================

    // Точка прицеливания берётся ИСКЛЮЧИТЕЛЬНО из хитбокса самой цели. Здесь нет
    // ни поиска «ближайшего игрока», ни захвата по FOV, поэтому прицел физически
    // не может оказаться на ком-то другом, даже если цель облепили со всех сторон.
    bool aimPointOf(Actor* target, glm::vec3& out)
    {
        if (!target) return false;

        try
        {
            const AABB box = target->getAABB();
            out.x = (box.mMin.x + box.mMax.x) * 0.5f;
            out.z = (box.mMin.z + box.mMax.z) * 0.5f;
            // Верх корпуса: и по цели видно, что прицел на нём, и попадание
            // гарантировано (AABB — реальный хитбокс Minecraft).
            out.y = box.mMin.y + (box.mMax.y - box.mMin.y) * 0.80f;
            return true;
        }
        catch (...) {}

        try
        {
            out = *target->getPos();
            out.y += PLAYER_HEIGHT * 0.9f;
            return true;
        }
        catch (...) { return false; }
    }

    // {pitch, yaw} в MC-градусах (та же конвенция, что у Aimbot/Aura).
    bool aimAt(Actor* player, Actor* target, float& pitchDeg, float& yawDeg)
    {
        if (!player || !target) return false;

        glm::vec3 point;
        if (!aimPointOf(target, point)) return false;

        glm::vec3 eye;
        try { eye = *player->getPos(); }
        catch (...) { return false; }
        eye.y += PLAYER_HEIGHT;

        const glm::vec2 rots = MathUtils::getRots(eye, point);
        pitchDeg = rots.x;
        yawDeg   = rots.y;
        return true;
    }

    // Камера/прицел клиента живёт в CameraDirectLookComponent (радианы):
    //   mRotRads.x = PI - yawMC,   mRotRads.y = -pitchMC
    // (конверсия снята с рабочего Aimbot::calcTargetRotRads / Scaffold::mcToRad).
    void applyAimNow(Actor* player, float pitchDeg, float yawDeg)
    {
        if (!player) return;

        if (auto* direct = player->getCameraDirectLookComponent())
        {
            float yawRad = glm::radians(180.f - yawDeg);
            while (yawRad >  3.14159265f) yawRad -= 2.f * 3.14159265f;
            while (yawRad < -3.14159265f) yawRad += 2.f * 3.14159265f;

            direct->mRotRads.x = yawRad;
            direct->mRotRads.y = std::clamp(glm::radians(-pitchDeg), -1.55f, 1.55f);
        }

        if (auto* rot = player->getActorRotationComponent())
        {
            rot->mPitch    = pitchDeg;
            rot->mOldPitch = pitchDeg;
            rot->mYaw      = yawDeg;
            rot->mOldYaw   = yawDeg;
        }

        if (auto* head = player->getActorHeadRotationComponent())
        {
            head->mHeadRot    = yawDeg;
            head->mOldHeadRot = yawDeg;
        }
    }
}

// =========================================================
// TRACKER STARTUP — runs once, stays for the whole session
// =========================================================
void LastTP::onInit()
{
    if (!gFeatureManager || !gFeatureManager->mDispatcher) return;

    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &LastTP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent,  &LastTP::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,   &LastTP::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<EntityHurtEvent, &LastTP::onEntityHurtEvent>(this);

    spdlog::info("[LastTP] combat tracker started");
}

// =========================================================
// TICK — health polling (works even when the hurt hook is silent)
// =========================================================
void LastTP::onBaseTickEvent(BaseTickEvent& event)
{
    // Держим прицел на цели, к которой только что телепортировались (AutoLock).
    // BaseTickEvent прилетает на КАЖДОГО актора, поэтому считаем тики только
    // по своему — иначе удержание закончилось бы за один кадр.
    if (mLockTicks > 0)
    {
        Actor* self = localPlayer();
        if (self && (!event.mActor || event.mActor == self)) updateAutoLock();
    }

    Actor* player = event.mActor ? event.mActor : localPlayer();
    if (!player) return;

    const uint64_t now = static_cast<uint64_t>(NOW);
    pruneEntries(now);

    // ---- did I take damage? -> someone hit me ----
    float myHealth = -1.f;
    try { myHealth = player->getHealth(); } catch (...) { myHealth = -1.f; }

    const bool lostHealth = myHealth > 0.f && mLocalHealth > 0.f &&
                            myHealth < mLocalHealth - 0.01f;
    if (myHealth > 0.f) mLocalHealth = myHealth;

    int myHurtTime = 0;
    try { if (auto* hurt = player->getMobHurtTimeComponent()) myHurtTime = hurt->mHurtTime; }
    catch (...) { myHurtTime = 0; }

    // Only react to the 0 -> >0 transition, otherwise a long hurt animation
    // would keep re-recording and overwrite a newer event.
    const bool justGotHurt = mPrevLocalHurtTime <= 0 && myHurtTime > 0;
    mPrevLocalHurtTime = myHurtTime;

    if (lostHealth || justGotHurt)
        confirmTheirAttack(now);

    // ---- did another player lose health? -> I hit them ----
    std::vector<Actor*> players;
    try { players = ActorUtils::getActorList(true, true); }
    catch (...) { return; }

    std::unordered_map<int64_t, float> nextCache;
    nextCache.reserve(players.size());

    for (auto* actor : players)
    {
        if (!actor || actor == player) continue;

        const int64_t runtimeId = runtimeIdOf(actor);
        if (runtimeId == -1) continue;

        float health = -1.f;
        try { health = actor->getHealth(); }
        catch (...) { continue; }

        int hurtTime = 0;
        try { if (auto* hurt = actor->getMobHurtTimeComponent()) hurtTime = hurt->mHurtTime; }
        catch (...) { hurtTime = 0; }

        auto it = mHealthCache.find(runtimeId);
        const bool victimLostHealth = it != mHealthCache.end() &&
                                      it->second > 0.f && health > 0.f &&
                                      health < it->second - 0.01f;

        nextCache[runtimeId] = health;

        // The hurt animation is the cleanest "this player really took damage"
        // signal the client receives for other players — combined with one of my
        // own pending attacks it proves the hit landed (gamemode players make
        // neither signal, so they are naturally ignored).
        if (victimLostHealth || hurtTime > 0)
            confirmMyAttack(actor, now);
    }

    // The cache only ever holds players the client currently sees, so a player
    // that unloads and comes back does not produce a phantom "damage" event.
    mHealthCache.swap(nextCache);
}

// =========================================================
// OUTGOING — I attacked somebody
// =========================================================
void LastTP::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::InventoryTransaction) return;

    auto* packet = event.getPacket<InventoryTransactionPacket>();
    if (!packet || !packet->mTransaction) return;
    if (packet->mTransaction->getTransacType() !=
        ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction)
        return;

    auto* transaction =
        reinterpret_cast<ItemUseOnActorInventoryTransaction*>(packet->mTransaction.get());
    if (!transaction) return;
    if (transaction->mActionType != ItemUseOnActorInventoryTransaction::ActionType::Attack)
        return;

    Actor* victim = actorFromPacketId(static_cast<int64_t>(transaction->mActorId));
    if (!isUsablePlayer(victim)) return;

    PendingAttack entry;
    entry.uniqueID  = uniqueIdOf(victim);
    entry.runtimeID = runtimeIdOf(victim);
    entry.time      = static_cast<uint64_t>(NOW);

    const int64_t entryUnique  = entry.uniqueID;
    const int64_t entryRuntime = entry.runtimeID;

    std::erase_if(mPendingAttacks, [&](const PendingAttack& pending)
    {
        return (entryUnique != -1 && pending.uniqueID == entryUnique) ||
               (entryRuntime != -1 && pending.runtimeID == entryRuntime);
    });

    mPendingAttacks.push_back(entry);
    if (mPendingAttacks.size() > kMaxEntries)
        mPendingAttacks.erase(mPendingAttacks.begin());
}

// =========================================================
// INCOMING — somebody swung next to me
// =========================================================
void LastTP::onPacketInEvent(PacketInEvent& event)
{
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::Animate) return;

    // PacketInEvent stores the packet in a shared_ptr, so this returns one too.
    auto packet = event.getPacket<AnimatePacket>();
    if (!packet || packet->mAction != Action::Swing) return;

    Actor* player = localPlayer();
    if (!player) return;
    if (packet->mRuntimeID == runtimeIdOf(player)) return; // my own swing

    Actor* swinger = nullptr;
    try { swinger = ActorUtils::getActorFromRuntimeID(packet->mRuntimeID); }
    catch (...) { return; }
    if (!isUsablePlayer(swinger)) return;

    // Far away swingers cannot have hit me.
    if (distanceBetween(swinger, player) > kSwingMaxDistance) return;

    Swinger entry;
    entry.runtimeID = packet->mRuntimeID;
    entry.time      = static_cast<uint64_t>(NOW);

    const int64_t entryRuntime = entry.runtimeID;
    std::erase_if(mRecentSwings, [&](const Swinger& swing)
    {
        return swing.runtimeID == entryRuntime;
    });

    mRecentSwings.push_back(entry);
    if (mRecentSwings.size() > kMaxEntries)
        mRecentSwings.erase(mRecentSwings.begin());
}

// =========================================================
// HURT — hook based confirmation (secondary path)
// =========================================================
void LastTP::onEntityHurtEvent(EntityHurtEvent& event)
{
    if (!event.mEntity) return;

    Actor* player = localPlayer();
    if (!player) return;

    const uint64_t now = static_cast<uint64_t>(NOW);

    if (event.mEntity == player)
    {
        confirmTheirAttack(now);
        return;
    }

    confirmMyAttack(event.mEntity, now);
}

// =========================================================
// CONFIRMATION
// =========================================================
void LastTP::confirmMyAttack(Actor* victim, uint64_t now)
{
    if (!isUsablePlayer(victim)) return;

    const int64_t victimUnique  = uniqueIdOf(victim);
    const int64_t victimRuntime = runtimeIdOf(victim);

    for (auto it = mPendingAttacks.begin(); it != mPendingAttacks.end(); ++it)
    {
        const bool matches =
            (it->uniqueID  != -1 && it->uniqueID  == victimUnique) ||
            (it->runtimeID != -1 && it->runtimeID == victimRuntime);
        if (!matches) continue;

        // The event is dated by the moment I *sent* the attack, not by the
        // moment the damage confirmation reaches me — otherwise a slow
        // confirmation would masquerade as the newest action.
        const uint64_t attackTime = it->time;
        const bool confirmed = freshEnough(now, attackTime);
        mPendingAttacks.erase(it);

        if (confirmed) recordCombatEvent(victim, false, attackTime);
        return;
    }
}

void LastTP::confirmTheirAttack(uint64_t now)
{
    Actor* player = localPlayer();
    if (!player) return;

    Actor*    best     = nullptr;
    uint64_t  bestTime = 0;
    float     bestDist = FLT_MAX;

    for (auto& swing : mRecentSwings)
    {
        if (now < swing.time || now - swing.time > kSwingWindowMs) continue;

        Actor* attacker = nullptr;
        try { attacker = ActorUtils::getActorFromRuntimeID(swing.runtimeID); }
        catch (...) { continue; }
        if (!isUsablePlayer(attacker)) continue;

        const float dist = distanceBetween(attacker, player);
        if (dist > kSwingMaxDistance) continue;

        // Newest swing wins, closest one breaks ties.
        if (swing.time > bestTime || (swing.time == bestTime && dist < bestDist))
        {
            best     = attacker;
            bestTime = swing.time;
            bestDist = dist;
        }
    }

    // Fallback for clients/servers where swing packets never reach us: assume
    // the closest player in melee reach, same idea as InfiniteAura's "Hurt By".
    // When swings ARE tracked, the candidate must have swung recently, so a
    // teammate standing next to us is not blamed for fall / fire damage.
    if (!best)
    {
        std::vector<Actor*> players;
        try { players = ActorUtils::getActorList(true, true); }
        catch (...) { players.clear(); }

        const bool haveSwingData = !mRecentSwings.empty();
        float closest = kFallbackDistance;

        for (auto* actor : players)
        {
            if (!actor || actor == player || !isUsablePlayer(actor)) continue;

            const float dist = distanceBetween(actor, player);
            if (dist >= closest) continue;

            if (haveSwingData)
            {
                const int64_t candidateId = runtimeIdOf(actor);
                bool swungRecently = false;

                for (const auto& swing : mRecentSwings)
                {
                    if (swing.runtimeID != candidateId) continue;
                    if (now >= swing.time && now - swing.time <= kLooseSwingWindowMs)
                        swungRecently = true;
                    break;
                }

                if (!swungRecently) continue;
            }

            closest = dist;
            best    = actor;
        }

        // With no swing packets to date the action by, the moment my health
        // dropped is the best estimate available.
        if (best) bestTime = now;
    }

    if (best) recordCombatEvent(best, true, bestTime);
}

void LastTP::recordCombatEvent(Actor* target, bool iWasHit, uint64_t actionTime)
{
    if (!isUsablePlayer(target)) return;

    // Only the most recent ACTION wins. Both tracked actions meet here — "I hit
    // Player1" (iWasHit = false) and "Player1 hit me" (iWasHit = true) — and the
    // one that happened later decides who the module teleports to.
    //
    // The comparison uses the action timestamp, never the confirmation time:
    // confirmations arrive out of order (server latency, damage ticks), so
    // dating by arrival would let an older action overwrite a newer one just
    // because its proof landed a frame later.
    if (mTargetEventTime != 0 && actionTime < mTargetEventTime) return;

    mTargetRuntimeID = runtimeIdOf(target);
    mTargetUniqueID  = uniqueIdOf(target);
    mTargetEventTime = actionTime;
    mTargetName      = displayNameOf(target);
    mTargetWasMe     = iWasHit;
}

void LastTP::pruneEntries(uint64_t now)
{
    std::erase_if(mPendingAttacks, [&](const PendingAttack& pending)
    {
        return now < pending.time || now - pending.time > kKeepPendingMs;
    });

    std::erase_if(mRecentSwings, [&](const Swinger& swing)
    {
        return now < swing.time || now - swing.time > kKeepPendingMs;
    });
}

// =========================================================
// TELEPORT
// =========================================================
Actor* LastTP::resolveTarget()
{
    Actor* player = localPlayer();
    if (!player) return nullptr;
    if (mTargetRuntimeID == -1 && mTargetUniqueID == -1) return nullptr;

    Actor* target = nullptr;

    if (mTargetRuntimeID != -1)
    {
        try { target = ActorUtils::getActorFromRuntimeID(mTargetRuntimeID); }
        catch (...) { target = nullptr; }
    }

    if (!target && mTargetUniqueID != -1)
    {
        try { target = ActorUtils::getActorFromUniqueId(mTargetUniqueID); }
        catch (...) { target = nullptr; }
    }

    // "The cheat cannot see him" -> just don't teleport.
    if (!target || target == player) return nullptr;
    if (!isUsablePlayer(target)) return nullptr;

    // Runtime ids can change, refresh the cached identity.
    mTargetRuntimeID = runtimeIdOf(target);
    mTargetUniqueID  = uniqueIdOf(target);
    mTargetName      = displayNameOf(target);
    return target;
}

void LastTP::attemptTeleport()
{
    Actor* player = localPlayer();
    if (!player) return;

    Actor* target = resolveTarget();
    if (!target) return;

    glm::vec3 targetPos;
    try { targetPos = *target->getPos(); }
    catch (...) { return; }

    // Where the target is facing (his head, not his body).
    float headYaw = 0.f;
    try
    {
        if (auto* head = target->getActorHeadRotationComponent())
            headYaw = head->mHeadRot;
        else if (auto* rot = target->getActorRotationComponent())
            headYaw = rot->mYaw;
    }
    catch (...) {}

    const float yawRad = headYaw * (kPi / 180.f);

    // Minecraft yaw: 0 = +Z. "right" is his own right hand, matching the
    // Left/Right naming users expect from InfiniteAura's KB directions.
    const glm::vec3 forward = { -std::sin(yawRad), 0.f,  std::cos(yawRad) };
    const glm::vec3 right   = { -std::cos(yawRad), 0.f, -std::sin(yawRad) };

    const float distance = std::clamp(mDistance.mValue, 0.1f, 10.f);
    glm::vec3 destination = targetPos;

    switch (mDirection.as<Direction>())
    {
        case Direction::Behind: destination = targetPos - forward * distance; break;
        case Direction::Front:  destination = targetPos + forward * distance; break;
        case Direction::Left:   destination = targetPos - right   * distance; break;
        case Direction::Right:  destination = targetPos + right   * distance; break;
        case Direction::Top:    destination = targetPos + glm::vec3(0.f, distance, 0.f); break;
    }

    auto* sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float pitch = 0.f, yaw = 0.f, headRot = 0.f;
    try
    {
        if (auto* rot = player->getActorRotationComponent())
        {
            pitch = rot->mPitch;
            yaw   = rot->mYaw;
        }
        if (auto* head = player->getActorHeadRotationComponent())
            headRot = head->mHeadRot;
        else
            headRot = yaw;
    }
    catch (...)
    {}

    // ── AutoLock: считаем углы ДО отправки пакетов ────────────────────────────
    // Пакеты телепорта несут уже наведённый yaw/pitch, поэтому сервер видит
    // прилёт с прицелом на цель, а не с прежними углами.
    bool lockAim = mAutoLock.mValue;
    if (lockAim)
    {
        float aimPitch = 0.f, aimYaw = 0.f;
        if (aimAt(player, target, aimPitch, aimYaw))
        {
            pitch   = aimPitch;
            yaw     = aimYaw;
            headRot = aimYaw;
        }
        else
        {
            lockAim = false;
        }
    }

    const int64_t playerId = runtimeIdOf(player);

    auto makePacket = [&](const glm::vec3& pos)
    {
        auto packet = MinecraftPackets::createPacket<MovePlayerPacket>();
        packet->mPos              = pos;
        packet->mPlayerID         = playerId;
        packet->mRot              = { pitch, yaw };
        packet->mYHeadRot         = headRot;
        packet->mResetPosition    = PositionMode::Teleport;
        packet->mOnGround         = true;
        packet->mRidingID         = -1;
        packet->mCause            = TeleportationCause::Unknown;
        packet->mSourceEntityType = ActorType::Player;
        packet->mTick             = 0;
        return packet;
    };

    glm::vec3 from;
    try { from = *player->getPos(); }
    catch (...) { return; }

    const glm::vec3 delta = destination - from;
    const float totalDistance = glm::length(delta);

    if (totalDistance < 0.01f)
    {
        sender->sendToServer(makePacket(destination).get());
    }
    else
    {
        const glm::vec3 direction = glm::normalize(delta);
        glm::vec3 current = from;

        int guard = 0;
        while (glm::distance(current, destination) > kStepDistance && guard++ < 512)
        {
            current += direction * kStepDistance;
            sender->sendToServer(makePacket(current).get());
        }
        sender->sendToServer(makePacket(destination).get());
    }

    try
    {
        player->setPosition(destination);
        if (auto* state = player->getStateVectorComponent())
            state->mVelocity = glm::vec3(0.f);
    }
    catch (...)
    {}

    // ── AutoLock: сразу доворачиваем камеру и запоминаем цель ────────────────
    if (lockAim)
    {
        applyAimNow(player, pitch, yaw);

        mLockRuntimeID = runtimeIdOf(target);
        mLockUniqueID  = uniqueIdOf(target);
        mLockTicks     = kAutoLockTicks;
    }
}

// =========================================================
// AUTOLOCK — удержание прицела на той же самой цели
// =========================================================
Actor* LastTP::resolveLockTarget()
{
    Actor* player = localPlayer();
    if (!player) return nullptr;

    Actor* target = nullptr;

    if (mLockRuntimeID != -1)
    {
        try { target = ActorUtils::getActorFromRuntimeID(mLockRuntimeID); }
        catch (...) { target = nullptr; }
    }

    // Уникальный id — вторая попытка: runtime id мог смениться после телепорта.
    if (!target && mLockUniqueID != -1)
    {
        try { target = ActorUtils::getActorFromUniqueId(mLockUniqueID); }
        catch (...) { target = nullptr; }
    }

    if (!target || target == player || !isUsablePlayer(target)) return nullptr;

    // Главная страховка: подтверждаем ЛИЧНОСТЬ цели. Движок умеет переиспользовать
    // runtime id под другого игрока — без этой проверки прицел мог бы уехать на него.
    if (mLockUniqueID != -1 && uniqueIdOf(target) != mLockUniqueID) return nullptr;

    mLockRuntimeID = runtimeIdOf(target);
    return target;
}

void LastTP::updateAutoLock()
{
    if (mLockTicks <= 0) return;

    --mLockTicks;

    Actor* player = localPlayer();
    Actor* target = resolveLockTarget();

    float aimPitch = 0.f, aimYaw = 0.f;
    if (!player || !target || !aimAt(player, target, aimPitch, aimYaw))
    {
        mLockTicks = 0;
        mLockRuntimeID = -1;
        mLockUniqueID  = -1;
        return;
    }

    applyAimNow(player, aimPitch, aimYaw);

    if (mLockTicks <= 0)
    {
        mLockRuntimeID = -1;
        mLockUniqueID  = -1;
    }
}

// =========================================================
// ENABLE — one shot: teleport, then turn itself back off
// =========================================================
void LastTP::onEnable()
{
    attemptTeleport();
    setEnabled(false);
}
