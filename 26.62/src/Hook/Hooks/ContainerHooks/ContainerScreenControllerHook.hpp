#pragma once
#include <Hook/Hook.hpp>
//
// Created by vastrakai on 7/5/2024.
//

#include <atomic>
#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>



class ContainerScreenControllerHook : public Hook {
public:
    ContainerScreenControllerHook() : Hook() {
        mName = "ContainerScreenController::tick";
    }

    static std::unique_ptr<Detour> mDetour;

    // Второй детур того же класса: колбэк, который игра зовёт при смене
    // подсвеченного слота — _handleSlotHovered(collectionName, slot). Только
    // игра знает, где курсор, поэтому без него не сделать «навёл на слот —
    // нажал цифру» (JavaInventory / JavaInventoryHotkeys).
    static std::unique_ptr<Detour> mHoveredDetour;

    static inline bool sScreenActive = false;
    static inline ContainerType sActiveType = ContainerType::None;

    // Индекс слота инвентаря под курсором: 0..8 хотбар, 9..35 основной.
    // -1 — не наш слот (сундук, броня, пустое место или экран закрыт).
    static inline std::atomic<int> sHoveredInvSlot{ -1 };

    static bool isScreenActive() { return sScreenActive; }
    static bool isHoverHookActive() { return mHoveredDetour != nullptr; }

    static uint32_t onContainerTick(class ContainerScreenController *csc);
    static void* onContainerSlotHovered(class ContainerScreenController* csc,
                                        const std::string* collectionName, int slot);
    void init() override;
};

