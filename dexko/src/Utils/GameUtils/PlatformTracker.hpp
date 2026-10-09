#pragma once
//
// Platform Tracker — определение платформы и устройства игроков
// Данные из PlayerListPacket и ConnectionRequest (наш реверс sub_141B18B30)
//

#include <unordered_map>
#include <mutex>
#include <string>
#include <SDK/Minecraft/Network/Packets/PlayerListPacket.hpp>

class PlatformTracker {
public:
    struct PlayerInfo {
        BuildPlatform platform = BuildPlatform::Unknown;
        std::string deviceModel;      // "Samsung Galaxy S23", "iPhone 15" и т.д.
        int inputMode = 0;            // 1=KB+M, 2=Touch, 3=Controller
        std::string name;
        int64_t runtimeId = -1;
    };

    static PlatformTracker& getInstance() {
        static PlatformTracker instance;
        return instance;
    }

    void addPlayer(int64_t runtimeId, BuildPlatform platform, const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mMutex);
        auto& info = mPlayers[runtimeId];
        info.platform = platform;
        info.runtimeId = runtimeId;
        if (!name.empty()) info.name = name;
    }

    void removePlayer(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        mPlayers.erase(runtimeId);
    }

    BuildPlatform getPlatform(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mPlayers.find(runtimeId);
        if (it != mPlayers.end()) return it->second.platform;
        return BuildPlatform::Unknown;
    }

    PlayerInfo* getPlayerInfo(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mPlayers.find(runtimeId);
        if (it != mPlayers.end()) return &it->second;
        return nullptr;
    }

    std::string getPlatformName(int64_t runtimeId) {
        auto platform = getPlatform(runtimeId);
        auto it = BuildPlatformNames.find(platform);
        if (it != BuildPlatformNames.end()) return it->second;
        return "Unknown";
    }

    std::string getPlatformIcon(int64_t runtimeId) {
        switch (getPlatform(runtimeId)) {
            case BuildPlatform::Android:      return "PE";
            case BuildPlatform::iOS:          return "PE";
            case BuildPlatform::WIN10:        return "W10";
            case BuildPlatform::Win32:        return "W32";
            case BuildPlatform::Xbox:         return "Xbox";
            case BuildPlatform::PS4:          return "PS";
            case BuildPlatform::Nx:           return "NS";
            case BuildPlatform::OSX:          return "Mac";
            case BuildPlatform::Linux:        return "Lnx";
            case BuildPlatform::Amazon:       return "Amz";
            case BuildPlatform::GearVR:       return "VR";
            case BuildPlatform::Dedicated:    return "Srv";
            case BuildPlatform::WindowsPhone: return "WP";
            default:                          return "?";
        }
    }

    // Получить цвет для платформы (для визуального различия)
    ImU32 getPlatformColor(int64_t runtimeId) {
        switch (getPlatform(runtimeId)) {
            case BuildPlatform::Android:      return IM_COL32(164, 198, 57, 255);  // зелёный Android
            case BuildPlatform::iOS:          return IM_COL32(150, 150, 150, 255);  // серый Apple
            case BuildPlatform::WIN10:        return IM_COL32(0, 120, 215, 255);    // синий Windows
            case BuildPlatform::Win32:        return IM_COL32(0, 120, 215, 255);    // синий Windows
            case BuildPlatform::Xbox:         return IM_COL32(16, 124, 16, 255);    // зелёный Xbox
            case BuildPlatform::PS4:          return IM_COL32(0, 55, 145, 255);     // синий PS
            case BuildPlatform::Nx:           return IM_COL32(230, 0, 18, 255);     // красный Nintendo
            case BuildPlatform::OSX:          return IM_COL32(150, 150, 150, 255);  // серый Apple
            case BuildPlatform::Linux:        return IM_COL32(255, 165, 0, 255);    // оранжевый Linux
            default:                          return IM_COL32(180, 180, 180, 255);  // серый
        }
    }

    // Определяем тип ввода (Touch/KB+M/Controller)
    std::string getInputIcon(int64_t runtimeId) {
        auto platform = getPlatform(runtimeId);
        switch (platform) {
            case BuildPlatform::Android:
            case BuildPlatform::iOS:
            case BuildPlatform::Amazon:
            case BuildPlatform::WindowsPhone:
                return "Touch";
            case BuildPlatform::Xbox:
            case BuildPlatform::PS4:
            case BuildPlatform::Nx:
            case BuildPlatform::GearVR:
                return "Ctrl";
            case BuildPlatform::WIN10:
            case BuildPlatform::Win32:
            case BuildPlatform::OSX:
            case BuildPlatform::Linux:
                return "KB+M";
            default:
                return "?";
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        mPlayers.clear();
    }

private:
    PlatformTracker() = default;
    std::mutex mMutex;
    std::unordered_map<int64_t, PlayerInfo> mPlayers;
};