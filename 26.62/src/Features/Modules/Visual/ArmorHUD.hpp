#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Utils/ItemTextures.hpp>

class ArmorHUD : public ModuleBase<ArmorHUD> {
public:
    NumberSetting mPosX = NumberSetting("X", "X position", 10.0f, 0.0f, 1920.0f, 1.0f);
    NumberSetting mPosY = NumberSetting("Y", "Y position", 200.0f, 0.0f, 1080.0f, 1.0f);
    NumberSetting mScale = NumberSetting("Scale", "Scale", 1.0f, 0.5f, 3.0f, 0.1f);
    BoolSetting mShowPercent = BoolSetting("Show %", "Show percentage text", true);
    BoolSetting mShowBar = BoolSetting("Show Bar", "Show durability bar", true);
    BoolSetting mShowName = BoolSetting("Show Name", "Show short item name (fallback)", false);
    BoolSetting mHorizontal = BoolSetting("Horizontal", "Horizontal layout", false);
    BoolSetting mUseTextures = BoolSetting("Use Textures", "Show item icons", true);
    NumberSetting mOpacity = NumberSetting("Opacity", "Background opacity", 0.6f, 0.0f, 1.0f, 0.05f);
    BoolSetting mShowMainHand = BoolSetting("Show Main Hand", "Show held item durability", true);
    BoolSetting mShowOffhand = BoolSetting("Show Offhand", "Show offhand item", false);

    ArmorHUD() : ModuleBase("ArmorHUD", "Shows armor durability on screen",
                            ModuleCategory::Visual, 0, false) {
        addSettings(&mPosX, &mPosY, &mScale, &mShowPercent, &mShowBar,
                    &mShowName, &mHorizontal, &mUseTextures, &mOpacity,
                    &mShowMainHand, &mShowOffhand);

        mNames = {
            {Lowercase, "armorhud"},
            {LowercaseSpaced, "armor hud"},
            {Normal, "ArmorHUD"},
            {NormalSpaced, "Armor HUD"}
        };
    }

    struct SlotRenderInfo {
        std::string label;
        std::string shortName;
        float durPercent;
        ImColor materialColor;
        bool valid;
    };

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    ImColor getDurabilityColor(float percent);
    ImColor getMaterialColor(const std::string& name);
    std::string getShortName(const std::string& fullName);
    float getFixedDurability(ItemStack* stack);
    SlotRenderInfo getSlotInfo(ItemStack* stack, const char* label);
    void renderSlot(ImDrawList* dl, float x, float y, float slotSize,
                    const SlotRenderInfo& info, const std::string& itemName);
};