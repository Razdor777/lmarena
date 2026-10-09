#pragma once
#include <Features/Command/Command.hpp>

class HtpCommand : public Command {
public:
    HtpCommand() : Command("htp") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
