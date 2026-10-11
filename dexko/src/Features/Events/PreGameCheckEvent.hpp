#pragma once

#include "Event.hpp"

class PreGameCheckEvent : public Event
{
public:
    bool mIsPreGame = false;
    bool mOverride = false;

    [[nodiscard]] bool isPreGame() const
    {
        return mIsPreGame;
    }

    void setPreGame(bool isPreGame)
    {
        mIsPreGame = isPreGame;
        mOverride = true;
    }

    explicit PreGameCheckEvent(bool isPreGame) : mIsPreGame(isPreGame)
    {

    }
};
