#include "ItemUseDelayFix.hpp"
#include <SDK/SigManager.hpp>
#include <Utils/MemUtils.hpp>

void ItemUseDelayFix::onInit() {
    mAddress = SigManager::ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil;
    
    if (mAddress == 0) return;
    
    mOriginal = MemUtils::readBytes(mAddress, 6);
}

void ItemUseDelayFix::onEnable() {
    if (mAddress == 0) return;
    
    MemUtils::NopBytes(mAddress, 6);
}

void ItemUseDelayFix::onDisable() {
    if (mAddress == 0 || mOriginal.empty()) return;
    
    MemUtils::writeBytes(mAddress, mOriginal);
}