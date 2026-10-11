#pragma once
#include <Utils/MemUtils.hpp>


class LevelRendererPlayer {
public:
    glm::vec3* getCameraPos();
    float* getFovX();
    float* getFovY();
};

class LevelRenderer {
public:
    //CLASS_FIELD(LevelRendererPlayer*, mLevelRendererPlayer, 0x308);
    LevelRendererPlayer* getRendererPlayer();
};