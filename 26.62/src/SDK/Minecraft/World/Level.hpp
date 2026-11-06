#pragma once
//
// Created by vastrakai on 7/10/2024.
//
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>
#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>
#include <unordered_map>
#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>
#include <glm/glm.hpp>
#include <Utils/MemUtils.hpp>

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
	uint64_t mId; // This is the ActorUniqueID
	mce::UUID mUuid;
	std::string mName, mXUID, mPlayFabId, mPlatformOnlineId;
	BuildPlatform mBuildPlatform;
	SerializedSkin mSkin;
	glm::vec4 mColor;
	bool mIsTeacher;
	bool mIsHost;
	bool mIsSubClient;

	PlayerListEntry() {
		mId = 0;
		mUuid = mce::UUID();
		mName = "";
		mXUID = "";
		mPlayFabId = "";
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

	// TODO: Correct these fields lol
	/*std::mutex mLegacyBlockStatesWarningMutex;
	std::set<std::pair<int, int>> mLegacyBlockStatesWarningSet;
	std::map<std::string, const BlockLegacy*> mNameLookup;
	std::vector<const Block*> mBlockFromRuntimeId;
	Level* mLevel;*/
	CLASS_FIELD(Level*, mLevel, OffsetProvider::BlockPalette_mLevel);
};

class Level {
public:
	CLASS_FIELD(uintptr_t**, mVfTable, 0x0);
	//CLASS_FIELD(class BlockPalette*, mBlockPalette, 0x1C8);
	///CLASS_FIELD(std::unordered_map<mce::UUID, PlayerListEntry>, pl, 0x1BC8); // class_field doesn't support std::unordered_map lol

	std::unordered_map<mce::UUID, PlayerListEntry>* getPlayerList();
	class HitResult* getHitResult();
	class SyncedPlayerMovementSettings* getPlayerMovementSettings();
	std::vector<Actor*> getRuntimeActorList();
	LevelData* getLevelData();
	class BlockPalette* getBlockPalette();

	// Спавн партиклов.
	// TODO(26.62): оффсет addParticle в Level.cpp снят с 1.21.44 — переснять!
	void addParticle(ParticleType type, const glm::vec3& pos, const glm::vec3& dir = glm::vec3(0.f, 0.05f, 0.f), int data = 0);
};