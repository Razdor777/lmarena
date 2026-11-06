#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>

class FakePlayer : public ModuleBase<FakePlayer>
{
public:
    BoolSetting mShowNametag = BoolSetting("Show Nametag", "Show name above clone", true);
    BoolSetting mShowHealthBar = BoolSetting("Health Bar", "Show health bar", true);

    FakePlayer() : ModuleBase("FakePlayer", "Client-side player clone for testing",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mShowNametag, &mShowHealthBar);
        mNames = {
            {Lowercase, "fakeplayer"}, {LowercaseSpaced, "fake player"},
            {Normal, "FakePlayer"}, {NormalSpaced, "Fake Player"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

private:
    glm::vec3 mClonePos = { 0, 0, 0 };
    float mCloneYaw = 0.f;
    std::string mCloneName;
    bool mPosSet = false;
};
