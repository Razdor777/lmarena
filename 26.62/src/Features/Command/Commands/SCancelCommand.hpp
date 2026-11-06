#pragma once
#include <Features/Command/Command.hpp>

class SCancelCommand : public Command {
public:
    SCancelCommand() : Command("scancel") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override { return {}; }
    [[nodiscard]] std::string getDescription() const override { return "Cancel current schematic operation."; }
    [[nodiscard]] std::string getUsage() const override { return "Usage: .scancel"; }
};