//
// Created by vastrakai on 7/22/2024.
//

#include "Freecam.hpp"

#include <Features/Events/ActorRenderEvent.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/CameraInstructionPacket.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Hook/Hooks/NetworkHooks/PacketReceiveHook.hpp>
#include <Hook/Hooks/RenderHooks/ActorRenderDispatcherHook.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/KeyboardRemappingLayout.hpp>
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

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Freecam::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &Freecam::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Freecam::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &Freecam::onRenderEvent>(this);



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

    mShouldDisable = false;


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
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Freecam::onRenderEvent>(this);

    // Queued before anything can bail out below: if the player is gone right now
    // (world change / death / disconnect) the camera state still has to be restored
    // on the next look input instead of leaving the freecam flags set forever.
    mResetRot = true;

    auto player = ClientInstance::get()->getLocalPlayer();

    // Everything from here on writes into the player's components. While the world
    // is unloading those are already destroyed, and dereferencing them is what made
    // Freecam crash when it was turned off.
    if (!player || !player->isValid()) return;

    player->setFlag<RenderCameraComponent>(false);
    player->setFlag<CameraRenderPlayerModelComponent>(false);
    player->setFlag<RedirectCameraInputComponent>(false);

    // NoClip: restore what onEnable saved, and kill the leftover motion
    player->setStatusFlag(ActorFlags::HasCollision, mHadCollision);
    player->setStatusFlag(ActorFlags::HasGravity, mHadGravity);

    auto sv = player->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
    player->setFallDistance(0.f);

    if (mMode.mValue == Mode::Normal)
    {
        if (auto aabb = player->getAABBShapeComponent())
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
}

void Freecam::onPacketInEvent(PacketInEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (event.mPacket->getId() == PacketID::MovePlayer)
    {
        auto mpp = event.getPacket<MovePlayerPacket>();
        if (mpp->mPlayerID == player->getRuntimeID())
        {
            NotifyUtils::notify("Lagback detected!", 5.f, Notification::Type::Warning);
            spdlog::debug("[Freecam] Listening to lagback, setting position to {}, {}, {}", mpp->mPos.x, mpp->mPos.y, mpp->mPos.z);
            mShouldDisable = true;
            setEnabled(false);
            return;
        }
    }
}

void Freecam::onPacketOutEvent(PacketOutEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || mShouldDisable) {
        return;
    }

    if ((event.mPacket->getId() == PacketID::PlayerAuthInput || event.mPacket->getId() == PacketID::MovePlayer))
        event.mCancelled = true;
}

void Freecam::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;
    auto ci = ClientInstance::get();
    if (!ci) return;

    if (mShouldDisable) {
        return;
    }

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

        glm::vec2 calc = MathUtils::getMotion(player->getActorRotationComponent()->mYaw, mSpeed.mValue / 10);
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

void Freecam::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!ClientInstance::get()->getLevelRenderer() || !player) return;
    
    AABB aabb = mOldAABB;

    auto drawList = ImGui::GetBackgroundDrawList();
    auto themeColor = ColorUtils::getThemedColor(0);

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
        if (player && player->isValid() && player->mContext.mRegistry)
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
