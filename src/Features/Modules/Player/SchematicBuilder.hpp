#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <set>
#include <map>
#include "Schematic.hpp"

class SchematicBuilder : public ModuleBase<SchematicBuilder>
{
public:
    enum class State {
        Idle,
        Previewing,
        Clearing,
        Building,
        Walking,
        WaitingForBlocks
    };

    // === НАСТРОЙКИ ===
    NumberSetting mBuildDelay = NumberSetting("Build Delay", "Delay between blocks (ms)", 150.0f, 0.0f, 300.0f, 10.0f);
    NumberSetting mBuildRange = NumberSetting("Build Range", "Max range to place blocks", 4.5f, 3.0f, 6.0f, 0.5f);
    NumberSetting mWalkSpeed = NumberSetting("Walk Speed", "Speed when walking to blocks", 4.0f, 1.0f, 10.0f, 0.5f);
    NumberSetting mPreviewLimit = NumberSetting("Preview Limit", "Max blocks to render in preview", 15.0f, 10.0f, 50.0f, 1.0f);
    NumberSetting mPreviewRadius = NumberSetting("Preview Radius", "Max distance to render blocks", 16.0f, 8.0f, 32.0f, 1.0f);
    
    BoolSetting mAutoWalk = BoolSetting("Auto Walk", "Walk to unreachable blocks", true);
    BoolSetting mAutoClear = BoolSetting("Auto Clear", "Clear area before building", true);
    BoolSetting mShowPreview = BoolSetting("Show Preview", "Show ghost blocks", true);
    BoolSetting mShowProgress = BoolSetting("Show Progress", "Show progress bar", true);
    BoolSetting mShowSelection = BoolSetting("Show Selection", "Show selection box", true);
    BoolSetting mShowMissingBlocks = BoolSetting("Show Missing", "Show missing blocks HUD", true);
    BoolSetting mSwing = BoolSetting("Swing", "Swing arm", true);
    BoolSetting mVerifyPlacement = BoolSetting("Verify Placement", "Double-check block placement", true);
    BoolSetting mExcludeGrass = BoolSetting("Exclude Grass", "Exclude grass/plants when copying", true);

    SchematicBuilder() : ModuleBase("SchematicBuilder", "Copy and paste structures", ModuleCategory::Player, 0, false) {
        addSettings(
            &mBuildDelay,
            &mBuildRange,
            &mWalkSpeed,
            &mPreviewLimit,
            &mPreviewRadius,
            &mAutoWalk,
            &mAutoClear,
            &mShowPreview,
            &mShowProgress,
            &mShowSelection,
            &mShowMissingBlocks,
            &mSwing,
            &mVerifyPlacement,
            &mExcludeGrass
        );

        VISIBILITY_CONDITION(mWalkSpeed, mAutoWalk.mValue);
        VISIBILITY_CONDITION(mPreviewLimit, mShowPreview.mValue);
        VISIBILITY_CONDITION(mPreviewRadius, mShowPreview.mValue);

        mNames = {
            {Lowercase, "schematicbuilder"},
            {LowercaseSpaced, "schematic builder"},
            {Normal, "SchematicBuilder"},
            {NormalSpaced, "Schematic Builder"}
        };
    }

    // === СОСТОЯНИЕ ===
    State mState = State::Idle;
    Schematic mClipboard;
    
    static inline glm::ivec3 sPos1 = glm::ivec3(0);
    static inline glm::ivec3 sPos2 = glm::ivec3(0);
    static inline bool sPos1Set = false;
    static inline bool sPos2Set = false;
    
    glm::ivec3 mPastePosition = glm::ivec3(0);
    bool mHasPastePosition = false;
    
    // Build queue
    std::vector<SchematicBlock> mBuildQueue;
    std::set<int> mCompletedIndices;
    std::set<int> mSkippedIndices;
    
    // Clear queue
    std::vector<glm::ivec3> mClearQueue;
    int mClearIndex = 0;
    
    int mTotalBlocksToBuild = 0;
    int mBlocksPlaced = 0;
    int mBlocksVerified = 0;
    int mBlocksCleared = 0;
    uint64_t mLastActionTime = 0;
    
    // Stages
    int mCurrentStage = 0;
    std::set<std::string> mAvailableBlocks;
    int mBlocksInCurrentStage = 0;
    int mBlocksBuiltInStage = 0;
    
    // Walking
    glm::vec3 mWalkTarget = glm::vec3(0);
    bool mIsWalking = false;
    int mWalkTicks = 0;
    
    // Verification
    glm::ivec3 mLastPlacedPos = glm::ivec3(0);
    std::string mLastPlacedBlockName = "";
    uint64_t mLastPlacedTime = 0;
    bool mWaitingForVerification = false;
    int mCurrentBuildIdx = -1;
    
    // Check timer
    uint64_t mLastInventoryCheck = 0;
    
    // Missing blocks cache
    std::map<std::string, int> mMissingBlocksCache;
    uint64_t mLastMissingBlocksUpdate = 0;
    
    // === МЕТОДЫ ===
    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onBaseTickEvent(BaseTickEvent& event);
    
    // Selection
    bool hasValidSelection();
    glm::ivec3 getSelectionMin();
    glm::ivec3 getSelectionMax();
    glm::ivec3 getSelectionSize();
    
    // Operations
    bool copySelection();
    bool preparePaste(glm::ivec3 position);
    void startBuilding();
    void stopBuilding();
    void cancelPaste();
    
    // Processing
    void processClearing();
    void processBuilding();
    void processWalking();
    void processVerification();
    void processWaitingForBlocks();
    
    // Preparation
    void prepareClearQueue();
    void updateAvailableBlocks();
    void startNextStage();
    void updateMissingBlocks();
    
    // Helpers
    int findNextBuildableIndex();
    int findBlockToWalkTo();
    bool canBuildAt(int index);
    bool isInRange(glm::ivec3 pos);
    bool isBlockInHotbar(const std::string& blockName);
    int countRemainingBlocksForCurrentStage();
    int countRemainingBlocksTotal();
    bool isGrassOrPlant(const std::string& blockName);
    
    // Inventory
    int findBlockInInventory(const std::string& blockName);
    int findBlockInFullInventory(const std::string& blockName);
    bool tryPlaceBlock(int queueIndex);
    bool tryBreakBlock(glm::ivec3 pos);
    
    // Movement
    void walkTowards(glm::vec3 target);
    glm::vec3 findWalkPosition(glm::ivec3 targetBlock);
    
    // Rendering
    void renderSelection();
    void renderPreview();
    void renderProgress();
    void renderWalkTarget();
    void renderClearArea();
    void renderMissingBlocks();

    std::string getSettingDisplay() override {
        switch (mState) {
            case State::Idle: 
                if (mClipboard.isEmpty()) return "Empty";
                return fmt::format("{} blocks", mClipboard.getTotalBlocks());
            case State::Previewing: return "Preview";
            case State::Clearing:
                return fmt::format("Clear {}/{}", mClearIndex, (int)mClearQueue.size());
            case State::WaitingForBlocks:
                return fmt::format("Wait S{}", mCurrentStage);
            case State::Building: 
            case State::Walking:
                if (mTotalBlocksToBuild == 0) return "0%";
                return fmt::format("S{} {}%", mCurrentStage, (mBlocksVerified * 100) / mTotalBlocksToBuild);
            default: return "";
        }
    }
};