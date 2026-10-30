//
// Created by vastrakai on 6/24/2024.
//

#include "Detour.hpp"

#include <magic_enum.hpp>
#include <MinHook.h>
#include <string>
#include <mutex>
#include <vector>

#include "spdlog/spdlog.h"

namespace
{
    // Журнал перехватов: имя, адрес, создался ли, включён ли.
    // Заполняется при создании Detour, обновляется в enable()/restore().
    std::mutex gDetourMutex;
    std::vector<Detour::Status> gDetours;

    int addEntry(const std::string& name, void* addr)
    {
        std::lock_guard lock(gDetourMutex);
        gDetours.push_back({ name, addr, false, false });
        return static_cast<int>(gDetours.size()) - 1;
    }

    void markCreated(int slot, bool created)
    {
        if (slot < 0) return;
        std::lock_guard lock(gDetourMutex);
        if (static_cast<size_t>(slot) >= gDetours.size()) return;
        gDetours[slot].created = created;
    }

    void markEnabled(int slot, bool enabled)
    {
        if (slot < 0) return;
        std::lock_guard lock(gDetourMutex);
        if (static_cast<size_t>(slot) >= gDetours.size()) return;
        gDetours[slot].enabled = enabled;
    }
}

std::vector<Detour::Status> Detour::snapshot()
{
    std::lock_guard lock(gDetourMutex);
    return gDetours;
}

Detour::~Detour()
{
    if (mFunc)
    {
        MH_DisableHook(mFunc);
        MH_RemoveHook(mFunc);
    }
}

Detour::Detour(const std::string& name, void* addr, void* detour, bool silent)
    : mName(name), mFunc(addr)
{
    this->mFunc = addr;
    this->mName = name;

    mSlot = addEntry(name, addr);

    const MH_STATUS result = MH_CreateHook(mFunc, detour, &mOriginalFunc);
    markCreated(mSlot, result == MH_STATUS::MH_OK);

    if (!silent)
    {
        if (result == MH_STATUS::MH_OK) {
            spdlog::info("Created detour for {} at {}", name, addr);
        } else {
            spdlog::critical("Failed to create detour for {} at {}, error: {}", name, addr, magic_enum::enum_name(result));
        }
    }
}

void Detour::enable(bool silent) const
{
    const MH_STATUS status = MH_EnableHook(mFunc);
    markEnabled(mSlot, status == MH_OK);

    if (silent) return;
    switch (status)
    {
    case MH_OK:
        spdlog::info("Enabled detour for {}", mName);
        break;
    default:
        spdlog::critical("Failed to enable detour for {}", mName);
        break;
    }
}

void Detour::restore() const
{
    if (!this)
    {
        spdlog::critical("Failed to restore detour [this == nullptr]");
        return;
    }
    if (!mFunc)
    {
        spdlog::critical("Failed to restore detour for {} [mFunc == nullptr]", mName);
        return;
    }
    MH_STATUS status = MH_DisableHook(mFunc);
    markEnabled(mSlot, false);
    switch (status)
    {
    case MH_OK:
        spdlog::info("Restored detour for {}", mName);
        break;
    default:
        spdlog::critical("Failed to restore detour for {} [MH_DisableHook returned {}]", mName, magic_enum::enum_name(status));
        break;
    }
}

