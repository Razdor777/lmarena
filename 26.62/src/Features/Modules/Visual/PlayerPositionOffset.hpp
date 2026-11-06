#pragma once
#include <Features/Modules/Module.hpp>

class PlayerPositionOffset : public ModuleBase<PlayerPositionOffset> {
public:
    NumberSetting mOffsetX = NumberSetting("OffsetX", "Вправо(+)/влево(-)", 0.f, -50.f, 50.f, 0.1f);
    NumberSetting mOffsetY = NumberSetting("OffsetY", "Вверх/вниз", 0.f, -50.f, 50.f, 0.1f);
    NumberSetting mOffsetZ = NumberSetting("OffsetZ", "Вперед(+)/назад(-)", 0.f, -50.f, 50.f, 0.1f);
    NumberSetting mReachOffset = NumberSetting("ReachAdd", "Добавочный рич (вдаль/ближе)", 0.f, -20.f, 20.f, 0.1f);

    BoolSetting mOrbit = BoolSetting("Orbit", "Кружиться вокруг тебя", false);
    NumberSetting mOrbitRadius = NumberSetting("OrbitRadius", "Радиус орбиты", 3.f, 0.5f, 20.f, 0.1f);
    NumberSetting mOrbitSpeed = NumberSetting("OrbitSpeed", "Скорость кружения", 1.f, 0.f, 5.f, 0.1f);

    BoolSetting mSpin = BoolSetting("Spin", "Крутиться на месте (spinbot)", false);
    NumberSetting mSpinSpeed = NumberSetting("SpinSpeed", "Скорость вращения", 10.f, 0.f, 100.f, 1.f);

    PlayerPositionOffset() : ModuleBase("PlayerPositionOffset", "Визуально смещает позицию выбранного игрока", ModuleCategory::Visual, 0, false) {
        addSetting(&mOffsetX);
        addSetting(&mOffsetY);
        addSetting(&mOffsetZ);
        addSetting(&mReachOffset);
        addSetting(&mOrbit);
        addSetting(&mOrbitRadius);
        addSetting(&mOrbitSpeed);
        addSetting(&mSpin);
        addSetting(&mSpinSpeed);
    }

    std::string mTargetPlayer = "";
    int64_t mTargetRuntimeID = -1;

    void setTargetPlayer(const std::string& name, int64_t runtimeID);
    void clearTarget();
    std::string getTargetPlayer() const { return mTargetPlayer; }

    void onEnable() override;
    void onDisable() override;
    void onActorRenderEvent(class ActorRenderEvent& event);
};