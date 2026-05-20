#pragma once

#include "../Module.hpp"
#include <chrono>
#include <deque>
#include <unordered_map>
#include <vector>

class FakeChat : public ModuleBase<FakeChat> {
public:
    struct ScheduledMsg {
        std::string text;
        std::chrono::steady_clock::time_point sendAt;
    };

    std::deque<std::string> mHistory;
    static const size_t MAX_HISTORY = 200;

    std::unordered_map<std::string, std::string> mPrefixes;

    std::vector<ScheduledMsg> mQueue;

    bool mDebugMode = false;

    static inline FakeChat* sInstance = nullptr;

    FakeChat() : ModuleBase("FakeChat",
        "Fake chat messages using captured formats",
        ModuleCategory::Misc, 0, false)
    {
        sInstance = this;
        mNames = {
            {Lowercase, "fakechat"},
            {LowercaseSpaced, "fake chat"},
            {Normal, "FakeChat"},
            {NormalSpaced, "Fake Chat"},
        };
    }

    static FakeChat* get() { return sInstance; }

    void onEnable() override;
    void onDisable() override;
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);

    bool findPrefix(const std::string& playerName, const std::string& saidText);
    void schedule(const std::string& fullText, float delaySec);
    void cancelAll();

    // ═══ НОВОЕ ═══
    // Клонирует префикс sourceName → targetName (заменяет ник внутри)
    bool clonePrefix(const std::string& sourceName, const std::string& targetName);
};