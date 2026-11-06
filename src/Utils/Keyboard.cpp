//
//

#include "Keyboard.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/KeyboardRemappingLayout.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>

#include "StringUtils.hpp"

int Keyboard::getKeyId(const std::string& str)
{
    return mKeyMap[StringUtils::toLower(str)];
}

bool Keyboard::isUsingMoveKeys(bool includeSpaceShift)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;

    auto* kbPtr = ClientInstance::get()->getKeyboardSettings();
    if (!kbPtr) return false;
    auto& keyboard = *kbPtr;

    auto moveInput = player->getMoveInputComponent();
    if (!moveInput) return false;
    bool isMoving = moveInput->isUp() || moveInput->isDown() || moveInput->isLeft() || moveInput->isRight();
    if (includeSpaceShift)
        return isMoving || Keyboard::mPressedKeys[keyboard["key.jump"]] || Keyboard::mPressedKeys[keyboard["key.sneak"]];

    return isMoving;
}

bool Keyboard::isStrafing()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return false;

    auto moveInput = player->getMoveInputComponent();
    return moveInput->isLeft() || moveInput->isRight() || moveInput->isDown();
}

// returns the char representation of the key
std::string Keyboard::getKey(int keyId)
{
    for (const auto& [key, value] : mKeyMap)
    {
        if (value == keyId)
        {
            return StringUtils::toUpper(key);
        }
    }

    return "unknown";
}
