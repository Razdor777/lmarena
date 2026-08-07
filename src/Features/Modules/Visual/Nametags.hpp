#pragma once

#include <Utils/ItemTextures.hpp>

class Nametags : public ModuleBase<Nametags>
{
public:
    enum class Style { Solstice, Advanced };

    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style",
        "The style of the nametags.", Style::Solstice, "Solstice", "Advanced");
    BoolSetting mShowFriends    = BoolSetting("Show Friends",
        "Render nametags on friends.", true);
    BoolSetting mRenderLocal    = BoolSetting("Render Local",
        "Render nametag on local player.", false);
    BoolSetting mCleanNames     = BoolSetting("Clean Names",
        "Remove donation tags and color codes.", true);
    BoolSetting mDistanceScaledFont = BoolSetting("Distance Scaled Font",
        "Scale font based on distance.", true);
    NumberSetting mBlurStrength = NumberSetting("Blur Strength",
        "Blur strength.", 0.f, 0.f, 10.f, 0.1f);
    NumberSetting mFontSize     = NumberSetting("Font Size",
        "Font size.", 23.f, 1.f, 40.f, 0.01f);
    NumberSetting mScalingMultiplier = NumberSetting("Scaling Multiplier",
        "Font scaling multiplier.", 1.0f, 0.f, 5.f, 0.01f);
    NumberSetting mMinScale     = NumberSetting("Minimum Scale",
        "Minimum font scale.", 10.f, 0.01f, 20.f, 0.01f);
    BoolSetting mShowHealth     = BoolSetting("Show Health",
        "Show player health.", true);
    BoolSetting mShowArmor      = BoolSetting("Show Armor",
        "Show armor pieces.", true);
    BoolSetting mShowHeldItem   = BoolSetting("Show Held Item",
        "Show main hand item.", true);
    BoolSetting mShowOffhand    = BoolSetting("Show Offhand",
        "Show offhand item.", true);
    BoolSetting mShowEnchants   = BoolSetting("Show Enchants",
        "Show enchantment labels.", false);
    BoolSetting mShowDurability = BoolSetting("Show Durability",
        "Show durability bars.", true);
    BoolSetting mShowDurPercent = BoolSetting("Show Dur %",
        "Show durability percentage.", false);
    BoolSetting mUseTextures    = BoolSetting("Use Textures",
        "Show item icons.", true);
    NumberSetting mIconSize     = NumberSetting("Icon Size",
        "Size of item icons.", 16.f, 8.f, 32.f, 1.f);
    NumberSetting mItemSpacing  = NumberSetting("Item Spacing",
        "Gap between item slots.", 2.f, 0.f, 12.f, 0.5f);
    NumberSetting mEnchantScale = NumberSetting("Enchant Scale",
        "Scale of enchant text.", 1.0f, 0.3f, 3.0f, 0.1f);
    NumberSetting mEnchantR     = NumberSetting("Enchant R",
        "Enchant text red.",   200.f, 0.f, 255.f, 1.f);
    NumberSetting mEnchantG     = NumberSetting("Enchant G",
        "Enchant text green.", 150.f, 0.f, 255.f, 1.f);
    NumberSetting mEnchantB     = NumberSetting("Enchant B",
        "Enchant text blue.",  255.f, 0.f, 255.f, 1.f);

    Nametags() : ModuleBase("Nametags", "Draws nametags above entities",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mStyle, &mShowFriends, &mRenderLocal, &mCleanNames,
            &mDistanceScaledFont, &mFontSize, &mScalingMultiplier, &mMinScale,
            &mBlurStrength, &mShowHealth,
            &mShowArmor, &mShowHeldItem, &mShowOffhand,
            &mShowEnchants, &mShowDurability, &mShowDurPercent,
            &mUseTextures, &mIconSize, &mItemSpacing,
            &mEnchantScale, &mEnchantR, &mEnchantG, &mEnchantB
        );

        VISIBILITY_CONDITION(mFontSize,          !mDistanceScaledFont.mValue);
        VISIBILITY_CONDITION(mScalingMultiplier,  mDistanceScaledFont.mValue);
        VISIBILITY_CONDITION(mMinScale,           mDistanceScaledFont.mValue);
        VISIBILITY_CONDITION(mShowEnchants,
            mShowArmor.mValue || mShowHeldItem.mValue);
        VISIBILITY_CONDITION(mShowDurability,
            mShowArmor.mValue || mShowHeldItem.mValue);
        VISIBILITY_CONDITION(mShowDurPercent,
            mShowArmor.mValue || mShowHeldItem.mValue || mShowOffhand.mValue);
        VISIBILITY_CONDITION(mIconSize,           mUseTextures.mValue);
        VISIBILITY_CONDITION(mItemSpacing,
            mShowArmor.mValue || mShowHeldItem.mValue || mShowOffhand.mValue);
        VISIBILITY_CONDITION(mEnchantScale,       mShowEnchants.mValue);
        VISIBILITY_CONDITION(mEnchantR,           mShowEnchants.mValue);
        VISIBILITY_CONDITION(mEnchantG,           mShowEnchants.mValue);
        VISIBILITY_CONDITION(mEnchantB,           mShowEnchants.mValue);

        mNames = {
            {Lowercase,       "nametags"},
            {LowercaseSpaced, "nametags"},
            {Normal,          "Nametags"},
            {NormalSpaced,    "Nametags"}
        };
    }

    static std::string cleanPlayerName(const std::string& name);

    void onEnable()  override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    void onNametagRenderEvent(class NametagRenderEvent& event);

private:
    struct CachedPlayerInfo {
        Actor*      actor       = nullptr;  // используется только в updatePlayerCache
        int64_t     runtimeID   = 0;
        std::string displayName;
        float       hp          = 0.f;
        float       maxHp       = 0.f;
        float       absorption  = 0.f;
        bool        tracked     = false;
        bool        stale       = false;
        float       secSince    = 0.f;
        bool        isFriend    = false;

        struct SlotCache {
            std::string itemName;
            std::string shortName;
            std::string enchants;
            float       durPercent  = -1.f;
            ImColor     tierColor   = ImColor(180, 180, 180, 255);
            bool        isEmpty     = true;
            int         count       = 0;
        };
        std::array<SlotCache, 4> armor;
        SlotCache                mainHand;
        SlotCache                offhand;

        bool valid = false;
    };

    std::vector<CachedPlayerInfo> mPlayerCache;
    int64_t   mLastCacheUpdate  = 0;
    int       mCacheUpdateDelay = 100;

    void updatePlayerCache();
    void buildSlotCache(ItemStack* stack, CachedPlayerInfo::SlotCache& out);

    void renderAdvancedNametag(const CachedPlayerInfo& info,
        ImDrawList* drawList, const glm::vec2& screen,
        float fontSize, float padding);

    struct ItemDisplayInfo {
        std::string itemName;
        std::string name;
        std::string enchants;
        float       durPercent = -1.f;
        ImColor     tierColor  = ImColor(180, 180, 180, 255);
        bool        isEmpty    = true;
    };

    ItemDisplayInfo makeDisplayInfo(const CachedPlayerInfo::SlotCache& slot) const;

    ImColor     getTierColor(const std::string& name);
    ImColor     getDurabilityColor(float percent);
    std::string getShortItemName(const std::string& fullName);
    std::string getShortEnchantString(ItemStack* stack);
    float       getFixedDurability(ItemStack* stack);

    void renderEquipmentAbove(ImDrawList* dl, float centerX, float bottomY,
        float fontSize, const CachedPlayerInfo& info);
    void renderItemSlot(ImDrawList* dl, float x, float y, float size,
        const ItemDisplayInfo& info, bool showDur, bool showEnch);

    ImFont* getCyrillicFont();
};