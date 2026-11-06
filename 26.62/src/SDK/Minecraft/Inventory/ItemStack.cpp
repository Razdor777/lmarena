//
// Created by vastrakai on 11/19/2024.
//
#include "ItemStack.hpp"

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>

struct _SampleLol
{
    PAD(0x80);
};

static std::unique_ptr<_SampleLol> _smplData = std::make_unique<_SampleLol>();
// 1.21.44 SDK, unchanged logic: the game builds a stack from a network
// descriptor given the block palette. Needs level + local player, so it is a
// .cpp function and not a header inline.
ItemStack ItemStack::fromDescriptor(class NetworkItemStackDescriptor const& desc)
{
    static auto func = SigManager::ItemStack_fromDescriptor;

    auto palette = ClientInstance::get()->getLocalPlayer()->getLevel()->getBlockPalette();

    ItemStack item = ItemStack();
    MemUtils::callFastcall<ItemStack*, ItemStack*, NetworkItemStackDescriptor const&, BlockPalette*, bool>(
        func, &item, desc, palette, true);
    return item;
}
