// SBuildCommand.hpp
#pragma once
#include <Features/Command/Command.hpp>

class SBuildCommand : public Command {
public:
    SBuildCommand() : Command("sbuild") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override { return {}; }
    [[nodiscard]] std::string getDescription() const override { return "Start building the pasted schematic."; }
    [[nodiscard]] std::string getUsage() const override { return "Usage: .sbuild"; }
};