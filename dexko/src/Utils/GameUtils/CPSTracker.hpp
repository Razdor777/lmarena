#pragma once

#include <unordered_map>
#include <deque>
#include <mutex>
#include <chrono>

class CPSTracker {
public:
    static CPSTracker& getInstance() {
        static CPSTracker instance;
        return instance;
    }

    // Вызывать каждый тик для каждого игрока
    void updatePlayer(int64_t runtimeId, bool isSwinging, int swingProgress) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto now = getNow();

        auto& state = mStates[runtimeId];
        bool detected = false;

        if (isSwinging) {
            // Способ 1: Начало нового свинга (false → true)
            if (!state.wasSwinging) {
                detected = true;
            }
            // Способ 2: Прогресс сбросился (был больше, стал меньше)
            // = новый клик ДО завершения предыдущего свинга
            else if (swingProgress < state.prevProgress && state.prevProgress > 1) {
                detected = true;
            }
        }

        if (detected) {
            state.clickTimes.push_back(now);
        }

        state.wasSwinging = isSwinging;
        state.prevProgress = swingProgress;
        state.lastUpdate = now;

        // Убираем клики старше 1 секунды
        while (!state.clickTimes.empty() && (now - state.clickTimes.front()) > 1000) {
            state.clickTimes.pop_front();
        }
    }

    // Для совместимости со старым кодом
    void registerSwing(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        mStates[runtimeId].clickTimes.push_back(getNow());
        cleanupOld(runtimeId);
    }

    int getCPS(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mStates.find(runtimeId);
        if (it == mStates.end()) return 0;
        cleanupOld(runtimeId);
        return static_cast<int>(it->second.clickTimes.size());
    }

    void removePlayer(int64_t runtimeId) {
        std::lock_guard<std::mutex> lock(mMutex);
        mStates.erase(runtimeId);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        mStates.clear();
    }

private:
    CPSTracker() = default;

    struct PlayerState {
        bool wasSwinging = false;
        int prevProgress = 0;
        int64_t lastUpdate = 0;
        std::deque<int64_t> clickTimes;
    };

    int64_t getNow() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    void cleanupOld(int64_t runtimeId) {
        auto now = getNow();
        auto& clicks = mStates[runtimeId].clickTimes;
        while (!clicks.empty() && (now - clicks.front()) > 1000) {
            clicks.pop_front();
        }
    }

    std::mutex mMutex;
    std::unordered_map<int64_t, PlayerState> mStates;
};