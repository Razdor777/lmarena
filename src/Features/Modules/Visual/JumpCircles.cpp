//
// JumpCircles — перенос из combatant-client (visuals/JumpCircles.java).
// Логика: круг появляется, когда игрок переходит из «на земле» в «в воздухе».
// Длительность и рост — как в combatant (легаси-режим и Pulse).
//

#include "JumpCircles.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace JumpCirclesImpl
{
    constexpr float kPi       = 3.14159265f;
    constexpr int   kSegments = 48;    // точек на окружности
    constexpr float kLegacyGrowMs = 5000.f;  // знаменатель роста (как в combatant)
    constexpr float kLegacyFadeMs = 6000.f;  // знаменатель затухания

    // Отскок в конце роста — из combatant (JumpCircles.bounceOut).
    float bounceOut(float value)
    {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        if (value < 1.f / d1)       return n1 * value * value;
        if (value < 2.f / d1) { value -= 1.5f / d1;   return n1 * value * value + 0.75f; }
        if (value < 2.5f / d1) { value -= 2.25f / d1; return n1 * value * value + 0.9375f; }
        value -= 2.625f / d1;
        return n1 * value * value + 0.984375f;
    }

    // Прозрачность пульса — из combatant (JumpCircles.pulseAlpha).
    float pulseAlpha(float progress)
    {
        const float fadeInDuration = 0.15f;
        const float glowStart = 0.65f;
        const float fadeOutStart = 0.85f;
        float alpha;
        if (progress < fadeInDuration) {
            alpha = progress / fadeInDuration;
        } else if (progress >= fadeOutStart) {
            const float fadeOutProgress = (progress - fadeOutStart) / (1.f - fadeOutStart);
            alpha = 1.f - fadeOutProgress;
            if (progress > glowStart) {
                const float glowProgress = (progress - glowStart) / (fadeOutStart - glowStart);
                const float glowPulse = std::sin(glowProgress * kPi * 3.f) * 0.3f + 0.3f;
                alpha += glowPulse * (1.f - fadeOutProgress);
            }
        } else if (progress > glowStart) {
            const float glowProgress = (progress - glowStart) / (fadeOutStart - glowStart);
            const float glowPulse = std::sin(glowProgress * kPi * 3.f) * 0.3f + 0.3f;
            alpha = 1.f + glowPulse;
        } else {
            alpha = 1.f;
        }
        return std::clamp(alpha, 0.f, 1.f);
    }

    ImU32 toU32(const ImVec4& c, float alpha)
    {
        return IM_COL32(static_cast<int>(std::clamp(c.x, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(c.y, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(c.z, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(alpha, 0.f, 1.f) * 255.f));
    }
}

using namespace JumpCirclesImpl;

void JumpCircles::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &JumpCircles::onRenderEvent>(this);
}

void JumpCircles::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &JumpCircles::onRenderEvent>(this);
    mCircles.clear();
    mGroundState.clear();
}

uint64_t JumpCircles::lifetimeMs() const
{
    if (mMode.mValue == Mode::Pulse) return static_cast<uint64_t>(mPulseLifetime.mValue);
    return mEaseOut.mValue ? 5000 : 6000;
}

ImVec4 JumpCircles::colorFor(float index, uint64_t now) const
{
    const float seconds = static_cast<float>(now) * mColorSpeed.mValue / 18000.f;
    switch (mColorMode.mValue)
    {
    case ColorMode::Static:
        return ImVec4(mColor.mValue[0], mColor.mValue[1], mColor.mValue[2], mColor.mValue[3]);

    case ColorMode::Rainbow:
        return ColorUtils::Rainbow(seconds, 1.f, 1.f, static_cast<int>(index)).Value;

    case ColorMode::DoubleColor:
    {
        const ImVec4 c1(mColor.mValue[0], mColor.mValue[1], mColor.mValue[2], mColor.mValue[3]);
        const ImVec4 c2(mColor2.mValue[0], mColor2.mValue[1], mColor2.mValue[2], mColor2.mValue[3]);
        return ColorUtils::LerpColors(seconds, index, { ImColor(c1), ImColor(c2) }).Value;
    }

    case ColorMode::Theme:
        return ColorUtils::getThemedColor(index).Value;
    }
    return ImVec4(1.f, 1.f, 1.f, 1.f);
}

void JumpCircles::onRenderEvent(RenderEvent& event)
{
    auto* localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer || !ClientInstance::get()->getLevelRenderer()) return;

    const uint64_t now = NOW;

    // Отрыв от земли: ground -> air. Круг ставится под ногами (getPos() — на высоте глаз).
    std::unordered_set<int64_t> active;
    for (Actor* actor : ActorUtils::getActorList(true, false))
    {
        if (mOnlySelf.mValue && actor != localPlayer) continue;

        const int64_t id = actor->getRuntimeID();
        active.insert(id);

        const auto it = mGroundState.find(id);
        const bool wasGround = it != mGroundState.end() && it->second;
        const bool isGround = actor->isOnGround();

        if (wasGround && !isGround)
        {
            if (auto* pos = actor->getPos())
            {
                const glm::vec3 feet = *pos - PLAYER_HEIGHT_VEC;
                mCircles.push_back({ glm::vec3(feet.x, std::floor(feet.y) + 0.001f, feet.z), now });
            }
        }
        mGroundState[id] = isGround;
    }

    for (auto it = mGroundState.begin(); it != mGroundState.end();)
    {
        if (!active.count(it->first)) it = mGroundState.erase(it);
        else ++it;
    }

    const uint64_t life = lifetimeMs();
    mCircles.erase(std::remove_if(mCircles.begin(), mCircles.end(),
        [&](const Circle& c) { return now - c.start >= life; }), mCircles.end());

    if (mCircles.empty()) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    // Новые круги рисуем первыми, как в combatant (обратный порядок).
    for (auto it = mCircles.rbegin(); it != mCircles.rend(); ++it)
        drawCircle(dl, *it, now);
}

void JumpCircles::drawCircle(ImDrawList* dl, const Circle& c, uint64_t now)
{
    const float passed = static_cast<float>(now - c.start);
    const bool pulse = mMode.mValue == Mode::Pulse;

    float half = 0.f;
    float alpha = 0.f;
    float angleDeg = 0.f;

    if (pulse)
    {
        const float progress = std::min(passed / std::max(1.f, static_cast<float>(mPulseLifetime.mValue)), 1.f);
        if (progress >= 1.f) return;
        const float scale = bounceOut(progress) * mPulseMaxSize.mValue * mCircleScale.mValue;
        if (scale <= 0.f) return;
        half = scale * 0.5f;
        alpha = pulseAlpha(progress);
    }
    else
    {
        const float t = passed * (mEaseOut.mValue ? 2.f : 1.f) / kLegacyGrowMs;
        const float size = mCircleScale.mValue - std::pow(1.f - t, 4.f);
        if (size <= 0.f) return;
        half = size;
        angleDeg = size * mRotateSpeed.mValue * 1000.f;
        alpha = 1.f - passed / kLegacyFadeMs;
    }

    alpha = std::clamp(alpha, 0.f, 1.f);
    if (alpha <= 0.f) return;

    // Окружность на земле, спроецированная на экран. Если часть за камерой — пропускаем круг.
    std::vector<ImVec2> ring;
    ring.reserve(kSegments);
    for (int i = 0; i < kSegments; i++)
    {
        const float a = static_cast<float>(i) / kSegments * 2.f * kPi;
        const glm::vec3 p = c.pos + glm::vec3(std::cos(a) * half, 0.f, std::sin(a) * half);
        ImVec2 s;
        if (!RenderUtils::worldToScreen(p, s)) return;
        ring.push_back(s);
    }

    const ImVec4 color = colorFor(static_cast<float>(c.start / 100 % 360), now);
    const bool bubble = mMode.mValue == Mode::Bubble;

    if (!bubble)
        dl->AddConvexPolyFilled(ring.data(), static_cast<int>(ring.size()), toU32(color, alpha * 0.22f));

    dl->AddPolyline(ring.data(), static_cast<int>(ring.size()), toU32(color, alpha * 0.9f),
                    ImDrawFlags_Closed, bubble ? 2.5f : 1.5f);

    // Вращающаяся подсветка — вместо вращения текстуры в combatant.
    if (mMode.mValue == Mode::Default)
    {
        const int start = static_cast<int>(std::fmod(angleDeg / 360.f * kSegments, static_cast<float>(kSegments)));
        std::vector<ImVec2> arc;
        for (int j = 0; j <= kSegments / 4; j++)
            arc.push_back(ring[(start + j) % kSegments]);
        dl->AddPolyline(arc.data(), static_cast<int>(arc.size()), toU32(color, alpha), 0, 3.f);
    }
}
