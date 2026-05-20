//
// Created by vastrakai on 8/4/2024.
//

#include "TargetHUD.hpp"

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Modules/Combat/Aura.hpp>
#include <Hook/Hooks/RenderHooks/D3DHook.hpp>
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>
#include <SDK/Minecraft/Actor/Components/ActorOwnerComponent.hpp>
#include <SDK/Minecraft/Actor/Components/ActorTypeComponent.hpp>
#include <SDK/Minecraft/Network/Packets/ActorEventPacket.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>

TargetHUD::TargetHUD(): ModuleBase("TargetHUD", "Shows target information", ModuleCategory::Visual, 0, false)
{
    addSettings(
        &mStyle,
        &mXOffset,
        &mYOffset,
        &mFontSize,
        &mHealthCalculation
    );

    mNames = {
        {Lowercase, "targethud"},
        {LowercaseSpaced, "target hud"},
        {Normal, "TargetHUD"},
        {NormalSpaced, "Target HUD"},
    };

    gFeatureManager->mDispatcher->listen<RenderEvent, &TargetHUD::onRenderEvent, nes::event_priority::LAST>(this);

    mElement = std::make_unique<HudElement>();
    mElement->mPos = { 500, 500 };
    const char* ModuleBaseType = ModuleBase<TargetHUD>::getTypeID();
    mElement->mParentTypeIdentifier = const_cast<char*>(ModuleBaseType);
    HudEditor::gInstance->registerElement(mElement.get());
}

void TargetHUD::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &TargetHUD::onBaseTickEvent, nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &TargetHUD::onPacketInEvent>(this);

    mElement->mVisible = true;
}

void TargetHUD::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &TargetHUD::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &TargetHUD::onPacketInEvent>(this);

    // Clear and release textures
    for (auto& [actor, textureHolder] : mTargetTextures)
    {
        if (textureHolder.texture) textureHolder.texture->Release();
    }
    mTargetTextures.clear();

    mElement->mVisible = false;
}

void TargetHUD::onBaseTickEvent(BaseTickEvent& event)
{
    validateTextures();
    if (mHealthCalculation.mValue) calculateHealths();

    if (Aura::sHasTarget && Aura::sTarget && Aura::sTarget->getMobHurtTimeComponent())
    {
        try {
            if (!Aura::sTarget->getActorTypeComponent())
            {
                spdlog::warn("TargetHUD: Target has no ActorTypeComponent");
                return;
            }

            mHealth = Aura::sTarget->getHealth();
            mMaxHealth = Aura::sTarget->getMaxHealth();

            if (Aura::sTarget->isPlayer()) {
                std::string targetName = Aura::sTarget->getNameTag();
                size_t nl = targetName.find('\n');
                if (nl != std::string::npos) targetName = targetName.substr(0, nl);
                
                float th = mHealth, tmh = mMaxHealth;
                bool tracked = false;
                if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
                    mHealth = th;
                    mMaxHealth = tmh;
                    tracked = true;
                } else {
                    std::string cleanName = Aura::sTarget->getRawName();
                    if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
                        mHealth = th;
                        mMaxHealth = tmh;
                        tracked = true;
                    } else if (mHealthCalculation.mValue) {
                        mHealth = mHealths[cleanName].health;
                    }
                }
                if (tracked) {
                    mHealths[Aura::sTarget->getRawName()].health = th;
                }
            } else if (mHealthCalculation.mValue) {
                mHealth = mHealths[Aura::sTarget->getRawName()].health;
            }

            mAbsorption = Aura::sTarget->getAbsorption();
            mMaxAbsorption = Aura::sTarget->getMaxAbsorption();
            if (!Aura::sTarget->isPlayer())
            {
                mLastPlayerName = "Mob";
                return;
            }

            mLastHurtTime = mHurtTime;
            mHurtTime = static_cast<float>(Aura::sTarget->getMobHurtTimeComponent()->mHurtTime);
            mLastHealth = mHealth;
            mLastAbsorption = mAbsorption;
            mLastMaxHealth = mMaxHealth;
            mLastMaxAbsorption = mMaxAbsorption;
            mLastPlayerName = Aura::sTarget->getRawName();

            if (mHurtTime > mLastHurtTime)
            {
                mLastHurtTime = mHurtTime;
            }
        } catch (...) {
            spdlog::error("TargetHUD: Exception in onBaseTickEvent");
        }
    }
}


void TargetHUD::calculateHealths() {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto actors = ActorUtils::getActorList(true, true);

    bool heal = 4000 <= NOW - mLastHealTime;
    if (heal) mLastHealTime = NOW;

    for (auto actor : actors) {
        if (!actor || actor == player) continue;

        try {
            if (!actor->getMobHurtTimeComponent() || !actor->getActorTypeComponent()) continue;

            std::string rawName = actor->getRawName();
            auto& info = mHealths[rawName];
            float absorption = actor->getAbsorption();
            int hurtTime = actor->getMobHurtTimeComponent()->mHurtTime;

            if (0 < hurtTime) {
                float damage = 0;
                if (absorption < info.lastAbsorption) {
                    if (0 < absorption) {
                        info.damage = abs(info.lastAbsorption - absorption);
                        damage = 0;
                    }
                    else if (0 < info.lastAbsorption) {
                        damage = abs(info.damage - info.lastAbsorption);
                    }
                }
                else if(hurtTime == 9)
                {
                    damage = info.damage;
                }

                if (absorption == 0 && 0 < damage) {
                    if (info.health - damage < 0) info.health = 0;
                    else info.health -= damage;
                }
            }

            if (heal) {
                if (info.health + 1 > 20) info.health = 20;
                else info.health++;
            }

            info.lastAbsorption = absorption;
        } catch (...) {
            continue;
        }
    }
}

void TargetHUD::validateTextures()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    try {
        std::vector<EntityId> foundEntities;
        for (auto &&[daId, moduleOwner, typeComponent]: player->mContext.mRegistry->view<ActorOwnerComponent, ActorTypeComponent>().each())
        {
            if (moduleOwner.mActor)
                foundEntities.push_back(moduleOwner.mActor->mContext.mEntityId);
        }

        for (auto it = mTargetTextures.begin(); it != mTargetTextures.end();)
        {
            if (std::ranges::find(foundEntities, it->second.associatedEntity) == foundEntities.end())
            {
                if (it->second.texture) it->second.texture->Release();
                it = mTargetTextures.erase(it);
            }
            else
            {
                ++it;
            }
        }
    } catch (...) {
        spdlog::error("TargetHUD: Exception in validateTextures");
    }
}

ID3D11ShaderResourceView* TargetHUD::getActorSkinTex(Actor* actor)
{
    if (!actor) return nullptr;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return nullptr;

    if (!mTargetTextures.contains(actor)) mTargetTextures[actor] = TargetTextureHolder();

    auto& [texture, loaded, id] = mTargetTextures[actor];

    if (!loaded) {
        try {
            auto skin = actor->getSkin();
            if (!skin) return nullptr;

            bool isPlayer = true;
            if (actor->isValid() && !actor->isPlayer())
            {
                isPlayer = false;
                skin = player->getSkin();
                if (!skin) return nullptr;
                spdlog::warn("Falling back to default LP skin for non-player actor");
            }

            int headSize = skin->skinWidth / 8;
            int headOffsetX = skin->skinWidth / 8;
            int headOffsetY = skin->skinHeight / 8;

            std::vector<uint8_t> headData(headSize * headSize * 4);

            for (int y = 0; y < headSize; y++) {
                for (int x = 0; x < headSize; x++) {
                    int srcIndex = ((y + headOffsetY) * skin->skinWidth + (x + headOffsetX)) * 4;
                    int dstIndex = (y * headSize + x) * 4;
                    std::copy_n(skin->skinData + srcIndex, 4, headData.data() + dstIndex);
                }
            }

            int scalingFactor = 8;
            std::vector<uint8_t> scaledHeadData(headSize * scalingFactor * headSize * scalingFactor * 4);

            for (int y = 0; y < headSize * scalingFactor; y++) {
                for (int x = 0; x < headSize * scalingFactor; x++) {
                    int srcX = x / scalingFactor;
                    int srcY = y / scalingFactor;
                    int srcIndex = (srcY * headSize + srcX) * 4;
                    int dstIndex = (y * headSize * scalingFactor + x) * 4;
                    std::copy_n(headData.data() + srcIndex, 4, scaledHeadData.data() + dstIndex);
                }
            }

            headSize *= scalingFactor;
            headData = std::move(scaledHeadData);

            spdlog::info("Loading skin texture for {}", isPlayer ? actor->getRawName() : "Mob");
            D3DHook::createTextureFromData(headData.data(), headSize, headSize, &texture);
            loaded = true;
            id = actor->mContext.mEntityId;
        } catch (...) {
            spdlog::error("TargetHUD: Exception loading skin texture");
            return nullptr;
        }
    }

    return texture;
}

void TargetHUD::onRenderEvent(RenderEvent& event)
{
    if (!ClientInstance::get()->getLocalPlayer()) return;
    if (!ClientInstance::get()->getLevelRenderer()) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    if (mStyle.mValue != Style::Solstice)
    {
        return;
    }

    // ИСПРАВЛЕНИЕ: безопасный resolve
    Actor* target = Aura::sTarget;
    bool hasTarget = Aura::sHasTarget;

    if (mElement->mSampleMode && !hasTarget) {
        target = ClientInstance::get()->getLocalPlayer();
        hasTarget = true;
        if (target) {
            try {
                mHealth = target->getHealth();
                mMaxHealth = target->getMaxHealth();
                mAbsorption = target->getAbsorption();
                mMaxAbsorption = target->getMaxAbsorption();
                mLastPlayerName = target->getRawName();
                mLastHurtTime = mHurtTime;
                if (auto* hc = target->getMobHurtTimeComponent())
                    mHurtTime = hc->mHurtTime;
            } catch (...) {
                hasTarget = false;
            }
        }
    }

    static float anim = 0.f;
    float delta = ImGui::GetIO().DeltaTime;

    float lerpedHurtTime = MathUtils::lerp(mLastHurtTime / 10.f, mHurtTime / 10.f, delta);

    static float hurtTimeAnimPerc = 0.f;
    static float healthAnimPerc = 0.f;
    static float absorptionAnimPerc = 0.f;

    if (mLastTarget != target)
    {
        mLastTarget = target;
        mLastHealth = mHealth;
        mLastAbsorption = mAbsorption;
        mLastMaxHealth = mMaxHealth;
        mLastMaxAbsorption = mMaxAbsorption;
        healthAnimPerc = mHealth / mMaxHealth;
        absorptionAnimPerc = mAbsorption / 20.f;
        mLerpedHealth = mHealth;
        mLerpedAbsorption = mAbsorption;
        spdlog::info("Recalcing health and absorption");
    }

    mLastTarget = target;

    hurtTimeAnimPerc = MathUtils::lerp(hurtTimeAnimPerc, lerpedHurtTime, delta * 20.f);

    float perc = mLastHealth / mLastMaxHealth;
    healthAnimPerc = MathUtils::lerp(healthAnimPerc, perc, delta * 6.f);
    float perc2 = mLastAbsorption / 20.f;
    absorptionAnimPerc = MathUtils::lerp(absorptionAnimPerc, perc2, delta * 6.f);

    mLerpedHealth = MathUtils::lerp(mLerpedHealth, mHealth, delta * 10.f);
    mLerpedAbsorption = MathUtils::lerp(mLerpedAbsorption, mAbsorption, delta * 10.f);

    bool showing = mEnabled && hasTarget && target;

    anim = MathUtils::lerp(anim, showing ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 10.f);

    float xpad = 5;
    float ypad = 5;

    if (anim < 0.01f)
    {
        return;
    }

    FontHelper::pushPrefFont(true, true);

    auto screenSize = ImGui::GetIO().DisplaySize;

    auto boxSize = ImVec2(230 * anim, 70 * anim);
    auto boxPos = ImVec2(mElement->getPos().x, mElement->getPos().y);
    boxPos.x -= boxSize.x / 2;
    boxPos.y -= boxSize.y / 2;

    mElement->mSize = glm::vec2(boxSize.x, boxSize.y);
    mElement->mCentered = true;

    auto headSize = ImVec2(60 * anim, 60 * anim);
    auto headPos = ImVec2(boxPos.x + xpad * anim, boxPos.y + ypad * anim);

    float headQuartY = headSize.y / 4;
    auto headSize2 = ImVec2(MathUtils::lerp(headSize.x, 40 * anim, hurtTimeAnimPerc), MathUtils::lerp(headSize.y, 40 * anim, hurtTimeAnimPerc));

    float daTopYdiff = headPos.y - boxPos.y;

    drawList->AddRectFilled(boxPos, ImVec2(boxPos.x + boxSize.x, boxPos.y + boxSize.y), ImColor(0.f, 0.f, 0.f, 0.5f * anim), 15.f * anim);

    ID3D11ShaderResourceView* texture = nullptr;
    static bool loaded = false;
    texture = getActorSkinTex(target);
    loaded = true;

    auto imageColor = ImColor(1.f, 1.f, 1.f, 1.f * anim);

    imageColor.Value.x = MathUtils::lerp(imageColor.Value.x, 1.f, hurtTimeAnimPerc);
    imageColor.Value.y = MathUtils::lerp(imageColor.Value.y, 1.f - hurtTimeAnimPerc, hurtTimeAnimPerc);
    imageColor.Value.z = MathUtils::lerp(imageColor.Value.z, 1.f - hurtTimeAnimPerc, hurtTimeAnimPerc);

    float healthStartY = boxPos.y + boxSize.y - (ypad + 2) * anim - 25 * anim;
    float ysize = 20 * anim;
    auto healthBarStart = ImVec2(boxPos.x + headSize.x + (xpad * 2) * anim, healthStartY);
    int barSizeX = boxSize.x - xpad;
    auto healthBarEnd = ImVec2(boxPos.x + barSizeX, healthStartY + ysize);

    std::string name = mLastPlayerName;
    auto textNameSize = ImGui::GetFont()->CalcTextSizeA(mFontSize.mValue * anim, FLT_MAX, 0, name.c_str());
    float ydiff = healthBarStart.y - boxPos.y;
    auto textNamePos = ImVec2(headPos.x + headSize.x + xpad * anim, boxPos.y + ydiff / 2 - textNameSize.y / 2 + (ypad * anim));

    std::string healthStr = "+" + std::to_string((int)mAbsorption);
    auto textHealthSize = ImGui::GetFont()->CalcTextSizeA(mFontSize.mValue * anim, FLT_MAX, 0, healthStr.c_str());
    auto textHealthPos = ImVec2(healthBarStart.x + xpad * anim, healthStartY);

    headPos.x += (headSize.x - headSize2.x) / 2;
    headPos.y += (headSize.y - headSize2.y) / 2;

    if (texture)
        drawList->AddImageRounded(texture, headPos, headPos + headSize2, ImVec2(0, 0), ImVec2(1, 1), imageColor, 10.f * anim);

    auto textStartPos = textNamePos;
    auto textEndPos = textHealthPos + textHealthSize;
    textEndPos.x = boxPos.x + boxSize.x - xpad * anim;
    drawList->PushClipRect(textStartPos, textEndPos, true);
    ImRenderUtils::drawShadowText(drawList, name, textNamePos, ImColor(255, 255, 255, static_cast<int>(255 * anim)), mFontSize.mValue * anim, false);
    drawList->PopClipRect();

    float daBottomYdiff = boxPos.y + boxSize.y - healthBarEnd.y;

    drawList->AddRectFilled(healthBarStart, healthBarEnd, ImColor(100, 100, 100, (int)((float)170 * anim)), 10.f);

    float healthPerc = mLerpedHealth / mMaxHealth;
    healthPerc = MathUtils::clamp(healthPerc, 0.f, 1.f);
    auto healthEnd = ImVec2(healthBarEnd.x, healthBarEnd.y);
    healthEnd.x = MathUtils::lerp(healthBarStart.x, healthBarEnd.x, healthPerc);

    float absorptionPerc = mLerpedAbsorption / 20.f;
    absorptionPerc = MathUtils::clamp(absorptionPerc, 0.f, 1.f);
    auto absorpEnd = ImVec2(healthBarEnd.x, healthBarEnd.y);
    absorpEnd.x = MathUtils::lerp(healthBarStart.x, healthBarEnd.x, absorptionPerc);

    float endXDiff = healthBarEnd.x - healthBarStart.x;
    ImColor startColor = ColorUtils::getThemedColor(0);
    ImColor endColor = ColorUtils::getThemedColor(endXDiff * 2);
    startColor.Value.w *= anim;
    endColor.Value.w *= anim;

    if (healthPerc > 0.01f)
    {
        drawList->PushClipRect(healthBarStart, healthEnd, true);
        drawList->AddRectFilledMultiColor(healthBarStart, healthBarEnd, startColor, endColor, endColor, startColor, 10.f, ImDrawCornerFlags_All);
        drawList->PopClipRect();
    }

    if (absorptionPerc > 0.01f)
    {
        drawList->PushClipRect(healthBarStart, absorpEnd, true);
        drawList->AddRectFilled(healthBarStart, healthBarEnd, ImColor(244, 204, 0, (int)(255 * anim)), 10.f);
        drawList->PopClipRect();
    }

    if (mAbsorption != 0)
    {
        drawList->PushClipRect(textStartPos, textEndPos, true);
        drawList->PopClipRect();
    }

    FontHelper::popPrefFont();
}

void TargetHUD::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() == PacketID::ActorEvent)
    {
        auto packet = event.getPacket<ActorEventPacket>();

        if (packet->mEvent != ActorEvent::HURT) return;

        Actor* target = ActorUtils::getActorFromRuntimeID(packet->mRuntimeID);
        if (!target) return;
    }
    else if (event.mPacket->getId() == PacketID::ChangeDimension) {
        // Clear textures
        for (auto& [actor, textureHolder] : mTargetTextures)
        {
            if (textureHolder.texture) textureHolder.texture->Release();
        }
        mTargetTextures.clear();

        mLastHealTime = NOW;
        mHealths.clear();
    }
}