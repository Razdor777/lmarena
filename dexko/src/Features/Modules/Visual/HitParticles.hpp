#pragma once
//
// HitParticles — rewritten from scratch.
//
// The old version called Level::addParticle, which internally does
// `base + 0x32A4800` — a hard-coded build address that no longer resolves,
// so nothing ever spawned. This version owns its particles completely and
// renders them world->screen through RenderUtils, exactly like Kagune does.
//

#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class HitParticles : public ModuleBase<HitParticles> {
public:
    enum class Style {
        Blood,      // тёмно-красные брызги, как у Кагуне
        Sparks,     // быстрые жёлто-белые искры
        Critical,   // бело-голубые криты
        Hearts,     // сердечки, всплывают вверх
        Fire,       // угли + пламя, поднимаются
        Frost,      // ледяная крошка, медленно оседает
        Soul,       // бирюзовые сгустки с длинным шлейфом
        Toxic       // кислотные пузыри
    };

    enum class ColorMode {
        StyleColor, // палитра выбранного стиля
        ThemeColor, // цвет из текущей темы клиента (ColorUtils)
        Custom      // свой цвет
    };

    EnumSettingT<Style> mStyle = EnumSettingT<Style>(
        "Style", "How the hit burst looks",
        Style::Blood,
        "Blood", "Sparks", "Critical", "Hearts", "Fire", "Frost", "Soul", "Toxic");

    EnumSettingT<ColorMode> mColorMode = EnumSettingT<ColorMode>(
        "Color Mode", "Where particle colors come from",
        ColorMode::StyleColor,
        "Style Color", "Theme Color", "Custom");

    ColorSetting mCustomColor = ColorSetting(
        "Custom Color", "Used when Color Mode = Custom", 1.0f, 0.12f, 0.10f, 1.0f);

    NumberSetting mAmount   = NumberSetting("Amount",   "Particles per hit", 12.0f, 1.0f, 15.0f, 1.0f);
    NumberSetting mSpread   = NumberSetting("Spread",   "Spawn radius around the contact point", 0.30f, 0.02f, 1.50f, 0.01f);
    NumberSetting mSpeed    = NumberSetting("Speed",    "How hard particles fly out", 4.5f, 0.2f, 14.0f, 0.1f);
    NumberSetting mSize     = NumberSetting("Size",     "Base particle size", 3.5f, 0.5f, 14.0f, 0.1f);
    NumberSetting mGravity  = NumberSetting("Gravity",  "Downward pull (>0 falls, <0 floats up)", 1.0f, -0.8f, 3.0f, 0.05f);
    NumberSetting mDrag     = NumberSetting("Drag",     "Air resistance", 2.0f, 0.0f, 8.0f, 0.1f);
    NumberSetting mLifetime = NumberSetting("Lifetime", "Seconds until fully faded", 0.90f, 0.10f, 3.0f, 0.05f);

    BoolSetting mDirectional = BoolSetting("Directional",   "Spray away from your eyes like a real impact splash", true);
    BoolSetting mGlow        = BoolSetting("Glow",          "Soft additive halo around every particle", true);
    BoolSetting mTrails      = BoolSetting("Trails",        "Motion streak behind fast particles", true);
    BoolSetting mFlash       = BoolSetting("Impact Flash",  "Expanding ring at the contact point", true);
    BoolSetting mSecondBurst = BoolSetting("Second Burst",  "Delayed wide splash that follows the first", true);

    HitParticles() : ModuleBase("HitParticles", "Custom particle burst wherever you land a hit", ModuleCategory::Visual, 0, true)
    {
        addSettings(
            &mStyle,
            &mColorMode,
            &mCustomColor,
            &mAmount, &mSpread, &mSpeed, &mSize,
            &mGravity, &mDrag, &mLifetime,
            &mDirectional, &mGlow, &mTrails, &mFlash, &mSecondBurst
        );

        VISIBILITY_CONDITION(mCustomColor, mColorMode.mValue == ColorMode::Custom);

        mNames = {
            {Lowercase, "hitparticles"},
            {LowercaseSpaced, "hit particles"},
            {Normal, "HitParticles"},
            {NormalSpaced, "Hit Particles"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);
    void onPacketOutEvent(PacketOutEvent& event);

private:
    struct Particle {
        glm::vec3 pos{};
        glm::vec3 vel{};
        float     life    = 0.f;
        float     maxLife = 1.f;
        float     size    = 3.f;
        ImColor   col     = ImColor(255, 255, 255, 255);
    };

    // Delayed follow-up splash.
    struct Pending {
        uint64_t  at    = 0;
        glm::vec3 pos{};
        glm::vec3 dir{};
    };

    // Expanding ring at the contact point.
    struct Flash {
        uint64_t  start  = 0;
        uint64_t  dur    = 260;
        glm::vec3 pos{};
        ImColor   col    = ImColor(255, 255, 255, 255);
        float     radius = 0.45f;
    };

    std::vector<Particle> mParticles;
    std::vector<Pending>  mPending;
    std::vector<Flash>    mFlashes;

    void spawnBurst(const glm::vec3& center, const glm::vec3& dir,
                    float amountMul, float speedMul, float spreadMul);
    ImColor pickColor(int index);
    void updateParticles(float dt);
    void drawParticles();

    std::string getSettingDisplay() override
    {
        return mStyle.mValues[mStyle.as<int>()];
    }
};
