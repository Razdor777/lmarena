#pragma once

#include "Event.hpp"

class KeyEvent : public CancelableEvent {
public:
    int mKey;
    bool mPressed;

    explicit KeyEvent(int key, bool pressed) : mKey(key), mPressed(pressed) {}
};