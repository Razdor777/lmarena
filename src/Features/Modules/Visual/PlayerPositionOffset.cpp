#include "PlayerPositionOffset.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Features/Events/ActorRenderEvent.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <imgui.h>

void PlayerPositionOffset::onEnable() {
    gFeatureManager->mDispatcher->listen<ActorRenderEvent, &PlayerPositionOffset::onActorRenderEvent>(this);
}

void PlayerPositionOffset::onDisable() {
    gFeatureManager->mDispatcher->deafen<ActorRenderEvent, &PlayerPositionOffset::onActorRenderEvent>(this);
}

void PlayerPositionOffset::setTargetPlayer(const std::string& name, int64_t runtimeID) {
    mTargetPlayer = name;
    mTargetRuntimeID = runtimeID;
    ChatUtils::displayClientMessage("§aЦель установлена: §f" + name);
}

void PlayerPositionOffset::clearTarget() {
    mTargetPlayer = "";
    mTargetRuntimeID = -1;
    ChatUtils::displayClientMessage("§cЦель сброшена");
}

void PlayerPositionOffset::onActorRenderEvent(ActorRenderEvent& event) {
    if (mTargetRuntimeID == -1) return;
    if (!event.mEntity || !event.mEntity->isPlayer()) return;
    if (event.mEntity->getRuntimeID() != mTargetRuntimeID) return;

    auto localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer) return;

    glm::vec3 originalPos = event.getPlayerPos();
    glm::vec3 targetPos = originalPos;
    glm::vec3 localEyePos = *localPlayer->getPos() + glm::vec3(0.0f, 1.62f, 0.0f);

    // 1. Локальные оси X/Z (относительно твоего yaw)
    float yaw = localPlayer->getActorRotationComponent()->mYaw;
    float yawRad = glm::radians(yaw);

    glm::vec3 forward(-sin(yawRad), 0.0f, cos(yawRad));
    glm::vec3 right(cos(yawRad), 0.0f, sin(yawRad));
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    targetPos += right * mOffsetX.mValue
               + up * mOffsetY.mValue
               + forward * mOffsetZ.mValue;

    // 2. ReachOffset: честное смещение по прямой от тебя к цели
    if (mReachOffset.mValue != 0.0f) {
        glm::vec3 dir = targetPos - localEyePos;
        float dist = glm::length(dir);
        if (dist > 0.001f) {
            dir = glm::normalize(dir);
            targetPos = localEyePos + dir * (dist + mReachOffset.mValue);
        }
    }

    // 3. Orbit: кружится вокруг тебя с фиксированным радиусом, смотрит на тебя
    if (mOrbit.mValue) {
        static float orbitAngle = 0.0f;
        orbitAngle += ImGui::GetIO().DeltaTime * mOrbitSpeed.mValue;

        float radius = mOrbitRadius.mValue;
        if (radius < 0.5f) radius = 0.5f;

        glm::vec3 center = *localPlayer->getPos() + glm::vec3(0.0f, 0.5f, 0.0f);

        targetPos.x = center.x + cos(orbitAngle) * radius;
        targetPos.z = center.z + sin(orbitAngle) * radius;
        targetPos.y = originalPos.y + mOffsetY.mValue;

        glm::vec3 lookDir = center - targetPos;
        float lookYaw = atan2(lookDir.x, lookDir.z);
        event.mRot->y = glm::degrees(lookYaw);
        event.mRot->x = 0.0f;
    }

    // === ВОТ ЭТО ГЛАВНОЕ: сдвигаем тень и ник вместе с моделью ===
    if (auto rpc = event.mEntity->getRenderPositionComponent()) {
        rpc->mPosition = targetPos;
    }

    event.setPlayerPos(targetPos);

    // 4. Spin: только если Orbit ВЫКЛЮЧЕН
    if (mSpin.mValue && !mOrbit.mValue) {
        static float spinYaw = 0.0f;
        spinYaw += ImGui::GetIO().DeltaTime * mSpinSpeed.mValue * 360.0f;
        while (spinYaw > 360.0f) spinYaw -= 360.0f;

        event.mRot->y = spinYaw;
        event.mRot->x = 0.0f;
    }
}