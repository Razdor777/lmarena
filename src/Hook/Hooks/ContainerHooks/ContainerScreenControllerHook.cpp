#include "ContainerScreenControllerHook.hpp"

#include <memory>
#include <atomic>
#include <Features/FeatureManager.hpp>
#include <Features/Events/ContainerScreenTickEvent.hpp>
#include <Features/Events/ContainerSlotHoveredEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

std::unique_ptr<Detour> ContainerScreenControllerHook::mDetour;
std::unique_ptr<Detour> ContainerScreenControllerHook::mHoveredDetour;

uint32_t ContainerScreenControllerHook::onContainerTick(class ContainerScreenController *csc)
{
    auto original = mDetour->getOriginal<&ContainerScreenControllerHook::onContainerTick>();

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        sScreenActive = false;
        sActiveType   = ContainerType::None;
        sHoveredInvSlot = -1;
        return original(csc);
    }

    auto* cmm = player->getContainerManagerModel();
    if (!cmm) {
        sScreenActive = false;
        sActiveType   = ContainerType::None;
        sHoveredInvSlot = -1;
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
        // Screen is gone — the cached hover must not survive into the next one,
        // otherwise the first number press after reopening would move a stale slot.
        sHoveredInvSlot = -1;
    }

    // ContainerScreenTickEvent только для сундука
    if (type == ContainerType::Container) {
        auto holder = nes::make_holder<ContainerScreenTickEvent>(csc);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return original(csc);
}

// _handleSlotHovered(const std::string& collectionName, int slot).
//
// Parameter order was read off the prologue rather than guessed:
//   mov r15d, r8d             ; r8d is an int  -> slot
//   mov rbx, rdx              ; rdx is a pointer
//   cmp [rcx+0F48h], r8d      ; ...compared with the cached slot this+0xF48
//   cmp qword [rdx+18h], 0Fh  ; std::string SSO check -> rdx is &std::string
//
// It is a virtual method, called only through the vtable, and it early-outs when
// neither the slot nor the collection name changed — so the value we cache here
// stays valid for as long as the cursor sits on that slot.
void* ContainerScreenControllerHook::onContainerSlotHovered(class ContainerScreenController* csc,
                                                            const std::string* collectionName, int slot)
{
    auto original = mHoveredDetour->getOriginal<&ContainerScreenControllerHook::onContainerSlotHovered>();

    if (collectionName)
    {
        const std::string& name = *collectionName;

        // Only the player's own collections. A chest slot is deliberately left
        // at -1: in Java you cannot hotkey-swap a chest item either.
        int invSlot = -1;
        if (name == "hotbar_items")         invSlot = slot;       // inventory 0..8
        else if (name == "inventory_items") invSlot = slot + 9;   // inventory 9..35

        if (invSlot < 0 || invSlot > 35) invSlot = -1;
        sHoveredInvSlot = invSlot;

        // A few lines of evidence in the log, so the mapping can be checked
        // without a debugger.
        static std::atomic<int> logged{ 0 };
        if (logged.fetch_add(1) < 8)
            spdlog::info("[JavaInventory] hover '{}' slot {} -> inv {}", name, slot, invSlot);

        auto holder = nes::make_holder<ContainerSlotHoveredEvent>(slot, name);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return original(csc, collectionName, slot);
}

void ContainerScreenControllerHook::init()
{
    auto func = SigManager::ContainerScreenController_tick;
    mDetour = std::make_unique<Detour>("ContainerScreenController::tick",
        reinterpret_cast<void*>(func),
        &ContainerScreenControllerHook::onContainerTick);
    mDetour->enable();

    // Without this one there is no way to know the hovered slot, so a missing
    // signature disables the feature instead of hooking address 0.
    auto hovered = SigManager::ContainerScreenController_onContainerSlotHovered;
    if (hovered)
    {
        mHoveredDetour = std::make_unique<Detour>("ContainerScreenController::_handleSlotHovered",
            reinterpret_cast<void*>(hovered),
            &ContainerScreenControllerHook::onContainerSlotHovered);
        mHoveredDetour->enable();
        spdlog::info("[JavaInventory] hovered-slot hook installed at 0x{:X}", hovered);
    }
    else
    {
        spdlog::warn("[JavaInventory] hovered-slot signature not resolved - hover tracking disabled");
    }
}
