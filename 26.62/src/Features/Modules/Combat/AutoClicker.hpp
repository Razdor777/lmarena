#pragma once

#include <Features/Modules/Setting.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Events/LookInputEvent.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <unordered_map>

class AutoClicker : public ModuleBase<AutoClicker>
{
public:
    enum class ClickMode
    {
        Left,
        Right,
        Both
    };

    EnumSettingT<ClickMode> mClickMode = EnumSettingT<ClickMode>("Click Mode", "The click mode", ClickMode::Left, "Left", "Right", "Both");
    BoolSetting mHold = BoolSetting("Hold", "Only click when holding", false);
    BoolSetting mRandomizeCPS = BoolSetting("Randomize CPS", "Randomize the CPS", false);
    BoolSetting mWeaponsOnly = BoolSetting("Weapons Only", "Only click when holding a weapon", false);
    NumberSetting mCPS = NumberSetting("CPS", "The amount of times to click every second.", 16, 1, 60, 1);
    NumberSetting mRandomCPSMin = NumberSetting("CPS Min", "The minimum amount of times to click every second.", 10, 1, 60, 1);
    NumberSetting mRandomCPSMax = NumberSetting("CPS Max", "The maximum amount of times to click every second.", 20, 1, 60, 1);
    BoolSetting mAllowBlockBreaking = BoolSetting("Allow Block Breaking", "Allow block breaking", false);

    // Tool Abuse
    BoolSetting mToolAbuse = BoolSetting("Tool Abuse", "Swap hotbar slots on attack to reset attack cooldown", false);
    NumberSetting mToolAbuseSwapDelay = NumberSetting("Swap Delay", "Delay between swaps (ms)", 0, 0, 500, 10);
    BoolSetting mToolAbuseOnlyOnHit = BoolSetting("Only On Entity", "Only swap when actually hitting an entity", true);

    // 360 Spin
    BoolSetting m360Spin = BoolSetting("360 Spin", "Spin 360 degrees on bind press", false);
    BoolSetting m360RandomDir = BoolSetting("Random Direction", "Randomize spin direction (left/right)", true);
    NumberSetting m360Speed = NumberSetting("Spin Speed", "Rotation speed", 45.f, 1.f, 180.f, 1.f);
    BoolSetting m360Smooth = BoolSetting("Smooth", "Smooth accel/decel", true);
    BoolSetting mShowBindPanel = BoolSetting("Show Bind Panel", "Show key bind panel", true);

    int mTriggerKey = VK_CAPITAL;
    std::unordered_map<int, bool> mKeyStates;
    bool mIsBindingKey = false;
    bool mBindWaitRelease = false;

    AutoClicker() : ModuleBase<AutoClicker>("AutoClicker", "Automatically clicks for you", ModuleCategory::Combat, 0, false) {
        addSettings(
            &mClickMode,
            &mRandomizeCPS,
            &mCPS,
            &mRandomCPSMin,
            &mRandomCPSMax,
            &mHold,
            &mWeaponsOnly,
            &mAllowBlockBreaking,
            &mToolAbuse,
            &mToolAbuseSwapDelay,
            &mToolAbuseOnlyOnHit,
            &m360Spin,
            &m360RandomDir,
            &m360Speed,
            &m360Smooth,
            &mShowBindPanel
        );

        VISIBILITY_CONDITION(mCPS, !mRandomizeCPS.mValue);
        VISIBILITY_CONDITION(mRandomCPSMin, mRandomizeCPS.mValue);
        VISIBILITY_CONDITION(mRandomCPSMax, mRandomizeCPS.mValue);
        VISIBILITY_CONDITION(mAllowBlockBreaking, mClickMode.mValue == ClickMode::Left);
        VISIBILITY_CONDITION(mToolAbuseSwapDelay, mToolAbuse.mValue);
        VISIBILITY_CONDITION(mToolAbuseOnlyOnHit, mToolAbuse.mValue);
        VISIBILITY_CONDITION(m360RandomDir, m360Spin.mValue);
        VISIBILITY_CONDITION(m360Speed, m360Spin.mValue);
        VISIBILITY_CONDITION(m360Smooth, m360Spin.mValue);
        VISIBILITY_CONDITION(mShowBindPanel, m360Spin.mValue);

        mNames = {
                {Lowercase, "autoclicker"},
                {LowercaseSpaced, "auto clicker"},
                {Normal, "AutoClicker"},
                {NormalSpaced, "Auto Clicker"}
        };
    }

    int mCurrentCPS = 10;
    bool mIsSpinning = false;
    float m360CurrentAngle = 0.f;
    float m360Direction = 1.f;
    float m360StartYaw = 0.f;
    float m360Progress = 0.f;
    float m360Duration = 0.f;

    void randomizeCPS()
    {
        mCurrentCPS = getCPS();
    }

    int getCPS()
    {
        if (mRandomizeCPS.mValue)
        {
            return MathUtils::random(mRandomCPSMin.as<int>(), mRandomCPSMax.as<int>());
        }

        return mCPS.mValue;
    }

    void tryToolAbuseSwap(class Actor* player, class HitResult* hitres);
    std::vector<int> getToolAbuseSlots(class Actor* player);

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onLookInputEvent(class LookInputEvent& event);

    static bool isAnyKeyHeld();
    static int findHeldKey();
    static const char* getKeyName(int vk);
    bool isKeyJustPressed(int vk);
};