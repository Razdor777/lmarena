#pragma once
#include <Hook/Hook.hpp>
//
// Created by vastrakai on 7/5/2024.
//

#include <atomic>
#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>
#include <SDK/Minecraft/Inventory/ContainerScreenController.hpp>

class ContainerScreenControllerHook : public Hook {
public:
    ContainerScreenControllerHook() : Hook() {
        mName = "ContainerScreenController::tick";
    }

    static std::unique_ptr<Detour> mDetour;

    // Second detour on the same class: the callback the game calls whenever the
    // hovered slot changes — _handleSlotHovered(collectionName, slot). The game
    // is the only one who knows where the cursor is, so this is what makes
    // Java-style "hover a slot, press a number" swapping possible.
    static std::unique_ptr<Detour> mHoveredDetour;

    static inline bool sScreenActive = false;
    static inline ContainerType sActiveType = ContainerType::None;

    // Inventory-container index of the hovered slot: 0..8 hotbar, 9..35 main
    // inventory. -1 means "not one of our slots" — a chest row, an armour slot,
    // empty space, or the screen not being open at all.
    static inline std::atomic<int> sHoveredInvSlot{ -1 };

    static bool isScreenActive() { return sScreenActive; }
    static bool isHoverHookActive() { return mHoveredDetour != nullptr; }

    static uint32_t onContainerTick(class ContainerScreenController *csc);
    static void* onContainerSlotHovered(class ContainerScreenController* csc,
                                        const std::string* collectionName, int slot);
    void init() override;
};
