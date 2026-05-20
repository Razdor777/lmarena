#pragma once
//
// SPos2Command - Set position 2 for SchematicBuilder
//

#include <Features/Command/Command.hpp>

class SPos2Command : public Command
{
public:
    SPos2Command() : Command("spos2") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};