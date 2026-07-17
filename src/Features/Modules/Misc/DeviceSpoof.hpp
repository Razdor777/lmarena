#pragma once
//
// Created by vastrakai on 7/3/2024.
//


class DeviceSpoof : public ModuleBase<DeviceSpoof>
{
public:
    DeviceSpoof() : ModuleBase("DeviceSpoof", "Spoofs all ur ids", ModuleCategory::Misc, 0, false)
    {
        // NOTE: не выставляем mEnabled здесь — onEnable/inject() вызывается
        // через setEnabled() (Solstice принудительно включает модуль при инжекте).
        mNames = {
            {Lowercase, "devicespoof"},
            {LowercaseSpaced, "device spoof"},
            {Normal, "DeviceSpoof"},
            {NormalSpaced, "Device Spoof"}
        };
    }

    static inline unsigned char originalData[7];
    static inline unsigned char patch[] = {0x48, 0xBA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    static inline void* patchPtr = nullptr;
    static inline std::string DeviceModel;

    void inject();
    void eject();
    void spoofMboard();

    void onInit() override;
    void onEnable() override;
    void onDisable() override;
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
};
