//
// Created by vastrakai on 7/3/2024.
//

#include "BlockLegacy.hpp"

#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

#include "BlockSource.hpp"

uint16_t BlockLegacy::getBlockId()
{
    return hat::member_at<uint16_t>(this, OffsetProvider::BlockLegacy_mBlockId);
}

bool BlockLegacy::mayPlaceOn(glm::ivec3 pos)
{
    BlockSource* blockSource = ClientInstance::get()->getBlockSource();
    static auto vIndex = OffsetProvider::BlockLegacy_mayPlaceOn;
    return MemUtils::callVirtualFunc<bool, void*, const glm::ivec3&>(vIndex, this, blockSource, pos);
}

bool BlockLegacy::isAir()
{
    // Legacy block ID 0 is air, BUT many modern Bedrock blocks (campfire,
    // soul campfire, sweet berry bush, etc.) also report blockId == 0 because
    // they don't have a legacy ID.  We therefore also check the block name:
    // if the name is non-empty and is NOT "air" / "minecraft:air",
    // then it is NOT air — it's a real block that just lacks a legacy ID.
    if (getBlockId() != 0) return false;

    // CLASS_FIELD members are accessed through generated property getters.
    // mName is at offset 0xA0, mTileName at 0x28 — both are std::string
    std::string name     = mName;     // calls getmName()
    std::string tileName = mTileName; // calls getmTileName()

    if (name.empty() && tileName.empty()) return true; // no name → treat as air

    // Standard air names
    if (name == "air" || name == "minecraft:air") return true;
    if (tileName == "tile.air" || tileName == "air") return true;

    // The block has a real name but ID 0 → it's NOT air (campfire, etc.)
    return false;
}

template <typename T>
class optional_ref
{
private:
    T* mPtr = nullptr;
};

AABB BlockLegacy::getCollisionShape(Block* block, BlockSource* source, BlockPos pos)
{
    //  virtual class AABB getCollisionShape(class Block const&, class IConstBlockSource const&, class BlockPos const&, class optional_ref<class GetCollisionShapeInterface const>) const;
    static int index = OffsetProvider::BlockLegacy_getCollisionShape;
    return MemUtils::callVirtualFunc<AABB, Block*, BlockSource*, BlockPos, optional_ref<void>>(index, this, block, source, pos, optional_ref<void>());
}
