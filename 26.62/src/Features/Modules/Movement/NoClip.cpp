#include "NoClip.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftSim.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorRotationComponent.hpp>
#include <SDK/Minecraft/Actor/Components/AABBShapeComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

void NoClip::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent,
        &NoClip::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent,
        &NoClip::onPacketOutEvent,
        nes::event_priority::VERY_LAST>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // ── Сохраняем оригинальные флаги ──
    mHadCollision = player->getStatusFlag(ActorFlags::HasCollision);
    mHadGravity   = player->getStatusFlag(ActorFlags::HasGravity);

    // ── Отключаем коллизию на уровне движка ──
    // Бит 48 (HasCollision) = 0 → NoClipOrNoBlockMoveFilterSystem
    // пропускает MoveCollisionSystem полностью
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
}

void NoClip::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,
        &NoClip::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent,
        &NoClip::onPacketOutEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player) {
        // ── Восстанавливаем движковые флаги ──
        player->setStatusFlag(ActorFlags::HasCollision, mHadCollision);
        player->setStatusFlag(ActorFlags::HasGravity, mHadGravity);

        // ── Убиваем velocity чтобы не улететь ──
        auto sv = player->getStateVectorComponent();
        if (sv) sv->mVelocity = glm::vec3(0.f);

        // ── Сброс fall distance → нет урона при выключении ──
        // ResetRunAndFallDistance система не сработает за нас
        player->setFallDistance(0.f);

        // ── AntiPush: предотвращаем выталкивание ──
        // MoveTowardsClosestSpaceSystemFromActor использует
        // PushTowardsClosestSpace (0x6D) флаг
        if (mAntiPush.mValue) {
            player->setOnGround(true);
        }
    }

    // ── Сброс таймера ──
    if (mTimerWasSet) {
        ClientInstance::get()->getMinecraftSim()->setSimTimer(20.f);
        mTimerWasSet = false;
    }
}

void NoClip::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto stateVector = player->getStateVectorComponent();
    auto rotComp     = player->getActorRotationComponent();
    auto moveInput   = player->getMoveInputComponent();

    if (!stateVector || !rotComp || !moveInput) return;

    // ══════════════════════════════════════════════════════
    // Принудительно выставляем флаги КАЖДЫЙ тик
    // Сервер может сбросить через ActorDataPacket
    // ══════════════════════════════════════════════════════
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);

    // Не даём MoveTowardsClosestSpaceSystemFromActor
    // выталкивать нас из блоков
    if (mAntiPush.mValue) {
        player->setStatusFlag(ActorFlags::PushTowardsClosestSpace, false);
    }

    // Предотвращаем ложное состояние "на земле"
    player->setOnGround(false);

    // Сбрасываем fall distance каждый тик → нет накопления
    player->setFallDistance(0.f);

    // ══════════════════════════════════════════════════════
    // Управление полётом
    // Перезаписываем velocity ПОСЛЕ всей нашей логики
    // Примечание: MoveSpeedCapSystem (sub_144FBC3A0+168)
    // выполняется ПЕРЕД NoClipFilter (+355) и может
    // ограничить скорость. Компенсируем через прямую
    // установку velocity — она применяется как deltaPos
    // в NoClipOrNoBlockMoveFilterSystem
    // ══════════════════════════════════════════════════════
    glm::vec3 motion(0.f);
    float speed = mSpeed.mValue / 10.f;

    // Горизонтальное движение по WASD + yaw
    if (Keyboard::isUsingMoveKeys(true)) {
        glm::vec2 calc = MathUtils::getMotion(rotComp->mYaw, speed);
        motion.x = calc.x;
        motion.z = calc.y;
    }

    // Вертикальное движение
    if (moveInput->mIsJumping)
        motion.y += speed;
    else if (moveInput->mIsSneakDown)
        motion.y -= speed;

    // Перезапись velocity — гравитация и инерция не действуют
    stateVector->mVelocity = motion;

    // ══════════════════════════════════════════════════════
    // Timer boost
    // ══════════════════════════════════════════════════════
    if (mTimerBoost.mValue) {
        mTimerWasSet = true;
        ClientInstance::get()->getMinecraftSim()->setSimTimer(
            mTimerBoostValue.mValue);
    } else if (mTimerWasSet) {
        mTimerWasSet = false;
        ClientInstance::get()->getMinecraftSim()->setSimTimer(20.f);
    }
}

void NoClip::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() != PacketID::PlayerAuthInput) return;

    auto packet = event.getPacket<PlayerAuthInputPacket>();

    if (mApplyGlideFlags.mValue) {
        packet->mInputData |= AuthInputAction::START_GLIDING;
        packet->mInputData &= ~AuthInputAction::STOP_GLIDING;
    }
}