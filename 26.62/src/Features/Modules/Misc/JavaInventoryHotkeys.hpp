#pragma once
//
// JavaInventoryHotkeys — SilentUse, DesyncUse, Offhand priority
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <unordered_map>

class JavaInventoryHotkeys : public ModuleBase<JavaInventoryHotkeys> {
public:
    BoolSetting mSilentUse = BoolSetting("Silent Use",
        "Use items from inventory without switching hotbar", false);

    BoolSetting mDesyncUse = BoolSetting("Desync Use",
        "Use item via inventory desync (experimental)", false);

    BoolSetting mOffhandPriority = BoolSetting("Offhand Priority",
        "Check offhand first before searching inventory", true);

    BoolSetting mShowBindPanel = BoolSetting("Show Bind Panel",
        "Show bind buttons on screen", true);

    JavaInventoryHotkeys() : ModuleBase("JavaInventoryHotkeys",
        "Advanced inventory interactions", ModuleCategory::Misc, 0, false)
    {
        addSettings(&mSilentUse, &mDesyncUse, &mOffhandPriority, &mShowBindPanel);
        mNames = {
            {Lowercase, "javainventoryhotkeys"},
            {Normal, "JavaInventoryHotkeys"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);

    bool hasCustomData() const override { return true; }
    nlohmann::json serializeCustomData() override;
    void deserializeCustomData(const nlohmann::json& j) override;

    std::string getSettingDisplay() override {
        if (mBindingSilent) return "Bind Silent...";
        if (mBindingDesync) return "Bind Desync...";
        return std::string("S:") + getKeyName(mSilentKey) + " D:" + getKeyName(mDesyncKey);
    }

private:
    bool mTriggerSilent = false;
    bool mTriggerDesync = false;

    int  mDesyncStage = 0;
    int  mDesyncFrom = -1;
    int  mDesyncTo = -1;

    // Binds
    int mSilentKey = 0;
    int mDesyncKey = 0;
    std::unordered_map<int, bool> mKeyStates;
    bool mBindingSilent = false;
    bool mBindingDesync = false;
    bool mBindWaitRelease = false;

    void doSilentUse();
    void doDesyncPhase1();
    void doDesyncPhase2();
    void resetDesync();

    void sendUseTxn(int fakeSlot, ItemStack* item, glm::vec3 pos);

    void renderBindButton(const char* label, bool& binding, int& key,
                          float& yOffset, float panX, float panY,
                          ImDrawList* drawList);

    static const char* getKeyName(int vk);
    bool isKeyJustPressed(int vk);
    static bool isAnyKeyHeld();
    static int findHeldKey();
};