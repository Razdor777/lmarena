#pragma once
//
// Created by vastrakai on 7/5/2024.
//

#include <string>

// ContainerType is defined in ContainerManagerModel.hpp

class ContainerScreenController {
public:
    void handleAutoPlace(const std::string& name, int slot);
    void* _tryExit();
    void _tryBeginTransition(int targetState, unsigned int flags, const std::string& collectionName, int slot);
};