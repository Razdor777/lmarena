#include "ChainTP.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

void ChainTP::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &ChainTP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &ChainTP::onPacketOutEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        setEnabled(false);
        return;
    }

    mTargetPos = {mTargetX.mValue, mTargetY.mValue, mTargetZ.mValue};
    mCurrentFakePos = *player->getPos();
    mTotalDistance = glm::distance(mCurrentFakePos, mTargetPos);
    mTraveledDistance = 0;
    mIsTeleporting = true;

    float blocksPerSecond = mStepSize.mValue * 20.0f * mStepsPerTick.mValue;
    float eta = mTotalDistance / blocksPerSecond;

    ChatUtils::displayClientMessage("§a[ChainTP] §fStarting teleport!");
    ChatUtils::displayClientMessage("§7  From: §e" +
        std::to_string((int)mCurrentFakePos.x) + ", " +
        std::to_string((int)mCurrentFakePos.y) + ", " +
        std::to_string((int)mCurrentFakePos.z));
    ChatUtils::displayClientMessage("§7  To: §e" +
        std::to_string((int)mTargetPos.x) + ", " +
        std::to_string((int)mTargetPos.y) + ", " +
        std::to_string((int)mTargetPos.z));
    ChatUtils::displayClientMessage("§7  Distance: §e" + std::to_string((int)mTotalDistance) +
        " §7blocks | ETA: §e" + std::to_string((int)eta) + "s");
    ChatUtils::displayClientMessage("§7  Speed: §e" + std::to_string((int)blocksPerSecond) + " §7blocks/sec");
}

void ChainTP::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &ChainTP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &ChainTP::onPacketOutEvent>(this);

    if (mIsTeleporting)
    {
        ChatUtils::displayClientMessage("§c[ChainTP] §fTeleport cancelled!");
    }

    mIsTeleporting = false;
}

// =========================================================
// Перехват PlayerAuthInput — подменяем позицию
// =========================================================
void ChainTP::onPacketOutEvent(PacketOutEvent& event)
{
    if (!mIsTeleporting) return;
    if (!mSpoofAuthInput.mValue) return;

    if (event.mPacket->getId() == PacketID::PlayerAuthInput)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();

        // Подменяем позицию на нашу фейковую
        paip->mPos = mCurrentFakePos;
        paip->mPosDelta = glm::vec3(0, 0, 0);
    }
}

// =========================================================
// Главная логика — каждый тик двигаемся к цели
// =========================================================
void ChainTP::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    // === Grab Pos ===
    if (mGrabPos.mValue)
    {
        mGrabPos.mValue = false;
        auto level = player->getLevel();
        if (level)
        {
            auto hitResult = level->getHitResult();
            if (hitResult && hitResult->mType == HitType::BLOCK)
            {
                mTargetX.mValue = static_cast<float>(hitResult->mBlockPos.x);
                mTargetY.mValue = static_cast<float>(hitResult->mBlockPos.y + 1);
                mTargetZ.mValue = static_cast<float>(hitResult->mBlockPos.z);
                ChatUtils::displayClientMessage("§a[ChainTP] §fTarget set: §e" +
                    std::to_string((int)mTargetX.mValue) + ", " +
                    std::to_string((int)mTargetY.mValue) + ", " +
                    std::to_string((int)mTargetZ.mValue));
            }
        }
    }

    // === Set Current Pos ===
    if (mGrabCurrentPos.mValue)
    {
        mGrabCurrentPos.mValue = false;
        glm::vec3 pos = *player->getPos();
        mTargetX.mValue = pos.x;
        mTargetY.mValue = pos.y;
        mTargetZ.mValue = pos.z;
        ChatUtils::displayClientMessage("§a[ChainTP] §fTarget set to current pos");
    }

    if (!mIsTeleporting) return;

    // =========================================================
    // Двигаемся к цели по шагам
    // =========================================================
    float stepSize = mStepSize.mValue;
    int stepsThisTick = static_cast<int>(mStepsPerTick.mValue);

    for (int step = 0; step < stepsThisTick; step++)
    {
        float remaining = glm::distance(mCurrentFakePos, mTargetPos);

        // === Прибыли? ===
        if (remaining <= stepSize)
        {
            mCurrentFakePos = mTargetPos;

            // Телепортируем игрока на финальную позицию
            player->setPosition(mTargetPos);

            // Отправляем MovePlayer чтобы сервер точно знал
            auto movePacket = MinecraftPackets::createPacket<MovePlayerPacket>();
            movePacket->mPos = mTargetPos;
            movePacket->mRot = {
                player->getActorRotationComponent()->mPitch,
                player->getActorRotationComponent()->mYaw
            };
            movePacket->mYHeadRot = player->getActorHeadRotationComponent()->mHeadRot;
            movePacket->mPlayerID = player->getRuntimeID();
            movePacket->mOnGround = true;
            movePacket->mResetPosition = PositionMode::Normal;
            movePacket->mTick = 0;

            auto sender = ClientInstance::get()->getPacketSender();
            if (sender) sender->sendToServer(movePacket.get());

            mIsTeleporting = false;

            ChatUtils::displayClientMessage("§a[ChainTP] §fArrived! §e" +
                std::to_string((int)mTotalDistance) + " §fblocks traveled");

            if (mAutoDisable.mValue)
            {
                setEnabled(false);
            }
            return;
        }

        // === Вычисляем направление к цели ===
        glm::vec3 direction = glm::normalize(mTargetPos - mCurrentFakePos);

        // === Двигаемся на stepSize блоков ===
        mCurrentFakePos += direction * stepSize;
        mTraveledDistance += stepSize;

        // === Отправляем MovePlayerPacket ===
        auto movePacket = MinecraftPackets::createPacket<MovePlayerPacket>();
        movePacket->mPos = mCurrentFakePos;
        movePacket->mRot = {
            player->getActorRotationComponent()->mPitch,
            player->getActorRotationComponent()->mYaw
        };
        movePacket->mYHeadRot = player->getActorHeadRotationComponent()->mHeadRot;
        movePacket->mPlayerID = player->getRuntimeID();
        movePacket->mOnGround = false;
        movePacket->mResetPosition = PositionMode::Normal;
        movePacket->mTick = 0;

        auto sender = ClientInstance::get()->getPacketSender();
        if (sender) sender->sendToServer(movePacket.get());
    }

    // === Визуально телепортируем игрока ===
    player->setPosition(mCurrentFakePos);
}