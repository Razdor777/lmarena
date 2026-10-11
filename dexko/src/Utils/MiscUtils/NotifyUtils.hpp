#pragma once
#include <Dexko.hpp>
#include <Features/Events/NotifyEvent.hpp>

class NotifyUtils {
public:
    static void notify(std::string message, float duration, Notification::Type type);
};