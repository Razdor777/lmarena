#pragma once
//
// Tiny command kept from the 1.21.44 build so the ported Aimbot module has the
// same `.aimbot` / `.ab` toggle it used to have.
//

#include <Solstice.hpp>
#include <Features/FeatureManager.hpp>

class AimbotCommand : public Command {
public:
    AimbotCommand() : Command("aimbot") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
