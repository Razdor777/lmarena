#include "Spammer.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/PacketUtils.hpp>
#include <Utils/StringUtils.hpp>

Spammer::Spammer(): ModuleBase("Spammer", "Automatically sends a chat message in specified delay", ModuleCategory::Misc, 0, false)
{
    addSetting(&mMode);
    addSetting(&mDelayMs);
    addSetting(&mAvoidKicks);
    addSetting(&mAddedDelay);

    mNames = {
        {Lowercase, "spammer"},
        {LowercaseSpaced, "spammer"},
        {Normal, "Spammer"},
        {NormalSpaced, "Spammer"}
    };
}

void Spammer::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Spammer::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &Spammer::onPacketInEvent>(this);
    mLastMessageSent = NOW;
}

void Spammer::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Spammer::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &Spammer::onPacketInEvent>(this);
}

std::string Spammer::getNearestPlayerName(Actor* player)
{
    if (!player) return "";

    auto actors = ActorUtils::getActorList(true, true);
    Actor* nearest = nullptr;
    float bestDist = FLT_MAX;

    for (auto* actor : actors)
    {
        if (!actor || actor == player) continue;
        if (actor->isDead()) continue;

        float dist = 0.f;
        try {
            dist = actor->distanceTo(player);
        } catch (...) { continue; }

        if (dist < bestDist)
        {
            bestDist = dist;
            nearest = actor;
        }
    }

    if (!nearest) return "";
    try {
        return nearest->getRawName();
    } catch (...) {
        return "";
    }
}

void Spammer::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    if (NOW <= mLastMessageSent + mDelayMs.mValue) return;

    std::string entry = mCurrentMessage;

    // Замена <near> на имя ближайшего игрока
    while (StringUtils::contains(entry, "<near>"))
    {
        std::string nearest = getNearestPlayerName(player);
        entry = StringUtils::replace(entry, "<near>", nearest);
    }

    PacketUtils::sendChatMessage(entry);
    mLastMessageSent = NOW;
}

void Spammer::onPacketInEvent(PacketInEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (event.mPacket->getId() == PacketID::Text)
    {
        std::string localName = player->getRawName();

        auto tp = event.getPacket<TextPacket>();
        if (mAvoidKicks.mValue && StringUtils::containsIgnoreCase(tp->mMessage, localName))
            mLastMessageSent = NOW + mAddedDelay.mValue;
    }
}