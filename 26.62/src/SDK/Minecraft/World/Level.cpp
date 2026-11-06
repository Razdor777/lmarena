//
// Created by vastrakai on 7/10/2024.
//

#include "Level.hpp"

#include <windows.h>
#include <SDK/heap_rd.hpp>
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
    /*
    auto unti = hat::member_at<std::shared_ptr<HitResult>>(this, OffsetProvider::Level_mHitResult);
    return &*unti;*/
    
    static auto vIndex = 335;
    return MemUtils::callVirtualFunc<HitResult*>(vIndex, this);
}

SyncedPlayerMovementSettings* Level::getPlayerMovementSettings()
{
    static auto vIndex = OffsetProvider::Level_getPlayerMovementSettings;
    return MemUtils::callVirtualFunc<SyncedPlayerMovementSettings*>(vIndex, this);
}

std::vector<Actor*> Level::getRuntimeActorList()
{
    static auto vIndex = OffsetProvider::Level_getRuntimeActorList;
    std::vector<Actor*> actors;
    MemUtils::callVirtualFunc<std::vector<Actor*>*>(vIndex, this, &actors);
    return actors;

    /*
    //static auto func = SigManager::Level_getRuntimeActorList;
    std::vector<Actor*> actors;
   // MemUtils::callFastcall<void>(func, this, &actors);
    return actors;*/
}

LevelData* Level::getLevelData()
{
    // TODO(26.62): у авторов новой базы этот вызов закомментирован — если будут
    // проблемы, закомментируй return тоже (как в их версии).
    return hat::member_at<LevelData*>(this, OffsetProvider::Level_mLevelData);
}

class BlockPalette* Level::getBlockPalette()
{
    static auto vIndex = OffsetProvider::Level_getBlockPalette;
    return MemUtils::callVirtualFunc<class BlockPalette*>(vIndex, this);
}

// Спавн партиклов.
// TODO(26.62): оффсет функции sub_1432A4800 снят с 1.21.44 — ОБЯЗАТЕЛЬНО переснять
// с нового Minecraft.Windows.exe (26.62), иначе вызов уйдёт по неверному адресу!
void Level::addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir, int data)
{
    static uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    static uintptr_t func = base + 0x32A4800; // sub_1432A4800 (снят с 1.21.44)

    MemUtils::callFastcall<void>(func, this, static_cast<int>(type), &pos, &dir, data);
}
