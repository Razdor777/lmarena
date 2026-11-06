//
// HurtColorEvent - Fired when the game calculates the hurt overlay color
// Listeners can modify the color to customize the hurt flash
//

#pragma once

#include "Event.hpp"

class HurtColorEvent : public Event {
public:
    // Pointer to the MCCColor returned by RenderController::getOverlayColor
    // Modify these floats to change the hurt color in-game
    float* mColor; // float[4]: R, G, B, A

    HurtColorEvent(float* color) : Event(), mColor(color) {}

    void setColor(float r, float g, float b, float a) {
        if (mColor) {
            mColor[0] = r;
            mColor[1] = g;
            mColor[2] = b;
            mColor[3] = a;
        }
    }
};
