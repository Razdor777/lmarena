#pragma once
#include <Features/FeatureManager.hpp>
#include <Features/Events/NotifyEvent.hpp>

class Notifications : public ModuleBase<Notifications> {
public:
    enum class Style {
        Modern,
    };

    EnumSettingT<Style> mStyle = EnumSettingT<Style>("Style", "Visual style", Style::Modern, "Modern");
    BoolSetting mShowOnToggle = BoolSetting("Show on toggle", "Show a notification when a module is toggled", true);
    BoolSetting mShowOnJoin = BoolSetting("Show on join", "Show a notification when you join a server", true);
    BoolSetting mLimitNotifications = BoolSetting("Limit notifications", "Limit the number of notifications shown at one time", false);
    NumberSetting mMaxNotifications = NumberSetting("Max notifications", "The maximum number of notifications shown at one time", 6, 1, 25, 1);
    NumberSetting mAnimSpeed = NumberSetting("Anim Speed", "Animation speed multiplier", 10.0f, 2.0f, 25.0f, 0.5f);

    Notifications() : ModuleBase("Notifications", "Shows notifications on module toggle and other events", ModuleCategory::Visual, 0, true) {
        addSettings(&mShowOnToggle, &mShowOnJoin, &mLimitNotifications, &mMaxNotifications, &mAnimSpeed);
        VISIBILITY_CONDITION(mMaxNotifications, mLimitNotifications.mValue == true);
        mNames = {
            {Lowercase, "notifications"},
            {LowercaseSpaced, "notifications"},
            {Normal, "Notifications"},
            {NormalSpaced, "Notifications"}
        };
        gFeatureManager->mDispatcher->listen<RenderEvent, &Notifications::onRenderEvent, nes::event_priority::VERY_LAST>(this);
    }

    std::vector<Notification> mNotifications;
    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(class RenderEvent& event);
    void onModuleStateChange(ModuleStateChangeEvent& event);
    void onConnectionRequestEvent(class ConnectionRequestEvent& event);
    void onNotifyEvent(class NotifyEvent& event);
    std::string getSettingDisplay() override { return "Modern"; }

private:
    void renderModern(ImDrawList* drawList, ImVec2 displaySize, float delta);
    ImColor getTypeColor(Notification::Type type, float offset);
};