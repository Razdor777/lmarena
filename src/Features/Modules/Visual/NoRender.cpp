//
// Created by vastrakai on 11/14/2024.
//

#include "NoRender.hpp"

std::vector<unsigned char> gRetOvrPatch = { 0xC3, 0x90 };
DEFINE_PATCH_FUNC(NoRender::patchOverlay, SigManager::Unknown_renderBlockOverlay, gRetOvrPatch);

std::vector<unsigned char> gFrBytes = { 0xC3 };
DEFINE_PATCH_FUNC(NoRender::patchFireRender, SigManager::FireRender, gFrBytes);

void NoRender::onEnable()
{
    if (SigManager::Unknown_renderBlockOverlay) {
        patchOverlay(mNoBlockOverlay.mValue);
    }
    if (SigManager::FireRender) {
        patchFireRender(mNoFire.mValue);
    }
}

void NoRender::onDisable()
{
    if (SigManager::Unknown_renderBlockOverlay) {
        patchOverlay(false);
    }
    if (SigManager::FireRender) {
        patchFireRender(false);
    }
}
