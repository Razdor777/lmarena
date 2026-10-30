//
// Diagnostics — см. Diagnostics.hpp.
//

#include "Diagnostics.hpp"

#include <Windows.h>

#include <atomic>
#include <string>
#include <vector>

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/FeatureManager.hpp>
#include <Features/Modules/ModuleManager.hpp>
#include <build_info.h>
#include <Hook/Detour.hpp>
#include <SDK/SigManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

namespace
{
    std::atomic<long long> gTicks{ 0 };
    std::atomic<long long> gRenders{ 0 };
    std::atomic<long long> gPacketsIn{ 0 };
    std::atomic<long long> gPacketsOut{ 0 };

    // GetTickCount64 в момент последнего события; 0 — не было ни разу.
    std::atomic<unsigned long long> gLastTick{ 0 };
    std::atomic<unsigned long long> gLastRender{ 0 };

}

void Diagnostics::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &Diagnostics::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,    &Diagnostics::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,  &Diagnostics::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &Diagnostics::onPacketOutEvent>(this);
}

void Diagnostics::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,  &Diagnostics::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &Diagnostics::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent,  &Diagnostics::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &Diagnostics::onPacketOutEvent>(this);
}

void Diagnostics::onBaseTickEvent(BaseTickEvent& event)
{
    ++gTicks;
    gLastTick.store(GetTickCount64(), std::memory_order_relaxed);
}

void Diagnostics::onRenderEvent(RenderEvent& event)
{
    ++gRenders;
    gLastRender.store(GetTickCount64(), std::memory_order_relaxed);
}

void Diagnostics::onPacketInEvent(PacketInEvent& event)
{
    ++gPacketsIn;
}

void Diagnostics::onPacketOutEvent(PacketOutEvent& event)
{
    ++gPacketsOut;
}

Diagnostics::Counters Diagnostics::counters()
{
    Counters out;
    out.ticks      = gTicks.load();
    out.renders    = gRenders.load();
    out.packetsIn  = gPacketsIn.load();
    out.packetsOut = gPacketsOut.load();
    return out;
}

void Diagnostics::resetCounters()
{
    gTicks = 0;
    gRenders = 0;
    gPacketsIn = 0;
    gPacketsOut = 0;
    gLastTick.store(0);
    gLastRender.store(0);
}

long long Diagnostics::millisSinceTick()
{
    const auto last = gLastTick.load();
    if (last == 0) return -1;
    return static_cast<long long>(GetTickCount64() - last);
}

long long Diagnostics::millisSinceRender()
{
    const auto last = gLastRender.load();
    if (last == 0) return -1;
    return static_cast<long long>(GetTickCount64() - last);
}

std::string Diagnostics::report(bool includeModules)
{
    if (!gFeatureManager || !gFeatureManager->mModuleManager) return {};

    std::string out;
    const auto put = [&out](const std::string& line)
    {
        ChatUtils::displayClientMessage(line);
        out += line;
        out += '\n';
    };

    // ── 1. Билд ───────────────────────────────────────────────────────────────
    put("§6[diag] §fБилд: §7" + std::string(SOLSTICE_VERSION) +
        "§f, собран §7" __DATE__ " " __TIME__);

    // ── 2. Сигнатуры ──────────────────────────────────────────────────────────
    size_t sigTotal = 0;
    std::vector<std::string> sigMissing;

    for (const auto& [name, address] : SigManager::mSigs)
    {
        ++sigTotal;
        if (address == 0) sigMissing.push_back(name);
    }

    put("§6[diag] §fСигнатуры: найдено §a" +
        std::to_string(sigTotal - sigMissing.size()) + "§f из " +
        std::to_string(sigTotal) + (sigMissing.empty() ? " §a(все)" : " §c(есть пропуски)"));

    if (!sigMissing.empty())
    {
        // Список длинный — режем по строкам, чтобы чат не превращался в кашу.
        std::string line;
        for (const auto& name : sigMissing)
        {
            if (line.size() + name.size() > 120)
            {
                put("§c  нет: §7" + line);
                line.clear();
            }
            if (!line.empty()) line += ", ";
            line += name;
        }
        if (!line.empty()) put("§c  нет: §7" + line);
    }

    // ── 3. Перехваты ──────────────────────────────────────────────────────────
    const auto detours = Detour::snapshot();

    size_t created = 0, enabled = 0;
    std::vector<std::string> dead;

    for (const auto& detour : detours)
    {
        if (detour.created) ++created;
        if (detour.enabled) ++enabled;
        if (!detour.created) dead.push_back(detour.name);
    }

    put("§6[diag] §fПерехваты: создано §a" + std::to_string(created) + "§f из " +
        std::to_string(detours.size()) + ", включено §a" + std::to_string(enabled));

    if (!dead.empty())
    {
        std::string line;
        for (const auto& name : dead)
        {
            if (line.size() + name.size() > 120)
            {
                put("§c  не создался: §7" + line);
                line.clear();
            }
            if (!line.empty()) line += ", ";
            line += name;
        }
        if (!line.empty()) put("§c  не создался: §7" + line);
    }

    // ── 4. События ────────────────────────────────────────────────────────────
    const auto c = counters();
    const auto sinceTick = millisSinceTick();
    const auto sinceRender = millisSinceRender();

    const auto ago = [](long long ms) -> std::string
    {
        if (ms < 0) return "ни разу";
        if (ms < 1000) return std::to_string(ms) + " мс назад";
        return std::to_string(ms / 1000) + " с назад";
    };

    put("§6[diag] §fТики: §a" + std::to_string(c.ticks) + "§f (последний: " + ago(sinceTick) + ")");
    put("§6[diag] §fРендер: §a" + std::to_string(c.renders) + "§f (последний: " + ago(sinceRender) + ")");
    put("§6[diag] §fПакеты: вход §a" + std::to_string(c.packetsIn) +
        "§f, выход §a" + std::to_string(c.packetsOut));

    if (c.ticks == 0)
        put("§c[diag] §fТик не приходил вообще: клиент не в мире, либо Actor::baseTick не перехвачен.");
    else if (sinceTick > 2000)
        put("§e[diag] §fТик молчит дольше 2 с — игра на паузе или загрузка.");

    if (c.renders == 0)
        put("§c[diag] §fРендер не приходил: ImGui-слой не подключился, всё визуальное работать не будет.");

    // ── 5. Модули ─────────────────────────────────────────────────────────────
    if (includeModules)
    {
        std::vector<std::string> running, stuck;

        for (const auto& module : gFeatureManager->mModuleManager->getModules())
        {
            if (module->mEnabled)
                running.push_back(module->mName);
            else if (module->mWantedState)
                stuck.push_back(module->mName);   // галочка есть, onEnable не звали
        }

        put("§6[diag] §fМодули работают: §a" + std::to_string(running.size()));

        if (!stuck.empty())
            put("§c[diag] §fВключены, но не запустились: §7" +
                [&stuck]
                {
                    std::string joined;
                    for (const auto& name : stuck)
                    {
                        if (!joined.empty()) joined += ", ";
                        joined += name;
                    }
                    return joined;
                }());
    }

    return out;
}
