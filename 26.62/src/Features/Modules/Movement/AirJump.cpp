#include "AirJump.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>

void AirJump::onEnable()
{
    mOldSpacePressed = false;
    gFeatureManager->mDispatcher->listen<BaseTickEvent,
        &AirJump::onBaseTickEvent>(this);
}

void AirJump::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,
        &AirJump::onBaseTickEvent>(this);
}

void AirJump::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto moveInput   = player->getMoveInputComponent();
    auto stateVector = player->getStateVectorComponent();
    if (!moveInput || !stateVector) return;

    bool spacePressed = moveInput->mIsJumping;

    // На земле — обычное поведение, просто обновляем флаг
    if (player->isOnGround()) {
        mOldSpacePressed = spacePressed;
        return;
    }

    // ══════════════════════════════════════════════════
    // Прыжок в воздухе разрешён только когда:
    //
    // 1) Пробел ТОЛЬКО ЧТО нажат (edge detection)
    //    → нельзя зажать и лететь вверх
    //
    // 2) Игрок уже ПАДАЕТ (velocity.y <= 0)
    //    → это даёт естественный кулдаун:
    //      прыжок → подъём ~6 тиков → пик → падение
    //      → ТОЛЬКО ТОГДА можно прыгнуть снова
    //    → каждый прыжок = полная дуга как обычный
    //
    // 3) Обнуляем Y velocity перед прыжком
    //    → jumpFromGround() ставит ~0.42
    //    → без обнуления оно СКЛАДЫВАЕТСЯ с текущей
    //    → поэтому раньше прыжки были выше
    // ══════════════════════════════════════════════════

    if (spacePressed && !mOldSpacePressed && stateVector->mVelocity.y <= 0.f)
    {
        // Обнуляем вертикальную скорость → чистый прыжок
        stateVector->mVelocity.y = 0.f;

        // Имитируем прыжок с земли
        player->setOnGround(true);
        player->jumpFromGround();
        player->setOnGround(false);
    }

    mOldSpacePressed = spacePressed;
}