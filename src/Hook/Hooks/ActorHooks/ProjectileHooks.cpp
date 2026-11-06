#include "ProjectileHooks.hpp"
#include <Features/Events/ProjectileHitEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <SDK/SigManager.hpp>

std::unique_ptr<Detour> ProjectileHooks::mHitEntityDetour;
std::unique_ptr<Detour> ProjectileHooks::mHitBlockDetour;

void ProjectileHooks::onHitEntity(void* _this, Actor* owner, Actor* hitEntity)
{
    auto original = mHitEntityDetour->getOriginal<&ProjectileHooks::onHitEntity>();
    original(_this, owner, hitEntity);

    if (owner && hitEntity && gFeatureManager && gFeatureManager->mDispatcher)
    {
        glm::vec3 hitPos = glm::vec3(0);
        if (auto p = hitEntity->getPos())
            hitPos = *p;

        auto holder = nes::make_holder<ProjectileHitEvent>(owner, hitEntity, hitPos, false);
        gFeatureManager->mDispatcher->trigger(holder);
    }
}

void ProjectileHooks::onHitBlock(void* _this, Actor* owner, glm::vec3* hitPos, int side, bool b)
{
    auto original = mHitBlockDetour->getOriginal<&ProjectileHooks::onHitBlock>();
    original(_this, owner, hitPos, side, b);

    if (owner && hitPos && gFeatureManager && gFeatureManager->mDispatcher)
    {
        auto holder = nes::make_holder<ProjectileHitEvent>(owner, nullptr, *hitPos, true);
        gFeatureManager->mDispatcher->trigger(holder);
    }
}

void ProjectileHooks::init()
{
    if (SigManager::Projectile_hitEntity)
    {
        mHitEntityDetour = std::make_unique<Detour>("ProjectileComponent::onHitEntity", reinterpret_cast<void*>(SigManager::Projectile_hitEntity), &ProjectileHooks::onHitEntity);
        mHitEntityDetour->enable();
    }
    if (SigManager::Projectile_hitBlock)
    {
        mHitBlockDetour = std::make_unique<Detour>("ProjectileComponent::onHitBlock", reinterpret_cast<void*>(SigManager::Projectile_hitBlock), &ProjectileHooks::onHitBlock);
        mHitBlockDetour->enable();
    }
}
