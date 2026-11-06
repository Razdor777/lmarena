#include "GrabMouseHook.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>

std::unique_ptr<Detour> GrabMouseHook::mDetour = nullptr;

void GrabMouseHook::onGrabMouse(ClientInstance* clientinstance)
{
    auto oFunc = mDetour->getOriginal<&onGrabMouse>();
    
    if (ShitNow)
    {
        return;
    }

    return oFunc(clientinstance);
}

void GrabMouseHook::init()
{
    mDetour = std::make_unique<Detour>("ClientInstance::grabMouse", reinterpret_cast<void*>(ClientInstance::get()->vtable[OffsetProvider::ClientInstance_grabMouse]), &GrabMouseHook::onGrabMouse);
    mDetour->enable();
}
