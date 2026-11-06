#pragma once
//
// Created by vastrakai on 7/10/2024.
//
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>
#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>
#include <unordered_map>
#include <Utils/MemUtils.hpp>

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

	// Virtual (vector deleting dtor is slot 0, so this lands on slot 1 in the
	// 1.26.x layout). In the .44 SDK it was a sig-resolved vtable index; the new
	// base never needed it, but ChestStealer/OreMiner do when they get ported.
	class BlockPalette* getBlockPalette()
	{
		return MemUtils::callVirtualFunc<class BlockPalette*>(1, this);
	}

	std::unordered_map<mce::UUID, PlayerListEntry>* getPlayerList();
	class HitResult* getHitResult();
	class SyncedPlayerMovementSettings* getPlayerMovementSettings();
	std::vector<Actor*> getRuntimeActorList();
	LevelData* getLevelData();
};