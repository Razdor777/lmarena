#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/ContainerScreenTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>

class ContainerActions : public ModuleBase<ContainerActions>
{
public:
    NumberSetting mActionDelay = NumberSetting("Action Delay", "ms between items", 5.f, 0.1f, 25.f, 0.1f);

    ContainerActions() : ModuleBase("ContainerActions",
        "Buttons for Drop All / Take All / Store All / Sort Chest",
        ModuleCategory::Player, 0, false)
    {
        addSettings(&mActionDelay);
        mNames = {
            {Lowercase,        "containeractions"},
            {LowercaseSpaced,  "container actions"},
            {Normal,           "ContainerActions"},
            {NormalSpaced,     "Container Actions"}
        };
    }

    enum class Action { None, DropAll, TakeAll, StoreAll, SortChest };

    Action   mAction       = Action::None;
    int      mActionSlot   = 0;
    int      mActionPhase  = 0;
    uint64_t mLastActionTime = 0;

    // Sort — general
    bool mSortUseInPlace = false;
    int  mChestSize = 0;

    // Sort — via inventory (handleAutoPlace)
    struct SortSlotEntry {
        std::string collection;
        int collectionSlot;
    };
    std::vector<SortSlotEntry> mSortOrder;
    int mSortOrderIdx = 0;

    // Sort — in-place (_tryBeginTransition)
    struct SwapPair { int a; int b; };
    std::vector<SwapPair> mSortSwaps;
    int mSortSwapIdx  = 0;
    int mSortSubStep  = 0;

    void onEnable()  override;
    void onDisable() override;
    void startAction(Action act);
    void resetAction();

    void onRenderEvent(RenderEvent& event);
    void onContainerScreenTickEvent(ContainerScreenTickEvent& event);
    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);

    void processDropAll(Actor* player);
    void processTakeAll(ContainerScreenController* csc, Actor* player);
    void processStoreAll(ContainerScreenController* csc, Actor* player);
    void processSortChest(ContainerScreenController* csc, Actor* player);

    int  getSortKey(ItemStack* item);
    void computeSortSwaps(Actor* player);
    int  countFreeInvSlots(Actor* player);
    int  countChestItems(Actor* player);

    bool drawButton(ImDrawList* dl, ImVec2 pos, ImVec2 size,
                    const char* label, bool disabled, bool active);

    std::string getSettingDisplay() override {
        switch (mAction) {
            case Action::DropAll:   return "Dropping...";
            case Action::TakeAll:   return "Taking...";
            case Action::StoreAll:  return "Storing...";
            case Action::SortChest: return mSortUseInPlace ? "Sorting (in-place)..." : "Sorting...";
            default: return "";
        }
    }
};