//
// Rewritten Aura — RMB lock-on, humanized rotations, no auto-switch after kill
//

#include "Aura.hpp"

#include <Windows.h>
#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
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
#include <SDK/Minecraft/Network/Packets/ActorEventPacket.hpp>
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

    // Совместимость со старым конфигом: раньше скорость доводки по умолчанию
    // была 55°/с, и поворот на 180° занимал больше трёх секунд — прицел
    // буквально «не поворачивался». Если в конфиге осталось ровно старое
    // значение, поднимаем его до нового (240°/с) один раз за запуск.
    static bool sRotSpeedMigrated = false;
    if (!sRotSpeedMigrated)
    {
        sRotSpeedMigrated = true;
        if (std::fabs(mRotSpeed.mValue - 55.f) < 0.01f)
            mRotSpeed.mValue = 240.f;
    }

    resetState();
}

void Aura::onDisable() {
    gFeatureManager->mDispatcher->deafen<MouseEvent,       &Aura::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,    &Aura::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent,   &Aura::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,    &Aura::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,      &Aura::onRenderEvent>(this);

    resetState();
}

void Aura::resetTarget() {
    mTargetRuntimeID  = 0;
    mRotating         = false;
    mLastAttack       = 0;
    mTiming           = HitTiming{};
    mLastSlot         = 0;
    sHasTarget        = false;
    sTargetRuntimeID  = 0;
    sTrailHistory.clear();
    sLastTrailUpdate  = 0;
    mCurrentAimOffset = glm::vec3(0.f);
    mCurrentRotations = glm::vec2(0.f);
    mRotStepAt = 0;
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
    auto* ci = ClientInstance::get();
    if (!ci || !target) return 0;
    auto* player = ci->getLocalPlayer();
    if (!player) return 0;

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

namespace {
constexpr glm::vec2 kNoRotation{ std::numeric_limits<float>::quiet_NaN(),
                                 std::numeric_limits<float>::quiet_NaN() };
}

glm::vec2 Aura::calcRotations(Actor* player) {
    if (!player) return kNoRotation;

    glm::vec3 eyePos;
    try {
        auto* pPos = player->getPos();
        if (!pPos) return kNoRotation;
        eyePos = *pPos + glm::vec3(0.f, PLAYER_HEIGHT, 0.f);
    } catch (...) {
        return kNoRotation;
    }

    glm::vec3 targetPoint;
    try {
        Actor* target = ActorUtils::getActorFromRuntimeID(mTargetRuntimeID);
        if (!target) return kNoRotation;

        auto* tPos = target->getPos();
        if (!tPos) return kNoRotation;
        targetPoint = *tPos;

        float height = 1.8f;
        if (auto* shape = target->getAABBShapeComponent())
            height = shape->mHeight;

        targetPoint.y += height * mAimY.mValue;
        targetPoint   += mCurrentAimOffset;
    } catch (...) {
        return kNoRotation;
    }

    glm::vec3 delta = targetPoint - eyePos;
    float horizontalDist = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    if (!std::isfinite(horizontalDist) || horizontalDist < 0.001f) return kNoRotation;

    float pitch = -std::atan2(delta.y, horizontalDist) * (180.f / glm::pi<float>());
    // atan2 даёт -180..180, а после «-90» получалось -270..90: yaw выходил за
    // диапазон пакета и на части углов доводка шла длинным путём.
    float yaw   = MathUtils::wrap(
        std::atan2(delta.z, delta.x) * (180.f / glm::pi<float>()) - 90.f, -180.f, 180.f);

    if (!std::isfinite(pitch) || !std::isfinite(yaw))
        return kNoRotation;

    pitch = std::clamp(pitch, -90.f, 90.f);

    return {pitch, yaw};
}

// ==================== ESP METHODS ====================

void Aura::drawWireframe3D(Actor* target, Actor* player, ImColor color, float alpha) {
    if (!target || !player) return;

    // Бокс берём у самого актора: getAABB() уже переводит позицию в «ноги» для
    // игроков, подставляет интерполированную позицию локального игрока и даёт
    // ровно то же, что ESP. Раньше здесь считалось вручную от mPosition, из-за
    // чего свой бокс отставал от чужого на кадр и дрожал при быстрой камере.
    AABB box = target->getAABB();
    glm::vec3 mn = box.mMin;
    glm::vec3 mx = box.mMax;

    // NaN/Inf из физики (телепорт, смена мира) — worldToScreen вернёт мусор,
    // отсекаем до проекции.
    if (!std::isfinite(mn.x) || !std::isfinite(mn.y) || !std::isfinite(mn.z) ||
        !std::isfinite(mx.x) || !std::isfinite(mx.y) || !std::isfinite(mx.z))
        return;

    glm::vec3 pts[8] = {
        { mn.x, mn.y, mn.z },
        { mx.x, mn.y, mn.z },
        { mx.x, mn.y, mx.z },
        { mn.x, mn.y, mx.z },
        { mn.x, mx.y, mn.z },
        { mx.x, mx.y, mn.z },
        { mx.x, mx.y, mx.z },
        { mn.x, mx.y, mx.z },
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

    AABB aabb = target->getAABB();

    // NaN/Inf из физики (телепорт, смена мира) роняет AddConvexPolyFilled в
    // ассерте — проверяем до проекции.
    if (!std::isfinite(aabb.mMin.x) || !std::isfinite(aabb.mMin.y) || !std::isfinite(aabb.mMin.z) ||
        !std::isfinite(aabb.mMax.x) || !std::isfinite(aabb.mMax.y) || !std::isfinite(aabb.mMax.z))
        return;

    // getImBoxPoints возвращает {} , если хотя бы один угол за камерой —
    // частичный полигон заливать нельзя.
    auto pts = MathUtils::getImBoxPoints(aabb);
    if (pts.size() < 3) return;

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
    if (!ClientInstance::get() || ClientInstance::get()->getLocalPlayer() != player) return;

    auto targetStrafe = gFeatureManager->mModuleManager
        ? gFeatureManager->mModuleManager->getModule<TargetStrafe>() : nullptr;
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
    // Без уровня/реестра дальше идти нельзя: всё остальное ходит по актёрам.
    if (!player->getLevel()) return;

    // Всё остальное — за одним try/catch: любое исключение из SDK (битый
    // компонент, нулевой контейнер) не должно ронять игру целиком.
    try {
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

                mCurrentRotations = cameraRotations(player);
                mRotStepAt = static_cast<uint64_t>(NOW);

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
                    mAutoTargeted    = true;
                    mTargetLockTime  = NOW;

                    mCurrentRotations = cameraRotations(player);
                    mRotStepAt = static_cast<uint64_t>(NOW);
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

    if (NOW - mTargetLockTime < static_cast<int64_t>(mReactionDelay.mValue))
        return;

    mRotating = (mRotateMode.mValue != RotateMode::None);

    // --- ATTACK ---
    if (isInAttackRange(target, player)) {
        const uint64_t now = static_cast<uint64_t>(NOW);

        // Темп задаёт НЕ настройка CPS, а неуязвимость цели: пока предыдущий
        // удар не подтверждён сервером, следующий всё равно уйдёт в пустоту
        // (в логах при 7.9 CPS засчиталось 0.6 — остальные пришли в окно
        // неуязвимости). APS-настройки убраны: они только противоречили
        // измеренному интервалу и давали «две правды» в одном модуле.
        uint64_t waitMs = 0;
        // Бьём только когда голова уже на цели: удар, отправленный
        // одновременно с доворотом, сервер видит с ПРОШЛОЙ ротацией.
        if (canAttack(now, waitMs) && rotationOnTarget(player)) {
            Actor* attackTarget = findObstructor(player, target);
            if (!attackTarget) attackTarget = target;

            if (mSwing.mValue) {
                try { player->swing(); } catch (...) {}
            }

            int bestSlot = getBestWeapon(target);
            // Слот хотбара — это 0..8. Раньше сюда мог прилететь мусор из
            // getBestItem (или -1), и игра падала на индексации инвентаря.
            if (bestSlot < 0 || bestSlot > 8)
                bestSlot = std::clamp(supplies->mSelectedSlot, 0, 8);

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

            mTiming.lastSent = now;
            mTiming.pending = true;
            mTiming.sentSinceConfirm++;
            mLastAttack = static_cast<int64_t>(now);
        }
    }

    // --- ROTATION ---
    // Компоненты головы/тела здесь больше НЕ трогаем. Их значение уходит в
    // пакет следующим тиком и перебивало то, что мы только что посчитали, а
    // тело, выставленное ровно по прицелу, дёргалось на каждом шаге. Ротация
    // живёт в одном месте — в пакете (см. onPacketOutEvent), а модель рисует
    // ровно то, что ушло на сервер.
    if (mRotating)
        stepRotation(player);
    } catch (...) {
        // Тихо сбрасываем цель: лучше один пропущенный тик, чем краш.
        resetTarget();
    }
}

// Текущая ротация игрока. head->mHeadRot игровая нить может держать в другой
// «ветке» (0..360) или ещё не обновить после поворота камеры, поэтому берём
// yaw из компоненты ротации актора — это ровно то, что уходит в пакет.
glm::vec2 Aura::cameraRotations(Actor* player) const
{
    glm::vec2 rots(0.f, 0.f);
    if (!player) return rots;

    try {
        if (auto* rot = player->getActorRotationComponent()) {
            rots.x = rot->mPitch;
            rots.y = rot->mYaw;
        } else if (auto* head = player->getActorHeadRotationComponent()) {
            rots.y = head->mHeadRot;
        }
    } catch (...) {}

    while (rots.y >  180.f) rots.y -= 360.f;
    while (rots.y < -180.f) rots.y += 360.f;
    rots.x = MathUtils::clamp(rots.x, -90.f, 90.f);
    return rots;
}

void Aura::stepRotation(Actor* player)
{
    if (!player) return;

    glm::vec2 want = calcRotations(player);
    if (!std::isfinite(want.x) || !std::isfinite(want.y)) return;   // цели нет

    // Реальное время с прошлого шага: раньше здесь стояли жёсткие 0.05 с,
    // и при 240°/с голова двигалась на 12° за тик независимо от того, сколько
    // прошло на самом деле.
    const uint64_t now = static_cast<uint64_t>(NOW);
    float dt = mRotStepAt && now > mRotStepAt
        ? static_cast<float>(now - mRotStepAt) / 1000.f
        : 0.05f;
    dt = std::clamp(dt, 0.001f, 0.1f);
    mRotStepAt = now;

    if (mRotSpeed.mValue <= 0.f) {
        mCurrentRotations = want;
        return;
    }

    const float maxStep = mRotSpeed.mValue * dt;
    float dYaw = MathUtils::wrap(want.y - mCurrentRotations.y, -180.f, 180.f);
    float dPitch = want.x - mCurrentRotations.x;

    // Дошли (шаг заведомо больше остатка) — ставим ровно цель. Без этого
    // голова «качалась» вокруг прицела и не проходила проверку в 3°.
    if (std::fabs(dYaw) <= maxStep + 0.05f)   mCurrentRotations.y = want.y;
    else mCurrentRotations.y += MathUtils::clamp(dYaw, -maxStep, maxStep);

    if (std::fabs(dPitch) <= maxStep + 0.05f) mCurrentRotations.x = want.x;
    else mCurrentRotations.x += MathUtils::clamp(dPitch, -maxStep, maxStep);

    mCurrentRotations.y = MathUtils::wrap(mCurrentRotations.y, -180.f, 180.f);
    mCurrentRotations.x = MathUtils::clamp(mCurrentRotations.x, -90.f, 90.f);
}

// ==================== PACKET OUT ====================

void Aura::onPacketOutEvent(PacketOutEvent& event) {
    if (!event.mPacket) return;

    // ВАЖНО: здесь нельзя трогать ничего, кроме самого пакета и простых чисел.
    // Этот код вызывается из пути отправки (LoopbackPacketSender::send) — в том
    // числе из середины тика уровня. Любой поход в реестр актёров оттуда
    // (getActorFromRuntimeID, calcRotations, getLocalPlayer()->getLevel()) —
    // итерация по реестру в момент, когда игра его меняет. Раньше здесь стоял
    // шаг доводки прицела (stepRotation) — именно он и ронял игру.
    const PacketID id = event.mPacket->getId();

    if (id == PacketID::PlayerAuthInput || id == PacketID::MovePlayer) {
        if (!mRotating || mTargetRuntimeID == 0) return;
        if (NOW - mTargetLockTime < static_cast<int64_t>(mReactionDelay.mValue)) return;

        const glm::vec2 rots = mCurrentRotations;   // просто копия, посчитанная в тике
        if (!std::isfinite(rots.x) || !std::isfinite(rots.y)) return;
        if (rots.x == 0.f && rots.y == 0.f) return;  // ещё не наводились

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

// ==================== HIT TIMING ====================

// Можно ли бить прямо сейчас.
//   - удар отправлен и не подтверждён → ждём (второй уйдёт в пустоту);
//   - сервер подтвердил попадание → ждём измеренный интервал до следующего.
bool Aura::canAttack(uint64_t now, uint64_t& waitMs)
{
    waitMs = 0;

    if (mTiming.pending)
    {
        // Сколько ждать ответа: по измеренной задержке, а пока её нет —
        // человеческие ~600 мс (не «молчим бесконечно»).
        const float cap = mTiming.confirmMs > 0.f
            ? std::clamp(mTiming.confirmMs * 2.2f, 250.f, 1500.f)
            : 600.f;

        const uint64_t age = now - mTiming.lastSent;
        if (age < static_cast<uint64_t>(cap))
        {
            waitMs = static_cast<uint64_t>(cap) - age;
            return false;
        }

        // Ответа нет — удар потерян: значит, рано. Отодвигаем темп, но если
        // сервер вообще не присылает HURT (ни одного подтверждения), верим,
        // что удар прошёл, и держим человеческий темп по настройке APS, иначе
        // на таком сервере аура замедлилась бы до 0.8 CPS.
        mTiming.pending = false;
        if (mTiming.lastConfirmed == 0)
        {
            // Сервер вообще не присылает HURT — держим человеческий темп.
            mTiming.intervalMs = kHurtCooldownMs;
        }
        else
        {
            mTiming.intervalMs = std::min(kMaxIntervalMs,
                                          std::max(mTiming.intervalMs, kHurtCooldownMs) + 70.f);
        }
        mTiming.nextAt = now + static_cast<uint64_t>(mTiming.intervalMs);
        waitMs = static_cast<uint64_t>(mTiming.intervalMs);
        return false;
    }

    if (mTiming.nextAt > now)
    {
        waitMs = mTiming.nextAt - now;
        return false;
    }

    return true;
}

// Голова уже смотрит в цель (по прошлому тику)? Значит сервер видел её на
// цели хотя бы один тик — удар не выглядит как выстрел в воздух.
bool Aura::rotationOnTarget(Actor* player) {
    if (mRotateMode.mValue == RotateMode::None) return true;
    if (!player) return false;

    glm::vec2 want = calcRotations(player);
    if (!std::isfinite(want.x) || !std::isfinite(want.y)) return false;

    float dYaw = mCurrentRotations.y - want.y;
    while (dYaw > 180.f)  dYaw -= 360.f;
    while (dYaw < -180.f) dYaw += 360.f;

    return std::fabs(mCurrentRotations.x - want.x) <= kAimToleranceDeg &&
           std::fabs(dYaw) <= kAimToleranceDeg;
}

// Сервер подтвердил урон по нашей цели: обновляем измеренный темп и ставим
// следующий удар чуть раньше конца неуязвимости — так он приходит ровно в
// тот тик, когда цель снова можно бить (как бьёт скиловый игрок).
void Aura::onHitConfirmed(int64_t targetId)
{
    if (targetId == 0 || targetId != mTargetRuntimeID) return;   // не наша цель
    if (!mTiming.pending) return;

    const uint64_t now = static_cast<uint64_t>(NOW);
    const float dt = static_cast<float>(now > mTiming.lastSent ? now - mTiming.lastSent : 0);
    mTiming.confirmMs = mTiming.confirmMs > 0.f ? (mTiming.confirmMs * 0.6f + dt * 0.4f) : dt;

    if (mTiming.intervalMs <= 0.f)
    {
        const float measured = (mTiming.lastConfirmed != 0 && now > mTiming.lastConfirmed)
            ? static_cast<float>(now - mTiming.lastConfirmed)
            : kHurtCooldownMs;
        mTiming.intervalMs = std::clamp(measured, kHurtCooldownMs, kMaxIntervalMs);
    }
    else if (mTiming.sentSinceConfirm <= 1)
    {
        // Попали с первого раза — можно на 20 мс раньше.
        mTiming.intervalMs = std::max(kHurtCooldownMs, mTiming.intervalMs - 20.f);
    }

    mTiming.lastConfirmed = now;
    mTiming.pending = false;
    mTiming.sentSinceConfirm = 0;

    const float jitter = MathUtils::randomFloat(-35.f, 35.f);
    const float next = std::max(kHurtCooldownMs, mTiming.intervalMs + jitter);
    mTiming.nextAt = now + static_cast<uint64_t>(std::max(next, 120.f));
}

// ==================== PACKET IN ====================

void Aura::onPacketInEvent(PacketInEvent& event) {
    if (!event.mPacket) return;
    if (!ClientInstance::get()) return;

    // ActorEvent HURT приходит на того, кому сервер реально засчитал урон —
    // это единственный честный признак «удар дошёл». По нему и строится темп.
    if (event.mPacket->getId() == PacketID::ActorEvent) {
        auto ev = event.getPacket<ActorEventPacket>();
        if (ev && ev->mEvent == ActorEvent::HURT)
            onHitConfirmed(ev->mRuntimeID);
        return;
    }

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
    if (!ImGui::GetCurrentContext()) return;
    if (!ClientInstance::get()) return;

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

    try {
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
        // Тот же стриппер, что у остальных модулей: старый локальный цикл
        // бежал «i += 3» и на однобайтовой кириллице съедал лишний символ,
        // а на границе строки читал за её концом.
        name = ColorUtils::removeColorCodes(resolvedTarget->getNameTag());
        dist = resolvedTarget->distanceTo(player);
    } catch (...) {
        return;
    }

    if (name.empty() || !std::isfinite(dist)) return;

    // Якорь подписи — верх хитбокса, как у ESP/Nametags. Раньше подпись
    // висела на «глаза + 2.2», то есть почти на блок выше макушки, и не
    // совпадала с рамкой — отсюда и ощущение, что Aura рисует не туда.
    ImVec2 screen;
    try {
        AABB aabb = resolvedTarget->getAABB();
        if (!std::isfinite(aabb.mMax.y)) return;

        glm::vec3 labelPos(
            (aabb.mMin.x + aabb.mMax.x) * 0.5f,
            aabb.mMax.y + 0.45f,
            (aabb.mMin.z + aabb.mMax.z) * 0.5f);

        if (!RenderUtils::worldToScreen(labelPos, screen)) return;
    } catch (...) {
        return;
    }

    if (!std::isfinite(screen.x) || !std::isfinite(screen.y)) return;

    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    std::string label = fmt::format("{} [{:.1f}m]", name, dist);

    ImFont* font = FontHelper::getFont(true);
    if (font) ImGui::PushFont(font);
    ImVec2 ts = ImGui::CalcTextSize(label.c_str());

    ImU32 shadow  = IM_COL32(0, 0, 0, (int)(alpha * 200));
    ImU32 textCol = IM_COL32(
        (int)(theme.Value.x * 255),
        (int)(theme.Value.y * 255),
        (int)(theme.Value.z * 255),
        (int)(alpha * 255)
    );

    const ImVec2 textPos(screen.x - ts.x / 2.f, screen.y - ts.y);
    dl->AddText({textPos.x + 1.f, textPos.y + 1.f}, shadow,  label.c_str());
    dl->AddText(textPos,                             textCol, label.c_str());

    if (font) ImGui::PopFont();
    } catch (...) {
        sHasTarget       = false;
        sTargetRuntimeID = 0;
    }
}

