#pragma once
//
// UpdatePlayerGameTypePacket — сервер сообщает клиенту, что у какого-то игрока
// сменился режим игры (в отличие от SetPlayerGameType, который приходит про
// локального игрока). Нужен SpectatorDetector: по нему видно, кто ушёл в
// спектатор.
//
// Раскладка как в базе 1.21.44: mPlayerGameType на this+0x30,
// mTargetPlayerUniqueId на this+0x38. Пакет не разбирается клиентом целиком —
// SpectatorDetector читает только эти два поля, — поэтому здесь достаточно
// объявления; требует проверки на бинарнике 26.62.
//

// GameType объявлен в SetPlayerGameTypePacket.hpp — второго определения быть
// не должно, иначе enum конфликтует.
#include "SetPlayerGameTypePacket.hpp"

class UpdatePlayerGameTypePacket : public Packet {
public:
    static inline PacketID ID = PacketID::UpdatePlayerGameType;

    GameType mPlayerGameType;       // this+0x30
    int64_t mTargetPlayerUniqueId;  // this+0x38
};
