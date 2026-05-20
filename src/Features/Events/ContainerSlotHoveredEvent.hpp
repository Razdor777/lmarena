// src/Features/Events/ContainerSlotHoveredEvent.hpp
#pragma once

#include "Event.hpp"
#include <string>

class ContainerSlotHoveredEvent : public Event {
public:
    int mHoveredSlot = -1;
    std::string mCollectionName;

    ContainerSlotHoveredEvent(int slot, const std::string& collection) 
        : mHoveredSlot(slot), mCollectionName(collection) {}

    int getHoveredSlot() const { return mHoveredSlot; }
    const std::string& getCollectionName() const { return mCollectionName; }
};