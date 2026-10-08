#include "DestroyProgress.hpp"

#include <algorithm>
#include <cmath>

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace
{
    float frameDt()
    {
        return std::clamp(ImGui::GetIO().DeltaTime, 0.0005f, 0.1f);
    }

    // Frame-rate independent approach: k = dt * speed, clamped, so a low FPS
    // cannot overshoot and a high FPS stays smooth.
    float approach(float current, float target, float speed, float dt)
    {
        return current + (target - current) * std::clamp(dt * speed, 0.f, 1.f);
    }

    ImColor withAlpha(const ImColor& c, float a)
    {
        return ImColor(c.Value.x, c.Value.y, c.Value.z, std::clamp(a, 0.f, 1.f));
    }

    // ---- real cube corners ------------------------------------------------
    //
    // MathUtils::getImBoxPoints() returns the 2D *silhouette hull* of the
    // projected box, not the 8 corners. Stroking that hull as a thick closed
    // polyline is exactly what made the old outline look uneven — the hull
    // turns on a diagonal at some corners, so the mitre join there was almost
    // twice as thick as the flat edges. Filling still uses the hull (it is
    // correct for convex fills); stroking uses our own 12 edges.
    void boxCorners(const glm::vec3& mn, const glm::vec3& mx, glm::vec3 out[8])
    {
        out[0] = { mn.x, mn.y, mn.z }; out[1] = { mx.x, mn.y, mn.z };
        out[2] = { mx.x, mn.y, mx.z }; out[3] = { mn.x, mn.y, mx.z };
        out[4] = { mn.x, mx.y, mn.z }; out[5] = { mx.x, mx.y, mn.z };
        out[6] = { mx.x, mx.y, mx.z }; out[7] = { mn.x, mx.y, mx.z };
    }

    constexpr int kBoxEdges[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },   // bottom ring
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },   // top ring
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }    // verticals
    };

    bool projectBox(const glm::vec3& mn, const glm::vec3& mx, ImVec2 out[8])
    {
        glm::vec3 corners[8];
        boxCorners(mn, mx, corners);

        for (int i = 0; i < 8; ++i)
        {
            if (!RenderUtils::worldToScreen(corners[i], out[i])) return false;
            if (!std::isfinite(out[i].x) || !std::isfinite(out[i].y)) return false;
        }
        return true;
    }

    // Every edge gets the same width — that is the fix for the lopsided look.
    void strokeBox(ImDrawList* dl, const ImVec2 pts[8], ImU32 col, float thickness)
    {
        for (const auto& e : kBoxEdges)
            dl->AddLine(pts[e[0]], pts[e[1]], col, thickness);
    }
}

void DestroyProgress::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &DestroyProgress::onRenderEvent>(this);

    mAnim = mFade = mPopAnim = 0.f;
    mHasTarget = false;
}

void DestroyProgress::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &DestroyProgress::onRenderEvent>(this);

    mAnim = mFade = mPopAnim = 0.f;
    mHasTarget = false;
}

// Red (untouched) -> amber -> green (almost broken). HSV keeps it saturated
// instead of the muddy rgb lerp the old version used.
ImColor DestroyProgress::progressColor(float progress) const
{
    const float t = std::clamp(progress, 0.f, 1.f);
    return ImColor::HSV(t * 0.34f, 0.85f, 1.f);
}

ImColor DestroyProgress::sourceColor(float progress) const
{
    switch (mColorMode.as<ColorMode>())
    {
    case ColorMode::Theme:
        return ColorUtils::getThemedColor(0);
    case ColorMode::Custom:
        return ImColor(mCustomColor.mValue[0], mCustomColor.mValue[1], mCustomColor.mValue[2], 1.f);
    default:
        return progressColor(progress);
    }
}

void DestroyProgress::onRenderEvent(RenderEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;
    if (!ci->getGuiData()) return;   // worldToScreen needs the resolution

    auto* player = ci->getLocalPlayer();
    if (!player || ci->getMouseGrabbed()) return;

    auto* gameMode = player->getGameMode();
    auto* level    = player->getLevel();
    if (!gameMode || !level) return;

    const float dt = frameDt();

    const float progress = std::clamp(gameMode->mBreakProgress, 0.f, 1.f);

    auto* hit = level->getHitResult();
    const bool mining = progress > 0.f && hit && hit->mType == HitType::BLOCK;

    // ---- state ------------------------------------------------------------
    if (mining)
    {
        const glm::ivec3 target = hit->mBlockPos;
        const bool newBlock = !mHasTarget || target != mLastPos;

        if (newBlock)
        {
            mAnim = progress;   // do not glide in from the previous block
        }
        else
        {
            mAnim = approach(mAnim, progress, 22.f, dt);
        }

        mLastPos   = target;
        mHasTarget = true;
        mFade      = std::min(1.f, mFade + dt * 10.f);
    }
    else
    {
        // Finished, or the player looked away: play the pop if the block was
        // almost broken, then let the leftovers fade out instead of vanishing.
        if (mHasTarget && mPop.mValue && mAnim > 0.85f) mPopAnim = 0.0001f;

        mHasTarget = false;
        mFade -= dt * mFadeSpeed.mValue;
        if (mFade < 0.f) mFade = 0.f;
    }

    if (mPopAnim > 0.f)
    {
        mPopAnim += dt / 0.28f;
        if (mPopAnim >= 1.f) mPopAnim = 0.f;
    }

    if (mFade <= 0.0005f && mPopAnim <= 0.f) return;

    // ---- colours ----------------------------------------------------------
    const float t      = std::clamp(mAnim, 0.f, 1.f);
    const ImColor base = sourceColor(t);

    float pulse = 1.f;
    if (mPulse.mValue) pulse = 0.84f + 0.16f * std::sin(static_cast<float>(ImGui::GetTime()) * 4.4f);

    const float alpha = mOpacity.mValue * mFade;
    const glm::vec3 blockPos = glm::vec3(mLastPos);
    const glm::vec3 kOne(1.f);

    auto* dl = ImGui::GetBackgroundDrawList();

    auto toU32 = [](const ImColor& c) -> ImU32 {
        return IM_COL32(
            static_cast<int>(std::clamp(c.Value.x, 0.f, 1.f) * 255.f),
            static_cast<int>(std::clamp(c.Value.y, 0.f, 1.f) * 255.f),
            static_cast<int>(std::clamp(c.Value.z, 0.f, 1.f) * 255.f),
            static_cast<int>(std::clamp(c.Value.w, 0.f, 1.f) * 255.f));
    };

    // ---- whole block outline ---------------------------------------------
    ImVec2 shell[8];
    const bool haveShell = projectBox(blockPos, blockPos + kOne, shell);

    if (mOutline.mValue && haveShell)
    {
        // Cheap glow: the same edges, wider and almost transparent, layered
        // under the crisp stroke.
        if (mGlow.mValue)
        {
            for (int i = 3; i >= 1; --i)
                strokeBox(dl, shell,
                    toU32(withAlpha(base, alpha * 0.05f * static_cast<float>(4 - i))),
                    mThickness.mValue + static_cast<float>(i) * 2.4f);
        }

        strokeBox(dl, shell, toU32(withAlpha(base, alpha * pulse)), mThickness.mValue);
    }

    // ---- shrinking cube (the actual break progress) -----------------------

    ImVec2 cubePts[8];
    std::vector<ImVec2> cubeFill{};
    const float s = std::clamp(t, 0.02f, 1.f);

    if (mFilled.mValue)
    {
        const glm::vec3 centre = blockPos + glm::vec3(0.5f);
        const glm::vec3 half   = glm::vec3(0.5f * s);
        const glm::vec3 mn = centre - half;
        const glm::vec3 mx = centre + half;

        const bool haveCube = projectBox(mn, mx, cubePts);
        cubeFill = MathUtils::getImBoxPoints(AABB(mn, mx, true));

        if (haveCube)
        {
            if (cubeFill.size() >= 3)
                dl->AddConvexPolyFilled(cubeFill.data(), static_cast<int>(cubeFill.size()),
                                        toU32(withAlpha(base, alpha * 0.22f)));

            if (mGlow.mValue)
                strokeBox(dl, cubePts, toU32(withAlpha(base, alpha * 0.14f)),
                          mThickness.mValue + 3.f);

            strokeBox(dl, cubePts, toU32(withAlpha(base, alpha * 0.9f * pulse)),
                      std::max(1.f, mThickness.mValue * 0.85f));
        }
    }

    // ---- break pop --------------------------------------------------------
    if (mPopAnim > 0.f)
    {
        const float pad = mPopAnim * 0.55f;
        const float a   = (1.f - mPopAnim) * (1.f - mPopAnim) * mOpacity.mValue;

        ImVec2 ring[8];
        if (projectBox(blockPos - glm::vec3(pad), blockPos + kOne + glm::vec3(pad), ring))
            strokeBox(dl, ring, toU32(withAlpha(sourceColor(1.f), a)),
                      std::max(1.f, mThickness.mValue * 1.4f));
    }
}
