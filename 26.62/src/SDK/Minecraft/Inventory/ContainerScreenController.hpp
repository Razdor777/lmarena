#pragma once
//
// Created by vastrakai on 7/5/2024.
//

#include <string>

class ContainerScreenController {
public:
    void handleAutoPlace(const std::string& name, int slot);
    void* _tryExit();
    // Restored from the 1.21.44 tree (used by ContainerActions sorting).
    void _tryBeginTransition(int targetState, unsigned int flags, const std::string& collectionName, int slot);
};