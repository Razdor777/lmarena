#pragma once

#include <Features/Command/Command.hpp>

class SPasteCommand : public Command
{
public:
    SPasteCommand() : Command("spaste") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};