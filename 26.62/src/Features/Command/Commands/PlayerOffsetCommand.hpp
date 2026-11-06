#pragma once
#include <Features/Command/Command.hpp>

class PlayerOffsetCommand : public Command {
public:
    PlayerOffsetCommand() : Command("playeroffset") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override { return {}; }
    [[nodiscard]] std::string getDescription() const override { return "Устанавливает цель для PlayerPositionOffset"; }
    [[nodiscard]] std::string getUsage() const override { return ".playeroffset <set/clear/get> [имя]"; }
};