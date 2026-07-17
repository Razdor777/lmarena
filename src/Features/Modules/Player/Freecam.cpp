//
// Created by vastrakai on 7/22/2024.
//

#include "Freecam.hpp"

#include <Features/Events/ActorRenderEvent.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/CameraInstructionPacket.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/LookInputEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Hook/Hooks/NetworkHooks/PacketReceiveHook.hpp>
#include <Hook/Hooks/RenderHooks/ActorRenderDispatcherHook.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/KeyboardMouseSettings.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Actor/ActorFlags.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/World/Level.hpp>

// Checks that the player and every component Freecam touches actually exist.
// Component getters can legitimately return nullptr (world change, death,
// dimension transfer) — dereferencing them is what made Freecam crash.
bool Freecam::isPlayerReady(Actor* player)
{
    if (!player) return false;
    if (!player->mContext.mRegistry) return false;

    if (!player->getActorRotationComponent()) return false;
    if (!player->getActorHeadRotationComponent()) return false;
    if (!player->getMobBodyRotationComponent()) return false;
    if (!player->getAABBShapeComponent()) return false;
    if (!player->getStateVectorComponent()) return false;
    if (!player->getRenderPositionComponent()) return false;
    if (!player->getWalkAnimationComponent()) return false;
    if (!player->getMoveInputComponent()) return false;

    return true;
}

void Freecam::onEnable()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!isPlayerReady(player))
    {
        // Player/components not available (e.g. toggling during a world change).
        // Just stay disabled instead of crashing the game.
        mEnabled = false;
        mWantedState = false;
        return;
    }

    // NoClip: save and disable collision/gravity
    mHadCollision = player->getStatusFlag(ActorFlags::HasCollision);
    mHadGravity   = player->getStatusFlag(ActorFlags::HasGravity);
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);

    player->setFlag<RenderCameraComponent>(true);
    player->setFlag<CameraRenderPlayerModelComponent>(true);
    player->setFlag<RedirectCameraInputComponent>(true);

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Freecam::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &Freecam::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Freecam::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<ActorRenderEvent, &Freecam::onActorRenderEvent, nes::event_priority::VERY_FIRST>(this);



    auto rotComp = player->getActorRotationComponent();
    auto headRotComp = player->getActorHeadRotationComponent();
    auto bodyRotComp = player->getMobBodyRotationComponent();

    if (rotComp) mLastRot = *rotComp;
    mLastRot.mOldYaw = mLastRot.mYaw;
    mLastRot.mOldPitch = mLastRot.mPitch;

    if (headRotComp) mLastHeadRot = *headRotComp;
    mLastHeadRot.mOldHeadRot = mLastHeadRot.mHeadRot;

    if (bodyRotComp) mLastBodyRot = *bodyRotComp;
    mLastBodyRot.yOldBodyRot = mLastBodyRot.yBodyRot;

    auto aabb = player->getAABBShapeComponent();
    auto sv = player->getStateVectorComponent();
    auto rp = player->getRenderPositionComponent();
    auto walkAnim = player->getWalkAnimationComponent();

    mAABBMin = aabb->mMin;
    mAABBMax = aabb->mMax;
    mSvPos = sv->mPos;
    mSvPosOld = sv->mPos;
    mOldPos = rp->mPosition;
    if (mMode.mValue != Mode::Detached && walkAnim)
        walkAnim->mWalkAnimSpeed = 0.0f;

    mOrigin = mSvPos;
    mOldOrigin = mSvPosOld;


    mHeadYaw = { mLastHeadRot.mHeadRot, mLastHeadRot.mOldHeadRot };

    if (!player->mContext.mRegistry) return;

    for (auto&& [id, cameraComponent] : player->mContext.mRegistry->view<CameraComponent>().each())
    {
        player->mContext.mRegistry->set_flag<CameraAlignWithTargetForwardComponent>(id, false);
        player->mContext.mRegistry->set_flag<ExtendPlayerRenderingComponent>(id, true);

        auto storage = player->mContext.mRegistry->assure_t<UpdatePlayerFromCameraComponent>();
        if (storage && mMode.mValue == Mode::Detached && storage->contains(id))
        {
            mCameras[id] = storage->get(id).mUpdateMode;
            storage->remove(id);
        }

        if (cameraComponent.getMode() == CameraMode::FirstPerson)
        {
            auto* gaming = player->mContext.mRegistry->try_get<CameraDirectLookComponent>(id);
            if (gaming)
            {
                mOriginalRotRads[cameraComponent.getMode()] = gaming->mRotRads;
            }

            // Remove the CameraRenderFirstPersonObjectsComponent flag
            player->mContext.mRegistry->set_flag<CameraRenderFirstPersonObjectsComponent>(id, false);
        } else if (cameraComponent.getMode() == CameraMode::ThirdPerson || cameraComponent.getMode() == CameraMode::ThirdPersonFront)
        {
            auto* gaming = player->mContext.mRegistry->try_get<CameraOrbitComponent>(id);
            if (gaming)
            {
                mOriginalRotRads[cameraComponent.getMode()] = gaming->mRotRads;
            }
        }
    }
}

void Freecam::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Freecam::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &Freecam::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Freecam::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<ActorRenderEvent, &Freecam::onActorRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    player->setFlag<RenderCameraComponent>(false);
    player->setFlag<CameraRenderPlayerModelComponent>(false);
    player->setFlag<RedirectCameraInputComponent>(false);

    // NoClip: restore flags and kill velocity
    player->setStatusFlag(ActorFlags::HasCollision, mHadCollision);
    player->setStatusFlag(ActorFlags::HasGravity, mHadGravity);

    auto sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
    player->setFallDistance(0.f);

    if (mMode.mValue == Mode::Normal)
    {
        auto aabb = player->getAABBShapeComponent();
        if (aabb)
        {
            aabb->mMin = mAABBMin;
            aabb->mMax = mAABBMax;
        }
        if (sv)
        {
            sv->mPos = mSvPos;
            sv->mPosOld = mSvPosOld;
        }
    }
    if (auto walkAnim = player->getWalkAnimationComponent())
        walkAnim->mWalkAnimSpeed = 1.0f;
    if (auto moveInput = player->getMoveInputComponent())
        moveInput->reset(false);


    mResetRot = true;
}

void Freecam::onPacketInEvent(PacketInEvent& event)
{

    if (event.mPacket->getId() == PacketID::MovePlayer)
    {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return;

        auto mpp = event.getPacket<MovePlayerPacket>();
        if (mpp->mPlayerID == player->getRuntimeID())
        {
            if (mDisableOnLagback.mValue)
            {
                NotifyUtils::notify("Lagback detected!", 5.f, Notification::Type::Warning);
                setEnabled(false);
                return;
            }
            glm::vec2 pktRot = mpp->mRot;
            float headRot = mpp->mYHeadRot;
            mLastRot.mYaw = pktRot.x;
            mLastRot.mPitch = pktRot.y;
            mLastHeadRot.mHeadRot = headRot;
            mLastBodyRot.yBodyRot = pktRot.x;

            mLastRot.mOldYaw = pktRot.x;
            mLastRot.mOldPitch = pktRot.y;
            mLastHeadRot.mOldHeadRot = headRot;
            mLastBodyRot.yOldBodyRot = pktRot.x;


            player->setPosition(mpp->mPos);
            mOrigin = mpp->mPos;
            mOldOrigin = mpp->mPos;
            spdlog::debug("[Freecam] Listening to lagback, setting position to {}, {}, {}", mpp->mPos.x, mpp->mPos.y, mpp->mPos.z);
        }
    }
}

void Freecam::onPacketOutEvent(PacketOutEvent& event)
{
    if ((event.mPacket->getId() == PacketID::PlayerAuthInput || event.mPacket->getId() == PacketID::MovePlayer) && mMode.mValue != Mode::Detached)
        event.mCancelled = true;

    if ((event.mPacket->getId() == PacketID::PlayerAuthInput) && mMode.mValue == Mode::Detached)
    {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) return;
        auto paip = event.getPacket<PlayerAuthInputPacket>();

        paip->mRot = { mLastRot.mPitch, mLastRot.mYaw };
        paip->mYHeadRot = mLastRot.mYaw;

        // Clamp the rot to -180 to 180 for yaw, -90 to 90 for pitch
        paip->mRot.x = MathUtils::wrap(paip->mRot.x, -90, 90);
        paip->mRot.y = MathUtils::wrap(paip->mRot.y, -180, 180);
        paip->mYHeadRot = MathUtils::wrap(paip->mYHeadRot, -180, 180);
        paip->removeMovingInput();
    }

}


void Freecam::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;
    auto ci = ClientInstance::get();
    if (!ci) return;

    player->setFlag<RenderCameraComponent>(true);
    player->setFlag<CameraRenderPlayerModelComponent>(true);
    //player->setFlag<CameraRenderFirstPersonObjects>(false);

    // NoClip: force collision off every tick
    player->setStatusFlag(ActorFlags::HasCollision, false);
    player->setStatusFlag(ActorFlags::HasGravity, false);
    player->setOnGround(false);
    player->setFallDistance(0.f);

    auto sv = player->getStateVectorComponent();
    if (!sv) return;

    glm::vec3 motion = glm::vec3(0, 0, 0);

    if (Keyboard::isUsingMoveKeys(true))
    {
        glm::vec2 rots = mRotRads;
        rots = glm::vec2(rots.y, rots.x); // Correct rotation (pitch, yaw)
        // convert the rots to degrees (-180 to 180 for yaw, -90 to 90 for pitch)
        rots = glm::degrees(rots);
        // Invert the yaw
        rots.y = -rots.y + 180;
        // Wrap
        rots.y = MathUtils::wrap(rots.y, -180, 180);

        glm::vec2 calc = MathUtils::getMotion(rots.y, mSpeed.mValue / 10, false);
        motion.x = calc.x;
        motion.z = calc.y;

        auto keyboard = ci->getKeyboardSettings();
        if (keyboard)
        {
            bool isJumping = Keyboard::mPressedKeys[(*keyboard)["key.jump"]];
            bool isSneaking = Keyboard::mPressedKeys[(*keyboard)["key.sneak"]];

            if (isJumping)
                motion.y += mSpeed.mValue / 10;
            else if (isSneaking)
                motion.y -= mSpeed.mValue / 10;
        }
    }

    if (mMode.mValue == Mode::Normal) sv->mVelocity = motion;
    else if (mMode.mValue == Mode::Detached)
    {
        mOldOrigin = mOrigin;
        mOrigin += motion;
    }
}

void Freecam::onActorRenderEvent(ActorRenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (event.mEntity != player) return;
    if (*event.mPos == glm::vec3(0.f, 0.f, 0.f) && *event.mRot == glm::vec2(0.f, 0.f)) event.cancel(); // Prevents the hand from rendering

    auto rotComp = player->getActorRotationComponent();
    auto headRotComp = player->getActorHeadRotationComponent();
    auto bodyRotComp = player->getMobBodyRotationComponent();
    if (!rotComp || !headRotComp || !bodyRotComp) return;

    auto oldRots = *rotComp;
    auto oldHeadRots = *headRotComp;
    auto oldBodyRots = *bodyRotComp;

    *rotComp = mLastRot;
    *headRotComp = mLastHeadRot;
    *bodyRotComp = mLastBodyRot;
    if (mMode.mValue == Mode::Detached)
    {
        if (auto moveInput = player->getMoveInputComponent())
            moveInput->reset(true, false);
        // TODO: Prevent the raycast from updating while in detached mode
    }

    if (!event.mDetour) return;
    auto original = event.mDetour->getOriginal<&ActorRenderDispatcherHook::render>();
    if (!original) return;
    auto newPos = *event.mPos - *event.mCameraTargetPos - *event.mPos + mOldPos;
    if (mMode.mValue == Mode::Detached)
    {
        auto rp = player->getRenderPositionComponent();
        if (rp) newPos = *event.mPos - *event.mCameraTargetPos - *event.mPos + rp->mPosition;
    }
    original(event._this, event.mEntityRenderContext, event.mEntity, event.mCameraTargetPos, &newPos, event.mRot, event.mIgnoreLighting);
    event.cancel();

    if (mMode.mValue != Mode::Detached)
    {
        *rotComp = oldRots;
        *headRotComp = oldHeadRots;
        *bodyRotComp = oldBodyRots;
    }
}

void Freecam::onLookInputEvent(LookInputEvent& event)
{
    if (mResetRot)
    {
        // Restore camera state. The player (or its registry) can be gone by
        // now (world change, death, relog) — bail out instead of crashing.
        auto player = ClientInstance::get()->getLocalPlayer();
        if (player && player->mContext.mRegistry)
        {
            for (auto&& [id, cameraComponent] : player->mContext.mRegistry->view<CameraComponent>().each())
            {
                player->mContext.mRegistry->set_flag<CameraAlignWithTargetForwardComponent>(id, true);
                auto storage = player->mContext.mRegistry->assure_t<UpdatePlayerFromCameraComponent>();
                if (storage && !storage->contains(id))
                {
                    storage->emplace(id, UpdatePlayerFromCameraComponent(mCameras[id]));
                }

                if (cameraComponent.getMode() == CameraMode::FirstPerson)
                {
                    auto* gaming = player->mContext.mRegistry->try_get<CameraDirectLookComponent>(id);
                    if (gaming)
                    {
                        gaming->mRotRads = mOriginalRotRads[cameraComponent.getMode()];
                    }

                    player->mContext.mRegistry->set_flag<CameraRenderFirstPersonObjectsComponent>(id, true);
                } else if (cameraComponent.getMode() == CameraMode::ThirdPerson || cameraComponent.getMode() == CameraMode::ThirdPersonFront)
                {
                    auto* gaming = player->mContext.mRegistry->try_get<CameraOrbitComponent>(id);
                    if (gaming)
                    {
                        gaming->mRotRads = mOriginalRotRads[cameraComponent.getMode()];
                    }
                }
            }
        }

        mResetRot = false;
    }


    if (!mEnabled) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (auto options = ClientInstance::get()->getOptions())
        if (options->mThirdPerson) options->mThirdPerson->value = 0;

    if (mMode.mValue == Mode::Detached)
    {
        if (event.mFirstPersonCamera) event.mFirstPersonCamera->mOrigin = getLerpedOrigin();
    }
    if (event.mCameraDirectLookComponent) mRotRads = event.mCameraDirectLookComponent->mRotRads;
}
