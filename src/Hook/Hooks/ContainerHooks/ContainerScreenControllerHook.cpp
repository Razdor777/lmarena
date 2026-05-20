#include "ContainerScreenControllerHook.hpp"

#include <memory>
#include <Features/Events/ContainerScreenTickEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

std::unique_ptr<Detour> ContainerScreenControllerHook::mDetour;

uint32_t ContainerScreenControllerHook::onContainerTick(class ContainerScreenController *csc)
{
    auto original = mDetour->getOriginal<&ContainerScreenControllerHook::onContainerTick>();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        sScreenActive = false;
        sActiveType   = ContainerType::None;
        return original(csc);
    }

    auto* cmm = player->getContainerManagerModel();
    if (!cmm) {
        sScreenActive = false;
        sActiveType   = ContainerType::None;
        return original(csc);
    }

    ContainerType type = cmm->mContainerType;

    // Выставляем флаги для рендера кнопок
    if (type == ContainerType::Container || type == ContainerType::Inventory) {
        sScreenActive = true;
        sActiveType   = type;
    } else {
        sScreenActive = false;
        sActiveType   = ContainerType::None;
    }

    // ContainerScreenTickEvent только для сундука
    if (type == ContainerType::Container) {
        auto holder = nes::make_holder<ContainerScreenTickEvent>(csc);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return original(csc);
}

void ContainerScreenControllerHook::init()
{
    auto func = SigManager::ContainerScreenController_tick;
    mDetour = std::make_unique<Detour>("ContainerScreenController::tick",
        reinterpret_cast<void*>(func),
        &ContainerScreenControllerHook::onContainerTick);
    mDetour->enable();
}