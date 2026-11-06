#pragma once
//
// SPos1Command - Set position 1 for SchematicBuilder
//

#include <Features/Command/Command.hpp>

class SPos1Command : public Command
{
public:
    SPos1Command() : Command("spos1") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};