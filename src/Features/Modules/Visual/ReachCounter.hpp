#pragma once

#include "Features/Modules/Module.hpp"
#include "HudEditor.hpp"
#include <chrono>
#include <random>

class ReachCounter : public ModuleBase<ReachCounter>
{
public:
    NumberSetting mResetTime = NumberSetting("Reset Time", "Time in seconds before resetting", 15.0f, 1.0f, 30.0f, 1.0f);
    BoolSetting mSpoof = BoolSetting("Spoof", "Show fake legit reach when using extended reach", false);
    NumberSetting mSpoofLimit = NumberSetting("Spoof Limit", "Spoof when reach exceeds this value", 3.0f, 2.5f, 6.0f, 0.1f);
    BoolSetting mDebug = BoolSetting("Debug", "Show debug info", false);

    ReachCounter() : ModuleBase<ReachCounter>("ReachCounter", "Displays your last hit range in blocks!", ModuleCategory::Visual, 0, false) {
        addSettings(&mResetTime, &mSpoof, &mSpoofLimit, &mDebug);

        mNames = {
            {Lowercase, "reachcounter"},
            {LowercaseSpaced, "reach counter"},
            {Normal, "ReachCounter"},
            {NormalSpaced, "Reach Counter"}
        };

        mElement = std::make_unique<HudElement>();
        mElement->mPos = { 0, 80 };
        mElement->mAnchor = HudElement::Anchor::TopMiddle;
        mElement->mCentered = true;
        const char* typeId = ModuleBase<ReachCounter>::getTypeID();
        mElement->mParentTypeIdentifier = const_cast<char*>(typeId);
        mElement->mSize = glm::vec2(200, 24);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(mElement.get());
    }

    void onEnable() override;
    void onDisable() override;
    void onPacketOutEvent(class PacketOutEvent& event);
    void onRenderEvent(class RenderEvent& event);

    std::unique_ptr<HudElement> mElement = nullptr;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastHit;
    float mReach = 0.f;
    float mDisplayReach = 0.f; // What we actually show (may be spoofed)
    int mHitCount = 0;

    // Spoof RNG
    std::mt19937 mRng{std::random_device{}()};
    float generateLegitReach();
};