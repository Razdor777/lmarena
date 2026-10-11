#include "HtpCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/HitParticles.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace
{
    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Extra names people actually type. Everything is compared lowercased and
    // without spaces, so ".htp critical", ".htp crit" and ".htp 2" all work.
    struct Alias { const char* name; HitParticles::Style style; };

    const Alias kAliases[] = {
        { "blood",     HitParticles::Style::Blood    },
        { "bloody",    HitParticles::Style::Blood    },
        { "spark",     HitParticles::Style::Sparks   },
        { "sparks",    HitParticles::Style::Sparks   },
        { "crit",      HitParticles::Style::Critical },
        { "critical",  HitParticles::Style::Critical },
        { "heart",     HitParticles::Style::Hearts   },
        { "hearts",    HitParticles::Style::Hearts   },
        { "love",      HitParticles::Style::Hearts   },
        { "fire",      HitParticles::Style::Fire     },
        { "flame",     HitParticles::Style::Fire     },
        { "flames",    HitParticles::Style::Fire     },
        { "ember",     HitParticles::Style::Fire     },
        { "frost",     HitParticles::Style::Frost    },
        { "ice",       HitParticles::Style::Frost    },
        { "snow",      HitParticles::Style::Frost    },
        { "soul",      HitParticles::Style::Soul     },
        { "spirit",    HitParticles::Style::Soul     },
        { "toxic",     HitParticles::Style::Toxic    },
        { "acid",      HitParticles::Style::Toxic    },
        { "poison",    HitParticles::Style::Toxic    },
    };

    // -1 when nothing matched.
    int resolveStyle(HitParticles* hp, const std::string& raw)
    {
        const std::string wanted = lower(raw);

        // Plain index: .htp 3
        if (!wanted.empty() &&
            std::all_of(wanted.begin(), wanted.end(), [](unsigned char c) { return std::isdigit(c); }))
        {
            const int idx = std::atoi(wanted.c_str());
            if (idx >= 0 && idx < static_cast<int>(hp->mStyle.mValues.size()))
                return idx;
            return -1;
        }

        // Exact name from the setting itself.
        for (int i = 0; i < static_cast<int>(hp->mStyle.mValues.size()); i++)
        {
            if (lower(hp->mStyle.mValues[i]) == wanted)
                return i;
        }

        // Friendly aliases.
        for (const auto& alias : kAliases)
        {
            if (wanted == alias.name)
                return static_cast<int>(alias.style);
        }

        return -1;
    }

    std::string styleList(HitParticles* hp)
    {
        std::string list;
        for (size_t i = 0; i < hp->mStyle.mValues.size(); i++)
        {
            list += "§e" + std::to_string(i) + "§7=§f" + hp->mStyle.mValues[i];
            if (i + 1 < hp->mStyle.mValues.size()) list += "§7, ";
        }
        return list;
    }
}

void HtpCommand::execute(const std::vector<std::string>& args)
{
    auto* hp = gFeatureManager->mModuleManager->getModule<HitParticles>();
    if (!hp)
    {
        ChatUtils::displayClientMessage("§cHitParticles module not available.");
        return;
    }

    // ── No argument / status ─────────────────────────────────────────────────
    if (args.size() < 2 || lower(args[1]) == "info" || lower(args[1]) == "status")
    {
        ChatUtils::displayClientMessage(
            "§7HitParticles is " + std::string(hp->mEnabled ? "§aon" : "§coff") +
            "§7, style §6" + hp->styleName() +
            "§7, particles §6" + std::to_string(hp->particleCount()) + "§7.");
        ChatUtils::displayClientMessage("§7Styles: " + styleList(hp));
        ChatUtils::displayClientMessage("§7Usage: §e.htp <style|number>§7, §e.htp test§7, §e.htp list");
        return;
    }

    const std::string sub = lower(args[1]);

    // ── .htp list ────────────────────────────────────────────────────────────
    if (sub == "list")
    {
        ChatUtils::displayClientMessage("§7Styles: " + styleList(hp));
        return;
    }

    // ── .htp test — spawn a burst in front of the camera ─────────────────────
    // This is the quick way to see what a style looks like: no target needed, so
    // it also proves that changing the style really changed something.
    if (sub == "test")
    {
        if (!hp->mEnabled) hp->setEnabled(true);

        hp->clearParticles();
        if (hp->spawnTestBurst())
        {
            ChatUtils::displayClientMessage(
                "§aTest burst fired with style §6" + hp->styleName() + "§a.");
        }
        else
        {
            ChatUtils::displayClientMessage(
                "§cNo local player yet — test burst skipped (style §6" + hp->styleName() + "§c).");
        }
        return;
    }

    // ── .htp <style|number> ──────────────────────────────────────────────────
    const int idx = resolveStyle(hp, args[1]);
    if (idx < 0)
    {
        ChatUtils::displayClientMessage("§cUnknown style §6" + args[1] + "§c.");
        ChatUtils::displayClientMessage("§7Styles: " + styleList(hp));
        return;
    }

    hp->mStyle.setValue(static_cast<HitParticles::Style>(idx));

    // The detour/listener setup only exists while the module is on.
    if (!hp->mEnabled) hp->setEnabled(true);

    // Leftovers of the previous style would hide the change until they expire.
    hp->clearParticles();

    ChatUtils::displayClientMessage(
        "§aHitParticles style set to §6" + hp->mStyle.mValues[static_cast<size_t>(idx)] +
        "§a. §7Check it with §e.htp test§7.");
}

std::vector<std::string> HtpCommand::getAliases() const
{
    return { "hitparticles", "hitp" };
}

std::string HtpCommand::getDescription() const
{
    return "Changes the HitParticles burst style.";
}

std::string HtpCommand::getUsage() const
{
    return "Usage: .htp <style|number|test|list>";
}
