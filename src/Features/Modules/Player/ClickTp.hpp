#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Modules/Visual/HudEditor.hpp>

const char* CTGetKeyName(int vk);

class ClickTp : public ModuleBase<ClickTp>
{
public:
    NumberSetting mStepDistance = NumberSetting("Step Distance", "Blocks per TP step", 8.f, 1.f, 12.f, 0.5f);
    NumberSetting mYOffset = NumberSetting("Y Offset", "Extra Y above target", 0.f, 0.f, 5.f, 0.5f);
    NumberSetting mScale = NumberSetting("Panel Scale", "Panel size %", 100.f, 50.f, 150.f, 5.f);
    BoolSetting mDrawPath = BoolSetting("Draw Path", "Draw TP path", true);
    BoolSetting mShowPoints = BoolSetting("Show Points", "Show waypoint panel", true);

    ClickTp() : ModuleBase("ClickTp", "TP to crosshair or saved waypoints",
        ModuleCategory::Player, 0, false)
    {
        addSettings(&mStepDistance, &mYOffset, &mScale, &mDrawPath, &mShowPoints);
        mNames = {
            {Lowercase, "clicktp"},
            {LowercaseSpaced, "click tp"},
            {Normal, "ClickTp"},
            {NormalSpaced, "Click Tp"}
        };
    }

    struct SavedPoint {
        std::string name;
        glm::vec3 position;
        int keybind = 0;
        int index = 0;
    };

    std::vector<SavedPoint> mPoints;

    int mBindingPointIndex = -1;
    bool mIsBindingKey = false;
    bool mIsBindingCrosshairKey = false;
    bool mIsBindingSaveKey = false;
    bool mBindWaitRelease = false;

    int mCrosshairKey = 'T';
    int mSavePointKey = 'G';

    glm::vec3 mRots = {0, 0, 0};
    std::vector<glm::vec3> mPacketPositions;
    uint64_t mLastPathTime = 0;
    std::mutex mMutex;          // Для mPacketPositions И mPoints
    // ^ Теперь защищает оба контейнера!

    std::unordered_map<int, bool> mKeyStates;

    int mRenamingIndex = -1;
    char mRenameBuffer[64] = {};
    bool mRenameJustOpened = false;

    HudElement mPanelElement;
    bool mHudRegistered = false;
    static inline char sHudId[] = "ClickTpPanel";

    void onEnable() override;
    void onDisable() override;

    glm::vec3 findTarget();
    glm::vec3 extendedRaytrace(float maxRange);
    glm::vec3 getTPDestination(glm::vec3 blockPos);
    std::shared_ptr<class MovePlayerPacket> createPacketForPos(glm::vec3 pos);
    void straightLineTP(glm::vec3 from, glm::vec3 to, bool saveForRender);
    void teleportTo(glm::vec3 destination);
    bool isKeyJustPressed(int vk);
    int getNextIndex();

    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
    void onRenderEvent(class RenderEvent& event);

    // ===== CUSTOM DATA — override virtual из Module =====
    bool hasCustomData() const override { return true; }
    nlohmann::json serializeCustomData() override;
    void deserializeCustomData(const nlohmann::json& j) override;
    // УБРАЛИ: nlohmann::json serialize() override; — больше не нужен!

    std::string getSettingDisplay() override {
        if (mIsBindingKey || mIsBindingCrosshairKey || mIsBindingSaveKey) return "Binding...";
        if (mPoints.empty()) return "No points";
        return std::to_string(mPoints.size()) + " pts";
    }
};