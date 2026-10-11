//
// HtpCommand — HitParticles style switcher.
//
// `.htp` never touches the particle vectors directly (they belong to the render
// thread): it changes the setting and asks the module for a preview, which the
// next frame renders right under the crosshair. So every call visibly does
// something, with or without a landed hit.
//
// Accepted forms:
//   .htp                        -> current style + usage
//   .htp <style>                -> set style (name, shorthand or number)
//   .htp style <style>          -> same, for scripts
//   .htp set <style>            -> same
//   .htp list                   -> list every style
//   .htp test                   -> show the current style
//   .htp test <style>           -> try a style without saving it first
//   .htp color <mode>           -> Style Color / Theme Color / Custom
//   .htp color #RRGGBB          -> custom colour + switch to Custom
//   .htp on | off | toggle      -> module state
//

#include "HtpCommand.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/HitParticles.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace
{
    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    std::string joinFrom(const std::vector<std::string>& values, const std::vector<int>& indices)
    {
        std::string out;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            out += "§e" + values[indices[i]];
            if (i + 1 < indices.size()) out += "§7, ";
        }
        return out;
    }

    // Resolves user input against a list of names.
    //   >= 0 -> index, -1 -> nothing matched, -2 -> several matches (ambiguity,
    //   the candidates are written into `ambiguous`).
    int resolveName(const std::vector<std::string>& values, const std::string& raw, std::string& ambiguous)
    {
        const std::string wanted = lower(raw);
        if (wanted.empty()) return -1;

        // 1. exact match
        for (size_t i = 0; i < values.size(); ++i)
            if (lower(values[i]) == wanted) return static_cast<int>(i);

        // 2. plain number, 1-based: `.htp 3` == `.htp hearts`
        const bool numeric = std::all_of(wanted.begin(), wanted.end(),
                                         [](unsigned char c) { return std::isdigit(c) != 0; });
        if (numeric)
        {
            const int idx = std::atoi(wanted.c_str()) - 1;
            if (idx >= 0 && idx < static_cast<int>(values.size())) return idx;
            return -1;
        }

        // 3. shorthand: unique prefix (`.htp sp`) or substring (`.htp heart`)
        std::vector<int> matches;
        for (size_t i = 0; i < values.size(); ++i)
        {
            const std::string v = lower(values[i]);
            if (v.starts_with(wanted) || v.find(wanted) != std::string::npos)
                matches.push_back(static_cast<int>(i));
        }

        if (matches.size() == 1) return matches[0];
        if (matches.size() > 1)
        {
            ambiguous = joinFrom(values, matches);
            return -2;
        }
        return -1;
    }

    int styleIndex(const HitParticles* hp)
    {
        const int count = static_cast<int>(hp->mStyle.mValues.size());
        if (count <= 0) return 0;
        return std::clamp(hp->mStyle.as<int>(), 0, count - 1);
    }

    std::string styleName(const HitParticles* hp)
    {
        if (hp->mStyle.mValues.empty()) return "?";
        return hp->mStyle.mValues[static_cast<size_t>(styleIndex(hp))];
    }

    std::string colorModeName(const HitParticles* hp)
    {
        if (hp->mColorMode.mValues.empty()) return "?";
        const int idx = std::clamp(hp->mColorMode.as<int>(), 0,
                                   static_cast<int>(hp->mColorMode.mValues.size()) - 1);
        return hp->mColorMode.mValues[static_cast<size_t>(idx)];
    }

    void printStatus(const HitParticles* hp)
    {
        ChatUtils::displayClientMessage("§7HitParticles: " + std::string(hp->mEnabled ? "§aon" : "§coff") +
                                        "§7, style: §6" + styleName(hp) +
                                        "§7, color: §6" + colorModeName(hp));
    }

    void printStyles(const HitParticles* hp)
    {
        std::string list;
        for (size_t i = 0; i < hp->mStyle.mValues.size(); ++i)
        {
            list += "§e" + std::to_string(i + 1) + ".§6" + hp->mStyle.mValues[i];
            if (i + 1 < hp->mStyle.mValues.size()) list += "§7, ";
        }
        ChatUtils::displayClientMessage("§7Styles: " + list);
    }

    void printUsage()
    {
        ChatUtils::displayClientMessage("§7Usage: §e.htp <style> §7| §e.htp test [style] §7| §e.htp color <mode/#hex>");
        ChatUtils::displayClientMessage("§7Example: §e.htp hearts§7, §e.htp fire§7, §e.htp 4§7, §e.htp test frost");
    }

    // #RRGGBB / RRGGBB / #RRGGBBAA -> floats, returns false when malformed.
    bool parseHexColor(const std::string& raw, float out[4])
    {
        std::string hex = raw;
        if (!hex.empty() && hex[0] == '#') hex.erase(0, 1);
        if (hex.size() != 6 && hex.size() != 8) return false;
        if (!std::all_of(hex.begin(), hex.end(), [](unsigned char c) { return std::isxdigit(c) != 0; }))
            return false;

        auto byteAt = [&hex](size_t i) {
            return static_cast<float>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16)) / 255.f;
        };

        out[0] = byteAt(0);
        out[1] = byteAt(2);
        out[2] = byteAt(4);
        out[3] = hex.size() == 8 ? byteAt(6) : 1.f;
        return true;
    }
}

void HtpCommand::execute(const std::vector<std::string>& args)
{
    auto* hp = gFeatureManager->mModuleManager->getModule<HitParticles>();
    if (!hp)
    {
        ChatUtils::displayClientMessage("§cHitParticles is not available in this build.");

        // No ClickGui and no module = nothing else can change the style, so make
        // the reason explicit instead of silently doing nothing.
        ChatUtils::displayClientMessage("§7Load the Dexko build that includes the module, then try §e.htp list§7.");
        return;
    }

    // Bare `.htp` — report where we are and how to move on.
    if (args.size() < 2)
    {
        printStatus(hp);
        printStyles(hp);
        printUsage();
        return;
    }

    const std::string action = lower(args[1]);

    if (action == "help")
    {
        printUsage();
        return;
    }

    if (action == "list" || action == "styles")
    {
        printStatus(hp);
        printStyles(hp);
        return;
    }

    if (action == "on" || action == "enable")
    {
        hp->setEnabled(true);
        ChatUtils::displayClientMessage("§aHitParticles §7enabled (style: §6" + styleName(hp) + "§7).");
        hp->requestPreview();
        return;
    }

    if (action == "off" || action == "disable")
    {
        hp->setEnabled(false);
        ChatUtils::displayClientMessage("§cHitParticles §7disabled.");
        return;
    }

    if (action == "toggle")
    {
        hp->toggle();
        ChatUtils::displayClientMessage(std::string("§7HitParticles is now ") +
                                        (hp->mWantedState ? "§aon" : "§coff") + "§7.");
        if (hp->mWantedState) hp->requestPreview();
        return;
    }

    // `.htp color ...`
    if (action == "color" || action == "colour")
    {
        if (args.size() < 3)
        {
            ChatUtils::displayClientMessage("§7Current color mode: §6" + colorModeName(hp));
            std::string modes;
            for (size_t i = 0; i < hp->mColorMode.mValues.size(); ++i)
            {
                modes += "§e" + hp->mColorMode.mValues[i];
                if (i + 1 < hp->mColorMode.mValues.size()) modes += "§7, ";
            }
            ChatUtils::displayClientMessage("§7Modes: " + modes + " §7(or a hex colour, e.g. §e#ff3355§7)");
            return;
        }

        float rgba[4];
        if (parseHexColor(args[2], rgba))
        {
            hp->mColorMode.setValue(HitParticles::ColorMode::Custom);
            hp->mCustomColor.setValue(rgba[0], rgba[1], rgba[2], rgba[3]);
            ChatUtils::displayClientMessage("§aParticle color: §6" + args[2] + " §7(color mode: §6Custom§7)");
            if (!hp->mEnabled) hp->setEnabled(true);
            hp->requestPreview();
            return;
        }

        std::string ambiguous;
        const int idx = resolveName(hp->mColorMode.mValues, args[2], ambiguous);
        if (idx == -2)
        {
            ChatUtils::displayClientMessage("§cAmbiguous color mode §6" + args[2] + "§c: §e" + ambiguous);
            return;
        }
        if (idx < 0)
        {
            ChatUtils::displayClientMessage("§cUnknown color mode §6" + args[2] + "§c.");
            return;
        }

        hp->mColorMode.setValue(static_cast<HitParticles::ColorMode>(idx));
        ChatUtils::displayClientMessage("§aParticle color mode: §6" + hp->mColorMode.mValues[static_cast<size_t>(idx)]);
        if (!hp->mEnabled) hp->setEnabled(true);
        hp->requestPreview();
        return;
    }

    // `.htp test [style]` — preview without touching the saved style.
    if (action == "test" || action == "preview" || action == "show")
    {
        if (args.size() >= 3)
        {
            std::string ambiguous;
            const int idx = resolveName(hp->mStyle.mValues, args[2], ambiguous);
            if (idx == -2)
            {
                ChatUtils::displayClientMessage("§cAmbiguous style §6" + args[2] + "§c: §e" + ambiguous);
                return;
            }
            if (idx < 0)
            {
                ChatUtils::displayClientMessage("§cUnknown style §6" + args[2] + "§c.");
                printStyles(hp);
                return;
            }
            hp->mStyle.setValue(static_cast<HitParticles::Style>(idx));
        }

        if (!hp->mEnabled) hp->setEnabled(true);

        hp->requestPreview();
        printStatus(hp);
        ChatUtils::displayClientMessage("§7Preview spawns right in front of you (look straight ahead).");
        return;
    }

    // `.htp style <style>` / `.htp set <style>`
    std::string wanted = args[1];
    if ((action == "style" || action == "set" || action == "s") && args.size() >= 3)
        wanted = args[2];

    std::string ambiguous;
    const int idx = resolveName(hp->mStyle.mValues, wanted, ambiguous);

    if (idx == -2)
    {
        ChatUtils::displayClientMessage("§cAmbiguous style §6" + wanted + "§c: §e" + ambiguous);
        return;
    }

    if (idx < 0)
    {
        ChatUtils::displayClientMessage("§cUnknown style §6" + wanted + "§c.");
        printStyles(hp);
        printUsage();
        return;
    }

    hp->mStyle.setValue(static_cast<HitParticles::Style>(idx));

    // A style with the module switched off is invisible — turn it on, that is
    // what the user obviously wanted.
    if (!hp->mEnabled) hp->setEnabled(true);

    ChatUtils::displayClientMessage("§aHitParticles style: §6" + hp->mStyle.mValues[static_cast<size_t>(idx)] + "§7.");

    if (colorModeName(hp) == "Custom")
    {
        // The bug report that started this: with a custom colour every style
        // looks identical, so say it out loud instead of looking broken.
        ChatUtils::displayClientMessage("§7Note: color mode is §6Custom§7, so all styles share one colour. "
                                        "Use §e.htp color style§7 for the style palette.");
    }

    hp->requestPreview();
    ChatUtils::displayClientMessage("§7Showing it in front of you — hit something to see it in combat.");
}

std::vector<std::string> HtpCommand::getAliases() const
{
    return { "hitparticles", "hp" };
}

std::string HtpCommand::getDescription() const
{
    return "Changes the HitParticles burst style (also shows a preview).";
}

std::string HtpCommand::getUsage() const
{
    return "Usage: .htp <style/list/test/color/on/off> (styles: blood, sparks, critical, hearts, fire, frost, soul, toxic)";
}
