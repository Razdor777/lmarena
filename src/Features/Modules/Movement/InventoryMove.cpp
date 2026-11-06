//
// Created by vastrakai on 7/12/2024.
//

#include "InventoryMove.hpp"

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/KeyboardRemappingLayout.hpp>
#include <SDK/Minecraft/Network/Packets/ContainerClosePacket.hpp>
#include <SDK/SigManager.hpp>

void InventoryMove::Patch() {
    if (Patched) {
        return;
    }
    if (!SigManager::PlayerMovement_clearInputStateInlined ||
        !SigManager::PlayerMovement_clearInputStateInlined2 ||
        !SigManager::PlayerMovement_clearInputStateInlined3) {
        return;
    }
    Patched = true;

    std::vector<unsigned char> Bytes = { 0x90, 0x90 };
    std::vector<unsigned char> Bytes2 = { 0x81 };
    std::vector<unsigned char> Bytes3 = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

    OriginalBytes = MemUtils::readBytes(SigManager::PlayerMovement_clearInputStateInlined, Bytes.size());
    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined, Bytes);
    OriginalBytes2 = MemUtils::readBytes(SigManager::PlayerMovement_clearInputStateInlined2 + 1, Bytes2.size());
    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined2 + 1, Bytes2);
    OriginalBytes3 = MemUtils::readBytes(SigManager::PlayerMovement_clearInputStateInlined3, Bytes3.size());
    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined3, Bytes3);
}

void InventoryMove::UnPatch() {
    if (!Patched) {
        return;
    }
    Patched = false;

    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined, OriginalBytes);
    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined2 + 1, OriginalBytes2);
    MemUtils::writeBytes(SigManager::PlayerMovement_clearInputStateInlined3, OriginalBytes3);
}

void InventoryMove::onEnable()
{
    Patch();
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &InventoryMove::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &InventoryMove::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &InventoryMove::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &InventoryMove::onPacketOutEvent>(this);
}

void InventoryMove::onDisable()
{
    UnPatch();

    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &InventoryMove::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &InventoryMove::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &InventoryMove::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &InventoryMove::onPacketOutEvent>(this);
}

void InventoryMove::onBaseTickEvent(BaseTickEvent& event)
{
    if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput) return;
    auto player = event.mActor;
    if (!player) return;

    auto input = player->getMoveInputComponent();
    if (!input) return;
    auto* kbPtr = ClientInstance::get()->getKeyboardSettings();
    if (!kbPtr) return;
    auto& keyboard = *kbPtr;

    bool w = Keyboard::mPressedKeys[keyboard["key.forward"]];
    bool a = Keyboard::mPressedKeys[keyboard["key.left"]];
    bool s = Keyboard::mPressedKeys[keyboard["key.back"]];
    bool d = Keyboard::mPressedKeys[keyboard["key.right"]];
    bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
    bool shift = Keyboard::mPressedKeys[keyboard["key.sneak"]];
    bool pressed = w || a || s || d || space || shift;

    std::string screenName = ClientInstance::get()->getScreenName();

    bool isInChatScreen = screenName == "chat_screen";
    if (isInChatScreen) return;

    if (!pressed)
    {
        input->setUp(false);
        input->setDown(false);
        input->setLeft(false);
        input->setRight(false);
        input->setJumping(false);
        input->setSneaking(false);
        input->mMove = glm::vec2();
        input->mInputState.AnalogMoveVector = glm::vec2();
        input->mRawInputState.AnalogMoveVector = glm::vec2();
        return;
    }

    input->setUp(w);
    input->setDown(s);
    input->setLeft(a);
    input->setRight(d);
    input->setJumping(space);
    input->setSneaking(shift && !mDisallowShift.mValue);
    input->mMove = MathUtils::getMovement();
    input->mInputState.AnalogMoveVector = MathUtils::getMovement();
    input->mRawInputState.AnalogMoveVector = MathUtils::getMovement();
}

void InventoryMove::onRenderEvent(RenderEvent& event)
{
    if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto input = player->getMoveInputComponent();
    if (!input) return;
    auto* kbPtr = ClientInstance::get()->getKeyboardSettings();
    if (!kbPtr) return;
    auto& keyboard = *kbPtr;

    bool w = Keyboard::mPressedKeys[keyboard["key.forward"]];
    bool a = Keyboard::mPressedKeys[keyboard["key.left"]];
    bool s = Keyboard::mPressedKeys[keyboard["key.back"]];
    bool d = Keyboard::mPressedKeys[keyboard["key.right"]];
    bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
    bool shift = Keyboard::mPressedKeys[keyboard["key.sneak"]];
    bool pressed = w || a || s || d || space || shift;

    std::string screenName = ClientInstance::get()->getScreenName();

    bool isInChatScreen = screenName == "chat_screen";
    if (isInChatScreen) return;

    if (!pressed)
    {
        input->setUp(false);
        input->setDown(false);
        input->setLeft(false);
        input->setRight(false);
        input->setJumping(false);
        input->setSneaking(false);
        input->mMove = glm::vec2();
        input->mInputState.AnalogMoveVector = glm::vec2();
        input->mRawInputState.AnalogMoveVector = glm::vec2();
        return;
    }

    input->setUp(w);
    input->setDown(s);
    input->setLeft(a);
    input->setRight(d);
    input->setJumping(space);
    input->setSneaking(shift && !mDisallowShift.mValue);
    input->mMove = MathUtils::getMovement();
    input->mInputState.AnalogMoveVector = MathUtils::getMovement();
    input->mRawInputState.AnalogMoveVector = MathUtils::getMovement();
}

void InventoryMove::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() == PacketID::ContainerOpen)
    {
        mHasOpenContainer = true;
    }
    if (event.mPacket->getId() == PacketID::ContainerClose)
    {
        mHasOpenContainer = false;
    }
}

void InventoryMove::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() == PacketID::ContainerClose)
    {
        mHasOpenContainer = false;
    }
    else if (event.mPacket->getId() == PacketID::ContainerOpen)
    {
        mHasOpenContainer = true;
    }
}
