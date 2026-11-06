#pragma once

#include <Features/Modules/Module.hpp>

class VSync : public ModuleBase<VSync>
{
public:
    NumberSetting mSyncInterval = NumberSetting("Sync Interval",
        "1 = 60Hz, 2 = 30Hz, 3 = 20Hz", 1, 1, 3, 1);

    VSync() : ModuleBase("VSync", "Vertical synchronization - eliminates screen tearing",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mSyncInterval);

        mNames = {
            {Lowercase, "vsync"},
            {LowercaseSpaced, "vsync"},
            {Normal, "VSync"},
            {NormalSpaced, "VSync"}
        };
    }

    // D3DHook читает эти значения каждый кадр
    static inline bool sEnabled = false;
    static inline int sSyncInterval = 0;

    void onEnable() override {
        sEnabled = true;
        sSyncInterval = static_cast<int>(mSyncInterval.mValue);
    }

    void onDisable() override {
        sEnabled = false;
        sSyncInterval = 0;
    }

    // Вызывается каждый кадр из D3DHook — обновляет значение
    static void updateFromSettings() {
        auto mod = gFeatureManager->mModuleManager->getModule<VSync>();
        if (mod && mod->mEnabled) {
            sSyncInterval = static_cast<int>(mod->mSyncInterval.mValue);
        }
    }

    std::string getSettingDisplay() override {
        if (mSyncInterval.mValue == 1) return "60Hz";
        if (mSyncInterval.mValue == 2) return "30Hz";
        if (mSyncInterval.mValue == 3) return "20Hz";
        return std::to_string((int)mSyncInterval.mValue);
    }
};