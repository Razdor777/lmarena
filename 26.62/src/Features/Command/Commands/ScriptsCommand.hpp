#pragma once
//
// ScriptsCommand — manages the Luau scripts that live in
// %LOCALAPPDATA%\...\Solstice\Scripts\{Modules,Commands}
//

class ScriptsCommand : public Command {
public:
    ScriptsCommand() : Command("scripts") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
