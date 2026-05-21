#include "HitParticles.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Features/FeatureManager.hpp>
#include <algorithm>
#include <cmath>

HitParticles::HitParticles()
    : ModuleBase("HitParticles", "Damage numbers, ghost trail & hit particles", ModuleCategory::Visual, 0, false,
        { {Lowercase, "hitparticles"}, {LowercaseSpaced, "hit particles"},
          {Normal, "HitParticles"}, {NormalSpaced, "Hit Particles"} })
{
    addSettings(
        &mDamageNumbers, &mRiseSpeed, &mDmgLifetime, &mScale, &mShadow,
        &mColorCrits, &mNormalColor, &mCritColor,
        &mGhostTrail, &mTrailInterval, &mTrailLifetime, &mTrailRadius,
        &mTrailRange, &mTrailColor, &mOnlyPlayers, &mMaxTrailPoints,
        &mHitBurst, &mParticleCount, &mParticleSpeed, &mParticleLifetime,
        &mParticleSize, &mParticleColor, &mParticleCritColor,
        &mMaxDist
    );
}

void HitParticles::onEnable() {
    mNumbers.clear();
    mTrailPoints.clear();
    mParticles.clear();
    mTrailTickCounter = 0;
    gFeatureManager->mDispatcher->listen<EntityHurtEvent, &HitParticles::onEntityHurt>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &HitParticles::onRenderEvent>(this);
}

void HitParticles::onDisable() {
    gFeatureManager->mDispatcher->deafen<EntityHurtEvent, &HitParticles::onEntityHurt>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &HitParticles::onRenderEvent>(this);
    mNumbers.clear();
    mTrailPoints.clear();
    mParticles.clear();
}

void HitParticles::onTick() {
    float dt = 0.05f;

    for (auto& n : mNumbers) n.mAge += dt;
    mNumbers.erase(
        std::remove_if(mNumbers.begin(), mNumbers.end(),
            [&](const DamageNumber& n) { return n.mAge >= mDmgLifetime.mValue; }),
        mNumbers.end());

    for (auto& p : mParticles) {
        p.mAge += dt;
        p.mPos += p.mVel * dt;
        p.mVel *= 0.92f;
        p.mVel.y -= 0.4f * dt;
    }
    mParticles.erase(
        std::remove_if(mParticles.begin(), mParticles.end(),
            [&](const HitParticle& p) { return p.mAge >= mParticleLifetime.mValue; }),
        mParticles.end());

    updateTrail();
}

void HitParticles::updateTrail() {
    if (!mGhostTrail.mValue) {
        mTrailPoints.clear();
        return;
    }

    float dt = 0.05f;
    for (auto& p : mTrailPoints) p.mAge += dt;

    mTrailPoints.erase(
        std::remove_if(mTrailPoints.begin(), mTrailPoints.end(),
            [&](const TrailPoint& p) { return p.mAge >= mTrailLifetime.mValue; }),
        mTrailPoints.end());

    mTrailTickCounter++;
    if (mTrailTickCounter < (int)mTrailInterval.mValue) return;
    mTrailTickCounter = 0;

    auto* ci = ClientInstance::get();
    auto* lp = ci->getLocalPlayer();
    if (!lp) return;

    Level* level = lp->getLevel();
    if (!level) return;

    for (auto* actor : level->getRuntimeActorList()) {
        if (!actor || !actor->isValid() || actor == lp) continue;
        if (actor->isDead()) continue;
        if (mOnlyPlayers.mValue && !actor->isPlayer()) continue;
        if (lp->distanceTo(actor) > mTrailRange.mValue) continue;

        glm::vec3 pos = *actor->getPos();
        pos.y += 0.1f;
        mTrailPoints.push_back({pos, 0.f});
    }

    while (mTrailPoints.size() > (size_t)mMaxTrailPoints.mValue)
        mTrailPoints.erase(mTrailPoints.begin());
}

void HitParticles::onEntityHurt(EntityHurtEvent& event) {
    if (!mEnabled) return;

    auto* ci = ClientInstance::get();
    auto* lp = ci->getLocalPlayer();
    if (!lp) return;

    Actor* entity = event.mEntity;
    if (entity == lp) return;

    float dist = lp->distanceTo(entity);
    if (dist > mMaxDist.mValue) return;

    bool isCrit = false;
    if (mColorCrits.mValue || mHitBurst.mValue) {
        auto* sv = lp->getStateVectorComponent();
        if (sv) isCrit = sv->mVelocity.y < -0.1f;
        if (!isCrit && event.mDamage >= 6) isCrit = true;
    }

    if (mDamageNumbers.mValue) {
        spawnDamageNumber(entity, (float)event.mDamage, isCrit);
    }

    if (mHitBurst.mValue) {
        glm::vec3 pos = *entity->getPos();
        AABB aabb = entity->getAABB();
        float topY = std::max(aabb.mMin.y, aabb.mMax.y);
        pos.y = (pos.y + topY) * 0.5f;
        spawnHitBurst(pos, isCrit);
    }
}

void HitParticles::onRenderEvent(RenderEvent& event) {
    if (!mEnabled) return;

    auto* ci = ClientInstance::get();
    auto* lp = ci->getLocalPlayer();
    if (!lp) return;

    glmatrixf mat = ci->getViewMatrix();
    glm::vec2 fov = ci->getFov();
    ImVec2 ss = ImRenderUtils::getScreenSize();
    glm::vec2 screenSize(ss.x, ss.y);
    glm::vec3 origin = *lp->getPos();

    if (mDamageNumbers.mValue) renderNumbers(mat, fov, screenSize, origin);
    if (mGhostTrail.mValue) renderTrail(mat, fov, screenSize, origin);
    if (mHitBurst.mValue) renderParticles(mat, fov, screenSize, origin);
}

void HitParticles::spawnDamageNumber(Actor* entity, float damage, bool isCrit) {
    glm::vec3 pos = *entity->getPos();
    AABB aabb = entity->getAABB();
    float topY = std::max(aabb.mMin.y, aabb.mMax.y);
    pos.y = topY + 0.3f;

    std::uniform_real_distribution<float> offsetDist(-0.3f, 0.3f);
    mNumbers.push_back({pos, damage, 0.f, isCrit, offsetDist(mRNG)});
}

void HitParticles::spawnHitBurst(const glm::vec3& pos, bool isCrit) {
    int count = (int)mParticleCount.mValue;
    float speed = mParticleSpeed.mValue;
    ImColor color = (isCrit ? mParticleCritColor : mParticleColor).getAsImColor();

    std::uniform_real_distribution<float> angleDist(0.f, 6.2831853f);
    std::uniform_real_distribution<float> pitchDist(-1.5f, 1.5f);
    std::uniform_real_distribution<float> speedDist(0.5f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(0.6f, 1.4f);

    for (int i = 0; i < count; i++) {
        float yaw = angleDist(mRNG);
        float pitch = pitchDist(mRNG);
        float s = speed * speedDist(mRNG);

        glm::vec3 vel(
            std::cos(yaw) * std::cos(pitch) * s,
            std::sin(pitch) * s + 0.15f,
            std::sin(yaw) * std::cos(pitch) * s
        );

        float sz = mParticleSize.mValue * sizeDist(mRNG);

        mParticles.push_back({pos, vel, 0.f, sz,
            ImColor(color.Value.x, color.Value.y, color.Value.z, color.Value.w)});
    }
}

void HitParticles::renderNumbers(const glmatrixf& mat, const glm::vec2& fov,
                                  const glm::vec2& screenSize, const glm::vec3& origin) {
    for (auto& num : mNumbers) {
        glm::vec3 wp = num.mPos;
        wp.y += num.mAge * mRiseSpeed.mValue;
        wp.x += num.mOffsetX * (1.0f + num.mAge * 0.5f);

        glm::vec2 sp;
        if (!mat.OWorldToScreen(origin, wp, sp, fov, screenSize)) continue;

        float progress = num.mAge / mDmgLifetime.mValue;
        float alpha = std::max(0.f, 1.0f - progress);

        float popT = std::min(num.mAge * 5.0f, 1.0f);
        float popScale = 1.0f + (1.0f - popT) * 0.4f;

        float dmgScale = 1.0f + std::min(num.mDamage / 15.0f, 1.0f) * 0.5f;
        float textSize = mScale.mValue * dmgScale * popScale;

        ImColor color = (mColorCrits.mValue && num.mIsCrit)
            ? mCritColor.getAsImColor()
            : mNormalColor.getAsImColor();
        color.Value.w = alpha;

        std::string text = std::to_string((int)num.mDamage);
        float tw = ImRenderUtils::getTextWidth(&text, textSize);

        ImRenderUtils::drawText(
            ImVec2(sp.x - tw / 2.f, sp.y),
            text, color, textSize, alpha, mShadow.mValue);
    }
}

void HitParticles::renderTrail(const glmatrixf& mat, const glm::vec2& fov,
                                const glm::vec2& screenSize, const glm::vec3& origin) {
    ImColor base = mTrailColor.getAsImColor();

    for (auto& pt : mTrailPoints) {
        glm::vec2 sp;
        if (!mat.OWorldToScreen(origin, pt.mPos, sp, fov, screenSize)) continue;

        float progress = pt.mAge / mTrailLifetime.mValue;
        float alpha = std::max(0.f, (1.0f - progress) * base.Value.w);
        float radius = mTrailRadius.mValue * (1.0f - progress * 0.5f);

        ImColor c(base.Value.x, base.Value.y, base.Value.z);

        ImRenderUtils::fillCircle(
            ImVec2(sp.x, sp.y), radius, c, alpha, 12);
    }
}

void HitParticles::renderParticles(const glmatrixf& mat, const glm::vec2& fov,
                                    const glm::vec2& screenSize, const glm::vec3& origin) {
    for (auto& p : mParticles) {
        glm::vec2 sp;
        if (!mat.OWorldToScreen(origin, p.mPos, sp, fov, screenSize)) continue;

        float progress = p.mAge / mParticleLifetime.mValue;
        float alpha = std::max(0.f, (1.0f - progress) * p.mColor.Value.w);
        float size = p.mSize * (1.0f - progress * 0.6f);

        ImColor c(p.mColor.Value.x, p.mColor.Value.y, p.mColor.Value.z);

        ImRenderUtils::fillCircle(
            ImVec2(sp.x, sp.y), size, c, alpha, 8);
    }
}