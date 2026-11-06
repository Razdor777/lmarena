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

// Событие «первый тик актора после установки детура»: RobloxCamera проверяет
// в нём, что все нужные компоненты актора уже существуют. В 26.62 оно пропало
// при переносе событий, а модуль его слушал — ссылка на обработчик не
// компилировалась.
class BaseTickInitEvent : public Event
{
public:
    Actor* mActor;

    explicit BaseTickInitEvent(Actor* actor) : mActor(actor) {}
};
