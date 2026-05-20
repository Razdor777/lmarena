//
// Criticals.cpp — Forces every melee hit to deal critical damage.
//
// HOW IT WORKS (Minecraft Bedrock):
// A hit is critical when the attacker is FALLING (negative Y velocity,
// not on ground). We exploit this with TWO complementary methods:
//
//   1) PlayerAuthInputPacket spoofing:
//      Every tick we alternate the Y position in the outgoing AuthInput
//      packet (+offset on even ticks, normal on odd ticks). This makes
//      the server think we're constantly micro-jumping. On "odd" ticks
//      the server sees us falling from the previous raised position.
//      We also clear the VERTICAL_COLLISION flag so the server doesn't
//      think we're on the ground.
//
//   2) MovePlayerPacket injection (on attack):
//      When we detect an outgoing attack (InventoryTransactionPacket),
//      we inject two MovePlayerPacket spoofs BEFORE the attack packet:
//        - pos+offset (onGround=false, PositionMode::Teleport)
//        - pos        (onGround=false, PositionMode::Teleport)
//      This is the exact same pattern used by InfiniteAura and GhostMode.
//      PositionMode::Teleport is KEY — Normal mode gets ignored by server.
//

#include "Criticals.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

// ==================== LIFECYCLE ====================

void Criticals::onEnable() {
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Criticals::onPacketOutEvent>(this);
    mTickCounter = 0;
}

void Criticals::onDisable() {
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Criticals::onPacketOutEvent>(this);
    mTickCounter = 0;
}

// ==================== PACKET HANDLER ====================

void Criticals::onPacketOutEvent(PacketOutEvent& event) {
    if (!event.mPacket) return;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    PacketID id = event.mPacket->getId();

    // ─────────────────────────────────────────────────────
    // METHOD 1: Modify PlayerAuthInputPacket every tick
    // Alternate Y position so server always sees us "falling"
    // ─────────────────────────────────────────────────────
    if (id == PacketID::PlayerAuthInput) {
        auto* pkt = event.getPacket<PlayerAuthInputPacket>();
        if (!pkt) return;

        float offset = mYOffset.mValue;
        mTickCounter++;

        if (mTickCounter % 2 == 0) {
            // Even tick: bump position UP
            pkt->mPos.y += offset;
            pkt->mPosDelta.y = offset;
        } else {
            // Odd tick: normal position (server sees FALLING from previous tick = crit!)
            pkt->mPosDelta.y = -offset;
        }

        // Remove vertical collision flag — we're "airborne"
        pkt->mInputData &= ~AuthInputAction::VERTICAL_COLLISION;
        return;
    }

    // ─────────────────────────────────────────────────────
    // METHOD 2: Inject MovePlayerPacket spoofs before attacks
    // Uses PositionMode::Teleport (same as InfiniteAura/GhostMode)
    // ─────────────────────────────────────────────────────
    if (id != PacketID::InventoryTransaction) return;

    auto* pkt = event.getPacket<InventoryTransactionPacket>();
    if (!pkt || !pkt->mTransaction) return;

    // Only process attack transactions
    if (pkt->mTransaction->getTransacType() != ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction)
        return;

    auto* attackTx = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(pkt->mTransaction.get());
    if (!attackTx) return;

    if (attackTx->mActionType != ItemUseOnActorInventoryTransaction::ActionType::Attack)
        return;

    auto* sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    auto* sv = player->getStateVectorComponent();
    if (!sv) return;

    glm::vec3 playerPos = sv->mPos;
    int64_t   rid       = player->getRuntimeID();
    float     offset    = mYOffset.mValue;

    // Read current rotation
    float yaw = 0.f, pitch = 0.f;
    if (auto* rot = player->getActorRotationComponent()) {
        pitch = rot->mPitch;
        yaw   = rot->mYaw;
    }

    // Spoof packet 1: slightly above current position, NOT on ground
    // PositionMode::Teleport forces the server to accept this position!
    auto upPkt = MinecraftPackets::createPacket<MovePlayerPacket>();
    upPkt->mPos              = playerPos + glm::vec3(0.f, offset, 0.f);
    upPkt->mPlayerID         = rid;
    upPkt->mRot              = { pitch, yaw };
    upPkt->mYHeadRot         = yaw;
    upPkt->mResetPosition    = PositionMode::Teleport;
    upPkt->mOnGround         = false;
    upPkt->mRidingID         = -1;
    upPkt->mCause            = TeleportationCause::Unknown;
    upPkt->mSourceEntityType = ActorType::Player;
    upPkt->mTick             = 0;
    sender->sendToServer(upPkt.get());

    // Spoof packet 2: back to original position, still NOT on ground
    // Server sees us FALLING from pos+offset to pos → critical hit!
    auto downPkt = MinecraftPackets::createPacket<MovePlayerPacket>();
    downPkt->mPos              = playerPos;
    downPkt->mPlayerID         = rid;
    downPkt->mRot              = { pitch, yaw };
    downPkt->mYHeadRot         = yaw;
    downPkt->mResetPosition    = PositionMode::Teleport;
    downPkt->mOnGround         = false;
    downPkt->mRidingID         = -1;
    downPkt->mCause            = TeleportationCause::Unknown;
    downPkt->mSourceEntityType = ActorType::Player;
    downPkt->mTick             = 0;
    sender->sendToServer(downPkt.get());

    // The original attack packet goes through right after these two spoofs
}
