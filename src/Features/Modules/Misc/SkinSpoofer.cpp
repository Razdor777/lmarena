//
// SkinSpoofer — Modify skin animation flags and properties
// Uses IDA-reversed fields from sub_1409A2FA0 / sub_141B18B30
//

#include "SkinSpoofer.hpp"

#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerSkinPacket.hpp>
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>

namespace {
void replaceAll(std::string& inout, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = inout.find(from, pos)) != std::string::npos) {
        inout.replace(pos, from.size(), to);
        pos += to.size();
    }
}
}

void SkinSpoofer::onEnable()
{
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent, &SkinSpoofer::onConnectionRequestEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    
    ChatUtils::displayClientMessage("§b[SkinSpoofer] §fEnabled — skin flags will be applied on next server join");
}

void SkinSpoofer::onDisable()
{
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent, &SkinSpoofer::onConnectionRequestEvent>(this);
    
    ChatUtils::displayClientMessage("§c[SkinSpoofer] §fDisabled");
}

uint16_t SkinSpoofer::getAnimationFlags() const
{
    // From IDA: sub_1409A2FA0 animation bitfield
    // Bit 0: animationArmsDown
    // Bit 1: animationArmsOutFront
    // Bit 2: animationStationaryLegs
    // Bit 3: animationSingleLegAnimation
    // Bit 4: animationSingleArmAnimation
    // Bit 5: animationStatueOfLibertyArms
    // Bit 6: animationDontShowArmor
    // Bit 7: animationNoHeadBob
    // Bit 8: animationUpsideDown
    // Bit 9: animationInvertedCrouch
    
    uint16_t flags = 0;
    if (mArmsDown.mValue)       flags |= (1 << 0);
    if (mArmsOutFront.mValue)   flags |= (1 << 1);
    if (mStationaryLegs.mValue) flags |= (1 << 2);
    if (mSingleLegAnim.mValue)  flags |= (1 << 3);
    if (mSingleArmAnim.mValue)  flags |= (1 << 4);
    if (mStatueOfLiberty.mValue) flags |= (1 << 5);
    if (mDontShowArmor.mValue)  flags |= (1 << 6);
    if (mNoHeadBob.mValue)      flags |= (1 << 7);
    if (mUpsideDown.mValue)     flags |= (1 << 8);
    if (mInvertedCrouch.mValue) flags |= (1 << 9);
    return flags;
}

void SkinSpoofer::onConnectionRequestEvent(ConnectionRequestEvent& event)
{
    if (!event.mSkin) return;
    
    SerializedSkin* skin = event.mSkin;
    
    // === Apply TrustedSkin flag ===
    if (mTrustedSkin.mValue) {
        skin->mIsTrustedSkin = TrustedSkinFlag::YES;
    }
    
    // === Apply PremiumSkin flag ===
    if (mPremiumSkin.mValue) {
        skin->mIsPremium = true;
    }
    
    // === Apply slim arms ===
    if (mSlimArms.mValue) {
        skin->mArmSizeType = persona::ArmSize::Type::Slim;
        if (skin->mDefaultGeometryName.find("geometry.humanoid") != std::string::npos) {
            skin->mDefaultGeometryName = "geometry.humanoid.customSlim";
        }
        std::string& rp = skin->mResourcePatch;
        if (!rp.empty()) {
            replaceAll(rp, "geometry.humanoid.customSlim", "geometry.humanoid.customSlim");
            replaceAll(rp, "geometry.humanoid.custom", "geometry.humanoid.customSlim");
            replaceAll(rp, "geometry.humanoid", "geometry.humanoid.customSlim");
        }
    }
    
    // === Apply Cape ===
    if (mCape.mValue != CapeType::None) {
        std::string capeId;
        switch (mCape.mValue) {
            case CapeType::Pancakes: capeId = "pancape"; break;
            case CapeType::Minecon2019: capeId = "minecon_2019_cape"; break;
            case CapeType::Minecon2016: capeId = "minecon_2016_cape"; break;
            case CapeType::Founder: capeId = "founder_cape"; break;
            default: break;
        }
        if (!capeId.empty()) {
            skin->mCapeId = capeId;
            if (event.mCapeId) *event.mCapeId = capeId;
        }
    }
    
    // === Apply Skin Color Hack ===
    if (mInvisibleSkin.mValue) {
        skin->mSkinColor.a = 0.0f; // Alpha to 0
    }
    
    // === Apply animation flags ===
    // Bedrock request parser uses SkinAnimationData / AnimationExpression fields.
    // Keep this as a plain JSON string to avoid unstable direct mutation of
    // internal MinecraftJson map storage.
    uint16_t animFlags = getAnimationFlags();
    if (animFlags != 0) {
        std::string animData = "{";
        if (mArmsDown.mValue)       animData += "\"animationArmsDown\":true,";
        if (mArmsOutFront.mValue)   animData += "\"animationArmsOutFront\":true,";
        if (mStationaryLegs.mValue) animData += "\"animationStationaryLegs\":true,";
        if (mSingleLegAnim.mValue)  animData += "\"animationSingleLegAnimation\":true,";
        if (mSingleArmAnim.mValue)  animData += "\"animationSingleArmAnimation\":true,";
        if (mStatueOfLiberty.mValue) animData += "\"animationStatueOfLibertyArms\":true,";
        if (mDontShowArmor.mValue)  animData += "\"animationDontShowArmor\":true,";
        if (mNoHeadBob.mValue)      animData += "\"animationNoHeadBob\":true,";
        if (mUpsideDown.mValue)     animData += "\"animationUpsideDown\":true,";
        if (mInvertedCrouch.mValue) animData += "\"animationInvertedCrouch\":true,";
        
        // Remove trailing comma
        if (!animData.empty() && animData.back() == ',')
            animData.pop_back();
        animData += "}";
        skin->mAnimationData = animData;
        
        // Log what we're spoofing
        std::string flagStr;
        if (mArmsDown.mValue) flagStr += "ArmsDown ";
        if (mArmsOutFront.mValue) flagStr += "ArmsOut ";
        if (mStationaryLegs.mValue) flagStr += "StaticLegs ";
        if (mStatueOfLiberty.mValue) flagStr += "T-Pose ";
        if (mUpsideDown.mValue) flagStr += "UpsideDown ";
        if (mInvertedCrouch.mValue) flagStr += "InvCrouch ";
        if (mNoHeadBob.mValue) flagStr += "NoHeadBob ";
        if (mDontShowArmor.mValue) flagStr += "HideArmor ";
        
        if (!flagStr.empty())
            ChatUtils::displayClientMessage("§b[SkinSpoofer] §fAnimation flags: §e" + flagStr);
    }
    
    // Log skin flags
    std::string skinInfo;
    if (mTrustedSkin.mValue) skinInfo += "§aTrusted ";
    if (mPremiumSkin.mValue) skinInfo += "§6Premium ";
    if (mSlimArms.mValue) skinInfo += "§bSlim ";
    if (mCape.mValue != CapeType::None) skinInfo += "§dCapeSpoof ";
    if (mInvisibleSkin.mValue) skinInfo += "§eInvisAlpha ";
    if (!skinInfo.empty())
        ChatUtils::displayClientMessage("§b[SkinSpoofer] §fSkin flags: " + skinInfo);
}
