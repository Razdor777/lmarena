//
// Created by vastrakai on 7/5/2024.
//

#include "ContainerScreenController.hpp"

#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/MemUtils.hpp>

void ContainerScreenController::handleAutoPlace(const std::string& name, int slot)
{
    static auto func = SigManager::ContainerScreenController_handleAutoPlace;
    if (!func) return;
    return MemUtils::callFastcall<void>(func, this, 0x7FFFFFFF, name, slot);
}

void ContainerScreenController::_tryBeginTransition(int type, unsigned int flags, const std::string& collection, int slot)
{
    static auto func = SigManager::ContainerScreenController_tryBeginTransition;
    if (!func) return;
    MemUtils::callFastcall<void>(func, this, type, flags, collection, slot);
}

void* ContainerScreenController::_tryExit()
{
    return MemUtils::callVirtualFunc<void*>(OffsetProvider::ContainerScreenController_tryExit, this);
}
