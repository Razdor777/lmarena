//
// .crit — см. CritCommand.hpp.
//

#include "CritCommand.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include <libhat.hpp>

#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/CritPatchUtils.hpp>

namespace
{
    // Результат последнего скана: по нему работает .crit apply <n>.
    std::vector<critpatch::Candidate> gLastScan;
    uintptr_t gTextBase = 0;

    void say(const std::string& text)
    {
        ChatUtils::displayClientMessage(text);
    }

    std::string hex(uintptr_t value)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "0x%llX", static_cast<unsigned long long>(value));
        return buffer;
    }

    void doScan()
    {
        auto text = hat::process::get_process_module().get_section_data(".text");
        if (text.empty())
        {
            say("§c[Crit] §fНе смог прочитать .text игры.");
            return;
        }

        gTextBase = reinterpret_cast<uintptr_t>(text.data());

        const auto begin = std::chrono::steady_clock::now();
        gLastScan = critpatch::scan(reinterpret_cast<const uint8_t*>(text.data()), text.size(), gTextBase, 6);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - begin).count();

        say("§6[Crit] §f.text " + std::to_string(text.size() / (1024 * 1024)) +
            " МБ, проход " + std::to_string(ms) + " мс, кандидатов: §6" +
            std::to_string(gLastScan.size()));

        if (gLastScan.empty())
        {
            say("§e[Crit] §fЦепочек «несколько проверок в один адрес» не нашлось вообще. "
                "В этой сборке криты считаются иначе — нужен листинг Player::attack.");
            return;
        }

        for (size_t i = 0; i < gLastScan.size(); ++i)
        {
            const critpatch::Candidate& candidate = gLastScan[i];
            const int percent = static_cast<int>(critpatch::similarity(candidate) * 100.f);

            say("§6#" + std::to_string(i + 1) + " §fпереходов: §6" +
                std::to_string(candidate.sites.size()) +
                "§f, вход " + hex(candidate.anchor - gTextBase) +
                ", крит " + hex(candidate.critBlock - gTextBase) +
                ", схожесть §6" + std::to_string(percent) + "%");

            say("    §7" + critpatch::fingerprint(candidate));
        }

        say("§7[Crit] §fЭталон 1.26.51: §7-0x192 -0x17E -0xD9 -0xC9 -0x29 -0x17 -0xA");
        say("§7[Crit] §fПрименить: §6.crit apply <номер>§7, снять: §6.crit off");
    }

    void doApply(const std::string& indexText)
    {
        if (gLastScan.empty())
        {
            say("§c[Crit] §fСначала §6.crit scan§f.");
            return;
        }

        int index = 0;
        try
        {
            index = std::stoi(indexText);
        }
        catch (...)
        {
            say("§c[Crit] §fНомер должен быть числом: §6.crit apply 1");
            return;
        }

        if (index < 1 || static_cast<size_t>(index) > gLastScan.size())
        {
            say("§c[Crit] §fНет такого номера, в списке §6" + std::to_string(gLastScan.size()) + "§f.");
            return;
        }

        critpatch::Plan plan;
        if (!critpatch::makePlan(gLastScan[index - 1], plan))
        {
            say("§c[Crit] §fИз этого кандидата патч не собрать (мало переходов).");
            return;
        }

        if (critpatch::isApplied())
        {
            say("§e[Crit] §fУже применён другой патч — сначала §6.crit off§f.");
            return;
        }

        if (!critpatch::apply(plan))
        {
            say("§c[Crit] §fЗаписать не удалось: поток стоит в месте записи или не сошлись байты.");
            return;
        }

        say("§a[Crit] §fПатч №" + std::to_string(index) + " применён (" +
            std::to_string(plan.sites.size()) + " переходов). Проверь удар по мобу.");

        if (critpatch::similarity(gLastScan[index - 1]) < 0.5f)
        {
            say("§e[Crit] §fРаскладка не как в эталоне — если игра вылетит или поведёт себя странно, "
                "сразу §6.crit off§f.");
        }
    }

    void doOff()
    {
        if (!critpatch::isApplied())
        {
            say("§7[Crit] §fПатч не применён.");
            return;
        }

        critpatch::restore();
        say("§a[Crit] §fПатч снят, байты возвращены.");
    }
}

void CritCommand::execute(const std::vector<std::string>& args)
{
    const std::string action = (args.size() >= 2) ? args[1] : "scan";

    if (action == "scan")        doScan();
    else if (action == "off")    doOff();
    else if (action == "apply")
    {
        if (args.size() < 3)
        {
            say("§c" + getUsage());
            return;
        }
        doApply(args[2]);
    }
    else
    {
        say("§c" + getUsage());
    }
}

std::vector<std::string> CritCommand::getAliases() const
{
    return { "critscan" };
}

std::string CritCommand::getDescription() const
{
    return "Scans .text for the native critical-hit checks and patches them";
}

std::string CritCommand::getUsage() const
{
    return "Usage: .crit scan | .crit apply <n> | .crit off";
}
