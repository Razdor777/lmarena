#pragma once
//
// InventoryHUD — инвентарь поверх HUD.
// Иконки рисует САМ ДВИЖОК из своего атласа (память игры) через
// MinecraftUIRenderContext::drawImage. ImGui рисует только рамки и счётчики.
// PNG (ItemTextures) остались как фоллбэк для блоков и если движок недоступен.
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>

class ItemStack;

class InventoryHUD : public ModuleBase<InventoryHUD>
{
public:
    BoolSetting mShowHotbar    = BoolSetting("Show Hotbar",   "Show hotbar row",              true);
    BoolSetting mShowInventory = BoolSetting("Show Inventory","Show full inventory",           true);
    BoolSetting mShowArmor     = BoolSetting("Show Armor",    "Show armor slots",             true);
    BoolSetting mShowOffhand   = BoolSetting("Show Offhand",  "Show offhand slot",            true);
    BoolSetting mOnlyInGame    = BoolSetting("Only In Game",  "Show only in hud_screen",      true);
    BoolSetting mHighlightSel  = BoolSetting("Highlight Slot","Highlight selected slot",      true);

    NumberSetting mSlotSize  = NumberSetting("Slot Size",  "Size of each slot in px",      28.f, 14.f, 60.f, 1.f);
    NumberSetting mScale     = NumberSetting("Scale",      "Overall scale",                1.f,  0.5f,  2.f, 0.05f);
    NumberSetting mOpacity   = NumberSetting("Opacity",    "Background opacity",           0.82f, 0.f,   1.f, 0.05f);
    NumberSetting mRounding  = NumberSetting("Rounding",   "Corner rounding",              4.f,   0.f,  12.f, 1.f);
    BoolSetting   mUseTextures = BoolSetting("Textures",   "Show item icons",               true);
    BoolSetting   mShowCount   = BoolSetting("Show Count", "Show stack count",              true);
    BoolSetting   mShowTitle   = BoolSetting("Title",      "Show 'Inventory' header",       true);

    // Включает отрисовку иконок предметов движком из его собственного атласа
    // (вместо PNG/текстового фоллбэка). Выключи, если что-то падает/глючит.
    BoolSetting mNativeDraw = BoolSetting("Native Icons", "Draw item icons via game engine", true);

    // GUI Scale из настроек Minecraft (Видео → GUI Scale). Обычно 2 или 3.
    // Нужен, чтобы координаты ImGui (пиксели) перевести в координаты движка.
    NumberSetting mNativeScale = NumberSetting("Native Scale", "GUI scale of Minecraft (для иконок из памяти)", 2.f, 1.f, 6.f, 0.5f);

    InventoryHUD() : ModuleBase("InventoryHUD",
        "Инвентарь поверх HUD",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mShowHotbar, &mShowInventory, &mShowArmor, &mShowOffhand,
            &mOnlyInGame, &mHighlightSel,
            &mSlotSize, &mScale, &mOpacity, &mRounding,
            &mUseTextures, &mShowCount, &mShowTitle,
            &mNativeDraw, &mNativeScale
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
                  ItemStack* stack, bool selected, float opacity, float rounding);
};
