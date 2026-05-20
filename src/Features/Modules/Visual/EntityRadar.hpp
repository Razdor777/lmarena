#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include "HudEditor.hpp"
#include <deque>

class EntityRadar : public ModuleBase<EntityRadar>
{
public:
    NumberSetting mSize = NumberSetting("Size", "Radar size in pixels", 150.f, 80.f, 300.f, 10.f);
    NumberSetting mRange = NumberSetting("Range", "Detection range in blocks", 50.f, 10.f, 200.f, 5.f);
    NumberSetting mOpacity = NumberSetting("Opacity", "Background opacity", 0.7f, 0.1f, 1.f, 0.05f);
    BoolSetting mShowPlayers = BoolSetting("Players", "Show players", true);
    BoolSetting mShowMobs = BoolSetting("Mobs", "Show mobs", false);
    BoolSetting mRotate = BoolSetting("Rotate", "Rotate with player yaw", true);
    BoolSetting mShowNames = BoolSetting("Show Names", "Show names on hover", true);

    EntityRadar() : ModuleBase("EntityRadar", "Tactical radar showing entities around you",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mSize, &mRange, &mOpacity, &mShowPlayers, &mShowMobs, &mRotate, &mShowNames);
        mNames = {
            {Lowercase, "entityradar"}, {LowercaseSpaced, "entity radar"},
            {Normal, "EntityRadar"}, {NormalSpaced, "Entity Radar"}
        };

        mElement = std::make_unique<HudElement>();
        mElement->mPos = { -160, 10 };
        mElement->mAnchor = HudElement::Anchor::TopRight;
        const char* typeId = ModuleBase<EntityRadar>::getTypeID();
        mElement->mParentTypeIdentifier = const_cast<char*>(typeId);
        mElement->mSize = glm::vec2(150, 150);
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(mElement.get());
    }

    void onEnable() override;
    void onDisable() override;
    void onRenderEvent(RenderEvent& event);

    std::unique_ptr<HudElement> mElement = nullptr;

private:
    float mSweepAngle = 0.f;
};
