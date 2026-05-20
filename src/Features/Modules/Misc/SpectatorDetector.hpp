#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <map>

class SpectatorDetector : public ModuleBase<SpectatorDetector>
{
public:
    // ── Настройки ──────────────────────────────────────────────────────────
    BoolSetting mSpoofCreative = BoolSetting(
        "Spoof Creative",
        "Make spectators appear as Creative players (visible, solid, not transparent)",
        true);
    BoolSetting mNotify = BoolSetting(
        "Notify",
        "Show notification when spectator is detected",
        true);
    BoolSetting mShowList = BoolSetting(
        "Show List",
        "Show spectator list on screen",
        true);
    BoolSetting mLogPackets = BoolSetting(
        "Log Packets",
        "Log GameType changes to chat",
        false);

    SpectatorDetector() : ModuleBase("SpectatorDetector",
        "Detects and optionally reveals Spectator-mode players as Creative",
        ModuleCategory::Misc, 0, false)
    {
        addSettings(&mSpoofCreative, &mNotify, &mShowList, &mLogPackets);
        mNames = {
            {Lowercase,       "spectatordetector"},
            {LowercaseSpaced, "spectator detector"},
            {Normal,          "SpectatorDetector"},
            {NormalSpaced,    "Spectator Detector"}
        };
    }

    // ── Данные ─────────────────────────────────────────────────────────────
    // id → имя: все игроки у которых мы видели Spectator пакет
    std::map<int64_t, std::string> mSpectators;

    // ── Методы ─────────────────────────────────────────────────────────────
    void onEnable()  override;
    void onDisable() override;
    void onPacketInEvent(PacketInEvent&  event);
    void onBaseTickEvent(BaseTickEvent&  event);
    void onRenderEvent(RenderEvent&      event);

    static std::string getNameById(int64_t id);
};