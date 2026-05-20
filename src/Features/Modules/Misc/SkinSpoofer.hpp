#pragma once
//
// SkinSpoofer — Modify your own skin data using IDA-reversed fields
// Animation flags, TrustedSkin, PremiumSkin, CapeId, SkinColor
//

#include <Features/FeatureManager.hpp>

class SkinSpoofer : public ModuleBase<SkinSpoofer> {
public:
    // Animation flags — from IDA reverse of sub_1409A2FA0
    BoolSetting mArmsDown = BoolSetting("Arms Down", "Disable arm swing animation", false);
    BoolSetting mArmsOutFront = BoolSetting("Arms Out Front", "Zombie-style arms extended forward", false);
    BoolSetting mStationaryLegs = BoolSetting("Stationary Legs", "Legs don't move when walking", false);
    BoolSetting mSingleLegAnim = BoolSetting("Single Leg Anim", "Only one leg animates", false);
    BoolSetting mSingleArmAnim = BoolSetting("Single Arm Anim", "Only one arm animates", false);
    BoolSetting mStatueOfLiberty = BoolSetting("T-Pose", "Statue of Liberty arms (T-pose)", false);
    BoolSetting mDontShowArmor = BoolSetting("Hide Armor", "Don't render armor visually", false);
    BoolSetting mNoHeadBob = BoolSetting("No Head Bob", "Disable head bobbing animation", false);
    BoolSetting mUpsideDown = BoolSetting("Upside Down", "Render player model upside down", false);
    BoolSetting mInvertedCrouch = BoolSetting("Inverted Crouch", "Invert crouch animation", false);
    
    // Skin flags
    BoolSetting mTrustedSkin = BoolSetting("Trusted Skin", "Mark skin as trusted (bypasses checks)", true);
    BoolSetting mPremiumSkin = BoolSetting("Premium Skin", "Mark skin as premium marketplace skin", false);
    BoolSetting mSlimArms = BoolSetting("Slim Arms", "Use slim (Alex) arm model", false);
    
    // Skins and Capes
    enum class CapeType {
        None, Pancakes, Minecon2019, Minecon2016, Founder
    };
    EnumSettingT<CapeType> mCape = EnumSettingT<CapeType>("Cape", "Spoof cape ID", CapeType::None, "None", "Pancakes", "Minecon 2019", "Minecon 2016", "Founder");
    BoolSetting mInvisibleSkin = BoolSetting("Invisible Variant", "Set skin color alpha to 0", false);
    
    SkinSpoofer() : ModuleBase("SkinSpoofer", "Modify skin animation flags and properties", ModuleCategory::Misc, 0, false) {
        addSettings(
            &mArmsDown,
            &mArmsOutFront,
            &mStationaryLegs,
            &mSingleLegAnim,
            &mSingleArmAnim,
            &mStatueOfLiberty,
            &mDontShowArmor,
            &mNoHeadBob,
            &mUpsideDown,
            &mInvertedCrouch,
            &mTrustedSkin,
            &mPremiumSkin,
            &mSlimArms,
            &mCape,
            &mInvisibleSkin
        );
        
        mNames = {
            {Lowercase, "skinspoofer"},
            {LowercaseSpaced, "skin spoofer"},
            {Normal, "SkinSpoofer"},
            {NormalSpaced, "Skin Spoofer"}
        };
    }
    
    void onEnable() override;
    void onDisable() override;
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
    
    // Build animation bitfield from settings
    uint16_t getAnimationFlags() const;
};
