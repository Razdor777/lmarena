//
// Created by Solar on 11/18/2024.
//

#include "Goofy.hpp"

#include <Features/Events/BoneRenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/ActorPartModel.hpp>

void Goofy::onEnable()
{
    gFeatureManager->mDispatcher->listen<BoneRenderEvent, &Goofy::onBoneRenderEvent>(this);
}

void Goofy::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BoneRenderEvent, &Goofy::onBoneRenderEvent>(this);
}

void Goofy::onBoneRenderEvent(BoneRenderEvent& event)
{
    if (!event.mBones) return;
    auto ent = event.mActor;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || ent != player) return;
    if (ClientInstance::get()->getMouseGrabbed()) return;

    for (auto& [key, boneVec] : *event.mBones)
    {
        for (auto& bone : boneVec)
        {
            if (bone.mBoneStr == "rightarm" || bone.mBoneStr == "leftarm")
            {
                auto* partModel = bone.getActorPartModel();
                if (!partModel) continue;

                if (auto* walk = player->getWalkAnimationComponent()) {
                    walk->mWalkAnimSpeed = 0.79f;
                }

                if (Keyboard::isUsingMoveKeys() && player->getSwingProgress() == 0)
                {
                    partModel->mRot.z *= 3.f;
                    partModel->mRot.x *= 1.4f;
                }

                if (Keyboard::isUsingMoveKeys())
                {
                    float mAngle = bone.mBoneStr == "rightarm" ? 10.38f : -10.38f;
                    partModel->mRot.z = mAngle;
                }
            }
        }
    }
}
