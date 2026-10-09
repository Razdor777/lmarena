//
// Created by vastrakai on 7/1/2024.
//

#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>

#include <Features/Modules/Visual/Interface.hpp>
#include <Hook/Hooks/RenderHooks/ActorRenderDispatcherHook.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Network/Packets/Packet.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <Utils/FontHelper.hpp>

#include <SDK/Minecraft/Actor/SyncedPlayerMovementSettings.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/World/Level.hpp>

template <typename T>
std::string combine(T t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

template <typename T, typename... Args>
std::string combine(T t, Args... args)
{
    std::stringstream ss;
    ss << t << combine(args...);
    return ss.str();
}

float pYaw;
float pOldYaw;

float pHeadYaw;
float pOldHeadYaw;

float pPitch;
float pOldPitch;

float pBodyYaw;
float pOldBodyYaw;

float pLerpedYaw;
float pLerpedHeadYaw;
float pLerpedPitch;
float pLerpedBodyYaw;

bool usingPaip = false;

// Returns the FontHelper key string for the currently selected FontType.
// Mntsb is the only text font the client ships with, so this is a fixed key.
static std::string fontTypeToKey(Interface::FontType)
{
    return "mntsb";
}

void Interface::onEnable()
{
    FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
}

void Interface::onDisable()
{
}

void Interface::onModuleStateChange(ModuleStateChangeEvent& event)
{
    if (event.mModule == this)
    {
        event.setCancelled(true);
    }
}

void Interface::onRenderEvent(RenderEvent& event)
{
    // Apply font changes immediately; Interface is always enabled, so relying
    // on onEnable alone made the selector appear broken until a restart.
    static int lastFont = -1;
    if (lastFont != mFont.as<int>()) {
        lastFont = mFont.as<int>();
        FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
    }

    auto player = ClientInstance::get()->getLocalPlayer();
    static bool lastPlayerState = false;

    if (player && !lastPlayerState)
    {
        usingPaip = false;
    }

    static constexpr float LERP_SPEED = 20.f;
    float deltaTime = ImGui::GetIO().DeltaTime;

    float yaw = MathUtils::wrap(pLerpedYaw, pYaw - 180, pYaw + 180);
    float headYaw = MathUtils::wrap(pLerpedHeadYaw, pHeadYaw - 180, pHeadYaw + 180);
    float pitch = pLerpedPitch;
    float bodyYaw = MathUtils::wrap(pLerpedBodyYaw, pBodyYaw - 180, pBodyYaw + 180);

    float preLerpedYaw = MathUtils::lerp(yaw, pYaw, deltaTime * LERP_SPEED);
    float preLerpedHeadYaw = MathUtils::lerp(headYaw, pHeadYaw, deltaTime * LERP_SPEED);
    float preLerpedPitch = MathUtils::lerp(pitch, pPitch, deltaTime * LERP_SPEED);
    float preLerpedBodyYaw = MathUtils::lerp(bodyYaw, pBodyYaw, deltaTime * LERP_SPEED);

    pLerpedYaw = MathUtils::wrap(pLerpedYaw, preLerpedYaw - 180, preLerpedYaw + 180);
    pLerpedHeadYaw = MathUtils::wrap(pLerpedHeadYaw, preLerpedHeadYaw - 180, preLerpedHeadYaw + 180);
    pLerpedBodyYaw = MathUtils::wrap(pLerpedBodyYaw, preLerpedBodyYaw - 180, preLerpedBodyYaw + 180);

    pLerpedYaw = MathUtils::lerp(preLerpedYaw, pLerpedYaw, deltaTime * LERP_SPEED);
    pLerpedHeadYaw = MathUtils::lerp(preLerpedHeadYaw, pLerpedHeadYaw, deltaTime * LERP_SPEED);
    pLerpedPitch = MathUtils::lerp(preLerpedPitch, pLerpedPitch, deltaTime * LERP_SPEED);
    pLerpedBodyYaw = MathUtils::lerp(preLerpedBodyYaw, pLerpedBodyYaw, deltaTime * LERP_SPEED);

}

void Interface::onActorRenderEvent(ActorRenderEvent& event)
{
    if (event.isCancelled()) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    if (event.mEntity != player) return;
    if (*event.mPos == glm::vec3(0.f, 0.f, 0.f) && *event.mRot == glm::vec2(0.f, 0.f)) return;

    bool firstPerson = ClientInstance::get()->getOptions()->mThirdPerson->value == 0;
    if (firstPerson && !player->getFlag<RenderCameraComponent>()) return;


    const auto actorRotations = event.mEntity->getActorRotationComponent();
    const auto headRotations = event.mEntity->getActorHeadRotationComponent();
    const auto bodyRotations = event.mEntity->getMobBodyRotationComponent();
    if (!actorRotations || !headRotations || !bodyRotations) return;

    float realOldPitch = actorRotations->mOldPitch;
    float realPitch = actorRotations->mPitch;
    float realHeadRot = headRotations->mHeadRot;
    float realOldHeadRot = headRotations->mOldHeadRot;
    float realBodyYaw = bodyRotations->yBodyRot;
    float realOldBodyYaw = bodyRotations->yOldBodyRot;

    actorRotations->mOldPitch = pLerpedPitch;
    actorRotations->mPitch = pLerpedPitch;
    headRotations->mHeadRot = pLerpedHeadYaw;
    headRotations->mOldHeadRot = pLerpedHeadYaw;
    bodyRotations->yBodyRot = pLerpedBodyYaw;
    bodyRotations->yOldBodyRot = pLerpedBodyYaw;

    auto original = event.mDetour->getOriginal<&ActorRenderDispatcherHook::render>();
    original(event._this, event.mEntityRenderContext, event.mEntity, event.mCameraTargetPos, event.mPos, event.mRot, event.mIgnoreLighting);
    event.cancel();

    actorRotations->mOldPitch = realOldPitch;
    actorRotations->mPitch = realPitch;
    headRotations->mHeadRot = realHeadRot;
    headRotations->mOldHeadRot = realOldHeadRot;
    bodyRotations->yBodyRot = realBodyYaw;
    bodyRotations->yOldBodyRot = realOldBodyYaw;
}

void Interface::onDrawImageEvent(DrawImageEvent& event)
{
    if (!mSlotEasing.mValue) return;
    // Wait for ImGui to be initialized
    if (!ImGui::GetCurrentContext()) return;

    static glm::vec2 hotbarPos = {};
    auto path = event.mTexture->mTexture->mFilePath.c_str();

    // textures/ui/selected_hotbar_slot
    // If the selected hotbar slot is being drawn
    if (strcmp(path, "textures/ui/selected_hotbar_slot") == 0)
    {
        float deltaTime = ImGui::GetIO().DeltaTime;
        if (hotbarPos.x == 0 || hotbarPos.y == 0) hotbarPos = *event.mPos;
        hotbarPos.x = MathUtils::lerp(hotbarPos.x, event.mPos->x, deltaTime * mSlotEasingSpeed.mValue);
        hotbarPos.y = event.mPos->y;
        *event.mPos = hotbarPos;
    }
}


void Interface::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();

    BodyYaw::updateRenderAngles(player, pYaw);
    pOldBodyYaw = pBodyYaw;
    pBodyYaw = BodyYaw::bodyYaw;

    // Apply font setting on change
    static FontType lastFont = (FontType)-1;
    if (mFont.mValue != lastFont) {
        lastFont = mFont.mValue;
        FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
    }
}

void Interface::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() == PacketID::PlayerAuthInput) usingPaip = true;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto level = player->getLevel();
    if (!level) return;
    auto moveSettings = level->getPlayerMovementSettings();
    if (!moveSettings) return;
    bool isServerAuthoritative = usingPaip;

    if (event.mPacket->getId() == PacketID::PlayerAuthInput && isServerAuthoritative)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();

        pOldYaw = pYaw;
        pOldPitch = pPitch;
        pOldBodyYaw = pBodyYaw;
        pYaw = paip->mRot.y;
        pPitch = paip->mRot.x;
        pHeadYaw = paip->mYHeadRot;
    }
    else if (event.mPacket->getId() == PacketID::MovePlayer && !isServerAuthoritative)
    {
        auto mpp = event.getPacket<MovePlayerPacket>();
        pOldYaw = pYaw;
        pOldPitch = pPitch;
        pOldBodyYaw = pBodyYaw;
        pYaw = mpp->mRot.y;
        pPitch = mpp->mRot.x;
        pHeadYaw = mpp->mYHeadRot;
    }
}
