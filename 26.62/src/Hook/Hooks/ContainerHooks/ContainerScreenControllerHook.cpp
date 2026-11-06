//
// Created by vastrakai on 7/5/2024.
//

#include "ContainerScreenControllerHook.hpp"

#include <memory>
#include <atomic>
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
    if (!player || !player->getContainerManagerModel())
    {
        sScreenActive = false;
        sActiveType = ContainerType::None;
        sHoveredInvSlot = -1;
        return original(csc);
    }

    auto* cmm = player->getContainerManagerModel();
    ContainerType type = cmm->mContainerType;

    // Экран открыт на нашем инвентаре (или сундуке) — запоминаем для рендера
    // кнопок и для JavaInventory.
    if (type == ContainerType::Container || type == ContainerType::Inventory)
    {
        sScreenActive = true;
        sActiveType = type;
    }
    else
    {
        sScreenActive = false;
        sActiveType = ContainerType::None;
        // Экран ушёл — кэш ховера не должен пережить его, иначе первое же
        // нажатие цифры после открытия двинуло бы устаревший слот.
        sHoveredInvSlot = -1;
    }

    // ContainerScreenTickEvent — только для сундука (как и раньше)
    if (type == ContainerType::Container)
    {
        auto holder = nes::make_holder<ContainerScreenTickEvent>(csc);
        gFeatureManager->mDispatcher->trigger(holder);
    }

    return original(csc);
}

// _handleSlotHovered(const std::string& collectionName, int slot).
//
// Порядок аргументов снят с пролога, а не угадан:
//   mov r15d, r8d             ; r8d — int -> slot
//   mov rbx, rdx              ; rdx — указатель
//   cmp [rcx+0F48h], r8d      ; сравнивается с кэшем слота this+0xF48
//   cmp qword [rdx+18h], 0Fh  ; SSO-проверка std::string -> rdx это &std::string
//
// Метод виртуальный, вызывается только через vtable и выходит рано, если не
// изменился ни слот, ни имя коллекции, — поэтому закэшированное значение
// остаётся валидным, пока курсор стоит на месте.
void* ContainerScreenControllerHook::onContainerSlotHovered(class ContainerScreenController* csc,
                                                            const std::string* collectionName, int slot)
{
    auto original = mHoveredDetour->getOriginal<&ContainerScreenControllerHook::onContainerSlotHovered>();

    if (collectionName)
    {
        const std::string& name = *collectionName;

        // Только собственные коллекции игрока. Слот сундука остаётся -1: в Java
        // предмет из сундука хоткеем тоже не перекинуть.
        int invSlot = -1;
        if (name == "hotbar_items")         invSlot = slot;       // инвентарь 0..8
        else if (name == "inventory_items") invSlot = slot + 9;   // инвентарь 9..35

        if (invSlot < 0 || invSlot > 35) invSlot = -1;
        sHoveredInvSlot = invSlot;

        // Пара строк в лог, чтобы соответствие слотов можно было проверить без
        // отладчика.
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
    mDetour = std::make_unique<Detour>("ContainerScreenController::tick", reinterpret_cast<void*>(func), &ContainerScreenControllerHook::onContainerTick);
    mDetour->enable();

    // Без этой сигнатуры слот под курсором узнать нечем — лучше выключить
    // функцию, чем ставить детур на адрес 0.
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