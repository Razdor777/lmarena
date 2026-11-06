#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class ItemDupe : public ModuleBase<ItemDupe>
{
public:
    NumberSetting mSlot = NumberSetting("Slot", "Inventory slot to dupe from (0-35)", 0.f, 0.f, 35.f, 1.f);
    EnumSetting mMethod = EnumSetting("Method", "Dupe method to try", 0,
        {"DoubleTransaction", "SwapExploit", "DropDupe"});
    NumberSetting mDelay = NumberSetting("Delay", "Ticks between attempts", 5.f, 1.f, 40.f, 1.f);

    ItemDupe() : ModuleBase("ItemDupe", "Attempt item duplication via packet exploit",
        ModuleCategory::Misc, 0, false)
    {
        addSettings(&mSlot, &mMethod, &mDelay);
        mNames = {
            {Lowercase, "itemdupe"}, {LowercaseSpaced, "item dupe"},
            {Normal, "ItemDupe"}, {NormalSpaced, "Item Dupe"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);

private:
    uint64_t mLastAttempt = 0;
    int mAttemptCount = 0;

    void tryDoubleTransaction(class Actor* player);
    void trySwapExploit(class Actor* player);
    void tryDropDupe(class Actor* player);
};
