#pragma once
//
// Dexko — .reach command.
//
//   .reach            — текущий статус
//   .reach <3.0-7.0>  — поставить combat reach и включить модуль
//   .reach off        — выключить модуль
//

#include <Solstice.hpp>
#include <Features/FeatureManager.hpp>

class ReachCommand : public Command {
public:
    ReachCommand() : Command("reach") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
