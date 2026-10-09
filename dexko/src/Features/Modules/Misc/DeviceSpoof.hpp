#pragma once
//
// Dexko DeviceSpoof — копия движкового модуля (1:1).
// Всегда включён: конструктор ставит mWantedState = true, а
// Dexko::shutdownThread дополнительно форсирует включение после загрузки
// конфига, чтобы конфиг не мог его выключить.
//


class DeviceSpoof : public ModuleBase<DeviceSpoof>
{
public:
    DeviceSpoof() : ModuleBase("DeviceSpoof", "Spoofs all ur ids", ModuleCategory::Misc, 0, true)
    {
        // Enabled by default so fresh configs always have spoofing active.
        // NOTE: only mWantedState may be set here. ModuleManager::onClientTick()
        // enables a module when mWantedState != mEnabled and then calls
        // onEnable() (which is what actually injects the patch). Setting
        // mEnabled = true here as well made the two match from the very start,
        // so onEnable()/inject() never ran on injection and DeviceSpoof silently
        // did nothing until it was toggled off and back on again.
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
    static inline bool mInjected = false;

    bool inject();
    void eject();
    void spoofMboard();

    void onInit() override;
    void onEnable() override;
    void onDisable() override;
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
};
