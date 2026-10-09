#pragma once
//
// Created by vastrakai on 7/10/2024.
//
#include <Utils/MemUtils.hpp>
#include <SDK/Minecraft/mce.hpp>
#include <unordered_map>
#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>
#include <glm/glm.hpp>

enum class ParticleType : int {
    Bubble = 1,
    Crit = 2,               // Синие криты
    BlockCrack = 3,         // Осколки блока
    Smoke = 4,              // Дым
    Explode = 5,            // Взрыв
    Heart = 12,             // Сердечки
    Flame = 15,             // Огонь
    Lava = 16,              // Брызги лавы
    Portal = 18,            // Фиолетовый портал
    EnchantingTable = 21,   // Символы чародейства
    Totem = 27,             // Искры тотема
    DragonBreath = 35,      // Дыхание дракона
    SonicBoom = 53          // Волна Вардена
};

class PlayerListEntry {
public:
    uint64_t mId;
    mce::UUID mUuid;
    std::string mName, mXUID, mPlatformOnlineId;
    BuildPlatform mBuildPlatform;

    PlayerListEntry() {
        mId = 0;
        mUuid = mce::UUID();
        mName = "";
        mXUID = "";
        mPlatformOnlineId = "";
        mBuildPlatform = BuildPlatform::Unknown;
    }
};

class LevelData
{
public:
    CLASS_FIELD(uint64_t, mTick, OffsetProvider::LevelData_mTick);
};

class BlockPalette {
public:
    virtual ~BlockPalette();
    virtual void getPaletteType();
    virtual void appendBlock(Block const&);
    virtual void getBlock(unsigned int const&);
    virtual void assignBlockNetworkId(Block const&, unsigned long);

    CLASS_FIELD(Level*, mLevel, OffsetProvider::BlockPalette_mLevel);
};

class Level {
public:
    CLASS_FIELD(uintptr_t**, mVfTable, 0x0);

    std::unordered_map<mce::UUID, PlayerListEntry>* getPlayerList();
    class HitResult* getHitResult();
    class SyncedPlayerMovementSettings* getPlayerMovementSettings();
    std::vector<Actor*> getRuntimeActorList();
    LevelData* getLevelData();
    class BlockPalette* getBlockPalette();

    // Наша функция спавна партиклов
    void addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir = glm::vec3(0.f, 0.05f, 0.f), int data = 0);
};