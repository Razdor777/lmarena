#pragma once

#include "../Command.hpp"

class FakeChatCommand : public Command {
public:
    FakeChatCommand();
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
    static std::string replaceColorCodes(const std::string& input);
};