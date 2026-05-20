#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>
#include <set>
#include <deque>
#include <fstream>
#include <nlohmann/json.hpp>

class InfiniteChestAura : public ModuleBase<InfiniteChestAura> {
public:
    // === Settings (14) ===
    NumberSetting mSearchRadius = NumberSetting("Search Radius", "Max search distance", 50.f, 10.f, 200.f, 1.f);
    NumberSetting mStepDistance = NumberSetting("Step Distance", "Blocks per TP step", 8.f, 1.f, 12.f, 0.5f);
    NumberSetting mWaitAfterTP = NumberSetting("Wait After TP", "Delay after TP before opening (ms)", 200.f, 50.f, 500.f, 10.f);
    BoolSetting mReturnBack = BoolSetting("Return Back", "TP back after looting", true);
    BoolSetting mIgnoreOpened = BoolSetting("Ignore Opened", "Skip looted chests", true);
    BoolSetting mIgnoreTrapped = BoolSetting("Ignore Trapped", "Skip trapped chests", false);
    BoolSetting mIgnoreEnder = BoolSetting("Ignore Ender", "Skip ender chests", false);
    BoolSetting mPersistentMemory = BoolSetting("Persistent Memory", "Save memory to file", true);
    BoolSetting mEventMode = BoolSetting("Event Mode", "Wait for chat trigger between cycles", false);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Show TP path", true);
    BoolSetting mRenderTarget = BoolSetting("Render Target", "Highlight target chest", true);
    BoolSetting mClearMemory = BoolSetting("Clear Memory", "Clear chest memory", false);
    BoolSetting mForceStart = BoolSetting("Force Start", "Force start scanning", false);
    BoolSetting mDebug = BoolSetting("Debug", "Debug messages + timings", false);

    InfiniteChestAura() : ModuleBase("InfiniteChestAura",
        "TP to chests, loot them, TP back", ModuleCategory::Player, 0, false)
    {
        addSettings(
            &mSearchRadius, &mStepDistance, &mWaitAfterTP,
            &mReturnBack, &mIgnoreOpened, &mIgnoreTrapped, &mIgnoreEnder,
            &mPersistentMemory, &mEventMode,
            &mDrawPath, &mRenderTarget,
            &mClearMemory, &mForceStart, &mDebug
        );

        mNames = {
            {Lowercase, "infinitechestaura"},
            {LowercaseSpaced, "infinite chest aura"},
            {Normal, "InfiniteChestAura"},
            {NormalSpaced, "Infinite Chest Aura"}
        };
    }

    // === State Machine (6 states) ===
    enum class State {
        Idle,
        WaitAfterTP,
        OpeningChest,
        WaitingForOpen,
        WaitingForStealer,
        WaitingForEvent
    };

    // === Core State ===
    State mState = State::Idle;
    glm::vec3 mOriginalPos{0};
    glm::vec3 mTargetChestPos{0};
    glm::ivec3 mTargetChestBlock{0};
    bool mIsChestOpen = false;
    bool mHasTarget = false;
    bool mChestStealerWasEnabled = false;
    int mChestsLooted = 0;
    uint64_t mStateStartTime = 0;
    bool mNeedsQuickScan = true;

    // === Cycle Timing (Debug) ===
    struct CycleTiming {
        uint64_t cycleStart = 0;
        uint64_t tpToStart = 0;
        uint64_t tpToEnd = 0;
        uint64_t waitEnd = 0;
        uint64_t openEnd = 0;
        uint64_t stealEnd = 0;
        uint64_t tpBackEnd = 0;
        float distance = 0.f;
        glm::ivec3 chestPos{0};
        int cycleNumber = 0;
    } mTiming;

    // === Chunk Scanner (OreMiner style) ===
    struct ScanState {
        ChunkPos center;
        ChunkPos current;
        int subChunkIdx = 0;
        int dirIdx = 0;
        int steps = 1;
        int stepCount = 0;
    } mScan;

    static constexpr float CHUNK_RADIUS = 10.f;
    static constexpr int CHUNKS_PER_TICK = 5;
    static constexpr int MAX_MEMORY = 200;

    struct FoundChest {
        glm::ivec3 position;
        int blockId;
    };
    std::unordered_map<BlockPos, FoundChest> mFoundChests;

    // === Opened chests memory ===
    std::set<glm::ivec3, std::function<bool(const glm::ivec3&, const glm::ivec3&)>> mOpenedChests{
        [](const glm::ivec3& a, const glm::ivec3& b) {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            return a.z < b.z;
        }
    };
    std::deque<glm::ivec3> mOpenedChestsOrder;

    // === TP path render ===
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;

    // === Block IDs ===
    static constexpr int CHEST_ID = 54;
    static constexpr int ENDER_CHEST_ID = 130;
    static constexpr int TRAPPED_CHEST_ID = 146;
    static constexpr int BARREL_ID = 458;

    // === Event trigger ===
    std::vector<std::string> mTriggerKeywords = {"airdrop", "air drop"};

    // === Methods ===
    void onEnable() override;
    void onDisable() override;

    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    void onRenderEvent(RenderEvent& event);

    // Scanner
    void resetScanner();
    void moveToNextChunk();
    bool scanSubChunk(ChunkPos chunk, int subIdx);
    void quickScan();
    void runChunkScanner();
    void cleanupFoundChests();
    bool isValidChest(int blockId);

    // TP (AutoLoot style)
    std::shared_ptr<MovePlayerPacket> createPacketForPos(Actor* player, glm::vec3 pos);
    void straightLineTP(Actor* player, glm::vec3 from, glm::vec3 to, bool saveForRender);

    // Chest ops
    glm::ivec3 findNearestChest(Actor* player);
    void openChest(Actor* player, glm::ivec3 pos);
    void closeChest();

    // Memory
    bool isChestOpened(glm::ivec3 pos);
    void markChestAsOpened(glm::ivec3 pos);
    void saveChestMemory();
    void loadChestMemory();
    std::string getMemoryFilePath();

    // Utils
    void setState(State newState);
    uint64_t getStateTime();
    std::string getStateName(State state);
    bool containsTriggerWord(const std::string& message);
    void logCycleTiming();

    std::string getSettingDisplay() override {
        if (mState == State::WaitingForEvent) return "Waiting";
        if (mHasTarget) return getStateName(mState);
        return fmt::format("{} found", mFoundChests.size());
    }
};