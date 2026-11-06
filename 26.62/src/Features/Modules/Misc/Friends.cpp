//
// Created by vastrakai on 7/12/2024.
//

#include "Friends.hpp"

#include <Dexko.hpp>
#include <Features/Modules/Player/Teams.hpp>

void Friends::onInit()
{
    mFriends = Dexko::Prefs->mFriends;
}

bool Friends::isFriend(const std::string& name)
{
    /*if (std::find(mFriends.begin(), mFriends.end(), name) != mFriends.end())
        return true;*/
    return StringUtils::containsAnyIgnoreCase(name, mFriends);
}

bool Friends::isFriend(Actor* actor)
{
    return isFriend(actor->getRawName()) || Teams::instance->isOnTeam(actor);
}

void Friends::addFriend(const std::string& name)
{
    mFriends.push_back(name);
    Dexko::Prefs->mFriends = mFriends;
    PreferenceManager::save(Dexko::Prefs);
}

void Friends::addFriend(Actor* actor)
{
    addFriend(actor->getRawName());
}

void Friends::removeFriend(const std::string& name)
{
    std::erase(mFriends, name);
    Dexko::Prefs->mFriends = mFriends;
    PreferenceManager::save(Dexko::Prefs);
}

void Friends::removeFriend(Actor* actor)
{
    removeFriend(actor->getRawName());
}

void Friends::clearFriends()
{
    mFriends.clear();
    Dexko::Prefs->mFriends = mFriends;
    PreferenceManager::save(Dexko::Prefs);
}
