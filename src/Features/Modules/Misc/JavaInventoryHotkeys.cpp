//
// JavaInventoryHotkeys - Ported from Flarial to Solstice
//

#include "JavaInventoryHotkeys.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/KeyEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/ContainerScreenTickEvent.hpp>
#include <Features/Events/ContainerSlotHoveredEvent.hpp>
#include <Features/Events/ConnectionRequestEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/ContainerScreenController.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void JavaInventoryHotkeys::onEnable() {
    gFeatureManager->mDispatcher->listen<KeyEvent, &JavaInventoryHotkeys::onKeyEvent>(this);
    gFeatureManager->mDispatcher->listen<MouseEvent, &JavaInventoryHotkeys::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<ContainerScreenTickEvent, &JavaInventoryHotkeys::onContainerTickEvent>(this);
    gFeatureManager->mDispatcher->listen<ContainerSlotHoveredEvent, &JavaInventoryHotkeys::onContainerSlotHoveredEvent>(this);
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent, &JavaInventoryHotkeys::onConnectionRequestEvent>(this);
}

void JavaInventoryHotkeys::onDisable() {
    gFeatureManager->mDispatcher->deafen<KeyEvent, &JavaInventoryHotkeys::onKeyEvent>(this);
    gFeatureManager->mDispatcher->deafen<MouseEvent, &JavaInventoryHotkeys::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<ContainerScreenTickEvent, &JavaInventoryHotkeys::onContainerTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<ContainerSlotHoveredEvent, &JavaInventoryHotkeys::onContainerSlotHoveredEvent>(this);
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent, &JavaInventoryHotkeys::onConnectionRequestEvent>(this);
    
    clearQueue();
}

void JavaInventoryHotkeys::onConnectionRequestEvent(ConnectionRequestEvent& event) {
    if (event.mServerAddress) {
        sCurrentServerAddress = *event.mServerAddress;
        spdlog::debug("JavaInventoryHotkeys: Connected to server: {}", sCurrentServerAddress);
    }
}

int JavaInventoryHotkeys::getSlotFromKeybind(int key) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return -1;
    
    auto keyboardSettings = ClientInstance::get()->getKeyboardSettings();
    if (!keyboardSettings) return -1;
    
    // Check hotbar binds (1-9)
    for (int i = 0; i < 9; i++) {
        std::string bindName = "hotbar." + std::to_string(i + 1);
        
        // Search in mKeyTypeA
        for (const auto& bind : keyboardSettings->mKeyTypeA) {
            if (bind.mBindName == bindName) {
                for (int boundKey : bind.mBindKey) {
                    if (boundKey == key) return i;
                }
            }
        }
        
        // Search in mKeyTypeB
        for (const auto& bind : keyboardSettings->mKeyTypeB) {
            if (bind.mBindName == bindName) {
                for (int boundKey : bind.mBindKey) {
                    if (boundKey == key) return i;
                }
            }
        }
    }
    
    return -1;
}

void JavaInventoryHotkeys::onKeyEvent(KeyEvent& event) {
    if (!event.mPressed) return;
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    std::string screenName = ClientInstance::get()->getScreenName();
    if (screenName == "hud_screen" || screenName == "pause_screen") {
        clearQueue();
        return;
    }
    
    // Check if a container is open
    auto containerManager = player->getContainerManagerModel();
    if (!containerManager || containerManager->mContainerType != ContainerType::Container) {
        return;
    }
    
    int targetSlot = getSlotFromKeybind(event.mKey);
    if (targetSlot == -1) return;
    
    if (mCurrentHoveredSlot < 0 || mCurrentCollectionName.empty()) return;
    
    mMoveRequests.push({targetSlot, mCurrentHoveredSlot, mCurrentCollectionName});
    
    // Cancel the event so the game doesn't also process this key
    event.cancel();
}

void JavaInventoryHotkeys::onMouseEvent(MouseEvent& event) {
    // Handle mouse button binds for hotbar (some players bind hotbar to mouse buttons)
    if (event.mButtonData != 1) return; // Only on press
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    std::string screenName = ClientInstance::get()->getScreenName();
    if (screenName == "hud_screen" || screenName == "pause_screen") return;
    
    // Convert mouse button to virtual key code for keybind check
    // Mouse buttons are typically: Left=1, Right=2, Middle=3, X1=4, X2=5
    int mouseKey = -100 + event.mActionButtonId; // Negative values for mouse buttons
    
    int targetSlot = getSlotFromKeybind(mouseKey);
    if (targetSlot == -1) return;
    
    if (mCurrentHoveredSlot < 0 || mCurrentCollectionName.empty()) return;
    
    mMoveRequests.push({targetSlot, mCurrentHoveredSlot, mCurrentCollectionName});
}

void JavaInventoryHotkeys::onContainerSlotHoveredEvent(ContainerSlotHoveredEvent& event) {
    mCurrentHoveredSlot = event.getHoveredSlot();
    mCurrentCollectionName = event.getCollectionName();
}

void JavaInventoryHotkeys::onContainerTickEvent(ContainerScreenTickEvent& event) {
    auto controller = event.mController;
    if (!controller) return;
    
    // Reset queue if container changed
    if (mLastContainer != controller) {
        clearQueue();
        mLastContainer = controller;
    }
    
    processSwapQueue(controller);
}

void JavaInventoryHotkeys::processSwapQueue(ContainerScreenController* controller) {
    while (!mMoveRequests.empty()) {
        auto request = mMoveRequests.front();
        mMoveRequests.pop();
        
        if (!canSwap(request.collectionName)) continue;
        
        // Check for restricted servers
        if (sCurrentServerAddress.find("nethergames") != std::string::npos) {
            if (!mRestricted) {
                ChatUtils::displayClientMessage("§c[JavaHotkeys] §eCan't use on NetherGames!");
                mRestricted = true;
            }
            clearQueue();
            return;
        }
        mRestricted = false;
        
        // Perform the swap using handleAutoPlace
        // First, pick up the item from source slot
        controller->handleAutoPlace(request.collectionName, request.hoveredSlot);
        // Then place it in the target hotbar slot
        controller->handleAutoPlace("hotbar_items", request.destSlot);
        // If there was an item in hotbar, place it back to source
        controller->handleAutoPlace(request.collectionName, request.hoveredSlot);
        
        spdlog::debug("JavaInventoryHotkeys: Swapped {} slot {} with hotbar slot {}", 
            request.collectionName, request.hoveredSlot, request.destSlot);
    }
}

bool JavaInventoryHotkeys::canSwap(const std::string& collectionName) {
    // Check if this is a valid container for swapping
    bool isItemContainer = collectionName.find("_item") != std::string::npos;
    bool isRecipeContainer = collectionName.find("recipe_") != std::string::npos;
    bool isSearchContainerOrBar = collectionName.find("search") != std::string::npos;
    bool isHotbar = collectionName == "hotbar_items";
    bool isInventory = collectionName == "inventory_items";
    
    return isItemContainer || isHotbar || isInventory || (isRecipeContainer && !isSearchContainerOrBar);
}

void JavaInventoryHotkeys::clearQueue() {
    while (!mMoveRequests.empty()) {
        mMoveRequests.pop();
    }
    mCurrentHoveredSlot = -1;
    mCurrentCollectionName.clear();
}