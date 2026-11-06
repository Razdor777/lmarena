#include "FakeChatCommand.hpp"
#include "../../Modules/Misc/FakeChat.hpp"
#include <Utils/GameUtils/ChatUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

FakeChatCommand::FakeChatCommand() : Command("fakechat") {}

std::vector<std::string> FakeChatCommand::getAliases() const { return {"fc"}; }
std::string FakeChatCommand::getDescription() const { return "Fake chat messages"; }
std::string FakeChatCommand::getUsage() const { return "<said|send|now|raw|list|queue|cancel|debug>"; }

std::string FakeChatCommand::replaceColorCodes(const std::string& input) {
    std::string result;
    result.reserve(input.size() + 32);
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '&' && i + 1 < input.size()) {
            char next = std::tolower(static_cast<unsigned char>(input[i + 1]));
            if ((next >= '0' && next <= '9') || (next >= 'a' && next <= 'f') ||
                next == 'r' || next == 'l' || next == 'o' ||
                next == 'k' || next == 'n' || next == 'm') {
                result += "\xC2\xA7";
                continue;
            }
        }
        result += input[i];
    }
    return result;
}

void FakeChatCommand::execute(const std::vector<std::string>& args) {
    auto* fc = FakeChat::get();
    if (!fc) {
        ChatUtils::displayClientMessage("§c[FakeChat] Module not found!");
        return;
    }
    if (!ClientInstance::get()->getLocalPlayer()) {
        ChatUtils::displayClientMessage("§cYou need to be in a world!");
        return;
    }

    // Авто-включение
    if (!fc->mEnabled) {
        fc->setEnabled(true);
        ChatUtils::displayClientMessage("§7[FakeChat] Auto-enabled");
    }

    // ════ HELP ════
    if (args.size() < 2) {
        ChatUtils::displayClientMessage("§6FakeChat §7— fake chat messages (only you see)");
        ChatUtils::displayClientMessage("");
        ChatUtils::displayClientMessage("§eStep 1: §fWait for person to write something");
        ChatUtils::displayClientMessage("§eStep 2: §f.fc said <name> <what they wrote>");
        ChatUtils::displayClientMessage("§eStep 3: §f.fc send <name> <sec> <fake msg>");
        ChatUtils::displayClientMessage("");
        ChatUtils::displayClientMessage("§eCommands:");
        ChatUtils::displayClientMessage("§f  .fc said <name> <text>       §7— save format");
        ChatUtils::displayClientMessage("§f  .fc send <name> <sec> <msg>  §7— delayed fake");
        ChatUtils::displayClientMessage("§f  .fc now <name> <msg>         §7— instant fake");
        ChatUtils::displayClientMessage("§f  .fc clone <src> <target>    §7— copy prefix, swap name");
        ChatUtils::displayClientMessage("§f  .fc raw <sec> <full msg>     §7— manual (&-colors)");
        ChatUtils::displayClientMessage("§f  .fc list / queue / cancel / debug");
        return;
    }

    std::string sub = args[1];
    for (auto& c : sub) c = std::tolower(static_cast<unsigned char>(c));

    // ════ SAID ════
    if (sub == "said") {
        if (args.size() < 4) {
            ChatUtils::displayClientMessage("§cUsage: .fc said <name> <what they said>");
            ChatUtils::displayClientMessage("§7Example: .fc said LX2213 камерамен топ");
            return;
        }

        std::string name = args[2];
        std::string saidText;
        for (size_t i = 3; i < args.size(); i++) {
            if (i > 3) saidText += " ";
            saidText += args[i];
        }

        fc->findPrefix(name, saidText);
        return;
    }

    // ════ SEND ════
    if (sub == "send") {
        if (args.size() < 5) {
            ChatUtils::displayClientMessage("§cUsage: .fc send <name> <seconds> <fake message>");
            ChatUtils::displayClientMessage("§7Example: .fc send LX2213 10 я читер лол");
            return;
        }

        std::string name = args[2];
        auto pit = fc->mPrefixes.find(name);
        if (pit == fc->mPrefixes.end()) {
            ChatUtils::displayClientMessage("§c[FakeChat] No prefix for §e" + name);
            ChatUtils::displayClientMessage("§7First do: .fc said " + name + " <what they said>");
            return;
        }

        float delay = 0;
        try { delay = std::stof(args[3]); }
        catch (...) {
            ChatUtils::displayClientMessage("§cInvalid delay: " + args[3]);
            return;
        }

        std::string msg;
        for (size_t i = 4; i < args.size(); i++) {
            if (i > 4) msg += " ";
            msg += args[i];
        }
        msg = replaceColorCodes(msg);

        std::string fullText = pit->second + msg;
        fc->schedule(fullText, delay);

        ChatUtils::displayClientMessage("§a[FakeChat] §fScheduled in §e" +
            std::to_string(static_cast<int>(delay)) + "s");
        ChatUtils::displayClientMessage("§7Preview: §r" + fullText);
        return;
    }

    // ════ NOW ════
    if (sub == "now") {
        if (args.size() < 4) {
            ChatUtils::displayClientMessage("§cUsage: .fc now <name> <fake message>");
            return;
        }

        std::string name = args[2];
        auto pit = fc->mPrefixes.find(name);
        if (pit == fc->mPrefixes.end()) {
            ChatUtils::displayClientMessage("§c[FakeChat] No prefix for §e" + name);
            ChatUtils::displayClientMessage("§7First do: .fc said " + name + " <what they said>");
            return;
        }

        std::string msg;
        for (size_t i = 3; i < args.size(); i++) {
            if (i > 3) msg += " ";
            msg += args[i];
        }
        msg = replaceColorCodes(msg);

        std::string fullText = pit->second + msg;
        ChatUtils::displayClientMessageRaw(fullText);
        return;
    }

    // ════ RAW ════
    if (sub == "raw") {
        if (args.size() < 4) {
            ChatUtils::displayClientMessage("§cUsage: .fc raw <seconds> <full formatted message>");
            ChatUtils::displayClientMessage("§7Use & for colors: &b = blue, &c = red, etc");
            return;
        }

        float delay = 0;
        try { delay = std::stof(args[2]); }
        catch (...) {
            ChatUtils::displayClientMessage("§cInvalid delay: " + args[2]);
            return;
        }

        std::string msg;
        for (size_t i = 3; i < args.size(); i++) {
            if (i > 3) msg += " ";
            msg += args[i];
        }
        msg = replaceColorCodes(msg);

        fc->schedule(msg, delay);
        ChatUtils::displayClientMessage("§a[FakeChat] §fRaw scheduled in §e" +
            std::to_string(static_cast<int>(delay)) + "s");
        return;
    }

    // ════ CLONE ════
    if (sub == "clone" || sub == "swap" || sub == "copy") {
        if (args.size() < 4) {
            ChatUtils::displayClientMessage("§cUsage: .fc clone <source_name> <target_name>");
            ChatUtils::displayClientMessage("");
            ChatUtils::displayClientMessage("§7Copies the prefix from source and replaces");
            ChatUtils::displayClientMessage("§7the name with target. Target can be offline.");
            ChatUtils::displayClientMessage("");
            ChatUtils::displayClientMessage("§eExample:");
            ChatUtils::displayClientMessage("§f  §7Someone with same rank writes in chat:");
            ChatUtils::displayClientMessage("§f  §7[Player] RandomGuy228 » hello");
            ChatUtils::displayClientMessage("§f  .fc said RandomGuy228 hello");
            ChatUtils::displayClientMessage("§f  .fc clone RandomGuy228 TargetPlayer");
            ChatUtils::displayClientMessage("§f  .fc now TargetPlayer i love hacking");
            return;
        }

        std::string source = args[2];
        std::string target = args[3];
        fc->clonePrefix(source, target);
        return;
    }

    // ════ DEBUG ════
    if (sub == "debug") {
        fc->mDebugMode = !fc->mDebugMode;
        ChatUtils::displayClientMessage("§a[FakeChat] §fDebug: " +
            std::string(fc->mDebugMode ? "§aON" : "§cOFF"));
        return;
    }

    // ════ LIST ════
    if (sub == "list") {
        if (fc->mPrefixes.empty()) {
            ChatUtils::displayClientMessage("§7[FakeChat] No prefixes saved");
            return;
        }
        ChatUtils::displayClientMessage("§6[FakeChat] §fSaved prefixes:");
        for (const auto& [name, prefix] : fc->mPrefixes) {
            std::string pre = prefix;
            if (pre.length() > 70) pre = pre.substr(0, 70) + "...";
            ChatUtils::displayClientMessage("  §e" + name + " §7= §r" + pre);
        }
        ChatUtils::displayClientMessage("§7History: §f" +
            std::to_string(fc->mHistory.size()) + " messages stored");
        return;
    }

    // ════ QUEUE ════
    if (sub == "queue") {
        if (fc->mQueue.empty()) {
            ChatUtils::displayClientMessage("§7[FakeChat] Queue empty");
            return;
        }
        auto now = std::chrono::steady_clock::now();
        ChatUtils::displayClientMessage("§6[FakeChat] §fQueue (" +
            std::to_string(fc->mQueue.size()) + "):");
        int idx = 1;
        for (const auto& m : fc->mQueue) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                m.sendAt - now).count();
            int sec = std::max(0, static_cast<int>(ms / 1000));
            ChatUtils::displayClientMessage("  §7#" + std::to_string(idx++) +
                " §7in §f" + std::to_string(sec) + "s");
        }
        return;
    }

    // ════ CANCEL ════
    if (sub == "cancel" || sub == "clear") {
        fc->cancelAll();
        return;
    }

    ChatUtils::displayClientMessage("§cUnknown: " + sub + " §7— .fc for help");
}