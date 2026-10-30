//
// AccountManager — см. комментарий в AccountManager.hpp.
//

#include "AccountManager.hpp"

#include <chrono>
#include <string>

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Modules/Misc/QuickConnect.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

namespace
{
    int64_t nowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    bool isValidNick(const std::string& nick)
    {
        if (nick.empty() || nick.size() > 16) return false;

        for (const char c : nick)
        {
            const auto u = static_cast<unsigned char>(c);
            if (u < 0x20 || u > 0x7E) return false;      // только печатный ASCII
            if (c == static_cast<char>(0xC2)) return false; // «§» — начало цветового кода
        }

        return true;
    }
}

std::string AccountManager::activeNick() const
{
    switch (mProfile.mValue)
    {
        case Profile::One:   return mNick1.mValue;
        case Profile::Two:   return mNick2.mValue;
        case Profile::Three: return mNick3.mValue;
        case Profile::Four:  return mNick4.mValue;
        default:             return {};
    }
}

void AccountManager::onEnable()
{
    gFeatureManager->mDispatcher->listen<ConnectionRequestEvent, &AccountManager::onConnectionRequestEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,         &AccountManager::onBaseTickEvent>(this);

    // Профиль мог быть уже выбран до включения — чтобы при первом же входе
    // ничего не «переподключать» зря, считаем его применённым.
    mAppliedNick = activeNick();
}

void AccountManager::onDisable()
{
    gFeatureManager->mDispatcher->deafen<ConnectionRequestEvent, &AccountManager::onConnectionRequestEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,         &AccountManager::onBaseTickEvent>(this);
}

void AccountManager::onConnectionRequestEvent(ConnectionRequestEvent& event)
{
    auto nick = activeNick();

    if (!isValidNick(nick))
    {
        // Ничего не подменяем: пустой/битый профиль не должен ломать вход.
        if (!nick.empty())
            ChatUtils::displayClientMessage("§c[AccountManager] §fНик не подходит: нужно 1–16 печатных символов без цветовых кодов.");
        return;
    }

    if (!event.mThirdPartyName) return;

    *event.mThirdPartyName = nick;
    mAppliedNick = nick;

    ChatUtils::displayClientMessage("§a[AccountManager] §fВхожу как §6" + nick + "§f.");
}

void AccountManager::onBaseTickEvent(BaseTickEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    auto* localPlayer = ci->getLocalPlayer();
    if (!localPlayer) return;                      // не в мире — переподключаться некуда
    if (event.mActor && event.mActor != localPlayer) return;

    const auto nick = activeNick();
    if (!isValidNick(nick)) return;
    if (nick == mAppliedNick) return;              // уже этот ник

    // Профиль переключили, пока мы в мире. Ник меняется только на новом входе,
    // поэтому либо сразу переподключаемся, либо ждём следующего захода.
    if (!mReconnect.mValue)
    {
        mAppliedNick = nick;                       // сообщать больше не надо
        ChatUtils::displayClientMessage(
            "§e[AccountManager] §fНик §6" + nick + "§f применится при следующем входе на сервер.");
        return;
    }

    const int64_t now = nowMs();
    if (now - mLastApplyAttempt < 5000) return;    // не спамим подключениями
    mLastApplyAttempt = now;
    mAppliedNick      = nick;

    const auto lastAddress = QuickConnect::lastAddress();
    if (lastAddress.empty() || !QuickConnect::isReady())
    {
        ChatUtils::displayClientMessage(
            "§e[AccountManager] §fНик §6" + nick + "§f применится при следующем входе: "
            "пока не знаю адрес последнего сервера.");
        return;
    }

    ChatUtils::displayClientMessage(
        "§a[AccountManager] §fМеняю ник на §6" + nick + "§f — переподключаюсь к §6" + lastAddress + "§f.");
    QuickConnect::join(lastAddress);
}
