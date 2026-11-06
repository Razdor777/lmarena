#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <deque>

struct GhostFrame {
    glm::vec3 position;
    float yaw;
    uint64_t timestamp;
};

class AfterImage : public ModuleBase<AfterImage>
{
public:
    NumberSetting mTrailCount = NumberSetting("Trail Count", "Number of ghost images", 5.f, 2.f, 10.f, 1.f);
    NumberSetting mSpacing = NumberSetting("Spacing", "Ticks between ghosts", 2.f, 1.f, 10.f, 1.f);
    NumberSetting mFadeSpeed = NumberSetting("Fade Speed", "How fast ghosts fade", 0.7f, 0.1f, 1.0f, 0.05f);
    EnumSetting mColorMode = EnumSetting("Color", "Ghost color mode", 0, {"Theme", "White", "Rainbow", "Red", "Blue"});
    BoolSetting mOnlyMoving = BoolSetting("Only Moving", "Show only when moving", true);

    AfterImage() : ModuleBase("AfterImage", "Anime-style speed trails behind player",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mTrailCount, &mSpacing, &mFadeSpeed, &mColorMode, &mOnlyMoving);
        mNames = {
            {Lowercase, "afterimage"}, {LowercaseSpaced, "after image"},
            {Normal, "AfterImage"}, {NormalSpaced, "After Image"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onRenderEvent(RenderEvent& event);

private:
    std::deque<GhostFrame> mFrames;
    uint64_t mTickCounter = 0;
    glm::vec3 mLastPos = { 0, 0, 0 };
};
