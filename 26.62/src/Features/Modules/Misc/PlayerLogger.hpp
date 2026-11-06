#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <vector>
#include <unordered_set>

struct PlayerLogEntry {
    std::string name;
    std::string xuid;
    uint64_t joinTime = 0;
    uint64_t leaveTime = 0;
    bool isOnline = true;
};

class PlayerLogger : public ModuleBase<PlayerLogger>
{
public:
    BoolSetting mNotifyJoin = BoolSetting("Notify Join", "Notification when player joins", true);
    BoolSetting mNotifyLeave = BoolSetting("Notify Leave", "Notification when player leaves", true);
    BoolSetting mLogToChat = BoolSetting("Log to Chat", "Print join/leave to chat", false);

    PlayerLogger() : ModuleBase("PlayerLogger", "Logs player joins and leaves",
        ModuleCategory::Misc, 0, false)
    {
        addSettings(&mNotifyJoin, &mNotifyLeave, &mLogToChat);
        mNames = {
            {Lowercase, "playerlogger"}, {LowercaseSpaced, "player logger"},
            {Normal, "PlayerLogger"}, {NormalSpaced, "Player Logger"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);

private:
    std::unordered_map<std::string, PlayerLogEntry> mPlayerLog; // xuid -> entry
    std::unordered_set<std::string> mKnownPlayers; // Names currently online
    bool mFirstScan = true;
};
