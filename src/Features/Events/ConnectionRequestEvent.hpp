//
// Created by vastrakai on 7/3/2024.
//

#pragma once

#include "Event.hpp"

class SerializedSkinImpl;

class ConnectionRequestEvent : public Event
{
public:
    std::string mSelfSignedId;
    std::string mServerAddress;
    int64_t mClientRandomId;
    std::string mSkinId;
    std::string mDeviceId;
    int8_t mInputMode;
    SerializedSkinImpl* mSkin = nullptr;

    explicit ConnectionRequestEvent(
        std::string selfsing,
        std::string serveraddr,
        __int64 clientid,
        std::string skinid,
        std::string devid,
        int8_t inputmode,
        SerializedSkinImpl* skin = nullptr)
        : mSelfSignedId(std::move(selfsing))
        , mServerAddress(std::move(serveraddr))
        , mClientRandomId(clientid)
        , mSkinId(std::move(skinid))
        , mDeviceId(std::move(devid))
        , mInputMode(inputmode)
        , mSkin(skin)
    {}
};
