#pragma once
//
// AutoPatrol - Automatically patrol between pos1 and pos2
//

#include <Features/Modules/Module.hpp>
#include <glm/vec3.hpp>

class AutoPatrol : public ModuleBase<AutoPatrol>
{
public:
    // Статические позиции, доступные из команд
    static glm::vec3 sPos1;
    static glm::vec3 sPos2;
    static bool sPos1Set;
    static bool sPos2Set;

private:
    bool mGoingToPos2 = true;  // true = идём к pos2, false = идём к pos1
    int mWaitTicks = 0;
    std::vector<glm::vec3> mCurrentPath;
    int mCurrentPathIndex = 0;

public:
    // Настройки
    NumberSetting mSpeed = NumberSetting("Speed", "Movement speed", 1.0f, 0.1f, 10.0f, 0.1f);
    BoolSetting mFlyMode = BoolSetting("Fly Mode", "Fly instead of walk", true);
    NumberSetting mWaitTime = NumberSetting("Wait Time", "Ticks to wait at each point", 0.0f, 0.0f, 200.0f, 1.0f);
    BoolSetting mLoop = BoolSetting("Loop", "Loop between positions infinitely", true);
    NumberSetting mStopDistance = NumberSetting("Stop Distance", "Distance to consider arrived", 1.5f, 0.1f, 5.0f, 0.1f);
    BoolSetting mUsePathfinding = BoolSetting("Use Pathfinding", "Use A* pathfinding (slower but avoids obstacles)", false);
    BoolSetting mRenderPath = BoolSetting("Render Path", "Show path visually", true);

    AutoPatrol() : ModuleBase("AutoPatrol", "Automatically patrol between pos1 and pos2", ModuleCategory::Movement, 0, false)
    {
        addSettings(
            &mSpeed,
            &mFlyMode,
            &mWaitTime,
            &mLoop,
            &mStopDistance,
            &mUsePathfinding,
            &mRenderPath
        );

        mNames = {
            {Lowercase, "autopatrol"},
            {LowercaseSpaced, "auto patrol"},
            {Normal, "AutoPatrol"},
            {NormalSpaced, "Auto Patrol"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onRenderEvent(class RenderEvent& event);
    
    [[nodiscard]] glm::vec3 getCurrentTarget() const;
    void switchTarget();
    void recalculatePath();
};