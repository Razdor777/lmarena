#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>

class ArrowTP : public ModuleBase<ArrowTP>
{
public:
    NumberSetting mStepDistance = NumberSetting("Step Distance", "TP step size", 8.f, 1.f, 12.f, 0.5f);
    BoolSetting mTPBack = BoolSetting("TP Back", "TP back after delay", false);
    NumberSetting mTPBackDelay = NumberSetting("TP Back Delay", "Ticks before TP back", 5, 1, 40, 1);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw TP path", true);
    BoolSetting mDrawGhost = BoolSetting("Draw Ghost", "Ghost at destination", true);
    NumberSetting mTrackWindow = NumberSetting("Track Window", "Seconds after shot", 15.f, 2.f, 30.f, 1.f);
    NumberSetting mMinFlightTicks = NumberSetting("Min Flight", "Min ticks arrow must fly before TP", 10.f, 3.f, 40.f, 1.f);
    NumberSetting mMinFlightDist = NumberSetting("Min Fly Dist", "Min blocks arrow must travel", 3.f, 1.f, 20.f, 0.5f);

    ArrowTP() : ModuleBase("ArrowTP", "TP to where your arrow lands", ModuleCategory::Combat, 0, false)
    {
        addSettings(&mStepDistance, &mTPBack, &mTPBackDelay, &mDrawPath, &mDrawGhost,
                    &mTrackWindow, &mMinFlightTicks, &mMinFlightDist);
        VISIBILITY_CONDITION(mTPBackDelay, mTPBack.mValue);
        mNames = {{Lowercase,"arrowtp"},{LowercaseSpaced,"arrow tp"},{Normal,"ArrowTP"},{NormalSpaced,"Arrow TP"}};
    }

    struct TrackedArrow {
        glm::vec3 spawnPos;       // Где стрела спавнилась
        glm::vec3 lastPos;        // Последняя позиция
        glm::vec3 prevPos;        // Предыдущая позиция (для определения движения)
        int aliveTicks = 0;       // Сколько тиков отслеживаем
        int stuckTicks = 0;       // Сколько тиков стоит на месте
        int missedTicks = 0;      // Сколько тиков пропало (для определения попадания в entity)
        bool hasFlown = false;    // Стрела уже ЛЕТЕЛА? (скорость была > 0)
        float maxDistFromSpawn = 0.f; // Максимальное расстояние от спавна
        bool tpDone = false;
    };

    struct PendingTPBack {
        glm::vec3 originalPos;
        int ticksRemaining = 0;
        bool active = false;
    };

    std::unordered_map<uint64_t, TrackedArrow> mTrackedArrows;
    PendingTPBack mPendingBack = {};
    glm::vec3 mRots = {0, 0, 0};
    uint64_t mLastBowRelease = 0;
    bool mBowDrawing = false;
    bool mHeldBow = false;
    bool mDidTP = false;

    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    glm::vec3 mGhostPos = {0, 0, 0};
    bool mGhostVisible = false;
    std::mutex mMutex;
    std::string mStatus = "Ready";

    void onEnable() override;
    void onDisable() override;
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool save);
    void performTP(glm::vec3 dest);
    bool isHoldingBow(Actor* player);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onRenderEvent(class RenderEvent& event);

    std::string getSettingDisplay() override {
        if (mBowDrawing) return "Drawing...";
        if (!mTrackedArrows.empty()) return "Tracking " + std::to_string(mTrackedArrows.size());
        return "Ready";
    }
};