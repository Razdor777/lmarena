#pragma once
#include <Features/Command/Command.hpp>

// .htp — controls the HitParticles module from chat.
//
//   .htp                 -> current style / module state
//   .htp list            -> every style with its number
//   .htp test            -> fire a burst in front of you right now
//   .htp <style|number>  -> switch the style (turns the module on)
class HtpCommand : public Command {
public:
    HtpCommand() : Command("htp") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
