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
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/FontHelper.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

TargetHUD::TargetHUD(): ModuleBase("TargetHUD", "Shows target information", ModuleCategory::Visual, 0, false)
{
    addSettings(
        &mStyle,
        &mXOffset,
        &mYOffset,
        &mFontSize,
        &mHealthCalculation,
        &mShowHead,
        &mShowHealthText,
        &mDamageTrail,
        &mBarColor,
        &mGlow,
        &mHurtFlash,
        &mShimmer,
        &mBackgroundAlpha
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
    auto* ci = ClientInstance::get();
    if (!ci) return;
    if (!ci->getLocalPlayer()) return;
    if (!ci->getLevelRenderer()) return;

    // Safe resolve: the HUD editor may preview the local player as a fake target.
    Actor* target = Aura::sTarget;
    bool hasTarget = Aura::sHasTarget;

    if (mElement->mSampleMode && !hasTarget) {
        target = ci->getLocalPlayer();
        hasTarget = target != nullptr;
        if (target) {
            try {
                mHealth = target->getHealth();
                mMaxHealth = target->getMaxHealth();
                mAbsorption = target->getAbsorption();
                mMaxAbsorption = target->getMaxAbsorption();
                mLastPlayerName = target->getRawName();
                mLastHurtTime = mHurtTime;
                if (auto* hc = target->getMobHurtTimeComponent())
                    mHurtTime = static_cast<float>(hc->mHurtTime);
            } catch (...) {
                hasTarget = false;
            }
        }
    }

    renderTargetHud(ImGui::GetBackgroundDrawList(), target, hasTarget);
}

void TargetHUD::renderTargetHud(ImDrawList* drawList, Actor* target, bool hasTarget)
{
    if (!drawList) return;

    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);
    const bool showing = mEnabled && hasTarget && target != nullptr;

    // ─── animation drivers ─────────────────────────────────────────────────
    mAnim = MathUtils::lerp(mAnim, showing ? 1.f : 0.f, std::clamp(dt * 11.f, 0.f, 1.f));

    const int64_t targetId = (target && target->isValid())
        ? static_cast<int64_t>(target->getRuntimeID()) : 0;

    if (targetId != mLastTargetId)
    {
        // New target: snap the bars (no sliding across from the old target),
        // restart the pop and the light sweep.
        mLastTargetId = targetId;
        mPop = 0.f;
        mLerpedHealth = mHealth;
        mLerpedAbsorption = mAbsorption;
        mGhostPercent = (mMaxHealth > 0.001f) ? std::clamp(mHealth / mMaxHealth, 0.f, 1.f) : 1.f;
        mPrevHealthPercent = mGhostPercent;
        mGhostDelay = 0.f;
        mFlash = 0.f;
        if (mShimmer.mValue) mShimmerPos = -0.35f;
    }

    if (mAnim < 0.01f) return;

    mPop = std::clamp(mPop + dt / 0.22f, 0.f, 1.f);
    mShimmerPos = (std::min)(1.5f, mShimmerPos + dt * 1.7f);

    // Health / absorption follow the reported values with a soft chase.
    mLerpedHealth = MathUtils::lerp(mLerpedHealth, mHealth, std::clamp(dt * 8.f, 0.f, 1.f));
    mLerpedAbsorption = MathUtils::lerp(mLerpedAbsorption, mAbsorption, std::clamp(dt * 8.f, 0.f, 1.f));

    const float maxHp = (std::max)(mMaxHealth, 0.001f);
    const float hpPct = std::clamp(mLerpedHealth / maxHp, 0.f, 1.f);
    const float absPct = std::clamp(mLerpedAbsorption / 20.f, 0.f, 1.f);

    // Damage trail: hold the lost chunk, then let it drain.
    if (hpPct < mPrevHealthPercent - 0.002f) mGhostDelay = 0.35f;
    mPrevHealthPercent = hpPct;

    if (mGhostDelay > 0.f) mGhostDelay -= dt;
    else if (mGhostPercent > hpPct) mGhostPercent = MathUtils::lerp(mGhostPercent, hpPct, std::clamp(dt * 4.f, 0.f, 1.f));
    if (mGhostPercent < hpPct) mGhostPercent = hpPct;

    const float hurtNorm = std::clamp(mHurtTime / 10.f, 0.f, 1.f);
    mFlash = MathUtils::lerp(mFlash, (mHurtFlash.mValue ? hurtNorm : 0.f), std::clamp(dt * 18.f, 0.f, 1.f));

    // ─── layout ────────────────────────────────────────────────────────────
    // The whole card scales with the font size so nothing overflows at 40pt.
    const float scale = std::clamp(mFontSize.mValue / 20.f, 0.6f, 2.f);
    const float fontSize = 20.f * scale;
    const float nameSize = fontSize;
    const float infoSize = fontSize * 0.66f;

    const float pad = 8.f * scale;
    const float headSize = mShowHead.mValue ? 58.f * scale : 0.f;
    const float gap = headSize > 0.f ? 10.f * scale : 0.f;
    const float barH = 12.f * scale;

    std::string name = mLastPlayerName.empty() ? std::string("Target") : mLastPlayerName;

    FontHelper::pushPrefFont(true, true);

    ImFont* font = ImGui::GetFont();
    const float nameW = font->CalcTextSizeA(nameSize, FLT_MAX, 0.f, name.c_str()).x;

    const float contentW = (std::max)(148.f * scale, nameW + 24.f * scale);
    const float textColH = nameSize + 5.f * scale + barH
        + (mShowHealthText.mValue ? infoSize + 4.f * scale : 0.f);
    const float bodyH = (std::max)(headSize, textColH);

    const float w = pad * 2.f + headSize + gap + contentW;
    const float h = pad * 2.f + bodyH;

    mElement->mSize = glm::vec2(w, h);
    mElement->mCentered = true;

    const float popEase = [&] {
        const float t = std::clamp(mPop, 0.f, 1.f);
        constexpr float c1 = 1.5f;
        constexpr float c3 = c1 + 1.f;
        const float x = t - 1.f;
        return 1.f + c3 * x * x * x + c1 * x * x;
    }();
    const float travel = std::clamp(popEase, 0.f, 1.f);
    const float alpha = std::clamp(mAnim * 1.15f, 0.f, 1.f);

    const float bw = w * (0.92f + 0.08f * popEase);
    const float bh = h * (0.92f + 0.08f * popEase);

    // getPos() returns ImVec2 — keep it in that type, GLM has no implicit
    // conversion from it.
    const ImVec2 elemPos = mElement->getPos();
    ImVec2 boxPos(elemPos.x - bw * 0.5f + (1.f - travel) * 16.f * scale,
                  elemPos.y - bh * 0.5f + (1.f - alpha) * 10.f * scale);

    // Impact shake while the target is hurt.
    if (mFlash > 0.001f && mHurtFlash.mValue)
    {
        const float t = static_cast<float>(ImGui::GetTime());
        boxPos.x += std::sin(t * 46.f) * 2.6f * scale * mFlash;
        boxPos.y += std::sin(t * 39.f + 1.1f) * 1.4f * scale * mFlash;
    }

    const ImVec2 boxMax(boxPos.x + bw, boxPos.y + bh);
    const float radius = 12.f * scale;
    const bool minimal = mStyle.mValue == Style::Minimal;

    ImColor accent = ColorUtils::getStaticAccentColor(boxPos.y * 1.5f);

    // ─── background ────────────────────────────────────────────────────────
    drawList->AddShadowRect(boxPos, boxMax, ImColor(0, 0, 0, static_cast<int>(120 * alpha)),
                            15.f * scale, { 0.f, 3.f * scale }, 0, radius);

    if (!minimal)
    {
        if (mGlow.mValue)
        {
            ImColor glow = accent; glow.Value.w = 0.17f * alpha;
            drawList->AddShadowRect(boxPos, boxMax, glow, 24.f * scale, { 0.f, 0.f }, 0, radius);
        }

        if (mStyle.mValue == Style::Glass)
            ImRenderUtils::addBlur(ImVec4(boxPos.x, boxPos.y, boxMax.x, boxMax.y), alpha * 2.0f * scale, radius);

        const float bgA = std::clamp(mBackgroundAlpha.mValue, 0.f, 1.f) * alpha;
        drawList->AddRectFilled(boxPos, boxMax, ImColor(10, 12, 17, static_cast<int>(255 * bgA)), radius);

        ImColor tint = accent; tint.Value.w = 0.10f * alpha;
        drawList->AddRectFilled(boxPos, boxMax, tint, radius);

        ImColor border = accent; border.Value.w = 0.30f * alpha;
        drawList->AddRect(boxPos, boxMax, border, radius, 0, 1.f * scale);

        if (mStyle.mValue == Style::Glass)
        {
            // Accent edge that fades down the left side.
            ImColor edgeTop = accent; edgeTop.Value.w = 0.85f * alpha;
            ImColor edgeBot = ColorUtils::getStaticAccentColor(boxPos.y * 1.5f + 70.f); edgeBot.Value.w = 0.20f * alpha;
            drawList->AddRectFilledMultiColor(
                { boxPos.x, boxPos.y }, { boxPos.x + 2.5f * scale, boxMax.y },
                edgeTop, edgeTop, edgeBot, edgeBot);
        }
    }

    // ─── head ──────────────────────────────────────────────────────────────
    if (headSize > 0.f)
    {
        const float hs = headSize * (0.92f + 0.08f * popEase);
        const ImVec2 hMin(boxPos.x + pad, boxPos.y + (bh - hs) * 0.5f);
        const ImVec2 hMax(hMin.x + hs, hMin.y + hs);

        ID3D11ShaderResourceView* texture = (target && target->isValid()) ? getActorSkinTex(target) : nullptr;

        if (texture)
        {
            ImColor imageColor(1.f, 1.f, 1.f, alpha);
            if (mHurtFlash.mValue && mFlash > 0.f)
            {
                imageColor.Value.y = 1.f - 0.55f * mFlash;
                imageColor.Value.z = 1.f - 0.55f * mFlash;
            }
            drawList->AddImageRounded(texture, hMin, hMax, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), imageColor, hs * 0.18f);
        }
        else
        {
            drawList->AddRectFilled(hMin, hMax, ImColor(42, 46, 55, static_cast<int>(230 * alpha)), hs * 0.18f);
        }

        ImColor ring = accent; ring.Value.w = (0.45f + 0.35f * mFlash) * alpha;
        drawList->AddRect(hMin, hMax, ring, hs * 0.18f, 0, 1.4f * scale);
    }

    // ─── text column ───────────────────────────────────────────────────────
    const float textX = boxPos.x + pad + headSize * (0.92f + 0.08f * popEase) + gap;
    const float textRight = boxMax.x - pad;

    drawList->PushClipRect({ textX, boxPos.y }, { textRight, boxMax.y }, true);

    float cursorY = boxPos.y + pad;
    ImRenderUtils::drawShadowText(drawList, name, { textX, cursorY },
                                  ImColor(255, 255, 255, static_cast<int>(252 * alpha)), nameSize, false);

    // ─── health bar ────────────────────────────────────────────────────────
    const float barY = cursorY + nameSize + 5.f * scale;
    const float barW = textRight - textX;
    const ImVec2 bMin(textX, barY);
    const ImVec2 bMax(textX + barW, barY + barH);
    const float barRounding = barH * 0.5f;

    drawList->AddRectFilled(bMin, bMax, ImColor(255, 255, 255, static_cast<int>(24 * alpha)), barRounding);

    // Damage trail sits under the healthy fill: it is the part just lost.
    if (mDamageTrail.mValue && mGhostPercent > hpPct + 0.002f)
    {
        const float gx0 = textX + barW * hpPct;
        const float gx1 = textX + barW * std::clamp(mGhostPercent, 0.f, 1.f);
        drawList->AddRectFilled({ gx0, barY }, { gx1, bMax.y },
                                ImColor(232, 88, 92, static_cast<int>(220 * alpha)), barRounding);
    }

    const float fillW = barW * hpPct;
    if (fillW > 0.6f)
    {
        ImColor c0, c1;
        if (mBarColor.mValue == BarColor::Health)
        {
            const float hue = 0.33f * hpPct;                       // red -> green
            c0 = ImColor::HSV(hue, 0.78f, 1.f);
            c1 = ImColor::HSV(std::fmod(hue + 0.06f, 1.f), 0.70f, 1.f);
        }
        else
        {
            c0 = ColorUtils::getThemedColor(0);
            c1 = ColorUtils::getThemedColor(barW * 2.f);
        }
        c0.Value.w = alpha;
        c1.Value.w = alpha;

        drawList->PushClipRect(bMin, { textX + fillW, bMax.y }, true);
        drawList->AddRectFilledMultiColor(bMin, bMax, c0, c1, c1, c0, barRounding, ImDrawCornerFlags_All);
        // Glassy sheen over the top half of the fill.
        drawList->AddRectFilledMultiColor(bMin, { bMax.x, barY + barH * 0.5f },
                                          ImColor(255, 255, 255, static_cast<int>(64 * alpha)),
                                          ImColor(255, 255, 255, static_cast<int>(64 * alpha)),
                                          ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 0));
        drawList->PopClipRect();
    }

    // ─── absorption overlay ────────────────────────────────────────────────
    if (absPct > 0.002f)
    {
        const float aw = barW * absPct;
        ImColor gold0(255, 216, 96, static_cast<int>(242 * alpha));
        ImColor gold1(246, 176, 34, static_cast<int>(242 * alpha));

        drawList->PushClipRect(bMin, { textX + aw, bMax.y }, true);
        drawList->AddRectFilledMultiColor(bMin, bMax, gold0, gold1, gold1, gold0, barRounding, ImDrawCornerFlags_All);
        drawList->PopClipRect();

        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 3.4f);
        drawList->AddRectFilled({ textX + aw - 1.6f * scale, barY }, { textX + aw, bMax.y },
                                ImColor(255, 244, 190, static_cast<int>(220 * alpha * (0.45f + 0.55f * pulse))),
                                barRounding * 0.5f);
    }

    // ─── hp text ───────────────────────────────────────────────────────────
    if (mShowHealthText.mValue)
    {
        cursorY = bMax.y + 4.f * scale;

        const float shownHp = (std::max)(0.f, mLerpedHealth);
        char hpBuf[32];
        if (std::fabs(shownHp - std::round(shownHp)) < 0.05f)
            std::snprintf(hpBuf, sizeof(hpBuf), "%d", static_cast<int>(std::round(shownHp)));
        else
            std::snprintf(hpBuf, sizeof(hpBuf), "%.1f", shownHp);

        char maxBuf[32];
        std::snprintf(maxBuf, sizeof(maxBuf), " / %d", static_cast<int>(std::round(maxHp)));

        ImColor hpCol = (mBarColor.mValue == BarColor::Health)
            ? ImColor::HSV(0.33f * hpPct, 0.72f, 1.f)
            : accent;
        hpCol.Value.w = 0.96f * alpha;

        drawList->AddText(font, infoSize, { textX, cursorY }, hpCol, hpBuf);

        const float hpW = font->CalcTextSizeA(infoSize, FLT_MAX, 0.f, hpBuf).x;
        drawList->AddText(font, infoSize, { textX + hpW, cursorY },
                          ImColor(255, 255, 255, static_cast<int>(150 * alpha)), maxBuf);

        if (mAbsorption > 0.f)
        {
            char absBuf[32];
            std::snprintf(absBuf, sizeof(absBuf), "+%d", static_cast<int>(std::round(mAbsorption)));

            const float maxW = font->CalcTextSizeA(infoSize, FLT_MAX, 0.f, maxBuf).x;
            drawList->AddText(font, infoSize, { textX + hpW + maxW + 3.f * scale, cursorY },
                              ImColor(255, 216, 96, static_cast<int>(242 * alpha)), absBuf);
        }
    }

    drawList->PopClipRect();

    // ─── light sweep ───────────────────────────────────────────────────────
    if (mShimmer.mValue && !minimal && mShimmerPos > -0.3f && mShimmerPos < 1.3f)
    {
        const float bandW = bw * 0.18f;
        const float skew = bh * 0.75f;
        const float cx = boxPos.x + bw * mShimmerPos;
        const float fade = 1.f - std::fabs(mShimmerPos - 0.5f) * 1.3f;

        if (fade > 0.02f)
        {
            drawList->PushClipRect(boxPos, boxMax, true);
            drawList->AddQuadFilled({ cx, boxPos.y }, { cx + bandW, boxPos.y },
                                    { cx + bandW - skew, boxMax.y }, { cx - skew, boxMax.y },
                                    ImColor(255, 255, 255, static_cast<int>(30 * alpha * fade)));
            drawList->PopClipRect();
        }
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
