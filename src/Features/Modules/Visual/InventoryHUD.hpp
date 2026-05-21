#pragma once
//
// InventoryHUD — показывает инвентарь поверх HUD как на скрине:
// [Inventory]               [x]
// [ ][ ][ ][ ][ ][ ][ ][ ][ ]  <- хотбар
// [ ][ ][ ][ ][ ][ ][ ][ ][ ]  <- инвентарь ряд 1
// [ ][ ][ ][ ][ ][ ][ ][ ][ ]  <- инвентарь ряд 2
// [ ][ ][ ][ ][ ][ ][ ][ ][ ]  <- инвентарь ряд 3
//
// С иконками предметов, текстурами, числом стаков.
// Перетаскивается через HudEditor.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class InventoryHUD : public ModuleBase<InventoryHUD>
{
public:
    // Что показывать
    BoolSetting mShowHotbar   = BoolSetting("Show Hotbar",   "Show hotbar row",           true);
    BoolSetting mShowInventory= BoolSetting("Show Inventory","Show full inventory",        true);
    BoolSetting mShowArmor    = BoolSetting("Show Armor",    "Show armor slots",           true);
    BoolSetting mShowOffhand  = BoolSetting("Show Offhand",  "Show offhand slot",          true);
    BoolSetting mOnlyInGame   = BoolSetting("Only In Game",  "Show only in hud_screen",    true);
    BoolSetting mHighlightSel = BoolSetting("Highlight Slot","Highlight selected hotbar slot", true);

    // Стиль
    NumberSetting mSlotSize  = NumberSetting("Slot Size",  "Size of each slot in px",     28.f, 14.f, 60.f, 1.f);
    NumberSetting mScale     = NumberSetting("Scale",      "Overall scale",               1.f,  0.5f,  2.f, 0.05f);
    NumberSetting mOpacity   = NumberSetting("Opacity",    "Background opacity",          0.82f, 0.f,   1.f, 0.05f);
    NumberSetting mRounding  = NumberSetting("Rounding",   "Corner rounding",             4.f,   0.f,  12.f, 1.f);
    BoolSetting   mUseTextures = BoolSetting("Textures",   "Show item textures",          true);
    BoolSetting   mShowCount  = BoolSetting("Show Count",  "Show item stack count",       true);
    BoolSetting   mShowTitle  = BoolSetting("Title",       "Show 'Inventory' header",     true);

    InventoryHUD() : ModuleBase("InventoryHUD",
        "Инвентарь поверх HUD с иконками как на скрине",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mShowHotbar, &mShowInventory, &mShowArmor, &mShowOffhand,
            &mOnlyInGame, &mHighlightSel,
            &mSlotSize, &mScale, &mOpacity, &mRounding,
            &mUseTextures, &mShowCount, &mShowTitle
        );
        mNames = {
            {Lowercase,       "inventoryhud"},
            {LowercaseSpaced, "inventory hud"},
            {Normal,          "InventoryHUD"},
            {NormalSpaced,    "Inventory HUD"}
        };
    }

    HudElement* mHudElem = nullptr;

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    void drawSlot(ImDrawList* dl, float x, float y, float size,
                  class ItemStack* stack, bool selected, float opacity, float rounding);
};
