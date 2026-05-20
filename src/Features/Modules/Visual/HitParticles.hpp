#pragma once
//
// HitParticles — Particle effects on hit
// Preset-based: no 50 sliders, just pick a style
//

#include <Features/FeatureManager.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <mutex>

struct HitParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
    float maxLifetime;
    float size;
    float rotation;
    float rotationSpeed;
    ImColor color;
    float gravity;
    
    float getProgress() const { return lifetime / maxLifetime; }
    bool isDead() const { return lifetime >= maxLifetime; }
};

class HitParticles : public ModuleBase<HitParticles> {
public:
    enum class Preset {
        Sparks,      // Orange/yellow directional sparks
        Blood,       // Red droplets with gravity
        Stars,       // Rainbow star particles
        Hearts,      // Pink/red heart particles
        Snow,        // White crystal particles
        Fire,        // Fire-colored rising particles
        Shatter,     // Glass shard fragments
        Custom       // Manual settings
    };
    
    EnumSettingT<Preset> mPreset = EnumSettingT<Preset>("Preset", "Particle style",
        Preset::Sparks, "Sparks", "Blood", "Stars", "Hearts", "Snow", "Fire", "Shatter", "Custom");
    
    // Custom-only settings
    NumberSetting mCount = NumberSetting("Count", "Particles per hit", 12, 3, 40, 1);
    NumberSetting mSize = NumberSetting("Size", "Particle size", 3.0f, 1.0f, 10.0f, 0.5f);
    NumberSetting mSpeed = NumberSetting("Speed", "Initial speed", 4.0f, 1.0f, 15.0f, 0.5f);
    NumberSetting mLifetime = NumberSetting("Lifetime", "Particle lifetime", 0.8f, 0.2f, 3.0f, 0.1f);
    NumberSetting mGravity = NumberSetting("Gravity", "Gravity strength", 5.0f, 0.0f, 20.0f, 0.5f);
    BoolSetting mRainbow = BoolSetting("Rainbow", "Rainbow colors", false);
    NumberSetting mColorR = NumberSetting("Red", "Red", 1.0f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorG = NumberSetting("Green", "Green", 0.5f, 0.0f, 1.0f, 0.01f);
    NumberSetting mColorB = NumberSetting("Blue", "Blue", 0.1f, 0.0f, 1.0f, 0.01f);
    BoolSetting mGlow = BoolSetting("Glow", "Add glow to particles", true);
    
    HitParticles() : ModuleBase("HitParticles", "Spawn particles when hitting entities", ModuleCategory::Visual, 0, false) {
        addSettings(
            &mPreset,
            &mCount, &mSize, &mSpeed, &mLifetime, &mGravity,
            &mRainbow, &mColorR, &mColorG, &mColorB, &mGlow
        );
        
        // Hide custom settings when using presets
        auto isCustom = [this]() { return mPreset.mValue == Preset::Custom; };
        VISIBILITY_CONDITION(mCount, isCustom());
        VISIBILITY_CONDITION(mSize, isCustom());
        VISIBILITY_CONDITION(mSpeed, isCustom());
        VISIBILITY_CONDITION(mLifetime, isCustom());
        VISIBILITY_CONDITION(mGravity, isCustom());
        VISIBILITY_CONDITION(mRainbow, isCustom());
        VISIBILITY_CONDITION(mColorR, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorG, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mColorB, isCustom() && !mRainbow.mValue);
        VISIBILITY_CONDITION(mGlow, isCustom());
        
        mNames = {
            {Lowercase, "hitparticles"},
            {LowercaseSpaced, "hit particles"},
            {Normal, "HitParticles"},
            {NormalSpaced, "Hit Particles"}
        };
    }
    
    std::vector<HitParticle> mParticles;
    std::mutex mParticleMutex;
    std::mt19937 mRng{std::random_device{}()};
    
    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);
    
private:
    void spawnParticles(const glm::vec3& position);
    void spawnPresetParticles(const glm::vec3& pos, Preset preset);
    
    // Helpers
    float randFloat(float min, float max);
    glm::vec3 randDir();
    ImColor randPresetColor(Preset preset, float variation = 0.0f);
    
    // Shape renderers
    void renderDiamond(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha);
    void renderStar(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha);
    void renderHeart(ImDrawList* dl, ImVec2 center, float size, float rotation, ImColor color, float alpha);
    void renderDot(ImDrawList* dl, ImVec2 center, float size, ImColor color, float alpha, bool glow);
    
    std::string getSettingDisplay() override {
        return mPreset.mValues[mPreset.as<int>()];
    }
};
