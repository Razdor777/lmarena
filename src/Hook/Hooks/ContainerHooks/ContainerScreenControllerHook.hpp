#pragma once
#include <Hook/Hook.hpp>
//
// Created by vastrakai on 7/5/2024.
//

#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>
#include <SDK/Minecraft/Inventory/ContainerScreenController.hpp>

class ContainerScreenControllerHook : public Hook {
public:
    ContainerScreenControllerHook() : Hook() {
        mName = "ContainerScreenController::tick";
    }

    static std::unique_ptr<Detour> mDetour;
    static inline bool sScreenActive = false;
    static inline ContainerType sActiveType = ContainerType::None;

    static bool isScreenActive() { return sScreenActive; }

    static uint32_t onContainerTick(class ContainerScreenController *csc);
    void init() override;
};
