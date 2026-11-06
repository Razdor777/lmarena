//
//

#pragma once

#include "Event.hpp"
#include <SDK/Minecraft/Actor/Actor.hpp>

class BaseTickEvent : public Event {
public:
    Actor* mActor;

    explicit BaseTickEvent(Actor* actor) : mActor(actor) {}
};

// ==== ported from the 1.21.44 master ====
// Отдельное событие: срабатывает до BaseTickEvent (RobloxCamera на нём
// перехватывает ввод до применения движения).
class BaseTickInitEvent : public Event
{
public:
    Actor* mActor;

    explicit BaseTickInitEvent(Actor* actor) : mActor(actor) {}
};
// ==== end of ported block ====
