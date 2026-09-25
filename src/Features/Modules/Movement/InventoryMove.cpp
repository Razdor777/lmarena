//
// Created by vastrakai on 7/12/2024.
//

#include "InventoryMove.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/Network/Packets/ContainerClosePacket.hpp>

std::vector<unsigned char> gClrBytes = { 0x90, 0x90 };
DEFINE_PATCH_FUNC(InventoryMove::patchFunc1, SigManager::PlayerMovement_clearInputStateInlined, gClrBytes);
std::vector<unsigned char> gClrBytes2 = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
DEFINE_PATCH_FUNC(InventoryMove::patchFunc2, SigManager::PlayerMovement_clearInputStateInlined2, gClrBytes2);


void InventoryMove::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &InventoryMove::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &InventoryMove::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &InventoryMove::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &InventoryMove::onPacketOutEvent>(this);
    patchFunc(true);
    auto player = ClientInstance::get()->getLocalPlayer();
}

void InventoryMove::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &InventoryMove::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &InventoryMove::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &InventoryMove::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &InventoryMove::onPacketOutEvent>(this);
    patchFunc(false);
    auto player = ClientInstance::get()->getLocalPlayer();
    if (player) player->getMoveInputComponent()->setMoveLocked(false);
}


void InventoryMove::onBaseTickEvent(BaseTickEvent& event)
{
    if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput) return;
    auto player = event.mActor;
    bool isUsingFreecam = player->getFlag<RenderCameraComponent>();
    if (isUsingFreecam)
    {
        patchFunc(false);
        return;
    }
    auto input = player->getMoveInputComponent();
    auto& keyboard = *ClientInstance::get()->getKeyboardSettings();

    bool w = Keyboard::mPressedKeys[keyboard["key.forward"]];
    bool a = Keyboard::mPressedKeys[keyboard["key.left"]];
    bool s = Keyboard::mPressedKeys[keyboard["key.back"]];
    bool d = Keyboard::mPressedKeys[keyboard["key.right"]];
    bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
    bool shift = Keyboard::mPressedKeys[keyboard["key.sneak"]];
    bool pressed = w || a || s || d || space || shift;

    std::string screenName = ClientInstance::get()->getScreenName();

    bool isInChatScreen = screenName == "chat_screen" || isUsingFreecam;
    if (!isInChatScreen) patchFunc(true);
    else patchFunc(false);

    if (isInChatScreen || !pressed)
    {
        input->setForward(false);
        input->setBackward(false);
        input->setLeft(false);
        input->setRight(false);
        input->setJumping(false);
        input->setJumpingCurrentlyDown(false);
        input->setMoveVector(glm::vec2(0.f, 0.f));
        input->setSneakDown(false);
        return;
    }

    if (screenName != "hud_screen") input->setMoveLocked(true);
    else input->setMoveLocked(false);

    input->setForward(w);
    input->setBackward(s);
    input->setLeft(a);
    input->setRight(d);
    if (screenName != "hud_screen") // I hate this game
    {
        input->setJumping(space);
        input->setJumpingCurrentlyDown(space);
    }
    if(mHasOpenContainer)
    {
        input->setSneakDown(mDisallowShift.mValue ? false : shift);
    }
    input->setMoveVector(MathUtils::getMovement());
}

void InventoryMove::onRenderEvent(RenderEvent& event)
{
    if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    bool isUsingFreecam = player->getFlag<RenderCameraComponent>();
    if (isUsingFreecam)
    {
        patchFunc(false);
        return;
    }
    auto input = player->getMoveInputComponent();
    auto& keyboard = *ClientInstance::get()->getKeyboardSettings();

    bool w = Keyboard::mPressedKeys[keyboard["key.forward"]];
    bool a = Keyboard::mPressedKeys[keyboard["key.left"]];
    bool s = Keyboard::mPressedKeys[keyboard["key.back"]];
    bool d = Keyboard::mPressedKeys[keyboard["key.right"]];
    bool space = Keyboard::mPressedKeys[keyboard["key.jump"]];
    bool shift = Keyboard::mPressedKeys[keyboard["key.sneak"]];
    bool pressed = w || a || s || d || space || shift;

    std::string screenName = ClientInstance::get()->getScreenName();

    bool isInChatScreen = screenName == "chat_screen";;
    if (!isInChatScreen) patchFunc(true);
    else patchFunc(false);

    if (isInChatScreen || !pressed)
    {
        input->setForward(false);
        input->setBackward(false);
        input->setLeft(false);
        input->setRight(false);
        input->setJumping(false);
        input->setJumpingCurrentlyDown(false);
        input->setMoveVector(glm::vec2(0.f, 0.f));
        input->setSneakDown(false);
        return;
    }

    if (screenName != "hud_screen") input->setMoveLocked(true);
    else input->setMoveLocked(false);

    input->setForward(w);
    input->setBackward(s);
    input->setLeft(a);
    input->setRight(d);
    if (screenName != "hud_screen") // I hate this game
    {
        input->setJumping(space);
        input->setJumpingCurrentlyDown(space);
    }
    if(mHasOpenContainer)
    {
        input->setSneakDown(mDisallowShift.mValue ? false : shift);
    }
    input->setMoveVector(MathUtils::getMovement());
}

void InventoryMove::onPacketInEvent(PacketInEvent& event)
{
    PacketID pid = event.mPacket->getId();
    // ContainerOpen — есть открытый контейнер
    if (pid == PacketID::ContainerOpen)
        mHasOpenContainer = true;
    // ContainerClose — закрыли
    if (pid == PacketID::ContainerClose)
        mHasOpenContainer = false;
}

void InventoryMove::onPacketOutEvent(PacketOutEvent& event)
{
    PacketID pid = event.mPacket->getId();
    if (pid == PacketID::ContainerClose)
        mHasOpenContainer = false;
}