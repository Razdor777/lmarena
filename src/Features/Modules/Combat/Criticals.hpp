#pragma once
//
// Criticals module — forces every melee hit to be a critical hit.
//
// Uses two complementary methods:
// 1) Intercepts outgoing attack packets and injects MovePlayerPacket
//    spoofs (PositionMode::Teleport) to fake a fall before the hit.
// 2) Modifies PlayerAuthInputPacket position to alternate Y offset,
//    ensuring the server always sees the player as "falling".
//

#include <Features/Modules/Setting.hpp>
#include <Features/Modules/Module.hpp>

class Criticals : public ModuleBase<Criticals>
{
public:
    NumberSetting mYOffset = NumberSetting("Y Offset", "Height of the position spoof",
        0.1f, 0.05f, 0.42f, 0.01f);

    Criticals() : ModuleBase<Criticals>("Criticals", "Every hit deals critical damage", ModuleCategory::Combat, 0, false) {
        mNames = {
            {Lowercase,       "criticals"},
            {LowercaseSpaced, "criticals"},
            {Normal,          "Criticals"},
            {NormalSpaced,    "Criticals"}
        };

        addSettings(&mYOffset);
    }

    void onEnable()  override;
    void onDisable() override;

    void onPacketOutEvent(class PacketOutEvent& event);

private:
    // Tick counter for alternating Y position in AuthInput
    int mTickCounter = 0;
};
