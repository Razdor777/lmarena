#pragma once
//
// Created by vastrakai on 6/25/2024.
//

#include <Hook/Hook.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

#include <mutex>
#include <string>
#include <vector>

class BaseTickHook : public Hook {
public:
    BaseTickHook() : Hook() {
        mName = "Actor::baseTick";
    }

    static std::unique_ptr<Detour> mDetour;

    static void onBaseTick(class Actor* actor);
    void init() override;

    // Client messages are queued from any thread and printed on the next base tick.
    static inline std::vector<std::string> mQueuedMessages;
    static inline std::mutex mQueueMutex;

    static void queueMsg(const std::string& msg)
    {
        std::lock_guard lock(mQueueMutex);
        mQueuedMessages.push_back(msg);
    }
};
