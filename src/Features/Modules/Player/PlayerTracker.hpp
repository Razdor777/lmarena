#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Network/PacketID.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Utils/GameUtils/CPSTracker.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

class PlayerTracker : public ModuleBase<PlayerTracker> {
public:
    BoolSetting mDebugPackets = BoolSetting("Debug Packets",
        "Log text/title packets to chat", false);

    PlayerTracker() : ModuleBase("PlayerTracker",
        "Tracks player CPS and health",
        ModuleCategory::Misc, 0, true)
    {
        addSetting(&mDebugPackets);
        mNames = {
            {Lowercase, "playertracker"},
            {LowercaseSpaced, "player tracker"},
            {Normal, "PlayerTracker"},
            {NormalSpaced, "Player Tracker"}
        };
    }

    void onEnable() override {
        gFeatureManager->mDispatcher->listen<BaseTickEvent, &PlayerTracker::onBaseTick>(this);
        gFeatureManager->mDispatcher->listen<PacketInEvent, &PlayerTracker::onPacketIn>(this);
    }

    void onDisable() override {
        gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PlayerTracker::onBaseTick>(this);
        gFeatureManager->mDispatcher->deafen<PacketInEvent, &PlayerTracker::onPacketIn>(this);
        CPSTracker::getInstance().clear();
    }

    void onBaseTick(BaseTickEvent& event) {
        auto* ci = ClientInstance::get();
        auto* player = ci->getLocalPlayer();
        if (!player) return;

        auto* level = player->getLevel();
        if (!level) return;

        auto actors = level->getRuntimeActorList();

        // CPS Tracking
        for (auto* actor : actors) {
            if (!actor || !actor->isPlayer() || actor == player) continue;
            CPSTracker::getInstance().updatePlayer(
                actor->getRuntimeID(),
                actor->isSwinging(),
                actor->getSwingProgress()
            );
        }

        // Track кого атакуем
        if (player->isSwinging()) {
            auto* hitResult = level->getHitResult();
            if (hitResult && hitResult->mType == HitType::ENTITY) {
                for (auto* actor : actors) {
                    if (!actor || !actor->isPlayer() || actor == player) continue;
                    float dist = actor->distanceTo(*player->getPos());
                    if (dist < 7.f) {
                        std::string name = actor->getNameTag();
                        size_t nl = name.find('\n');
                        if (nl != std::string::npos) name = name.substr(0, nl);
                        if (!name.empty()) {
                            HealthTracker::getInstance().setLastAttacked(name);
                        }
                    }
                }
            }
        }
    }

    void onPacketIn(PacketInEvent& event) {
        // TextPacket
        if (event.mPacket->getId() == PacketID::Text) {
            auto pkt = event.getPacket<TextPacket>();

            if (mDebugPackets.mValue) {
                ChatUtils::displayClientMessage(
                    "§7[PT] §eText §ftype=§a{} §fauthor='§b{}§f' msg='§d{}§f'",
                    (int)pkt->mType, pkt->mAuthor, pkt->mMessage);
            }

            // Парсим HP из не-чат сообщений
            if (pkt->mType != TextPacketType::Chat &&
                pkt->mType != TextPacketType::Whisper)
            {
                std::string target = HealthTracker::getInstance().getLastAttacked();
                if (!target.empty()) {
                    bool ok = HealthTracker::getInstance().parseHealthText(
                        pkt->mMessage, target);
                    if (ok && mDebugPackets.mValue) {
                        float hp, maxHp;
                        HealthTracker::getInstance().getHealth(target, hp, maxHp);
                        ChatUtils::displayClientMessage(
                            "§7[PT] §aParsed HP §ffor '§b{}§f': §c{:.1f}§f/§c{:.1f}",
                            target, hp, maxHp);
                    }
                }
            }
        }

        // SetTitle (ActionBar)
        if (event.mPacket->getId() == PacketID::SetTitle) {
            auto* raw = reinterpret_cast<uint8_t*>(event.mPacket.get());
            int titleType = *reinterpret_cast<int*>(raw + 0x30);

            if (mDebugPackets.mValue) {
                try {
                    std::string* text = reinterpret_cast<std::string*>(raw + 0x38);
                    ChatUtils::displayClientMessage(
                        "§7[PT] §eTitle §ftype=§a{} §ftext='§d{}§f'",
                        titleType, *text);
                } catch (...) {
                    ChatUtils::displayClientMessage(
                        "§7[PT] §eTitle §ftype=§a{} §c(read failed)", titleType);
                }
            }

            if (titleType == 4 || titleType == 2 || titleType == 3) {
                try {
                    std::string* text = reinterpret_cast<std::string*>(raw + 0x38);
                    std::string target = HealthTracker::getInstance().getLastAttacked();
                    if (!target.empty()) {
                        bool ok = HealthTracker::getInstance().parseHealthText(*text, target);
                        if (ok && mDebugPackets.mValue) {
                            float hp, maxHp;
                            HealthTracker::getInstance().getHealth(target, hp, maxHp);
                            ChatUtils::displayClientMessage(
                                "§7[PT] §aTitle HP §ffor '§b{}§f': §c{:.1f}§f/§c{:.1f}",
                                target, hp, maxHp);
                        }
                    }
                } catch (...) {}
            }
        }
    }
};