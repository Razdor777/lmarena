#pragma once
//
// Criticals — каждый удар критический.
//
// Метод: меняем клиентские компоненты (FallDistance, OnGround, velocity.y)
// ДО того как движок собирает PlayerAuthInputPacket.
// Сервер получает легитимный пакет с признаками падения = крит.
//

#include <Features/Modules/Setting.hpp>
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>

class Criticals : public ModuleBase<Criticals>
{
public:
    NumberSetting mYOffset = NumberSetting(
        "Y Offset", "Fall offset per tick", 0.11f, 0.05f, 0.42f, 0.01f);

    Criticals() : ModuleBase<Criticals>(
        "Criticals", "Every hit deals critical damage", ModuleCategory::Combat, 0, false)
    {
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
    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);

private:
    int   mTickCounter    = 0;
    bool  mWasOnGround    = true;
    float mSavedVelocityY = 0.f;
    bool  mAttackThisTick = false;
};
