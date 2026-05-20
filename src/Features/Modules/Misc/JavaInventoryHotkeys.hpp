#pragma once
//
// JavaInventoryHotkeys - Allows to swap items in your inventory java-like
// Ported from Flarial to Solstice
//

#include <Features/Modules/Module.hpp>
#include <queue>

struct MoveRequest {
    int64_t destSlot;
    int64_t hoveredSlot;
    std::string collectionName;
};

class JavaInventoryHotkeys : public ModuleBase<JavaInventoryHotkeys> {
private:
    int64_t mCurrentHoveredSlot = -1;
    std::string mCurrentCollectionName;
    std::queue<MoveRequest> mMoveRequests;
    void* mLastContainer = nullptr;
    
    // Server address for restriction check
    static inline std::string sCurrentServerAddress;
    bool mRestricted = false;

public:
    JavaInventoryHotkeys() : ModuleBase("JavaInventoryHotkeys", 
        "Allows to swap items in your inventory java-like using hotbar keys",
        ModuleCategory::Misc, 0, false) 
    {
        mNames = {
            {Lowercase, "javainventoryhotkeys"},
            {LowercaseSpaced, "java inventory hotkeys"},
            {Normal, "JavaInventoryHotkeys"},
            {NormalSpaced, "Java Inventory Hotkeys"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    
    void onKeyEvent(class KeyEvent& event);
    void onMouseEvent(class MouseEvent& event);
    void onContainerTickEvent(class ContainerScreenTickEvent& event);
    void onContainerSlotHoveredEvent(class ContainerSlotHoveredEvent& event);
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);

private:
    int getSlotFromKeybind(int key);
    bool canSwap(const std::string& collectionName);
    void clearQueue();
    void processSwapQueue(class ContainerScreenController* controller);
};