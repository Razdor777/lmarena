#include "Criticals.hpp"

#include <cstdio>

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/CritPatchUtils.hpp>
#include <libhat.hpp>

void Criticals::onEnable()
{
    mSnap = {};
    mSending = false;
    mLastFakeTick = -1;

    if (mMode.mValue == Mode::Native)
    {
        applyNative();
        return;
    }

    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Criticals::onPacketOutEvent,
        nes::event_priority::VERY_LAST>(this);
}

void Criticals::onDisable()
{
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Criticals::onPacketOutEvent>(this);
    mSnap = {};

    if (critpatch::isApplied())
    {
        critpatch::restore();
        ChatUtils::displayClientMessage("§e[Criticals] §fНативный патч снят, проверки критов возвращены как было.");
    }
}

// Нативный режим: ищем в .text цепочку проверок и глушим её.
// Ничего не делаем, если структура не совпала — молча патчить незнакомый код
// хуже, чем не работать вообще.
void Criticals::applyNative()
{
    if (critpatch::isApplied())
    {
        ChatUtils::displayClientMessage("§a[Criticals] §fНативный патч уже применён.");
        return;
    }

    auto text = hat::process::get_process_module().get_section_data(".text");
    if (text.empty())
    {
        ChatUtils::displayClientMessage("§c[Criticals] §fНе смог прочитать .text игры.");
        setEnabled(false);
        return;
    }

    critpatch::Plan plan;
    std::string why;

    const uintptr_t base = reinterpret_cast<uintptr_t>(text.data());
    if (!critpatch::find(reinterpret_cast<const uint8_t*>(text.data()), text.size(), base, plan, why))
    {
        ChatUtils::displayClientMessage(
            "§c[Criticals] §fАвтопоиск проверок критов не сработал (" + why + "). "
            "Глянь, что есть в этой сборке: §6.crit scan§f. "
            "Если там пусто — оставь режим PocketMine или AuthInput.");
        setEnabled(false);
        return;
    }

    if (!critpatch::apply(plan))
    {
        ChatUtils::displayClientMessage("§c[Criticals] §fЦепочка найдена, но записать патч не удалось.");
        setEnabled(false);
        return;
    }

    char buffer[192];
    snprintf(buffer, sizeof(buffer),
             "§a[Criticals] §fНативный патч применён: §6%zu§f переходов, крит-блок +0x%llX.",
             plan.sites.size(), static_cast<unsigned long long>(plan.critBlock - base));
    ChatUtils::displayClientMessage(buffer);
}

void Criticals::sendFake(float yOffset, float yDelta, AuthInputAction flags)
{
    if (!mSnap.valid) return;

    auto ci = ClientInstance::get();
    if (!ci) return;
    auto sender = ci->getPacketSender();
    if (!sender) return;

    auto pkt = MinecraftPackets::createPacket<PlayerAuthInputPacket>();
    if (!pkt) return;

    pkt->mRot               = mSnap.rot;
    pkt->mPos               = mSnap.pos;
    pkt->mPos.y            += yOffset;            // mPos в auth input уже на высоте глаз
    pkt->mYHeadRot          = mSnap.headRot;
    pkt->mPosDelta          = { 0.f, yDelta, 0.f };
    pkt->mAnalogMoveVector  = { 0.f, 0.f };
    pkt->mVehicleRotation   = { 0.f, 0.f };
    pkt->mMove              = { 0.f, 0.f };
    pkt->mInteractRots      = mSnap.interactRots;
    pkt->mCameraOrientation = mSnap.cameraOrientation;
    pkt->mInputData         = flags;
    pkt->mInputMode         = mSnap.inputMode;
    pkt->mPlayMode          = mSnap.playMode;
    pkt->mNewInteractionModel = mSnap.interaction;
    pkt->mClientTick        = mSnap.tick;          // см. вопрос к другу про тик
    pkt->mPredictedVehicle  = mSnap.predictedVehicle;
    pkt->mPlayerBlockActions.mActions.clear();

    mSending = true;
    sender->sendToServer(pkt.get());
    mSending = false;
}

void Criticals::onPacketOutEvent(PacketOutEvent& event)
{
    if (mSending || !event.mPacket) return;

    const PacketID id = event.mPacket->getId();

    // 1) запоминаем последний настоящий auth input
    if (id == PacketID::PlayerAuthInput) {
        auto* p = event.getPacket<PlayerAuthInputPacket>();
        mSnap.valid             = true;
        mSnap.rot               = p->mRot;
        mSnap.pos               = p->mPos;
        mSnap.headRot           = p->mYHeadRot;
        mSnap.interactRots      = p->mInteractRots;
        mSnap.cameraOrientation = p->mCameraOrientation;
        mSnap.inputMode         = p->mInputMode;
        mSnap.playMode          = p->mPlayMode;
        mSnap.interaction       = p->mNewInteractionModel;
        mSnap.tick              = p->mClientTick;
        mSnap.predictedVehicle  = p->mPredictedVehicle;
        return;
    }

    // 2) ловим атаку
    if (id != PacketID::InventoryTransaction) return;
    if (event.isCancelled()) return;
    if (!mSnap.valid) return;

    auto* itp = event.getPacket<InventoryTransactionPacket>();
    if (!itp || !itp->mTransaction) return;
    if (itp->mTransaction->type != ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) return;

    auto* tx = reinterpret_cast<ItemUseOnActorInventoryTransaction*>(itp->mTransaction.get());
    if (tx->mActionType != ItemUseOnActorInventoryTransaction::ActionType::Attack) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    if (mOnlyOnGround.mValue && !player->isOnGround()) return;

    // уже слали фейки в этом тике (мульти-таргет и т.п.)
    if (mLastFakeTick == mSnap.tick) return;
    mLastFakeTick = mSnap.tick;

    const float h = mHeight.mValue;

    if (mMode.mValue == Mode::PocketMine) {
        // мини-прыжок: вверх на h, затем вниз на h/2 -> !onGround и fallDistance > 0
        sendFake(h,        +h,         AuthInputAction::NONE);
        sendFake(h * 0.5f, -h * 0.5f,  AuthInputAction::NONE);
    } else {
        // имитация прыжка: вверх с флагами прыжка, потом вниз без них
        const AuthInputAction up =
            AuthInputAction::JUMPING | AuthInputAction::JUMP_DOWN |
            AuthInputAction::WANT_UP | AuthInputAction::START_JUMPING;
        sendFake(h,        +h,         up);
        sendFake(h * 0.5f, -h * 0.5f,  AuthInputAction::NONE);
    }
    // оригинальная атака уйдёт сразу после наших пакетов
}
