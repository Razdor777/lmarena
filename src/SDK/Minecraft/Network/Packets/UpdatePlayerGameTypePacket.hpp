#pragma once

#include "Packet.hpp"
#include "SetPlayerGameTypePacket.hpp"

class UpdatePlayerGameTypePacket : public Packet {
public:
    static inline PacketID ID = PacketID::UpdatePlayerGameType;
    
    GameType mPlayerGameType;       // this+0x30
    int64_t mTargetPlayerUniqueId;  // this+0x38
};