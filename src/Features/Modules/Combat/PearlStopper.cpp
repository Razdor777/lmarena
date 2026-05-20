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

// Constants tuned for 1.21.44 Bedrock
constexpr float PEARL_GRAVITY      = 0.03f;
constexpr float PEARL_DRAG         = 0.99f;
constexpr float TICK_TIME          = 0.05f;
constexpr int   MAX_FREEZE_TICKS   = 280;
constexpr int   PREDICT_TICKS      = 160;
constexpr int   REACTION_TICKS     = 3;
constexpr int   PING_TICKS         = 2;
constexpr float MIN_INTERCEPT_DIST = 2.5f;
constexpr float MAX_INTERCEPT_DIST = 90.f;
constexpr int   MISSED_TOLERANCE   = 4;

// ═════════════════════════════════════════════════════════════════════════════
//  Enable / Disable
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onEnable() {
    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);

    mTotalStops = 0;
    mPearlLastPos.clear();
    mPearlLastTime.clear();
    mPredictedPath.clear();
    mTeleportPath.clear();

    if (player) {
        if (auto* rot = player->getActorRotationComponent()) {
            mCurrentRotation = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);
        }
    }

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &PearlStopper::onRenderEvent>(this);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Enabled - intercepting enemy pearls", 2.5f, Notification::Type::Info);
}

void PearlStopper::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PearlStopper::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &PearlStopper::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &PearlStopper::onRenderEvent>(this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    resetState(player);

    if (mNotifications.mValue)
        NotifyUtils::notify("[PearlStopper] Disabled", 2.f, Notification::Type::Info);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Block collision (improved)
// ═════════════════════════════════════════════════════════════════════════════

bool PearlStopper::isSolidBlock(glm::vec3 pos) {
    auto* bs = ClientInstance::get()->getBlockSource();
    if (!bs) return false;

    glm::ivec3 bp = glm::ivec3(glm::floor(pos));
    Block* block = bs->getBlock(bp);
    if (!block || !block->mLegacy) return false;

    int id = block->mLegacy->getBlockId();
    if (id == 0 || (id >= 8 && id <= 11)) return false; // air or liquid
    return true;
}

glm::vec3 PearlStopper::ensureNotInBlock(glm::vec3 pos) {
    int safety = 0;
    while (isSolidBlock(pos) && safety++ < 12) pos.y += 0.6f;
    while (isSolidBlock(pos + glm::vec3(0, 1.8f, 0)) && safety++ < 12) pos.y += 0.6f;
    return pos;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Pearl detection (only enemy pearls)
// ═════════════════════════════════════════════════════════════════════════════

bool PearlStopper::isEnderPearl(Actor* actor) {
    if (!actor) return false;
    auto* tc = actor->getActorTypeComponent();
    return tc && (static_cast<uint32_t>(tc->mType) & 0xFFu) == 87u; // 87 = EnderPearl
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

std::vector<PearlStopper::PearlData> PearlStopper::findEnemyPearls(Actor* local) {
    std::vector<PearlData> result;
    if (!local) return result;

    auto actors = ActorUtils::getActorList(false, false);
    for (Actor* a : actors) {
        if (!a || !isEnderPearl(a)) continue;
        if (isOwnPearl(a, local)) continue;

        glm::vec3 pos = a->getPos() ? *a->getPos() : glm::vec3(0.f);
        glm::vec3 vel = a->getStateVectorComponent() ? a->getStateVectorComponent()->mVelocity : glm::vec3(0.f);

        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) continue;

        PearlData d;
        d.actor = a;
        d.runtimeId = a->getRuntimeID();
        d.position = pos;
        d.velocity = vel;
        d.throwerPos = pos;
        if (auto* owner = getPearlOwner(a)) {
            if (auto* op = owner->getPos()) d.throwerPos = *op;
        }
        result.push_back(d);
    }
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Reliable velocity (smoothed from last ticks)
// ═════════════════════════════════════════════════════════════════════════════

glm::vec3 PearlStopper::getReliableVelocity(const PearlData& pearl, uint64_t now) {
    auto itPos  = mPearlLastPos.find(pearl.runtimeId);
    auto itTime = mPearlLastTime.find(pearl.runtimeId);

    glm::vec3 vel = pearl.velocity;

    if (itPos != mPearlLastPos.end() && itTime != mPearlLastTime.end()) {
        float dt = (now - itTime->second) / 1000.0f;
        if (dt > 0.04f && dt < 0.25f) {
            glm::vec3 delta = pearl.position - itPos->second;
            float ticks = dt / TICK_TIME;
            if (ticks > 0.8f) {
                glm::vec3 empirical = delta / ticks;
                float lenE = glm::length(empirical);
                float lenV = glm::length(vel);
                if (lenE > 0.3f && lenE < 3.5f) {
                    vel = empirical * 0.65f + vel * 0.35f;
                }
            }
        }
    }

    mPearlLastPos[pearl.runtimeId] = pearl.position;
    mPearlLastTime[pearl.runtimeId] = now;

    float len = glm::length(vel);
    if (len < 0.2f || len > 4.0f) {
        glm::vec3 dir = pearl.position - pearl.throwerPos;
        float d = glm::length(dir);
        if (d > 0.6f) vel = glm::normalize(dir) * 1.45f;
        else vel = glm::vec3(0, 0.4f, 1.6f);
    }
    return vel;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Improved trajectory simulation (with grace period)
// ═════════════════════════════════════════════════════════════════════════════

std::vector<glm::vec3> PearlStopper::simulateTrajectory(glm::vec3 start, glm::vec3 vel, int maxTicks, bool checkCollision) {
    std::vector<glm::vec3> path;
    path.reserve(maxTicks + 1);

    glm::vec3 p = start;
    glm::vec3 v = vel;
    path.push_back(p);

    for (int t = 0; t < maxTicks; ++t) {
        v.y -= PEARL_GRAVITY;
        v *= PEARL_DRAG;
        p += v;
        path.push_back(p);

        if (checkCollision && t > 5 && isSolidBlock(p)) break;
        if (p.y < -64.f) break;
    }
    return path;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Improved intercept (feet position + hitbox air check)
// ═════════════════════════════════════════════════════════════════════════════

PearlStopper::InterceptResult PearlStopper::findBestIntercept(const std::vector<glm::vec3>& traj,
    glm::vec3 playerPos, glm::vec3 throwerPos, float step) {

    InterceptResult res;
    if (traj.size() < 6) return res;

    for (size_t t = 5; t < traj.size(); ++t) {
        const glm::vec3& pearl = traj[t];

        if (glm::distance(pearl, throwerPos) < 3.0f) continue;

        glm::vec3 feet = pearl - glm::vec3(0.f, mYOffset.mValue, 0.f);

        // Check that our hitbox (feet, body, head) is in air
        if (isSolidBlock(feet) || isSolidBlock(feet + glm::vec3(0,1.0f,0)) || isSolidBlock(feet + glm::vec3(0,1.8f,0)))
            continue;

        float dist = glm::distance(playerPos, feet);
        int packets = static_cast<int>(std::ceil(dist / step));
        int ourTicks = packets + mReactionTicks.mValue + mPingTicks.mValue;

        if (static_cast<int>(t) < ourTicks + 3) continue; // too early

        res.found = true;
        res.position = feet;
        res.tickIndex = static_cast<int>(t);
        res.distance = dist;
        res.timeNeeded = ourTicks * TICK_TIME;
        return res;
    }
    return res;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Reliable straight-line teleport (smaller step, rotation preserved)
// ═════════════════════════════════════════════════════════════════════════════

std::shared_ptr<MovePlayerPacket> PearlStopper::createTeleportPacket(glm::vec3 pos, glm::vec3 rot) {
    auto pk = MinecraftPackets::createPacket<MovePlayerPacket>();
    pk->mPos = pos;
    pk->mPlayerID = ClientInstance::get()->getLocalPlayer()->getRuntimeID();
    pk->mRot = { rot.x, rot.y };
    pk->mYHeadRot = rot.z;
    pk->mResetPosition = PositionMode::Teleport;
    pk->mOnGround = true;
    pk->mRidingID = -1;
    pk->mCause = TeleportationCause::Unknown;
    pk->mSourceEntityType = ActorType::Player;
    pk->mTick = 0;
    return pk;
}

void PearlStopper::straightLineTeleport(glm::vec3 from, glm::vec3 to, bool savePath) {
    if (glm::length(to - from) < 0.1f) {
        auto pk = createTeleportPacket(to, mCurrentRotation);
        ClientInstance::get()->getPacketSender()->sendToServer(pk.get());
        return;
    }

    glm::vec3 dir = glm::normalize(to - from);
    glm::vec3 cur = from;
    std::vector<glm::vec3> path;

    float step = mStepDistance.mValue;
    auto sender = ClientInstance::get()->getPacketSender();

    while (glm::distance(cur, to) > step * 0.6f) {
        cur += dir * step;
        path.push_back(cur);
        sender->sendToServer(createTeleportPacket(cur, mCurrentRotation).get());
    }

    path.push_back(to);
    sender->sendToServer(createTeleportPacket(to, mCurrentRotation).get());

    if (savePath) {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mTeleportPath = std::move(path);
        mLastRenderTime = NOW;
    }
}

void PearlStopper::teleportTo(glm::vec3 dest, bool savePath) {
    auto* p = ClientInstance::get()->getLocalPlayer();
    if (!p) return;
    if (!std::isfinite(dest.x)) return;

    straightLineTeleport(*p->getPos(), dest, savePath);
    p->setPosition(dest);

    if (auto* sv = p->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);
}

// ═════════════════════════════════════════════════════════════════════════════
//  Freeze system (improved)
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::freezePlayer(Actor* p) {
    if (!p) return;

    mSavedCollision = p->getStatusFlag(ActorFlags::HasCollision);
    mSavedGravity   = p->getStatusFlag(ActorFlags::HasGravity);
    mSavedPush      = p->getStatusFlag(ActorFlags::PushTowardsClosestSpace);

    p->setStatusFlag(ActorFlags::HasCollision, false);
    p->setStatusFlag(ActorFlags::HasGravity, false);
    p->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    p->setOnGround(false);
    p->setFallDistance(0.f);

    if (auto* sv = p->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);

    mIsFrozen = true;
    mFrozenTicks = 0;
    mMissedTicks = 0;
    mInterceptPosition = *p->getPos(); // fallback
}

void PearlStopper::maintainFreeze(Actor* p) {
    if (!p || !mIsFrozen) return;
    mFrozenTicks++;

    p->setStatusFlag(ActorFlags::HasCollision, false);
    p->setStatusFlag(ActorFlags::HasGravity, false);
    p->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    p->setOnGround(false);
    p->setFallDistance(0.f);
    p->setPosition(mInterceptPosition);

    if (auto* sv = p->getStateVectorComponent())
        sv->mVelocity = glm::vec3(0.f);

    auto sender = ClientInstance::get()->getPacketSender();
    if (sender) sender->sendToServer(createTeleportPacket(mInterceptPosition, mCurrentRotation).get());
}

void PearlStopper::unfreezePlayer(Actor* p) {
    if (!p || !mIsFrozen) return;

    p->setStatusFlag(ActorFlags::HasCollision, mSavedCollision);
    p->setStatusFlag(ActorFlags::HasGravity, mSavedGravity);
    p->setStatusFlag(ActorFlags::PushTowardsClosestSpace, mSavedPush);
    p->setFallDistance(0.f);

    mIsFrozen = false;
}

void PearlStopper::resetState(Actor* p) {
    if (mIsFrozen && p) unfreezePlayer(p);
    mIsActive = false;
    mIsFrozen = false;
    mTargetPearlId = 0;
    mFrozenTicks = 0;
    mMissedTicks = 0;
    mPredictedPath.clear();
    mTeleportPath.clear();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Main tick logic (clean & precise)
// ═════════════════════════════════════════════════════════════════════════════

void PearlStopper::onBaseTickEvent(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    if (auto* rot = player->getActorRotationComponent())
        mCurrentRotation = glm::vec3(rot->mPitch, rot->mYaw, rot->mYaw);

    if (mIsFrozen) {
        if (mFrozenTicks > MAX_FREEZE_TICKS) {
            unfreezePlayer(player);
            if (mTeleportBack.mValue) teleportTo(mOriginalPosition, false);
            resetState(player);
            if (mNotifications.mValue) NotifyUtils::notify("[PearlStopper] Timeout", 2.f, Notification::Type::Warning);
            return;
        }

        maintainFreeze(player);

        if (!isPearlAlive(mTargetPearlId, player)) {
            mMissedTicks++;
            if (mMissedTicks > MISSED_TOLERANCE) {
                mTotalStops++;
                unfreezePlayer(player);
                if (mTeleportBack.mValue) {
                    teleportTo(mOriginalPosition, false);
                    if (mNotifications.mValue) NotifyUtils::notify("[PearlStopper] Blocked & returned", 2.f, Notification::Type::Info);
                } else if (mNotifications.mValue) {
                    NotifyUtils::notify("[PearlStopper] Blocked", 2.f, Notification::Type::Info);
                }
                resetState(player);
            }
        } else mMissedTicks = 0;
        return;
    }

    auto pearls = findEnemyPearls(player);
    if (pearls.empty()) return;

    uint64_t now = NOW;
    for (auto& p : pearls) getReliableVelocity(p, now); // update velocity cache

    PearlData* best = nullptr;
    float bestScore = 999999.f;

    for (auto& pearl : pearls) {
        glm::vec3 vel = getReliableVelocity(pearl, now);
        if (glm::length(vel) < 0.2f) continue;

        float dist = glm::distance(pearl.position, *player->getPos());
        if (dist > 85.f) continue;

        glm::vec3 toPlayer = glm::normalize(*player->getPos() - pearl.position);
        float approach = glm::dot(glm::normalize(vel), toPlayer);
        float score = dist * (2.8f - approach * 2.2f);

        if (score < bestScore) {
            bestScore = score;
            best = &pearl;
        }
    }

    if (!best) return;

    auto trajectory = simulateTrajectory(best->position, getReliableVelocity(*best, now), 160, true);

    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        mPredictedPath = trajectory;
    }

    auto intercept = findBestIntercept(trajectory, *player->getPos(), best->throwerPos, mStepDistance.mValue);
    if (!intercept.found) return;

    mOriginalPosition = *player->getPos();
    mInterceptPosition = intercept.position;
    mTargetPearlId = best->runtimeId;
    mIsActive = true;
    mMissedTicks = 0;

    if (mNotifications.mValue) {
        int dist = static_cast<int>(intercept.distance);
        NotifyUtils::notify("[PearlStopper] Intercepting at " + std::to_string(dist) + " blocks", 1.8f, Notification::Type::Info);
    }

    freezePlayer(player);
    teleportTo(intercept.position, true);
}

void PearlStopper::onPacketInEvent(PacketInEvent& event) {
    if (!mIsFrozen) return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto* p = ClientInstance::get()->getLocalPlayer();
    if (!p || event.getPacket<MovePlayerPacket>()->mPlayerID != p->getRuntimeID()) return;

    event.cancel();

    auto sender = ClientInstance::get()->getPacketSender();
    if (sender) sender->sendToServer(createTeleportPacket(mInterceptPosition, mCurrentRotation).get());
}

void PearlStopper::onRenderEvent(RenderEvent& event) {
    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    float alpha = 0.f;
    uint64_t now = NOW;
    {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        if (mIsFrozen) alpha = 1.f;
        else if (now < mLastRenderTime + 1800) {
            alpha = 1.f - (now - mLastRenderTime) / 1800.f;
            alpha = std::max(0.f, alpha);
        }
    }

    if (mDrawPrediction.mValue) {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        for (size_t i = 0; i + 1 < mPredictedPath.size(); ++i) {
            ImVec2 a, b;
            if (!RenderUtils::worldToScreen(mPredictedPath[i], a)) continue;
            if (!RenderUtils::worldToScreen(mPredictedPath[i+1], b)) continue;
            dl->AddLine(a, b, ImColor(1.0f, 0.9f, 0.0f, alpha * 0.65f), 2.2f);
        }
    }

    if (alpha > 0.02f) {
        std::lock_guard<std::mutex> lock(mRenderMutex);
        for (size_t i = 0; i + 1 < mTeleportPath.size(); ++i) {
            ImVec2 a, b;
            if (!RenderUtils::worldToScreen(mTeleportPath[i], a)) continue;
            if (!RenderUtils::worldToScreen(mTeleportPath[i+1], b)) continue;
            ImColor c(0.1f, 0.7f, 1.0f, alpha * 0.85f);
            dl->AddLine(a, b, c, 3.0f);
        }
    }

    if (mIsFrozen) {
        ImVec2 sp;
        if (RenderUtils::worldToScreen(mInterceptPosition, sp)) {
            dl->AddCircleFilled(sp, 7.f, ImColor(1.0f, 0.55f, 0.0f, 0.95f));
            dl->AddCircle(sp, 13.f, ImColor(1.0f, 0.7f, 0.0f, 0.5f), 0, 2.5f);
        }
    }

    if (mTotalStops > 0) {
        ImVec2 scr = ImGui::GetIO().DisplaySize;
        std::string txt = "Blocked: " + std::to_string(mTotalStops);
        ImVec2 ts = ImGui::CalcTextSize(txt.c_str());
        ImVec2 p(scr.x * 0.5f - ts.x * 0.5f, scr.y * 0.28f);
        dl->AddText({p.x+2, p.y+2}, ImColor(0,0,0,0.7f), txt.c_str());
        dl->AddText(p, ImColor(0.1f, 1.0f, 0.2f, 0.95f), txt.c_str());
    }
}
