#pragma once

#include <Features/Command/Command.hpp>
#include <Features/Modules/Misc/SkinChanger.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/FileUtils.hpp>
#include <filesystem>

class SkinCommand : public Command {
public:
    SkinCommand() : Command("skin") {}

    void execute(const std::vector<std::string>& args) override {
        if (args.size() < 2) {
            ChatUtils::displayClientMessage("§6=== SkinChanger Help ===");
            ChatUtils::displayClientMessage("§fUsage: §b.skin <filename>");
            ChatUtils::displayClientMessage("§fExamples:");
            ChatUtils::displayClientMessage("§7  .skin 1.png");
            ChatUtils::displayClientMessage("§7  .skin custom_skin.png");
            ChatUtils::displayClientMessage("§fSkins folder: §7Solstice/Skins/");
            return;
        }

        // Собираем путь
        std::string filename = args[1];
        for (size_t i = 2; i < args.size(); i++) {
            filename += " " + args[i];
        }

        std::string fullPath;
        
        // Если указан полный путь (содержит :)
        if (filename.find(':') != std::string::npos) {
            fullPath = filename;
        } else {
            // Относительный путь - добавляем папку Skins
            fullPath = FileUtils::getSolsticeDir() + "Skins\\" + filename;
        }
        
        // Добавляем .png если нет расширения
        if (fullPath.find('.') == std::string::npos) {
            fullPath += ".png";
        }

        ChatUtils::displayClientMessage("§7Loading: " + fullPath);

        if (SkinChanger::loadSkinFromFile(fullPath)) {
            ChatUtils::displayClientMessage("§a[SkinChanger] §fSuccess!");
        } else {
            ChatUtils::displayClientMessage("§c[SkinChanger] §fFailed to load skin");
        }
    }

    [[nodiscard]] std::vector<std::string> getAliases() const override {
        return {"loadskin"};
    }

    [[nodiscard]] std::string getDescription() const override {
        return "Load a custom skin";
    }

    [[nodiscard]] std::string getUsage() const override {
        return ".skin <filename>";
    }
};