//
// Created by alteik on 25/09/2024.
//

#include "EditionFaker.hpp"
#include <SDK/SigManager.hpp>
#include <Utils/Buffer.hpp>
#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/Network/ConnectionRequest.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

static uintptr_t func = 0;
static uintptr_t func2 = 0;

void EditionFaker::onInit() {
    func = SigManager::ConnectionRequest_create_DeviceOS;
    func2 = SigManager::ConnectionRequest_create_DefaultInputMode;
}

void EditionFaker::inject()
{
    if (!func || !func2) return;

    if (!mOsPtr) {
        mOsPtr = (int32_t*)AllocateBuffer((void*)func);
    }
    if (!mOsPtr) return;
    *mOsPtr = mOs.as<int>();

    {
        MemUtils::ReadBytes((void*)func2, mOriginalData3, sizeof(mOriginalData3));
        MemUtils::writeBytes(func2, mPatchBytes, sizeof(mPatchBytes));
        int32_t inputMode = mInputMethod.as<int>();
        MemUtils::writeBytes(func2 + 1, &inputMode, sizeof(uint32_t));
    }

    { // os
        MemUtils::ReadBytes((void*)func, mOriginalData, sizeof(mOriginalData));
        MemUtils::writeBytes(func, mPatchBytes2, sizeof(mPatchBytes2));
        int32_t rel = MemUtils::GetRelativeAddress(func + 2, (uintptr_t)mOsPtr);
        MemUtils::writeBytes(func + 2, &rel, sizeof(rel));
    }
}

void EditionFaker::eject() {
    if (func2) {
        MemUtils::writeBytes(func2, mOriginalData3, sizeof(mOriginalData3));
    }
    if (func) {
        MemUtils::writeBytes(func, mOriginalData, sizeof(mOriginalData));
    }
    if (mOsPtr) {
        FreeBuffer(mOsPtr);
        mOsPtr = nullptr;
    }
}

void EditionFaker::spoofEdition() {
    if (!func || !func2 || !mOsPtr) return;

    int32_t InputMode = mInputMethod.as<int>();
    int32_t Os = mOs.as<int>();

    MemUtils::writeBytes(func2 + 1, &InputMode, sizeof(uint32_t));
    *mOsPtr = Os;
}

void EditionFaker::onEnable() {
    inject();
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &EditionFaker::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent, &EditionFaker::onConnectionRequestEvent, nes::event_priority::ABSOLUTE_LAST>(this);
}

void EditionFaker::onDisable() {
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &EditionFaker::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent, &EditionFaker::onConnectionRequestEvent>(this);
    eject();
}

void EditionFaker::onConnectionRequestEvent(ConnectionRequestEvent& event) {
    spoofEdition();
    event.mInputMode = mInputMethod.as<int>();
}

void EditionFaker::onPacketOutEvent(PacketOutEvent& event) {
    if (event.mPacket->getId() == PacketID::PlayerAuthInput) {
        auto paip = event.getPacket<PlayerAuthInputPacket>();
        paip->mInputMode = (InputMode)mInputMethod.as<int>();
    }
}
