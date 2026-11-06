#pragma once
//
// Created by vastrakai on 7/5/2024.
//

#include <Utils/MemUtils.hpp>
#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>

class Actor;

class GameMode {
public:
    CLASS_FIELD(class Actor*, mPlayer, 0x8);
    CLASS_FIELD(float, mBreakProgress, 0x24);

    bool startDestroyBlock(glm::ivec3 pos, unsigned char face, bool hasDestroyedBlock)
    {
        return MemUtils::callVirtualFunc<bool, glm::ivec3, unsigned char, bool&>(1, this, pos, face, hasDestroyedBlock);
    }
    bool destroyBlock(glm::ivec3 pos, unsigned char face)
    {
        return MemUtils::callVirtualFunc<bool, glm::ivec3, unsigned char>(2, this, pos, face);
    }
    bool stopDestroyBlock(glm::ivec3 pos)
    {
        return MemUtils::callVirtualFunc<bool, glm::ivec3>(4, this, pos);
    }
    bool buildBlock(glm::ivec3 pos, unsigned char face, unsigned char slot /*HAND SLOT*/, bool issimtick)
    {
        return MemUtils::callVirtualFunc<bool, glm::ivec3, unsigned char, unsigned char, bool>(OffsetProvider::GameMode_buildBlock, this, pos, face, slot, issimtick);
    }
    bool buildBlock(glm::ivec3 pos, int face, bool issimtick)
    {
        return buildBlock(pos, static_cast<unsigned char>(face), 0, issimtick);
    }
    bool interact(Actor* Entity, glm::vec3 Location, unsigned char slot = 0 /*HAND SLOT*/)
    {
        return MemUtils::callVirtualFunc<bool, Actor*, glm::vec3, unsigned char>(15, this, Entity, Location, slot);
    }
    bool attack(Actor& Entity, glm::vec3 AttackPos = glm::vec3())
    {
        return MemUtils::callVirtualFunc<bool, Actor&, glm::vec3>(15, this, Entity, AttackPos);
    }
    bool attack(Actor* Entity, glm::vec3 AttackPos = glm::vec3())
    {
        if (!Entity) return false;
        return attack(*Entity, AttackPos);
    }
    void releaseUsingItem()
    {
        MemUtils::callVirtualFunc<void>(16, this);
    }

    float getDestroyRate(const class Block& block);
    bool baseUseItem(class ItemStack* itemStack);
};
