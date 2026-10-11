#pragma once

#include "Event.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>

class BaseTickEvent : public Event {
public:
    Actor* mActor;

    explicit BaseTickEvent(Actor* actor) : mActor(actor) {}
};

class BaseTickInitEvent : public Event
{
public:
    Actor* mActor;

    explicit BaseTickInitEvent(Actor* actor) : mActor(actor) {}
};