#pragma once

#include <Features/Command/Command.hpp>

//
// .crit — диагностика «нативного» патча критов прямо в игре, без IDA.
//
// Зачем: RVA и байты от другой сборки не переносятся (проверено — совпадают
// только величины смещений, и те в 86 разных функциях). Поэтому искать надо в
// том бинарнике, который прямо сейчас запущен, и лучше всего это делает сам
// клиент.
//
//   .crit scan        — пройти по .text и показать кандидатов с отпечатком
//   .crit apply <n>   — применить патч к кандидату n (обратимо)
//   .crit off         — снять патч
//
class CritCommand : public Command
{
public:
    CritCommand() : Command("crit") {}

    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};
