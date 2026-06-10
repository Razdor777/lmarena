//
// Rewritten Aura — RMB lock-on, humanized rotations, no auto-switch after kill
//

#include "Aura.hpp"

#include <Windows.h>
#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/BobHurtEvent.hpp>
#include <Features/Events/BoneRenderEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <Features/Modules/Movement/TargetStrafe.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Actor/Components/RuntimeIDComponent.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/Packets/RemoveActorPacket.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

// ==================== LIFECYCLE ====================

void Aura::onEnable() {
    gFeatureManager->mDispatcher->listen<MouseEvent,       &Aura::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,    &Aura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent,   &Aura::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,    &Aura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,      &Aura::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BobHurtEvent,     &Aura::onBobHurtEvent,
        nes::event_priority::FIRST>(this);
    gFeatureManager->mDispatcher->listen<BoneRenderEvent,  &Aura::onBoneRenderEvent,
        nes::event_priority::FIRST>(this);

    resetState();
}

void Aura::onDisable() {
    gFeatureManager->mDispatcher->deafen<MouseEvent,       &Aura::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,    &Aura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent,   &Aura::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,    &Aura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,      &Aura::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BobHurtEvent,     &Aura::onBobHurtEvent>(this);
    gFeatureManager->mDispatcher->deafen<BoneRenderEvent,  &Aura::onBoneRenderEvent>(this);

    resetState();
}

void Aura::resetTarget() {
    mTargetRuntimeID  = 0;
    mRotating         = false;
    mLastAttack       = 0;
    mLastSlot         = 0;
    sHasTarget        = false;
    sTargetRuntimeID  = 0;
    sTarget           = nullptr;
    sTrailHistory.clear();
    sLastTrailUpdate  = 0;
    mCurrentAimOffset = glm::vec3(0.f);
    mCurrentRotations = glm::vec2(0.f);
    mTargetLockTime   = 0;
}

void Aura::resetState() {
    resetTarget();
    mLmbHeld      = false;
    mRmbClicked   = false;
    mAutoTargeted = false;
}

// ==================== VALIDATION ====================

bool Aura::isValidTarget(Actor* actor, Actor* player) const {
    if (!actor || !player) return false;
    if (actor == player)   return false;

    try {
        if (actor->isDead())           return false;
        if (actor->getHealth() <= 0.f) return false;
    } catch (...) {
        return false;
    }

    try {
        if (actor->isPlayer() && gFriendManager) {
            if (gFriendManager->isFriend(actor) && !mFistFriends.mValue)
                return false;
        }
    } catch (...) {
        return false;
    }

    if (!mThroughWalls.mValue) {
        try {
            if (!player->canSee(actor)) return false;
        } catch (...) {
            return false;
        }
    }

    return true;
}

bool Aura::isInAimRange(Actor* actor, Actor* player) const {
    if (!actor || !player) return false;
    try {
        return actor->distanceTo(player) <= mAimRange.mValue;
    } catch (...) {
        return false;
    }
}

bool Aura::isInAttackRange(Actor* actor, Actor* player) const {
    if (!actor || !player) return false;
    try {
        return actor->distanceTo(player) <= mAttackRange.mValue;
    } catch (...) {
        return false;
    }
}

// ==================== TARGET SELECTION ====================

Actor* Aura::resolveTarget(Actor* player) {
    if (mTargetRuntimeID == 0) return nullptr;

    Actor* actor = ActorUtils::getActorFromRuntimeID(mTargetRuntimeID);
    if (!actor) {
        mTargetRuntimeID = 0;
        return nullptr;
    }

    try {
        if (actor->isDead() || actor->getHealth() <= 0.f) {
            mTargetRuntimeID = 0;
            return nullptr;
        }
    } catch (...) {
        mTargetRuntimeID = 0;
        return nullptr;
    }

    if (!isInAimRange(actor, player)) {
        mTargetRuntimeID = 0;
        return nullptr;
    }

    if (!isValidTarget(actor, player)) {
        mTargetRuntimeID = 0;
        return nullptr;
    }

    return actor;
}

Actor* Aura::findBestTarget(Actor* player) {
    if (!player) return nullptr;

    auto actors = ActorUtils::getActorList(false, true);

    Actor* best     = nullptr;
    float  bestDist = FLT_MAX;

    for (auto* actor : actors) {
        if (!actor)                        continue;
        if (!isValidTarget(actor, player)) continue;
        if (!isInAimRange(actor, player))  continue;

        float dist = 0.f;
        try {
            dist = actor->distanceTo(player);
        } catch (...) {
            continue;
        }

        if (dist < bestDist) {
            bestDist = dist;
            best     = actor;
        }
    }

    return best;
}

// ==================== BYPASS ====================

Actor* Aura::findObstructor(Actor* player, Actor* target) {
    if (!player || !target)                     return target;
    if (mBypassMode.mValue == BypassMode::None) return target;

    glm::vec3 rayStart, rayEnd;
    try {
        auto* pPos = player->getPos();
        auto* tPos = target->getPos();
        if (!pPos || !tPos) return target;
        rayStart = *pPos;
        rayEnd   = *tPos;
    } catch (...) {
        return target;
    }

    auto actors = ActorUtils::getActorList(false, false);

    Actor* closestObstacle = nullptr;
    float  minDist         = FLT_MAX;

    for (auto* actor : actors) {
        if (!actor)                             continue;
        if (actor == player || actor == target) continue;

        try {
            if (actor->isDead() || actor->getHealth() <= 0.f) continue;
        } catch (...) {
            continue;
        }

        if (!isInAttackRange(actor, player)) continue;

        auto* shape = actor->getAABBShapeComponent();
        if (!shape) continue;

        try {
            if (!MathUtils::rayIntersectsAABB(rayStart, rayEnd, shape->mMin, shape->mMax))
                continue;
        } catch (...) {
            continue;
        }

        glm::vec3 pos;
        try {
            auto* aPos = actor->getPos();
            if (!aPos) continue;
            pos = *aPos;
        } catch (...) {
            continue;
        }

        float dist = glm::distance(rayStart, pos);
        if (dist < minDist) {
            minDist         = dist;
            closestObstacle = actor;
        }
    }

    return closestObstacle ? closestObstacle : target;
}

// ==================== WEAPON ====================

int Aura::getBestWeapon(Actor* target) {
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player || !target) return 0;

    auto* supplies = player->getSupplies();
    if (!supplies) return 0;

    try {
        if (gFriendManager && gFriendManager->isFriend(target) && mFistFriends.mValue) {
            auto* container = supplies->getContainer();
            if (container) {
                int limit = mHotbarOnly.mValue ? 9 : 36;
                for (int i = 0; i < limit; i++) {
                    auto* item = container->getItem(i);
                    if (!item) continue;
                    auto* itemBase = item->getItem();
                    if (!itemBase || itemBase->mItemId == 0)
                        return i;
                }
            }
            return supplies->mSelectedSlot;
        }
    } catch (...) {}

    return ItemUtils::getBestItem(SItemType::Sword, mHotbarOnly.mValue);
}

// ==================== ROTATION ====================

glm::vec2 Aura::calcRotations(Actor* player) {
    if (!player) return {0.f, 0.f};

    glm::vec3 eyePos;
    try {
        auto* pPos = player->getPos();
        if (!pPos) return {0.f, 0.f};
        eyePos = *pPos + glm::vec3(0.f, PLAYER_HEIGHT, 0.f);
    } catch (...) {
        return {0.f, 0.f};
    }

    glm::vec3 targetPoint;
    try {
        Actor* target = ActorUtils::getActorFromRuntimeID(mTargetRuntimeID);
        if (!target) return {0.f, 0.f};

        auto* tPos = target->getPos();
        if (!tPos) return {0.f, 0.f};
        targetPoint = *tPos;

        float height = 1.8f;
        if (auto* shape = target->getAABBShapeComponent())
            height = shape->mHeight;

        targetPoint.y += height * mAimY.mValue;
        targetPoint   += mCurrentAimOffset;
    } catch (...) {
        return {0.f, 0.f};
    }

    glm::vec3 delta = targetPoint - eyePos;
    float horizontalDist = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    if (!std::isfinite(horizontalDist) || horizontalDist < 0.001f) return {0.f, 0.f};

    float pitch = -std::atan2(delta.y, horizontalDist) * (180.f / glm::pi<float>());
    float yaw   =  std::atan2(delta.z, delta.x)        * (180.f / glm::pi<float>()) - 90.f;

    if (!std::isfinite(pitch) || !std::isfinite(yaw))
        return {0.f, 0.f};

    pitch = std::clamp(pitch, -90.f, 90.f);

    return {pitch, yaw};
}

// ==================== ESP METHODS ====================

void Aura::drawWireframe3D(Actor* target, Actor* player, ImColor color, float alpha) {
    if (!target || !player) return;

    auto* shape = target->getAABBShapeComponent();
    if (!shape) return;

    auto* renderPos = target->getRenderPositionComponent();
    if (!renderPos) return;

    glm::vec3 center = renderPos->mPosition;
    if (target->isPlayer()) center.y -= PLAYER_HEIGHT;

    float w = shape->mWidth;
    float h = shape->mHeight;

    glm::vec3 pts[8] = {
        center + glm::vec3(-w/2, 0, -w/2),
        center + glm::vec3( w/2, 0, -w/2),
        center + glm::vec3( w/2, 0,  w/2),
        center + glm::vec3(-w/2, 0,  w/2),
        center + glm::vec3(-w/2, h, -w/2),
        center + glm::vec3( w/2, h, -w/2),
        center + glm::vec3( w/2, h,  w/2),
        center + glm::vec3(-w/2, h,  w/2),
    };

    ImVec2 screenPts[8];
    bool valid = true;
    for (int i = 0; i < 8; i++) {
        if (!RenderUtils::worldToScreen(pts[i], screenPts[i])) {
            valid = false;
            break;
        }
    }
    if (!valid) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(
        (int)(color.Value.x * 255),
        (int)(color.Value.y * 255),
        (int)(color.Value.z * 255),
        (int)(alpha * 255)
    );

    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };

    for (auto& e : edges)
        dl->AddLine(screenPts[e[0]], screenPts[e[1]], col, 2.f);
}

void Aura::drawTrail(Actor* target, ImColor color) {
    if (!target) return;

    auto* renderPos = target->getRenderPositionComponent();
    if (!renderPos) return;

    int64_t now = NOW;

    if (now - sLastTrailUpdate > 50) {
        sLastTrailUpdate = now;
        sTrailHistory.push_back({renderPos->mPosition, now, 1.f});
    }

    int64_t maxAge = static_cast<int64_t>(mTrailLength.mValue * 1000.f);
    sTrailHistory.erase(
        std::remove_if(sTrailHistory.begin(), sTrailHistory.end(),
            [&](const TrailPoint& p) { return now - p.timestamp > maxAge; }),
        sTrailHistory.end()
    );

    if (sTrailHistory.size() < 2) return;

    auto* dl = ImGui::GetBackgroundDrawList();

    for (size_t i = 1; i < sTrailHistory.size(); i++) {
        ImVec2 s1, s2;
        if (!RenderUtils::worldToScreen(sTrailHistory[i-1].pos, s1)) continue;
        if (!RenderUtils::worldToScreen(sTrailHistory[i].pos,   s2)) continue;

        float age1 = static_cast<float>(now - sTrailHistory[i-1].timestamp) / maxAge;
        float a1   = (1.f - age1) * mESPAlpha.mValue;
        if (a1 <= 0) continue;

        ImU32 c1 = IM_COL32(
            (int)(color.Value.x * 255),
            (int)(color.Value.y * 255),
            (int)(color.Value.z * 255),
            (int)(a1 * 255)
        );

        float thick = 3.f * (1.f - age1);
        dl->AddLine(s1, s2, c1, thick);

        if (i % 3 == 0)
            dl->AddCircleFilled(s1, thick * 1.5f, c1, 8);
    }
}

void Aura::drawChams(Actor* target, ImColor color, float alpha) {
    if (!target) return;

    auto* shape = target->getAABBShapeComponent();
    if (!shape) return;

    auto* renderPos = target->getRenderPositionComponent();
    if (!renderPos) return;

    glm::vec3 pos = renderPos->mPosition;
    if (target->isPlayer()) pos.y -= PLAYER_HEIGHT;

    float w = shape->mWidth;
    float h = shape->mHeight;

    AABB aabb;
    aabb.mMin = pos - glm::vec3(w/2, 0, w/2);
    aabb.mMax = pos + glm::vec3(w/2, h, w/2);

    auto pts = MathUtils::getImBoxPoints(aabb);
    if (pts.size() < 4) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    ImU32 fillCol = IM_COL32(
        (int)(color.Value.x * 255),
        (int)(color.Value.y * 255),
        (int)(color.Value.z * 255),
        (int)(alpha * 80)
    );

    dl->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()), fillCol);
}

// ==================== MOUSE EVENT ====================

void Aura::onMouseEvent(MouseEvent& event) {
    if (event.mActionButtonId == 1) {
        bool wasHeld = mLmbHeld;
        mLmbHeld = (event.mButtonData != 0);
        if (mLmbHeld && !wasHeld)
            mAutoTargeted = false;
        return;
    }

    if (event.mActionButtonId == 2 && event.mButtonData != 0) {
        if (!mLmbHeld) return;
        mRmbClicked = true;
    }
}

// ==================== MAIN TICK ====================

void Aura::onBaseTickEvent(BaseTickEvent& event) {
    auto* player = event.mActor;
    if (!player) return;

    auto targetStrafe = gFeatureManager->mModuleManager->getModule<TargetStrafe>();
    if (targetStrafe) {
        if (mTargetStrafe.mValue  && !targetStrafe->mEnabled) targetStrafe->setEnabled(true);
        if (!mTargetStrafe.mValue &&  targetStrafe->mEnabled) targetStrafe->setEnabled(false);
    }

    try {
        if (player->getHealth() <= 0.f) return;
    } catch (...) { return; }

    if (!mLmbHeld) {
        resetState();
        return;
    }

    auto* supplies = player->getSupplies();
    if (!supplies) return;

    if (mRmbClicked) {
        mRmbClicked = false;

        auto actors = ActorUtils::getActorList(false, true);
        Actor* newTarget = nullptr;

        glm::vec3 eyePos, lookDir;
        bool hasLook = false;

        try {
            auto* pPos = player->getPos();
            if (pPos) {
                eyePos = *pPos + glm::vec3(0.f, PLAYER_HEIGHT, 0.f);

                float yaw = 0.f, pitch = 0.f;
                if (auto* head = player->getActorHeadRotationComponent()) yaw = head->mHeadRot;
                else if (auto* body = player->getMobBodyRotationComponent()) yaw = body->yBodyRot;
                if (auto* rot = player->getActorRotationComponent()) pitch = rot->mPitch;

                float yawRad   = glm::radians(yaw + 90.f);
                float pitchRad = glm::radians(-pitch);
                lookDir = glm::vec3(
                    cosf(pitchRad) * cosf(yawRad),
                    sinf(pitchRad),
                    cosf(pitchRad) * sinf(yawRad)
                );
                hasLook = true;
            }
        } catch (...) {}

        if (hasLook) {
            Actor* aimed     = nullptr;
            float  bestAngle = FLT_MAX;
            constexpr float MAX_ANGLE = 12.5f;

            for (auto* actor : actors) {
                if (!actor) continue;
                if (!isValidTarget(actor, player)) continue;

                glm::vec3 targetCenter;
                try {
                    auto* pos   = actor->getPos();
                    auto* shape = actor->getAABBShapeComponent();
                    if (!pos || !shape) continue;
                    targetCenter = *pos + glm::vec3(0.f, shape->mHeight * 0.5f, 0.f);
                } catch (...) { continue; }

                glm::vec3 toTarget = targetCenter - eyePos;
                float dist = glm::length(toTarget);
                if (dist < 0.001f) continue;
                toTarget /= dist;

                float dot   = MathUtils::clamp(glm::dot(lookDir, toTarget), -1.f, 1.f);
                float angle = glm::degrees(acosf(dot));

                if (angle < MAX_ANGLE && angle < bestAngle) {
                    bestAngle = angle;
                    aimed     = actor;
                }
            }
            newTarget = aimed;
        }

        if (!newTarget)
            newTarget = findBestTarget(player);

        if (newTarget) {
            try {
                resetTarget();

                mTargetRuntimeID = newTarget->getRuntimeID();
                sTargetRuntimeID = mTargetRuntimeID;
                sHasTarget       = true;
                sTarget          = newTarget;
                mAutoTargeted    = true;
                mTargetLockTime  = NOW;

                auto* shape = newTarget->getAABBShapeComponent();
                if (shape && mAimRandom.mValue > 0.f) {
                    float rx = MathUtils::randomFloat(-1.f, 1.f) * (shape->mWidth  * 0.5f) * mAimRandom.mValue;
                    float rz = MathUtils::randomFloat(-1.f, 1.f) * (shape->mWidth  * 0.5f) * mAimRandom.mValue;
                    float ry = MathUtils::randomFloat(-0.5f,0.5f) * shape->mHeight          * mAimRandom.mValue;
                    mCurrentAimOffset = glm::vec3(rx, ry, rz);
                } else {
                    mCurrentAimOffset = glm::vec3(0.f);
                }

                float curYaw = 0.f, curPitch = 0.f;
                if (auto* head = player->getActorHeadRotationComponent()) curYaw   = head->mHeadRot;
                if (auto* rot  = player->getActorRotationComponent())     curPitch = rot->mPitch;
                mCurrentRotations = glm::vec2(curPitch, curYaw);

            } catch (...) {}
        }
    }

    Actor* target = resolveTarget(player);

    if (!target) {
        if (mTargetRuntimeID != 0) {
            resetTarget();
            return;
        }

        if (!mAutoTargeted) {
            target = findBestTarget(player);
            if (target) {
                try {
                    mTargetRuntimeID = target->getRuntimeID();
                    sTargetRuntimeID = mTargetRuntimeID;
                    sHasTarget       = true;
                    sTarget          = target;
                    mAutoTargeted    = true;
                    mTargetLockTime  = NOW;

                    float curYaw = 0.f, curPitch = 0.f;
                    if (auto* head = player->getActorHeadRotationComponent()) curYaw   = head->mHeadRot;
                    if (auto* rot  = player->getActorRotationComponent())     curPitch = rot->mPitch;
                    mCurrentRotations = glm::vec2(curPitch, curYaw);
                } catch (...) {
                    resetTarget();
                    return;
                }
            } else {
                return;
            }
        } else {
            return;
        }
    }

    sTargetRuntimeID = mTargetRuntimeID;
    sHasTarget       = true;
    sTarget          = target;

    if (NOW - mTargetLockTime < static_cast<int64_t>(mReactionDelay.mValue))
        return;

    mRotating = (mRotateMode.mValue != RotateMode::None);

    // --- ATTACK ---
    if (isInAttackRange(target, player)) {
        int64_t now = NOW;
        float aps = mRandomizeAPS.mValue
            ? MathUtils::randomFloat(mAPSMin.mValue, mAPSMax.mValue)
            : mAPS.mValue;
        if (aps < 1.f) aps = 1.f;
        int64_t delay = static_cast<int64_t>(1000.f / aps);

        if (now - mLastAttack >= delay) {
            Actor* attackTarget = findObstructor(player, target);
            if (!attackTarget) attackTarget = target;

            if (mSwing.mValue) {
                try { player->swing(); } catch (...) {}
            }

            int bestSlot = getBestWeapon(target);

            if (mSwitchMode.mValue == SwitchMode::Full)
                supplies->mSelectedSlot = bestSlot;

            if (mAttackMode.mValue == AttackMode::Synched) {
                int spoofSlot = (mSwitchMode.mValue == SwitchMode::Spoof) ? bestSlot : -1;
                auto pkt = ActorUtils::createAttackTransaction(attackTarget, spoofSlot);
                if (pkt) PacketUtils::queueSend(pkt, false);
            } else {
                int oldSlot = supplies->mSelectedSlot;
                if (mSwitchMode.mValue == SwitchMode::Spoof)
                    supplies->mSelectedSlot = bestSlot;
                try {
                    auto* gameMode = player->getGameMode();
                    if (gameMode) gameMode->attack(attackTarget);
                } catch (...) {}
                supplies->mSelectedSlot = oldSlot;
            }
            mLastAttack = now;
        }
    }

    // --- ROTATION ---
    if (mRotating) {
        glm::vec2 targetRots = calcRotations(player);
        if (std::isfinite(targetRots.x) && std::isfinite(targetRots.y)) {
            if (mRotSpeed.mValue > 0.f) {
                float dt      = 0.05f;
                float maxStep = mRotSpeed.mValue * dt;

                float deltaYaw = targetRots.y - mCurrentRotations.y;
                while (deltaYaw >  180.f) deltaYaw -= 360.f;
                while (deltaYaw < -180.f) deltaYaw += 360.f;

                float deltaPitch = targetRots.x - mCurrentRotations.x;

                mCurrentRotations.y += MathUtils::clamp(deltaYaw,   -maxStep, maxStep);
                mCurrentRotations.x += MathUtils::clamp(deltaPitch, -maxStep, maxStep);

                while (mCurrentRotations.y >  180.f) mCurrentRotations.y -= 360.f;
                while (mCurrentRotations.y < -180.f) mCurrentRotations.y += 360.f;
                mCurrentRotations.x = MathUtils::clamp(mCurrentRotations.x, -90.f, 90.f);
            } else {
                mCurrentRotations = targetRots;
            }

            try {
                if (auto* headRot = player->getActorHeadRotationComponent()) {
                    headRot->mHeadRot    = mCurrentRotations.y;
                    headRot->mOldHeadRot = mCurrentRotations.y;
                }
                if (auto* bodyRot = player->getMobBodyRotationComponent()) {
                    bodyRot->yBodyRot    = mCurrentRotations.y;
                    bodyRot->yOldBodyRot = mCurrentRotations.y;
                }
            } catch (...) {}
        }
    }
}

// ==================== PACKET OUT ====================

void Aura::onPacketOutEvent(PacketOutEvent& event) {
    if (!mRotating)          return;
    if (mTargetRuntimeID == 0) return;
    if (NOW - mTargetLockTime < static_cast<int64_t>(mReactionDelay.mValue)) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player)        return;
    if (!event.mPacket) return;

    const PacketID id = event.mPacket->getId();

    if (id == PacketID::PlayerAuthInput || id == PacketID::MovePlayer) {
        glm::vec2 rots = mCurrentRotations;

        if (!std::isfinite(rots.x) || !std::isfinite(rots.y)) return;
        if (rots.x == 0.f && rots.y == 0.f) {
            rots = calcRotations(player);
            if (rots.x == 0.f && rots.y == 0.f) return;
        }

        if (id == PacketID::PlayerAuthInput) {
            auto* pkt = event.getPacket<PlayerAuthInputPacket>();
            if (!pkt) return;
            pkt->mRot      = rots;
            pkt->mYHeadRot = rots.y;
        } else {
            auto* pkt = event.getPacket<MovePlayerPacket>();
            if (!pkt) return;
            pkt->mRot      = rots;
            pkt->mYHeadRot = rots.y;
        }

        if (mRotateMode.mValue == RotateMode::Flick)
            mRotating = false;

    } else if (id == PacketID::MobEquipment) {
        auto* pkt = event.getPacket<MobEquipmentPacket>();
        if (!pkt) return;
        mLastSlot = pkt->mSlot;
    }
}

// ==================== PACKET IN ====================

void Aura::onPacketInEvent(PacketInEvent& event) {
    if (!event.mPacket) return;

    if (event.mPacket->getId() == PacketID::RemoveActor) {
        auto packet = event.getPacket<RemoveActorPacket>();
        if (packet && packet->mRuntimeID == mTargetRuntimeID)
            resetTarget();
        return;
    }

    if (event.mPacket->getId() == PacketID::ChangeDimension) {
        resetState();
        if (mAutoDisable.mValue)
            this->setEnabled(false);
    }
}

// ==================== RENDER ====================

void Aura::onRenderEvent(RenderEvent& event) {
    if (!mShowESP.mValue)      return;
    if (!sHasTarget)           return;
    if (sTargetRuntimeID == 0) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    Actor* resolvedTarget = ActorUtils::getActorFromRuntimeID(sTargetRuntimeID);
    if (!resolvedTarget) {
        sHasTarget       = false;
        sTargetRuntimeID = 0;
        sTrailHistory.clear();
        return;
    }

    try {
        if (resolvedTarget->isDead() || resolvedTarget->getHealth() <= 0.f) {
            sHasTarget       = false;
            sTargetRuntimeID = 0;
            sTrailHistory.clear();
            return;
        }
    } catch (...) {
        sHasTarget       = false;
        sTargetRuntimeID = 0;
        sTrailHistory.clear();
        return;
    }

    ImColor theme = ColorUtils::getThemedColor(0);
    float   alpha = mESPAlpha.mValue;

    switch (mESPStyle.mValue) {
        case ESPStyle::Wireframe3D: drawWireframe3D(resolvedTarget, player, theme, alpha); break;
        case ESPStyle::Chams:       drawChams(resolvedTarget, theme, alpha);               break;
        case ESPStyle::Trail:       drawTrail(resolvedTarget, theme);                      break;
        default: break;
    }

    std::string name;
    float dist = 0.f;
    try {
        name = resolvedTarget->getNameTag();
        dist = resolvedTarget->distanceTo(player);
    } catch (...) {
        return;
    }

    {
        std::string cleaned;
        cleaned.reserve(name.size());
        for (size_t i = 0; i < name.size(); ) {
            unsigned char c0 = static_cast<unsigned char>(name[i]);
            if (c0 == 0xC2 && i + 1 < name.size() &&
                static_cast<unsigned char>(name[i + 1]) == 0xA7 &&
                i + 2 < name.size())
            {
                i += 3;
                continue;
            }
            cleaned += name[i++];
        }
        name = std::move(cleaned);
    }

    if (name.empty()) return;

    auto* renderPos = resolvedTarget->getRenderPositionComponent();
    if (!renderPos) return;

    glm::vec3 labelPos = renderPos->mPosition;
    labelPos.y += 2.2f;

    ImVec2 screen;
    if (!RenderUtils::worldToScreen(labelPos, screen)) return;

    std::string label = fmt::format("{} [{:.1f}m]", name, dist);
    ImVec2 ts = ImGui::CalcTextSize(label.c_str());

    auto* dl = ImGui::GetBackgroundDrawList();
    ImU32 shadow  = IM_COL32(0, 0, 0, (int)(alpha * 200));
    ImU32 textCol = IM_COL32(
        (int)(theme.Value.x * 255),
        (int)(theme.Value.y * 255),
        (int)(theme.Value.z * 255),
        (int)(alpha * 255)
    );

    dl->AddText({screen.x - ts.x/2 + 1, screen.y - ts.y/2 + 1}, shadow,   label.c_str());
    dl->AddText({screen.x - ts.x/2,     screen.y - ts.y/2},      textCol,  label.c_str());
}

// ==================== BOB / BONE ====================

void Aura::onBobHurtEvent(BobHurtEvent& event) {
    if (sHasTarget) event.mDoBlockAnimation = true;
}

void Aura::onBoneRenderEvent(BoneRenderEvent& event) {
    if (sHasTarget) event.mDoBlockAnimation = true;
}