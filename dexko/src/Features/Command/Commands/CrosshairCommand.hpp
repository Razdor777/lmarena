#pragma once
#include <Features/Command/Command.hpp>

class CrosshairCommand : public Command {
public:
    CrosshairCommand() : Command("crosshair") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
