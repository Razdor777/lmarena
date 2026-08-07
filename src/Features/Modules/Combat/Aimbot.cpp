#include <glm/glm.hpp>

#include "Aimbot.hpp"
#include "pch.hpp"

#include <Features/Events/LookInputEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Misc/Friends.hpp>

#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>

#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

void Aimbot::onEnable() {
    gFeatureManager->mDispatcher->listen<LookInputEvent, &Aimbot::onLookInputEvent, nes::event_priority::FIRST>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &Aimbot::onRenderEvent>(this);
    sCurrentTarget = nullptr;
    sHasTarget = false;
}

void Aimbot::onDisable() {
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &Aimbot::onLookInputEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Aimbot::onRenderEvent>(this);
    sCurrentTarget = nullptr;
    sHasTarget = false;
}

glm::vec2 Aimbot::normAngles(glm::vec2 a) {
    while (a.x > 90.f)   a.x -= 180.f;
    while (a.x < -90.f)  a.x += 180.f;
    while (a.y > 180.f)  a.y -= 360.f;
    while (a.y < -180.f) a.y += 360.f;
    return a;
}

glm::vec2 Aimbot::calcAngle(const glm::vec3& src, const glm::vec3& dst) {
    return MathUtils::getRots(src, dst);
}

bool Aimbot::isEligibleTarget(Actor* actor, Actor* player) {
    if (!actor || actor == player) return false;
    if (!TRY_CALL([&]() { actor->isPlayer(); })) return false;
    if (actor->distanceTo(player) > mRange.mValue) return false;
    if (actor->isDead()) return false;
    if (actor->isPlayer()) {
        if (!mTargetPlayers.mValue) return false;
        if (mIgnoreFriends.mValue && gFriendManager && gFriendManager->mEnabled)
            if (gFriendManager->isFriend(actor)) return false;
    } else {
        if (!mTargetMobs.mValue) return false;
    }
    if (mIgnoreInvisible.mValue && actor->getStatusFlag(ActorFlags::Invisible)) return false;
    if (!mThroughWalls.mValue && !player->canSee(actor)) return false;
    return true;
}

glm::vec3 Aimbot::getAimPoint(Actor* target) {
    glm::vec3 pos = *target->getPos();
    auto shape = target->getAABBShapeComponent();
    if (shape) pos.y += shape->mHeight * mAimHeight.mValue;
    return pos;
}

// ==========================================
// Вычисление целевых mRotRads напрямую в радианах
// Без getRots, без конверсий
// ==========================================
static void calcTargetRotRads(const glm::vec3& eyePos, const glm::vec3& target, float& outYawRad, float& outPitchRad) {
    glm::vec3 diff = target - eyePos;
    float horizontalDist = std::sqrtf(diff.x * diff.x + diff.z * diff.z);

    // mRotRads.x = PI - yawMC(rad), где yawMC = atan2(-x, z)
    // (конвенция CameraDirectLookComponent — та же, что в Freecam/GhostMode:
    //  yawMC = -degrees(mRotRads.x) + 180).
    // Старая формула atan2(z,x)-PI/2 давала ошибку 180° на севере/юге —
    // в комментарии-проверке были только +X/-X, где обе формулы совпадают :)
    //   Восток (+X): yawMC=-90°  → mRotRads.x = 270° ≡ -90°  ✓
    //   Север (-Z):  yawMC=180°  → mRotRads.x = 0            ✓
    //   Юг (+Z):     yawMC=0°    → mRotRads.x = 180°         ✓
    outYawRad = IM_PI - std::atan2f(-diff.x, diff.z);
    while (outYawRad > IM_PI)  outYawRad -= 2.0f * IM_PI;
    while (outYawRad < -IM_PI) outYawRad += 2.0f * IM_PI;

    // mRotRads.y = pitch: atan2(y, horizontal)
    // Проверка из данных:
    //   Вверх (y > 0): pitch > 0 = +90° ✓
    //   Вниз (y < 0):  pitch < 0 = -90° ✓
    outPitchRad = std::atan2f(diff.y, horizontalDist);
}

void Aimbot::onLookInputEvent(LookInputEvent& event) {
    if (!event.mCameraDirectLookComponent) return;
    if (!mEnabled) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (mRequireClick.mValue && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        sHasTarget = false;
        sCurrentTarget = nullptr;
        return;
    }

    glm::vec3 eyePos = *player->getPos();
    eyePos.y += PLAYER_HEIGHT;

    // Поиск ближайшей цели
    auto actors = ActorUtils::getActorList(false, true);
    Actor* bestTarget = nullptr;
    float bestDist = mRange.mValue + 1.0f;
    for (auto actor : actors) {
        if (!isEligibleTarget(actor, player)) continue;
        float dist = actor->distanceTo(player);
        if (dist < bestDist) { bestDist = dist; bestTarget = actor; }
    }

    if (!bestTarget) {
        sCurrentTarget = nullptr;
        sHasTarget = false;
        return;
    }

    sCurrentTarget = bestTarget;
    sHasTarget = true;

    glm::vec3 aimPoint = getAimPoint(bestTarget);

    // Целевые углы (та же математика, что была тут инлайном:
    // yaw = atan2(-x, -z) ≡ PI - atan2(-x, z))
    float targetYaw, targetPitch;
    calcTargetRotRads(eyePos, aimPoint, targetYaw, targetPitch);

    // ==========================================
    // AimLock
    // ==========================================
    if (mAimLock.mValue) {
        event.mCameraDirectLookComponent->mRotRads.x = targetYaw;
        event.mCameraDirectLookComponent->mRotRads.y = targetPitch;
        return;
    }

    // ==========================================
    // Сглаженное наведение
    // ==========================================
    float curYaw = event.mCameraDirectLookComponent->mRotRads.x;
    float curPitch = event.mCameraDirectLookComponent->mRotRads.y;

    float deltaYaw = targetYaw - curYaw;
    float deltaPitch = targetPitch - curPitch;

    // Нормализация yaw
    while (deltaYaw > IM_PI)  deltaYaw -= 2.0f * IM_PI;
    while (deltaYaw < -IM_PI) deltaYaw += 2.0f * IM_PI;

    // Проверка FOV
    float angleDist = glm::degrees(std::sqrtf(deltaYaw * deltaYaw + deltaPitch * deltaPitch));
    if (angleDist > mFOV.mValue) {
        sCurrentTarget = nullptr;
        sHasTarget = false;
        return;
    }

    // Сглаживание
    float hDiv = std::max(1.0f, 100.0f - mHorizontalSpeed.mValue);
    float vDiv = std::max(1.0f, 100.0f - mVerticalSpeed.mValue);

    deltaYaw /= hDiv;
    deltaPitch /= vDiv;

    if (!mVertical.mValue) deltaPitch = 0;

    event.mCameraDirectLookComponent->mRotRads.x += deltaYaw;
    event.mCameraDirectLookComponent->mRotRads.y += deltaPitch;
}

void Aimbot::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto guiData = ClientInstance::get()->getGuiData();
    if (!guiData) return;

    glm::vec2 res = guiData->mResolution;
    ImVec2 center(res.x / 2.0f, res.y / 2.0f);

    if (mVisuals.mValue) {
        float fovRatio = mFOV.mValue / 90.0f;
        float radius = (res.x / 4.0f) * fovRatio;
        int opacity = static_cast<int>(mCircleOpacity.mValue);
        ImColor color = sHasTarget ? ImColor(0, 255, 0, opacity) : ImColor(255, 255, 255, opacity);
        ImGui::GetBackgroundDrawList()->AddCircle(center, radius, color, 64, 2.0f);

        if (sHasTarget && sCurrentTarget) {
            std::string name = sCurrentTarget->getNameTag();
            float dist = sCurrentTarget->distanceTo(player);
            char label[128];
            snprintf(label, sizeof(label), "Target: %s [%.1fm]", name.c_str(), dist);
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(center.x + radius + 10, center.y - 10),
                ImColor(0, 255, 0, 255), label);
        }
    }
}