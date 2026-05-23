#pragma once
#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Events/PreGameCheckEvent.hpp>

class Interface : public ModuleBase<Interface>
{
public:
    enum ColorTheme {
        Midnight,
        Slate,
        Ruby,
        Forest,
        Amethyst,
        Rainbow
    };

    enum class FontType {
        ProductSans,
        Mojangles,
        Comfortaa,
        OpenSans,
        SFPro,
        Sarabun,
    };

    EnumSettingT<NamingStyle> mNamingStyle = EnumSettingT<NamingStyle>("Naming", "The style of the module names.", NamingStyle::NormalSpaced, "lowercase", "lower spaced", "Normal", "Spaced");
    EnumSettingT<ColorTheme> mMode = EnumSettingT<ColorTheme>("Theme", "The mode of the interface.", Midnight, "Midnight", "Rainbow", "Slate", "Obsidian", "Forest", "Ember", "Ocean");
    EnumSettingT<FontType> mFont = EnumSettingT<FontType>("Font", "The font of the interface.", FontType::ProductSans, "Product Sans", "Mojangles", "Comfortaa", "Open Sans", "SF Pro", "Sarabun");
    NumberSetting mColorSpeed = NumberSetting("Color Speed", "The speed of the color change.", 3.f, 0.01f, 20.f, 0.01);
    NumberSetting mSaturation = NumberSetting("Saturation", "The saturation of the interface.", 1.f, 0.f, 1.f, 0.01);
    BoolSetting mSlotEasing = BoolSetting("Slot Easing", "Eases the selection of slots", true);
    NumberSetting mSlotEasingSpeed = NumberSetting("Easing Speed", "The speed of the slot easing", 20.f, 0.1f, 20.f, 0.01f);
#ifdef __DEBUG__
    BoolSetting mForcePackSwitching = BoolSetting("Force Pack Switching", "Allows pack switching in-game", false);
#endif

    Interface() : ModuleBase("Interface", "Customize the visuals!", ModuleCategory::Visual, 0, true) {
        gFeatureManager->mDispatcher->listen<ModuleStateChangeEvent, &Interface::onModuleStateChange, nes::event_priority::FIRST>(this);
        gFeatureManager->mDispatcher->listen<RenderEvent, &Interface::onRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<ActorRenderEvent, &Interface::onActorRenderEvent, nes::event_priority::NORMAL>(this);
        gFeatureManager->mDispatcher->listen<BaseTickEvent, &Interface::onBaseTickEvent>(this);
        gFeatureManager->mDispatcher->listen<PacketOutEvent, &Interface::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
        gFeatureManager->mDispatcher->listen<DrawImageEvent, &Interface::onDrawImageEvent>(this);
        gFeatureManager->mDispatcher->listen<PreGameCheckEvent, &Interface::onPregameCheckEvent>(this);

        addSettings(
            &mNamingStyle,
            &mMode,
            &mFont,
            &mColorSpeed,
            &mSaturation,
            &mSlotEasing,
            &mSlotEasingSpeed
#ifdef __DEBUG__
            ,&mForcePackSwitching
#endif
        );

        VISIBILITY_CONDITION(mColorSpeed, mMode.mValue == Rainbow);
        VISIBILITY_CONDITION(mSaturation, mMode.mValue == Rainbow);
        VISIBILITY_CONDITION(mSlotEasingSpeed, mSlotEasing.mValue);

        mNames = {
            {Lowercase, "interface"},
            {LowercaseSpaced, "interface"},
            {Normal, "Interface"},
            {NormalSpaced, "Interface"}
        };
    }

    static inline std::unordered_map<int, std::vector<ImColor>> ColorThemes = {
        {Midnight, {ImColor(45,55,75), ImColor(60,75,100), ImColor(35,45,65), ImColor(55,70,95)}},
        {Slate,    {ImColor(85,90,100), ImColor(100,105,115), ImColor(70,75,85), ImColor(90,95,105)}},
        {Ruby,     {ImColor(160,50,60), ImColor(190,70,80), ImColor(130,40,50), ImColor(180,60,70)}},
        {Forest,   {ImColor(50,110,80), ImColor(65,135,100), ImColor(40,90,65), ImColor(60,125,90)}},
        {Amethyst, {ImColor(110,80,150), ImColor(130,100,175), ImColor(95,70,130), ImColor(120,90,160)}},
        {Rainbow,  {}}
    };

    void onEnable() override;
    void onDisable() override;
    void renderHoverText();
    void onModuleStateChange(ModuleStateChangeEvent& event);
    void onPregameCheckEvent(class PreGameCheckEvent& event);
    void onRenderEvent(class RenderEvent& event);
    void onActorRenderEvent(class ActorRenderEvent& event);
    void onDrawImageEvent(class DrawImageEvent& event);
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketOutEvent(class PacketOutEvent& event);
};

/*
class BodyYaw
{
public:
    static inline float bodyYaw = 0.f;
    static inline glm::vec3 posOld = glm::vec3(0, 0, 0);
    static inline glm::vec3 pos = glm::vec3(0, 0, 0);

    static inline void updateRenderAngles(Actor* plr, float headYaw)
    {
        posOld = pos;
        pos = *plr->getPos();
        float diffX = pos.x - posOld.x;
        float diffZ = pos.z - posOld.z;
        float diff = diffX * diffX + diffZ * diffZ;
        float body = bodyYaw;
        if (diff > 0.0025000002F)
        {
            float anglePosDiff = atan2f(diffZ, diffX) * 180.f / 3.14159265358979323846f - 90.f;
            float degrees = abs(wrapAngleTo180_float(headYaw) - anglePosDiff);
            if (95.f < degrees && degrees < 265.f)
                body = anglePosDiff - 180.f;
            else
                body = anglePosDiff;
        }
        turnBody(body, headYaw);
    };

    static inline void turnBody(float bodyRot, float headYaw)
    {
        float amazingDegreeDiff = wrapAngleTo180_float(bodyRot - bodyYaw);
        bodyYaw += amazingDegreeDiff * 0.3f;
        float bodyDiff = wrapAngleTo180_float(headYaw - bodyYaw);
        if (bodyDiff < -75.f) bodyDiff = -75.f;
        if (bodyDiff >= 75.f) bodyDiff = 75.f;
        bodyYaw = headYaw - bodyDiff;
        if (bodyDiff * bodyDiff > 2500.f)
            bodyYaw += bodyDiff * 0.2f;
    };

    static inline float wrapAngleTo180_float(float value)
    {
        value = fmodf(value, 360.f);
        if (value >= 180.0F) value -= 360.0F;
        if (value < -180.0F) value += 360.0F;
        return value;
    };
};
*/