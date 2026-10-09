//
// Created by vastrakai on 7/10/2024.
//

#include "Level.hpp"

#include <windows.h>
#include <libhat.hpp>
#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/Actor/SyncedPlayerMovementSettings.hpp>

std::unordered_map<mce::UUID, PlayerListEntry>* Level::getPlayerList()
{
    static auto vIndex = OffsetProvider::Level_getPlayerList;
    return MemUtils::callVirtualFunc<std::unordered_map<mce::UUID, PlayerListEntry>*>(vIndex, this);
}

HitResult* Level::getHitResult()
{
    static auto vIndex = OffsetProvider::Level_getHitResult;
    return MemUtils::callVirtualFunc<HitResult*>(vIndex, this);
}

SyncedPlayerMovementSettings* Level::getPlayerMovementSettings()
{
    static auto vIndex = OffsetProvider::Level_getPlayerMovementSettings;
    return MemUtils::callVirtualFunc<SyncedPlayerMovementSettings*>(vIndex, this);
}

std::vector<Actor*> Level::getRuntimeActorList()
{
    static auto func = SigManager::Level_getRuntimeActorList;
    std::vector<Actor*> actors;
    MemUtils::callFastcall<void>(func, this, &actors);
    return actors;
}

LevelData* Level::getLevelData()
{
    static auto vIndex = OffsetProvider::Level_getLevelData;
    return MemUtils::callVirtualFunc<LevelData*>(vIndex, this);
}

class BlockPalette* Level::getBlockPalette()
{
    static auto vIndex = OffsetProvider::Level_getBlockPalette;
    return MemUtils::callVirtualFunc<class BlockPalette*>(vIndex, this);
}

void Level::addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir, int data)
{
    static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    static uintptr_t func = base + 0x32A4800; // Оффсет функции sub_1432A4800

    MemUtils::callFastcall<void>(func, this, static_cast<int>(type), &pos, &dir, data);
}