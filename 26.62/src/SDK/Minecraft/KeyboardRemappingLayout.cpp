//
// Created by vastrakai on 10/10/2024.
//

#include "KeyboardRemappingLayout.hpp"

int KeyboardRemappingLayout::operator[](const std::string& key)
{
    for (auto& bind : mKeyTypeA) {
        if (bind.mBindName == key)
            return bind.mBindKey[0];
    }

    spdlog::error("Failed to find keybind: {}", key);
    return -1;
}

KeyboardRemappingLayout* ClientInputMappingFactory::getKeyboardRemappingLayout()
{
    return hat::member_at<KeyboardRemappingLayout*>(this, OffsetProvider::ClientInputMappingFactory_mKeyboardRemappingLayout);
}

ClientInputMappingFactory* ClientInputHandler::getMappingFactory()
{
    return hat::member_at<ClientInputMappingFactory*>(this, OffsetProvider::ClientInputHandler_mMappingFactory);
}
