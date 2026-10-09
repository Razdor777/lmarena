#pragma once
//
// Dexko — .htp command (HitParticles).
//
//   .htp            — текущий стиль + список
//   .htp <style>    — сменить стиль и включить модуль
//                     (blood / sparks / critical / hearts / fire / frost / soul / toxic)
//   .htp off        — выключить модуль
//

#include <Solstice.hpp>
#include <Features/FeatureManager.hpp>

class HtpCommand : public Command {
public:
    HtpCommand() : Command("htp") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
