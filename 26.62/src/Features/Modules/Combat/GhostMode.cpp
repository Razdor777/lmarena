//
// GhostMode.cpp
//
// Uses Freecam Detached camera system (RedirectCameraInputComponent)
// so WASD does NOT move the player entity. Body stays frozen at A.
// Ghost movement is purely camera-based via mOrigin.
// Attack uses InfiniteAura chain TP pattern: body(A) → ghost(B) → attack → body(A).
//

#include "GhostMode.hpp"

#include <cmath>
#include <algorithm>

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftSim.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Actor/Components/FlagComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorWalkAnimationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/Keyboard.hpp>

// =========================================================
static std::shared_ptr<MovePlayerPacket> makePkt(glm::vec3 pos, glm::vec3 rots, int64_t rid)
{
    auto p = MinecraftPackets::createPacket<MovePlayerPacket>();
    p->mPos              = pos;
    p->mPlayerID         = rid;
    p->mRot              = { rots.x, rots.y };
    p->mYHeadRot         = rots.z;
    p->mResetPosition    = PositionMode::Teleport;
    p->mOnGround         = true;
    p->mRidingID         = -1;
    p->mCause            = TeleportationCause::Unknown;
    p->mSourceEntityType = ActorType::Player;
    p->mTick             = 0;
    return p;
}

// =========================================================
//  ENABLE
// =========================================================
void GhostMode::onEnable()
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) { setEnabled(false); return; }

    auto* sv = player->getStateVectorComponent();
    if (!sv) { setEnabled(false); return; }

    // Save body position (point A) ONCE
    mBodyPos   = sv->mPos;
    mSvPosOld  = sv->mPosOld;

    // Ghost starts at body
    mOrigin    = mBodyPos;
    mOldOrigin = mBodyPos;

    // Read current rotation
    auto rot = player->getActorRotationComponent();
    if (rot) mRots = { rot->mPitch, rot->mYaw, rot->mYaw };

    mWantsAttack        = false;
    mAttackGhostVisible = false;
    mAttackPath.clear();

    // === KEY FIX: Camera flags (same as Freecam Detached) ===
    // RedirectCameraInputComponent prevents WASD from moving the player entity.
    // RenderCameraComponent + CameraRenderPlayerModelComponent let you see your body at A.
    player->setFlag<RenderCameraComponent>(true);
    player->setFlag<CameraRenderPlayerModelComponent>(true);
    player->setFlag<RedirectCameraInputComponent>(true);

    // Stop walk animation so body looks frozen
    player->getWalkAnimationComponent()->mWalkAnimSpeed = 0.0f;

    // Freeze physics
    sv->mVelocity = {};
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity,   false);
    player->setFallDistance(0.f);

    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &GhostMode::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &GhostMode::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,  &GhostMode::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<LookInputEvent, &GhostMode::onLookInputEvent>(this);
    gFeatureManager->mDispatcher->listen<MouseEvent,     &GhostMode::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,    &GhostMode::onRenderEvent>(this);

    NotifyUtils::notify("§aGhost ON", 2.f, Notification::Type::Info);
}

// =========================================================
//  DISABLE
// =========================================================
void GhostMode::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,  &GhostMode::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &GhostMode::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,  &GhostMode::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &GhostMode::onLookInputEvent>(this);
    gFeatureManager->mDispatcher->deafen<MouseEvent,     &GhostMode::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &GhostMode::onRenderEvent>(this);

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (player) {
        // Clear camera flags — back to normal
        player->setFlag<RenderCameraComponent>(false);
        player->setFlag<CameraRenderPlayerModelComponent>(false);
        player->setFlag<RedirectCameraInputComponent>(false);

        // Restore position to body
        auto* sv = player->getStateVectorComponent();
        if (sv) {
            sv->mPos      = mBodyPos;
            sv->mPosOld   = mSvPosOld;
            sv->mVelocity = {};
        }

        // Re-enable physics
        player->setStatusFlag(ActorFlags::HasCollision, true);
        player->setStatusFlag(ActorFlags::HasGravity,   true);
        player->setFallDistance(0.f);

        // Restore walk animation and input
        player->getWalkAnimationComponent()->mWalkAnimSpeed = 1.0f;
        player->getMoveInputComponent()->reset(false);
    }

    NotifyUtils::notify("§eGhost OFF", 2.f, Notification::Type::Info);
}

// =========================================================
//  MAIN TICK
// =========================================================
void GhostMode::onBaseTickEvent(BaseTickEvent& event)
{
    auto* player = event.mActor;
    if (!player) return;

    // ===========================
    // 1) Keep camera flags active
    // ===========================
    player->setFlag<RenderCameraComponent>(true);
    player->setFlag<CameraRenderPlayerModelComponent>(true);

    // ===========================
    // 2) Freeze body at point A (safety net)
    //    With RedirectCameraInputComponent, WASD doesn't move the entity,
    //    so this is just insurance — no jitter since body never moves.
    // ===========================
    auto* sv = player->getStateVectorComponent();
    if (sv) {
        sv->mVelocity = {};
        sv->mPos      = mBodyPos;
        sv->mPosOld   = mBodyPos;
    }
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity,   false);
    player->setFallDistance(0.f);

    // ===========================
    // 3) Ghost movement (camera only, applied to mOrigin)
    //    WASD is read from keyboard directly, not from player input.
    // ===========================
    glm::vec3 motion{};
    if (Keyboard::isUsingMoveKeys(true))
    {
        glm::vec2 rots = mRotRads;
        rots = glm::vec2(rots.y, rots.x);
        rots = glm::degrees(rots);
        rots.y = -rots.y + 180.f;
        rots.y = MathUtils::wrap(rots.y, -180.f, 180.f);
        glm::vec2 calc = MathUtils::getMotion(rots.y, mSpeed.mValue / 10.f, false);
        motion.x = calc.x;
        motion.z = calc.y;

        auto& kb = *ClientInstance::get()->getKeyboardSettings();
        if (Keyboard::mPressedKeys[kb["key.jump"]])
            motion.y += mSpeed.mValue / 10.f;
        else if (Keyboard::mPressedKeys[kb["key.sneak"]])
            motion.y -= mSpeed.mValue / 10.f;
    }

    mOldOrigin = mOrigin;
    mOrigin   += motion;

    // ===========================
    // 4) Attack on click
    // ===========================
    if (mWantsAttack)
    {
        mWantsAttack = false;

        auto* sender   = ClientInstance::get()->getPacketSender();
        auto* supplies = player->getSupplies();
        if (!sender || !supplies) return;

        glm::vec3 ghostPos = mOrigin;
        int64_t   rid      = player->getRuntimeID();

        // --- Find target near ghost ---
        auto actors = ActorUtils::getActorList(true, true);
        Actor* best = nullptr;
        float  bestD = mAttackRange.mValue;
        for (auto* a : actors) {
            if (!a || a == player) continue;
            if (!player->mContext.mRegistry->valid(a->mContext.mEntityId)) continue;
            try { if (a->isDead() || a->getHealth() <= 0.f) continue; } catch (...) { continue; }
            if (mIgnoreFriends.mValue && gFriendManager && gFriendManager->mEnabled)
                try { if (gFriendManager->isFriend(a)) continue; } catch (...) {}
            try {
                float d = glm::distance(ghostPos, *a->getPos());
                if (d < bestD) { bestD = d; best = a; }
            } catch (...) {}
        }
        if (!best) return;

        // --- STEP 1: Chain TP  body(A) → ghost(B) ---
        {
            glm::vec3 from = mBodyPos, to = ghostPos;
            glm::vec3 diff = to - from;
            float dist = glm::length(diff);
            std::vector<glm::vec3> positions;

            if (dist >= 0.01f) {
                glm::vec3 dir = glm::normalize(diff);
                glm::vec3 cur = from;
                while (glm::distance(cur, to) > mStepDist.mValue) {
                    cur += dir * mStepDist.mValue;
                    positions.push_back(cur);
                    sender->sendToServer(makePkt(cur, mRots, rid).get());
                }
            }
            positions.push_back(to);
            sender->sendToServer(makePkt(to, mRots, rid).get());

            std::lock_guard<std::mutex> lock(mMutex);
            mAttackPath = positions;
            mLastPathTime = NOW;
        }

        // --- STEP 2: Criticals at ghost pos ---
        if (mCriticals.mValue) {
            auto up = makePkt(ghostPos + glm::vec3(0.f, 0.1f, 0.f), mRots, rid);
            up->mOnGround = false;
            sender->sendToServer(up.get());
            auto dn = makePkt(ghostPos, mRots, rid);
            dn->mOnGround = false;
            sender->sendToServer(dn.get());
        }

        // --- STEP 3: Attack ---
        try {
            player->swing();
            auto atk = ActorUtils::createAttackTransaction(best, supplies->mSelectedSlot);
            if (atk) sender->sendToServer(atk.get());
        } catch (...) {}

        // --- STEP 4: Chain TP back  ghost(B) → body(A) ---
        {
            glm::vec3 from = ghostPos, to = mBodyPos;
            glm::vec3 diff = to - from;
            float dist = glm::length(diff);
            if (dist >= 0.01f) {
                glm::vec3 dir = glm::normalize(diff);
                glm::vec3 cur = from;
                while (glm::distance(cur, to) > mStepDist.mValue) {
                    cur += dir * mStepDist.mValue;
                    sender->sendToServer(makePkt(cur, mRots, rid).get());
                }
            }
            sender->sendToServer(makePkt(to, mRots, rid).get());
        }

        mAttackGhostPos     = ghostPos;
        mAttackGhostVisible = true;
    }
}

// =========================================================
//  PACKET OUT
// =========================================================
void GhostMode::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;

    // Read rotation from outgoing MovePlayer (InfiniteAura pattern)
    if (event.mPacket->getId() == PacketID::MovePlayer) {
        auto pkt = event.getPacket<MovePlayerPacket>();
        if (pkt) mRots = { pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot };
    }

    // Remove WASD input from PlayerAuthInput so server doesn't process movement
    if (event.mPacket->getId() == PacketID::PlayerAuthInput) {
        auto* paip = event.getPacket<PlayerAuthInputPacket>();
        if (paip) paip->removeMovingInput();
    }
}

// =========================================================
//  PACKET IN — echo lagbacks (InfiniteAura pattern)
// =========================================================
void GhostMode::onPacketInEvent(PacketInEvent& event)
{
    if (!mSilentAccept.mValue) return;
    if (!event.mPacket) return;
    if (event.mPacket->getId() != PacketID::MovePlayer) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MovePlayerPacket>();
    if (pkt && pkt->mPlayerID == player->getRuntimeID()) {
        event.cancel();
        ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
    }
}

// =========================================================
//  LOOK INPUT — set camera to ghost position
//  Uses absolute origin (like Freecam Detached), not offset.
// =========================================================
void GhostMode::onLookInputEvent(LookInputEvent& event)
{
    // Read camera rotation (for WASD direction calculation)
    if (event.mCameraDirectLookComponent)
        mRotRads = event.mCameraDirectLookComponent->mRotRads;

    // Set camera to ghost position (absolute, smooth lerp)
    if (event.mFirstPersonCamera)
        event.mFirstPersonCamera->mOrigin = getLerpedOrigin();
}

// =========================================================
//  MOUSE — left click attack
// =========================================================
void GhostMode::onMouseEvent(MouseEvent& event)
{
    if (event.mActionButtonId == 1 && event.mButtonData == 1)
        mWantsAttack = true;
}

// =========================================================
//  RENDER
// =========================================================
void GhostMode::onRenderEvent(RenderEvent& event)
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    uint64_t fadeTime = 600;
    float alpha = 1.0f;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mLastPathTime + fadeTime < NOW)
            mAttackPath.clear();
        else
            alpha = 1.0f - (float)(NOW - mLastPathTime) / (float)fadeTime;
    }
    alpha = std::clamp(alpha, 0.f, 1.f);

    // BODY (blue box at A)
    if (mDrawBody.mValue) {
        AABB box;
        box.mMin = mBodyPos - glm::vec3(0.3f, 1.62f, 0.3f);
        box.mMax = mBodyPos + glm::vec3(0.3f, 0.18f, 0.3f);
        auto pts = MathUtils::getImBoxPoints(box);
        if (pts.size() >= 3) {
            dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), IM_COL32(50, 100, 255, 40));
            dl->AddPolyline(pts.data(), (int)pts.size(), IM_COL32(50, 100, 255, 200), true, 2.f);
        }
    }

    // CHAIN TP PATH (fading lines)
    if (mDrawPath.mValue && alpha > 0.01f) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mAttackPath.empty()) {
            std::vector<ImVec2> points;
            for (auto& pos : mAttackPath) {
                ImVec2 pt;
                if (RenderUtils::worldToScreen(pos, pt)) points.emplace_back(pt);
            }
            for (size_t i = 0; i + 1 < points.size(); i++) {
                ImColor c = ColorUtils::getThemedColor((float)i * 0.05f);
                c.Value.w *= alpha;
                dl->AddLine(points[i], points[i + 1], c, 2.f);
            }
        }
    }

    // GHOST box at hit pos
    if (mDrawGhost.mValue && mAttackGhostVisible && alpha > 0.01f) {
        AABB box;
        box.mMin = mAttackGhostPos - glm::vec3(0.3f, 1.62f, 0.3f);
        box.mMax = mAttackGhostPos + glm::vec3(0.3f, 0.18f, 0.3f);
        auto pts = MathUtils::getImBoxPoints(box);
        if (!pts.empty()) {
            ImColor col = ColorUtils::getThemedColor(0);
            col.Value.w = 0.15f * alpha;
            dl->AddConvexPolyFilled(pts.data(), (int)pts.size(), col);
            col.Value.w = 0.8f * alpha;
            dl->AddPolyline(pts.data(), (int)pts.size(), col, true, 2.f);
        }
    }

    if (NOW - mLastPathTime > fadeTime)
        mAttackGhostVisible = false;
}
