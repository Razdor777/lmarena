//
// Created by vastrakai on 11/19/2024.
//
#include "ItemStack.hpp"
#include "NetworkItemStackDescriptor.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/World/Level.hpp>

struct _SampleLol
{
    PAD(0x80);
};

static std::unique_ptr<_SampleLol> _smplData = std::make_unique<_SampleLol>();

ItemStack ItemStack::fromDescriptor(NetworkItemStackDescriptor const& desc)
{
    if (!SigManager::ItemStack_fromDescriptor) {
        return ItemStack();
    }
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || !player->getLevel()) return ItemStack();

    ItemStack out;
    // Legacy signature fallback if SigManager::ItemStack_fromDescriptor is populated
    MemUtils::callFastcall<ItemStack*>((void*)SigManager::ItemStack_fromDescriptor, &out, &desc, _smplData.get(), true);
    return out;
}
