#pragma once
#include <string>
#include <Utils/MemUtils.hpp>


//
// Created by vastrakai on 6/25/2024.
//

class GuiData {
public:
    // Re-pinned for 26.62: the whole block moved forward by 0x10 (a new member
    // was inserted before it), so every field below shifted.
    //   1.21.44:  mResolution 0x30, mGuiScale 0x4C
    //   26.62  :  mResolution 0x40, mGuiScale 0x5C
    CLASS_FIELD(glm::vec2, mResolution, 0x40);
    CLASS_FIELD(glm::vec2, mResolutionRounded, 0x48);
    CLASS_FIELD(glm::vec2, mResolutionScaled, 0x50);

    CLASS_FIELD(float, mGuiScale, 0x5C);
    CLASS_FIELD(float, mScalingMultiplier, 0x60);

    void displayClientMessageQueued(const std::string& msg);
    void displayClientMessage(const std::string& msg);
};