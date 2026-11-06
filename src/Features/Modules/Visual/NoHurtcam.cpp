//
// Created by vastrakai on 7/18/2024.
//

#include "NoHurtcam.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

DEFINE_NOP_PATCH_FUNC(hurtTimePatch, SigManager::CameraComponent_applyRotation, 21);

void NoHurtcam::onEnable()
{
    if (SigManager::CameraComponent_applyRotation) hurtTimePatch(true);
}

void NoHurtcam::onDisable()
{
    if (SigManager::CameraComponent_applyRotation) hurtTimePatch(false);
}
