#pragma once
#include <Utils/MemUtils.hpp>
//
//

class Simulation {
public:
    float mTimer = 20.f;
    PAD(0x4);
    float mDeltaTime = 0.f;
    float mTimerMultiplier = 1.f;
    PAD(0x8);
    float mTime = 1.f;
};

class MinecraftSim {
public:
    Simulation* getGameSim();
    Simulation* getRenderSim();
    void setSimTimer(float timer);
    void setSimSpeed(float speed);
    float getSimTimer();
    float getSimSpeed();
    class GameSession* getGameSession();
};

