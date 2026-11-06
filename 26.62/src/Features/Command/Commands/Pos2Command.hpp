#pragma once
//
// Pos2Command - Set position 2 for AutoPatrol
//

#include <Features/Command/Command.hpp>

class Pos2Command : public Command
{
public:
    Pos2Command() : Command("pos2") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};