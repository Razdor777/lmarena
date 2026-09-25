//
// Created by vastrakai on 7/25/2024.
//

#include "ActorModelHook.hpp"

#include <SDK/OffsetProvider.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorPartModel.hpp>
#include <Features/Events/BoneRenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

// [1.26] — сверка с заголовками LeviLamina 26.51 (см. docs/migration-1.26/audit.md):
// [1.26] ОБНОВЛЕНО: ActorAnimationControllerPlayer::applyToPose -> src/mc/world/actor/animation/ActorAnimationControllerPlayer.h:122 — символ есть, но сигнатура изменилась: (ApplyAnimationContext const&, RenderParams&, unordered_map<SkeletalHierarchyIndex, vector<BoneOrientation>>&, float) — тело хука переписывать

std::unique_ptr<Detour> ActorModelHook::mDetour;

void ActorModelHook::onActorModel(uintptr_t a1, uintptr_t a2, uintptr_t a3, float a4, int a5)
{
    auto original = mDetour->getOriginal<&ActorModelHook::onActorModel>();
    original(a1, a2, a3, a4, a5);

    auto ent = *reinterpret_cast<Actor**>(a2 + 0x38);
    if (!ent) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    const auto bone = reinterpret_cast<Bone*>(a3);
    const auto partModel = bone->getActorPartModel();

    auto holder = nes::make_holder<BoneRenderEvent>(bone, partModel, ent);
    gFeatureManager->mDispatcher->trigger(holder);
}

void ActorModelHook::init()
{
    mDetour = std::make_unique<Detour>("ActorAnimationControllerPlayer::applyToPose", reinterpret_cast<void*>(SigManager::ActorAnimationControllerPlayer_applyToPose), &ActorModelHook::onActorModel);
}
