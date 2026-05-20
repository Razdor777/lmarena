#include "Velocity.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/PacketID.hpp>
#include <SDK/Minecraft/Network/Packets/SetActorMotionPacket.hpp>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void Velocity::onEnable()
{
    gFeatureManager->mDispatcher->listen<PacketInEvent, &Velocity::onPacketInEvent>(this);
}

void Velocity::onDisable()
{
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &Velocity::onPacketInEvent>(this);
}

void Velocity::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() != PacketID::SetActorMotion) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto packet = std::reinterpret_pointer_cast<SetActorMotionPacket>(event.mPacket);
    if (!packet) return;
    if (packet->mRuntimeID != player->getRuntimeID()) return;

    // --- Chance roll: if fails, let full KB through ---
    if ((int)mChance.mValue < 100) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.f, 100.f);
        if (dist(rng) > mChance.mValue)
            return; // full KB this hit
    }

    switch (mMode.mValue)
    {
    case Mode::Full:
        event.setCancelled(true);
        break;

    case Mode::Percent:
    {
        glm::vec3 motion = packet->mMotion;
        motion.x *= mHorizontal.mValue;
        motion.z *= mHorizontal.mValue;
        motion.y *= mVertical.mValue;
        packet->mMotion = motion;
        break;
    }

    case Mode::Redirect:
    {
        float angleDeg;
        switch (mDirection.mValue) {
            case Direction::Reverse: angleDeg = 180.f; break;
            case Direction::Left:    angleDeg = 90.f;  break;
            case Direction::Right:   angleDeg = 270.f; break;
            case Direction::Custom:  angleDeg = mCustomAngle.mValue; break;
            default:                 angleDeg = 180.f; break;
        }

        glm::vec3 motion = packet->mMotion;
        float vx = motion.x;
        float vz = motion.z;
        float magnitude = std::sqrt(vx * vx + vz * vz);

        if (magnitude > 0.001f) {
            float currentAngle = std::atan2(vz, vx);
            float redirectRad = angleDeg * (float)(M_PI / 180.0);
            float newAngle = currentAngle + redirectRad;
            float newMagnitude = magnitude * mRedirectStrength.mValue;

            motion.x = newMagnitude * std::cos(newAngle);
            motion.z = newMagnitude * std::sin(newAngle);
        }

        motion.y *= mRedirectVertical.mValue;
        packet->mMotion = motion;
        break;
    }
    }
}