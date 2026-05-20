#pragma once

#include <Features/Modules/Module.hpp>

class ItemUseDelayFix : public ModuleBase<ItemUseDelayFix> {
public:
    ItemUseDelayFix() : ModuleBase("ItemUseDelayFix", 
        "Removes 200ms delay after attack on using items (e.g projectiles).", 
        ModuleCategory::Combat, 0, false) 
    {
        mNames = {
            {Lowercase, "itemusedelayfix"},
            {LowercaseSpaced, "item use delay fix"},
            {Normal, "ItemUseDelayFix"},
            {NormalSpaced, "Item Use Delay Fix"}
        };
    }

    static inline std::vector<unsigned char> mOriginal;
    static inline uintptr_t mAddress = 0;

    void onEnable() override;
    void onDisable() override;
    void onInit() override;
};