#include "CustomFog.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

void CustomFog::onEnable() {
    gFeatureManager->mDispatcher->listen<RenderEvent, &CustomFog::onRenderEvent>(this);
}

void CustomFog::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &CustomFog::onRenderEvent>(this);
    sNoFog = false;
    sFogColorR = 0.0f;
    sFogColorG = 0.0f;
    sFogColorB = 0.0f;
    sFogColorA = 1.0f;
}

void CustomFog::onRenderEvent(RenderEvent& event) {
    auto ci = ClientInstance::get();
    if (!ci) return;
    auto player = ci->getLocalPlayer();
    if (!player) return;

    float r = std::clamp(mRed.mValue   / 255.0f, 0.0f, 1.0f);
    float g = std::clamp(mGreen.mValue / 255.0f, 0.0f, 1.0f);
    float b = std::clamp(mBlue.mValue  / 255.0f, 0.0f, 1.0f);

    switch (mMode.mValue) {
        case FogMode::NoFog:
            sNoFog = true;
            sFogColorR = 0.f;
            sFogColorG = 0.f;
            sFogColorB = 0.f;
            sFogColorA = 0.f;
            break;

        case FogMode::ColorOnly:
            sNoFog = false;
            sFogColorR = r;
            sFogColorG = g;
            sFogColorB = b;
            sFogColorA = 1.0f;
            break;

        case FogMode::SilentHill:
            sNoFog = false;
            sFogColorR = r;
            sFogColorG = g;
            sFogColorB = b;
            sFogColorA = 1.35f; // alpha > 1.0 = плотный туман
            break;
    }
}