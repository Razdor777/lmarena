#pragma once
#include <Features/Modules/Module.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>

class PacketOutEvent;

class Criticals : public ModuleBase<Criticals> {
public:
    enum class Mode { PocketMine, AuthInput };

    EnumSettingT<Mode> mMode = EnumSettingT<Mode>(
        "Mode", "How to fake the fall", Mode::PocketMine, "PocketMine", "AuthInput");
    NumberSetting mHeight = NumberSetting(
        "Height", "Fake jump height (PMMP ~0.1, AuthInput try 0.1-0.42)", 0.1f, 0.01f, 0.5f, 0.01f);
    BoolSetting mOnlyOnGround = BoolSetting(
        "Only On Ground", "Send only when you are standing on the ground", true);

    Criticals() : ModuleBase("Criticals", "Always deal critical hits", ModuleCategory::Combat, 0, false,
        {
            {Lowercase,       "criticals"},
            {LowercaseSpaced, "criticals"},
            {Normal,          "Criticals"},
            {NormalSpaced,    "Criticals"}
        })
    {
        addSettings(&mMode, &mHeight, &mOnlyOnGround);
    }

    void onEnable() override;
    void onDisable() override;
    void onPacketOutEvent(PacketOutEvent& event);

private:
    // Снимок последнего настоящего PlayerAuthInput
    struct AuthSnapshot {
        bool                valid = false;
        glm::vec2           rot{};
        glm::vec3           pos{};
        float               headRot = 0.f;
        glm::vec2           interactRots{};
        glm::vec2           cameraOrientation{};
        InputMode           inputMode = InputMode::Mouse;
        ClientPlayMode      playMode = ClientPlayMode::Normal;
        NewInteractionModel interaction = NewInteractionModel::Touch;
        int64_t             tick = 0;
        uint64_t            predictedVehicle = 0;
    } mSnap;

    bool    mSending = false;      // защита от рекурсии (ловим свои же пакеты)
    int64_t mLastFakeTick = -1;    // чтобы не слать фейки несколько раз за один тик

    void sendFake(float yOffset, float yDelta, AuthInputAction flags);
};
