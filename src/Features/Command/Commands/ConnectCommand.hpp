#pragma once

#include <Features/Command/Command.hpp>

//
// .connect <ip[:port]>  — то же, что нажать QuickConnect в ClickGui.
// Отдельная команда нужна, чтобы можно было подключиться из чата, не открывая меню.
//
class ConnectCommand : public Command
{
public:
    ConnectCommand() : Command("connect") {}

    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
