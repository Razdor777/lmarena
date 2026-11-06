#include "PlayerLogger.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

void PlayerLogger::onEnable() {
    mKnownPlayers.clear();
    mPlayerLog.clear();
    mFirstScan = true;
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PlayerLogger::onBaseTickEvent>(this);
}

void PlayerLogger::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PlayerLogger::onBaseTickEvent>(this);
}

void PlayerLogger::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto level = player->getLevel();
    if (!level) return;

    // Get all current players
    auto actors = level->getRuntimeActorList();
    std::unordered_set<std::string> currentPlayers;

    for (Actor* actor : actors) {
        if (!actor || !actor->isValid() || !actor->isPlayer()) continue;
        if (actor == player) continue;

        std::string name = actor->getNameTag();
        if (name.empty()) continue;

        currentPlayers.insert(name);
    }

    if (mFirstScan) {
        // First scan — just record everyone, no notifications
        mKnownPlayers = currentPlayers;
        mFirstScan = false;
        spdlog::info("[PlayerLogger] Initial scan: {} players", currentPlayers.size());
        return;
    }

    // Detect joins
    for (const auto& name : currentPlayers) {
        if (mKnownPlayers.find(name) == mKnownPlayers.end()) {
            // New player joined!
            if (mNotifyJoin.mValue) {
                NotifyUtils::notify("§a+ §f" + name + " §7joined",
                    3.f, Notification::Type::Info);
            }
            if (mLogToChat.mValue) {
                ChatUtils::displayClientMessage("§8[§aPlayerLogger§8] §a+ §f" + name + " §7joined the game");
            }
            spdlog::info("[PlayerLogger] JOIN: {}", name);

            PlayerLogEntry entry;
            entry.name = name;
            entry.joinTime = NOW;
            entry.isOnline = true;
            mPlayerLog[name] = entry;
        }
    }

    // Detect leaves
    for (const auto& name : mKnownPlayers) {
        if (currentPlayers.find(name) == currentPlayers.end()) {
            // Player left!
            if (mNotifyLeave.mValue) {
                NotifyUtils::notify("§c- §f" + name + " §7left",
                    3.f, Notification::Type::Warning);
            }
            if (mLogToChat.mValue) {
                ChatUtils::displayClientMessage("§8[§cPlayerLogger§8] §c- §f" + name + " §7left the game");
            }
            spdlog::info("[PlayerLogger] LEAVE: {}", name);

            if (mPlayerLog.find(name) != mPlayerLog.end()) {
                mPlayerLog[name].leaveTime = NOW;
                mPlayerLog[name].isOnline = false;
            }
        }
    }

    // Update known players
    mKnownPlayers = currentPlayers;
}
