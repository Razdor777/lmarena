#pragma once

//
// Tracers — перенос из combatant-client (visuals/Tracers.java).
//   Lines:  линия от точки в 10 блоках перед камерой до каждого игрока.
//   Arrows: стрелки по кругу вокруг прицела, указывающие на игроков.
//           Радиус зависит от дистанции и (опционально) растёт при движении.
//

#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

class Tracers : public ModuleBase<Tracers>
{
public:
    enum class Mode {
        Lines,
        Arrows
    };

    EnumSettingT<Mode> mMode = EnumSettingT<Mode>(
        "Mode", "Lines to players or arrows around the crosshair", Mode::Lines, "Lines", "Arrows");
    BoolSetting mTraceFriends = BoolSetting("Trace Friends", "Also draw tracers to friends", false);
    BoolSetting mMoveRadiusBonus = BoolSetting("Move Radius Bonus", "Arrow circle wobbles and grows while you move", false);

    Tracers() : ModuleBase("Tracers", "Lines or arrows pointing to other players", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mMode,
            &mTraceFriends,
            &mMoveRadiusBonus
        );

        VISIBILITY_CONDITION(mMoveRadiusBonus, mMode.mValue == Mode::Arrows);

        mNames = {
            {Lowercase, "tracers"},
            {LowercaseSpaced, "tracers"},
            {Normal, "Tracers"},
            {NormalSpaced, "Tracers"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override
    {
        return mMode.mValues[mMode.as<int>()];
    }

private:
    float mAnimStep = 0.f;   // сглаженный радиус круга стрелок
    float mAnimYaw = 0.f;    // сглаженный сдвиг круга при движении (X)
    float mAnimPitch = 0.f;  // сглаженный сдвиг круга при движении (Y)
    float mYaw = 0.f;        // сглаженный yaw камеры

    void drawLines(class Actor* localPlayer, const glm::vec3& camPos, const glm::vec3& look);
    void drawArrows(class Actor* localPlayer, const glm::vec3& camPos, float yaw, float dt);
};
