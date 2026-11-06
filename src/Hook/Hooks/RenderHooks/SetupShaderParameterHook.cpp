#include "SetupShaderParameterHook.hpp"

#include <Features/Events/HurtColorEvent.hpp>
#include <Features/Events/SetupShaderParameterEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

std::unique_ptr<Detour> SetupShaderParameterHook::mDetour;

void SetupShaderParameterHook::setupShaderParameters(
    ScreenContext* screenContext,
    BaseActorRenderContext* entityContext,
    Actor* entity,
    glm::vec4* overlay,
    glm::vec4* changeColor,
    glm::vec4* changeColor2,
    glm::vec4* glintColor,
    int uvOffset1,
    int uvOffset2,
    float uvRot1,
    float uvRot2,
    void** glintUVScale,
    void* uvAnim,
    float br,
    unsigned char lightEmission)
{
    auto oFunc = mDetour->getOriginal<&setupShaderParameters>();

    auto localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer)
    {
        oFunc(screenContext, entityContext, entity, overlay, changeColor, changeColor2, glintColor, uvOffset1, uvOffset2, uvRot1, uvRot2, glintUVScale, uvAnim, br, lightEmission);
        return;
    }

    // Native 26.62 HitColor support via ActorShaderManager overlay parameter
    if (overlay && overlay->w > 0.001f)
    {
        auto hurtHolder = nes::make_holder<HurtColorEvent>(&overlay->x);
        gFeatureManager->mDispatcher->trigger(hurtHolder);
    }

    auto holder = nes::make_holder<SetupShaderParameterEvent>(
        screenContext, entityContext, entity, overlay, changeColor, changeColor2, glintColor,
        uvOffset1, uvOffset2, uvRot1, uvRot2, glintUVScale, uvAnim, br, lightEmission);
    gFeatureManager->mDispatcher->trigger(holder);

    oFunc(screenContext, entityContext, entity, overlay, changeColor, changeColor2, glintColor, uvOffset1, uvOffset2, uvRot1, uvRot2, glintUVScale, uvAnim, br, lightEmission);
}

void SetupShaderParameterHook::init()
{
    mDetour = std::make_unique<Detour>("ActorShaderManager::setupShaderParameters", reinterpret_cast<void*>(SigManager::ActorShaderManager_setupShaderParameters), &SetupShaderParameterHook::setupShaderParameters);
}
