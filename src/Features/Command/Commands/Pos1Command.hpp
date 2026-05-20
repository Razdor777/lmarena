#pragma once
//
// Pos1Command - Set position 1 for AutoPatrol
//

#include <Features/Command/Command.hpp>

class Pos1Command : public Command
{
public:
    Pos1Command() : Command("pos1") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};