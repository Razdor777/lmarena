#pragma once
#include <SDK/Minecraft/Actor/EntityId.hpp>

#include "HudEditor.hpp"
//
// Created by vastrakai on 8/4/2024.
//


class TargetHUD : public ModuleBase<TargetHUD> {
public:
    enum class Style {
        Solstice,   // full card: head, name, health bar, hp text
        Glass,      // wider frosted card with accent edge and glow
        Minimal,    // no card background, just text + bar
    };

    enum class BarColor {
        Theme,      // taken from the Interface theme (flowing gradient)
        Health,     // green -> yellow -> red by remaining health
    };

    EnumSettingT<Style> mStyle = EnumSettingT("Style", "The style of the target HUD", Style::Solstice, "Solstice", "Glass", "Minimal");
    NumberSetting mXOffset = NumberSetting("X Offset", "The X offset of the target HUD", 100, -400, 400, 1);
    NumberSetting mYOffset = NumberSetting("Y Offset", "The Y offset of the target HUD", 100, -400, 400, 1);
    NumberSetting mFontSize = NumberSetting("Font Size", "The size of the font", 20, 1, 40, 1);
    BoolSetting mHealthCalculation = BoolSetting("Health Calculation", "Calculate health", false);

    BoolSetting mShowHead = BoolSetting("Head", "Show the target's head", true);
    BoolSetting mShowHealthText = BoolSetting("HP Text", "Show the numeric health and absorption", true);
    BoolSetting mDamageTrail = BoolSetting("Damage Trail", "Lingering red chunk that shows health just lost", true);
    EnumSettingT<BarColor> mBarColor = EnumSettingT("Bar Color", "How the health bar is colored", BarColor::Theme, "Theme", "Health");
    BoolSetting mGlow = BoolSetting("Glow", "Soft themed glow around the card", true);
    BoolSetting mHurtFlash = BoolSetting("Hurt Flash", "Red tint and shake when the target takes damage", true);
    BoolSetting mShimmer = BoolSetting("Shimmer", "Light sweep when the HUD appears or the target changes", true);
    NumberSetting mBackgroundAlpha = NumberSetting("Background Alpha", "Opacity of the card background", 0.65f, 0.0f, 1.0f, 0.05f);

    TargetHUD();


    struct TargetTextureHolder {
        ID3D11ShaderResourceView* texture = nullptr;
        bool loaded = false;
        EntityId associatedEntity = EntityId();
    };

    float mHealth = 0;
    float mMaxHealth = 0;
    float mLastHealth = 0;
    float mLastMaxHealth = 0;
    float mAbsorption = 0;
    float mMaxAbsorption = 0;
    float mLastAbsorption = 0;
    float mLastMaxAbsorption = 0;
    float mLerpedHealth = 0;
    float mLerpedAbsorption = 0;
    std::string mLastPlayerName = "";
    float mLastHurtTime = 0;
    float mHurtTime = 0;
    Actor* mLastTarget = nullptr;
    std::map<Actor*, TargetTextureHolder> mTargetTextures;
    constexpr static uint64_t cHurtTimeDuration = 500;

    // ---- animation state ----
    float mAnim = 0.f;            // 0 = hidden, 1 = fully shown
    float mPop = 1.f;             // entrance pop (easeOutBack)
    float mGhostPercent = 1.f;    // lagging health fraction for the damage trail
    float mPrevHealthPercent = 1.f;
    float mGhostDelay = 0.f;      // time the trail holds before draining
    float mFlash = 0.f;           // smoothed hurt flash 0..1
    float mShimmerPos = 1.5f;     // light sweep position
    int64_t mLastTargetId = 0;

    // Health Calculation
    struct HealthInfo {
        float health = 20;
        float lastAbsorption = 0;
        float damage = 1;
    };
    std::map<std::string, HealthInfo> mHealths;
    uint64_t mLastHealTime = 0;

    std::unique_ptr<HudElement> mElement;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void validateTextures();
    void calculateHealths();
    ID3D11ShaderResourceView* getActorSkinTex(Actor* actor);
    void onRenderEvent(class RenderEvent& event);
    void onPacketInEvent(class PacketInEvent& event);

private:
    void renderTargetHud(ImDrawList* drawList, Actor* target, bool hasTarget);
};
