#pragma once
//
// Created by vastrakai on 11/9/2024.
//
#include <Features/Command/Command.hpp>

#include <atomic>
#include <cstdint>

class TeleportCommand : public Command
{
public:
    // ==== ported from the 1.21.44 master ====
    // Когда последний раз выполняли .tp (NOW, мс): OreMinerV2 не путает ручной
    // телепорт с тем, что сервер сам перекинул игрока.
    static inline std::atomic<uint64_t> sLastUse{0};
    // ==== end of ported block ====

    TeleportCommand() : Command("teleport") {}
    void execute(const std::vector<std::string>& args) override;
    [[nodiscard]] std::vector<std::string> getAliases() const override;
    [[nodiscard]] std::string getDescription() const override;
    [[nodiscard]] std::string getUsage() const override;
};