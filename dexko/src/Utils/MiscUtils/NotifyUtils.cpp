#include "NotifyUtils.hpp"

#include <Features/FeatureManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <utility>

#include "spdlog/spdlog.h"

void NotifyUtils::notify(std::string message, float duration, Notification::Type type)
{
    const auto notification = Notification(std::move(message), type, duration);
    spdlog::info("[NotifyUtils] Notified: {}", notification.mMessage);

    // В Dexko нет отрисовки тостов (она жила в GUI-части), поэтому уведомление
    // уходит в чат — иначе оно пропадало бы бесследно.
    const char* prefix = "§b";
    switch (type)
    {
        case Notification::Type::Info:    prefix = "§b"; break;
        case Notification::Type::Warning: prefix = "§e"; break;
        case Notification::Type::Error:   prefix = "§c"; break;
    }
    ChatUtils::displayClientMessage(prefix + notification.mMessage);

    // Событие оставляем: любой модуль может на него подписаться.
    auto holder = nes::make_holder<NotifyEvent>(notification);
    gFeatureManager->mDispatcher->trigger<NotifyEvent>(holder);
}
