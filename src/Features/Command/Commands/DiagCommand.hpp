#pragma once

#include <Features/Command/Command.hpp>

//
// .diag — состояние клиента в этом запуске, без IDA и без консоли.
//
//   .diag           — билд, сигнатуры, перехваты, события, модули
//   .diag reset     — обнулить счётчики событий
//
// Нужен, чтобы на вопрос «почему модуль не работает» отвечать фактами:
// не нашлась сигнатура / не встал перехват / не идёт тик / модуль включён, но
// onEnable никто не вызывал.
//
class DiagCommand : public Command
{
public:
    DiagCommand() : Command("diag") {}

    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
