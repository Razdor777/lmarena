#pragma once

#include "Event.hpp"

class SetupAndRenderEvent : public Event {
public:
    void* mcuirc;
    explicit SetupAndRenderEvent(void* mcuirc) : Event(), mcuirc(mcuirc) {}
};
